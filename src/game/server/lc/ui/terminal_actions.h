#ifndef GAME_SERVER_LC_TERMINAL_ACTIONS_H
#define GAME_SERVER_LC_TERMINAL_ACTIONS_H

struct SLcStoreItem
{
	const char *m_pCmd;
	const char *m_pNameKey;
	int m_Cost;
};

const SLcStoreItem *LcGetStoreItems(int *pCount);
const char *LcStoreItemName(const char *pCmd);
const char *LcMoonRouteLabel(int Moon);
const char *LcMonsterName(int Type);

#endif
