/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include "chat.h"

#include <base/color.h>
#include <base/io.h>
#include <base/log.h>
#include <base/log_color.h>
#include <base/time.h>

#include <engine/editor.h>
#include <engine/graphics.h>
#include <engine/http.h>
#include <engine/keys.h>
#include <engine/shared/config.h>
#include <engine/shared/csv.h>
#include <engine/shared/json.h>
#include <engine/textrender.h>

#include <generated/protocol.h>
#include <generated/protocol7.h>

#include <game/client/animstate.h>
#include <game/client/components/censor.h>
#include <game/client/components/menus.h>
#include <game/client/components/scoreboard.h>
#include <game/client/components/skins.h>
#include <game/client/components/sounds.h>
#include <game/client/gameclient.h>
#include <game/localization.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iterator>
#include <string>

char CChat::ms_aDisplayText[MAX_CHAT_LENGTH] = "";

namespace
{
enum
{
	TRANSLATE_BACKEND_BING = 0,
	TRANSLATE_BACKEND_GOOGLE = 1,
};

struct STranslateLanguage
{
	const char *m_pCode;
	const char *m_pName;
};

constexpr STranslateLanguage s_aTranslateLanguages[] = {
	{"ru", "Russian"},
	{"en", "English"},
	{"de", "German"},
	{"fr", "French"},
	{"es", "Spanish"},
	{"pl", "Polish"},
	{"tr", "Turkish"},
	{"uk", "Ukrainian"},
	{"pt", "Portuguese"},
	{"it", "Italian"},
	{"zh-CN", "Chinese"},
	{"ja", "Japanese"},
};

int TranslateLanguageIndex(const char *pCode)
{
	for(int i = 0; i < (int)std::size(s_aTranslateLanguages); i++)
	{
		if(str_comp(pCode, s_aTranslateLanguages[i].m_pCode) == 0)
			return i;
	}
	return 0;
}

void TranslateUrlEncode(const char *pText, char *pOut, size_t Size)
{
	size_t Pos = 0;
	for(const unsigned char *p = (const unsigned char *)pText; *p && Pos + 4 < Size; ++p)
	{
		if(isalnum(*p) || *p == '-' || *p == '_' || *p == '.' || *p == '~')
		{
			pOut[Pos++] = (char)*p;
		}
		else
		{
			str_format(pOut + Pos, 4, "%%%02X", *p);
			Pos += 3;
		}
	}
	pOut[Pos] = '\0';
}

void TranslateStripApostrophes(char *pText)
{
	char *pWrite = pText;
	for(const char *pRead = pText; *pRead;)
	{
		if(*pRead == '\'')
		{
			++pRead;
			continue;
		}
		// U+2018 and U+2019 (typographic apostrophes)
		if((unsigned char)pRead[0] == 0xE2 && (unsigned char)pRead[1] == 0x80 && ((unsigned char)pRead[2] == 0x98 || (unsigned char)pRead[2] == 0x99))
		{
			pRead += 3;
			continue;
		}
		*pWrite++ = *pRead++;
	}
	*pWrite = '\0';
}

// Google translate response: [[["translated","original",...], ...], "en", ...]
bool ParseGoogleTranslation(const json_value *pRoot, char (&aOut)[MAX_CHAT_LENGTH], char (&aLang)[16])
{
	aOut[0] = '\0';
	aLang[0] = '\0';
	if(!pRoot || pRoot->type != json_array || json_array_length(pRoot) < 1)
		return false;
	const json_value *pDetected = json_array_get(pRoot, 2);
	if(pDetected && pDetected->type == json_string)
		str_copy(aLang, json_string_get(pDetected));
	const json_value *pSentences = json_array_get(pRoot, 0);
	if(!pSentences || pSentences->type != json_array)
		return false;
	for(int i = 0; i < json_array_length(pSentences); i++)
	{
		const json_value *pSentence = json_array_get(pSentences, i);
		if(!pSentence || pSentence->type != json_array)
			continue;
		const json_value *pPart = json_array_get(pSentence, 0);
		if(pPart && pPart->type == json_string)
			str_append(aOut, json_string_get(pPart));
	}
	return aOut[0] != '\0';
}

void TranslateJsonEscape(const char *pText, char *pOut, size_t Size)
{
	static const char s_aHex[] = "0123456789abcdef";
	size_t Pos = 0;
	for(const char *p = pText; *p && Pos + 7 < Size; ++p)
	{
		const unsigned char c = (unsigned char)*p;
		if(c == (unsigned char)'"' || c == (unsigned char)'\\')
		{
			pOut[Pos++] = '\\';
			pOut[Pos++] = (char)c;
		}
		else if(c < 0x20)
		{
			pOut[Pos++] = '\\';
			pOut[Pos++] = 'u';
			pOut[Pos++] = '0';
			pOut[Pos++] = '0';
			pOut[Pos++] = s_aHex[c >> 4];
			pOut[Pos++] = s_aHex[c & 0xf];
		}
		else
		{
			pOut[Pos++] = (char)c;
		}
	}
	pOut[Pos] = '\0';
}

// Bing translate response: [{"detectedLanguage":{"language":"en"},"translations":[{"text":"..."}]}]
bool ParseBingTranslation(const json_value *pRoot, char (&aOut)[MAX_CHAT_LENGTH], char (&aLang)[16])
{
	aOut[0] = '\0';
	aLang[0] = '\0';
	if(!pRoot || pRoot->type != json_array || json_array_length(pRoot) < 1)
		return false;
	const json_value *pResult = json_array_get(pRoot, 0);
	if(!pResult || pResult->type != json_object)
		return false;
	const json_value *pTranslations = json_object_get(pResult, "translations");
	if(!pTranslations || pTranslations->type != json_array || json_array_length(pTranslations) < 1)
		return false;
	const json_value *pTranslation = json_array_get(pTranslations, 0);
	if(!pTranslation || pTranslation->type != json_object)
		return false;
	const json_value *pText = json_object_get(pTranslation, "text");
	if(!pText || pText->type != json_string)
		return false;
	const json_value *pDetected = json_object_get(pResult, "detectedLanguage");
	if(pDetected && pDetected->type == json_object)
	{
		const json_value *pLanguage = json_object_get(pDetected, "language");
		if(pLanguage && pLanguage->type == json_string)
			str_copy(aLang, pLanguage->u.string.ptr);
	}
	str_copy(aOut, pText->u.string.ptr);
	return aOut[0] != '\0';
}

bool ParseTranslation(int Backend, const json_value *pRoot, char (&aOut)[MAX_CHAT_LENGTH], char (&aLang)[16])
{
	return Backend == TRANSLATE_BACKEND_BING ? ParseBingTranslation(pRoot, aOut, aLang) : ParseGoogleTranslation(pRoot, aOut, aLang);
}

// In auto mode fall back to the other backend when the preferred one is unavailable
int TranslateNextBackend(int Tried)
{
	if(g_Config.m_EcChatTranslateBackend != 0)
		return -1;
	return Tried == TRANSLATE_BACKEND_BING ? TRANSLATE_BACKEND_GOOGLE : TRANSLATE_BACKEND_BING;
}
}

int CChat::NumTranslateLanguages()
{
	return (int)std::size(s_aTranslateLanguages);
}

const char *CChat::TranslateLanguageCode(int Index)
{
	return s_aTranslateLanguages[Index].m_pCode;
}

const char *CChat::TranslateLanguageName(int Index)
{
	return s_aTranslateLanguages[Index].m_pName;
}

int CChat::TranslateLanguageIndexOf(const char *pCode)
{
	return TranslateLanguageIndex(pCode);
}

CChat::CLine::CLine()
{
	m_TextContainerIndex.Reset();
	m_QuadContainerIndex = -1;
}

void CChat::CLine::Reset(CChat &This)
{
	This.TextRender()->DeleteTextContainer(m_TextContainerIndex);
	This.Graphics()->DeleteQuadContainer(m_QuadContainerIndex);
	m_Initialized = false;
	m_Time = 0;
	m_aText[0] = '\0';
	m_aTranslatedText[0] = '\0';
	m_TranslateState = 0;
	m_aName[0] = '\0';
	m_Friend = false;
	m_TimesRepeated = 0;
	m_aRenderedPrefix[0] = '\0';
	m_aRenderedText[0] = '\0';
	m_pManagedTeeRenderInfo = nullptr;
}

CChat::CChat()
{
	m_Mode = MODE_NONE;

	m_TranslateLineIndex = -1;
	m_NextTranslateTime = 0;
	m_TranslateOutgoingTeam = 0;
	m_SendingTranslated = false;
	m_TranslateBackend = -1;
	m_TranslateIncomingBackend = -1;
	m_TranslateOutgoingBackend = -1;

	m_NumLineRects = 0;
	m_ChatOpenTime = 0;
	m_SelectionPress = vec2(0.0f, 0.0f);
	m_SelectionRelease = vec2(0.0f, 0.0f);
	m_Selecting = false;
	m_HasSelection = false;
	m_BacklogCurLine = 0;
	m_LinesRendered = 0;

	m_Input.SetCalculateOffsetCallback([this]() { return m_IsInputCensored; });
	m_Input.SetDisplayTextCallback([this](char *pStr, size_t NumChars) {
		m_IsInputCensored = false;
		if(
			g_Config.m_ClStreamerMode &&
			(str_startswith(pStr, "/login ") ||
				str_startswith(pStr, "/register ") ||
				str_startswith(pStr, "/code ") ||
				str_startswith(pStr, "/timeout ") ||
				str_startswith(pStr, "/save ") ||
				str_startswith(pStr, "/load ")))
		{
			bool Censor = false;
			const size_t NumLetters = std::min(NumChars, sizeof(ms_aDisplayText) - 1);
			for(size_t i = 0; i < NumLetters; ++i)
			{
				if(Censor)
					ms_aDisplayText[i] = '*';
				else
					ms_aDisplayText[i] = pStr[i];
				if(pStr[i] == ' ')
				{
					Censor = true;
					m_IsInputCensored = true;
				}
			}
			ms_aDisplayText[NumLetters] = '\0';
			return ms_aDisplayText;
		}
		return pStr;
	});
}

void CChat::RegisterCommand(const char *pName, const char *pParams, const char *pHelpText)
{
	// Don't allow duplicate commands.
	for(const auto &Command : m_vServerCommands)
		if(str_comp(Command.m_aName, pName) == 0)
			return;

	m_vServerCommands.emplace_back(pName, pParams, pHelpText);
	m_ServerCommandsNeedSorting = true;
}

void CChat::UnregisterCommand(const char *pName)
{
	m_vServerCommands.erase(std::remove_if(m_vServerCommands.begin(), m_vServerCommands.end(), [pName](const CCommand &Command) { return str_comp(Command.m_aName, pName) == 0; }), m_vServerCommands.end());
}

void CChat::RebuildChat()
{
	for(auto &Line : m_aLines)
	{
		if(!Line.m_Initialized)
			continue;
		TextRender()->DeleteTextContainer(Line.m_TextContainerIndex);
		Graphics()->DeleteQuadContainer(Line.m_QuadContainerIndex);
		// recalculate sizes
		Line.m_aYOffset[0] = -1.0f;
		Line.m_aYOffset[1] = -1.0f;
	}
}

void CChat::ClearLines()
{
	for(auto &Line : m_aLines)
		Line.Reset(*this);
	m_PrevScoreBoardShowed = false;
	m_PrevShowChat = false;
}

void CChat::OnWindowResize()
{
	RebuildChat();
}

void CChat::Reset()
{
	ClearLines();

	m_Show = false;
	m_CompletionUsed = false;
	m_CompletionChosen = -1;
	m_aCompletionBuffer[0] = 0;
	m_PlaceholderOffset = 0;
	m_PlaceholderLength = 0;
	m_pHistoryEntry = nullptr;
	m_PendingChatCounter = 0;
	m_LastChatSend = 0;
	m_CurrentLine = 0;
	m_IsInputCensored = false;
	m_EditingNewLine = true;
	m_ServerSupportsCommandInfo = false;
	m_ServerCommandsNeedSorting = false;
	m_aCurrentInputText[0] = '\0';
	ClearChatSelection();
	DisableMode();
	m_vServerCommands.clear();
	std::fill(std::begin(m_aLastSoundPlayed), std::end(m_aLastSoundPlayed), 0);
}

void CChat::OnRelease()
{
	m_Show = false;
}

void CChat::OnStateChange(int NewState, int OldState)
{
	if(OldState <= IClient::STATE_CONNECTING)
		Reset();
}

void CChat::ConSay(IConsole::IResult *pResult, void *pUserData)
{
	((CChat *)pUserData)->SendChat(0, pResult->GetString(0));
}

void CChat::ConSayTeam(IConsole::IResult *pResult, void *pUserData)
{
	((CChat *)pUserData)->SendChat(1, pResult->GetString(0));
}

void CChat::ConChat(IConsole::IResult *pResult, void *pUserData)
{
	const char *pMode = pResult->GetString(0);
	if(str_comp(pMode, "all") == 0)
		((CChat *)pUserData)->EnableMode(0);
	else if(str_comp(pMode, "team") == 0)
		((CChat *)pUserData)->EnableMode(1);
	else
		log_error("chat", "expected all or team as mode");

	if(pResult->GetString(1)[0] || g_Config.m_ClChatReset)
		((CChat *)pUserData)->m_Input.Set(pResult->GetString(1));
}

void CChat::ConShowChat(IConsole::IResult *pResult, void *pUserData)
{
	((CChat *)pUserData)->m_Show = pResult->GetInteger(0) != 0;
}

void CChat::ConEcho(IConsole::IResult *pResult, void *pUserData)
{
	((CChat *)pUserData)->Echo(pResult->GetString(0));
}

void CChat::ConEchoConfig(IConsole::IResult *pResult, void *pUserData)
{
	((CChat *)pUserData)->EchoConfig(pResult->GetString(0));
}

void CChat::ConClearChat(IConsole::IResult *pResult, void *pUserData)
{
	((CChat *)pUserData)->ClearLines();
}

void CChat::ConchainChatOld(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	((CChat *)pUserData)->RebuildChat();
}

void CChat::ConchainChatFontSize(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	CChat *pChat = (CChat *)pUserData;
	pChat->EnsureCoherentWidth();
	pChat->RebuildChat();
}

void CChat::ConchainChatWidth(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	CChat *pChat = (CChat *)pUserData;
	pChat->EnsureCoherentFontSize();
	pChat->RebuildChat();
}

void CChat::Echo(const char *pString)
{
	AddLine(CLIENT_MSG, 0, pString);
}

void CChat::EchoConfig(const char *pName)
{
	char aBuf[2048];
	const SConfigVariable *pVariable = GameClient()->ConfigManager()->FindVariable(pName);
	if(pVariable == nullptr)
	{
		str_format(aBuf, sizeof(aBuf), "Invalid config variable: %s", pName);
	}
	else
	{
		pVariable->SerializeHumanReadable(aBuf, sizeof(aBuf));
	}
	Echo(aBuf);
}

void CChat::OnConsoleInit()
{
	Console()->Register("say", "r[message]", CFGFLAG_CLIENT, ConSay, this, "Say in chat");
	Console()->Register("say_team", "r[message]", CFGFLAG_CLIENT, ConSayTeam, this, "Say in team chat");
	Console()->Register("chat", "s['team'|'all'] ?r[message]", CFGFLAG_CLIENT, ConChat, this, "Enable chat with all/team mode");
	Console()->Register("+show_chat", "", CFGFLAG_CLIENT, ConShowChat, this, "Show chat");
	Console()->Register("echo", "r[message]", CFGFLAG_CLIENT | CFGFLAG_STORE, ConEcho, this, "Echo the text in chat window");
	Console()->Register("echo_config", "r[config]", CFGFLAG_CLIENT | CFGFLAG_STORE, ConEchoConfig, this, "Echo the config value in chat window");
	Console()->Register("clear_chat", "", CFGFLAG_CLIENT | CFGFLAG_STORE, ConClearChat, this, "Clear chat messages");
}

void CChat::OnInit()
{
	Reset();
	Console()->Chain("cl_chat_old", ConchainChatOld, this);
	Console()->Chain("cl_chat_size", ConchainChatFontSize, this);
	Console()->Chain("cl_chat_width", ConchainChatWidth, this);
}

bool CChat::OnInput(const IInput::CEvent &Event)
{
	if(m_Mode == MODE_NONE)
		return false;

	if(Event.m_Flags & IInput::FLAG_PRESS && Input()->ModifierIsPressed() && !Input()->AltIsPressed() && Event.m_Key == KEY_C && !m_Input.HasSelection() && m_HasSelection)
	{
		CopyChatSelection();
		return true;
	}

	if(Event.m_Flags & IInput::FLAG_PRESS && Event.m_Key == KEY_ESCAPE)
	{
		ClearChatSelection();
		DisableMode();
		GameClient()->OnRelease();
		if(g_Config.m_ClChatReset)
		{
			m_Input.Clear();
			m_pHistoryEntry = nullptr;
			m_EditingNewLine = true;
		}
	}
	else if(Event.m_Flags & IInput::FLAG_PRESS && (Event.m_Key == KEY_RETURN || Event.m_Key == KEY_KP_ENTER))
	{
		if(m_ServerCommandsNeedSorting)
		{
			std::sort(m_vServerCommands.begin(), m_vServerCommands.end());
			m_ServerCommandsNeedSorting = false;
		}

		SendChatQueued(m_Input.GetString());
		m_pHistoryEntry = nullptr;
		m_EditingNewLine = true;
		DisableMode();
		GameClient()->OnRelease();
		m_Input.Clear();
	}
	if(Event.m_Flags & IInput::FLAG_PRESS && Event.m_Key == KEY_TAB)
	{
		const bool ShiftPressed = Input()->ShiftIsPressed();

		// fill the completion buffer
		if(!m_CompletionUsed)
		{
			const char *pCursor = m_Input.GetString() + m_Input.GetCursorOffset();
			for(size_t Count = 0; Count < m_Input.GetCursorOffset() && *(pCursor - 1) != ' '; --pCursor, ++Count)
				;
			m_PlaceholderOffset = pCursor - m_Input.GetString();

			for(m_PlaceholderLength = 0; *pCursor && *pCursor != ' '; ++pCursor)
				++m_PlaceholderLength;

			str_truncate(m_aCompletionBuffer, sizeof(m_aCompletionBuffer), m_Input.GetString() + m_PlaceholderOffset, m_PlaceholderLength);
		}

		if(!m_CompletionUsed && m_aCompletionBuffer[0] != '/')
		{
			// Create the completion list of player names through which the player can iterate
			const char *PlayerName, *FoundInput;
			m_PlayerCompletionListLength = 0;
			for(auto &PlayerInfo : GameClient()->m_Snap.m_apInfoByName)
			{
				if(PlayerInfo)
				{
					PlayerName = GameClient()->m_aClients[PlayerInfo->m_ClientId].m_aName;
					FoundInput = str_utf8_find_nocase(PlayerName, m_aCompletionBuffer);
					if(FoundInput != nullptr)
					{
						m_aPlayerCompletionList[m_PlayerCompletionListLength].m_ClientId = PlayerInfo->m_ClientId;
						// The score for suggesting a player name is determined by the distance of the search input to the beginning of the player name
						m_aPlayerCompletionList[m_PlayerCompletionListLength].m_Score = (int)(FoundInput - PlayerName);
						m_PlayerCompletionListLength++;
					}
				}
			}
			std::stable_sort(m_aPlayerCompletionList, m_aPlayerCompletionList + m_PlayerCompletionListLength,
				[](const CRateablePlayer &Player1, const CRateablePlayer &Player2) -> bool {
					return Player1.m_Score < Player2.m_Score;
				});
		}

		if(m_aCompletionBuffer[0] == '/' && !m_vServerCommands.empty())
		{
			CCommand *pCompletionCommand = nullptr;

			const size_t NumCommands = m_vServerCommands.size();

			if(ShiftPressed && m_CompletionUsed)
				m_CompletionChosen--;
			else if(!ShiftPressed)
				m_CompletionChosen++;
			m_CompletionChosen = (m_CompletionChosen + 2 * NumCommands) % (2 * NumCommands);

			m_CompletionUsed = true;

			const char *pCommandStart = m_aCompletionBuffer + 1;
			for(size_t i = 0; i < 2 * NumCommands; ++i)
			{
				int SearchType;
				int Index;

				if(ShiftPressed)
				{
					SearchType = ((m_CompletionChosen - i + 2 * NumCommands) % (2 * NumCommands)) / NumCommands;
					Index = (m_CompletionChosen - i + NumCommands) % NumCommands;
				}
				else
				{
					SearchType = ((m_CompletionChosen + i) % (2 * NumCommands)) / NumCommands;
					Index = (m_CompletionChosen + i) % NumCommands;
				}

				auto &Command = m_vServerCommands[Index];

				if(str_startswith_nocase(Command.m_aName, pCommandStart))
				{
					pCompletionCommand = &Command;
					m_CompletionChosen = Index + SearchType * NumCommands;
					break;
				}
			}

			// insert the command
			if(pCompletionCommand)
			{
				char aBuf[MAX_CHAT_LENGTH];
				// add part before the name
				str_truncate(aBuf, sizeof(aBuf), m_Input.GetString(), m_PlaceholderOffset);

				// add the command
				str_append(aBuf, "/");
				str_append(aBuf, pCompletionCommand->m_aName);

				// add separator
				const char *pSeparator = pCompletionCommand->m_aParams[0] == '\0' ? "" : " ";
				str_append(aBuf, pSeparator);

				// add part after the name
				str_append(aBuf, m_Input.GetString() + m_PlaceholderOffset + m_PlaceholderLength);

				m_PlaceholderLength = str_length(pSeparator) + str_length(pCompletionCommand->m_aName) + 1;
				m_Input.Set(aBuf);
				m_Input.SetCursorOffset(m_PlaceholderOffset + m_PlaceholderLength);
			}
		}
		else
		{
			// find next possible name
			const char *pCompletionString = nullptr;
			if(m_PlayerCompletionListLength > 0)
			{
				// We do this in a loop, if a player left the game during the repeated pressing of Tab, they are skipped
				CGameClient::CClientData *pCompletionClientData;
				for(int i = 0; i < m_PlayerCompletionListLength; ++i)
				{
					if(ShiftPressed && m_CompletionUsed)
					{
						m_CompletionChosen--;
					}
					else if(!ShiftPressed)
					{
						m_CompletionChosen++;
					}
					if(m_CompletionChosen < 0)
					{
						m_CompletionChosen += m_PlayerCompletionListLength;
					}
					m_CompletionChosen %= m_PlayerCompletionListLength;
					m_CompletionUsed = true;

					pCompletionClientData = &GameClient()->m_aClients[m_aPlayerCompletionList[m_CompletionChosen].m_ClientId];
					if(!pCompletionClientData->m_Active)
					{
						continue;
					}

					pCompletionString = pCompletionClientData->m_aName;
					break;
				}
			}

			// insert the name
			if(pCompletionString)
			{
				char aBuf[MAX_CHAT_LENGTH];
				// add part before the name
				str_truncate(aBuf, sizeof(aBuf), m_Input.GetString(), m_PlaceholderOffset);

				// quote the name
				char aQuoted[128];
				if(m_Input.GetString()[0] == '/' && (str_find(pCompletionString, " ") || str_find(pCompletionString, "\"")))
				{
					// escape the name
					str_copy(aQuoted, "\"");
					char *pDst = aQuoted + str_length(aQuoted);
					str_escape(&pDst, pCompletionString, aQuoted + sizeof(aQuoted));
					str_append(aQuoted, "\"");

					pCompletionString = aQuoted;
				}

				// add the name
				str_append(aBuf, pCompletionString);

				// add separator
				const char *pSeparator = "";
				if(*(m_Input.GetString() + m_PlaceholderOffset + m_PlaceholderLength) != ' ')
					pSeparator = m_PlaceholderOffset == 0 ? ": " : " ";
				else if(m_PlaceholderOffset == 0)
					pSeparator = ":";
				if(*pSeparator)
					str_append(aBuf, pSeparator);

				// add part after the name
				str_append(aBuf, m_Input.GetString() + m_PlaceholderOffset + m_PlaceholderLength);

				m_PlaceholderLength = str_length(pSeparator) + str_length(pCompletionString);
				m_Input.Set(aBuf);
				m_Input.SetCursorOffset(m_PlaceholderOffset + m_PlaceholderLength);
			}
		}
	}
	else
	{
		// reset name completion process
		if(Event.m_Flags & IInput::FLAG_PRESS && Event.m_Key != KEY_TAB && Event.m_Key != KEY_LSHIFT && Event.m_Key != KEY_RSHIFT)
		{
			m_CompletionChosen = -1;
			m_CompletionUsed = false;
		}

		m_Input.ProcessInput(Event);
	}

	if(Event.m_Flags & IInput::FLAG_PRESS && g_Config.m_EcChatHistory)
	{
		if(Event.m_Key == KEY_MOUSE_WHEEL_UP)
		{
			ClearChatSelection();
			m_BacklogCurLine += GetLinesToScroll(-1, 1);
			return true;
		}
		else if(Event.m_Key == KEY_MOUSE_WHEEL_DOWN)
		{
			ClearChatSelection();
			m_BacklogCurLine = std::max(0, m_BacklogCurLine - GetLinesToScroll(1, 1));
			return true;
		}
		else if(Event.m_Key == KEY_PAGEUP)
		{
			ClearChatSelection();
			ScrollPageUp();
			return true;
		}
		else if(Event.m_Key == KEY_PAGEDOWN)
		{
			ClearChatSelection();
			ScrollPageDown();
			return true;
		}
		else if(Event.m_Key == KEY_HOME && m_Input.IsEmpty())
		{
			ClearChatSelection();
			ScrollToTop();
			return true;
		}
		else if(Event.m_Key == KEY_END && m_Input.IsEmpty())
		{
			ClearChatSelection();
			ScrollToBottom();
			return true;
		}
	}

	if(Event.m_Flags & IInput::FLAG_PRESS && Event.m_Key == KEY_UP)
	{
		if(m_EditingNewLine)
		{
			str_copy(m_aCurrentInputText, m_Input.GetString());
			m_EditingNewLine = false;
		}

		if(m_pHistoryEntry)
		{
			CHistoryEntry *pTest = m_History.Prev(m_pHistoryEntry);

			if(pTest)
				m_pHistoryEntry = pTest;
		}
		else
		{
			m_pHistoryEntry = m_History.Last();
		}

		if(m_pHistoryEntry)
			m_Input.Set(m_pHistoryEntry->m_aText);
	}
	else if(Event.m_Flags & IInput::FLAG_PRESS && Event.m_Key == KEY_DOWN)
	{
		if(m_pHistoryEntry)
			m_pHistoryEntry = m_History.Next(m_pHistoryEntry);

		if(m_pHistoryEntry)
		{
			m_Input.Set(m_pHistoryEntry->m_aText);
		}
		else if(!m_EditingNewLine)
		{
			m_Input.Set(m_aCurrentInputText);
			m_EditingNewLine = true;
		}
	}

	return true;
}

void CChat::EnableMode(int Team)
{
	if(Client()->State() == IClient::STATE_DEMOPLAYBACK)
		return;

	if(m_Mode == MODE_NONE)
	{
		if(Team)
			m_Mode = MODE_TEAM;
		else
			m_Mode = MODE_ALL;

		m_CompletionChosen = -1;
		m_CompletionUsed = false;
		m_ChatOpenTime = time();
		m_PrevInputText.clear();
		m_vTypingAnims.clear();
		m_Input.Activate(EInputPriority::CHAT);
		if(g_Config.m_EcChatMouseSelection)
			Input()->MouseModeAbsolute();
	}
}

void CChat::DisableMode()
{
	if(m_Mode != MODE_NONE)
	{
		m_Mode = MODE_NONE;
		m_Input.Deactivate();
		m_ChatOpenTime = 0;
		m_PrevInputText.clear();
		m_vTypingAnims.clear();
		m_BacklogCurLine = 0;
		ClearChatSelection();
		if(g_Config.m_EcChatMouseSelection)
			Input()->MouseModeRelative();
	}
}

void CChat::OnMessage(int MsgType, void *pRawMsg)
{
	if(GameClient()->m_SuppressEvents)
		return;

	if(MsgType == NETMSGTYPE_SV_CHAT)
	{
		CNetMsg_Sv_Chat *pMsg = (CNetMsg_Sv_Chat *)pRawMsg;

		/*
		if(g_Config.m_ClCensorChat)
		{
			char aMessage[MAX_CHAT_LENGTH];
			str_copy(aMessage, pMsg->m_pMessage);
			GameClient()->m_Censor.CensorMessage(aMessage);
			AddLine(pMsg->m_ClientId, pMsg->m_Team, aMessage);
		}
		else
			AddLine(pMsg->m_ClientId, pMsg->m_Team, pMsg->m_pMessage);
		*/

		AddLine(pMsg->m_ClientId, pMsg->m_Team, pMsg->m_pMessage);

		if(Client()->State() != IClient::STATE_DEMOPLAYBACK &&
			pMsg->m_ClientId == SERVER_MSG)
		{
			StoreSave(pMsg->m_pMessage);
		}
	}
	else if(MsgType == NETMSGTYPE_SV_COMMANDINFO)
	{
		CNetMsg_Sv_CommandInfo *pMsg = (CNetMsg_Sv_CommandInfo *)pRawMsg;
		if(!m_ServerSupportsCommandInfo)
		{
			m_vServerCommands.clear();
			m_ServerSupportsCommandInfo = true;
		}
		RegisterCommand(pMsg->m_pName, pMsg->m_pArgsFormat, pMsg->m_pHelpText);
	}
	else if(MsgType == NETMSGTYPE_SV_COMMANDINFOREMOVE)
	{
		CNetMsg_Sv_CommandInfoRemove *pMsg = (CNetMsg_Sv_CommandInfoRemove *)pRawMsg;
		UnregisterCommand(pMsg->m_pName);
	}
}

bool CChat::LineShouldHighlight(const char *pLine, const char *pName)
{
	const char *pHit = str_utf8_find_nocase(pLine, pName);

	while(pHit)
	{
		int Length = str_length(pName);

		if(Length > 0 && (pLine == pHit || pHit[-1] == ' ') && (pHit[Length] == 0 || pHit[Length] == ' ' || pHit[Length] == '.' || pHit[Length] == '!' || pHit[Length] == ',' || pHit[Length] == '?' || pHit[Length] == ':'))
			return true;

		pHit = str_utf8_find_nocase(pHit + 1, pName);
	}

	return false;
}

static constexpr const char *SAVES_HEADER[] = {
	"Time",
	"Player",
	"Map",
	"Code",
};

// TODO: remove this in a few releases (in 2027 or later)
//       it got deprecated by CGameClient::StoreSave
void CChat::StoreSave(const char *pText)
{
	const char *pStart = str_find(pText, "Team successfully saved by ");
	const char *pMid = str_find(pText, ". Use '/load ");
	const char *pOn = str_find(pText, "' on ");
	const char *pEnd = str_find(pText, pOn ? " to continue" : "' to continue");

	if(!pStart || !pMid || !pEnd || pMid < pStart || pEnd < pMid || (pOn && (pOn < pMid || pEnd < pOn)))
		return;

	char aName[16];
	str_truncate(aName, sizeof(aName), pStart + 27, pMid - pStart - 27);

	char aSaveCode[64];

	str_truncate(aSaveCode, sizeof(aSaveCode), pMid + 13, (pOn ? pOn : pEnd) - pMid - 13);

	char aTimestamp[20];
	str_timestamp_format(aTimestamp, sizeof(aTimestamp), TimestampFormat::SPACE);

	const bool SavesFileExists = Storage()->FileExists(SAVES_FILE, IStorage::TYPE_SAVE);
	IOHANDLE File = Storage()->OpenFile(SAVES_FILE, IOFLAG_APPEND, IStorage::TYPE_SAVE);
	if(!File)
		return;

	const char *apColumns[4] = {
		aTimestamp,
		aName,
		GameClient()->Map()->BaseName(),
		aSaveCode,
	};

	if(!SavesFileExists)
	{
		CsvWrite(File, 4, SAVES_HEADER);
	}
	CsvWrite(File, 4, apColumns);
	io_close(File);
}

void CChat::AddLine(int ClientId, int Team, const char *pLine)
{
	if(*pLine == 0 ||
		(ClientId == SERVER_MSG && !g_Config.m_ClShowChatSystem) ||
		(ClientId >= 0 && (GameClient()->m_aClients[ClientId].m_aName[0] == '\0' || // unknown client
					  GameClient()->m_aClients[ClientId].m_ChatIgnore ||
					  (GameClient()->m_Snap.m_LocalClientId != ClientId && g_Config.m_ClShowChatFriends && !GameClient()->m_aClients[ClientId].m_Friend) ||
					  (GameClient()->m_Snap.m_LocalClientId != ClientId && g_Config.m_ClShowChatTeamMembersOnly && GameClient()->IsOtherTeam(ClientId) && GameClient()->m_Teams.Team(GameClient()->m_Snap.m_LocalClientId) != TEAM_FLOCK) ||
					  (GameClient()->m_Snap.m_LocalClientId != ClientId && GameClient()->m_aClients[ClientId].m_Foe))))
		return;

	// trim right and set maximum length to 256 utf8-characters
	int Length = 0;
	const char *pStr = pLine;
	const char *pEnd = nullptr;
	while(*pStr)
	{
		const char *pStrOld = pStr;
		int Code = str_utf8_decode(&pStr);

		// check if unicode is not empty
		if(!str_utf8_isspace(Code))
		{
			pEnd = nullptr;
		}
		else if(pEnd == nullptr)
		{
			pEnd = pStrOld;
		}

		if(++Length >= MAX_CHAT_LENGTH)
		{
			*(const_cast<char *>(pStr)) = '\0';
			break;
		}
	}
	if(pEnd != nullptr)
		*(const_cast<char *>(pEnd)) = '\0';

	if(*pLine == 0)
		return;

	bool Highlighted = false;

	auto &&FChatMsgCheckAndPrint = [](const CLine &Line) {
		ColorRGBA ChatLogColor = ColorRGBA(1.0f, 1.0f, 1.0f, 1.0f);
		if(Line.m_Highlighted)
		{
			ChatLogColor = color_cast<ColorRGBA>(ColorHSLA(g_Config.m_ClMessageHighlightColor));
		}
		else
		{
			if(Line.m_Friend && g_Config.m_ClMessageFriend)
				ChatLogColor = color_cast<ColorRGBA>(ColorHSLA(g_Config.m_ClMessageFriendColor));
			else if(Line.m_Team)
				ChatLogColor = color_cast<ColorRGBA>(ColorHSLA(g_Config.m_ClMessageTeamColor));
			else if(Line.m_ClientId == SERVER_MSG)
				ChatLogColor = color_cast<ColorRGBA>(ColorHSLA(g_Config.m_ClMessageSystemColor));
			else if(Line.m_ClientId == CLIENT_MSG)
				ChatLogColor = color_cast<ColorRGBA>(ColorHSLA(g_Config.m_ClMessageClientColor));
			else // regular message
				ChatLogColor = color_cast<ColorRGBA>(ColorHSLA(g_Config.m_ClMessageColor));
		}

		const char *pFrom;
		if(Line.m_Whisper)
			pFrom = "chat/whisper";
		else if(Line.m_Team)
			pFrom = "chat/team";
		else if(Line.m_ClientId == SERVER_MSG)
			pFrom = "chat/server";
		else if(Line.m_ClientId == CLIENT_MSG)
			pFrom = "chat/client";
		else
			pFrom = "chat/all";

		log_info_color(color_cast<LOG_COLOR>(ChatLogColor), pFrom, "%s%s%s", Line.m_aName, Line.m_ClientId >= 0 ? ": " : "", Line.m_aText);
	};

	// Custom color for new line
	std::optional<ColorRGBA> CustomColor = std::nullopt;
	if(ClientId == CLIENT_MSG)
		CustomColor = color_cast<ColorRGBA>(ColorHSLA(g_Config.m_ClMessageClientColor));

	CLine &PreviousLine = m_aLines[m_CurrentLine];

	// Team Number:
	// 0 = global; 1 = team; 2 = sending whisper; 3 = receiving whisper

	// If it's a client message, m_aText will have ": " prepended so we have to work around it.
	if(PreviousLine.m_Initialized &&
		PreviousLine.m_TeamNumber == Team &&
		PreviousLine.m_ClientId == ClientId &&
		str_comp(PreviousLine.m_aText, pLine) == 0 &&
		PreviousLine.m_CustomColor == CustomColor)
	{
		PreviousLine.m_TimesRepeated++;
		TextRender()->DeleteTextContainer(PreviousLine.m_TextContainerIndex);
		Graphics()->DeleteQuadContainer(PreviousLine.m_QuadContainerIndex);
		PreviousLine.m_Time = time();
		PreviousLine.m_aYOffset[0] = -1.0f;
		PreviousLine.m_aYOffset[1] = -1.0f;

		FChatMsgCheckAndPrint(PreviousLine);
		return;
	}

	m_CurrentLine = (m_CurrentLine + 1) % MAX_LINES;

	CLine &CurrentLine = m_aLines[m_CurrentLine];
	CurrentLine.Reset(*this);
	CurrentLine.m_Initialized = true;
	CurrentLine.m_Time = time();
	CurrentLine.m_aYOffset[0] = -1.0f;
	CurrentLine.m_aYOffset[1] = -1.0f;
	CurrentLine.m_ClientId = ClientId;
	CurrentLine.m_TeamNumber = Team;
	CurrentLine.m_Team = Team == 1;
	CurrentLine.m_Whisper = Team >= 2;
	CurrentLine.m_NameColor = -2;
	CurrentLine.m_CustomColor = CustomColor;

	// check for highlighted name
	if(Client()->State() != IClient::STATE_DEMOPLAYBACK)
	{
		if(ClientId >= 0 && ClientId != GameClient()->m_aLocalIds[0] && ClientId != GameClient()->m_aLocalIds[1])
		{
			for(int LocalId : GameClient()->m_aLocalIds)
			{
				Highlighted |= LocalId >= 0 && LineShouldHighlight(pLine, GameClient()->m_aClients[LocalId].m_aName);
			}
		}
	}
	else
	{
		// on demo playback use local id from snap directly,
		// since m_aLocalIds isn't valid there
		Highlighted |= GameClient()->m_Snap.m_LocalClientId >= 0 && LineShouldHighlight(pLine, GameClient()->m_aClients[GameClient()->m_Snap.m_LocalClientId].m_aName);
	}
	CurrentLine.m_Highlighted = Highlighted;

	str_copy(CurrentLine.m_aText, pLine);
	if(g_Config.m_EcChatTranslate && ClientId >= 0 &&
		ClientId != GameClient()->m_aLocalIds[0] && ClientId != GameClient()->m_aLocalIds[1] &&
		CurrentLine.m_aText[0] != '\0')
	{
		CurrentLine.m_TranslateState = 1;
	}

	if(CurrentLine.m_ClientId == SERVER_MSG)
	{
		str_copy(CurrentLine.m_aName, "*** ");
	}
	else if(CurrentLine.m_ClientId == CLIENT_MSG)
	{
		str_copy(CurrentLine.m_aName, "— ");
	}
	else
	{
		const auto &LineAuthor = GameClient()->m_aClients[CurrentLine.m_ClientId];

		if(LineAuthor.m_Active)
		{
			if(LineAuthor.m_Team == TEAM_SPECTATORS)
				CurrentLine.m_NameColor = TEAM_SPECTATORS;

			if(GameClient()->IsTeamPlay())
			{
				if(LineAuthor.m_Team == TEAM_RED)
					CurrentLine.m_NameColor = TEAM_RED;
				else if(LineAuthor.m_Team == TEAM_BLUE)
					CurrentLine.m_NameColor = TEAM_BLUE;
			}
		}

		if(Team == TEAM_WHISPER_SEND)
		{
			str_copy(CurrentLine.m_aName, "→");
			if(LineAuthor.m_Active)
			{
				str_append(CurrentLine.m_aName, " ");
				str_append(CurrentLine.m_aName, LineAuthor.m_aName);
			}
			CurrentLine.m_NameColor = TEAM_BLUE;
			CurrentLine.m_Highlighted = false;
			Highlighted = false;
		}
		else if(Team == TEAM_WHISPER_RECV)
		{
			str_copy(CurrentLine.m_aName, "←");
			if(LineAuthor.m_Active)
			{
				str_append(CurrentLine.m_aName, " ");
				str_append(CurrentLine.m_aName, LineAuthor.m_aName);
			}
			CurrentLine.m_NameColor = TEAM_RED;
			CurrentLine.m_Highlighted = true;
			Highlighted = true;
		}
		else
		{
			str_copy(CurrentLine.m_aName, LineAuthor.m_aName);
		}

		if(LineAuthor.m_Active)
		{
			CurrentLine.m_Friend = LineAuthor.m_Friend;
			CurrentLine.m_pManagedTeeRenderInfo = GameClient()->CreateManagedTeeRenderInfo(LineAuthor);
		}
	}

	FChatMsgCheckAndPrint(CurrentLine);

	// keep the scrolled view stable when new lines arrive
	if(m_BacklogCurLine > 0)
		m_BacklogCurLine = std::min(m_BacklogCurLine + 1, GetMaxBacklogCurLine());

	// play sound
	int64_t Now = time();
	if(ClientId == SERVER_MSG)
	{
		if(Now - m_aLastSoundPlayed[CHAT_SERVER] >= time_freq() * 3 / 10)
		{
			if(g_Config.m_SndServerMessage)
			{
				GameClient()->m_Sounds.Play(CSounds::CHN_GUI, SOUND_CHAT_SERVER, 1.0f);
				m_aLastSoundPlayed[CHAT_SERVER] = Now;
			}
		}
	}
	else if(ClientId == CLIENT_MSG)
	{
		// No sound yet
	}
	else if(Highlighted && Client()->State() != IClient::STATE_DEMOPLAYBACK)
	{
		if(Now - m_aLastSoundPlayed[CHAT_HIGHLIGHT] >= time_freq() * 3 / 10)
		{
			char aBuf[1024];
			str_format(aBuf, sizeof(aBuf), "%s: %s", CurrentLine.m_aName, CurrentLine.m_aText);
			Client()->Notify("DDNet Chat", aBuf);
			if(g_Config.m_SndHighlight)
			{
				GameClient()->m_Sounds.Play(CSounds::CHN_GUI, SOUND_CHAT_HIGHLIGHT, 1.0f);
				m_aLastSoundPlayed[CHAT_HIGHLIGHT] = Now;
			}

			if(g_Config.m_ClEditor)
			{
				GameClient()->Editor()->UpdateMentions();
			}
		}
	}
	else if(Team != TEAM_WHISPER_SEND)
	{
		if(Now - m_aLastSoundPlayed[CHAT_CLIENT] >= time_freq() * 3 / 10)
		{
			bool PlaySound = CurrentLine.m_Team ? g_Config.m_SndTeamChat : g_Config.m_SndChat;
#if defined(CONF_VIDEORECORDER)
			if(IVideo::Current())
			{
				PlaySound &= (bool)g_Config.m_ClVideoShowChat;
			}
#endif
			if(PlaySound)
			{
				GameClient()->m_Sounds.Play(CSounds::CHN_GUI, SOUND_CHAT_CLIENT, 1.0f);
				m_aLastSoundPlayed[CHAT_CLIENT] = Now;
			}
		}
	}
}

void CChat::OnPrepareLines(float y)
{
	float x = 5.0f;
	float FontSize = this->FontSize();

	const bool IsScoreBoardOpen = GameClient()->m_Scoreboard.IsActive() && (Graphics()->ScreenAspect() > 1.7f); // only assume scoreboard when screen ratio is widescreen(something around 16:9)
	const bool ShowLargeArea = m_Show || (m_Mode != MODE_NONE && g_Config.m_ClShowChat == 1) || g_Config.m_ClShowChat == 2;
	const bool ForceRecreate = IsScoreBoardOpen != m_PrevScoreBoardShowed || ShowLargeArea != m_PrevShowChat;
	m_PrevScoreBoardShowed = IsScoreBoardOpen;
	m_PrevShowChat = ShowLargeArea;

	const int TeeSize = MessageTeeSize();
	float RealMsgPaddingX = MessagePaddingX();
	float RealMsgPaddingY = MessagePaddingY();
	float RealMsgPaddingTee = TeeSize + MESSAGE_TEE_PADDING_RIGHT;

	if(g_Config.m_ClChatOld)
	{
		RealMsgPaddingX = 0;
		RealMsgPaddingY = 0;
		RealMsgPaddingTee = 0;
	}

	int64_t Now = time();
	float LineWidth = (IsScoreBoardOpen ? std::max(85.0f, FontSize * 85.0f / 6.0f) : g_Config.m_ClChatWidth) - (RealMsgPaddingX * 1.5f) - RealMsgPaddingTee;

	float HeightLimit = IsScoreBoardOpen ? 180.0f : (m_PrevShowChat ? 50.0f : 200.0f);
	float Begin = x;
	float TextBegin = Begin + RealMsgPaddingX / 2.0f;
	int OffsetType = IsScoreBoardOpen ? 1 : 0;

	for(int i = 0; i < MAX_LINES; i++)
	{
		CLine &Line = m_aLines[((m_CurrentLine - i) + MAX_LINES) % MAX_LINES];
		if(!Line.m_Initialized)
			break;
		if(Now > Line.m_Time + 16 * time_freq() && !m_PrevShowChat)
			break;

		if(i < m_BacklogCurLine)
			continue;

		if(Line.m_TextContainerIndex.Valid() && !ForceRecreate)
			continue;

		TextRender()->DeleteTextContainer(Line.m_TextContainerIndex);
		Graphics()->DeleteQuadContainer(Line.m_QuadContainerIndex);

		char aClientId[16] = "";
		if(g_Config.m_ClShowIds && Line.m_ClientId >= 0 && Line.m_aName[0] != '\0')
		{
			GameClient()->FormatClientId(Line.m_ClientId, aClientId, EClientIdFormat::INDENT_AUTO);
		}

		char aCount[12];
		if(Line.m_ClientId < 0)
			str_format(aCount, sizeof(aCount), "[%d] ", Line.m_TimesRepeated + 1);
		else
			str_format(aCount, sizeof(aCount), " [%d]", Line.m_TimesRepeated + 1);

		const char *pText = Line.m_aText;
		if(Line.m_TranslateState == 2 && Line.m_aTranslatedText[0] != '\0')
			pText = Line.m_aTranslatedText;
		if(Config()->m_ClStreamerMode && Line.m_ClientId == SERVER_MSG)
		{
			if(str_startswith(Line.m_aText, "Team save in progress. You'll be able to load with '/load ") && str_endswith(Line.m_aText, "'"))
			{
				pText = "Team save in progress. You'll be able to load with '/load *** *** ***'";
			}
			else if(str_startswith(Line.m_aText, "Team save in progress. You'll be able to load with '/load") && str_endswith(Line.m_aText, "if it fails"))
			{
				pText = "Team save in progress. You'll be able to load with '/load *** *** ***' if save is successful or with '/load *** *** ***' if it fails";
			}
			else if(str_startswith(Line.m_aText, "Team successfully saved by ") && str_endswith(Line.m_aText, " to continue"))
			{
				pText = "Team successfully saved by ***. Use '/load *** *** ***' to continue";
			}
		}

		char aPrefix[MAX_CHAT_LENGTH] = "";
		if(Line.m_ClientId >= 0 && Line.m_aName[0] != '\0')
		{
			if(Line.m_Friend && g_Config.m_ClMessageFriend)
				str_append(aPrefix, "♥ ");
			str_append(aPrefix, aClientId);
			str_append(aPrefix, Line.m_aName);
			if(Line.m_TimesRepeated > 0)
				str_append(aPrefix, aCount);
			str_append(aPrefix, ": ");
		}
		else
		{
			str_append(aPrefix, aClientId);
			str_append(aPrefix, Line.m_aName);
			if(Line.m_TimesRepeated > 0)
				str_append(aPrefix, aCount);
		}
		str_copy(Line.m_aRenderedPrefix, aPrefix);
		str_copy(Line.m_aRenderedText, pText);

		const bool ShowOriginal = Line.m_TranslateState == 2 && Line.m_aTranslatedText[0] != '\0' && g_Config.m_EcChatTranslateShowOriginal;

		// get the y offset (calculate it if we haven't done that yet)
		if(Line.m_aYOffset[OffsetType] < 0.0f)
		{
			CTextCursor MeasureCursor;
			MeasureCursor.SetPosition(vec2(TextBegin, 0.0f));
			MeasureCursor.m_FontSize = FontSize;
			MeasureCursor.m_Flags = 0;
			MeasureCursor.m_LineWidth = LineWidth;

			if(Line.m_ClientId >= 0 && Line.m_aName[0] != '\0')
			{
				MeasureCursor.m_X += RealMsgPaddingTee;

				if(Line.m_Friend && g_Config.m_ClMessageFriend)
				{
					TextRender()->TextEx(&MeasureCursor, "♥ ");
				}
			}

			TextRender()->TextEx(&MeasureCursor, aClientId);
			TextRender()->TextEx(&MeasureCursor, Line.m_aName);
			if(Line.m_TimesRepeated > 0)
				TextRender()->TextEx(&MeasureCursor, aCount);

			if(Line.m_ClientId >= 0 && Line.m_aName[0] != '\0')
			{
				TextRender()->TextEx(&MeasureCursor, ": ");
			}

			CTextCursor AppendCursor = MeasureCursor;
			AppendCursor.m_LongestLineWidth = 0.0f;
			if(!IsScoreBoardOpen && !g_Config.m_ClChatOld)
			{
				AppendCursor.m_StartX = MeasureCursor.m_X;
				AppendCursor.m_LineWidth -= MeasureCursor.m_LongestLineWidth;
			}

			if(ShowOriginal)
			{
				TextRender()->TextEx(&AppendCursor, pText);
				TextRender()->TextEx(&AppendCursor, "\n");
				AppendCursor.m_FontSize *= 0.8f;
				TextRender()->TextEx(&AppendCursor, Line.m_aText);
				AppendCursor.m_FontSize /= 0.8f;
			}
			else
			{
				TextRender()->TextEx(&AppendCursor, pText);
			}

			Line.m_aYOffset[OffsetType] = AppendCursor.Height() + RealMsgPaddingY;
		}

		y -= Line.m_aYOffset[OffsetType];

		// cut off if msgs waste too much space
		if(y < HeightLimit)
			break;

		// the position the text was created
		Line.m_TextYOffset = y + RealMsgPaddingY / 2.0f;

		int CurRenderFlags = TextRender()->GetRenderFlags();
		TextRender()->SetRenderFlags(CurRenderFlags | ETextRenderFlags::TEXT_RENDER_FLAG_NO_AUTOMATIC_QUAD_UPLOAD);

		// reset the cursor
		CTextCursor LineCursor;
		LineCursor.SetPosition(vec2(TextBegin, Line.m_TextYOffset));
		LineCursor.m_FontSize = FontSize;
		LineCursor.m_LineWidth = LineWidth;

		// Message is from valid player
		if(Line.m_ClientId >= 0 && Line.m_aName[0] != '\0')
		{
			LineCursor.m_X += RealMsgPaddingTee;

			if(Line.m_Friend && g_Config.m_ClMessageFriend)
			{
				TextRender()->TextColor(color_cast<ColorRGBA>(ColorHSLA(g_Config.m_ClMessageFriendColor)).WithAlpha(1.0f));
				TextRender()->CreateOrAppendTextContainer(Line.m_TextContainerIndex, &LineCursor, "♥ ");
			}
		}

		// render name
		ColorRGBA NameColor;
		if(Line.m_CustomColor)
			NameColor = *Line.m_CustomColor;
		else if(Line.m_ClientId == SERVER_MSG)
			NameColor = color_cast<ColorRGBA>(ColorHSLA(g_Config.m_ClMessageSystemColor));
		else if(Line.m_ClientId == CLIENT_MSG)
			NameColor = color_cast<ColorRGBA>(ColorHSLA(g_Config.m_ClMessageClientColor));
		else if(Line.m_Team)
			NameColor = CalculateNameColor(ColorHSLA(g_Config.m_ClMessageTeamColor));
		else if(Line.m_NameColor == TEAM_RED)
			NameColor = ColorRGBA(1.0f, 0.5f, 0.5f, 1.0f);
		else if(Line.m_NameColor == TEAM_BLUE)
			NameColor = ColorRGBA(0.7f, 0.7f, 1.0f, 1.0f);
		else if(Line.m_NameColor == TEAM_SPECTATORS)
			NameColor = ColorRGBA(0.75f, 0.5f, 0.75f, 1.0f);
		else if(Line.m_ClientId >= 0 && g_Config.m_ClChatTeamColors && GameClient()->m_Teams.Team(Line.m_ClientId))
			NameColor = GameClient()->GetDDTeamColor(GameClient()->m_Teams.Team(Line.m_ClientId), 0.75f);
		else
			NameColor = ColorRGBA(0.8f, 0.8f, 0.8f, 1.0f);

		TextRender()->TextColor(NameColor);
		TextRender()->CreateOrAppendTextContainer(Line.m_TextContainerIndex, &LineCursor, aClientId);
		TextRender()->CreateOrAppendTextContainer(Line.m_TextContainerIndex, &LineCursor, Line.m_aName);

		if(Line.m_TimesRepeated > 0)
		{
			TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.3f);
			TextRender()->CreateOrAppendTextContainer(Line.m_TextContainerIndex, &LineCursor, aCount);
		}

		if(Line.m_ClientId >= 0 && Line.m_aName[0] != '\0')
		{
			TextRender()->TextColor(NameColor);
			TextRender()->CreateOrAppendTextContainer(Line.m_TextContainerIndex, &LineCursor, ": ");
		}

		ColorRGBA Color;
		if(Line.m_CustomColor)
			Color = *Line.m_CustomColor;
		else if(Line.m_ClientId == SERVER_MSG)
			Color = color_cast<ColorRGBA>(ColorHSLA(g_Config.m_ClMessageSystemColor));
		else if(Line.m_ClientId == CLIENT_MSG)
			Color = color_cast<ColorRGBA>(ColorHSLA(g_Config.m_ClMessageClientColor));
		else if(Line.m_Highlighted)
			Color = color_cast<ColorRGBA>(ColorHSLA(g_Config.m_ClMessageHighlightColor));
		else if(Line.m_Team)
			Color = color_cast<ColorRGBA>(ColorHSLA(g_Config.m_ClMessageTeamColor));
		else // regular message
			Color = color_cast<ColorRGBA>(ColorHSLA(g_Config.m_ClMessageColor));
		TextRender()->TextColor(Color);

		CTextCursor AppendCursor = LineCursor;
		AppendCursor.m_LongestLineWidth = 0.0f;
		if(!IsScoreBoardOpen && !g_Config.m_ClChatOld)
		{
			AppendCursor.m_StartX = LineCursor.m_X;
			AppendCursor.m_LineWidth -= LineCursor.m_LongestLineWidth;
		}

		if(ShowOriginal)
		{
			TextRender()->CreateOrAppendTextContainer(Line.m_TextContainerIndex, &AppendCursor, pText);
			ColorRGBA SubColor = Color;
			SubColor.r *= 0.7f;
			SubColor.g *= 0.7f;
			SubColor.b *= 0.7f;
			TextRender()->TextColor(SubColor);
			TextRender()->CreateOrAppendTextContainer(Line.m_TextContainerIndex, &AppendCursor, "\n");
			AppendCursor.m_FontSize *= 0.8f;
			TextRender()->CreateOrAppendTextContainer(Line.m_TextContainerIndex, &AppendCursor, Line.m_aText);
			AppendCursor.m_FontSize /= 0.8f;
			TextRender()->TextColor(Color);
		}
		else
		{
			TextRender()->CreateOrAppendTextContainer(Line.m_TextContainerIndex, &AppendCursor, pText);
		}

		if(!g_Config.m_ClChatOld && (Line.m_aText[0] != '\0' || Line.m_aName[0] != '\0'))
		{
			float FullWidth = RealMsgPaddingX * 1.5f;
			if(!IsScoreBoardOpen && !g_Config.m_ClChatOld)
			{
				FullWidth += LineCursor.m_LongestLineWidth + AppendCursor.m_LongestLineWidth;
			}
			else
			{
				FullWidth += std::max(LineCursor.m_LongestLineWidth, AppendCursor.m_LongestLineWidth);
			}
			Graphics()->SetColor(1, 1, 1, 1);
			Line.m_QuadContainerIndex = Graphics()->CreateRectQuadContainer(Begin, y, FullWidth, Line.m_aYOffset[OffsetType], MessageRounding(), IGraphics::CORNER_ALL);
		}

		TextRender()->SetRenderFlags(CurRenderFlags);
		if(Line.m_TextContainerIndex.Valid())
			TextRender()->UploadTextContainer(Line.m_TextContainerIndex);
	}

	TextRender()->TextColor(TextRender()->DefaultTextColor());
}

void CChat::OnRender()
{
	if(Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
		return;

	UpdateChatTranslation();

	// send pending chat messages
	if(m_PendingChatCounter > 0 && m_LastChatSend + time_freq() < time())
	{
		CHistoryEntry *pEntry = m_History.Last();
		for(int i = m_PendingChatCounter - 1; pEntry; --i, pEntry = m_History.Prev(pEntry))
		{
			if(i == 0)
			{
				SendChat(pEntry->m_Team, pEntry->m_aText);
				break;
			}
		}
		--m_PendingChatCounter;
	}

	const float Height = 300.0f;
	const float Width = Height * Graphics()->ScreenAspect();
	Graphics()->MapScreenToSize(Width, Height);

	float x = 5.0f;
	float y = 300.0f - 20.0f * FontSize() / 6.0f;
	float ScaledFontSize = FontSize() * (8.0f / 6.0f);

	const bool ChatAnimated = g_Config.m_EcUiAnimations && g_Config.m_EcChatAnimation;
	float ChatOpenAlpha = 1.0f;
	if(ChatAnimated && m_Mode != MODE_NONE && m_ChatOpenTime != 0)
	{
		const float Age = (time() - m_ChatOpenTime) / (float)time_freq();
		const float Progress = std::clamp(Age / 0.15f, 0.0f, 1.0f);
		const float Eased = 1.0f - (1.0f - Progress) * (1.0f - Progress) * (1.0f - Progress);
		y += (1.0f - Eased) * 10.0f;
		ChatOpenAlpha = 0.25f + 0.75f * Eased;
	}

	if(m_Mode != MODE_NONE)
	{
		// render chat input
		CTextCursor InputCursor;
		InputCursor.SetPosition(vec2(x, y));
		InputCursor.m_FontSize = ScaledFontSize;
		InputCursor.m_LineWidth = Width - 190.0f;

		if(m_Mode == MODE_ALL)
			TextRender()->TextEx(&InputCursor, Localize("All"));
		else if(m_Mode == MODE_TEAM)
			TextRender()->TextEx(&InputCursor, Localize("Team"));
		else
			TextRender()->TextEx(&InputCursor, Localize("Chat"));

		TextRender()->TextEx(&InputCursor, ": ");

		const float MessageMaxWidth = InputCursor.m_LineWidth - (InputCursor.m_X - InputCursor.m_StartX);
		const CUIRect ClippingRect = {InputCursor.m_X, InputCursor.m_Y, MessageMaxWidth, 2.25f * InputCursor.m_FontSize};
		const float XScale = Graphics()->ScreenWidth() / Width;
		const float YScale = Graphics()->ScreenHeight() / Height;
		Graphics()->ClipEnable((int)(ClippingRect.x * XScale), (int)(ClippingRect.y * YScale), (int)(ClippingRect.w * XScale), (int)(ClippingRect.h * YScale));

		float ScrollOffset = m_Input.GetScrollOffset();
		float ScrollOffsetChange = m_Input.GetScrollOffsetChange();

		m_Input.Activate(EInputPriority::CHAT); // Ensure that the input is active
		const CUIRect InputCursorRect = {InputCursor.m_X, InputCursor.m_Y - ScrollOffset, 0.0f, 0.0f};
		const bool WasChanged = m_Input.WasChanged();
		const bool WasCursorChanged = m_Input.WasCursorChanged();
		const bool Changed = WasChanged || WasCursorChanged;

		UpdateTypingAnimations();

		std::vector<STextColorSplit> vTypingSplits;
		const char *pDisplayedInput = m_Input.GetDisplayedString();
		if(ChatAnimated)
		{
			for(auto It = m_vTypingAnims.begin(); It != m_vTypingAnims.end();)
			{
				const float Age = (time() - It->m_StartTime) / (float)time_freq();
				if(Age > 0.12f || It->m_ByteIndex < 0 || It->m_ByteIndex + It->m_ByteLength > (int)str_length(pDisplayedInput))
				{
					It = m_vTypingAnims.erase(It);
					continue;
				}
				vTypingSplits.emplace_back(It->m_ByteIndex, It->m_ByteLength, ColorRGBA(1.0f, 1.0f, 1.0f, 0.0f));
				++It;
			}
		}

		const STextBoundingBox BoundingBox = m_Input.Render(&InputCursorRect, InputCursor.m_FontSize, TEXTALIGN_TL, Changed, MessageMaxWidth, 0.0f, vTypingSplits);

		if(!vTypingSplits.empty())
		{
			char aPrefix[MAX_CHAT_LENGTH];
			for(const STypingAnim &Anim : m_vTypingAnims)
			{
				const float Age = (time() - Anim.m_StartTime) / (float)time_freq();
				const float Progress = std::clamp(Age / 0.12f, 0.0f, 1.0f);
				const float Eased = 1.0f - (1.0f - Progress) * (1.0f - Progress) * (1.0f - Progress);
				if(Anim.m_ByteIndex < 0 || Anim.m_ByteIndex + Anim.m_ByteLength > (int)str_length(pDisplayedInput))
					continue;

				str_truncate(aPrefix, sizeof(aPrefix), pDisplayedInput, Anim.m_ByteIndex);
				CTextCursor MeasureCursor;
				MeasureCursor.SetPosition(vec2(InputCursor.m_X, InputCursor.m_Y - ScrollOffset));
				MeasureCursor.m_FontSize = InputCursor.m_FontSize;
				MeasureCursor.m_LineWidth = MessageMaxWidth;
				TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.0f);
				TextRender()->TextEx(&MeasureCursor, aPrefix);

				char aGlyph[8] = "";
				str_truncate(aGlyph, sizeof(aGlyph), pDisplayedInput + Anim.m_ByteIndex, Anim.m_ByteLength);
				CTextCursor GlyphCursor;
				GlyphCursor.SetPosition(vec2(MeasureCursor.m_X, MeasureCursor.m_Y - 2.0f * (1.0f - Eased)));
				GlyphCursor.m_FontSize = InputCursor.m_FontSize;
				GlyphCursor.m_LineWidth = MessageMaxWidth;
				TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.3f + 0.7f * Eased);
				TextRender()->TextEx(&GlyphCursor, aGlyph);
			}
			TextRender()->TextColor(TextRender()->DefaultTextColor());
		}

		Graphics()->ClipDisable();

		// Scroll up or down to keep the caret inside the clipping rect
		const float CaretPositionY = m_Input.GetCaretPosition().y - ScrollOffsetChange;
		if(CaretPositionY < ClippingRect.y)
			ScrollOffsetChange -= ClippingRect.y - CaretPositionY;
		else if(CaretPositionY + InputCursor.m_FontSize > ClippingRect.y + ClippingRect.h)
			ScrollOffsetChange += CaretPositionY + InputCursor.m_FontSize - (ClippingRect.y + ClippingRect.h);

		Ui()->DoSmoothScrollLogic(&ScrollOffset, &ScrollOffsetChange, ClippingRect.h, BoundingBox.m_H);

		m_Input.SetScrollOffset(ScrollOffset);
		m_Input.SetScrollOffsetChange(ScrollOffsetChange);

		// Autocompletion hint
		if(m_Input.GetString()[0] == '/' && m_Input.GetString()[1] != '\0' && !m_vServerCommands.empty())
		{
			for(const auto &Command : m_vServerCommands)
			{
				if(str_startswith_nocase(Command.m_aName, m_Input.GetString() + 1))
				{
					InputCursor.m_X = InputCursor.m_X + TextRender()->TextWidth(InputCursor.m_FontSize, m_Input.GetString(), -1, InputCursor.m_LineWidth);
					InputCursor.m_Y = m_Input.GetCaretPosition().y;
					TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.5f);
					TextRender()->TextEx(&InputCursor, Command.m_aName + str_length(m_Input.GetString() + 1));
					TextRender()->TextColor(TextRender()->DefaultTextColor());
					break;
				}
			}
		}
	}

#if defined(CONF_VIDEORECORDER)
	if(!((g_Config.m_ClShowChat && !IVideo::Current()) || (g_Config.m_ClVideoShowChat && IVideo::Current())))
#else
	if(!g_Config.m_ClShowChat)
#endif
		return;

	m_BacklogCurLine = std::clamp(m_BacklogCurLine, 0, GetMaxBacklogCurLine());

	y -= ScaledFontSize;

	OnPrepareLines(y);

	bool IsScoreBoardOpen = GameClient()->m_Scoreboard.IsActive() && (Graphics()->ScreenAspect() > 1.7f); // only assume scoreboard when screen ratio is widescreen(something around 16:9)

	int64_t Now = time();
	float HeightLimit = IsScoreBoardOpen ? 180.0f : (m_PrevShowChat ? 50.0f : 200.0f);
	int OffsetType = IsScoreBoardOpen ? 1 : 0;

	float RealMsgPaddingX = MessagePaddingX();
	float RealMsgPaddingY = MessagePaddingY();
	const int TeeSize = MessageTeeSize();
	float RealMsgPaddingTee = TeeSize + MESSAGE_TEE_PADDING_RIGHT;

	if(g_Config.m_ClChatOld)
	{
		RealMsgPaddingX = 0;
		RealMsgPaddingY = 0;
		RealMsgPaddingTee = 0;
	}

	m_NumLineRects = 0;
	m_LinesRendered = 0;
	const bool SelectionActive = m_Mode != MODE_NONE && g_Config.m_EcChatMouseSelection && (m_Selecting || m_HasSelection);
	m_vSelectionLines.clear();

	for(int i = 0; i < MAX_LINES; i++)
	{
		CLine &Line = m_aLines[((m_CurrentLine - i) + MAX_LINES) % MAX_LINES];
		if(!Line.m_Initialized)
			break;
		if(Now > Line.m_Time + 16 * time_freq() && !m_PrevShowChat)
			break;

		if(i < m_BacklogCurLine)
			continue;

		y -= Line.m_aYOffset[OffsetType];

		// cut off if msgs waste too much space
		if(y < HeightLimit)
			break;

		m_LinesRendered++;

		float Blend = Now > Line.m_Time + 14 * time_freq() && !m_PrevShowChat ? 1.0f - (Now - Line.m_Time - 14 * time_freq()) / (2.0f * time_freq()) : 1.0f;

		float SlideY = 0.0f;
		if(ChatAnimated)
		{
			const float LineAge = (Now - Line.m_Time) / (float)time_freq();
			if(LineAge >= 0.0f && LineAge < 0.15f)
			{
				const float Progress = std::clamp(LineAge / 0.15f, 0.0f, 1.0f);
				const float Eased = 1.0f - (1.0f - Progress) * (1.0f - Progress) * (1.0f - Progress);
				SlideY = (1.0f - Eased) * 8.0f;
				Blend *= 0.2f + 0.8f * Eased;
			}
			Blend *= ChatOpenAlpha;
		}

		if(Line.m_TextContainerIndex.Valid())
		{
			const STextBoundingBox BoundingBox = TextRender()->GetBoundingBoxTextContainer(Line.m_TextContainerIndex);
			const float TextStart = x + RealMsgPaddingX / 2.0f;
			const float RectY = y + RealMsgPaddingY / 2.0f - 2.0f;
			const float RectH = Line.m_aYOffset[OffsetType] - RealMsgPaddingY + 4.0f;

			if(m_NumLineRects < MAX_LINES)
			{
				m_aLineRects[m_NumLineRects].m_X = TextStart;
				m_aLineRects[m_NumLineRects].m_Y = RectY + SlideY;
				m_aLineRects[m_NumLineRects].m_W = BoundingBox.m_W;
				m_aLineRects[m_NumLineRects].m_H = RectH;
				m_NumLineRects++;
			}
		}

		// Draw backgrounds for messages in one batch
		if(!g_Config.m_ClChatOld)
		{
			Graphics()->TextureClear();
			if(Line.m_QuadContainerIndex != -1)
			{
				Graphics()->SetColor(color_cast<ColorRGBA>(ColorHSLA(g_Config.m_ClChatBackgroundColor, true)).WithMultipliedAlpha(Blend));
				Graphics()->RenderQuadContainerEx(Line.m_QuadContainerIndex, 0, -1, 0, ((y + SlideY + RealMsgPaddingY / 2.0f) - Line.m_TextYOffset));
			}
		}

		if(Line.m_TextContainerIndex.Valid())
		{
			if(!g_Config.m_ClChatOld && Line.m_pManagedTeeRenderInfo != nullptr)
			{
				CTeeRenderInfo &TeeRenderInfo = Line.m_pManagedTeeRenderInfo->TeeRenderInfo();
				const int TeeSize = MessageTeeSize();
				TeeRenderInfo.m_Size = TeeSize;

				float RowHeight = FontSize() + RealMsgPaddingY;
				float OffsetTeeY = TeeSize / 2.0f;
				float FullHeightMinusTee = RowHeight - TeeSize;

				RenderTools()->RenderIdleTeeAtCenter(&TeeRenderInfo, vec2(x + (RealMsgPaddingX + TeeSize) / 2.0f, y + SlideY + OffsetTeeY + FullHeightMinusTee / 2.0f), EMOTE_NORMAL, vec2(1, 0.1f), Blend);
			}

			const ColorRGBA TextColor = TextRender()->DefaultTextColor().WithMultipliedAlpha(Blend);
			const ColorRGBA TextOutlineColor = TextRender()->DefaultTextOutlineColor().WithMultipliedAlpha(Blend);
			TextRender()->RenderTextContainer(Line.m_TextContainerIndex, TextColor, TextOutlineColor, 0, (y + SlideY + RealMsgPaddingY / 2.0f) - Line.m_TextYOffset);
		}

		if(SelectionActive)
			RenderChatSelection(Line, y + SlideY, x, RealMsgPaddingX, RealMsgPaddingY, RealMsgPaddingTee, IsScoreBoardOpen);
	}

	if(SelectionActive)
	{
		m_SelectionText.clear();
		for(auto It = m_vSelectionLines.rbegin(); It != m_vSelectionLines.rend(); ++It)
		{
			if(!m_SelectionText.empty())
				m_SelectionText += '\n';
			m_SelectionText += *It;
		}
		m_HasSelection = !m_SelectionText.empty();
	}

	UpdateChatSelection(Height);
}

void CChat::ClearChatSelection()
{
	m_SelectionPress = vec2(0.0f, 0.0f);
	m_SelectionRelease = vec2(0.0f, 0.0f);
	m_Selecting = false;
	m_HasSelection = false;
	m_SelectionText.clear();
	m_vSelectionLines.clear();
}

void CChat::UpdateChatSelection(float Height)
{
	if(m_Mode == MODE_NONE || !g_Config.m_EcChatMouseSelection)
	{
		if(m_Selecting || m_HasSelection)
			ClearChatSelection();
		return;
	}

	const vec2 WindowSize = vec2(Graphics()->WindowWidth(), Graphics()->WindowHeight());
	const vec2 ScreenSize = vec2(Height * Graphics()->ScreenAspect(), Height);
	const vec2 Mouse = Input()->NativeMousePos() / WindowSize * ScreenSize;
	const bool MousePressed = Input()->NativeMousePressed(1);

	if(MousePressed && !m_Selecting)
	{
		bool OnMessage = false;
		for(int i = 0; i < m_NumLineRects; i++)
		{
			const SLineRect &Rect = m_aLineRects[i];
			if(Mouse.x >= Rect.m_X - 4.0f && Mouse.x <= Rect.m_X + Rect.m_W + 8.0f && Mouse.y >= Rect.m_Y && Mouse.y <= Rect.m_Y + Rect.m_H)
			{
				OnMessage = true;
				break;
			}
		}
		if(OnMessage)
		{
			m_Selecting = true;
			m_SelectionPress = Mouse;
			m_SelectionRelease = Mouse;
			m_HasSelection = false;
			m_SelectionText.clear();
		}
		else if(m_HasSelection)
		{
			ClearChatSelection();
		}
	}
	else if(m_Selecting)
	{
		m_SelectionRelease = Mouse;
		if(!MousePressed)
			m_Selecting = false;
	}
}

void CChat::RenderChatSelection(CLine &Line, float y, float x, float RealMsgPaddingX, float RealMsgPaddingY, float RealMsgPaddingTee, bool IsScoreBoardOpen)
{
	const int OffsetType = IsScoreBoardOpen ? 1 : 0;
	const float SelectionMinY = std::min(m_SelectionPress.y, m_SelectionRelease.y);
	const float SelectionMaxY = std::max(m_SelectionPress.y, m_SelectionRelease.y);
	const float LineTop = y + RealMsgPaddingY / 2.0f;
	const float LineBottom = y + Line.m_aYOffset[OffsetType];
	if(LineBottom < SelectionMinY || LineTop > SelectionMaxY)
		return;

	const float TextBegin = x + RealMsgPaddingX / 2.0f;
	const float LineWidth = (IsScoreBoardOpen ? std::max(85.0f, FontSize() * 85.0f / 6.0f) : g_Config.m_ClChatWidth) - (RealMsgPaddingX * 1.5f) - RealMsgPaddingTee;

	CTextCursor PrefixCursor;
	PrefixCursor.SetPosition(vec2(TextBegin, y + RealMsgPaddingY / 2.0f));
	PrefixCursor.m_FontSize = FontSize();
	PrefixCursor.m_LineWidth = LineWidth;
	PrefixCursor.m_Flags = TEXTFLAG_RENDER;
	PrefixCursor.m_CalculateSelectionMode = TEXT_CURSOR_SELECTION_MODE_CALCULATE;
	PrefixCursor.m_PressMouse = m_SelectionPress;
	PrefixCursor.m_ReleaseMouse = m_SelectionRelease;
	if(Line.m_ClientId >= 0 && Line.m_aName[0] != '\0')
		PrefixCursor.m_X += RealMsgPaddingTee;

	TextRender()->TextSelectionColor(TextRender()->DefaultTextSelectionColor());
	TextRender()->TextColor(0.0f, 0.0f, 0.0f, 0.0f);
	TextRender()->TextEx(&PrefixCursor, Line.m_aRenderedPrefix, -1);

	CTextCursor MsgCursor = PrefixCursor;
	MsgCursor.m_LongestLineWidth = 0.0f;
	if(!IsScoreBoardOpen && !g_Config.m_ClChatOld)
	{
		MsgCursor.m_StartX = PrefixCursor.m_X;
		MsgCursor.m_LineWidth -= PrefixCursor.m_LongestLineWidth;
	}
	TextRender()->TextEx(&MsgCursor, Line.m_aRenderedText, -1);
	TextRender()->TextColor(TextRender()->DefaultTextColor());

	// Both cursors keep counting glyphs across the appended text, so the selection indexes
	// returned after rendering the message are already offsets into the full "prefix + text".
	const int PrefixSelStart = std::min(PrefixCursor.m_SelectionStart, PrefixCursor.m_SelectionEnd);
	const int PrefixSelEnd = std::max(PrefixCursor.m_SelectionStart, PrefixCursor.m_SelectionEnd);
	const int MsgSelStart = std::min(MsgCursor.m_SelectionStart, MsgCursor.m_SelectionEnd);
	const int MsgSelEnd = std::max(MsgCursor.m_SelectionStart, MsgCursor.m_SelectionEnd);

	int SelStart = -1;
	int SelEnd = -1;
	if(PrefixSelEnd > PrefixSelStart)
	{
		SelStart = PrefixSelStart;
		SelEnd = PrefixSelEnd;
	}
	if(MsgSelEnd > MsgSelStart)
	{
		SelStart = SelStart < 0 ? MsgSelStart : std::min(SelStart, MsgSelStart);
		SelEnd = SelEnd < 0 ? MsgSelEnd : std::max(SelEnd, MsgSelEnd);
	}
	if(SelStart < 0 || SelEnd <= SelStart)
		return;

	char aDisplay[MAX_CHAT_LENGTH * 2];
	str_format(aDisplay, sizeof(aDisplay), "%s%s", Line.m_aRenderedPrefix, Line.m_aRenderedText);

	const size_t OffStart = str_utf8_offset_chars_to_bytes(aDisplay, SelStart);
	const size_t OffEnd = str_utf8_offset_chars_to_bytes(aDisplay, SelEnd);
	if(OffEnd > OffStart && OffEnd <= str_length(aDisplay))
	{
		m_vSelectionLines.emplace_back(aDisplay + OffStart, OffEnd - OffStart);
	}
}

void CChat::CopyChatSelection()
{
	if(m_SelectionText.empty())
		return;
	Input()->SetClipboardText(m_SelectionText.c_str());
}

int CChat::NumInitializedLines() const
{
	int Count = 0;
	for(int i = 0; i < MAX_LINES; i++)
	{
		if(!m_aLines[((m_CurrentLine - i) + MAX_LINES) % MAX_LINES].m_Initialized)
			break;
		Count++;
	}
	return Count;
}

int CChat::GetMaxBacklogCurLine() const
{
	return std::max(0, NumInitializedLines() - std::max(1, m_LinesRendered));
}

int CChat::GetLinesToScroll(int Direction, int LinesToScroll) const
{
	const int RenderableLines = NumInitializedLines();
	int LinesToSkip = Direction == -1 ? m_BacklogCurLine + m_LinesRendered : m_BacklogCurLine - 1;
	LinesToSkip = std::clamp(LinesToSkip, 0, std::max(0, RenderableLines));

	int Amount;
	if(Direction == -1)
	{
		const int RemainingAbove = std::max(0, RenderableLines - LinesToSkip);
		Amount = LinesToScroll <= 0 ? RemainingAbove : std::min(RemainingAbove, LinesToScroll);
	}
	else
	{
		Amount = LinesToScroll <= 0 ? m_BacklogCurLine : std::min(m_BacklogCurLine, LinesToScroll);
	}
	return std::max(0, Amount);
}

void CChat::ScrollToTop()
{
	m_BacklogCurLine += GetLinesToScroll(-1, -1);
}

void CChat::ScrollToBottom()
{
	m_BacklogCurLine = 0;
}

void CChat::ScrollPageUp()
{
	m_BacklogCurLine += GetLinesToScroll(-1, std::max(1, m_LinesRendered));
}

void CChat::ScrollPageDown()
{
	m_BacklogCurLine = std::max(0, m_BacklogCurLine - GetLinesToScroll(1, std::max(1, m_LinesRendered)));
}

void CChat::EnsureCoherentFontSize() const
{
	// Adjust font size based on width
	if(g_Config.m_ClChatWidth / (float)g_Config.m_ClChatFontSize >= CHAT_FONTSIZE_WIDTH_RATIO)
		return;

	// We want to keep a ration between font size and font width so that we don't have a weird rendering
	g_Config.m_ClChatFontSize = g_Config.m_ClChatWidth / CHAT_FONTSIZE_WIDTH_RATIO;
}

void CChat::EnsureCoherentWidth() const
{
	// Adjust width based on font size
	if(g_Config.m_ClChatWidth / (float)g_Config.m_ClChatFontSize >= CHAT_FONTSIZE_WIDTH_RATIO)
		return;

	// We want to keep a ration between font size and font width so that we don't have a weird rendering
	g_Config.m_ClChatWidth = CHAT_FONTSIZE_WIDTH_RATIO * g_Config.m_ClChatFontSize;
}

// ----- chat translation -----

void CChat::StartChatTranslateRequest(const char *pText, const char *pTarget, bool Outgoing, int Backend)
{
	if(Backend < 0)
		Backend = g_Config.m_EcChatTranslateBackend == 2 ? TRANSLATE_BACKEND_GOOGLE : TRANSLATE_BACKEND_BING;

	std::shared_ptr<IHttpRequest> pRequest;
	if(Backend == TRANSLATE_BACKEND_BING)
	{
		char aEscaped[MAX_CHAT_LENGTH * 2];
		TranslateJsonEscape(pText, aEscaped, sizeof(aEscaped));
		char aBody[sizeof(aEscaped) + 8];
		str_format(aBody, sizeof(aBody), "[\"%s\"]", aEscaped);
		char aUrl[256];
		str_format(aUrl, sizeof(aUrl), "https://edge.microsoft.com/translate/translatetext?to=%s", pTarget);
		pRequest = HttpGet(aUrl);
		pRequest->PostJson(aBody);
	}
	else
	{
		char aEncoded[MAX_CHAT_LENGTH * 3];
		TranslateUrlEncode(pText, aEncoded, sizeof(aEncoded));
		char aBody[sizeof(aEncoded) + 64];
		str_format(aBody, sizeof(aBody), "client=gtx&sl=auto&dt=t&tl=%s&q=%s", pTarget, aEncoded);
		pRequest = HttpGet("https://translate.googleapis.com/translate_a/single");
		pRequest->HeaderString("Content-Type", "application/x-www-form-urlencoded");
		pRequest->Post((const unsigned char *)aBody, str_length(aBody));
	}
	pRequest->Timeout(CTimeout{4000, 8000, 0, 0});
	pRequest->FailOnErrorStatus(false);
	pRequest->LogProgress(HTTPLOG::FAILURE);
	Http()->Run(pRequest);

	if(Outgoing)
	{
		m_TranslateOutgoingBackend = Backend;
		m_pTranslateOutgoingRequest = pRequest;
	}
	else
	{
		m_TranslateIncomingBackend = Backend;
		m_pTranslateIncomingRequest = pRequest;
	}
}

void CChat::UpdateTypingAnimations()
{
	const char *pCurrent = m_Input.GetDisplayedString();
	if(!g_Config.m_EcUiAnimations || !g_Config.m_EcChatAnimation || m_Input.HasSelection())
	{
		m_vTypingAnims.clear();
		m_PrevInputText.assign(pCurrent);
		return;
	}

	const size_t CurrentLen = str_length(pCurrent);
	const size_t PreviousLen = m_PrevInputText.size();
	size_t Prefix = 0;
	while(Prefix < CurrentLen && Prefix < PreviousLen && pCurrent[Prefix] == m_PrevInputText[Prefix])
		Prefix++;

	if(CurrentLen > PreviousLen && Prefix == PreviousLen)
	{
		for(size_t Pos = PreviousLen; Pos < CurrentLen;)
		{
			const size_t Next = (size_t)str_utf8_forward(pCurrent, (int)Pos);
			STypingAnim Anim;
			Anim.m_StartTime = time();
			Anim.m_ByteIndex = (int)Pos;
			Anim.m_ByteLength = (int)(Next - Pos);
			m_vTypingAnims.push_back(Anim);
			Pos = Next;
		}
	}
	else if(CurrentLen != PreviousLen)
	{
		m_vTypingAnims.clear();
	}
	m_PrevInputText.assign(pCurrent);
}
void CChat::UpdateChatTranslation()
{
	if(m_pTranslateIncomingRequest && m_pTranslateIncomingRequest->Done())
	{
		CLine *pLine = (m_TranslateLineIndex >= 0 && m_TranslateLineIndex < MAX_LINES) ? &m_aLines[m_TranslateLineIndex] : nullptr;
		bool Success = false;
		bool SameLanguage = false;
		char aDetectedLanguage[16] = "";
		if(pLine && m_pTranslateIncomingRequest->State() == EHttpState::DONE && m_pTranslateIncomingRequest->StatusCode() == 200)
		{
			json_value *pJson = m_pTranslateIncomingRequest->ResultJson();
			Success = ParseTranslation(m_TranslateIncomingBackend, pJson, pLine->m_aTranslatedText, aDetectedLanguage);
			json_value_free(pJson);
			if(Success && str_comp_nocase(aDetectedLanguage, g_Config.m_EcChatTranslateTarget) == 0)
			{
				SameLanguage = true;
				Success = false;
			}
		}

		const int NextBackend = Success ? -1 : TranslateNextBackend(m_TranslateIncomingBackend);
		if(pLine && !Success && !SameLanguage && NextBackend >= 0)
		{
			m_TranslateBackend = NextBackend;
			m_pTranslateIncomingRequest = nullptr;
			StartChatTranslateRequest(pLine->m_aText, g_Config.m_EcChatTranslateTarget, false, NextBackend);
		}
		else
		{
			if(Success)
				m_TranslateBackend = m_TranslateIncomingBackend;
			if(pLine && pLine->m_Initialized && pLine->m_TranslateState == 1)
			{
				if(Success && g_Config.m_EcChatTranslateApostrophes)
					TranslateStripApostrophes(pLine->m_aTranslatedText);
				pLine->m_TranslateState = Success ? 2 : 3;
				// rebuild this line with the translated text
				TextRender()->DeleteTextContainer(pLine->m_TextContainerIndex);
				Graphics()->DeleteQuadContainer(pLine->m_QuadContainerIndex);
				pLine->m_aYOffset[0] = -1.0f;
				pLine->m_aYOffset[1] = -1.0f;
			}
			m_pTranslateIncomingRequest = nullptr;
			m_TranslateLineIndex = -1;
		}
	}

	if(m_pTranslateOutgoingRequest && m_pTranslateOutgoingRequest->Done())
	{
		char aText[MAX_CHAT_LENGTH];
		aText[0] = '\0';
		bool Success = false;
		bool SameLanguage = false;
		char aDetectedLanguage[16] = "";
		if(m_pTranslateOutgoingRequest->State() == EHttpState::DONE && m_pTranslateOutgoingRequest->StatusCode() == 200)
		{
			json_value *pJson = m_pTranslateOutgoingRequest->ResultJson();
			Success = ParseTranslation(m_TranslateOutgoingBackend, pJson, aText, aDetectedLanguage);
			json_value_free(pJson);
			if(Success && str_comp_nocase(aDetectedLanguage, g_Config.m_EcChatTranslateOutTarget) == 0)
			{
				SameLanguage = true;
				Success = false;
			}
		}

		const int NextBackend = Success ? -1 : TranslateNextBackend(m_TranslateOutgoingBackend);
		if(!Success && !SameLanguage && NextBackend >= 0)
		{
			m_TranslateBackend = NextBackend;
			m_pTranslateOutgoingRequest = nullptr;
			StartChatTranslateRequest(m_aTranslateOutgoingText, g_Config.m_EcChatTranslateOutTarget, true, NextBackend);
		}
		else
		{
			if(Success)
				m_TranslateBackend = m_TranslateOutgoingBackend;
			if(Success && g_Config.m_EcChatTranslateApostrophes)
				TranslateStripApostrophes(aText);
			m_pTranslateOutgoingRequest = nullptr;

			// on failure the original message is sent
			m_SendingTranslated = true;
			SendChat(m_TranslateOutgoingTeam, Success ? aText : m_aTranslateOutgoingText);
			m_SendingTranslated = false;
		}
	}

	if(!g_Config.m_EcChatTranslate || m_pTranslateIncomingRequest || m_TranslateLineIndex != -1 || time() < m_NextTranslateTime)
		return;

	// translate the newest incoming line that is still waiting, one request at a time
	for(int i = 0; i < MAX_LINES; i++)
	{
		const int Index = (m_CurrentLine - i + MAX_LINES) % MAX_LINES;
		CLine &Line = m_aLines[Index];
		if(!Line.m_Initialized || Line.m_TranslateState != 1)
			continue;
		if(time() > Line.m_Time + 16 * time_freq())
		{
			Line.m_TranslateState = 0;
			continue;
		}
		m_TranslateLineIndex = Index;
		StartChatTranslateRequest(Line.m_aText, g_Config.m_EcChatTranslateTarget, false, m_TranslateBackend);
		m_NextTranslateTime = time() + time_freq() / 2;
		break;
	}
}

// ----- send functions -----

void CChat::SendChat(int Team, const char *pLine)
{
	// don't send empty messages
	if(*str_utf8_skip_whitespaces(pLine) == '\0')
		return;

	// translate own messages first, the translated text is sent when the response arrives
	if(!m_SendingTranslated && g_Config.m_EcChatTranslate && g_Config.m_EcChatTranslateOutgoing && pLine[0] != '/' && !m_pTranslateOutgoingRequest)
	{
		str_copy(m_aTranslateOutgoingText, pLine);
		m_TranslateOutgoingTeam = Team;
		StartChatTranslateRequest(pLine, g_Config.m_EcChatTranslateOutTarget, true, m_TranslateBackend);
		return;
	}

	m_LastChatSend = time();

	if(GameClient()->Client()->IsSixup())
	{
		protocol7::CNetMsg_Cl_Say Msg7;
		Msg7.m_Mode = Team == 1 ? protocol7::CHAT_TEAM : protocol7::CHAT_ALL;
		Msg7.m_Target = -1;
		Msg7.m_pMessage = pLine;
		Client()->SendPackMsgActive(&Msg7, MSGFLAG_VITAL, true);
		return;
	}

	// send chat message
	CNetMsg_Cl_Say Msg;
	Msg.m_Team = Team;
	Msg.m_pMessage = pLine;
	Client()->SendPackMsgActive(&Msg, MSGFLAG_VITAL);
}

void CChat::SendChatQueued(const char *pLine)
{
	if(!pLine || str_length(pLine) < 1)
		return;

	bool AddEntry = false;

	if(m_LastChatSend + time_freq() < time())
	{
		SendChat(m_Mode == MODE_ALL ? 0 : 1, pLine);
		AddEntry = true;
	}
	else if(m_PendingChatCounter < 3)
	{
		++m_PendingChatCounter;
		AddEntry = true;
	}

	if(AddEntry)
	{
		const int Length = str_length(pLine);
		CHistoryEntry *pEntry = m_History.Allocate(sizeof(CHistoryEntry) + Length);
		pEntry->m_Team = m_Mode == MODE_ALL ? 0 : 1;
		str_copy(pEntry->m_aText, pLine, Length + 1);
	}
}
