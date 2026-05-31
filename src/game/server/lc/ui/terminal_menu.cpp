#include "terminal_menu.h"

#include "../expedition/balance.h"
#include "guide.h"
#include "gameplay_ui.h"
#include "localize_util.h"
#include "../expedition/moons.h"
#include "terminal_actions.h"
#include "../../entities/core/character.h"
#include "../../entities/lc/monster.h"
#include "../../core/gamecontext.h"
#include "../../core/player.h"
#include "../../scrap/scrap_info.h"

#include <engine/shared/config.h>
#include <game/generated/protocol.h>
#include <teeuniverses/components/localization.h>

void CLcTerminalMenu::Clear()
{
	m_aEntries.clear();
}

void CLcTerminalMenu::AddInfo(const char *pLabel)
{
	SEntry Entry;
	str_copy(Entry.m_aLabel, pLabel, sizeof(Entry.m_aLabel));
	Entry.m_aCommand[0] = 0;
	m_aEntries.add(Entry);
}

void CLcTerminalMenu::AddAction(const char *pCmd, const char *pLabel)
{
	SEntry Entry;
	str_copy(Entry.m_aLabel, pLabel, sizeof(Entry.m_aLabel));
	str_copy(Entry.m_aCommand, pCmd, sizeof(Entry.m_aCommand));
	m_aEntries.add(Entry);
}

void CLcTerminalMenu::AppendMotdLine(std::string &Motd, const char *pLine) const
{
	if((int)Motd.size() >= TERMINAL_MOTD_MAX - 2)
		return;

	int LineCount = 1;
	for(size_t i = 0; i < Motd.size(); i++)
	{
		if(Motd[i] == '\n')
			LineCount++;
	}
	if(LineCount >= 64)
		return;

	if(!pLine)
		return;

	Motd.append(pLine);
	Motd.push_back('\n');
}

int CLcTerminalMenu::NumActions() const
{
	int Num = 0;
	for(int i = 0; i < m_aEntries.size(); i++)
	{
		if(m_aEntries[i].m_aCommand[0])
			Num++;
	}
	return Num;
}

int CLcTerminalMenu::NumInfoLines() const
{
	int Num = 0;
	for(int i = 0; i < m_aEntries.size(); i++)
	{
		if(!m_aEntries[i].m_aCommand[0])
			Num++;
	}
	return Num;
}

void CLcTerminalMenu::AppendMotdLineLoc(std::string &Motd, CLocalization *pLoc, const char *pLang, const char *pKey) const
{
	char aBuf[256];
	LcLocalizeCopy(aBuf, sizeof(aBuf), pLoc, pLang, pKey);
	AppendMotdLine(Motd, aBuf);
}

static bool MotdAppendLine(char *pBuf, int Size, const char *pLine)
{
	if(!pBuf || Size <= 1 || !pLine)
		return false;
	int Len = str_length(pBuf);
	if(Len >= Size - 2)
		return false;
	if(Len > 0)
	{
		pBuf[Len++] = '\n';
		pBuf[Len] = 0;
	}
	const int LenBefore = str_length(pBuf);
	str_append(pBuf, pLine, Size);
	return str_length(pBuf) > LenBefore;
}

static void MotdTruncateUtf8(char *pBuf, int MaxBytes)
{
	if(!pBuf)
		return;
	int Len = str_length(pBuf);
	if(Len <= MaxBytes)
		return;
	int Size = MaxBytes;
	while(Size > 0 && ((unsigned char)pBuf[Size - 1] & 0xC0) == 0x80)
		Size--;
	pBuf[Size] = 0;
}

void CLcTerminalMenu::AddStoreBuyLoc(const char *pCmd, const char *pNameKey, int Cost, CLocalization *pLoc, const char *pLang)
{
	char aLabel[128];
	LcFormatCopy(aLabel, sizeof(aLabel), pLoc, pLang, _("{lstr:name} [{int:cost} 币]"),
		"name", pNameKey, "cost", &Cost);
	char aFullCmd[64];
	str_format(aFullCmd, sizeof(aFullCmd), "lcm_buy %s", pCmd);
	AddAction(aFullCmd, aLabel);
}

static void AddInfoLoc(CLcTerminalMenu *pMenu, CLocalization *pLoc, const char *pLang, const char *pKey)
{
	char aBuf[128];
	LcLocalizeCopy(aBuf, sizeof(aBuf), pLoc, pLang, pKey);
	pMenu->AddInfo(aBuf);
}

static void AddActionLoc(CLcTerminalMenu *pMenu, CLocalization *pLoc, const char *pLang, const char *pCmd, const char *pKey)
{
	char aBuf[128];
	LcLocalizeCopy(aBuf, sizeof(aBuf), pLoc, pLang, pKey);
	pMenu->AddAction(pCmd, aBuf);
}

static void FillMainPage(CLcTerminalMenu *pMenu, CGameContext *pGameServer, CPlayer *pP, int ClientID, CLocalization *pLoc, const char *pLang)
{
	char aStatus[128];
	int Money = g_Config.m_GcMoney;
	int Quota = g_Config.m_GcQuota;
	int Days = g_Config.m_GcDays;
	char aProgress[32];
	LcFormatQuotaProgress(aProgress, sizeof(aProgress), Money, Quota);
	LcFormatCopy(aStatus, sizeof(aStatus), pLoc, pLang, _("指标 {int:money}/{int:quota} | {int:days}天"),
		"money", &Money, "quota", &Quota, "days", &Days);
	pMenu->AddInfo(aStatus);
	pMenu->AddInfo(aProgress);
	AddInfoLoc(pMenu, pLoc, pLang, _("ESC=投票 | 滚轮选 | 开火确认 | 钩索返回"));
	pMenu->AddInfo("");

	if(pGameServer->Server()->m_LocateGame == LOCATE_LOBBY)
	{
		LcFillLobbyNextSteps(pMenu, pLoc, pLang);
		pMenu->AddInfo("");
		AddActionLoc(pMenu, pLoc, pLang, "lcm_goto 1", _("☞ 公司商店"));
		AddActionLoc(pMenu, pLoc, pLang, "lcm_goto 7", _("☞ 选择路线"));
		AddActionLoc(pMenu, pLoc, pLang, "lcm_goto 3", _("☞ 图鉴"));
		if(pGameServer->m_CountInGame >= g_Config.m_SvLessPlayerStart)
		{
			char aLaunch[96];
			int NeedStart = pGameServer->GetNeedVoteStart();
			LcFormatCopy(aLaunch, sizeof(aLaunch), pLoc, pLang, _("☞ 出发 [{int:count}/{int:need}]"),
				"count", &pGameServer->m_VoteStart, "need", &NeedStart);
			pMenu->AddAction("lcm_vote qstart", aLaunch);
		}
	}
	else
	{
		CCharacter *pChr = pP->GetCharacter();
		AddActionLoc(pMenu, pLoc, pLang, "lcm_goto 20", _("☞ 队员状态"));
		AddActionLoc(pMenu, pLoc, pLang, "lcm_goto 21", _("☞ 分步教程"));
		if(pChr && pChr->m_InShip)
		{
			LcFillShipQuotaSummary(pMenu, pGameServer, pLoc, pLang);
			pMenu->AddInfo("");
			int NeedStart = pGameServer->GetNeedVoteStart();
			char aLaunch[96];
			LcFormatCopy(aLaunch, sizeof(aLaunch), pLoc, pLang, _("☞ 启动飞船 [{int:count}/{int:need}]"),
				"count", &pGameServer->m_VoteStart, "need", &NeedStart);
			pMenu->AddAction("lcm_vote qstart", aLaunch);
			AddActionLoc(pMenu, pLoc, pLang, "lcm_goto 8", _("☞ 个人背包"));
			AddActionLoc(pMenu, pLoc, pLang, "lcm_goto 16", _("☞ 飞船库存"));
			AddActionLoc(pMenu, pLoc, pLang, "lcm_goto 10", _("☞ 威胁列表"));
			AddActionLoc(pMenu, pLoc, pLang, "lcm_goto 5", _("☞ 怪物图鉴"));
		}
		else if(pChr && !pChr->m_Freeze)
		{
			char aStock[64];
			int Stock = pGameServer->m_aAircraftStock[ClientID];
			LcFormatCopy(aStock, sizeof(aStock), pLoc, pLang, _("飞行器库存: {int:stock}"), "stock", &Stock);
			pMenu->AddInfo(aStock);
			AddActionLoc(pMenu, pLoc, pLang, "lcm_goto 2", _("☞ 部署飞行器"));
			AddActionLoc(pMenu, pLoc, pLang, "lcm_goto 8", _("☞ 背包"));
			AddActionLoc(pMenu, pLoc, pLang, "lcm_vote scan", _("☞ 扫描"));
			AddActionLoc(pMenu, pLoc, pLang, "lcm_goto 3", _("☞ 图鉴"));
		}
		else
		{
			AddInfoLoc(pMenu, pLoc, pLang, _("☪ 死人无法操作"));
			AddActionLoc(pMenu, pLoc, pLang, "lcm_goto 3", _("☞ 图鉴"));
		}
	}
}

void CLcTerminalMenu::Populate(CGameContext *pGameServer, int ClientID)
{
	CPlayer *pP = pGameServer->m_apPlayers[ClientID];
	if(!pP)
		return;
	if(!pP->GetCharacter() && pGameServer->Server()->m_LocateGame != LOCATE_LOBBY)
		return;

	Clear();

	CLocalization *pLoc = pGameServer->Server()->Localization();
	const char *pLang = pP->GetLanguage();

	switch(pP->m_TerminalMenuPage)
	{
	case LC_PAGE_WELCOME:
		AddInfoLoc(this, pLoc, pLang, _("=== 欢迎加入 Tee命公司 ==="));
		AddInfoLoc(this, pLoc, pLang, _("你是公司雇员：在截止日前达成指标，否则被解雇。"));
		AddInfo("");
		AddInfoLoc(this, pLoc, pLang, _("【三步开始】"));
		AddInfoLoc(this, pLoc, pLang, _("1. ESC 投票 → 选路线 → 投票出发"));
		AddInfoLoc(this, pLoc, pLang, _("2. F3 终端 → 商店购买 → 查看图鉴"));
		AddInfoLoc(this, pLoc, pLang, _("3. 设施内锤子拾取废品 → 回着陆飞船"));
		AddInfo("");
		AddInfoLoc(this, pLoc, pLang, _("输入 /help 1 查看分步教程 | /status 查看任务"));
		AddActionLoc(this, pLoc, pLang, "lcm_goto 21", _("☞ 分步教程"));
		AddActionLoc(this, pLoc, pLang, "lcm_goto 0", _("⏎ 进入终端"));
		break;
	case LC_PAGE_TEAM:
		LcFillTeamStatusInfo(this, pGameServer, pLoc, pLang);
		LcFillTeammateOverview(this, pGameServer, pLoc, pLang);
		LcFillAircraftStatus(this, pGameServer, pLoc, pLang);
		AddInfo("");
		AddActionLoc(this, pLoc, pLang, "lcm_goto 0", _("⏎ 返回"));
		break;
	case LC_PAGE_HELP_TUT:
		AddInfoLoc(this, pLoc, pLang, _("【教程 1/4】大厅：ESC 投票选路线，全员投票出发"));
		AddInfoLoc(this, pLoc, pLang, _("F3 终端可购买商店、查看图鉴"));
		AddInfo("");
		AddInfoLoc(this, pLoc, pLang, _("【教程 2/4】设施内：锤子拾取废品，4 格背包"));
		AddInfoLoc(this, pLoc, pLang, _("重量越大移动越慢，注意班次倒计时"));
		AddInfo("");
		AddInfoLoc(this, pLoc, pLang, _("【教程 3/4】ESC/F3：理由 1=使用物品，理由空=放下/放入飞船"));
		AddInfoLoc(this, pLoc, pLang, _("冻结时用队友锤子救活；登船自动修复"));
		AddInfo("");
		AddInfoLoc(this, pLoc, pLang, _("【教程 4/4】全员登船后投票启动飞船返航"));
		AddInfoLoc(this, pLoc, pLang, _("截止日前达成指标，否则全进度重置"));
		AddInfoLoc(this, pLoc, pLang, _("/help store|monsters|scrap 查看图鉴 | /status 任务状态"));
		AddActionLoc(this, pLoc, pLang, "lcm_goto 0", _("⏎ 返回"));
		break;
	case LC_PAGE_STORE:
	{
		AddInfoLoc(this, pLoc, pLang, _("☪ 公司商店"));
		AddInfoLoc(this, pLoc, pLang, _("购买后下趟出发生效"));
		LcFillStoreRecommendations(this, pLoc, pLang, g_Config.m_GcMoon);
		AddInfo("");
		{
			int Count = 0;
			const SLcStoreItem *pItems = LcGetStoreItems(&Count);
			for(int i = 0; i < Count; i++)
				AddStoreBuyLoc(pItems[i].m_pCmd, LcStoreItemName(pItems[i].m_pCmd), pItems[i].m_Cost, pLoc, pLang);
		}
		AddActionLoc(this, pLoc, pLang, "lcm_goto 0", _("⏎ 返回"));
		break;
	}
	case LC_PAGE_AIRCRAFT:
	{
		int Stock = pGameServer->m_aAircraftStock[ClientID];
		char aStock[64];
		LcFormatCopy(aStock, sizeof(aStock), pLoc, pLang, _("库存: {int:stock}"), "stock", &Stock);
		AddInfoLoc(this, pLoc, pLang, _("☪ 公司飞行器"));
		AddInfoLoc(this, pLoc, pLang, _("左键射击 | 右键按朝向升降 | 空格下机"));
		AddInfoLoc(this, pLoc, pLang, _("自动吸附附近废品(不含飞船内)，最多16件"));
		AddInfoLoc(this, pLoc, pLang, _("飞回飞船区域自动卸下"));
		AddInfo(aStock);
		AddInfo("");
		if(Stock > 0)
			AddActionLoc(this, pLoc, pLang, "lcm_deploy_aircraft", _("☞ 部署飞行器"));
		else
			AddInfoLoc(this, pLoc, pLang, _("商店购买后下趟获得库存"));
		AddActionLoc(this, pLoc, pLang, "lcm_goto 0", _("⏎ 返回"));
		break;
	}
	case LC_PAGE_GUIDE:
		AddInfoLoc(this, pLoc, pLang, _("☪ 图鉴"));
		AddActionLoc(this, pLoc, pLang, "lcm_goto 4", _("☞ 商店说明"));
		AddActionLoc(this, pLoc, pLang, "lcm_goto 5", _("☞ 怪物图鉴"));
		AddActionLoc(this, pLoc, pLang, "lcm_goto 6", _("☞ 废品说明"));
		AddActionLoc(this, pLoc, pLang, "lcm_goto 0", _("⏎ 返回"));
		break;
	case LC_PAGE_GUIDE_STORE:
		LcFillGuideStoreIndex(this, pLoc, pLang);
		break;
	case LC_PAGE_GUIDE_STORE_SUPPLIES:
		LcFillGuideStoreCategory(this, 0, pLoc, pLang);
		break;
	case LC_PAGE_GUIDE_STORE_WEAPONS:
		LcFillGuideStoreCategory(this, 1, pLoc, pLang);
		break;
	case LC_PAGE_GUIDE_STORE_FIELD:
		LcFillGuideStoreCategory(this, 2, pLoc, pLang);
		break;
	case LC_PAGE_GUIDE_MONSTERS:
		LcFillGuideMonsterIndex(this, pLoc, pLang);
		break;
	case LC_PAGE_GUIDE_SCRAP:
		LcFillGuideScrapIndex(this, pLoc, pLang);
		break;
	case LC_PAGE_GUIDE_SCRAP_L1:
		LcFillGuideScrapTier(this, pGameServer, 0, pLoc, pLang);
		break;
	case LC_PAGE_GUIDE_SCRAP_L2:
		LcFillGuideScrapTier(this, pGameServer, 1, pLoc, pLang);
		break;
	case LC_PAGE_GUIDE_SCRAP_L3:
		LcFillGuideScrapTier(this, pGameServer, 2, pLoc, pLang);
		break;
	case LC_PAGE_MOON:
	{
		const SLcMoonConfig *pMoon = LcGetMoon(g_Config.m_GcMoon);
		dynamic_string aRoute;
		pGameServer->Server()->Localization()->Format_L(aRoute, pP->GetLanguage(), _("当前: {lstr:moon} {int:stars}星"),
			"moon", LcMoonName(g_Config.m_GcMoon), "stars", &pMoon->m_HazardStars);
		char aRouteLine[128];
		str_copy(aRouteLine, aRoute.buffer(), sizeof(aRouteLine));
		AddInfoLoc(this, pLoc, pLang, _("☪ 选择路线"));
		AddInfo(aRouteLine);
		AddInfo("");
		AddActionLoc(this, pLoc, pLang, "lcm_vote moon 0", LcMoonRouteLabel(0));
		AddActionLoc(this, pLoc, pLang, "lcm_vote moon 1", LcMoonRouteLabel(1));
		AddActionLoc(this, pLoc, pLang, "lcm_vote moon 2", LcMoonRouteLabel(2));
		AddActionLoc(this, pLoc, pLang, "lcm_vote moon 3", LcMoonRouteLabel(3));
		AddActionLoc(this, pLoc, pLang, "lcm_vote moon 4", LcMoonRouteLabel(4));
		AddActionLoc(this, pLoc, pLang, "lcm_goto 0", _("⏎ 返回"));
		break;
	}
	case LC_PAGE_INVENTORY:
		LcFillInventoryIndex(this, pGameServer, pP, pLoc, pLang);
		break;
	case LC_PAGE_INVENTORY_USE:
		LcFillInventoryUseList(this, pGameServer, pP, pLoc, pLang);
		break;
	case LC_PAGE_INVENTORY_DROP:
		LcFillInventoryDropList(this, pGameServer, pP, pLoc, pLang);
		break;
	case LC_PAGE_SHIP_CARGO:
		LcFillShipCargoIndex(this, pGameServer, pLoc, pLang);
		break;
	case LC_PAGE_THREATS:
	{
		AddInfoLoc(this, pLoc, pLang, _("☪ 威胁列表"));
		AddInfo("");
		int c = 0;
		for(int i = 0; i < MAX_MONSTERS; i++)
		{
			if(!pGameServer->m_apMonsters[i])
				continue;
			c++;
			char aLine[192];
			ivec2 P = ivec2(pGameServer->m_apMonsters[i]->m_Pos.x / 32, pGameServer->m_apMonsters[i]->m_Pos.y / 32);
			LcFormatCopy(aLine, sizeof(aLine), pLoc, pLang, _("#{int:c} {lstr:name}·{lstr:desc} [{int:x},{int:y}]"),
				"c", &c, "name", pGameServer->m_apMonsters[i]->MonsterName(),
				"desc", pGameServer->m_apMonsters[i]->MonsterDescShort(), "x", &P.x, "y", &P.y);
			AddInfo(aLine);
		}
		if(c == 0)
			AddInfoLoc(this, pLoc, pLang, _("当前无已知威胁"));
		AddActionLoc(this, pLoc, pLang, "lcm_vote refresh_monsters", _("☞ 刷新列表"));
		AddActionLoc(this, pLoc, pLang, "lcm_goto 0", _("⏎ 返回"));
		break;
	}
	case LC_PAGE_MAIN:
		FillMainPage(this, pGameServer, pP, ClientID, pLoc, pLang);
		break;
	default:
		if(LcTerminalIsGuideScrapDetail(pP->m_TerminalMenuPage))
		{
			LcFillGuideScrapDetail(this, pGameServer, pP->m_TerminalMenuPage - LC_PAGE_GUIDE_SCRAP_DETAIL_BASE, pLoc, pLang);
			break;
		}
		if(LcTerminalIsGuideStoreDetail(pP->m_TerminalMenuPage))
		{
			LcFillGuideStoreDetail(this, pP->m_TerminalMenuPage - LC_PAGE_GUIDE_STORE_DETAIL_BASE, pLoc, pLang);
			break;
		}
		if(LcTerminalIsGuideMonsterDetail(pP->m_TerminalMenuPage))
		{
			LcFillGuideMonsterDetail(this, pP->m_TerminalMenuPage - LC_PAGE_GUIDE_MONSTER_BASE, pLoc, pLang);
			break;
		}
		if(LcTerminalIsInventoryDetail(pP->m_TerminalMenuPage))
		{
			LcFillInventoryDetail(this, pGameServer, pP, pP->m_TerminalMenuPage - LC_PAGE_INVENTORY_DETAIL_BASE, pLoc, pLang);
			break;
		}
		FillMainPage(this, pGameServer, pP, ClientID, pLoc, pLang);
		break;
	}
}

void CLcTerminalMenu::SendMotd(CGameContext *pGameServer, int ClientID, int SelectedAction)
{
	CPlayer *pPlayer = pGameServer->m_apPlayers[ClientID];
	if(!pPlayer)
		return;

	Populate(pGameServer, ClientID);

	const int NumInfoItems = NumInfoLines();
	const int NumActionItems = NumActions();
	int TextScroll = pPlayer->m_TerminalMenuTextScroll;
	if(TextScroll < 0)
		TextScroll = 0;
	else if(TextScroll > maximum(0, NumInfoItems - TERMINAL_VISIBLE_INFO))
		TextScroll = maximum(0, NumInfoItems - TERMINAL_VISIBLE_INFO);
	if(SelectedAction < 0)
		SelectedAction = 0;
	else if(SelectedAction >= NumActionItems)
		SelectedAction = maximum(0, NumActionItems - 1);

	int WindowStart = 0;
	int WindowEnd = NumActionItems;
	if(NumActionItems > TERMINAL_VISIBLE_ACTIONS)
	{
		WindowStart = SelectedAction - TERMINAL_VISIBLE_ACTIONS / 2;
		if(WindowStart < 0)
			WindowStart = 0;
		WindowEnd = WindowStart + TERMINAL_VISIBLE_ACTIONS;
		if(WindowEnd > NumActionItems)
		{
			WindowEnd = NumActionItems;
			WindowStart = WindowEnd - TERMINAL_VISIBLE_ACTIONS;
			if(WindowStart < 0)
				WindowStart = 0;
		}
	}

	const int InfoWindowEnd = NumInfoItems > TERMINAL_VISIBLE_INFO ? TextScroll + TERMINAL_VISIBLE_INFO : NumInfoItems;
	char *pMotd = pPlayer->m_aTerminalMotd;
	pMotd[0] = 0;
	bool MotdTruncated = false;

	CLocalization *pLoc = pGameServer->Server()->Localization();
	const char *pLang = pPlayer->GetLanguage();
	char aBuf[256];

	LcLocalizeCopy(aBuf, sizeof(aBuf), pLoc, pLang, _("=== 公司终端 ==="));
	if(!MotdAppendLine(pMotd, TERMINAL_MOTD_MAX, aBuf))
		MotdTruncated = true;
	LcLocalizeCopy(aBuf, sizeof(aBuf), pLoc, pLang, _("滚轮选择/开火确认/钩索返回"));
	if(!MotdAppendLine(pMotd, TERMINAL_MOTD_MAX, aBuf))
		MotdTruncated = true;

	if(NumActionItems > TERMINAL_VISIBLE_ACTIONS)
	{
		str_format(aBuf, sizeof(aBuf), "[%d/%d]", SelectedAction + 1, NumActionItems);
		if(!MotdAppendLine(pMotd, TERMINAL_MOTD_MAX, aBuf))
			MotdTruncated = true;
	}

	if(NumInfoItems > 0 || NumActionItems > 0)
		if(!MotdAppendLine(pMotd, TERMINAL_MOTD_MAX, ""))
			MotdTruncated = true;

	if(TextScroll > 0)
		if(!MotdAppendLine(pMotd, TERMINAL_MOTD_MAX, "  ..."))
			MotdTruncated = true;

	int InfoIndex = 0;
	for(int i = 0; i < m_aEntries.size(); i++)
	{
		if(m_aEntries[i].m_aCommand[0])
			continue;
		if(InfoIndex >= TextScroll && InfoIndex < InfoWindowEnd)
			if(!MotdAppendLine(pMotd, TERMINAL_MOTD_MAX, m_aEntries[i].m_aLabel))
				MotdTruncated = true;
		InfoIndex++;
	}

	if(InfoWindowEnd < NumInfoItems)
		if(!MotdAppendLine(pMotd, TERMINAL_MOTD_MAX, "  ..."))
			MotdTruncated = true;

	if(NumActionItems > 0)
		if(!MotdAppendLine(pMotd, TERMINAL_MOTD_MAX, ""))
			MotdTruncated = true;

	if(WindowStart > 0)
		if(!MotdAppendLine(pMotd, TERMINAL_MOTD_MAX, "  ..."))
			MotdTruncated = true;

	int ActionIndex = 0;
	for(int i = 0; i < m_aEntries.size(); i++)
	{
		if(!m_aEntries[i].m_aCommand[0])
			continue;
		if(ActionIndex >= WindowStart && ActionIndex < WindowEnd)
		{
			if(ActionIndex == SelectedAction)
				str_format(aBuf, sizeof(aBuf), "> %s", m_aEntries[i].m_aLabel);
			else
				str_format(aBuf, sizeof(aBuf), "  %s", m_aEntries[i].m_aLabel);
			if(!MotdAppendLine(pMotd, TERMINAL_MOTD_MAX, aBuf))
				MotdTruncated = true;
		}
		ActionIndex++;
	}

	if(WindowEnd < NumActionItems)
		if(!MotdAppendLine(pMotd, TERMINAL_MOTD_MAX, "  ..."))
			MotdTruncated = true;

	if(NumActionItems == 0)
	{
		LcLocalizeCopy(aBuf, sizeof(aBuf), pLoc, pLang, _("本页无可用操作"));
		if(!MotdAppendLine(pMotd, TERMINAL_MOTD_MAX, aBuf))
			MotdTruncated = true;
	}

	if(MotdTruncated)
	{
		LcLocalizeCopy(aBuf, sizeof(aBuf), pLoc, pLang, _("--- 内容未完全显示，请滚轮查看 ---"));
		MotdAppendLine(pMotd, TERMINAL_MOTD_MAX, aBuf);
	}

	if(pMotd[0] == 0)
		str_copy(pMotd, "=== LeTeehal Terminal ===", TERMINAL_MOTD_MAX);

	MotdTruncateUtf8(pMotd, TERMINAL_MOTD_MAX - 1);

	CNetMsg_Sv_Motd Msg;
	Msg.m_pMessage = pMotd;
	pGameServer->Server()->SendPackMsg(&Msg, MSGFLAG_VITAL|MSGFLAG_FLUSH, ClientID);
}

bool CLcTerminalMenu::ExecuteAction(CGameContext *pGameServer, CPlayer *pPlayer, int ActionIndex)
{
	if(ActionIndex < 0 || ActionIndex >= NumActions())
		return false;

	int Current = 0;
	for(int i = 0; i < m_aEntries.size(); i++)
	{
		if(!m_aEntries[i].m_aCommand[0])
			continue;

		if(Current == ActionIndex)
		{
			const char *pCmd = m_aEntries[i].m_aCommand;
			const int ClientID = pPlayer->GetCID();

			if(str_comp_num(pCmd, "lcm_goto ", 9) == 0)
			{
				const int Page = str_toint(pCmd + 9);
				if(!LcTerminalIsValidPage(Page))
					return false;
				pPlayer->m_TerminalMenuPage = Page;
				pPlayer->m_TerminalMenuSelection = 0;
				pPlayer->m_TerminalMenuTextScroll = 0;
				return true;
			}
			if(str_comp_num(pCmd, "lcm_buy ", 8) == 0)
			{
				pGameServer->ExecutePlayerVoteCommand(ClientID, pCmd + 8, "");
				pGameServer->ResetVotes(ClientID);
				return true;
			}
			if(str_comp_num(pCmd, "lcm_vote ", 9) == 0)
			{
				pGameServer->ExecutePlayerVoteCommand(ClientID, pCmd + 9, "");
				pGameServer->ResetVotes(ClientID);
				return true;
			}
			if(str_comp_num(pCmd, "lcm_scrap use ", 14) == 0)
			{
				char aVoteCmd[32];
				str_format(aVoteCmd, sizeof(aVoteCmd), "scrap %d", str_toint(pCmd + 14));
				pGameServer->ExecutePlayerVoteCommand(ClientID, aVoteCmd, "1");
				pGameServer->ResetVotes(ClientID);
				return true;
			}
			if(str_comp_num(pCmd, "lcm_scrap drop ", 15) == 0)
			{
				char aVoteCmd[32];
				str_format(aVoteCmd, sizeof(aVoteCmd), "scrap %d", str_toint(pCmd + 15));
				pGameServer->ExecutePlayerVoteCommand(ClientID, aVoteCmd, "");
				pGameServer->ResetVotes(ClientID);
				return true;
			}
			if(str_comp(pCmd, "lcm_deploy_aircraft") == 0)
			{
				pGameServer->DeployAircraft(ClientID);
				return true;
			}
			return true;
		}
		Current++;
	}

	return false;
}
