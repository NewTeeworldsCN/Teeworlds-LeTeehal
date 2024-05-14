/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/shared/config.h>
#include <game/mapitems.h>

#include <game/generated/protocol.h>

#include "gamecontroller.h"
#include "gamecontext.h"

#include "entities/pickup.h"
#include "entities/ship.h"

CGameController::CGameController(class CGameContext *pGameServer)
{
	m_pGameServer = pGameServer;
	m_pServer = m_pGameServer->Server();
	m_pGameType = "LeteehalCompany";

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

	m_PrepareTick = 50;

	m_LaunchShip = false;
	m_ReloadTick = 0;

	m_MonsterSpawnNum = 0;
	m_MonsterSpawnCurrentNum = 0;

	m_EndRound2 = false;
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

	*pOutPos = Eval.m_Pos;
	return Eval.m_Got;
}


bool CGameController::OnEntity(int Index, vec2 Pos)
{
	int Type = -1;
	int SubType = 0;
	Scrap Useless;

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
	if(m_LaunchShip)
		return;

	dbg_msg("sdasda", "sdaa");

	GameServer()->SendChatTarget(-1, _("[警告]为保证公司利益最大化，飞船已起飞"));
	m_LaunchShip = true;
	g_Config.m_GcMoney += m_pShip->GetValue();
}

void CGameController::ResetGame()
{
	// GameServer()->m_World.m_ResetRequested = true;
}

static bool IsSeparator(char c) { return c == ';' || c == ' ' || c == ',' || c == '\t'; }

void CGameController::StartRound()
{
	ResetGame();

	m_RoundStartTick = Server()->Tick();
	m_GameOverTick = Server()->Tick() + 10000;
	GameServer()->m_World.m_Paused = false;
	for (int i = 0; i < MAX_PLAYER; i++)
	{
		if(!GameServer()->m_apPlayers[i])
			continue;
		
		GameServer()->m_apPlayers[i]->m_Score = 0;
		GameServer()->m_apPlayers[i]->ResetScraps();
		GameServer()->m_apPlayers[i]->m_Hand = 0;
		GameServer()->m_apPlayers[i]->m_ItemCount = 0;
	}
	Server()->DemoRecorder_HandleAutoStart();
}

void CGameController::ChangeMap(const char *pToMap)
{
	str_copy(m_aMapWish, pToMap, sizeof(m_aMapWish));
	EndRound();
}

void CGameController::CycleMap()
{

}

void CGameController::PostReset()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(GameServer()->m_apPlayers[i])
		{
			GameServer()->m_apPlayers[i]->Respawn();
			GameServer()->m_apPlayers[i]->m_Score = 0;
			GameServer()->m_apPlayers[i]->m_ScoreStartTick = Server()->Tick();
			GameServer()->m_apPlayers[i]->m_RespawnTick = Server()->Tick()+Server()->TickSpeed()/2;
		}
	}
}

void CGameController::OnPlayerInfoChange(class CPlayer *pP)
{
}


int CGameController::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon)
{
	pVictim->GetPlayer()->SetTeam(TEAM_SPECTATORS);
	return 0;
}

void CGameController::OnCharacterSpawn(class CCharacter *pChr)
{
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
	return true;
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
	if(!m_LaunchShip)
		m_GameOverTick = Server()->Tick() + 1;
	
	if(Server()->m_LocateGame == LOCATE_LOBBY && m_LaunchShip)
	{
		m_GameOverTick = Server()->Tick() + 100;
		if(Server()->m_MapGenerated)
		{
			m_ReloadTick = 100;
			GameServer()->SendChatTarget(-1 ,_("地图生成完毕！飞船即将起飞..."));
			GameServer()->CreateSoundGlobal(SOUND_CTF_CAPTURE);
			Server()->m_LocateGame = LOCATE_GAME;
		}
	}

	if(m_ReloadTick > 0)
	{
		m_ReloadTick--; // c = 1, c--
		if(m_ReloadTick == 0) // c == 0
		{
			GameServer()->Console()->ExecuteLine("reload", -1);;
			m_ReloadTick--; // c == -1, c > 0(false), c == 0(false)
		}
	}

	if(Server()->m_LocateGame == LOCATE_LOBBY && !m_LaunchShip)
	{
		if(
			// Time end.
			(g_Config.m_SvTimelimit > 0 && (Server()->Tick()-m_RoundStartTick) >= g_Config.m_SvTimelimit*Server()->TickSpeed()*60)
			|| 
			// Vote passed.
			(GameServer()->m_VoteStart >= GameServer()->GetNeedVoteStart())
		)
		{
			bool Game = false;
			GameServer()->m_VoteStart = 0;
			if(!g_Config.m_GcDays)
			{
				g_Config.m_GcDays = 3;
				if(g_Config.m_GcMoney >= g_Config.m_GcQuota)
				{
					g_Config.m_GcDays = 3;
					g_Config.m_GcRounds++;
					g_Config.m_GcQuota = g_Config.m_GcRounds*300+20*g_Config.m_GcRounds;
					GameServer()->SendChatTarget(-1, _("你们没有被解雇，你们会继续在这里工作."));
					Game = true;
				}
				else
				{
					GameServer()->SendChatTarget(-1, _("你们没能达成指标..."));
					GameServer()->SendChatTarget(-1, _("你们被解雇了."));
					g_Config.m_GcDays = 3;
					g_Config.m_GcRounds = 1;
					g_Config.m_GcQuota = 300;
					Server()->m_LocateGame = LOCATE_LOBBY;
					m_ReloadTick = 150;
					Game = false;
				}
			}
			else
			{
				Game = true;
				g_Config.m_GcDays--;
			}
			if(Game)
			{
				GameServer()->GenTheMap();
				GameServer()->SendChatTarget(-1, _("游戏地图生成中..."));
				m_LaunchShip = true;
			}

		}
		GameServer()->SendBroadcast(-1, BROADCAST_PRIORITY_INTERFACE, BROADCAST_DURATION_GAMEANNOUNCE, _("你现在在：飞船\n在投票界面进行游戏选择"));
		return;
	}

	if(Server()->m_LocateGame == LOCATE_GAME)
	{
		if(GameServer()->m_VoteStart >= GameServer()->GetNeedVoteStart() && GameServer()->m_CountInGame > 0 && !m_LaunchShip)
		{
			m_GameOverTick = Server()->Tick() + 100;
			EndRound();
		}

		if(Server()->Tick() - m_GameOverTick >= 0 && m_LaunchShip)
		{
			if(m_EndRound2)
			{
				Server()->m_LocateGame = LOCATE_LOBBY;
				str_copy(g_Config.m_SvMap, g_Config.m_SvMapLobby, sizeof(g_Config.m_SvMap));
				GameServer()->Console()->ExecuteLine("reload", -1);
			}
			else
			{
				m_EndRound2 = true;
				m_GameOverTick = Server()->Tick() + 500;
				GameServer()->SendChatTarget(-1, _("本轮已结束！"));
				int Count = 0;
				for (int i = 0; i < MAX_CLIENTS; i++)
				{
					if(!GameServer()->GetPlayerChar(i))
						continue;
	
					if(!GameServer()->m_apPlayers[i]->GetCharacter()->m_InShip)
					{
						Count++;
						GameServer()->SendChatTarget(-1, _("##{str:name} 被抛弃了!"), "name", Server()->ClientName(i));
						GameServer()->m_apPlayers[i]->GetCharacter()->m_Freeze = true;
					}
				}
				if(Count > 0)
				{
					int Sub = g_Config.m_GcMoney - (g_Config.m_GcMoney / clamp(Count, 1, g_Config.m_GcMoney));
					GameServer()->SendChatTarget(-1, _("##扣除{int:money}元"), "money", &Sub);
					g_Config.m_GcMoney -= Sub;
				}
				GameServer()->SendChatTarget(-1, _("$$你们将在数秒内回到飞船"));
			}
		}
	}

	if(m_PrepareTick > 0)
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

bool CGameController::CheckTeamBalance()
{
}

bool CGameController::CanChangeTeam(CPlayer *pPlayer, int JoinTeam)
{
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
	// check win
	if (g_Config.m_SvTimelimit > 0 && 
	(Server()->Tick()-m_RoundStartTick) >= g_Config.m_SvTimelimit*Server()->TickSpeed()*60 || GameServer()->m_VoteStart >= GameServer()->GetNeedVoteStart())
		EndRound();
}


int CGameController::ClampTeam(int Team)
{
	if(Team < 0)
		return TEAM_SPECTATORS;
	return 0;
}
