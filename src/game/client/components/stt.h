/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_STT_H
#define GAME_CLIENT_COMPONENTS_STT_H

#include <game/client/component.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

class CStt : public CComponent
{
public:
	enum
	{
		STT_IDLE = 0,
		STT_RECORDING,
		STT_PROCESSING,
	};

	int Sizeof() const override { return sizeof(*this); }
	~CStt() override;

	void OnConsoleInit() override;
	void OnShutdown() override;
	bool OnInput(const IInput::CEvent &Event) override;
	void OnRender() override;

	void ToggleRecording();
	int CurrentKey() const;
	static int ParseKey(const char *pName);

private:
	std::thread m_Worker;
	std::mutex m_ThreadMutex;
	std::condition_variable m_ThreadCv;
	bool m_StartRequest = false;
	std::atomic<bool> m_Quit{false};
	std::atomic<bool> m_StopRequest{false};
	std::atomic<int> m_State{STT_IDLE};

	std::mutex m_TextMutex;
	std::string m_PendingText;
	bool m_HasPending = false;
	int64_t m_FlashStart = 0;

	void StartRecording();
	void StopRecording();
	void WorkerMain();
	void RecordSession();
	void PushRecognizedText(const char *pText);
};

#endif
