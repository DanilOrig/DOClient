#include "warlist.h"

#include <base/fs.h>
#include <base/io.h>
#include <base/log.h>
#include <base/math.h>

#include <engine/console.h>
#include <engine/shared/config.h>
#include <engine/storage.h>

#include <game/client/gameclient.h>

#include <cstdlib>

static const char *WARLIST_FILE = "warlist.cfg";

static bool WarNamesMatch(const char *pA, const char *pB)
{
	char aBufA[128] = "";
	char aBufB[128] = "";
	str_copy(aBufA, str_utf8_skip_whitespaces(pA));
	str_copy(aBufB, str_utf8_skip_whitespaces(pB));
	str_utf8_trim_right(aBufA);
	str_utf8_trim_right(aBufB);
	return aBufA[0] != '\0' && str_comp_nocase(aBufA, aBufB) == 0;
}

void CWarList::OnInit()
{
	m_vTypes.clear();
	CWarListType None;
	str_copy(None.m_aName, "none");
	None.m_Color = ColorRGBA(1.0f, 1.0f, 1.0f, 1.0f);
	None.m_Removable = false;
	m_vTypes.push_back(None);

	CWarListType Enemy;
	str_copy(Enemy.m_aName, "enemy");
	Enemy.m_Color = ColorRGBA(1.0f, 0.25f, 0.25f, 1.0f);
	Enemy.m_Removable = false;
	m_vTypes.push_back(Enemy);

	CWarListType Team;
	str_copy(Team.m_aName, "team");
	Team.m_Color = ColorRGBA(0.2f, 0.9f, 0.3f, 1.0f);
	Team.m_Removable = false;
	m_vTypes.push_back(Team);

	CWarListType Mute;
	str_copy(Mute.m_aName, "mute");
	Mute.m_Color = ColorRGBA(0.55f, 0.55f, 0.55f, 1.0f);
	Mute.m_Removable = false;
	m_vTypes.push_back(Mute);

	Load();
	UpdateCache();
}

void CWarList::OnConsoleInit()
{
	Console()->Register("war_name", "s[name] ?r[reason]", CFGFLAG_CLIENT, ConWarName, this, "Add a war entry for a player name");
	Console()->Register("war_clan", "s[clan] ?r[reason]", CFGFLAG_CLIENT, ConWarClan, this, "Add a war entry for a clan");
	Console()->Register("remove_war_name", "s[name]", CFGFLAG_CLIENT, ConRemoveWarName, this, "Remove war entries of a player name");
	Console()->Register("remove_war_clan", "s[clan]", CFGFLAG_CLIENT, ConRemoveWarClan, this, "Remove war entries of a clan");
	Console()->Register("add_war_entry", "s[type] s[name] s[clan] r[reason]", CFGFLAG_CLIENT, ConAddWarEntry, this, "Add a war entry of a specific type");
	Console()->Register("update_war_group", "i[index] s[name] i[color]", CFGFLAG_CLIENT, ConUpdateWarGroup, this, "Update or add a war group");
	Console()->Register("war_list", "", CFGFLAG_CLIENT, ConWarList, this, "Print the war list to the console");
	Console()->Register("war_debug", "", CFGFLAG_CLIENT, ConWarDebug, this, "Print war list matching debug information");
}

int CWarList::FindType(const char *pType) const
{
	for(size_t i = 0; i < m_vTypes.size(); i++)
	{
		if(str_comp_nocase(m_vTypes[i].m_aName, pType) == 0)
			return (int)i;
	}
	return -1;
}

bool CWarList::AddEntry(const char *pName, const char *pClan, const char *pReason, const char *pType)
{
	int Type = FindType(pType);
	if(Type < 0)
		Type = 1; // enemy by default

	if(pName[0] == '\0' && pClan[0] == '\0')
		return false;

	CWarListEntry Entry;
	str_copy(Entry.m_aName, pName);
	str_copy(Entry.m_aClan, pClan);
	str_copy(Entry.m_aReason, pReason);
	Entry.m_Type = Type;
	m_vEntries.push_back(Entry);
	UpdateCache();
	Save();
	return true;
}

bool CWarList::RemoveEntry(int Index)
{
	if(Index < 0 || Index >= (int)m_vEntries.size())
		return false;
	m_vEntries.erase(m_vEntries.begin() + Index);
	UpdateCache();
	Save();
	return true;
}

bool CWarList::RemoveEntry(const char *pName, const char *pClan)
{
	bool Removed = false;
	for(int i = (int)m_vEntries.size() - 1; i >= 0; i--)
	{
		const CWarListEntry &Entry = m_vEntries[i];
		if((pName[0] != '\0' && str_comp_nocase(Entry.m_aName, pName) == 0) || (pClan[0] != '\0' && str_comp_nocase(Entry.m_aClan, pClan) == 0))
		{
			m_vEntries.erase(m_vEntries.begin() + i);
			Removed = true;
		}
	}
	if(Removed)
	{
		UpdateCache();
		Save();
	}
	return Removed;
}

void CWarList::SetEntryType(int Index, int Type)
{
	if(Index < 0 || Index >= (int)m_vEntries.size() || Type < 0 || Type >= (int)m_vTypes.size())
		return;
	m_vEntries[Index].m_Type = Type;
	UpdateCache();
	Save();
}

void CWarList::SetTypeName(int Index, const char *pName)
{
	if(Index < 0 || Index >= (int)m_vTypes.size() || pName[0] == '\0')
		return;
	str_copy(m_vTypes[Index].m_aName, pName);
	UpdateCache();
	Save();
}

void CWarList::SetTypeColor(int Index, ColorRGBA Color)
{
	if(Index < 0 || Index >= (int)m_vTypes.size())
		return;
	m_vTypes[Index].m_Color = Color;
	UpdateCache();
}

bool CWarList::AddType(const char *pName, ColorRGBA Color)
{
	if(pName[0] == '\0' || FindType(pName) >= 0)
		return false;
	CWarListType Type;
	str_copy(Type.m_aName, pName);
	Type.m_Color = Color;
	m_vTypes.push_back(Type);
	UpdateCache();
	Save();
	return true;
}

bool CWarList::RemoveType(int Index)
{
	if(Index < 0 || Index >= (int)m_vTypes.size() || !m_vTypes[Index].m_Removable)
		return false;
	for(auto &Entry : m_vEntries)
	{
		if(Entry.m_Type == Index)
			Entry.m_Type = 0;
		else if(Entry.m_Type > Index)
			Entry.m_Type--;
	}
	m_vTypes.erase(m_vTypes.begin() + Index);
	UpdateCache();
	Save();
	return true;
}

void CWarList::UpdateCache()
{
	for(auto &Cache : m_aCache)
	{
		Cache.m_Type = -1;
		Cache.m_Color = ColorRGBA(1.0f, 1.0f, 1.0f, 1.0f);
		Cache.m_aReason[0] = '\0';
	}

	if(GameClient() == nullptr)
		return;

	for(int ClientId = 0; ClientId < MAX_CLIENTS; ClientId++)
	{
		const auto &Client = GameClient()->m_aClients[ClientId];
		if(Client.m_aName[0] == '\0')
			continue;

		for(const auto &Entry : m_vEntries)
		{
			const bool NameMatch = WarNamesMatch(Client.m_aName, Entry.m_aName);
			const bool ClanMatch = Entry.m_aClan[0] != '\0' && Client.m_aClan[0] != '\0' && WarNamesMatch(Client.m_aClan, Entry.m_aClan);
			if(!NameMatch && !ClanMatch)
				continue;

			if(Entry.m_Type >= 0 && Entry.m_Type < (int)m_vTypes.size())
			{
				m_aCache[ClientId].m_Type = Entry.m_Type;
				m_aCache[ClientId].m_Color = m_vTypes[Entry.m_Type].m_Color;
			}
			str_copy(m_aCache[ClientId].m_aReason, Entry.m_aReason);
			break;
		}
	}
}



const CWarListEntry *CWarList::FindEntryForClient(int ClientId) const
{
	if(ClientId < 0 || ClientId >= MAX_CLIENTS || GameClient() == nullptr)
		return nullptr;
	const auto &Client = GameClient()->m_aClients[ClientId];
	if(Client.m_aName[0] == '\0')
		return nullptr;
	for(const auto &Entry : m_vEntries)
	{
		const bool NameMatch = WarNamesMatch(Client.m_aName, Entry.m_aName);
		const bool ClanMatch = Entry.m_aClan[0] != '\0' && Client.m_aClan[0] != '\0' && WarNamesMatch(Client.m_aClan, Entry.m_aClan);
		if(NameMatch || ClanMatch)
			return &Entry;
	}
	return nullptr;
}

ColorRGBA CWarList::ColorForClient(int ClientId) const
{
	const CWarListEntry *pEntry = FindEntryForClient(ClientId);
	if(pEntry == nullptr || pEntry->m_Type < 0 || pEntry->m_Type >= (int)m_vTypes.size())
		return ColorRGBA(1.0f, 1.0f, 1.0f, 1.0f);
	return m_vTypes[pEntry->m_Type].m_Color;
}

const char *CWarList::ReasonForClient(int ClientId) const
{
	const CWarListEntry *pEntry = FindEntryForClient(ClientId);
	return pEntry != nullptr ? pEntry->m_aReason : "";
}

bool CWarList::IsMuted(int ClientId) const
{
	const CWarListEntry *pEntry = FindEntryForClient(ClientId);
	if(pEntry == nullptr || pEntry->m_Type < 0 || pEntry->m_Type >= (int)m_vTypes.size())
		return false;
	return str_comp(m_vTypes[pEntry->m_Type].m_aName, "mute") == 0;
}

void CWarList::OnNewSnapshot()
{
	UpdateCache();
}

void CWarList::OnUpdate()
{
	UpdateCache();
}

void CWarList::Load()
{
	m_vEntries.clear();

	void *pFileData = nullptr;
	unsigned int FileSize = 0;
	if(!Storage()->ReadFile(WARLIST_FILE, IStorage::TYPE_SAVE, &pFileData, &FileSize) || pFileData == nullptr)
		return;

	char *pData = (char *)pFileData;
	std::string Text(pData, FileSize);
	free(pFileData);

	size_t Pos = 0;
	while(Pos < Text.size())
	{
		size_t End = Text.find('\n', Pos);
		if(End == std::string::npos)
			End = Text.size();
		std::string Line = Text.substr(Pos, End - Pos);
		Pos = End + 1;
		while(!Line.empty() && (Line.back() == '\r' || Line.back() == ' '))
			Line.pop_back();

		if(Line.rfind("group ", 0) == 0)
		{
			const char *pLine = Line.c_str() + 6;
			char aName[WARLIST_MAX_TYPE_NAME] = "";
			int Len = 0;
			while(pLine[Len] != '\0' && pLine[Len] != ' ' && Len < WARLIST_MAX_TYPE_NAME - 1)
			{
				aName[Len] = pLine[Len];
				Len++;
			}
			aName[Len] = '\0';
			float r = 1.0f, g = 1.0f, b = 1.0f;
			int Removable = 1;
			sscanf(pLine + Len, "%f %f %f %d", &r, &g, &b, &Removable);
			int Existing = FindType(aName);
			if(Existing >= 0)
			{
				m_vTypes[Existing].m_Color = ColorRGBA(r, g, b, 1.0f);
			}
			else
			{
				CWarListType Type;
				str_copy(Type.m_aName, aName);
				Type.m_Color = ColorRGBA(r, g, b, 1.0f);
				Type.m_Removable = Removable != 0;
				m_vTypes.push_back(Type);
			}
		}
		else if(Line.rfind("war ", 0) == 0)
		{
			const char *pLine = Line.c_str() + 4;
			char aType[WARLIST_MAX_TYPE_NAME] = "";
			char aName[MAX_NAME_LENGTH] = "";
			char aClan[MAX_CLAN_LENGTH] = "";
			char aReason[WARLIST_MAX_REASON] = "";
			int Matched = sscanf(pLine, "%15s %63s %63s %127[^\n]", aType, aName, aClan, aReason);
			if(Matched < 3)
				continue;
			if(str_comp(aName, "-") == 0)
				aName[0] = '\0';
			if(str_comp(aClan, "-") == 0)
				aClan[0] = '\0';

			int Type = FindType(aType);
			if(Type < 0)
				Type = 1;

			CWarListEntry Entry;
			str_copy(Entry.m_aName, aName);
			str_copy(Entry.m_aClan, aClan);
			str_copy(Entry.m_aReason, Matched >= 4 ? aReason : "");
			Entry.m_Type = Type;
			m_vEntries.push_back(Entry);
		}
	}
}

void CWarList::Save() const
{
	IOHANDLE File = Storage()->OpenFile(WARLIST_FILE, IOFLAG_WRITE, IStorage::TYPE_SAVE);
	if(!File)
	{
		log_error("warlist", "Failed to save '%s'", WARLIST_FILE);
		return;
	}

	for(const auto &Type : m_vTypes)
	{
		char aLine[128];
		str_format(aLine, sizeof(aLine), "group %s %.3f %.3f %.3f %d\n", Type.m_aName, Type.m_Color.r, Type.m_Color.g, Type.m_Color.b, Type.m_Removable ? 1 : 0);
		io_write(File, aLine, str_length(aLine));
	}

	for(const auto &Entry : m_vEntries)
	{
		char aLine[512];
		const char *pType = (Entry.m_Type >= 0 && Entry.m_Type < (int)m_vTypes.size()) ? m_vTypes[Entry.m_Type].m_aName : "enemy";
		str_format(aLine, sizeof(aLine), "war %s %s %s %s\n", pType,
			Entry.m_aName[0] != '\0' ? Entry.m_aName : "-",
			Entry.m_aClan[0] != '\0' ? Entry.m_aClan : "-",
			Entry.m_aReason);
		io_write(File, aLine, str_length(aLine));
	}

	io_close(File);
}

void CWarList::ConWarName(IConsole::IResult *pResult, void *pUserData)
{
	CWarList *pThis = (CWarList *)pUserData;
	const char *pName = pResult->GetString(0);
	const char *pReason = pResult->NumArguments() > 1 ? pResult->GetString(1) : "";
	if(pThis->AddEntry(pName, "", pReason, "enemy"))
		pThis->GameClient()->Echo("War entry added");
	else
		pThis->GameClient()->Echo("Invalid name");
}

void CWarList::ConWarClan(IConsole::IResult *pResult, void *pUserData)
{
	CWarList *pThis = (CWarList *)pUserData;
	const char *pClan = pResult->GetString(0);
	const char *pReason = pResult->NumArguments() > 1 ? pResult->GetString(1) : "";
	if(pThis->AddEntry("", pClan, pReason, "enemy"))
		pThis->GameClient()->Echo("Clan war entry added");
	else
		pThis->GameClient()->Echo("Invalid clan");
}

void CWarList::ConRemoveWarName(IConsole::IResult *pResult, void *pUserData)
{
	CWarList *pThis = (CWarList *)pUserData;
	if(pThis->RemoveEntry(pResult->GetString(0), ""))
		pThis->GameClient()->Echo("War entries removed");
	else
		pThis->GameClient()->Echo("No matching war entries");
}

void CWarList::ConRemoveWarClan(IConsole::IResult *pResult, void *pUserData)
{
	CWarList *pThis = (CWarList *)pUserData;
	if(pThis->RemoveEntry("", pResult->GetString(0)))
		pThis->GameClient()->Echo("War entries removed");
	else
		pThis->GameClient()->Echo("No matching war entries");
}

void CWarList::ConAddWarEntry(IConsole::IResult *pResult, void *pUserData)
{
	CWarList *pThis = (CWarList *)pUserData;
	const char *pType = pResult->GetString(0);
	const char *pName = pResult->GetString(1);
	const char *pClan = pResult->GetString(2);
	const char *pReason = pResult->NumArguments() > 3 ? pResult->GetString(3) : "";
	if(str_comp(pName, "-") == 0)
		pName = "";
	if(str_comp(pClan, "-") == 0)
		pClan = "";
	pThis->AddEntry(pName, pClan, pReason, pType);
}

void CWarList::ConUpdateWarGroup(IConsole::IResult *pResult, void *pUserData)
{
	CWarList *pThis = (CWarList *)pUserData;
	const int Index = pResult->GetInteger(0);
	const char *pName = pResult->GetString(1);
	const unsigned ColorValue = (unsigned)pResult->GetInteger(2);
	const ColorRGBA Color = color_cast<ColorRGBA>(ColorHSLA(ColorValue, true));

	if(Index < 0 || Index >= (int)pThis->m_vTypes.size())
	{
		pThis->AddType(pName, Color);
		return;
	}
	pThis->SetTypeColor(Index, Color);
	if(pName[0] != '\0')
		pThis->SetTypeName(Index, pName);
}

void CWarList::ConWarList(IConsole::IResult *pResult, void *pUserData)
{
	CWarList *pThis = (CWarList *)pUserData;
	pThis->GameClient()->Echo("War list:");
	for(const auto &Entry : pThis->m_vEntries)
	{
		char aBuf[512];
		str_format(aBuf, sizeof(aBuf), "  %s%s%s - %s (reason: %s)",
			Entry.m_aName[0] != '\0' ? Entry.m_aName : "",
			Entry.m_aClan[0] != '\0' ? "clan " : "",
			Entry.m_aClan[0] != '\0' ? Entry.m_aClan : "",
			(Entry.m_Type >= 0 && Entry.m_Type < (int)pThis->m_vTypes.size()) ? pThis->m_vTypes[Entry.m_Type].m_aName : "?",
			Entry.m_aReason);
		pThis->GameClient()->Echo(aBuf);
	}
}

bool CWarList::HandleChatCommand(const char *pLine)
{
	if(!g_Config.m_EcWarListChatCommands)
		return false;
	if(pLine[0] != '!')
		return false;

	char aCommand[32] = "";
	const char *pRest = pLine + 1;
	int Len = 0;
	while(pRest[Len] != '\0' && pRest[Len] != ' ' && Len < 31)
	{
		aCommand[Len] = pRest[Len];
		Len++;
	}
	aCommand[Len] = '\0';
	while(*pRest != '\0' && *pRest != ' ')
		pRest++;
	while(*pRest == ' ')
		pRest++;

	const bool IsWar = str_comp_nocase(aCommand, "war") == 0;
	const bool IsTeam = str_comp_nocase(aCommand, "team") == 0;
	const bool IsMute = str_comp_nocase(aCommand, "mute") == 0;
	const bool IsUnmute = str_comp_nocase(aCommand, "unmute") == 0 || str_comp_nocase(aCommand, "delwar") == 0 || str_comp_nocase(aCommand, "delteam") == 0;
	if(!IsWar && !IsTeam && !IsMute && !IsUnmute)
		return false;

	if(pRest[0] == '\0')
	{
		GameClient()->Echo("Usage: !war <name> [reason] | !team <name> | !mute <name> | !unmute <name>");
		return true;
	}

	if(IsUnmute)
	{
		char aRemoveName[MAX_NAME_LENGTH] = "";
		const char *pUnusedReason = "";
		ResolveChatName(pRest, aRemoveName, sizeof(aRemoveName), &pUnusedReason);
		const char *pRemoveType = str_comp_nocase(aCommand, "unmute") == 0 ? "mute" : (str_comp_nocase(aCommand, "delteam") == 0 ? "team" : "enemy");
		if(RemoveEntryByNameType(aRemoveName, pRemoveType))
			GameClient()->Echo("War list entry removed");
		else
			GameClient()->Echo("No matching war list entry");
		return true;
	}

	const char *pType = IsMute ? "mute" : (IsTeam ? "team" : "enemy");

	char aName[MAX_NAME_LENGTH] = "";
	const char *pReason = "";
	ResolveChatName(pRest, aName, sizeof(aName), &pReason);

	AddEntry(aName, "", IsMute ? "" : pReason, pType);
	char aBuf[160];
	str_format(aBuf, sizeof(aBuf), "Added to war list: %s (%s)", pRest, pType);
	GameClient()->Echo(aBuf);
	return true;
}

void CWarList::ConWarDebug(IConsole::IResult *pResult, void *pUserData)
{
	CWarList *pThis = (CWarList *)pUserData;
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "entries: %d, types: %d", (int)pThis->m_vEntries.size(), (int)pThis->m_vTypes.size());
	pThis->GameClient()->Echo(aBuf);
	for(size_t i = 0; i < pThis->m_vEntries.size(); i++)
	{
		const auto &Entry = pThis->m_vEntries[i];
		char aHex[3 * MAX_NAME_LENGTH + 1] = "";
		for(int k = 0; Entry.m_aName[k] != '\0' && k < MAX_NAME_LENGTH - 1; k++)
			str_format(aHex + k * 2, sizeof(aHex) - k * 2, "%02x", (unsigned char)Entry.m_aName[k]);
		str_format(aBuf, sizeof(aBuf), "[%d] name='%s' hex=%s clan='%s' type=%d", (int)i, Entry.m_aName, aHex, Entry.m_aClan, Entry.m_Type);
		pThis->GameClient()->Echo(aBuf);
	}
	for(int ClientId = 0; ClientId < MAX_CLIENTS; ClientId++)
	{
		const auto &Client = pThis->GameClient()->m_aClients[ClientId];
		if(Client.m_aName[0] == '\0')
			continue;
		char aHex[3 * MAX_NAME_LENGTH + 1] = "";
		for(int k = 0; Client.m_aName[k] != '\0' && k < MAX_NAME_LENGTH - 1; k++)
			str_format(aHex + k * 2, sizeof(aHex) - k * 2, "%02x", (unsigned char)Client.m_aName[k]);
		str_format(aBuf, sizeof(aBuf), "client %d name='%s' hex=%s match=%d muted=%d", ClientId, Client.m_aName, aHex, pThis->FindEntryForClient(ClientId) != nullptr ? 1 : 0, pThis->IsMuted(ClientId) ? 1 : 0);
		pThis->GameClient()->Echo(aBuf);
	}
}

void CWarList::ResolveChatName(const char *pArg, char *pOutName, int Size, const char **ppReason) const
{
	pOutName[0] = '\0';
	*ppReason = "";
	const char *pExactName = nullptr;
	int BestLen = 0;

	// online players first, so names with spaces work
	for(int ClientId = 0; ClientId < MAX_CLIENTS; ClientId++)
	{
		const char *pClientName = GameClient()->m_aClients[ClientId].m_aName;
		if(pClientName[0] == '\0')
			continue;
		const int Len = str_length(pClientName);
		if(Len <= BestLen || Len >= Size)
			continue;
		if(str_comp_nocase_num(pArg, pClientName, Len) == 0 && (pArg[Len] == '\0' || pArg[Len] == ' '))
		{
			BestLen = Len;
			pExactName = pClientName;
		}
	}

	// then stored entries (player may be offline)
	if(pExactName == nullptr)
	{
		for(const auto &Entry : m_vEntries)
		{
			if(Entry.m_aName[0] == '\0')
				continue;
			const int Len = str_length(Entry.m_aName);
			if(Len <= BestLen || Len >= Size)
				continue;
			if(str_comp_nocase_num(pArg, Entry.m_aName, Len) == 0 && (pArg[Len] == '\0' || pArg[Len] == ' '))
			{
				BestLen = Len;
				pExactName = Entry.m_aName;
			}
		}
	}

	if(pExactName != nullptr)
	{
		str_copy(pOutName, pExactName, Size);
		*ppReason = str_utf8_skip_whitespaces(pArg + BestLen);
		return;
	}

	const char *pSpace = str_find(pArg, " ");
	if(pSpace == nullptr)
	{
		str_copy(pOutName, pArg, Size);
	}
	else
	{
		str_copy(pOutName, pArg, (int)(pSpace - pArg) + 1 < Size ? (int)(pSpace - pArg) + 1 : Size);
		*ppReason = str_utf8_skip_whitespaces(pSpace);
	}
}

bool CWarList::RemoveEntryByNameType(const char *pName, const char *pType)
{
	const int Type = FindType(pType);
	bool Removed = false;
	for(int i = (int)m_vEntries.size() - 1; i >= 0; i--)
	{
		const auto &Entry = m_vEntries[i];
		if(Entry.m_aName[0] == '\0' || !WarNamesMatch(Entry.m_aName, pName))
			continue;
		if(Type >= 0 && Entry.m_Type != Type)
			continue;
		m_vEntries.erase(m_vEntries.begin() + i);
		Removed = true;
	}
	if(Removed)
	{
		UpdateCache();
		Save();
	}
	return Removed;
}
