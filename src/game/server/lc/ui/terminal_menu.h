#ifndef GAME_SERVER_LC_TERMINAL_MENU_H
#define GAME_SERVER_LC_TERMINAL_MENU_H

#include <base/tl/array.h>
#include <string>
#include <teeuniverses/components/localization.h>

class CGameContext;
class CPlayer;

enum
{
	LC_PAGE_MAIN = 0,
	LC_PAGE_STORE,
	LC_PAGE_AIRCRAFT,
	LC_PAGE_GUIDE,
	LC_PAGE_GUIDE_STORE,
	LC_PAGE_GUIDE_MONSTERS,
	LC_PAGE_GUIDE_SCRAP,
	LC_PAGE_MOON,
	LC_PAGE_INVENTORY,
	LC_PAGE_GUIDE_STORE_SUPPLIES = 9,
	LC_PAGE_THREATS = 10,
	LC_PAGE_GUIDE_STORE_WEAPONS = 11,
	LC_PAGE_GUIDE_STORE_FIELD = 12,
	LC_PAGE_GUIDE_SCRAP_L1 = 13,
	LC_PAGE_GUIDE_SCRAP_L2 = 14,
	LC_PAGE_GUIDE_SCRAP_L3 = 15,
	LC_PAGE_SHIP_CARGO = 16,
	LC_PAGE_INVENTORY_USE = 17,
	LC_PAGE_INVENTORY_DROP = 18,
	LC_PAGE_WELCOME = 19,
	LC_PAGE_TEAM = 20,
	LC_PAGE_HELP_TUT = 21,

	LC_PAGE_GUIDE_MONSTER_BASE = 22,
	LC_PAGE_GUIDE_MONSTER_END = 31,
	LC_PAGE_CREDITS = 32,

	LC_PAGE_GUIDE_STORE_DETAIL_BASE = 64,
	LC_PAGE_GUIDE_STORE_DETAIL_END = 80,

	LC_PAGE_GUIDE_SCRAP_DETAIL_BASE = 40,
	LC_PAGE_GUIDE_SCRAP_DETAIL_END = 64,

	LC_PAGE_INVENTORY_DETAIL_BASE = 80,
	LC_PAGE_INVENTORY_DETAIL_END = 96,
};

inline bool LcTerminalIsGuideMonsterDetail(int Page)
{
	return Page >= LC_PAGE_GUIDE_MONSTER_BASE && Page < LC_PAGE_GUIDE_MONSTER_END;
}

inline bool LcTerminalIsGuideStoreDetail(int Page)
{
	return Page >= LC_PAGE_GUIDE_STORE_DETAIL_BASE && Page < LC_PAGE_GUIDE_STORE_DETAIL_END;
}

inline bool LcTerminalIsGuideScrapDetail(int Page)
{
	return Page >= LC_PAGE_GUIDE_SCRAP_DETAIL_BASE && Page < LC_PAGE_GUIDE_SCRAP_DETAIL_END;
}

inline bool LcTerminalIsInventoryDetail(int Page)
{
	return Page >= LC_PAGE_INVENTORY_DETAIL_BASE && Page < LC_PAGE_INVENTORY_DETAIL_END;
}

bool LcTerminalInfoScrollPage(int Page);
int LcTerminalParentPage(int Page);
bool LcTerminalIsValidPage(int Page);

class CLcTerminalMenu
{
public:
	static const int TERMINAL_MOTD_MAX = 1200;
	static const int TERMINAL_MOTD_MAX_LINES = 22;
	static const int TERMINAL_VISIBLE_ACTIONS = 6;
	static const int TERMINAL_VISIBLE_INFO = 12;

	struct SEntry
	{
		char m_aLabel[256];
		char m_aCommand[256];
	};

	void Populate(CGameContext *pGameServer, int ClientID);
	void SendMotd(CGameContext *pGameServer, int ClientID, int SelectedAction);
	bool ExecuteAction(CGameContext *pGameServer, CPlayer *pPlayer, int ActionIndex);
	int NumActions() const;
	int NumInfoLines() const;

	void AddInfo(const char *pLabel);
	void AddAction(const char *pCmd, const char *pLabel);

private:
	array<SEntry> m_aEntries;
	std::string m_MotdMessage;

	void Clear();
	void AddStoreBuyLoc(const char *pCmd, const char *pNameKey, int Cost, CLocalization *pLoc, const char *pLang);
	void AppendMotdLineLoc(std::string &Motd, CLocalization *pLoc, const char *pLang, const char *pKey) const;
	void AppendMotdLine(std::string &Motd, const char *pLine) const;
};

#endif
