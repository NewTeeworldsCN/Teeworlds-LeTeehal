/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/shared/config.h>
#include <game/mapitems.h>

#include <game/generated/protocol.h>

#include "gamecontroller.h"
#include "gamecontext.h"
#include "../lc/expedition/balance.h"
#include "../lc/expedition/moons.h"
#include "../lc/ui/gameplay_ui.h"
#include "../lc/economy/company_stats.h"
#include "../lc/ui/terminal_actions.h"

#include "../entities/core/pickup.h"
#include "../entities/lc/ship.h"
#include "../entities/lc/turret.h"

CGameController::CGameController(class CGameContext *pGameServer)
{
	m_pGameServer = pGameServer;
	m_pServer = m_pGameServer->Server();
	m_pGameType = "LeteehalCompany";
	m_pShip = 0;

	//
	DoWarmup(g_Config.m_SvWarmup);
	m_UnpauseTimer = 0;
	m_RoundStartTick = Server()->Tick();
	m_RoundCount = 0;
	m_GameFlags = 0;
	m_aTeamscore[TEAM_RED] = 0;
	m_aTeamscore[TEAM_BLUE] = 0;
	m_aMapWish[0] = 0;

	m_UnbalancedTick = -1;
	m_ForceBalanced = false;

	m_aNumSpawnPoints[0] = 0;
	m_aNumSpawnPoints[1] = 0;
	m_aNumSpawnPoints[2] = 0;

	m_PrepareTick = m_pServer->m_LocateGame == LOCATE_GAME ? 50 : 0;

	m_ExpeditionPhase = m_pServer->m_LocateGame == LOCATE_GAME ? LC_PHASE_EXPEDITION : LC_PHASE_LOBBY_IDLE;
	m_ReturnFinalized = false;
	m_LastRoundShipValue = 0;
	m_LastRoundPenalty = 0;
	m_LastRoundEarnings = 0;
	m_ReloadTick = 0;
	m_FacilityMarkersBuilt = false;

	m_MonsterSpawnNum = 0;
	m_MonsterSpawnCurrentNum = 0;

	m_LastLobbyBroadcastTick = 0;
	m_TimeWarningMask = 0;
	m_LastBroadcastRemainingSec = -1;
	m_ReturnDoorCloseSec = 0;
	m_LastReturnDoorBroadcastSec = -1;
	m_ExpeditionTimeBonusSec = 0;
}

CGameController::~CGameController()
{
}

float CGameController::EvaluateSpawnPos(CSpawnEval *pEval, vec2 Pos)
{
	float Score = 0.0f;
	CCharacter *pC = static_cast<CCharacter *>(GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_CHARACTER));
	for(; pC; pC = (CCharacter *)pC->TypeNext())
	{
		// team mates are not as dangerous as enemies
		float Scoremod = 1.0f;
		if(pEval->m_FriendlyTeam != -1 && pC->GetPlayer()->GetTeam() == pEval->m_FriendlyTeam)
			Scoremod = 0.5f;

		float d = distance(Pos, pC->m_Pos);
		Score += Scoremod * (d == 0 ? 1000000000.0f : 1.0f/d);
	}

	return Score;
}

bool CGameController::IsSpawnSafe(vec2 Pos) const
{
	if(GameServer()->Collision()->TestBox(Pos, vec2(28.0f, 28.0f)))
		return false;
	if(GameServer()->Collision()->CheckPoint(Pos.x, Pos.y))
		return false;
	for(int dy = 20; dy <= 52; dy += 16)
	{
		if(GameServer()->Collision()->CheckPoint(Pos.x + 14.0f, Pos.y + (float)dy))
			return true;
		if(GameServer()->Collision()->CheckPoint(Pos.x - 14.0f, Pos.y + (float)dy))
			return true;
	}
	return false;
}

bool CGameController::GetSafeSpawnNear(vec2 Center, vec2 *pOut, float MaxRadius) const
{
	if(IsSpawnSafe(Center))
	{
		*pOut = Center;
		return true;
	}

	const vec2 Offsets[] = {
		vec2(0, -32), vec2(32, 0), vec2(-32, 0), vec2(0, 32),
		vec2(32, -32), vec2(-32, -32), vec2(64, 0), vec2(-64, 0),
		vec2(0, -64), vec2(32, 32), vec2(-32, 32), vec2(64, -32),
		vec2(-64, -32), vec2(96, 0), vec2(-96, 0), vec2(0, -96),
	};

	for(int Ring = 1; Ring <= 4; Ring++)
	{
		for(unsigned i = 0; i < sizeof(Offsets)/sizeof(Offsets[0]); i++)
		{
			vec2 Pos = Center + Offsets[i] * (float)Ring;
			if(distance(Center, Pos) > MaxRadius)
				continue;
			if(IsSpawnSafe(Pos))
			{
				*pOut = Pos;
				return true;
			}
		}
	}

	return false;
}

void CGameController::EvaluateSpawnType(CSpawnEval *pEval, int Type)
{
	// get spawn point
	for(int i = 0; i < m_aNumSpawnPoints[Type]; i++)
	{
		// check if the position is occupado
		CCharacter *aEnts[MAX_CLIENTS];
		int Num = GameServer()->m_World.FindEntities(m_aaSpawnPoints[Type][i], 64, (CEntity**)aEnts, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);
		vec2 Positions[5] = { vec2(0.0f, 0.0f), vec2(-32.0f, 0.0f), vec2(0.0f, -32.0f), vec2(32.0f, 0.0f), vec2(0.0f, 32.0f) };	// start, left, up, right, down
		int Result = -1;
		for(int Index = 0; Index < 5 && Result == -1; ++Index)
		{
			Result = Index;
			for(int c = 0; c < Num; ++c)
				if(GameServer()->Collision()->CheckPoint(m_aaSpawnPoints[Type][i]+Positions[Index]) ||
					distance(aEnts[c]->m_Pos, m_aaSpawnPoints[Type][i]+Positions[Index]) <= aEnts[c]->m_ProximityRadius)
				{
					Result = -1;
					break;
				}
		}
		if(Result == -1)
			continue;	// try next spawn point

		vec2 P = m_aaSpawnPoints[Type][i]+Positions[Result];
		float S = EvaluateSpawnPos(pEval, P);
		if(!pEval->m_Got || pEval->m_Score > S)
		{
			pEval->m_Got = true;
			pEval->m_Score = S;
			pEval->m_Pos = P;
		}
	}
}

bool CGameController::CanSpawn(int Team, vec2 *pOutPos)
{
	CSpawnEval Eval;

	// spectators can't spawn
	if(Team == TEAM_SPECTATORS)
		return false;

	{
		EvaluateSpawnType(&Eval, 0);
		EvaluateSpawnType(&Eval, 1);
		EvaluateSpawnType(&Eval, 2);
	}

	if(!Eval.m_Got && Server()->m_LocateGame == LOCATE_GAME)
	{
		if(m_aNumSpawnPoints[0] > 0)
		{
			*pOutPos = m_aaSpawnPoints[0][0];
			return true;
		}
		if(m_pShip && GetSafeSpawnNear(m_pShip->m_Pos, pOutPos, 960.0f))
			return true;
	}

	*pOutPos = Eval.m_Pos;
	return Eval.m_Got;
}


bool CGameController::OnEntity(int Index, vec2 Pos)
{
	int Type = -1;
	int SubType = 0;
	Scrap Useless;
	Useless.m_InShip = false;

	switch (Index)
	{
	case ENTITY_SPAWN:
		m_aaSpawnPoints[0][m_aNumSpawnPoints[0]++] = Pos;
		break;
	
	case ENTITY_SPAWN_RED:
		m_aaSpawnPoints[1][m_aNumSpawnPoints[1]++] = Pos;
		break;
	
	case ENTITY_SPAWN_BLUE:
		m_aaSpawnPoints[2][m_aNumSpawnPoints[2]++] = Pos;
		break;

	case ENTITY_ARMOR:
		Type = POWERUP_ARMOR;
		break;
	case ENTITY_HEALTH:
		Type = POWERUP_HEALTH;
		break;
	
	case ENTITY_WEAPON_SHOTGUN:
		{
			Type = POWERUP_WEAPON;
			SubType = WEAPON_SHOTGUN;
		}
		break;
	
	case ENTITY_WEAPON_GRENADE:
	{
		Type = POWERUP_WEAPON;
		SubType = WEAPON_GRENADE;
	}
	break;
	
	case ENTITY_WEAPON_RIFLE:
	{
		Type = POWERUP_WEAPON;
		SubType = WEAPON_RIFLE;
	}
	break;
	
	case ENTITY_POWERUP_NINJA:
	if (g_Config.m_SvPowerups)
	{
		Type = POWERUP_WEAPON;
		SubType = WEAPON_NINJA;
	}
	break;

	case ENTITY_SCRAP_L1:
	case ENTITY_SCRAP_L2:
	case ENTITY_SCRAP_L3:
		new CScrap(&GameServer()->m_World, Index, Pos, true, false, Useless);
		break;
	
	case ENTITY_SHIP:
		m_pShip = new CShip(&GameServer()->m_World, Pos);
		break;

	case ENTITY_MONSTER_SPAWN:
		m_aMonsterSpawnPos.add(Pos);
		break;

	case ENTITY_TURRET:
		new CTurret(&GameServer()->m_World, Pos);
		break;
	default:
		break;
	}
	

	if(Type != -1)
	{
		CPickup *pPickup = new CPickup(&GameServer()->m_World, Type, SubType);
		pPickup->m_Pos = Pos;
		return true;
	}

	return false;
}

void CGameController::EndRound()
{
	if(m_ExpeditionPhase == LC_PHASE_RETURNING || m_ExpeditionPhase == LC_PHASE_MAP_GENERATING || m_ExpeditionPhase == LC_PHASE_DEPARTING)
		return;

	if(!m_pShip)
		return;

	GameServer()->SendChatTarget(-1, _("[公司] 班次结束 — 飞船即将起飞"));
	m_ExpeditionPhase = LC_PHASE_RETURNING;
	m_ReturnFinalized = false;
	m_ReturnDoorCloseSec = 5;
	m_LastReturnDoorBroadcastSec = -1;
	m_GameOverTick = Server()->Tick() + m_ReturnDoorCloseSec * Server()->TickSpeed();
	m_LastRoundShipValue = m_pShip->GetValue();
	m_LastRoundEarnings = m_LastRoundShipValue;
	int PrevMoney = g_Config.m_GcMoney;
	g_Config.m_GcMoney += m_LastRoundShipValue;
	LcCheckQuotaMilestones(GameServer(), PrevMoney);
	GameServer()->SendBroadcast(-1, BROADCAST_PRIORITY_GAMEANNOUNCE, Server()->TickSpeed() * 2,
		_("【公司】舱门将在 {int:sec} 秒后关闭 — 未登船将被抛弃"), "sec", &m_ReturnDoorCloseSec);
}

void CGameController::ResetGame()
{
	// GameServer()->m_World.m_ResetRequested = true;
}

static bool IsSeparator(char c) { return c == ';' || c == ' ' || c == ',' || c == '\t'; }

void CGameController::StartRound()
{
	if(m_ExpeditionPhase == LC_PHASE_MAP_GENERATING || m_ExpeditionPhase == LC_PHASE_DEPARTING || m_ExpeditionPhase == LC_PHASE_RETURNING)
		return;

	ResetGame();

	m_RoundId = rand();
	m_RoundStartTick = Server()->Tick();
	m_GameOverTick = Server()->Tick() + 10000;
	m_TimeWarningMask = 0;
	m_LastBroadcastRemainingSec = -1;
	m_ExpeditionPhase = LC_PHASE_EXPEDITION;
	m_ReturnFinalized = false;
	m_ExpeditionTimeBonusSec = GameServer()->m_NextExpeditionTimeBonusSec;
	GameServer()->m_NextExpeditionTimeBonusSec = 0;
	GameServer()->m_LastRoundBossBonus = 0;
	LcResetRoundStats(GameServer());
	GameServer()->m_World.m_Paused = false;
	for (int i = 0; i < MAX_PLAYER; i++)
	{
		if(!GameServer()->m_apPlayers[i])
			continue;

		CPlayer *pP = GameServer()->m_apPlayers[i];
		if(pP->m_LcSpectatorOptIn || GameServer()->m_aLcSpectatorOptIn[i])
		{
			pP->m_LcSpectatorOptIn = true;
			GameServer()->m_aLcSpectatorOptIn[i] = true;
			pP->SetTeam(TEAM_SPECTATORS);
			pP->m_LcExpeditionParticipant = false;
			GameServer()->SendChatTarget(i, _("本班次你以旁观者身份观看"));
			continue;
		}

		if(pP->GetTeam() == TEAM_SPECTATORS)
			continue;
		
		pP->m_Score = 0;
		GameServer()->m_apPlayers[i]->ResetScraps();
		GameServer()->m_apPlayers[i]->m_Hand = 0;
		GameServer()->m_apPlayers[i]->m_ItemCount = 0;
		GameServer()->ApplyExpeditionBonuses(i);

		Server()->GetClientSession(i)->m_RoundId = m_RoundId;
		pP->m_LcExpeditionParticipant = true;
	}
	Server()->DemoRecorder_HandleAutoStart();

	if(g_Config.m_GcRounds % GC_BOSS_ROUND_INTERVAL == 0)
	{
		const int BossType = rand() % NUM_MONSTER_TYPES;
		GameServer()->SendBroadcast(-1, BROADCAST_PRIORITY_GAMEANNOUNCE, Server()->TickSpeed() * 2, _("【公司通知】本班次为头目轮次：{lstr:name}，设施内存在高威胁目标，请谨慎行动。"), "name", LcMonsterName(BossType));
		GameServer()->NewMonster(BossType, true);
	}
}

void CGameController::ChangeMap(const char *pToMap)
{
	str_copy(m_aMapWish, pToMap, sizeof(m_aMapWish));
	EndRound();
}

void CGameController::CycleMap()
{
	// Map rotation is unused; expeditions use procedural map generation instead.
}

static int NextQuota(int Rounds)
{
	return BalanceNextQuota(Rounds);
}

// Called when the deadline is reached (gc_days == 0).
static bool HandleQuotaDeadline(CGameContext *pGameServer, IServer *pServer)
{
	g_Config.m_GcDays = GC_STARTING_DAYS;
	if(g_Config.m_GcMoney >= g_Config.m_GcQuota)
	{
		g_Config.m_GcRounds++;
		g_Config.m_GcQuota = NextQuota(g_Config.m_GcRounds);
		pGameServer->SendChatTarget(-1, _("【公司通知】指标已达成。合同续签，欢迎回来。"));
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(pGameServer->m_apPlayers[i])
			{
				LcSendQuotaCelebrationMotd(pGameServer, i);
				LcGrantAchievement(pGameServer, i, LC_ACH_QUOTA_MET, _("【成就】{str:name} 达成「指标达标」"));
			}
		}
		LcSaveCompanyStats(&pGameServer->m_CycleStats, pGameServer->m_aCareerStats);
		return true;
	}

	pGameServer->SendChatTarget(-1, _("【公司通知】指标未达成。"));
	pGameServer->SendChatTarget(-1, _("你们被解雇了。所有进度已重置。"));
	for(int i = 0; i < MAX_CLIENTS; i++)
		if(pGameServer->m_apPlayers[i])
			LcSendTerminationReviewMotd(pGameServer, i, &pGameServer->m_CycleStats);
	SLcCycleStats EmptyCycle;
	mem_zero(&EmptyCycle, sizeof(EmptyCycle));
	LcSaveCompanyStats(&EmptyCycle, pGameServer->m_aCareerStats);
	pGameServer->m_CycleStats = EmptyCycle;
	g_Config.m_GcRounds = GC_STARTING_ROUNDS;
	g_Config.m_GcQuota = GC_STARTING_QUOTA;
	pServer->m_LocateGame = LOCATE_LOBBY;
	return false;
}

static bool AnyPlayerLeftShip(CGameContext *pGameServer)
{
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		CCharacter *pChr = pGameServer->GetPlayerChar(i);
		if(pChr && !pChr->m_InShip)
			return true;
	}
	return false;
}

static void TickExpeditionTimer(CGameController *pController, CGameContext *pGameServer, IServer *pServer)
{
	if(g_Config.m_SvTimelimit <= 0 || pServer->m_LocateGame != LOCATE_GAME || pController->m_ExpeditionPhase != LC_PHASE_EXPEDITION)
		return;

	if(!AnyPlayerLeftShip(pGameServer))
		return;

	int LimitTicks = (g_Config.m_SvTimelimit * 60 + pController->ExpeditionTimeBonusSec()) * pServer->TickSpeed();
	int RemainingTicks = LimitTicks - (pServer->Tick() - pController->RoundStartTick());
	int RemainingSec = max(0, RemainingTicks / pServer->TickSpeed());

	const int aWarnings[] = {120, 60, 30, 10};
	for(unsigned i = 0; i < sizeof(aWarnings)/sizeof(aWarnings[0]); i++)
	{
		if(RemainingSec <= aWarnings[i] && !(pController->m_TimeWarningMask & (1u << i)))
		{
			pController->m_TimeWarningMask |= (1u << i);
			int WarnSec = aWarnings[i];
			pGameServer->SendChatTarget(-1, _("[公司] 班次剩余 {int:sec} 秒 — 请返回着陆飞船"), "sec", &WarnSec);
			pGameServer->CreateSoundGlobal(SOUND_WEAPON_NOAMMO, -1);
			break;
		}
	}

	if(RemainingSec <= 60 && RemainingSec > 0 && RemainingSec % 15 == 0)
		pGameServer->CreateSoundGlobal(SOUND_WEAPON_NOAMMO, -1);

	if(RemainingSec <= 120)
	{
		if(RemainingSec <= 10 && RemainingSec != pController->m_LastBroadcastRemainingSec && RemainingSec % 5 == 0)
		{
			pController->m_LastBroadcastRemainingSec = RemainingSec;
			pGameServer->SendBroadcast(-1, BROADCAST_PRIORITY_INTERFACE, pServer->TickSpeed() * 2,
				_("【班次】剩余 {int:sec} 秒 — 请返回飞船！"), "sec", &RemainingSec);
		}
		else if(RemainingSec > 10 && RemainingSec != pController->m_LastBroadcastRemainingSec && RemainingSec % 30 == 0)
		{
			pController->m_LastBroadcastRemainingSec = RemainingSec;
			pGameServer->SendBroadcast(-1, BROADCAST_PRIORITY_INTERFACE, pServer->TickSpeed() * 2,
				_("【班次】剩余 {int:sec} 秒"), "sec", &RemainingSec);
		}
	}
}

void CGameController::PostReset()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		CPlayer *pP = GameServer()->m_apPlayers[i];
		if(!pP)
			continue;

		if(Server()->m_LocateGame == LOCATE_LOBBY)
		{
			if(pP->GetTeam() == TEAM_SPECTATORS)
				pP->SetTeam(TEAM_RED);
		}
		else if(Server()->m_LocateGame == LOCATE_GAME && (pP->m_LcSpectatorOptIn || GameServer()->m_aLcSpectatorOptIn[i]))
		{
			pP->m_LcSpectatorOptIn = true;
			GameServer()->m_aLcSpectatorOptIn[i] = true;
			pP->SetTeam(TEAM_SPECTATORS);
			pP->m_Score = 0;
			pP->m_ScoreStartTick = Server()->Tick();
			continue;
		}

		pP->Respawn();
		pP->m_Score = 0;
		pP->m_ScoreStartTick = Server()->Tick();
		pP->m_RespawnTick = Server()->Tick()+Server()->TickSpeed()/2;
	}
}

void CGameController::OnPlayerInfoChange(class CPlayer *pP)
{
}


int CGameController::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon)
{
	(void)pKiller;
	(void)Weapon;
	// Death is handled by CCharacter::Die() (freeze + drop scrap) in this mod.
	(void)pVictim;
	return 0;
}

void CGameController::OnCharacterSpawn(class CCharacter *pChr)
{
	if(Server()->m_LocateGame == LOCATE_GAME)
	{
		vec2 SafePos = pChr->m_Pos;
		GetSafeSpawnNear(pChr->m_Pos, &SafePos, 640.0f);
		pChr->m_Pos = SafePos;
		pChr->m_Core.m_Pos = SafePos;
	}

	pChr->IncreaseHealth(10);
	pChr->GiveWeapon(WEAPON_HAMMER, -1);
}

void CGameController::DoWarmup(int Seconds)
{
	if(Seconds < 0)
		m_Warmup = 0;
	else
		m_Warmup = Seconds*Server()->TickSpeed();
}

void CGameController::TogglePause()
{
	if(IsGameOver())
		return;

	if(GameServer()->m_World.m_Paused)
	{
		// unpause
		if(g_Config.m_SvUnpauseTimer > 0)
			m_UnpauseTimer = g_Config.m_SvUnpauseTimer*Server()->TickSpeed();
		else
		{
			GameServer()->m_World.m_Paused = false;
			m_UnpauseTimer = 0;
		}
	}
	else
	{
		// pause
		GameServer()->m_World.m_Paused = true;
		m_UnpauseTimer = 0;
	}
}

bool CGameController::IsFriendlyFire(int ClientID1, int ClientID2)
{
	if(ClientID1 < 0 || ClientID2 < 0 || ClientID1 == ClientID2)
		return false;

	if(!GameServer()->m_apPlayers[ClientID1] || !GameServer()->m_apPlayers[ClientID2])
		return false;

	const int Team1 = GameServer()->m_apPlayers[ClientID1]->GetTeam();
	const int Team2 = GameServer()->m_apPlayers[ClientID2]->GetTeam();
	if(Team1 == TEAM_SPECTATORS || Team2 == TEAM_SPECTATORS)
		return false;

	return Team1 == Team2;
}

bool CGameController::IsForceBalanced()
{
	if(m_ForceBalanced)
	{
		m_ForceBalanced = false;
		return true;
	}
	else
		return false;
}

bool CGameController::CanBeMovedOnBalance(int ClientID)
{
	return true;
}

void CGameController::Tick()
{
	if(m_ExpeditionPhase == LC_PHASE_EXPEDITION)
		m_GameOverTick = Server()->Tick() + 1;
	
	if(Server()->m_LocateGame == LOCATE_LOBBY && m_ExpeditionPhase == LC_PHASE_MAP_GENERATING)
	{
		m_GameOverTick = Server()->Tick() + 100;
		if(GameServer()->m_MapGenFailed)
		{
			GameServer()->m_MapGenFailed = false;
			if(GameServer()->m_MapGenRetryLeft > 0)
			{
				GameServer()->m_MapGenRetryLeft--;
				GameServer()->SendChatTarget(-1, _("地图生成失败，正在自动重试…"));
				GameServer()->GenTheMap();
			}
			else
			{
				m_ExpeditionPhase = LC_PHASE_LOBBY_IDLE;
				g_Config.m_GcDays++;
				GameServer()->SendChatTarget(-1, _("地图生成失败，远征已取消"));
			}
		}
		else if(Server()->m_MapGenerated)
		{
			m_ExpeditionPhase = LC_PHASE_DEPARTING;
			m_ReloadTick = 100;
			m_FacilityMarkersBuilt = false;
			GameServer()->SendChatTarget(-1 ,_("地图生成完毕！飞船即将起飞..."));
			GameServer()->CreateSoundGlobal(SOUND_CTF_CAPTURE);
			Server()->m_LocateGame = LOCATE_GAME;
		}
	}

	if(m_ReloadTick > 0)
	{
		m_ReloadTick--;
		if(m_ReloadTick == 0)
		{
			GameServer()->Console()->ExecuteLine("reload", -1);
			m_ReloadTick--;
		}
	}

	if(Server()->m_LocateGame == LOCATE_GAME && !m_FacilityMarkersBuilt && m_ReloadTick < 0)
	{
		GameServer()->BuildFacilityMarkers();
		m_FacilityMarkersBuilt = true;
	}

	if(Server()->m_LocateGame == LOCATE_LOBBY && m_ExpeditionPhase == LC_PHASE_LOBBY_IDLE)
	{
		if(GameServer()->m_VoteStart >= GameServer()->GetNeedVoteStart())
		{
			GameServer()->m_VoteStart = 0;
			bool StartExpedition = false;
			if(g_Config.m_GcDays > 0)
			{
				g_Config.m_GcDays--;
				StartExpedition = true;
			}
			else
			{
				StartExpedition = HandleQuotaDeadline(GameServer(), Server());
			}

			if(StartExpedition)
			{
				m_PrepareTick = 0;
				LcApplyMoon(g_Config.m_GcMoon);
				GameServer()->m_MapGenRetryLeft = 1;
				GameServer()->m_MapGenLoadingBroadcastTick = 0;
				GameServer()->SendChatTarget(-1, _("正在前往 {lstr:moon} ..."), "moon", LcMoonName(g_Config.m_GcMoon));
				GameServer()->SendChatTarget(-1, _("设施地图生成中，请稍候…"));
				GameServer()->GenTheMap();
				GameServer()->SendChatTarget(-1, _("设施地图生成中..."));
				m_ExpeditionPhase = LC_PHASE_MAP_GENERATING;
			}
			else
			{
				m_ReloadTick = 150;
			}

		}
		if(Server()->Tick() - m_LastLobbyBroadcastTick >= Server()->TickSpeed() * 5)
		{
			m_LastLobbyBroadcastTick = Server()->Tick();
			GameServer()->SendBroadcast(-1, BROADCAST_PRIORITY_INTERFACE, BROADCAST_DURATION_GAMEANNOUNCE, _("你现在在：飞船\n在投票界面进行游戏选择"));
		}
		return;
	}

	if(Server()->m_LocateGame == LOCATE_GAME)
	{
		TickExpeditionTimer(this, GameServer(), Server());

		if(m_ExpeditionPhase == LC_PHASE_RETURNING)
		{
			if(!m_ReturnFinalized)
			{
				int DoorRemaining = maximum(0, (int)((m_GameOverTick - Server()->Tick()) / Server()->TickSpeed()));
				if(DoorRemaining > 0)
				{
					if(DoorRemaining != m_LastReturnDoorBroadcastSec)
					{
						m_LastReturnDoorBroadcastSec = DoorRemaining;
						GameServer()->SendBroadcast(-1, BROADCAST_PRIORITY_GAMEANNOUNCE, Server()->TickSpeed(),
							_("【舱门关闭】{int:sec} 秒"), "sec", &DoorRemaining);
						for(int i = 0; i < MAX_CLIENTS; i++)
						{
							CCharacter *pChr = GameServer()->GetPlayerChar(i);
							if(pChr && !pChr->m_InShip)
								GameServer()->SendBroadcast(i, BROADCAST_PRIORITY_GAMEANNOUNCE, Server()->TickSpeed() * 2, _("警告：你尚未登船，舱门关闭后将被抛弃！"));
						}
					}
				}
				else if(Server()->Tick() >= m_GameOverTick)
				{
					m_ReturnFinalized = true;
					m_GameOverTick = Server()->Tick() + 500;
					GameServer()->SendChatTarget(-1, _("本轮已结束！"));
					int Penalty = 0;
					int Abandoned = 0;
					for (int i = 0; i < MAX_CLIENTS; i++)
					{
						if(!GameServer()->GetPlayerChar(i))
							continue;

						CPlayer *pP = GameServer()->m_apPlayers[i];
						if(!pP->GetCharacter()->m_InShip)
						{
							int LostValue = pP->GetBackpackValue();
							if(LostValue > 0)
								GameServer()->SendChatTarget(-1, _("##{str:name} 被抛弃了! 遗失废品价值 {int:value}元"), "name", Server()->ClientName(i), "value", &LostValue);
							else
								GameServer()->SendChatTarget(-1, _("##{str:name} 被抛弃了!"), "name", Server()->ClientName(i));
							Penalty += LostValue;
							Abandoned++;
							pP->ResetScraps();
						}

						Server()->GetClientSession(i)->m_Freeze = false;
					}
					m_LastRoundPenalty = 0;
					if(Penalty > 0 && g_Config.m_GcMoney > 0)
					{
						int Sub = min(Penalty, g_Config.m_GcMoney);
						m_LastRoundPenalty = Sub;
						GameServer()->SendChatTarget(-1, _("##因队员被抛弃，公司扣除 {int:money}元"), "money", &Sub);
						g_Config.m_GcMoney -= Sub;
					}
					GameServer()->m_CycleStats.m_RoundsCompleted++;
					GameServer()->m_CycleStats.m_TotalRecovered += m_LastRoundShipValue;
					GameServer()->m_CycleStats.m_TotalPenalty += m_LastRoundPenalty;
					GameServer()->m_CycleStats.m_AbandonedTimes += Abandoned;

					GameServer()->SendChatTarget(-1, _("=== 本轮结算 ==="));
					if(GameServer()->m_LastMapGenSeed > 0)
					{
						int Seed = GameServer()->m_LastMapGenSeed;
						GameServer()->SendChatTarget(-1, _("本局地图种子: {int:seed}"), "seed", &Seed);
					}
					GameServer()->SendChatTarget(-1, _("飞船回收: {int:value}元 | 抛弃罚金: {int:penalty}元 | 公司余额: {int:money}元"), "value", &m_LastRoundShipValue, "penalty", &m_LastRoundPenalty, "money", &g_Config.m_GcMoney);
					LcAddSettlementMvp(GameServer());
					for(int i = 0; i < MAX_CLIENTS; i++)
					{
						if(!GameServer()->m_apPlayers[i])
							continue;
						LcUpdateCareerFromRound(GameServer(), i, &GameServer()->m_aRoundStats[i]);
						LcSendSettlementMotd(GameServer(), i, m_LastRoundShipValue, m_LastRoundPenalty, GameServer()->m_LastRoundBossBonus);
						if(Abandoned == 0)
							LcGrantAchievement(GameServer(), i, LC_ACH_CLEAN_RETURN, _("【成就】{str:name} 达成「零抛弃返航」"));
					}
					char aChart[128];
					LcFormatCycleChart(aChart, sizeof(aChart), GameServer()->m_CycleStats.m_RoundsCompleted, GameServer()->m_CycleStats.m_TotalRecovered, g_Config.m_GcQuota, g_Config.m_GcMoney);
					GameServer()->SendChatTarget(-1, aChart);
					LcSaveCompanyStats(&GameServer()->m_CycleStats, GameServer()->m_aCareerStats);
					LcPlayUiSound(GameServer(), SOUND_CTF_CAPTURE, -1);
					GameServer()->SendChatTarget(-1, _("$$你们将在数秒内回到飞船"));
				}
			}
			else if(Server()->Tick() - m_GameOverTick >= 0)
			{
				for(int i = 0; i < MAX_CLIENTS; i++)
				{
					if(!GameServer()->m_apPlayers[i])
						continue;
					if(GameServer()->m_apPlayers[i]->m_TerminalMenuOpen)
						GameServer()->CloseTerminalMenu(i);
				}
				GameServer()->SendChatTarget(-1, _("正在返回飞船，地图切换中请稍候…"));
				Server()->m_LocateGame = LOCATE_LOBBY;
				str_copy(g_Config.m_SvMap, g_Config.m_SvMapLobby, sizeof(g_Config.m_SvMap));
				GameServer()->Console()->ExecuteLine("reload", -1);
			}
		}
	}

	if(m_PrepareTick > 0 && Server()->m_LocateGame == LOCATE_GAME &&
		m_ExpeditionPhase != LC_PHASE_MAP_GENERATING && m_ExpeditionPhase != LC_PHASE_DEPARTING && m_ExpeditionPhase != LC_PHASE_RETURNING)
	{
		m_RoundStartTick = Server()->Tick();
		m_PrepareTick--; // c = 1, c--
		if(m_PrepareTick == 0) // c == 0
		{
			StartRound();
			m_PrepareTick--; // c == -1, c > 0(false), c == 0(false)
		}
	}
	

	// game is Paused
	if(GameServer()->m_World.m_Paused)
		++m_RoundStartTick;

	DoWincheck();
}

void CGameController::Snap(int SnappingClient)
{
	CNetObj_GameInfo *pGameInfoObj = (CNetObj_GameInfo *)Server()->SnapNewItem(NETOBJTYPE_GAMEINFO, 0, sizeof(CNetObj_GameInfo));
	if(!pGameInfoObj)
		return;

	pGameInfoObj->m_GameFlags = m_GameFlags;
	pGameInfoObj->m_GameStateFlags = 0;
	if(GameServer()->m_World.m_Paused)
		pGameInfoObj->m_GameStateFlags |= GAMESTATEFLAG_PAUSED;
	pGameInfoObj->m_RoundStartTick = m_RoundStartTick;
	pGameInfoObj->m_WarmupTimer = GameServer()->m_World.m_Paused ? m_UnpauseTimer : m_Warmup;

	pGameInfoObj->m_ScoreLimit = 0;
	pGameInfoObj->m_TimeLimit = g_Config.m_SvTimelimit;

	pGameInfoObj->m_RoundNum = (str_length(g_Config.m_SvMaprotation) && g_Config.m_SvRoundsPerMap) ? g_Config.m_SvRoundsPerMap : 0;
	pGameInfoObj->m_RoundCurrent = m_RoundCount+1;
}

int CGameController::GetAutoTeam(int NotThisID)
{
	// this will force the auto balancer to work overtime aswell
	if(g_Config.m_DbgStress)
		return 0;

	int aNumplayers[2] = {0,0};
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(GameServer()->m_apPlayers[i] && i != NotThisID)
		{
			if(GameServer()->m_apPlayers[i]->GetTeam() >= TEAM_RED && GameServer()->m_apPlayers[i]->GetTeam() <= TEAM_BLUE)
				aNumplayers[GameServer()->m_apPlayers[i]->GetTeam()]++;
		}
	}

	int Team = 0;

	if(CanJoinTeam(Team, NotThisID))
		return Team;
	return -1;
}

bool CGameController::CanJoinTeam(int Team, int NotThisID)
{
	if(Team == TEAM_SPECTATORS || (GameServer()->m_apPlayers[NotThisID] && GameServer()->m_apPlayers[NotThisID]->GetTeam() != TEAM_SPECTATORS))
		return true;

	int aNumplayers[2] = {0,0};
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(GameServer()->m_apPlayers[i] && i != NotThisID)
		{
			if(GameServer()->m_apPlayers[i]->GetTeam() >= TEAM_RED && GameServer()->m_apPlayers[i]->GetTeam() <= TEAM_BLUE)
				aNumplayers[GameServer()->m_apPlayers[i]->GetTeam()]++;
		}
	}

	return (aNumplayers[0] + aNumplayers[1]) < Server()->MaxClients()-g_Config.m_SvSpectatorSlots;
}

bool CGameController::CanChangeTeam(CPlayer *pPlayer, int JoinTeam)
{
	if(pPlayer && pPlayer->GetTeam() == TEAM_SPECTATORS && JoinTeam != TEAM_SPECTATORS &&
		Server()->m_LocateGame == LOCATE_GAME && m_ExpeditionPhase == LC_PHASE_EXPEDITION && m_PrepareTick < 0)
		return false;

	int aT[2] = {0, 0};

	if (JoinTeam == TEAM_SPECTATORS || !g_Config.m_SvTeambalanceTime)
		return true;

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		CPlayer *pP = GameServer()->m_apPlayers[i];
		if(pP && pP->GetTeam() != TEAM_SPECTATORS)
			aT[pP->GetTeam()]++;
	}

	// simulate what would happen if changed team
	aT[JoinTeam]++;
	if (pPlayer->GetTeam() != TEAM_SPECTATORS)
		aT[JoinTeam^1]--;

	// there is a player-difference of at least 2
	if(absolute(aT[0]-aT[1]) >= 2)
	{
		// player wants to join team with less players
		if ((aT[0] < aT[1] && JoinTeam == TEAM_RED) || (aT[0] > aT[1] && JoinTeam == TEAM_BLUE))
			return true;
		else
			return false;
	}
	else
		return true;
}

void CGameController::DoWincheck()
{
	if(Server()->m_LocateGame != LOCATE_GAME || m_ExpeditionPhase != LC_PHASE_EXPEDITION)
		return;

	if(GameServer()->m_VoteStart >= GameServer()->GetNeedVoteStart() && GameServer()->m_CountInGame > 0)
		EndRound();
	else if(g_Config.m_SvTimelimit > 0 &&
		(Server()->Tick()-m_RoundStartTick) >= (g_Config.m_SvTimelimit*Server()->TickSpeed()*60 + m_ExpeditionTimeBonusSec*Server()->TickSpeed()))
		EndRound();
}


int CGameController::ClampTeam(int Team)
{
	if(Team < 0)
		return TEAM_SPECTATORS;
	return 0;
}
