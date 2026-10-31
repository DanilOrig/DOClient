/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "stt.h"

#include <base/fs.h>
#include <base/log.h>
#include <base/math.h>
#include <base/str.h>
#include <base/time.h>

#include <engine/graphics.h>
#include <engine/keys.h>
#include <engine/shared/config.h>
#include <engine/shared/json.h>
#include <engine/storage.h>
#include <engine/textrender.h>

#include <game/client/gameclient.h>
#include <game/client/ui.h>

#include <SDL.h>
#include <windows.h>

#include <cctype>
#include <cmath>
#include <cstring>
#include <vector>

// Offline speech recognition with Vosk (Apache License 2.0), loaded dynamically.
// The library and the models live in data/stt next to the client, nothing is sent over the network.
namespace
{
using SttModel = struct SttModel;
using SttRecognizer = struct SttRecognizer;

using FnModelNew = SttModel *(*)(const char *);
using FnModelFree = void (*)(SttModel *);
using FnRecognizerNew = SttRecognizer *(*)(SttModel *, float);
using FnRecognizerFree = void (*)(SttRecognizer *);
using FnAcceptWaveform = int (*)(SttRecognizer *, const char *, int);
using FnResult = const char *(*)(SttRecognizer *);
using FnFinalResult = const char *(*)(SttRecognizer *);
using FnReset = int (*)(SttRecognizer *);

// Windows APIs that take ANSI paths break on folders with non-ASCII characters
// (e.g. "OneDrive\??????? ????"), so the library is loaded with wide strings and
// the model path is passed as an ASCII 8.3 path.
static bool SttUtf8ToWide(const char *pUtf8, wchar_t *pOut, int OutSize)
{
	return MultiByteToWideChar(CP_UTF8, 0, pUtf8, -1, pOut, OutSize) > 0;
}

static bool SttShortPath(const char *pUtf8Path, char *pOut, int OutSize)
{
	wchar_t aWide[IO_MAX_PATH_LENGTH];
	if(!SttUtf8ToWide(pUtf8Path, aWide, IO_MAX_PATH_LENGTH))
		return false;
	wchar_t aShort[IO_MAX_PATH_LENGTH];
	if(GetShortPathNameW(aWide, aShort, IO_MAX_PATH_LENGTH) == 0)
		return false;
	return WideCharToMultiByte(CP_UTF8, 0, aShort, -1, pOut, OutSize, nullptr, nullptr) > 0;
}
class CVosk
{
public:
	HMODULE m_hLib = nullptr;
	FnModelNew m_pfnModelNew = nullptr;
	FnModelFree m_pfnModelFree = nullptr;
	FnRecognizerNew m_pfnRecognizerNew = nullptr;
	FnRecognizerFree m_pfnRecognizerFree = nullptr;
	FnAcceptWaveform m_pfnAcceptWaveform = nullptr;
	FnResult m_pfnResult = nullptr;
	FnFinalResult m_pfnFinalResult = nullptr;
	FnReset m_pfnReset = nullptr;

	bool Load(const char *pLibPath, const char *pLibDir)
	{
		char aShortDir[IO_MAX_PATH_LENGTH];
		char aShortLib[IO_MAX_PATH_LENGTH];
		if(!SttShortPath(pLibDir, aShortDir, sizeof(aShortDir)))
			str_copy(aShortDir, pLibDir, sizeof(aShortDir));
		if(!SttShortPath(pLibPath, aShortLib, sizeof(aShortLib)))
			str_copy(aShortLib, pLibPath, sizeof(aShortLib));

		wchar_t aWideDir[IO_MAX_PATH_LENGTH];
		wchar_t aWideLib[IO_MAX_PATH_LENGTH];
		if(SttUtf8ToWide(aShortDir, aWideDir, IO_MAX_PATH_LENGTH) && SttUtf8ToWide(aShortLib, aWideLib, IO_MAX_PATH_LENGTH))
		{
			SetDllDirectoryW(aWideDir);
			m_hLib = LoadLibraryW(aWideLib);
			SetDllDirectoryW(nullptr);
		}
		if(m_hLib == nullptr)
			m_hLib = LoadLibraryA(pLibPath);
		if(m_hLib == nullptr && str_comp(aShortLib, pLibPath) != 0)
			m_hLib = LoadLibraryA(aShortLib);		if(m_hLib == nullptr)
		{
			log_error("stt", "Failed to load '%s'", pLibPath);
			return false;
		}

		m_pfnModelNew = (FnModelNew)GetProcAddress(m_hLib, "vosk_model_new");
		m_pfnModelFree = (FnModelFree)GetProcAddress(m_hLib, "vosk_model_free");
		m_pfnRecognizerNew = (FnRecognizerNew)GetProcAddress(m_hLib, "vosk_recognizer_new");
		m_pfnRecognizerFree = (FnRecognizerFree)GetProcAddress(m_hLib, "vosk_recognizer_free");
		m_pfnAcceptWaveform = (FnAcceptWaveform)GetProcAddress(m_hLib, "vosk_recognizer_accept_waveform");
		m_pfnResult = (FnResult)GetProcAddress(m_hLib, "vosk_recognizer_result");
		m_pfnFinalResult = (FnFinalResult)GetProcAddress(m_hLib, "vosk_recognizer_final_result");
		m_pfnReset = (FnReset)GetProcAddress(m_hLib, "vosk_recognizer_reset");

		const bool Ok = m_pfnModelNew != nullptr && m_pfnModelFree != nullptr && m_pfnRecognizerNew != nullptr &&
				m_pfnRecognizerFree != nullptr && m_pfnAcceptWaveform != nullptr && m_pfnResult != nullptr &&
				m_pfnFinalResult != nullptr && m_pfnReset != nullptr;
		if(!Ok)
		{
			log_error("stt", "Vosk library is incomplete");
		}
		return Ok;
	}

	void Unload()
	{
		if(m_hLib != nullptr)
		{
			FreeLibrary(m_hLib);
			m_hLib = nullptr;
		}
	}
};

CVosk gs_Vosk;
SttModel *gs_pModel = nullptr;
SttRecognizer *gs_pRecognizer = nullptr;

void AppendRecognizedText(const char *pJsonText, std::string &Out)
{
	if(pJsonText == nullptr || pJsonText[0] == '\0')
		return;
	json_settings Settings{};
	char aError[256];
	json_value *pJson = JsonParseEx(&Settings, pJsonText, str_length(pJsonText), aError);
	if(pJson == nullptr)
		return;
	if(pJson->type == json_object)
	{
		const json_value &Text = (*pJson)["text"];
		if(Text.type == json_string)
		{
			const char *pText = Text.u.string.ptr;
			if(pText[0] != '\0')
			{
				if(!Out.empty())
					Out += ' ';
				Out += pText;
			}
		}
	}
	json_value_free(pJson);
}

bool HasMeaningfulText(const char *pText)
{
	int Letters = 0;
	for(const char *p = pText; *p;)
	{
		const int Code = str_utf8_decode(&p);
		if(Code < 0)
			break;
		if((Code >= 'a' && Code <= 'z') || (Code >= 'A' && Code <= 'Z') || (Code >= 0x410 && Code <= 0x44F) || (Code >= 0x30 && Code <= 0x39))
			Letters++;
	}
	return Letters >= 2;
}
} // namespace

CStt::~CStt()
{
	m_Quit = true;
	m_StopRequest = true;
	m_ThreadCv.notify_all();
	if(m_Worker.joinable())
		m_Worker.join();
	if(gs_pRecognizer != nullptr && gs_Vosk.m_pfnRecognizerFree != nullptr)
		gs_Vosk.m_pfnRecognizerFree(gs_pRecognizer);
	if(gs_pModel != nullptr && gs_Vosk.m_pfnModelFree != nullptr)
		gs_Vosk.m_pfnModelFree(gs_pModel);
	gs_pRecognizer = nullptr;
	gs_pModel = nullptr;
	gs_Vosk.Unload();
}

void CStt::OnConsoleInit()
{
	Console()->Register("stt", "", CFGFLAG_CLIENT, [](IConsole::IResult *pResult, void *pUserData) {
		static_cast<CStt *>(pUserData)->ToggleRecording();
	}, this, "Toggle push-to-talk speech recognition");
}

void CStt::OnShutdown()
{
	m_Quit = true;
	m_StopRequest = true;
	m_ThreadCv.notify_all();
	if(m_Worker.joinable())
		m_Worker.join();
}

int CStt::ParseKey(const char *pName)
{
	if(pName == nullptr || pName[0] == '\0')
		return KEY_UNKNOWN;
	char aBuf[32];
	str_copy(aBuf, pName, sizeof(aBuf));
	for(char *p = aBuf; *p; p++)
		*p = (char)tolower((unsigned char)*p);

	if(str_length(aBuf) == 1)
	{
		const char c = aBuf[0];
		if(c >= 'a' && c <= 'z')
			return KEY_A + (c - 'a');
		switch(c)
		{
		case '1': return KEY_1;
		case '2': return KEY_2;
		case '3': return KEY_3;
		case '4': return KEY_4;
		case '5': return KEY_5;
		case '6': return KEY_6;
		case '7': return KEY_7;
		case '8': return KEY_8;
		case '9': return KEY_9;
		case '0': return KEY_0;
		default: break;
		}
	}

	if(str_comp(aBuf, "space") == 0)
		return KEY_SPACE;
	if(str_comp(aBuf, "tab") == 0)
		return KEY_TAB;
	if(str_comp(aBuf, "enter") == 0 || str_comp(aBuf, "return") == 0)
		return KEY_RETURN;
	if(str_comp(aBuf, "escape") == 0)
		return KEY_ESCAPE;
	if(str_comp(aBuf, "lshift") == 0 || str_comp(aBuf, "shift") == 0)
		return KEY_LSHIFT;
	if(str_comp(aBuf, "rshift") == 0)
		return KEY_RSHIFT;
	if(str_comp(aBuf, "lctrl") == 0 || str_comp(aBuf, "ctrl") == 0)
		return KEY_LCTRL;
	if(str_comp(aBuf, "rctrl") == 0)
		return KEY_RCTRL;
	if(str_comp(aBuf, "lalt") == 0 || str_comp(aBuf, "alt") == 0)
		return KEY_LALT;
	if(str_comp(aBuf, "ralt") == 0)
		return KEY_RALT;
	if(str_comp(aBuf, "mouse1") == 0)
		return KEY_MOUSE_1;
	if(str_comp(aBuf, "mouse2") == 0)
		return KEY_MOUSE_2;
	if(str_comp(aBuf, "mouse3") == 0)
		return KEY_MOUSE_3;
	if(aBuf[0] == 'f' && aBuf[1] >= '1' && aBuf[1] <= '9')
	{
		const int Num = str_toint(aBuf + 1);
		if(Num >= 1 && Num <= 12 && aBuf[2] == '\0')
			return KEY_F1 + (Num - 1);
	}
	return KEY_UNKNOWN;
}

int CStt::CurrentKey() const
{
	return ParseKey(g_Config.m_EcSttKey);
}

bool CStt::OnInput(const IInput::CEvent &Event)
{
	if(!g_Config.m_EcStt)
		return false;
	if(!(Event.m_Flags & IInput::FLAG_PRESS))
		return false;

	const int Key = ParseKey(g_Config.m_EcSttKey);
	if(Key == KEY_UNKNOWN || Event.m_Key != Key)
		return false;

	ToggleRecording();
	return true;
}

void CStt::ToggleRecording()
{
	if(!g_Config.m_EcStt)
		return;

	const int State = m_State.load();
	if(State == STT_IDLE)
		StartRecording();
	else if(State == STT_RECORDING)
		StopRecording();
}

void CStt::StartRecording()
{
	if(m_State.load() != STT_IDLE)
		return;
	if(!g_Config.m_EcStt)
		return;

	m_StopRequest = false;
	m_State = STT_RECORDING;

	{
		std::lock_guard<std::mutex> Lock(m_ThreadMutex);
		m_StartRequest = true;
	}
	m_ThreadCv.notify_one();

	if(!m_Worker.joinable())
		m_Worker = std::thread([this] { WorkerMain(); });
}

void CStt::StopRecording()
{
	if(m_State.load() != STT_RECORDING)
		return;
	m_StopRequest = true;
	m_State = STT_PROCESSING;
}

void CStt::WorkerMain()
{
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

	while(true)
	{
		{
			std::unique_lock<std::mutex> Lock(m_ThreadMutex);
			m_ThreadCv.wait(Lock, [this] { return m_StartRequest || m_Quit; });
			if(m_Quit)
				return;
			m_StartRequest = false;
		}
		RecordSession();
		m_State = STT_IDLE;
	}
}

void CStt::RecordSession()
{
	if(!g_Config.m_EcStt)
		return;

	char aBinDir[IO_MAX_PATH_LENGTH];
	char aLibPath[IO_MAX_PATH_LENGTH];
	Storage()->GetBinaryPathAbsolute("data/stt/bin", aBinDir, sizeof(aBinDir));
	Storage()->GetBinaryPathAbsolute("data/stt/bin/libvosk.dll", aLibPath, sizeof(aLibPath));

	if(gs_pModel == nullptr)
	{
		if(gs_Vosk.m_hLib == nullptr && !gs_Vosk.Load(aLibPath, aBinDir))
			return;

		char aModelDir[IO_MAX_PATH_LENGTH];
		Storage()->GetBinaryPathAbsolute(g_Config.m_EcSttLang == 1 ? "data/stt/vosk-model-small-en-us-0.15" : "data/stt/vosk-model-small-ru-0.22", aModelDir, sizeof(aModelDir));
		if(!fs_is_dir(aModelDir))
		{
			log_error("stt", "Speech model not found in '%s'", aModelDir);
			return;
		}
		char aModelDirShort[IO_MAX_PATH_LENGTH];
		const char *pModelDir = aModelDir;
		if(SttShortPath(aModelDir, aModelDirShort, sizeof(aModelDirShort)))
			pModelDir = aModelDirShort;
		gs_pModel = gs_Vosk.m_pfnModelNew(pModelDir);
		if(gs_pModel == nullptr && pModelDir == aModelDir)
		{
			// Last resort for file systems without 8.3 names: load the model
			// relative to its own directory, which keeps the paths ASCII.
			wchar_t aWideModel[IO_MAX_PATH_LENGTH];
			if(SttUtf8ToWide(aModelDir, aWideModel, IO_MAX_PATH_LENGTH))
			{
				wchar_t aOldDir[IO_MAX_PATH_LENGTH];
				if(GetCurrentDirectoryW(IO_MAX_PATH_LENGTH, aOldDir) > 0 && SetCurrentDirectoryW(aWideModel))
				{
					gs_pModel = gs_Vosk.m_pfnModelNew(".");
					SetCurrentDirectoryW(aOldDir);
				}
			}
		}
		if(gs_pModel == nullptr)
		{
			log_error("stt", "Failed to load speech model '%s'", aModelDir);
			return;
		}
	}

	if(gs_pRecognizer == nullptr)
	{
		gs_pRecognizer = gs_Vosk.m_pfnRecognizerNew(gs_pModel, 16000.0f);
		if(gs_pRecognizer == nullptr)
			return;
	}
	else
	{
		gs_Vosk.m_pfnReset(gs_pRecognizer);
	}

	SDL_AudioSpec Want{};
	Want.freq = 16000;
	Want.format = AUDIO_S16SYS;
	Want.channels = 1;
	Want.samples = 2048;
	Want.callback = nullptr;

	SDL_AudioSpec Have{};
	SDL_AudioDeviceID Device = SDL_OpenAudioDevice(nullptr, 1, &Want, &Have, 0);
	if(Device == 0)
	{
		Device = SDL_OpenAudioDevice(nullptr, 1, &Want, &Have, SDL_AUDIO_ALLOW_ANY_CHANGE);
	}
	if(Device == 0 || Have.freq != 16000 || Have.format != AUDIO_S16SYS || Have.channels < 1)
	{
		if(Device != 0)
			SDL_CloseAudioDevice(Device);
		log_error("stt", "Unsupported microphone format: %s", SDL_GetError());
		return;
	}

	std::string Recognized;
	std::vector<unsigned char> Buffer((size_t)Have.size * Have.channels * 2);
	SDL_PauseAudioDevice(Device, 0);

	while(!m_StopRequest && !m_Quit)
	{
		const Uint32 Read = SDL_DequeueAudio(Device, Buffer.data(), (Uint32)Buffer.size());
		if(Read == 0)
		{
			SDL_Delay(10);
			continue;
		}
		if(gs_Vosk.m_pfnAcceptWaveform(gs_pRecognizer, (const char *)Buffer.data(), (int)Read) != 0)
			AppendRecognizedText(gs_Vosk.m_pfnResult(gs_pRecognizer), Recognized);
	}

	SDL_CloseAudioDevice(Device);

	if(m_Quit)
		return;

	AppendRecognizedText(gs_Vosk.m_pfnFinalResult(gs_pRecognizer), Recognized);
	PushRecognizedText(Recognized.c_str());
	m_StopRequest = false;
}

void CStt::PushRecognizedText(const char *pText)
{
	if(pText == nullptr)
		return;

	char aText[MAX_CHAT_LENGTH];
	str_copy(aText, pText, sizeof(aText));
	str_utf8_trim_right(aText);

	const char *pTrimmed = str_utf8_skip_whitespaces(aText);
	if(pTrimmed != aText)
	{
		const size_t Len = str_length(pTrimmed);
		memmove(aText, pTrimmed, Len + 1);
	}

	// collapse repeated spaces and drop control characters
	char aClean[MAX_CHAT_LENGTH];
	size_t Out = 0;
	bool LastSpace = false;
	for(const char *p = aText; *p && Out + 1 < sizeof(aClean); p++)
	{
		const unsigned char c = (unsigned char)*p;
		if(c == ' ' || c == '\t' || c == '\r' || c == '\n')
		{
			if(!LastSpace && Out > 0)
				aClean[Out++] = ' ';
			LastSpace = true;
			continue;
		}
		LastSpace = false;
		aClean[Out++] = (char)c;
	}
	aClean[Out] = '\0';
	str_utf8_trim_right(aClean);

	if(aClean[0] == '\0' || !HasMeaningfulText(aClean))
		return;

	str_utf8_fix_truncation(aClean);

	std::lock_guard<std::mutex> Lock(m_TextMutex);
	m_PendingText = aClean;
	m_HasPending = true;
}

void CStt::OnRender()
{
	if(!g_Config.m_EcStt)
		return;

	if(m_HasPending)
	{
		char aText[MAX_CHAT_LENGTH];
		{
			std::lock_guard<std::mutex> Lock(m_TextMutex);
			str_copy(aText, m_PendingText.c_str(), sizeof(aText));
			m_HasPending = false;
			m_PendingText.clear();
		}

		const bool CanSend = Client()->State() == IClient::STATE_ONLINE &&
				     !GameClient()->m_Chat.IsActive() &&
				     !GameClient()->m_GameConsole.IsActive() &&
				     !GameClient()->m_Menus.IsActive();
		if(CanSend && aText[0] != '\0')
		{
			GameClient()->m_Chat.SendChatQueued(aText);
			m_FlashStart = time();
		}
	}

	const int State = m_State.load();
	const bool Recording = State == STT_RECORDING;
	const bool Processing = State == STT_PROCESSING;
	const bool Flash = m_FlashStart != 0 && time() < m_FlashStart + time_freq();
	if(!Recording && !Processing && !Flash)
		return;

	const float Height = 300.0f;
	const float Width = Height * Graphics()->ScreenAspect();
	Graphics()->MapScreenToSize(Width, Height);

	float Alpha = 0.7f;
	if(Recording)
		Alpha = 0.35f + 0.65f * (0.5f + 0.5f * std::sin((float)(time() % time_freq()) / (float)time_freq() * 12.0f));

	CUIRect Rect = {6.0f, 6.0f, 44.0f, 11.0f};
	Graphics()->TextureClear();
	Graphics()->DrawRect(Rect.x, Rect.y, Rect.w, Rect.h, ColorRGBA(0.0f, 0.0f, 0.0f, 0.35f), IGraphics::CORNER_ALL, 3.0f);

	ColorRGBA DotColor(0.3f, 1.0f, 0.3f, Alpha);
	if(Recording)
		DotColor = ColorRGBA(1.0f, 0.15f, 0.15f, Alpha);
	else if(Processing)
		DotColor = ColorRGBA(1.0f, 1.0f, 1.0f, Alpha);
	Graphics()->DrawRect(Rect.x + 3.0f, Rect.y + 3.0f, 5.0f, 5.0f, DotColor, IGraphics::CORNER_ALL, 2.5f);

	const char *pLabel = Recording ? "REC" : (Processing ? "..." : "SENT");
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.9f);
	TextRender()->Text(Rect.x + 11.0f, Rect.y + 2.0f, 6.5f, pLabel, -1.0f);
	TextRender()->TextColor(TextRender()->DefaultTextColor());
}
