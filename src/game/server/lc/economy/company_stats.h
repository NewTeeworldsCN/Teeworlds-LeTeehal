#ifndef GAME_SERVER_LC_COMPANY_STATS_H
#define GAME_SERVER_LC_COMPANY_STATS_H

#include <engine/shared/protocol.h>

struct SLcCycleStats;
struct SLcPlayerRoundStats;

struct SLcPlayerCareerStats
{
	int m_TotalDeposited;
	int m_TotalRevives;
	int m_TotalDeaths;
	int m_BestRoundDeposit;
	int m_Achievements;
};

void LcLoadCompanyStats(SLcCycleStats *pCycle, SLcPlayerCareerStats aCareer[MAX_CLIENTS]);
void LcSaveCompanyStats(const SLcCycleStats *pCycle, const SLcPlayerCareerStats aCareer[MAX_CLIENTS]);
void LcUpdateCareerFromRound(class CGameContext *pGameServer, int ClientID, const SLcPlayerRoundStats *pRound);

enum
{
	LC_ACH_FIRST_BOSS = 1 << 0,
	LC_ACH_CLEAN_RETURN = 1 << 1,
	LC_ACH_QUOTA_MET = 1 << 2,
};

void LcGrantAchievement(class CGameContext *pGameServer, int ClientID, int Flag, const char *pMsgKey);
void LcFormatCycleChart(char *pBuf, int Size, int Rounds, int Recovered, int Quota, int Money);

#endif
