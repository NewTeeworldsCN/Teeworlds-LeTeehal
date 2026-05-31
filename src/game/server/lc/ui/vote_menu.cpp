#include "vote_menu.h"
#include "../../core/gamecontext.h"
#include "../../core/gameworld.h"
#include "../expedition/balance.h"
#include "guide.h"
#include "gameplay_ui.h"
#include "../expedition/moons.h"
#include "terminal_actions.h"
#include "../../entities/lc/monster.h"
#include "../../entities/lc/scrap.h"
#include "../../scrap/scrap_info.h"
#include <engine/shared/config.h>

void LcBuildVoteMenu(CGameContext *pGameServer, int ClientID)
{
		pGameServer->CreateSoundGlobal(SOUND_PICKUP_ARMOR, ClientID);
	
		CPlayer *pP = pGameServer->m_apPlayers[ClientID];
		if(pGameServer->Server()->m_LocateGame == LOCATE_GAME)
		{
			int NeedStart = pGameServer->GetNeedVoteStart();
			int Days = g_Config.m_GcDays;
			int Money = g_Config.m_GcMoney;
			int Quota = g_Config.m_GcQuota;
			int RemainingSec = 0;
			if(g_Config.m_SvTimelimit > 0)
			{
				int LimitTicks = g_Config.m_SvTimelimit * pGameServer->Server()->TickSpeed() * 60 + pGameServer->m_pController->ExpeditionTimeBonusSec() * pGameServer->Server()->TickSpeed();
				int RemainingTicks = max(0, LimitTicks - (pGameServer->Server()->Tick() - pGameServer->m_pController->RoundStartTick()));
				RemainingSec = RemainingTicks / pGameServer->Server()->TickSpeed();
			}
			pGameServer->AddVote(ClientID, "null", _("【公司终端】"));
			pGameServer->AddVote(ClientID, "null", _("F3=公司终端 | /help 1 分步教程"));
			LcAddVoteQuotaProgress(pGameServer, ClientID);
			pGameServer->AddVote(ClientID, "null", _("### 指标：{int:money}/{int:quota}"), "money", &Money, "quota", &Quota);
			pGameServer->AddVote(ClientID, "null", _("### 截止：{int:days} 天"), "days", &Days);
			if(g_Config.m_SvTimelimit > 0)
				pGameServer->AddVote(ClientID, "null", _("### 班次剩余：{int:sec} 秒"), "sec", &RemainingSec);
			pGameServer->AddVote(ClientID, "null", _("- - - - - - - - - - - -"));
			LcAddVoteTeamStatus(pGameServer, ClientID);
			LcAddVoteTeammateOverview(pGameServer, ClientID);
			pGameServer->AddVote(ClientID, "null", _("- - - - - - - - - - - -"));
			CCharacter *pChr = pGameServer->m_apPlayers[ClientID]->GetCharacter();
			const bool InShip = pChr && pChr->m_InShip;
			const bool Freeze = pChr && pChr->m_Freeze;

			if(InShip)
			{
				if(pGameServer->m_pController && pGameServer->m_pController->m_pShip)
					pGameServer->m_pController->m_pShip->RecalculateValue();
				pGameServer->AddVote(ClientID, "null", _("☪ 着陆飞船"));
				pGameServer->AddVote(ClientID, "null", _("将携带废品放入船内 (投票放下)"));
				int Num = pGameServer->m_pController->m_pShip->GetNum();
				int Value = pGameServer->m_pController->m_pShip->GetValue();
				pGameServer->AddVote(ClientID, "null", _("### 船上废品：{int:num} 件"), "num", &Num);
				pGameServer->AddVote(ClientID, "null", _("### 船上价值：{int:value}"), "value", &Value);
				LcAddVoteShipQuotaSummary(pGameServer, ClientID);
				pGameServer->AddVote(ClientID, "qstart", _("☞ 启动飞船 [{int:count}/{int:need}]"), "count", &pGameServer->m_VoteStart, "need", &NeedStart);
				ivec2 PShip = ivec2(pGameServer->m_pController->m_pShip->m_Pos.x/32, pGameServer->m_pController->m_pShip->m_Pos.y/32);
				pGameServer->AddVote(ClientID, "null", _("飞船坐标:[x:{int:x}, y:{int:y}]"), "x", &PShip.x, "y", &PShip.y);
				pGameServer->AddVote(ClientID, "null", _("###### 威胁列表 ######"));
				pGameServer->AddVote(ClientID, "help_monsters", _("☞ 怪物图鉴"));
				pGameServer->AddVote(ClientID, "refresh_monsters", _("☞ 刷新威胁列表"));
				int c = 0;
				for(int i = 0; i < MAX_MONSTERS; i++)
				{
					if(!pGameServer->m_apMonsters[i])
						continue;
					c++;
					char Name[64];
					char Hint[64];
					str_copy(Name, pGameServer->m_apMonsters[i]->MonsterName(), sizeof(Name));
					str_copy(Hint, pGameServer->m_apMonsters[i]->MonsterDescShort(), sizeof(Hint));
					ivec2 P = ivec2(pGameServer->m_apMonsters[i]->m_Pos.x/32, pGameServer->m_apMonsters[i]->m_Pos.y/32);
					pGameServer->AddVote(ClientID, "null", _("#{int:c}: {lstr:name}·{lstr:hint} [{int:x},{int:y}]"), "c", &c, "name", Name, "hint", Hint, "x", &P.x, "y", &P.y);
				}
				pGameServer->AddVote(ClientID, "null", _(" "), "count", &pGameServer->m_VoteStart, "need", &NeedStart);
				pGameServer->AddVote(ClientID, "null", _("- - - - - - - - - - - -"));
				pGameServer->AddVote(ClientID, "null", _("###### 个人背包 ######"));
				pGameServer->AddVote(ClientID, "null", _("投票理由 1 = 使用物品"));
				pGameServer->AddVote(ClientID, "null", _("理由为空 = 放入飞船"));
				pP->m_AddedWeight = 0;
			}
			else if(Freeze)
			{
				pGameServer->AddVote(ClientID, "null", _("☪ 死人无法操作"));
			}
			else
			{
				int Slots = pP->m_vScraps.size();
				int MaxSlots = GC_MAX_SCRAP_SLOTS;
				pGameServer->AddVote(ClientID, "null", _("☪ 背包 ({int:slots}/{int:max})"), "slots", &Slots, "max", &MaxSlots);
				pGameServer->AddVote(ClientID, "scan", _("☞ 扫描最近目标（或发点点表情）"));
				pGameServer->AddVote(ClientID, "help_scrap", _("☞ 废品说明"));
				pGameServer->AddVote(ClientID, "help_monsters", _("☞ 怪物图鉴"));
				pGameServer->AddVote(ClientID, "null", _("投票理由 1 = 使用物品"));
				pGameServer->AddVote(ClientID, "null", _("理由为空 = 放下物品"));
			}

			if(!Freeze)
			{
				int Lb = pP->m_AddedWeight;
				int Value = 0;
				for(int i = 0; i < pP->m_vScraps.size(); i++)
				{
					if(pP->m_vScraps[i])
					{
						Lb += pP->m_vScraps[i]->m_Weight;
						Value += pP->m_vScraps[i]->m_Value;
					}
				}
				pP->m_Weight = Lb;
				if(InShip)
					pP->m_AddedWeight = 0;
				pGameServer->AddVote(ClientID, "null", _("携带: {int:lb}镑 / {int:value}元"), "lb", &Lb, "value", &Value);
				pGameServer->AddVote(ClientID, "null", _(".-=携带废品=-."));

				for(int i = 0; i < pP->m_vScraps.size(); i++)
				{
					if(!pP->m_vScraps[i])
						continue;

					char aBuf[32];
					char aHint[64];
					str_format(aBuf, sizeof(aBuf), "scrap %d", i);
					str_copy(aHint, pGameServer->ScrapInfo()->GetScrapDescShort(pP->m_vScraps[i]->m_ScrapID), sizeof(aHint));
					pGameServer->AddVote(ClientID, aBuf, _("⊹ {lstr:name} {int:value}元/{int:weight}镑"), "name", pGameServer->ScrapInfo()->GetScrapName(pP->m_vScraps[i]->m_ScrapID), "value", &pP->m_vScraps[i]->m_Value, "weight", &pP->m_vScraps[i]->m_Weight);
					pGameServer->AddVote(ClientID, "null", _("  └ {lstr:hint}"), "hint", aHint);
				}

				if(InShip && pGameServer->m_pController && pGameServer->m_pController->m_pShip)
				{
					int ShipNum = pGameServer->m_pController->m_pShip->GetNum();
					int ShipValue = pGameServer->m_pController->m_pShip->GetValue();
					pGameServer->AddVote(ClientID, "null", _("- - - - - - - - - - - -"));
					pGameServer->AddVote(ClientID, "null", _("###### 飞船库存 ######"));
					pGameServer->AddVote(ClientID, "null", _("船上: {int:num} 件 / {int:value} 元"), "num", &ShipNum, "value", &ShipValue);
					pGameServer->AddVote(ClientID, "null", _(".-=船上废品=-."));
					int c = 0;
					for(CScrap *pScrap = (CScrap *)pGameServer->m_World.FindFirst(CGameWorld::ENTTYPE_SCRAP); pScrap; pScrap = (CScrap *)pScrap->TypeNext())
					{
						if(!pScrap->GetInShip())
							continue;
						c++;
						char aHint[64];
						int ScrapValue = pScrap->GetScrapValue();
						int ScrapWeight = pScrap->GetWeight();
						str_copy(aHint, pGameServer->ScrapInfo()->GetScrapDescShort(pScrap->GetScrapType()), sizeof(aHint));
						pGameServer->AddVote(ClientID, "null", _("⊹ #{int:c} {lstr:name} {int:value}元/{int:weight}镑"), "c", &c, "name", pGameServer->ScrapInfo()->GetScrapName(pScrap->GetScrapType()), "value", &ScrapValue, "weight", &ScrapWeight);
						pGameServer->AddVote(ClientID, "null", _("  └ {lstr:hint}"), "hint", aHint);
					}
					if(c == 0)
						pGameServer->AddVote(ClientID, "null", _("（船上暂无废品）"));
				}
			}
		}
		else
		{
			const SLcMoonConfig *pMoon = LcGetMoon(g_Config.m_GcMoon);
			int Hazard = pMoon->m_HazardStars;
			pGameServer->AddVote(ClientID, "null", _("【公司飞船终端】"));
			LcAddVoteLobbyNextSteps(pGameServer, ClientID);
			pGameServer->AddVote(ClientID, "null", _("欢迎回来，员工。达成指标，否则将被解雇。"));
			pGameServer->AddVote(ClientID, "null", _("---------------"));
			int Rounds = g_Config.m_GcRounds;
			int Days = g_Config.m_GcDays;
			int Money = g_Config.m_GcMoney;
			int Quota = g_Config.m_GcQuota;
			LcAddVoteQuotaProgress(pGameServer, ClientID);
			pGameServer->AddVote(ClientID, "null", _("第 {int:rounds} 班次"), "rounds", &Rounds);
			pGameServer->AddVote(ClientID, "null", _("截止：{int:days} 天"), "days", &Days);
			pGameServer->AddVote(ClientID, "null", _("指标：{int:money}/{int:quota}"), "money", &Money, "quota", &Quota);
			{
				pGameServer->AddVote(ClientID, "null", _("路线：{lstr:moon}"), "moon", LcMoonName(g_Config.m_GcMoon));
			}
			pGameServer->AddVote(ClientID, "null", _("危险度：{int:stars} 星"), "stars", &Hazard);
			if(pGameServer->m_LastMapGenSeed > 0 || g_Config.m_SvMapGen)
			{
				int Seed = pGameServer->m_LastMapGenSeed;
				pGameServer->AddVote(ClientID, "null", _("上局种子：{int:seed}"), "seed", &Seed);
			}
			if(pGameServer->m_pController->LastRoundShipValue() > 0 || pGameServer->m_pController->LastRoundPenalty() > 0)
			{
				int ShipValue = pGameServer->m_pController->LastRoundShipValue();
				int Penalty = pGameServer->m_pController->LastRoundPenalty();
				pGameServer->AddVote(ClientID, "null", _("上局结算 | 回收:{int:ship} 罚金:{int:penalty}"), "ship", &ShipValue, "penalty", &Penalty);
			}
			pGameServer->AddVote(ClientID, "null", _("---------------"));
			pGameServer->AddVote(ClientID, "null", _("###### 公司商店 ######"));
			LcAddVoteStoreBudget(pGameServer, ClientID);
			LcAddVoteStoreRecommendations(pGameServer, ClientID, g_Config.m_GcMoon);
			pGameServer->AddVote(ClientID, "help_store", _("☞ 商店说明"));
			pGameServer->AddVote(ClientID, "help_monsters", _("☞ 怪物图鉴"));
			{
				int CostTime = GC_STORE_TIME_COST;
				int CostFlash = GC_STORE_FLASH_COST;
				int CostArmor = GC_STORE_ARMOR_COST;
				int CostShotgun = GC_STORE_SHOTGUN_COST;
				int CostRifle = GC_STORE_RIFLE_COST;
				int CostGrenade = GC_STORE_GRENADE_COST;
				int CostMedkit = GC_STORE_MEDKIT_COST;
				int CostWhistle = GC_STORE_WHISTLE_COST;
				int CostSoda = GC_STORE_SODA_COST;
				int CostGun = GC_STORE_GUN_COST;
				int CostNinja = GC_STORE_NINJA_COST;
				int CostHealth = GC_STORE_HEALTH_COST;
				int CostAircraft = GC_STORE_AIRCRAFT_COST;
				int CostMegaphone = GC_STORE_MEGAPHONE_COST;
				int CostBoombox = GC_STORE_BOOMBOX_COST;
				int CostRemote = GC_STORE_REMOTE_COST;
				int AmmoShotgun = GC_STORE_SHOTGUN_AMMO;
				int AmmoRifle = GC_STORE_RIFLE_AMMO;
				int AmmoGrenade = GC_STORE_GRENADE_AMMO;
				int AmmoGun = GC_STORE_GUN_AMMO;
				int HealthBonus = GC_STORE_HEALTH_BONUS;
				pGameServer->AddVote(ClientID, "buy_time", _("☞ 延长班次 +2分钟 [{int:cost} 币]"), "cost", &CostTime);
				pGameServer->AddVote(ClientID, "null", _("  └ {lstr:hint}"), "hint", LcStoreItemDescShort("buy_time"));
				pGameServer->AddVote(ClientID, "buy_flash", _("☞ 闪光弹装备 [{int:cost} 币]"), "cost", &CostFlash);
				pGameServer->AddVote(ClientID, "null", _("  └ {lstr:hint}"), "hint", LcStoreItemDescShort("buy_flash"));
				pGameServer->AddVote(ClientID, "buy_armor", _("☞ 防护服 +3甲 [{int:cost} 币]"), "cost", &CostArmor);
				pGameServer->AddVote(ClientID, "null", _("  └ {lstr:hint}"), "hint", LcStoreItemDescShort("buy_armor"));
				pGameServer->AddVote(ClientID, "buy_health", _("☞ 生命保障 +{int:hp} [{int:cost} 币]"), "hp", &HealthBonus, "cost", &CostHealth);
				pGameServer->AddVote(ClientID, "null", _("  └ {lstr:hint}"), "hint", LcStoreItemDescShort("buy_health"));
				pGameServer->AddVote(ClientID, "buy_aircraft", _("☞ 飞行器 [{int:cost} 币]"), "cost", &CostAircraft);
				pGameServer->AddVote(ClientID, "null", _("  └ {lstr:hint}"), "hint", LcStoreItemDescShort("buy_aircraft"));
				pGameServer->AddVote(ClientID, "buy_shotgun", _("☞ 散弹枪 {int:ammo}发 [{int:cost} 币]"), "ammo", &AmmoShotgun, "cost", &CostShotgun);
				pGameServer->AddVote(ClientID, "null", _("  └ {lstr:hint}"), "hint", LcStoreItemDescShort("buy_shotgun"));
				pGameServer->AddVote(ClientID, "buy_rifle", _("☞ 激光枪 {int:ammo}发 [{int:cost} 币]"), "ammo", &AmmoRifle, "cost", &CostRifle);
				pGameServer->AddVote(ClientID, "null", _("  └ {lstr:hint}"), "hint", LcStoreItemDescShort("buy_rifle"));
				pGameServer->AddVote(ClientID, "buy_grenade", _("☞ 榴弹 {int:ammo}发 [{int:cost} 币]"), "ammo", &AmmoGrenade, "cost", &CostGrenade);
				pGameServer->AddVote(ClientID, "null", _("  └ {lstr:hint}"), "hint", LcStoreItemDescShort("buy_grenade"));
				pGameServer->AddVote(ClientID, "buy_gun", _("☞ 手枪 {int:ammo}发 [{int:cost} 币]"), "ammo", &AmmoGun, "cost", &CostGun);
				pGameServer->AddVote(ClientID, "null", _("  └ {lstr:hint}"), "hint", LcStoreItemDescShort("buy_gun"));
				pGameServer->AddVote(ClientID, "buy_ninja", _("☞ 忍者刀 [{int:cost} 币]"), "cost", &CostNinja);
				pGameServer->AddVote(ClientID, "null", _("  └ {lstr:hint}"), "hint", LcStoreItemDescShort("buy_ninja"));
				pGameServer->AddVote(ClientID, "buy_medkit", _("☞ 急救包 [{int:cost} 币]"), "cost", &CostMedkit);
				pGameServer->AddVote(ClientID, "null", _("  └ {lstr:hint}"), "hint", LcStoreItemDescShort("buy_medkit"));
				pGameServer->AddVote(ClientID, "buy_whistle", _("☞ 驱虫哨 [{int:cost} 币]"), "cost", &CostWhistle);
				pGameServer->AddVote(ClientID, "null", _("  └ {lstr:hint}"), "hint", LcStoreItemDescShort("buy_whistle"));
				pGameServer->AddVote(ClientID, "buy_soda", _("☞ 能量汽水 [{int:cost} 币]"), "cost", &CostSoda);
				pGameServer->AddVote(ClientID, "null", _("  └ {lstr:hint}"), "hint", LcStoreItemDescShort("buy_soda"));
				pGameServer->AddVote(ClientID, "buy_megaphone", _("☞ 扩音器 [{int:cost} 币]"), "cost", &CostMegaphone);
				pGameServer->AddVote(ClientID, "null", _("  └ {lstr:hint}"), "hint", LcStoreItemDescShort("buy_megaphone"));
				pGameServer->AddVote(ClientID, "buy_boombox", _("☞ 音响 [{int:cost} 币]"), "cost", &CostBoombox);
				pGameServer->AddVote(ClientID, "null", _("  └ {lstr:hint}"), "hint", LcStoreItemDescShort("buy_boombox"));
				pGameServer->AddVote(ClientID, "buy_remote", _("☞ 遥控器 [{int:cost} 币]"), "cost", &CostRemote);
				pGameServer->AddVote(ClientID, "null", _("  └ {lstr:hint}"), "hint", LcStoreItemDescShort("buy_remote"));
			}
			pGameServer->AddVote(ClientID, "null", _("---------------"));
			pGameServer->AddVote(ClientID, "null", _("###### 选择路线 ######"));
			LcAddVoteMoonInfoCards(pGameServer, ClientID);
			pGameServer->AddVote(ClientID, "moon 0", LcMoonRouteLabel(0));
			pGameServer->AddVote(ClientID, "moon 1", LcMoonRouteLabel(1));
			pGameServer->AddVote(ClientID, "moon 2", LcMoonRouteLabel(2));
			pGameServer->AddVote(ClientID, "moon 3", LcMoonRouteLabel(3));
			pGameServer->AddVote(ClientID, "moon 4", LcMoonRouteLabel(4));
			pGameServer->AddVote(ClientID, "null", _("---------------"));
			if(pGameServer->m_CountInGame < g_Config.m_SvLessPlayerStart)
			{
				int Need = g_Config.m_SvLessPlayerStart;
				pGameServer->AddVote(ClientID, "null", _("需要至少 {int:need} 名员工才能出发"), "need", &Need);
			}
			else
			{
				int NeedStart = pGameServer->GetNeedVoteStart();
				pGameServer->AddVote(ClientID, "qstart", _("☞ 出发 [{int:count}/{int:need}]"), "count", &pGameServer->m_VoteStart, "need", &NeedStart);
			}
		}
}
