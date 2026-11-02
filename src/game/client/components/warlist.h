#ifndef GAME_CLIENT_COMPONENTS_WARLIST_H
#define GAME_CLIENT_COMPONENTS_WARLIST_H

#include <base/color.h>
#include <base/str.h>

#include <engine/console.h>
#include <engine/shared/protocol.h>

#include <game/client/component.h>

#include <string>
#include <vector>

enum
{
	WARLIST_MAX_TYPE_NAME = 16,
	WARLIST_MAX_REASON = 128,
};

class CWarListType
{
public:
	char m_aName[WARLIST_MAX_TYPE_NAME] = "";
	ColorRGBA m_Color = ColorRGBA(1.0f, 1.0f, 1.0f, 1.0f);
	bool m_Removable = true;
};

class CWarListEntry
{
public:
	// an entry has either a name or a clan, not both
	char m_aName[MAX_NAME_LENGTH] = "";
	char m_aClan[MAX_CLAN_LENGTH] = "";
	char m_aReason[WARLIST_MAX_REASON] = "";
	int m_Type = -1; // index into CWarList::m_vTypes
};

class CWarList : public CComponent
{
public:
	int Sizeof() const override { return sizeof(*this); }

	std::vector<CWarListType> m_vTypes;
	std::vector<CWarListEntry> m_vEntries;

	struct SWarCache
	{
		int m_Type = -1;
		ColorRGBA m_Color = ColorRGBA(1.0f, 1.0f, 1.0f, 1.0f);
		char m_aReason[WARLIST_MAX_REASON] = "";
	};
	SWarCache m_aCache[MAX_CLIENTS];

	void OnInit() override;
	void OnNewSnapshot() override;
	void OnUpdate() override;
	void OnConsoleInit() override;

	void UpdateCache();

	int FindType(const char *pType) const;
	bool AddEntry(const char *pName, const char *pClan, const char *pReason, const char *pType);
	bool RemoveEntry(int Index);
	bool RemoveEntry(const char *pName, const char *pClan);
	void SetEntryType(int Index, int Type);
	void SetTypeName(int Index, const char *pName);
	void SetTypeColor(int Index, ColorRGBA Color);
	bool AddType(const char *pName, ColorRGBA Color);
	bool RemoveType(int Index);

	const CWarListEntry *FindEntryForClient(int ClientId) const;
	bool IsWar(int ClientId) const { return FindEntryForClient(ClientId) != nullptr; }
	ColorRGBA ColorForClient(int ClientId) const;
	const char *ReasonForClient(int ClientId) const;

	bool HandleChatCommand(const char *pLine);
	void ResolveChatName(const char *pArg, char *pOutName, int Size, const char **ppReason) const;
	bool RemoveEntryByNameType(const char *pName, const char *pType);
	bool IsMuted(int ClientId) const;

	void Load();
	void Save() const;

private:
	static void ConWarName(IConsole::IResult *pResult, void *pUserData);
	static void ConWarClan(IConsole::IResult *pResult, void *pUserData);
	static void ConRemoveWarName(IConsole::IResult *pResult, void *pUserData);
	static void ConRemoveWarClan(IConsole::IResult *pResult, void *pUserData);
	static void ConAddWarEntry(IConsole::IResult *pResult, void *pUserData);
	static void ConUpdateWarGroup(IConsole::IResult *pResult, void *pUserData);
	static void ConWarList(IConsole::IResult *pResult, void *pUserData);
	static void ConWarDebug(IConsole::IResult *pResult, void *pUserData);
};

#endif
