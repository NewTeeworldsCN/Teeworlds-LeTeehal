#include "gameplay_ui.h"

#include "localize_util.h"
#include "../expedition/balance.h"
#include "../expedition/moons.h"
#include "../../core/gamecontext.h"
#include "../../core/gamecontroller.h"
#include "../../core/player.h"
#include "../../entities/core/character.h"
#include "terminal_menu.h"
#include "../../entities/lc/ship.h"
#include "../../entities/lc/monster.h"
#include "../../lc/hazards/hazards.h"

#include <engine/shared/config.h>
#include <game/generated/protocol.h>
#include <game/version.h>
#include <teeuniverses/components/localization.h>

void LcFormatQuotaProgress(char *pBuf, int Size, int Money, int Quota)
{
	int Pct = 100;
	if(Quota > 0)
	{
		Pct = Money * 100 / Quota;
		if(Pct > 100)
			Pct = 100;
		if(Pct < 0)
			Pct = 0;
	}
	int Filled = Pct / 10;
	if(Filled > 10)
		Filled = 10;
	char aBar[12];
	for(int i = 0; i < 10; i++)
		aBar[i] = i < Filled ? '#' : '-';
	aBar[10] = 0;
	str_format(pBuf, Size, "[%s] %d%%", aBar, Pct);
}

void LcSendPlainMotd(CGameContext *pGameServer, int ClientID, const char *pMessage)
{
	if(!pMessage || ClientID < 0 || ClientID >= MAX_CLIENTS)
		return;
	CPlayer *pP = pGameServer->m_apPlayers[ClientID];
	if(pP && pP->m_TerminalMenuOpen)
		pGameServer->CloseTerminalMenu(ClientID);

	const char *pLine = pMessage;
	while(*pLine)
	{
		const char *pEnd = pLine;
		while(*pEnd && *pEnd != '\n')
			pEnd++;
		if(pEnd > pLine)
		{
			char aBuf[512];
			int Len = minimum((int)(pEnd - pLine), (int)sizeof(aBuf) - 1);
			mem_copy(aBuf, pLine, Len);
			aBuf[Len] = 0;
			pGameServer->SendChatTarget(ClientID, aBuf);
		}
		if(*pEnd == '\n')
			pLine = pEnd + 1;
		else
			break;
	}
}

void LcSendWelcomeMotd(CGameContext *pGameServer, int ClientID)
{
	CPlayer *pP = pGameServer->m_apPlayers[ClientID];
	if(!pP)
		return;
	CLocalization *pLoc = pGameServer->Server()->Localization();
	const char *pLang = pP->GetLanguage();
	char aLine[256];
	char aMotd[900];
	aMotd[0] = 0;

	LcLocalizeCopy(aLine, sizeof(aLine), pLoc, pLang, _("=== 欢迎加入 Tee命公司 ==="));
	str_append(aMotd, aLine, sizeof(aMotd));
	str_append(aMotd, "\n", sizeof(aMotd));
	LcLocalizeCopy(aLine, sizeof(aLine), pLoc, pLang, _("你是公司雇员：在截止日前达成指标，否则被解雇。"));
	str_append(aMotd, aLine, sizeof(aMotd));
	str_append(aMotd, "\n\n", sizeof(aMotd));
	LcLocalizeCopy(aLine, sizeof(aLine), pLoc, pLang, _("【三步开始】"));
	str_append(aMotd, aLine, sizeof(aMotd));
	str_append(aMotd, "\n", sizeof(aMotd));
	LcLocalizeCopy(aLine, sizeof(aLine), pLoc, pLang, _("1. ESC 投票 → 选路线 → 投票出发"));
	str_append(aMotd, aLine, sizeof(aMotd));
	str_append(aMotd, "\n", sizeof(aMotd));
	LcLocalizeCopy(aLine, sizeof(aLine), pLoc, pLang, _("2. F3 终端 → 商店购买 → 查看图鉴"));
	str_append(aMotd, aLine, sizeof(aMotd));
	str_append(aMotd, "\n", sizeof(aMotd));
	LcLocalizeCopy(aLine, sizeof(aLine), pLoc, pLang, _("3. 设施内锤子拾取废品 → 回着陆飞船"));
	str_append(aMotd, aLine, sizeof(aMotd));
	str_append(aMotd, "\n\n", sizeof(aMotd));
	if(MOD_QQ_GROUP[0])
	{
		LcFormatCopy(aLine, sizeof(aLine), pLoc, pLang, _("【官方 QQ 交流群】{str:qq}"), "qq", MOD_QQ_GROUP);
		str_append(aMotd, aLine, sizeof(aMotd));
		str_append(aMotd, "\n", sizeof(aMotd));
	}
	LcLocalizeCopy(aLine, sizeof(aLine), pLoc, pLang, _("提示：F3 打开终端后内容较多，建议 F1 输入 cl_motd_time 100 延长显示。"));
	str_append(aMotd, aLine, sizeof(aMotd));
	str_append(aMotd, "\n", sizeof(aMotd));
	LcLocalizeCopy(aLine, sizeof(aLine), pLoc, pLang, _("输入 /help 1 查看分步教程 | /status 查看任务"));
	str_append(aMotd, aLine, sizeof(aMotd));
	LcSendPlainMotd(pGameServer, ClientID, aMotd);
}

void LcSendSettlementMotd(CGameContext *pGameServer, int ClientID, int ShipValue, int Penalty, int BossBonus)
{
	CPlayer *pP = pGameServer->m_apPlayers[ClientID];
	if(!pP)
		return;
	int Money = g_Config.m_GcMoney;
	int Quota = g_Config.m_GcQuota;
	char aProgress[32];
	LcFormatQuotaProgress(aProgress, sizeof(aProgress), Money, Quota);
	const SLcPlayerRoundStats *pS = &pGameServer->m_aRoundStats[ClientID];

	pGameServer->SendChatTarget(ClientID, _("=== 本轮结算 ==="));
	pGameServer->SendChatTarget(ClientID, _("飞船回收: {int:value} 元"), "value", &ShipValue);
	if(Penalty > 0)
		pGameServer->SendChatTarget(ClientID, _("抛弃罚金: {int:penalty} 元"), "penalty", &Penalty);
	if(BossBonus > 0)
		pGameServer->SendChatTarget(ClientID, _("头目击杀奖励: {int:bonus} 元"), "bonus", &BossBonus);
	pGameServer->SendChatTarget(ClientID, _("公司余额: {int:money} / 指标 {int:quota}"), "money", &Money, "quota", &Quota);
	pGameServer->SendChatTarget(ClientID, aProgress);
	pGameServer->SendChatTarget(ClientID, _("你的贡献: 上交 {int:dep} 元 | 丢失 {int:lost} 元 | 救人 {int:rev} 次 | 死亡 {int:death} 次"),
		"dep", &pS->m_Deposited, "lost", &pS->m_LostOnDeath, "rev", &pS->m_RevivesGiven, "death", &pS->m_Deaths);
}

void LcSendQuotaCelebrationMotd(CGameContext *pGameServer, int ClientID)
{
	if(!pGameServer->m_apPlayers[ClientID])
		return;
	int NextQuota = BalanceNextQuota(g_Config.m_GcRounds);
	pGameServer->SendChatTarget(ClientID, _("=== 指标达成！合同续签 ==="));
	pGameServer->SendChatTarget(ClientID, _("公司认可你们的表现，欢迎回来。"));
	pGameServer->SendChatTarget(ClientID, _("下一档指标: {int:quota} 元"), "quota", &NextQuota);
}

void LcSendTerminationReviewMotd(CGameContext *pGameServer, int ClientID, const SLcCycleStats *pStats)
{
	if(!pGameServer->m_apPlayers[ClientID] || !pStats)
		return;
	pGameServer->SendChatTarget(ClientID, _("=== 被解雇 · 本周期战绩 ==="));
	pGameServer->SendChatTarget(ClientID, _("完成班次: {int:rounds} | 总回收: {int:rec} 元"),
		"rounds", &pStats->m_RoundsCompleted, "rec", &pStats->m_TotalRecovered);
	pGameServer->SendChatTarget(ClientID, _("总罚金: {int:pen} 元 | 死亡 {int:death} 次 | 被抛弃 {int:ab} 次"),
		"pen", &pStats->m_TotalPenalty, "death", &pStats->m_TotalDeaths, "ab", &pStats->m_AbandonedTimes);
	pGameServer->SendChatTarget(ClientID, _("所有进度已重置。下次加油。"));
}

static void AddInfoLoc(CLcTerminalMenu *pMenu, CLocalization *pLoc, const char *pLang, const char *pKey)
{
	char aBuf[256];
	LcLocalizeCopy(aBuf, sizeof(aBuf), pLoc, pLang, pKey);
	pMenu->AddInfo(aBuf);
}

void LcFillLobbyNextSteps(CLcTerminalMenu *pMenu, CLocalization *pLoc, const char *pLang)
{
	AddInfoLoc(pMenu, pLoc, pLang, _("【下一步】"));
	AddInfoLoc(pMenu, pLoc, pLang, _("① ESC/F3 → 选路线"));
	AddInfoLoc(pMenu, pLoc, pLang, _("② 商店购买装备"));
	AddInfoLoc(pMenu, pLoc, pLang, _("③ 投票出发"));
}

void LcFillMoonInfoCard(CLcTerminalMenu *pMenu, CLocalization *pLoc, const char *pLang, int Moon)
{
	const SLcMoonConfig *pMoon = LcGetMoon(Moon);
	char aLine[192];
	LcFormatCopy(aLine, sizeof(aLine), pLoc, pLang, _("{lstr:name} | {int:stars}星 | {int:min}分钟 | 废品{int:scrap}% | 怪物{int:mon}%"),
		"name", LcMoonName(Moon), "stars", &pMoon->m_HazardStars, "min", &pMoon->m_TimelimitMin,
		"scrap", &pMoon->m_ScrapPercent, "mon", &pMoon->m_MonsterPercent);
	pMenu->AddInfo(aLine);
	LcFormatCopy(aLine, sizeof(aLine), pLoc, pLang, _("设施: {lstr:fac}"), "fac", LcFacilityName(pMoon->m_FacilityType));
	pMenu->AddInfo(aLine);
	int RecPlayers = pMoon->m_HazardStars >= 4 ? 3 : (pMoon->m_HazardStars >= 2 ? 2 : 1);
	LcFormatCopy(aLine, sizeof(aLine), pLoc, pLang, _("推荐 {int:n} 人以上 | 难度 {int:stars} 星"), "n", &RecPlayers, "stars", &pMoon->m_HazardStars);
	pMenu->AddInfo(aLine);
}

void LcFillStoreRecommendations(CLcTerminalMenu *pMenu, CLocalization *pLoc, const char *pLang, int Moon)
{
	const SLcMoonConfig *pMoon = LcGetMoon(Moon);
	AddInfoLoc(pMenu, pLoc, pLang, _("【本局推荐】"));
	if(pMoon->m_HazardStars >= 3)
	{
		AddInfoLoc(pMenu, pLoc, pLang, _("☆ 防护服 / 生命保障（高危险）"));
		AddInfoLoc(pMenu, pLoc, pLang, _("☆ 延长班次（时间紧）"));
	}
	else if(pMoon->m_TimelimitMin <= 4)
	{
		AddInfoLoc(pMenu, pLoc, pLang, _("☆ 延长班次 + 驱虫哨"));
	}
	else
	{
		AddInfoLoc(pMenu, pLoc, pLang, _("☆ 扩音器 / 闪光弹（探路）"));
	}
	if(pMoon->m_FacilityType == LC_FACILITY_MANSION)
		AddInfoLoc(pMenu, pLoc, pLang, _("☆ 蔓背怪多 → 扩音器扫描"));
}

void LcFillShipQuotaSummary(CLcTerminalMenu *pMenu, CGameContext *pGameServer, CLocalization *pLoc, const char *pLang)
{
	if(!pGameServer->m_pController || !pGameServer->m_pController->m_pShip)
		return;
	int ShipValue = pGameServer->m_pController->m_pShip->GetValue();
	int ShipNum = pGameServer->m_pController->m_pShip->GetNum();
	int Money = g_Config.m_GcMoney;
	int Quota = g_Config.m_GcQuota;
	int Need = Quota - Money;
	if(Need < 0)
		Need = 0;
	char aLine[192];
	char aProgress[32];
	LcFormatQuotaProgress(aProgress, sizeof(aProgress), Money, Quota);
	LcFormatCopy(aLine, sizeof(aLine), pLoc, pLang, _("船上 {int:num} 件 / {int:value} 元"), "num", &ShipNum, "value", &ShipValue);
	pMenu->AddInfo(aLine);
	LcFormatCopy(aLine, sizeof(aLine), pLoc, pLang, _("距指标还差 {int:need} 元"), "need", &Need);
	pMenu->AddInfo(aLine);
	pMenu->AddInfo(aProgress);
}

void LcFillTeamStatusInfo(CLcTerminalMenu *pMenu, CGameContext *pGameServer, CLocalization *pLoc, const char *pLang)
{
	if(pGameServer->Server()->m_LocateGame != LOCATE_GAME)
		return;
	AddInfoLoc(pMenu, pLoc, pLang, _("【队员状态】"));
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(!pGameServer->m_apPlayers[i] || !pGameServer->GetPlayerChar(i))
			continue;
		const char *pStatus = LcPlayerExpeditionStatus(pGameServer, i);
		char aLine[128];
		str_format(aLine, sizeof(aLine), "%s: %s", pGameServer->Server()->ClientName(i), pStatus);
		pMenu->AddInfo(aLine);
	}
}

void LcFillTeammateOverview(CLcTerminalMenu *pMenu, CGameContext *pGameServer, CLocalization *pLoc, const char *pLang)
{
	if(pGameServer->Server()->m_LocateGame != LOCATE_GAME)
		return;
	AddInfoLoc(pMenu, pLoc, pLang, _("【队友携带】"));
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		CPlayer *pP = pGameServer->m_apPlayers[i];
		if(!pP || !pGameServer->GetPlayerChar(i))
			continue;
		int Lb = pP->GetBackpackWeight();
		int Value = pP->GetBackpackValue();
		for(int s = 0; s < pP->m_vScraps.size(); s++)
			if(pP->m_vScraps[s])
			{
				Lb += pP->m_vScraps[s]->m_Weight;
				Value += pP->m_vScraps[s]->m_Value;
			}
		char aLine[128];
		LcFormatCopy(aLine, sizeof(aLine), pLoc, pLang, _("{str:name}: {int:lb}镑 / {int:value}元"),
			"name", pGameServer->Server()->ClientName(i), "lb", &Lb, "value", &Value);
		pMenu->AddInfo(aLine);
	}
}

void LcAddVoteQuotaProgress(CGameContext *pGameServer, int ClientID)
{
	char aProgress[32];
	LcFormatQuotaProgress(aProgress, sizeof(aProgress), g_Config.m_GcMoney, g_Config.m_GcQuota);
	pGameServer->AddVote(ClientID, "null", aProgress);
}

void LcAddVoteLobbyNextSteps(CGameContext *pGameServer, int ClientID)
{
	pGameServer->AddVote(ClientID, "null", _("【下一步】①选路线 ②商店 ③投票出发"));
}

void LcAddVoteMoonInfoCards(CGameContext *pGameServer, int ClientID)
{
	for(int m = 0; m < NUM_LC_MOONS; m++)
	{
		const SLcMoonConfig *pMoon = LcGetMoon(m);
		pGameServer->AddVote(ClientID, "null", _("{lstr:name} {int:min}分 {int:stars}星 废品{int:scrap}%"),
			"name", LcMoonName(m), "min", &pMoon->m_TimelimitMin, "stars", &pMoon->m_HazardStars, "scrap", &pMoon->m_ScrapPercent);
	}
}

void LcAddVoteStoreRecommendations(CGameContext *pGameServer, int ClientID, int Moon)
{
	const SLcMoonConfig *pMoon = LcGetMoon(Moon);
	if(pMoon->m_HazardStars >= 3)
		pGameServer->AddVote(ClientID, "null", _("推荐: 防护服/生命保障/延长时间"));
	else if(pMoon->m_TimelimitMin <= 4)
		pGameServer->AddVote(ClientID, "null", _("推荐: 延长时间/驱虫哨"));
	else
		pGameServer->AddVote(ClientID, "null", _("推荐: 扩音器/闪光弹"));
}

void LcAddVoteShipQuotaSummary(CGameContext *pGameServer, int ClientID)
{
	if(!pGameServer->m_pController || !pGameServer->m_pController->m_pShip)
		return;
	int ShipValue = pGameServer->m_pController->m_pShip->GetValue();
	int Need = g_Config.m_GcQuota - g_Config.m_GcMoney;
	if(Need < 0)
		Need = 0;
	pGameServer->AddVote(ClientID, "null", _("船上价值 {int:value} 元 | 距指标差 {int:need} 元"), "value", &ShipValue, "need", &Need);
	LcAddVoteQuotaProgress(pGameServer, ClientID);
}

void LcAddVoteTeamStatus(CGameContext *pGameServer, int ClientID)
{
	if(pGameServer->Server()->m_LocateGame != LOCATE_GAME)
		return;
	pGameServer->AddVote(ClientID, "null", _("--- 队员状态 ---"));
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(!pGameServer->m_apPlayers[i] || !pGameServer->GetPlayerChar(i))
			continue;
		pGameServer->AddVote(ClientID, "null", _("{str:name}: {lstr:status}"),
			"name", pGameServer->Server()->ClientName(i), "status", LcPlayerExpeditionStatus(pGameServer, i));
	}
}

void LcAddVoteTeammateOverview(CGameContext *pGameServer, int ClientID)
{
	if(pGameServer->Server()->m_LocateGame != LOCATE_GAME)
		return;
	pGameServer->AddVote(ClientID, "null", _("--- 队友携带 ---"));
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		CPlayer *pP = pGameServer->m_apPlayers[i];
		if(!pP || !pGameServer->GetPlayerChar(i))
			continue;
		int Lb = pP->GetBackpackWeight();
		int Value = pP->GetBackpackValue();
		for(int s = 0; s < pP->m_vScraps.size(); s++)
			if(pP->m_vScraps[s])
			{
				Lb += pP->m_vScraps[s]->m_Weight;
				Value += pP->m_vScraps[s]->m_Value;
			}
		pGameServer->AddVote(ClientID, "null", _("{str:name} {int:lb}镑/{int:value}元"),
			"name", pGameServer->Server()->ClientName(i), "lb", &Lb, "value", &Value);
	}
}

static const char *LcCompassDirKey(vec2 From, vec2 To)
{
	vec2 D = To - From;
	if(length(D) < 1.0f)
		return _("近旁");
	if(fabs(D.x) > fabs(D.y))
		return D.x > 0 ? _("东") : _("西");
	return D.y > 0 ? _("南") : _("北");
}

static int LcZoneBandFromShip(vec2 Pos, vec2 Ship)
{
	float Dist = distance(Pos, Ship);
	if(Dist < 900.f)
		return 0;
	if(Dist < 2200.f)
		return 1;
	return 2;
}

void LcFormatZoneLabel(char *pBuf, int Size, vec2 Pos, vec2 Ship)
{
	switch(LcZoneBandFromShip(Pos, Ship))
	{
	case 0: str_copy(pBuf, _("着陆区"), Size); break;
	case 1: str_copy(pBuf, _("主通道"), Size); break;
	default: str_copy(pBuf, _("深区"), Size); break;
	}
}

void LcFormatLandmarkLabel(char *pBuf, int Size, CGameContext *pGameServer, vec2 Pos, vec2 Ref, int RoomType)
{
	(void)pGameServer;
	char aZone[32];
	LcFormatZoneLabel(aZone, sizeof(aZone), Pos, Ref);
	const char *pRoom = LcFacilityRoomNameZh(RoomType);
	str_format(pBuf, Size, "%s·%s区·%s", aZone, LcCompassDirKey(Ref, Pos), pRoom);
}

const char *LcPlayerExpeditionStatus(CGameContext *pGameServer, int ClientID)
{
	CPlayer *pP = pGameServer->m_apPlayers[ClientID];
	CCharacter *pChr = pGameServer->GetPlayerChar(ClientID);
	if(pP && LcPlayerIsExpeditionSpectator(pP) && !pChr)
		return _("旁观");
	if(!pChr)
		return _("离线");
	if(pChr->m_Freeze)
		return _("冻结·待救");
	if(pChr->m_InShip)
		return _("已登船");
	return _("设施内");
}

bool LcPlayerIsExpeditionSpectator(CPlayer *pPlayer)
{
	return pPlayer && pPlayer->GetTeam() == TEAM_SPECTATORS;
}

bool LcPlayerCountsForStart(CGameContext *pGameServer, CPlayer *pPlayer)
{
	if(!pGameServer || !pPlayer || !pPlayer->GetCharacter())
		return false;
	if(LcPlayerIsExpeditionSpectator(pPlayer))
		return false;
	if(pGameServer->Server()->m_LocateGame == LOCATE_LOBBY && pPlayer->m_LcSpectatorOptIn)
		return false;
	return true;
}

void LcSetSpectatorOptIn(CGameContext *pGameServer, int ClientID, bool OptIn)
{
	CPlayer *pP = pGameServer->m_apPlayers[ClientID];
	if(!pP)
		return;

	if(pGameServer->Server()->m_LocateGame != LOCATE_LOBBY)
	{
		pGameServer->SendChatTarget(ClientID, OptIn
			? _("请在飞船上登记：下趟以旁观者加入")
			: _("请回到飞船后再取消旁观登记"));
		return;
	}

	if(pP->m_VoteStarted)
	{
		pP->m_VoteStarted = false;
		pGameServer->m_VoteStart--;
	}

	pP->m_LcSpectatorOptIn = OptIn;
	pGameServer->m_aLcSpectatorOptIn[ClientID] = OptIn;
	if(OptIn)
		pGameServer->SendChatTarget(ClientID, _("已登记：下趟远征将以旁观者加入（仅观看，无法中途加入）"));
	else
		pGameServer->SendChatTarget(ClientID, _("已取消旁观登记，下趟将正常参与班次"));

	pGameServer->ResetVotes(ClientID);
	if(pP->m_TerminalMenuOpen)
		pGameServer->RefreshTerminalMenu(ClientID);
}

static void AddActionLocJoinRole(CLcTerminalMenu *pMenu, CLocalization *pLoc, const char *pLang, const char *pCmd, const char *pKey)
{
	char aBuf[128];
	LcLocalizeCopy(aBuf, sizeof(aBuf), pLoc, pLang, pKey);
	pMenu->AddAction(pCmd, aBuf);
}

void LcAddJoinRoleTerminalActions(CLcTerminalMenu *pMenu, CGameContext *pGameServer, CPlayer *pPlayer, CLocalization *pLoc, const char *pLang)
{
	if(!pMenu || !pGameServer || !pPlayer || pGameServer->Server()->m_LocateGame != LOCATE_LOBBY)
		return;

	AddInfoLoc(pMenu, pLoc, pLang, _("【旁观登记】"));
	if(pPlayer->m_LcSpectatorOptIn)
	{
		AddInfoLoc(pMenu, pLoc, pLang, _("下趟远征：旁观者（仅观看）"));
		AddActionLocJoinRole(pMenu, pLoc, pLang, "lcm_role play", _("☞ 取消旁观登记"));
	}
	else
	{
		AddInfoLoc(pMenu, pLoc, pLang, _("下趟远征：正常参与"));
		AddActionLocJoinRole(pMenu, pLoc, pLang, "lcm_role spec", _("☞ 登记下趟旁观"));
	}
	AddInfoLoc(pMenu, pLoc, pLang, _("出发进入设施后才会进入旁观，回飞船可取消"));
}

void LcAddJoinRoleVoteOptions(CGameContext *pGameServer, int ClientID, CPlayer *pPlayer)
{
	if(!pGameServer || !pPlayer || pGameServer->Server()->m_LocateGame != LOCATE_LOBBY)
		return;

	pGameServer->AddVote(ClientID, "null", _("------ 旁观登记 ------"));
	if(pPlayer->m_LcSpectatorOptIn)
	{
		pGameServer->AddVote(ClientID, "null", _("下趟远征：旁观者（仅观看）"));
		pGameServer->AddVote(ClientID, "lc_play", _("☞ 取消旁观登记"));
	}
	else
	{
		pGameServer->AddVote(ClientID, "null", _("下趟远征：正常参与"));
		pGameServer->AddVote(ClientID, "lc_spec", _("☞ 登记下趟旁观"));
	}
	pGameServer->AddVote(ClientID, "null", _("出发后进入旁观，回飞船可取消"));
	pGameServer->AddVote(ClientID, "null", _("---------------"));
}

static const char *LcMonsterProximityMsg(CMonster *pMon)
{
	if(!pMon)
		return _("【威胁接近】附近有怪物");
	switch(pMon->MonsterType())
	{
	case TYPE_FEAR: return _("【弹簧头】附近有弹簧头潜伏");
	case TYPE_BOMBER: return _("【爆壳虫】爆炸型怪物接近");
	case TYPE_HUNTER: return _("【猛禽】远程怪物在附近");
	case TYPE_STALKER: return _("【潜追者】高速怪物正在接近");
	case TYPE_SATIETY: return _("【囤积虫】附近有囤积虫");
	default: return _("【威胁接近】附近有怪物（约 {int:m} 格）");
	}
}

void LcTickMonsterProximity(CGameContext *pGameServer)
{
	if(pGameServer->Server()->m_LocateGame != LOCATE_GAME)
		return;
	const int WarnRange = 420;
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		CCharacter *pChr = pGameServer->GetPlayerChar(i);
		if(!pChr || pChr->m_InShip || pChr->m_Freeze)
			continue;
		float Best = 1e30f;
		CMonster *pClosest = 0;
		for(int m = 0; m < MAX_MONSTERS; m++)
		{
			if(!pGameServer->m_apMonsters[m])
				continue;
			float Dist = distance(pChr->m_Pos, pGameServer->m_apMonsters[m]->m_Pos);
			if(Dist < Best)
			{
				Best = Dist;
				pClosest = pGameServer->m_apMonsters[m];
			}
		}
		if(Best > WarnRange)
			continue;
		if(pGameServer->Server()->Tick() - pChr->m_LastProximityWarnTick < pGameServer->Server()->TickSpeed() * 8)
			continue;
		pChr->m_LastProximityWarnTick = pGameServer->Server()->Tick();
		int DistTiles = (int)(Best / 32.f);
		const char *pMsg = LcMonsterProximityMsg(pClosest);
		if(str_find(pMsg, "{int:m}"))
			pGameServer->SendBroadcast(i, BROADCAST_PRIORITY_EFFECTSTATE, pGameServer->Server()->TickSpeed() * 2, pMsg, "m", &DistTiles);
		else
			pGameServer->SendBroadcast(i, BROADCAST_PRIORITY_EFFECTSTATE, pGameServer->Server()->TickSpeed() * 2, pMsg);
		pGameServer->CreateSound(pChr->m_Pos, SOUND_PLAYER_PAIN_SHORT, CmaskOne(i));
	}
}

void LcTickMapGenLoadingHint(CGameContext *pGameServer)
{
	if(!pGameServer->m_MapGenActive && !pGameServer->m_MapGenPending)
		return;
	if(pGameServer->Server()->Tick() - pGameServer->m_MapGenLoadingBroadcastTick < pGameServer->Server()->TickSpeed() * 4)
		return;
	pGameServer->m_MapGenLoadingBroadcastTick = pGameServer->Server()->Tick();
	const char *pMsg = _("地图生成中…请稍候，请勿重复投票出发");
	if(pGameServer->m_MapGenFailed)
		pMsg = _("地图生成失败（布局/连通性），请重试");
	else if(pGameServer->m_MapGenPending)
		pMsg = _("地图生成排队中…");
	else switch(pGameServer->m_MapGenProgressStage)
	{
	case 2: pMsg = _("地图生成中… 布局与房间"); break;
	case 3: pMsg = _("地图生成中… 写入地图"); break;
	default: break;
	}
	pGameServer->SendBroadcast(-1, BROADCAST_PRIORITY_INTERFACE, pGameServer->Server()->TickSpeed() * 2, pMsg);
}

void LcResetRoundStats(CGameContext *pGameServer)
{
	for(int i = 0; i < MAX_CLIENTS; i++)
		mem_zero(&pGameServer->m_aRoundStats[i], sizeof(SLcPlayerRoundStats));
}

void LcRecordDeposit(CGameContext *pGameServer, int ClientID, int Value)
{
	if(ClientID >= 0 && ClientID < MAX_CLIENTS && Value > 0)
		pGameServer->m_aRoundStats[ClientID].m_Deposited += Value;
}

void LcRecordDeathLoss(CGameContext *pGameServer, int ClientID, int Value)
{
	if(ClientID >= 0 && ClientID < MAX_CLIENTS && Value > 0)
		pGameServer->m_aRoundStats[ClientID].m_LostOnDeath += Value;
}

void LcRecordRevive(CGameContext *pGameServer, int Helper, int Victim)
{
	if(Helper >= 0 && Helper < MAX_CLIENTS)
		pGameServer->m_aRoundStats[Helper].m_RevivesGiven++;
	if(Victim >= 0 && Victim < MAX_CLIENTS)
		pGameServer->m_aRoundStats[Victim].m_RevivesReceived++;
}

void LcRecordDeath(CGameContext *pGameServer, int ClientID)
{
	if(ClientID >= 0 && ClientID < MAX_CLIENTS)
	{
		pGameServer->m_aRoundStats[ClientID].m_Deaths++;
		pGameServer->m_CycleStats.m_TotalDeaths++;
	}
}

void LcPlayUiSound(CGameContext *pGameServer, int Sound, int ClientID)
{
	if(ClientID >= 0)
		pGameServer->CreateSound(pGameServer->GetPlayerChar(ClientID) ? pGameServer->GetPlayerChar(ClientID)->m_Pos : vec2(0, 0), Sound, CmaskOne(ClientID));
	else
		pGameServer->CreateSoundGlobal(Sound, -1);
}

void LcCheckQuotaMilestones(CGameContext *pGameServer, int PrevMoney)
{
	if(g_Config.m_GcQuota <= 0)
		return;
	int PctBefore = PrevMoney * 100 / g_Config.m_GcQuota;
	int PctNow = g_Config.m_GcMoney * 100 / g_Config.m_GcQuota;
	const int aMarks[] = {50, 80, 100};
	for(unsigned i = 0; i < sizeof(aMarks)/sizeof(aMarks[0]); i++)
	{
		if(PctBefore < aMarks[i] && PctNow >= aMarks[i])
		{
			int Need = g_Config.m_GcQuota - g_Config.m_GcMoney;
			if(Need < 0)
				Need = 0;
			pGameServer->SendChatTarget(-1, _("【指标里程碑】已达 {int:pct}% | 还差 {int:need} 元"), "pct", &aMarks[i], "need", &Need);
			LcPlayUiSound(pGameServer, SOUND_CTF_CAPTURE, -1);
		}
	}
}

void LcSendMidJoinBriefing(CGameContext *pGameServer, int ClientID)
{
	if(pGameServer->Server()->m_LocateGame != LOCATE_GAME)
		return;
	int RemainingSec = 0;
	if(g_Config.m_SvTimelimit > 0 && pGameServer->m_pController)
	{
		int LimitTicks = g_Config.m_SvTimelimit * pGameServer->Server()->TickSpeed() * 60 + pGameServer->m_pController->ExpeditionTimeBonusSec() * pGameServer->Server()->TickSpeed();
		int RemainingTicks = LimitTicks - (pGameServer->Server()->Tick() - pGameServer->m_pController->RoundStartTick());
		RemainingSec = RemainingTicks > 0 ? RemainingTicks / pGameServer->Server()->TickSpeed() : 0;
	}
	pGameServer->SendChatTarget(ClientID, _("【中途加入】你在着陆飞船附近复活"));
	pGameServer->SendChatTarget(ClientID, _("班次剩余 {int:sec} 秒 | 路线 {lstr:moon}"), "sec", &RemainingSec, "moon", LcMoonName(g_Config.m_GcMoon));
	if(pGameServer->m_LastMapGenSeed > 0)
	{
		int Seed = pGameServer->m_LastMapGenSeed;
		pGameServer->SendChatTarget(ClientID, _("本局地图种子: {int:seed}"), "seed", &Seed);
	}
}

void LcFillAircraftStatus(CLcTerminalMenu *pMenu, CGameContext *pGameServer, CLocalization *pLoc, const char *pLang)
{
	char aBuf[128];
	LcLocalizeCopy(aBuf, sizeof(aBuf), pLoc, pLang, _("【飞行器状态】"));
	pMenu->AddInfo(aBuf);
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(!pGameServer->m_apPlayers[i])
			continue;
		int Stock = pGameServer->m_aAircraftStock[i];
		if(Stock <= 0)
			continue;
		char aLine[128];
		LcFormatCopy(aLine, sizeof(aLine), pLoc, pLang, _("{str:name}: 库存 {int:stock}"), "name", pGameServer->Server()->ClientName(i), "stock", &Stock);
		pMenu->AddInfo(aLine);
	}
}

void LcAddVoteStoreBudget(CGameContext *pGameServer, int ClientID)
{
	int RecCost = GC_STORE_TIME_COST + GC_STORE_ARMOR_COST;
	pGameServer->AddVote(ClientID, "null", _("公司余额 {int:money} 币 | 推荐组合约 {int:cost} 币"), "money", &g_Config.m_GcMoney, "cost", &RecCost);
}

void LcAddSettlementMvp(CGameContext *pGameServer)
{
	int BestDep = -1, BestDepId = -1, BestRev = -1, BestRevId = -1;
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(!pGameServer->m_apPlayers[i])
			continue;
		if(pGameServer->m_aRoundStats[i].m_Deposited > BestDep)
		{
			BestDep = pGameServer->m_aRoundStats[i].m_Deposited;
			BestDepId = i;
		}
		if(pGameServer->m_aRoundStats[i].m_RevivesGiven > BestRev)
		{
			BestRev = pGameServer->m_aRoundStats[i].m_RevivesGiven;
			BestRevId = i;
		}
	}
	if(BestDepId >= 0 && BestDep > 0)
		pGameServer->SendChatTarget(-1, _("【MVP·回收】{str:name} 上交 {int:v} 元"), "name", pGameServer->Server()->ClientName(BestDepId), "v", &BestDep);
	if(BestRevId >= 0 && BestRev > 0)
		pGameServer->SendChatTarget(-1, _("【MVP·救援】{str:name} 救人 {int:n} 次"), "name", pGameServer->Server()->ClientName(BestRevId), "n", &BestRev);
}

void LcLogStorePurchase(CGameContext *pGameServer, int ClientID, const char *pItem, int Cost)
{
	(void)pItem;
	if(ClientID >= 0 && ClientID < MAX_CLIENTS)
		pGameServer->m_aStorePurchaseCount[ClientID]++;
	pGameServer->m_TotalStoreSpend += Cost;
}

void LcTickGameplaySystems(CGameContext *pGameServer)
{
	if(!pGameServer || !pGameServer->m_pController)
		return;

	LcTickMonsterProximity(pGameServer);
	LcTickMapGenLoadingHint(pGameServer);

	if(pGameServer->m_MapGenActive && pGameServer->Server()->Tick() % (pGameServer->Server()->TickSpeed() * 3) == 0)
	{
		int Stage = pGameServer->m_MapGenProgressStage;
		if(Stage < 1)
			Stage = 1;
		pGameServer->SendBroadcast(-1, BROADCAST_PRIORITY_INTERFACE, pGameServer->Server()->TickSpeed() * 2,
			_("地图生成进度… 阶段 {int:s}/3"), "s", &Stage);
	}

	if(pGameServer->Server()->m_LocateGame != LOCATE_GAME || !pGameServer->m_pController->m_pShip)
		return;

	vec2 ShipPos = pGameServer->m_pController->m_pShip->m_Pos;
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		CCharacter *pChr = pGameServer->GetPlayerChar(i);
		if(!pChr || pChr->m_InShip || pChr->m_Freeze)
		{
			if(pChr)
				pChr->m_LastZoneBand = -1;
			continue;
		}
		bool FacilityMsg = false;
		for(int r = 0; r < (int)pGameServer->m_aFacilityMarkers.size(); r++)
		{
			float Dist = distance(pChr->m_Pos, pGameServer->m_aFacilityMarkers[r].m_Pos);
			if(Dist > 160.f)
				continue;
			if(pChr->m_LastFacilityRoomType == pGameServer->m_aFacilityMarkers[r].m_Type)
				continue;
			pChr->m_LastFacilityRoomType = pGameServer->m_aFacilityMarkers[r].m_Type;
			char aLandmark[64];
			LcFormatLandmarkLabel(aLandmark, sizeof(aLandmark), pGameServer, pGameServer->m_aFacilityMarkers[r].m_Pos, ShipPos, pGameServer->m_aFacilityMarkers[r].m_Type);
			pGameServer->SendBroadcast(i, BROADCAST_PRIORITY_EFFECTSTATE, pGameServer->Server()->TickSpeed() * 2, _("【区域】进入 {str:zone}"), "zone", aLandmark);
			pChr->m_LastZoneBand = LcZoneBandFromShip(pChr->m_Pos, ShipPos);
			FacilityMsg = true;
			break;
		}
		if(!FacilityMsg)
		{
			int Band = LcZoneBandFromShip(pChr->m_Pos, ShipPos);
			if(Band != pChr->m_LastZoneBand)
			{
				pChr->m_LastZoneBand = Band;
				char aZone[32];
				LcFormatZoneLabel(aZone, sizeof(aZone), pChr->m_Pos, ShipPos);
				pGameServer->SendBroadcast(i, BROADCAST_PRIORITY_EFFECTSTATE, pGameServer->Server()->TickSpeed() * 2, _("【区域】进入 {str:zone}"), "zone", aZone);
			}
		}
	}
}
