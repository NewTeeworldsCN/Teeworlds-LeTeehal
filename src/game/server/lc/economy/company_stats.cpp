#include "company_stats.h"

#include "../ui/gameplay_ui.h"
#include "../../core/gamecontext.h"

#include <base/system.h>

static const char *STATS_PATH = "leteehal_company_stats.txt";

void LcLoadCompanyStats(SLcCycleStats *pCycle, SLcPlayerCareerStats aCareer[MAX_CLIENTS])
{
	if(pCycle)
		mem_zero(pCycle, sizeof(*pCycle));
	for(int i = 0; i < MAX_CLIENTS; i++)
		mem_zero(&aCareer[i], sizeof(aCareer[i]));

	IOHANDLE f = io_open(STATS_PATH, IOFLAG_READ);
	if(!f)
		return;

	char aBuf[512];
	while(io_read(f, aBuf, sizeof(aBuf) - 1) > 0)
	{
		aBuf[sizeof(aBuf) - 1] = 0;
		char *pLine = aBuf;
		while(pLine && *pLine)
		{
			const char *pNl = str_find(pLine, "\n");
			char aTmp[512];
			str_copy(aTmp, pLine, sizeof(aTmp));
			if(pNl)
			{
				int Off = (int)(pNl - pLine);
				if(Off < (int)sizeof(aTmp))
					aTmp[Off] = 0;
				pNl++;
			}
			if(str_comp_num(aTmp, "cycle ", 6) == 0 && pCycle)
			{
				sscanf(aTmp + 6, "rounds=%d recovered=%d penalty=%d deaths=%d abandoned=%d",
					&pCycle->m_RoundsCompleted, &pCycle->m_TotalRecovered, &pCycle->m_TotalPenalty,
					&pCycle->m_TotalDeaths, &pCycle->m_AbandonedTimes);
			}
			else if(str_comp_num(aTmp, "career ", 7) == 0)
			{
				int Id = 0;
				SLcPlayerCareerStats S;
				mem_zero(&S, sizeof(S));
				sscanf(aTmp + 7, "id=%d dep=%d rev=%d death=%d best=%d ach=%d",
					&Id, &S.m_TotalDeposited, &S.m_TotalRevives, &S.m_TotalDeaths, &S.m_BestRoundDeposit, &S.m_Achievements);
				if(Id >= 0 && Id < MAX_CLIENTS)
					aCareer[Id] = S;
			}
			pLine = pNl ? (char *)pNl : nullptr;
		}
	}
	io_close(f);
}

void LcSaveCompanyStats(const SLcCycleStats *pCycle, const SLcPlayerCareerStats aCareer[MAX_CLIENTS])
{
	IOHANDLE f = io_open(STATS_PATH, IOFLAG_WRITE);
	if(!f)
		return;

	char aLine[256];
	if(pCycle)
	{
		str_format(aLine, sizeof(aLine), "cycle rounds=%d recovered=%d penalty=%d deaths=%d abandoned=%d\n",
			pCycle->m_RoundsCompleted, pCycle->m_TotalRecovered, pCycle->m_TotalPenalty,
			pCycle->m_TotalDeaths, pCycle->m_AbandonedTimes);
		io_write(f, aLine, str_length(aLine));
	}
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(aCareer[i].m_TotalDeposited == 0 && aCareer[i].m_TotalRevives == 0 && aCareer[i].m_Achievements == 0)
			continue;
		str_format(aLine, sizeof(aLine), "career id=%d dep=%d rev=%d death=%d best=%d ach=%d\n",
			i, aCareer[i].m_TotalDeposited, aCareer[i].m_TotalRevives, aCareer[i].m_TotalDeaths,
			aCareer[i].m_BestRoundDeposit, aCareer[i].m_Achievements);
		io_write(f, aLine, str_length(aLine));
	}
	io_close(f);
}

void LcUpdateCareerFromRound(CGameContext *pGameServer, int ClientID, const SLcPlayerRoundStats *pRound)
{
	if(!pGameServer || ClientID < 0 || ClientID >= MAX_CLIENTS || !pRound)
		return;
	SLcPlayerCareerStats &C = pGameServer->m_aCareerStats[ClientID];
	C.m_TotalDeposited += pRound->m_Deposited;
	C.m_TotalRevives += pRound->m_RevivesGiven;
	C.m_TotalDeaths += pRound->m_Deaths;
	if(pRound->m_Deposited > C.m_BestRoundDeposit)
		C.m_BestRoundDeposit = pRound->m_Deposited;
}

void LcGrantAchievement(CGameContext *pGameServer, int ClientID, int Flag, const char *pMsgKey)
{
	if(!pGameServer || ClientID < 0 || ClientID >= MAX_CLIENTS || !pMsgKey)
		return;
	SLcPlayerCareerStats &C = pGameServer->m_aCareerStats[ClientID];
	if(C.m_Achievements & Flag)
		return;
	C.m_Achievements |= Flag;
	pGameServer->SendChatTarget(-1, pMsgKey, "name", pGameServer->Server()->ClientName(ClientID));
}

void LcFormatCycleChart(char *pBuf, int Size, int Rounds, int Recovered, int Quota, int Money)
{
	if(Size <= 0)
		return;
	int Pct = Quota > 0 ? Money * 100 / Quota : 0;
	if(Pct > 100)
		Pct = 100;
	str_format(pBuf, Size, _("周期图表 | 班次:%d 回收:%d | 指标进度:%d%%"), Rounds, Recovered, Pct);
}
