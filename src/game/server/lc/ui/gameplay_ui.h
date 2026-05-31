#ifndef GAME_SERVER_LC_GAMEPLAY_UI_H
#define GAME_SERVER_LC_GAMEPLAY_UI_H

#include <base/vmath.h>

class CGameContext;
class CLcTerminalMenu;
class CLocalization;

struct SLcPlayerRoundStats
{
	int m_Deposited;
	int m_LostOnDeath;
	int m_RevivesGiven;
	int m_RevivesReceived;
	int m_Deaths;
};

struct SLcCycleStats
{
	int m_RoundsCompleted;
	int m_TotalRecovered;
	int m_TotalPenalty;
	int m_TotalDeaths;
	int m_AbandonedTimes;
};

void LcFormatQuotaProgress(char *pBuf, int Size, int Money, int Quota);
void LcSendPlainMotd(CGameContext *pGameServer, int ClientID, const char *pMessage);
void LcSendWelcomeMotd(CGameContext *pGameServer, int ClientID);
void LcSendSettlementMotd(CGameContext *pGameServer, int ClientID, int ShipValue, int Penalty, int BossBonus);
void LcSendQuotaCelebrationMotd(CGameContext *pGameServer, int ClientID);
void LcSendTerminationReviewMotd(CGameContext *pGameServer, int ClientID, const SLcCycleStats *pStats);

void LcFillLobbyNextSteps(CLcTerminalMenu *pMenu, CLocalization *pLoc, const char *pLang);
void LcFillMoonInfoCard(CLcTerminalMenu *pMenu, CLocalization *pLoc, const char *pLang, int Moon);
void LcFillStoreRecommendations(CLcTerminalMenu *pMenu, CLocalization *pLoc, const char *pLang, int Moon);
void LcFillShipQuotaSummary(CLcTerminalMenu *pMenu, CGameContext *pGameServer, CLocalization *pLoc, const char *pLang);
void LcFillTeamStatusInfo(CLcTerminalMenu *pMenu, CGameContext *pGameServer, CLocalization *pLoc, const char *pLang);
void LcFillTeammateOverview(CLcTerminalMenu *pMenu, CGameContext *pGameServer, CLocalization *pLoc, const char *pLang);

void LcAddVoteQuotaProgress(CGameContext *pGameServer, int ClientID);
void LcAddVoteLobbyNextSteps(CGameContext *pGameServer, int ClientID);
void LcAddVoteMoonInfoCards(CGameContext *pGameServer, int ClientID);
void LcAddVoteStoreRecommendations(CGameContext *pGameServer, int ClientID, int Moon);
void LcAddVoteShipQuotaSummary(CGameContext *pGameServer, int ClientID);
void LcAddVoteTeamStatus(CGameContext *pGameServer, int ClientID);
void LcAddVoteTeammateOverview(CGameContext *pGameServer, int ClientID);

void LcFormatLandmarkLabel(char *pBuf, int Size, CGameContext *pGameServer, vec2 Pos, vec2 Ref, int RoomType);
const char *LcPlayerExpeditionStatus(CGameContext *pGameServer, int ClientID);

void LcTickMonsterProximity(CGameContext *pGameServer);
void LcTickMapGenLoadingHint(CGameContext *pGameServer);

void LcResetRoundStats(CGameContext *pGameServer);
void LcRecordDeposit(CGameContext *pGameServer, int ClientID, int Value);
void LcRecordDeathLoss(CGameContext *pGameServer, int ClientID, int Value);
void LcRecordRevive(CGameContext *pGameServer, int Helper, int Victim);
void LcRecordDeath(CGameContext *pGameServer, int ClientID);

void LcTickGameplaySystems(CGameContext *pGameServer);
void LcCheckQuotaMilestones(CGameContext *pGameServer, int PrevMoney);
void LcSendMidJoinBriefing(CGameContext *pGameServer, int ClientID);
void LcPlayUiSound(CGameContext *pGameServer, int Sound, int ClientID = -1);
void LcFillAircraftStatus(CLcTerminalMenu *pMenu, CGameContext *pGameServer, CLocalization *pLoc, const char *pLang);
void LcAddVoteStoreBudget(CGameContext *pGameServer, int ClientID);
void LcAddSettlementMvp(CGameContext *pGameServer);
void LcFormatZoneLabel(char *pBuf, int Size, vec2 Pos, vec2 Ship);
void LcLogStorePurchase(CGameContext *pGameServer, int ClientID, const char *pItem, int Cost);

#endif
