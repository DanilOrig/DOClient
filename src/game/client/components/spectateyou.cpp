#include "spectateyou.h"

#include <base/math.h>

#include <algorithm>

#include <engine/console.h>
#include <engine/graphics.h>
#include <engine/shared/config.h>
#include <engine/textrender.h>

#include <generated/protocol.h>
#include <generated/protocol7.h>

#include <game/client/gameclient.h>
#include <game/client/ui.h>

void CSpectateYou::OnConsoleInit()
{
	Console()->Register("spec_debug", "", CFGFLAG_CLIENT, ConSpecDebug, this, "Print spectator detection debug info");
}

void CSpectateYou::ConSpecDebug(IConsole::IResult *pResult, void *pUserData)
{
	CSpectateYou *pThis = (CSpectateYou *)pUserData;
	auto *pGameClient = pThis->GameClient();

	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "sixup=%d spectators count=%d", pThis->Client()->IsSixup() ? 1 : 0,
		pGameClient->m_Snap.m_pSpectatorCount ? pGameClient->m_Snap.m_pSpectatorCount->m_NumSpectators : -1);
	pGameClient->Echo(aBuf);

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		const auto &Client = pGameClient->m_aClients[i];
		if(Client.m_aName[0] == '\0')
			continue;

		const CNetObj_PlayerInfo *pInfo = pGameClient->m_Snap.m_apPlayerInfos[i];
		const int Team = pInfo ? pInfo->m_Team : -1;
		int Watching = 0;
		if(pThis->Client()->IsSixup())
			Watching = (pThis->Client()->m_TranslationContext.m_aClients[i].m_PlayerFlags7 & protocol7::PLAYERFLAG_WATCHING) != 0;

		str_format(aBuf, sizeof(aBuf), "id=%d name='%s' active=%d team=%d spec=%d paused=%d watching=%d",
			i, Client.m_aName, Client.m_Active ? 1 : 0, Team, Client.m_Spec ? 1 : 0, Client.m_Paused ? 1 : 0, Watching);
		pGameClient->Echo(aBuf);
	}
}


static int CollectSpectatorIds(CGameClient *pGameClient, IClient *pClient, int *pIds, int MaxIds, int *pCount)
{
	int aAdded[MAX_CLIENTS] = {};
	int Num = 0;
	const int LocalId = pGameClient->m_Snap.m_LocalClientId;
	auto Add = [&](int ClientId) {
		if(ClientId < 0 || ClientId >= MAX_CLIENTS || aAdded[ClientId] || ClientId == LocalId)
			return;
		aAdded[ClientId] = 1;
		if(Num < MaxIds)
			pIds[Num] = ClientId;
		Num++;
	};

	int Count = 0;
	if(pClient->IsSixup())
	{
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(pClient->m_TranslationContext.m_aClients[i].m_PlayerFlags7 & protocol7::PLAYERFLAG_WATCHING)
				Add(i);
		}
		Count = Num;
	}
	else
	{
		const CNetObj_SpectatorCount *pSpectatorCount = pGameClient->m_Snap.m_pSpectatorCount;
		if(pSpectatorCount == nullptr)
		{
			if(pCount != nullptr)
				*pCount = 0;
			return 0;
		}
		Count = pSpectatorCount->m_NumSpectators;
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			const auto &Client = pGameClient->m_aClients[i];
			if(!Client.m_Active)
				continue;
			const CNetObj_PlayerInfo *pInfo = pGameClient->m_Snap.m_apPlayerInfos[i];
			const bool IsSpectator = (pInfo && pInfo->m_Team == TEAM_SPECTATORS) || Client.m_Spec || Client.m_Paused;
			if(IsSpectator)
				Add(i);
		}
	}

	if(pCount != nullptr)
		*pCount = Count;
	return Num;
}

void CSpectateYou::OnUpdate()
{
	if(!g_Config.m_EcShowSpectators)
	{
		m_CachedNumIds = 0;
		m_CachedCount = 0;
		return;
	}

	int aIds[MAX_CLIENTS];
	int Count = 0;
	const int NumIds = CollectSpectatorIds(GameClient(), Client(), aIds, MAX_CLIENTS, &Count);
	m_CachedCount = Count;

	// in 0.6 the server only sends the number of spectators, so the names are a guess:
	// accept them only while the guess matches the count and keep the last good list for a
	// couple of seconds, otherwise the names would flicker or disappear
	// the count is authoritative: as soon as the resolved list covers it, take exactly that many
	const bool Reliable = Client()->IsSixup() || (Count > 0 && NumIds >= Count);
	if(Reliable)
	{
		m_CachedNumIds = Client()->IsSixup() ? NumIds : (NumIds < Count ? NumIds : Count);
		for(int i = 0; i < m_CachedNumIds; i++)
			m_CachedIds[i] = aIds[i];
		m_LastValidTime = time_get();
	}
	else if(m_CachedNumIds > 0 && time_get() - m_LastValidTime > 2 * time_freq())
	{
		m_CachedNumIds = 0;
	}
}

int CSpectateYou::GetSpectatorData(int *pIds, int MaxIds, int *pCount) const
{
	if(pCount != nullptr)
		*pCount = m_CachedCount;
	const int Num = m_CachedNumIds < MaxIds ? m_CachedNumIds : MaxIds;
	for(int i = 0; i < Num; i++)
		pIds[i] = m_CachedIds[i];
	return Num;
}

void CSpectateYou::OnRender()
{
	// the panel is drawn by the HUD (spectator count element)
}
