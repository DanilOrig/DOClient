#ifndef GAME_CLIENT_COMPONENTS_SPECTATEYOU_H
#define GAME_CLIENT_COMPONENTS_SPECTATEYOU_H

#include <base/types.h>

#include <engine/shared/protocol.h>

#include <engine/console.h>
#include <base/time.h>

#include <game/client/component.h>

class CSpectateYou : public CComponent
{
public:
	int Sizeof() const override { return sizeof(*this); }

	void OnConsoleInit() override;
	void OnRender() override;

	int NumSpectators() const { return m_NumSpectators; }
	// fills pIds with client ids of players watching us and returns how many were filled;
	// pCount receives the spectator count reported by the server
	int GetSpectatorData(int *pIds, int MaxIds, int *pCount) const;
	void OnUpdate() override;

private:
	int m_CachedIds[MAX_CLIENTS] = {};
	int m_CachedNumIds = 0;
	int m_CachedCount = 0;
	int64_t m_LastValidTime = 0;
	static void ConSpecDebug(IConsole::IResult *pResult, void *pUserData);
	int m_NumSpectators = 0;
	char m_aaNames[MAX_CLIENTS][MAX_NAME_LENGTH] = {};
};

#endif
