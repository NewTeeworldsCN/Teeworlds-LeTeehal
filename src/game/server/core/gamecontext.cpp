/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <new>
#include <base/math.h>
#include <base/system.h>
#include <engine/shared/config.h>
#include <engine/map.h>
#include <engine/console.h>
#include "gamecontext.h"
#include <game/server/lc/mapgen/mapgen_random.h>
#include <game/version.h>
#include <game/collision.h>
#include <game/gamecore.h>
#include "../entities/lc/scrap.h"
#include <engine/shared/datafile.h> // MapGen

#include <teeuniverses/components/localization.h>
#include "../entities/lc/ship.h"
#include "../lc/expedition/balance.h"
#include "../lc/expedition/moons.h"
#include "../lc/ui/guide.h"
#include "../lc/ui/gameplay_ui.h"
#include "../lc/economy/company_stats.h"
#include "../lc/ui/terminal_actions.h"
#include "../scrap/scrap_info.h"
#include "../entities/lc/monster.h"
#include "../entities/lc/scan_link.h"
#include "../entities/lc/hazard_marker.h"
#include "../lc/hazards/hazards.h"
#include "../entities/vehicle/aircraft.h"
#include "../entities/vehicle/vehicle_util.h"

enum
{
	RESET,
	NO_RESET
};

void CGameContext::Construct(int Resetting)
{
	m_Resetting = 0;
	m_pServer = 0;

	for(int i = 0; i < MAX_CLIENTS; i++)
		m_apPlayers[i] = 0;

	for(int i = 0; i < MAX_MONSTERS; i++)
		m_apMonsters[i] = 0;

	m_pController = 0;
	m_VoteCloseTime = 0;
	m_pVoteOptionFirst = 0;
	m_pVoteOptionLast = 0;
	m_NumVoteOptions = 0;
	m_LockTeams = 0;
	m_MapGenPending = false;
	m_MapGenActive = false;
	m_MapGenApplying = false;
	m_MapGenFailed = false;
	m_LastMapGenSeed = 0;
	m_MapGenRetryLeft = 0;
	m_MapGenLoadingBroadcastTick = 0;
	m_LastRoundBossBonus = 0;
	mem_zero(m_aRoundStats, sizeof(m_aRoundStats));
	mem_zero(&m_CycleStats, sizeof(m_CycleStats));
	mem_zero(m_aCareerStats, sizeof(m_aCareerStats));
	mem_zero(m_aStorePurchaseCount, sizeof(m_aStorePurchaseCount));
	m_TotalStoreSpend = 0;
	m_MapGenProgressStage = 0;
	m_LastQuotaMilestonePct = 0;
	m_pMapGenJob = 0;
	m_FacilityMarkersBuilt = false;
	m_NextExpeditionTimeBonusSec = 0;
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		m_aNextArmorBonus[i] = 0;
		m_aNextFlashBonus[i] = 0;
		m_aStoreBonus[i].Reset();
		m_aAircraftStock[i] = 0;
	}
	m_ConsoleOutputHandle_ChatPrint = -1;
	m_ConsoleOutput_Target = -1;

	if(Resetting==NO_RESET)
		m_pVoteOptionHeap = new CHeap();
}

CGameContext::CGameContext(int Resetting)
{
	Construct(Resetting);
}

CGameContext::CGameContext()
{
	Construct(NO_RESET);
}

CGameContext::~CGameContext()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
		delete m_apPlayers[i];
	for(int i = 0; i < MAX_MONSTERS; i++)
		delete m_apMonsters[i];
	if(!m_Resetting)
		delete m_pVoteOptionHeap;
}

void CGameContext::OnSetAuthed(int ClientID, int Level)
{
	if(m_apPlayers[ClientID])
		m_apPlayers[ClientID]->m_Authed = Level;
}

void CGameContext::Clear()
{
	CHeap *pVoteOptionHeap = m_pVoteOptionHeap;
	CVoteOptionServer *pVoteOptionFirst = m_pVoteOptionFirst;
	CVoteOptionServer *pVoteOptionLast = m_pVoteOptionLast;
	int NumVoteOptions = m_NumVoteOptions;
	CTuningParams Tuning = m_Tuning;

	CLcStoreBonus aStoreBonus[MAX_CLIENTS];
	int aNextArmorBonus[MAX_CLIENTS];
	int aNextFlashBonus[MAX_CLIENTS];
	int aAircraftStock[MAX_CLIENTS];
	int NextExpeditionTimeBonusSec = m_NextExpeditionTimeBonusSec;
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		aStoreBonus[i] = m_aStoreBonus[i];
		aNextArmorBonus[i] = m_aNextArmorBonus[i];
		aNextFlashBonus[i] = m_aNextFlashBonus[i];
		aAircraftStock[i] = m_aAircraftStock[i];
	}

	m_Resetting = true;
	this->~CGameContext();
	mem_zero(this, sizeof(*this));
	new (this) CGameContext(RESET);

	m_pVoteOptionHeap = pVoteOptionHeap;
	m_pVoteOptionFirst = pVoteOptionFirst;
	m_pVoteOptionLast = pVoteOptionLast;
	m_NumVoteOptions = NumVoteOptions;
	m_Tuning = Tuning;
	m_NextExpeditionTimeBonusSec = NextExpeditionTimeBonusSec;
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		m_aStoreBonus[i] = aStoreBonus[i];
		m_aNextArmorBonus[i] = aNextArmorBonus[i];
		m_aNextFlashBonus[i] = aNextFlashBonus[i];
		m_aAircraftStock[i] = aAircraftStock[i];
		m_aBroadcastStates[i].m_NoChangeTick = 0;
		m_aBroadcastStates[i].m_LifeSpanTick = 0;
		m_aBroadcastStates[i].m_Priority = BROADCAST_PRIORITY_LOWEST;
		m_aBroadcastStates[i].m_aPrevMessage[0] = 0;
		m_aBroadcastStates[i].m_aNextMessage[0] = 0;
	}
}


class CCharacter *CGameContext::GetPlayerChar(int ClientID)
{
	if(ClientID < 0 || ClientID >= MAX_CLIENTS || !m_apPlayers[ClientID])
		return 0;
	return m_apPlayers[ClientID]->GetCharacter();
}

void CGameContext::CreateDamageInd(vec2 Pos, float Angle, int Amount)
{
	float a = 3 * 3.14159f / 2 + Angle;
	//float a = get_angle(dir);
	float s = a-pi/3;
	float e = a+pi/3;
	for(int i = 0; i < Amount; i++)
	{
		float f = mix(s, e, float(i+1)/float(Amount+2));
		CNetEvent_DamageInd *pEvent = (CNetEvent_DamageInd *)m_Events.Create(NETEVENTTYPE_DAMAGEIND, sizeof(CNetEvent_DamageInd));
		if(pEvent)
		{
			pEvent->m_X = (int)Pos.x;
			pEvent->m_Y = (int)Pos.y;
			pEvent->m_Angle = (int)(f*256.0f);
		}
	}
}

void CGameContext::CreateHammerHit(vec2 Pos)
{
	// create the event
	CNetEvent_HammerHit *pEvent = (CNetEvent_HammerHit *)m_Events.Create(NETEVENTTYPE_HAMMERHIT, sizeof(CNetEvent_HammerHit));
	if(pEvent)
	{
		pEvent->m_X = (int)Pos.x;
		pEvent->m_Y = (int)Pos.y;
	}
}


void CGameContext::CreateExplosion(vec2 Pos, int Owner, int Weapon, bool NoDamage)
{
	// create the event
	CNetEvent_Explosion *pEvent = (CNetEvent_Explosion *)m_Events.Create(NETEVENTTYPE_EXPLOSION, sizeof(CNetEvent_Explosion));
	if(pEvent)
	{
		pEvent->m_X = (int)Pos.x;
		pEvent->m_Y = (int)Pos.y;
	}

	if (!NoDamage)
	{
		// deal damage
		CCharacter *apEnts[MAX_CLIENTS];
		float Radius = 135.0f;
		float InnerRadius = 48.0f;
		int Num = m_World.FindEntities(Pos, Radius, (CEntity**)apEnts, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);
		for(int i = 0; i < Num; i++)
		{
			vec2 Diff = apEnts[i]->m_Pos - Pos;
			vec2 ForceDir(0,1);
			float l = length(Diff);
			if(l)
				ForceDir = normalize(Diff);
			l = 1-clamp((l-InnerRadius)/(Radius-InnerRadius), 0.0f, 1.0f);
			float Dmg = 6 * l;
			if((int)Dmg)
				apEnts[i]->TakeDamage(ForceDir*Dmg*2, (int)Dmg, Owner, Weapon);
		}
		DamageMonstersInRadius(Pos, Owner, Weapon, Radius, InnerRadius, GC_PLAYER_GRENADE_MONSTER_DMG);
	}
}

/*
void create_smoke(vec2 Pos)
{
	// create the event
	EV_EXPLOSION *pEvent = (EV_EXPLOSION *)events.create(EVENT_SMOKE, sizeof(EV_EXPLOSION));
	if(pEvent)
	{
		pEvent->x = (int)Pos.x;
		pEvent->y = (int)Pos.y;
	}
}*/

void CGameContext::CreatePlayerSpawn(vec2 Pos)
{
	// create the event
	CNetEvent_Spawn *ev = (CNetEvent_Spawn *)m_Events.Create(NETEVENTTYPE_SPAWN, sizeof(CNetEvent_Spawn));
	if(ev)
	{
		ev->m_X = (int)Pos.x;
		ev->m_Y = (int)Pos.y;
	}
}

void CGameContext::CreateDeath(vec2 Pos, int ClientID)
{
	// create the event
	CNetEvent_Death *pEvent = (CNetEvent_Death *)m_Events.Create(NETEVENTTYPE_DEATH, sizeof(CNetEvent_Death));
	if(pEvent)
	{
		pEvent->m_X = (int)Pos.x;
		pEvent->m_Y = (int)Pos.y;
		pEvent->m_ClientID = ClientID;
	}
}

void CGameContext::CreateSound(vec2 Pos, int Sound, int Mask)
{
	if (Sound < 0)
		return;

	// create a sound
	CNetEvent_SoundWorld *pEvent = (CNetEvent_SoundWorld *)m_Events.Create(NETEVENTTYPE_SOUNDWORLD, sizeof(CNetEvent_SoundWorld), Mask);
	if(pEvent)
	{
		pEvent->m_X = (int)Pos.x;
		pEvent->m_Y = (int)Pos.y;
		pEvent->m_SoundID = Sound;
	}
}

void CGameContext::CreateSoundGlobal(int Sound, int Target)
{
	if (Sound < 0)
		return;

	CNetMsg_Sv_SoundGlobal Msg;
	Msg.m_SoundID = Sound;
	if(Target == -2)
		Server()->SendPackMsg(&Msg, MSGFLAG_NOSEND, -1);
	else
	{
		int Flag = MSGFLAG_VITAL;
		if(Target != -1)
			Flag |= MSGFLAG_NORECORD;
		Server()->SendPackMsg(&Msg, Flag, Target);
	}
}


void CGameContext::SendChatTarget(int To, const char *pText, ...)
{
	int Start = (To < 0 ? 0 : To);
	int End = (To < 0 ? MAX_CLIENTS : To+1);
	
	CNetMsg_Sv_Chat Msg;
	Msg.m_Team = 0;
	Msg.m_ClientID = -1;
	
	dynamic_string Buffer;
	
	va_list VarArgs;
	va_start(VarArgs, pText);
	
	for(int i = Start; i < End; i++)
	{
		if(m_apPlayers[i])
		{
			Buffer.clear();
			Server()->Localization()->Format_VL(Buffer, m_apPlayers[i]->GetLanguage(), pText, VarArgs);
			
			Msg.m_pMessage = Buffer.buffer();
			Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, i);
		}
	}
	
	va_end(VarArgs);
}


void CGameContext::SendChat(int ChatterClientID, int Team, const char *pText)
{
	char aBuf[256];
	if(ChatterClientID >= 0 && ChatterClientID < MAX_CLIENTS)
		str_format(aBuf, sizeof(aBuf), "%d:%d:%s: %s", ChatterClientID, Team, Server()->ClientName(ChatterClientID), pText);
	else
		str_format(aBuf, sizeof(aBuf), "*** %s", pText);
	Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, Team!=CHAT_ALL?"teamchat":"chat", aBuf);

	if(Team == CHAT_ALL)
	{
		CNetMsg_Sv_Chat Msg;
		Msg.m_Team = 0;
		Msg.m_ClientID = ChatterClientID;
		Msg.m_pMessage = pText;
		Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, -1);
	}
	else
	{
		CNetMsg_Sv_Chat Msg;
		Msg.m_Team = 1;
		Msg.m_ClientID = ChatterClientID;
		Msg.m_pMessage = pText;

		// pack one for the recording only
		Server()->SendPackMsg(&Msg, MSGFLAG_VITAL|MSGFLAG_NOSEND, -1);

		// send to the clients
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(m_apPlayers[i] && m_apPlayers[i]->GetTeam() == Team)
				Server()->SendPackMsg(&Msg, MSGFLAG_VITAL|MSGFLAG_NORECORD, i);
		}
	}
}

void CGameContext::SendEmoticon(int ClientID, int Emoticon)
{
	CNetMsg_Sv_Emoticon Msg;
	Msg.m_ClientID = ClientID;
	Msg.m_Emoticon = Emoticon;
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, -1);
}

void CGameContext::SendWeaponPickup(int ClientID, int Weapon)
{
	CNetMsg_Sv_WeaponPickup Msg;
	Msg.m_Weapon = Weapon;
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, ClientID);
}


void CGameContext::SendBroadcast(int To, int Priority, int LifeSpan, const char *pText, ...)
{
	int Start = (To < 0 ? 0 : To);
	int End = (To < 0 ? MAX_PLAYER : To+1);
	
	dynamic_string Buffer;
	
	va_list VarArgs;
	va_start(VarArgs, pText);
	
	for(int i = Start; i < End; i++)
	{
		if(m_apPlayers[i])
		{
			va_list ArgsCopy;
			va_copy(ArgsCopy, VarArgs);
			Buffer.clear();
			Server()->Localization()->Format_VL(Buffer, m_apPlayers[i]->GetLanguage(), pText, ArgsCopy);
			va_end(ArgsCopy);
			AddBroadcast(i, Buffer.buffer(), Priority, LifeSpan);
		}
	}
	
	va_end(VarArgs);
}

//
void CGameContext::StartVote(const char *pDesc, const char *pCommand, const char *pReason)
{
	// check if a vote is already running
	if(m_VoteCloseTime)
		return;

	// reset votes
	m_VoteEnforce = VOTE_ENFORCE_UNKNOWN;
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(m_apPlayers[i])
		{
			m_apPlayers[i]->m_Vote = 0;
			m_apPlayers[i]->m_VotePos = 0;
		}
	}

	// start vote
	m_VoteCloseTime = time_get() + time_freq()*25;
	str_copy(m_aVoteDescription, pDesc, sizeof(m_aVoteDescription));
	str_copy(m_aVoteCommand, pCommand, sizeof(m_aVoteCommand));
	str_copy(m_aVoteReason, pReason, sizeof(m_aVoteReason));
	SendVoteSet(-1);
	m_VoteUpdate = true;
}


void CGameContext::EndVote()
{
	m_VoteCloseTime = 0;
	SendVoteSet(-1);
}

void CGameContext::SendVoteSet(int ClientID)
{
	CNetMsg_Sv_VoteSet Msg;
	if(m_VoteCloseTime)
	{
		Msg.m_Timeout = (m_VoteCloseTime-time_get())/time_freq();
		Msg.m_pDescription = m_aVoteDescription;
		Msg.m_pReason = m_aVoteReason;
	}
	else
	{
		Msg.m_Timeout = 0;
		Msg.m_pDescription = "";
		Msg.m_pReason = "";
	}
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, ClientID);
}

void CGameContext::SendVoteStatus(int ClientID, int Total, int Yes, int No)
{
	CNetMsg_Sv_VoteStatus Msg = {0};
	Msg.m_Total = Total;
	Msg.m_Yes = Yes;
	Msg.m_No = No;
	Msg.m_Pass = Total - (Yes+No);

	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, ClientID);

}

void CGameContext::AbortVoteKickOnDisconnect(int ClientID)
{
	if(m_VoteCloseTime && ((!str_comp_num(m_aVoteCommand, "kick ", 5) && str_toint(&m_aVoteCommand[5]) == ClientID) ||
		(!str_comp_num(m_aVoteCommand, "set_team ", 9) && str_toint(&m_aVoteCommand[9]) == ClientID)))
		m_VoteCloseTime = -1;
}


void CGameContext::CheckPureTuning()
{
	// might not be created yet during start up
	if(!m_pController)
		return;
}

void CGameContext::SendTuningParams(int ClientID)
{
	CheckPureTuning();

	CMsgPacker Msg(NETMSGTYPE_SV_TUNEPARAMS);
	int *pParams = (int *)&m_Tuning;
	for(unsigned i = 0; i < sizeof(m_Tuning)/sizeof(int); i++)
		Msg.AddInt(pParams[i]);
	Server()->SendMsg(&Msg, MSGFLAG_VITAL, ClientID);
}

void CGameContext::SwapTeams()
{
}

void CGameContext::OnTick()
{
	ProcessMapGen();

	// check tuning
	CheckPureTuning();

	// copy tuning
	m_World.m_Core.m_Tuning = m_Tuning;
	m_World.Tick();

	if(m_pController)
		m_pController->Tick();

	LcTickGameplaySystems(this);

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(m_apPlayers[i])
		{
			m_apPlayers[i]->Tick();
			m_apPlayers[i]->PostTick();
		}
	}

	if(Server()->Tick() % (Server()->TickSpeed()*120) == 0)
	{
		SendChatTarget(-1, _("=== 优秀员工 ==="));
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(!m_apPlayers[i] || m_apPlayers[i]->GetTeam() == TEAM_SPECTATORS)
				continue;
			int Earn = Server()->GetClientSession(i)->m_TotalEarn;
			if(Earn > 0)
				SendChatTarget(-1, _("{str:name}: {int:earn}元"), "name", Server()->ClientName(i), "earn", &Earn);
		}
	}

	// Check for new broadcast
	for (int i = 0; i < MAX_PLAYER; i++)
	{
		if (m_apPlayers[i])
		{
			if (m_aBroadcastStates[i].m_LifeSpanTick > 0 && m_aBroadcastStates[i].m_TimedPriority > m_aBroadcastStates[i].m_Priority)
			{
				str_copy(m_aBroadcastStates[i].m_aNextMessage, m_aBroadcastStates[i].m_aTimedMessage, sizeof(m_aBroadcastStates[i].m_aNextMessage));
			}

			// Send broadcast only if the message is different, or to fight auto-fading
			if (
				str_comp(m_aBroadcastStates[i].m_aPrevMessage, m_aBroadcastStates[i].m_aNextMessage) != 0 ||
				(m_aBroadcastStates[i].m_NoChangeTick > Server()->TickSpeed() && str_length(m_aBroadcastStates[i].m_aNextMessage) > 0))
			{
				CNetMsg_Sv_Broadcast Msg;
				str_copy(m_aBroadcastStates[i].m_aPrevMessage, m_aBroadcastStates[i].m_aNextMessage, sizeof(m_aBroadcastStates[i].m_aPrevMessage));
				Msg.m_pMessage = m_aBroadcastStates[i].m_aNextMessage;
				Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, i);
				m_aBroadcastStates[i].m_NoChangeTick = 0;
			}
			else
			{
				m_aBroadcastStates[i].m_NoChangeTick++;
			}
			// Update broadcast state
			if (m_aBroadcastStates[i].m_LifeSpanTick > 0)
				m_aBroadcastStates[i].m_LifeSpanTick--;

			if (m_aBroadcastStates[i].m_LifeSpanTick <= 0)
			{
				m_aBroadcastStates[i].m_aTimedMessage[0] = 0;
				m_aBroadcastStates[i].m_TimedPriority = BROADCAST_PRIORITY_LOWEST;
			}
			m_aBroadcastStates[i].m_aNextMessage[0] = 0;
			m_aBroadcastStates[i].m_Priority = BROADCAST_PRIORITY_LOWEST;
		}
		else
		{
			m_aBroadcastStates[i].m_NoChangeTick = 0;
			m_aBroadcastStates[i].m_LifeSpanTick = 0;
			m_aBroadcastStates[i].m_Priority = BROADCAST_PRIORITY_LOWEST;
			m_aBroadcastStates[i].m_TimedPriority = BROADCAST_PRIORITY_LOWEST;
			m_aBroadcastStates[i].m_aPrevMessage[0] = 0;
			m_aBroadcastStates[i].m_aNextMessage[0] = 0;
			m_aBroadcastStates[i].m_aTimedMessage[0] = 0;
		}
	}

	// update voting
	if(m_VoteCloseTime)
	{
		// abort the kick-vote on player-leave
		if(m_VoteCloseTime == -1)
		{
			SendChat(-1, CGameContext::CHAT_ALL, "Vote aborted");
			EndVote();
		}
		else
		{
			int Total = 0, Yes = 0, No = 0;
			if(m_VoteUpdate)
			{
				// count votes
				char aaBuf[MAX_CLIENTS][NETADDR_MAXSTRSIZE] = {{0}};
				for(int i = 0; i < MAX_CLIENTS; i++)
					if(m_apPlayers[i])
						Server()->GetClientAddr(i, aaBuf[i], NETADDR_MAXSTRSIZE);
				bool aVoteChecked[MAX_CLIENTS] = {0};
				for(int i = 0; i < MAX_CLIENTS; i++)
				{
					if(!m_apPlayers[i] || m_apPlayers[i]->GetTeam() == TEAM_SPECTATORS || aVoteChecked[i])	// don't count in votes by spectators
						continue;

					int ActVote = m_apPlayers[i]->m_Vote;
					int ActVotePos = m_apPlayers[i]->m_VotePos;

					// check for more players with the same ip (only use the vote of the one who voted first)
					for(int j = i+1; j < MAX_CLIENTS; ++j)
					{
						if(!m_apPlayers[j] || aVoteChecked[j] || str_comp(aaBuf[j], aaBuf[i]))
							continue;

						aVoteChecked[j] = true;
						if(m_apPlayers[j]->m_Vote && (!ActVote || ActVotePos > m_apPlayers[j]->m_VotePos))
						{
							ActVote = m_apPlayers[j]->m_Vote;
							ActVotePos = m_apPlayers[j]->m_VotePos;
						}
					}

					Total++;
					if(ActVote > 0)
						Yes++;
					else if(ActVote < 0)
						No++;
				}

				if(Yes >= Total/2+1)
					m_VoteEnforce = VOTE_ENFORCE_YES;
				else if(No >= (Total+1)/2)
					m_VoteEnforce = VOTE_ENFORCE_NO;
			}

			if(m_VoteEnforce == VOTE_ENFORCE_YES)
			{
				Server()->SetRconCID(IServer::RCON_CID_VOTE);
				Console()->ExecuteLine(m_aVoteCommand, -1);
				Server()->SetRconCID(IServer::RCON_CID_SERV);
				EndVote();
				SendChat(-1, CGameContext::CHAT_ALL, "Vote passed");

				if(m_apPlayers[m_VoteCreator])
					m_apPlayers[m_VoteCreator]->m_LastVoteCall = 0;
			}
			else if(m_VoteEnforce == VOTE_ENFORCE_NO || time_get() > m_VoteCloseTime)
			{
				EndVote();
				SendChat(-1, CGameContext::CHAT_ALL, "Vote failed");
			}
			else if(m_VoteUpdate)
			{
				m_VoteUpdate = false;
				SendVoteStatus(-1, Total, Yes, No);
			}
		}
	}

	Count();
	if(Server()->m_LocateGame == LOCATE_GAME)
		HandleMonsterSpawn();
}

// Server hooks
void CGameContext::OnClientDirectInput(int ClientID, void *pInput)
{
	if(!m_World.m_Paused)
		m_apPlayers[ClientID]->OnDirectInput((CNetObj_PlayerInput *)pInput);
}

void CGameContext::OnClientPredictedInput(int ClientID, void *pInput)
{
	if(!m_World.m_Paused)
		m_apPlayers[ClientID]->OnPredictedInput((CNetObj_PlayerInput *)pInput);
}

void CGameContext::OnClientEnter(int ClientID)
{
	if(ShouldPersistPlayer(ClientID) && m_aPlayerPersist[ClientID].m_Active)
	{
		RestorePersistedPlayer(ClientID);
	}
	else if(Server()->m_LocateGame == LOCATE_GAME && m_pController && m_pController->m_pShip)
	{
		vec2 SpawnPos = m_pController->m_pShip->m_Pos;
		m_pController->GetSafeSpawnNear(SpawnPos, &SpawnPos, 480.0f);
		Server()->GetClientSession(ClientID)->m_X = SpawnPos.x;
		Server()->GetClientSession(ClientID)->m_Y = SpawnPos.y;
		Server()->GetClientSession(ClientID)->m_RoundId = m_pController->m_RoundId;
	}

	m_apPlayers[ClientID]->Respawn();

	if(Server()->m_LocateGame == LOCATE_GAME && HasPendingExpeditionBonus(ClientID))
		ApplyExpeditionBonuses(ClientID);

	char aBuf[512];

	SendChatTarget(-1, _("{str:name} 入职了公司"), "name", Server()->ClientName(ClientID));
	str_format(aBuf, sizeof(aBuf), "team_join player='%d:%s' team=%d", ClientID, Server()->ClientName(ClientID), m_apPlayers[ClientID]->GetTeam());
	Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);

	SendChatTarget(ClientID, _("欢迎来到Tee命公司！"));
	SendChatTarget(ClientID, _("按 F3 打开公司终端；ESC 发起投票"));
	SendChatTarget(ClientID, _("输入 /help 1 查看分步教程 | /status 查看任务"));
	if(!m_apPlayers[ClientID]->m_LcOnboarded)
	{
		m_apPlayers[ClientID]->m_LcOnboarded = true;
		if(Server()->m_LocateGame == LOCATE_LOBBY)
		{
			SendChatTarget(ClientID, _("首次加入：按 F3 打开公司终端引导"));
			m_apPlayers[ClientID]->m_TerminalMenuPage = LC_PAGE_WELCOME;
			m_apPlayers[ClientID]->m_TerminalWelcomePending = true;
		}
	}

	if(Server()->m_LocateGame == LOCATE_LOBBY)
		m_apPlayers[ClientID]->m_LcExpeditionParticipant = false;
	else if(Server()->m_LocateGame == LOCATE_GAME && m_pController &&
		m_pController->ExpeditionPhase() == LC_PHASE_EXPEDITION &&
		m_pController->m_PrepareTick < 0 &&
		!m_apPlayers[ClientID]->m_LcExpeditionParticipant)
	{
		LcSendMidJoinBriefing(this, ClientID);
		m_apPlayers[ClientID]->m_LcExpeditionParticipant = true;
	}

	m_VoteUpdate = true;
}

void CGameContext::OnClientConnected(int ClientID)
{
	// Check which team the player should be on
	const int StartTeam = g_Config.m_SvTournamentMode ? TEAM_SPECTATORS : m_pController->GetAutoTeam(ClientID);

	if(m_apPlayers[ClientID])
		return;

	m_apPlayers[ClientID] = new(ClientID) CPlayer(this, ClientID, StartTeam);

	// send active vote
	if(m_VoteCloseTime)
		SendVoteSet(ClientID);

	// send motd
	if(m_apPlayers[ClientID]->m_TerminalMenuOpen)
		m_TerminalMenu.SendMotd(this, ClientID, m_apPlayers[ClientID]->m_TerminalMenuSelection);
	else
	{
		CNetMsg_Sv_Motd Msg;
		Msg.m_pMessage = g_Config.m_SvMotd;
		Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, ClientID);
	}

	m_aBroadcastStates[ClientID].m_NoChangeTick = 0;
	m_aBroadcastStates[ClientID].m_LifeSpanTick = 0;
	m_aBroadcastStates[ClientID].m_Priority = BROADCAST_PRIORITY_LOWEST;
	m_aBroadcastStates[ClientID].m_aPrevMessage[0] = 0;
	m_aBroadcastStates[ClientID].m_aNextMessage[0] = 0;
}

void CGameContext::OnClientDrop(int ClientID, const char *pReason)
{
	if(m_apPlayers[ClientID]->m_VoteStarted)
		m_VoteStart--;
	
	if(GetPlayerChar(ClientID))
		if(GetPlayerChar(ClientID)->m_LeekTick > 1)
			Server()->GetClientSession(ClientID)->m_Freeze = true; // AntiCheat.

	AbortVoteKickOnDisconnect(ClientID);
	m_apPlayers[ClientID]->OnDisconnect(pReason);
	delete m_apPlayers[ClientID];
	m_apPlayers[ClientID] = 0;

	m_VoteUpdate = true;

	// update spectator modes
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		if(m_apPlayers[i] && m_apPlayers[i]->m_SpectatorID == ClientID)
			m_apPlayers[i]->m_SpectatorID = SPEC_FREEVIEW;
	}
}

void CGameContext::OnMessage(int MsgID, CUnpacker *pUnpacker, int ClientID)
{
	void *pRawMsg = m_NetObjHandler.SecureUnpackMsg(MsgID, pUnpacker);
	CPlayer *pPlayer = m_apPlayers[ClientID];

	if(!pRawMsg)
	{
		if(g_Config.m_Debug)
		{
			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "dropped weird message '%s' (%d), failed on '%s'", m_NetObjHandler.GetMsgName(MsgID), MsgID, m_NetObjHandler.FailedMsgOn());
			Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "server", aBuf);
		}
		return;
	}

	if(Server()->ClientIngame(ClientID))
	{
		if(MsgID == NETMSGTYPE_CL_SAY)
		{
			if(g_Config.m_SvSpamprotection && pPlayer->m_LastChat && pPlayer->m_LastChat+Server()->TickSpeed() > Server()->Tick())
				return;

			CNetMsg_Cl_Say *pMsg = (CNetMsg_Cl_Say *)pRawMsg;
			if(!str_utf8_check(pMsg->m_pMessage))
			{
				return;
			}
			int Team = pMsg->m_Team ? pPlayer->GetTeam() : CGameContext::CHAT_ALL;
			
			// trim right and set maximum length to 128 utf8-characters
			int Length = 0;
			const char *p = pMsg->m_pMessage;
			const char *pEnd = 0;
			while(*p)
 			{
				const char *pStrOld = p;
				int Code = str_utf8_decode(&p);

				// check if unicode is not empty
				if(Code > 0x20 && Code != 0xA0 && Code != 0x034F && (Code < 0x2000 || Code > 0x200F) && (Code < 0x2028 || Code > 0x202F) &&
					(Code < 0x205F || Code > 0x2064) && (Code < 0x206A || Code > 0x206F) && (Code < 0xFE00 || Code > 0xFE0F) &&
					Code != 0xFEFF && (Code < 0xFFF9 || Code > 0xFFFC))
				{
					pEnd = 0;
				}
				else if(pEnd == 0)
					pEnd = pStrOld;

				if(++Length >= 127)
				{
					*(const_cast<char *>(p)) = 0;
					break;
				}
 			}
			if(pEnd != 0)
				*(const_cast<char *>(pEnd)) = 0;

			// drop empty and autocreated spam messages (more than 16 characters per second)
			if(Length == 0 || (pMsg->m_pMessage[0] != '/' && g_Config.m_SvSpamprotection && pPlayer->m_LastChat && pPlayer->m_LastChat+Server()->TickSpeed()*((15+Length)/16) > Server()->Tick()))
				return;

			pPlayer->m_LastChat = Server()->Tick();
			
			if(pMsg->m_pMessage[0] == '/' || pMsg->m_pMessage[0] == '\\')
			{
				switch(m_apPlayers[ClientID]->m_Authed)
				{
					case IServer::AUTHED_ADMIN:
						Console()->SetAccessLevel(IConsole::ACCESS_LEVEL_ADMIN);
						break;
					case IServer::AUTHED_MOD:
						Console()->SetAccessLevel(IConsole::ACCESS_LEVEL_MOD);
						break;
					default:
						Console()->SetAccessLevel(IConsole::ACCESS_LEVEL_USER);
				}	

				Console()->ExecuteLineFlag(pMsg->m_pMessage + 1, ClientID, CFGFLAG_CHAT);
				
				Console()->SetAccessLevel(IConsole::ACCESS_LEVEL_ADMIN);
			}
			else
			{
				SendChat(ClientID, Team, pMsg->m_pMessage);
			}
		}
		else if(MsgID == NETMSGTYPE_CL_CALLVOTE)
		{
			CNetMsg_Cl_CallVote const *pMsg = (CNetMsg_Cl_CallVote *)pRawMsg;
			const char *pReason = pMsg->m_Reason[0] ? pMsg->m_Reason : "No reason given";

			if (str_comp_nocase(pMsg->m_Type, "kick") == 0)
			{
				int KickID = str_toint(pMsg->m_Value);
				if (KickID < 0 || KickID >= MAX_PLAYER || !m_apPlayers[KickID])
				{
					SendChatTarget(ClientID, _("无效的被踢玩家编号"));
					return;
				}
				if (KickID == ClientID)
				{
					SendChatTarget(ClientID, _("你不能把你自己踢出房间"));
					return;
				}
				if (Server()->IsAuthed(KickID))
				{
					SendChatTarget(ClientID, _("你不能把管理员踢出房间"));
					char aBufKick[128];
					str_format(aBufKick, sizeof(aBufKick), "'%s' 投票把你踢出房间", Server()->ClientName(ClientID));
					SendChatTarget(KickID, aBufKick);
					return;
				}
			}
			else
			{
				char aDesc[VOTE_DESC_LENGTH] = {0};
				char aCmd[VOTE_CMD_LENGTH] = {0};

				if (str_comp_nocase(pMsg->m_Type, "option") == 0)
				{
					for (int i = 0; i < m_aPlayerVotes[ClientID].size(); ++i)
					{
						if (str_comp_nocase(pMsg->m_Value, m_aPlayerVotes[ClientID][i].m_aDescription) == 0)
						{
							str_format(aDesc, sizeof(aDesc), "%s", m_aPlayerVotes[ClientID][i].m_aDescription);
							str_format(aCmd, sizeof(aCmd), "%s", m_aPlayerVotes[ClientID][i].m_aCommand);
						}
					}
				}
				else if(str_comp_nocase(pMsg->m_Type, "kick") == 0)
				{
					if(!g_Config.m_SvVoteKick)
					{
						SendChatTarget(ClientID, _("本服务器不允许投票踢人"));
						return;
					}

					int KickID = str_toint(pMsg->m_Value);
					if(KickID < 0 || KickID >= MAX_CLIENTS || !m_apPlayers[KickID])
					{
						SendChatTarget(ClientID, _("无效的被踢玩家编号"));
						return;
					}
					if(KickID == ClientID)
					{
						SendChatTarget(ClientID, _("你不能踢自己"));
						return;
					}
					if(Server()->IsAuthed(KickID))
					{
						SendChatTarget(ClientID, _("你不能踢管理员"));
						char aBufKick[128];
						str_format(aBufKick, sizeof(aBufKick), "'%s' called for vote to kick you", Server()->ClientName(ClientID));
						SendChatTarget(KickID, aBufKick);
						return;
					}

					str_format(aDesc, sizeof(aDesc), "Kick '%s'", Server()->ClientName(KickID));
					if (!g_Config.m_SvVoteKickBantime)
						str_format(aCmd, sizeof(aCmd), "kick %d Kicked by vote", KickID);
					else
					{
						char aAddrStr[NETADDR_MAXSTRSIZE] = {0};
						Server()->GetClientAddr(KickID, aAddrStr, sizeof(aAddrStr));
						str_format(aCmd, sizeof(aCmd), "ban %s %d Banned by vote", aAddrStr, g_Config.m_SvVoteKickBantime);
					}
				}
				else if(str_comp_nocase(pMsg->m_Type, "spectate") == 0)
				{
					if(!g_Config.m_SvVoteSpectate)
					{
						SendChatTarget(ClientID, _("本服务器不允许投票将玩家移至观战"));
						return;
					}

					int SpectateID = str_toint(pMsg->m_Value);
					if(SpectateID < 0 || SpectateID >= MAX_CLIENTS || !m_apPlayers[SpectateID] || m_apPlayers[SpectateID]->GetTeam() == TEAM_SPECTATORS)
					{
						SendChatTarget(ClientID, _("无效的玩家编号"));
						return;
					}
					if(SpectateID == ClientID)
					{
						SendChatTarget(ClientID, _("你不能移动自己"));
						return;
					}

					str_format(aDesc, sizeof(aDesc), "move '%s' to spectators", Server()->ClientName(SpectateID));
					str_format(aCmd, sizeof(aCmd), "set_team %d -1 %d", SpectateID, g_Config.m_SvVoteSpectateRejoindelay);
				}
				
				if(aCmd[0])
					ExecutePlayerVoteCommand(ClientID, aCmd, pReason);

				ResetVotes(ClientID);
				
			}
		}
		else if(MsgID == NETMSGTYPE_CL_VOTE)
		{
			if(!m_VoteCloseTime)
			{
				CNetMsg_Cl_Vote *pMsg = (CNetMsg_Cl_Vote *)pRawMsg;
				if(pMsg->m_Vote == 1)
				{
					if(pPlayer->m_LastMenuVoteKey != 1)
					{
						ToggleTerminalMenu(ClientID);
						pPlayer->m_LastMenuVoteKey = 1;
					}
				}
				else
					pPlayer->m_LastMenuVoteKey = pMsg->m_Vote;
				return;
			}

			if(pPlayer->m_Vote == 0)
			{
				CNetMsg_Cl_Vote *pMsg = (CNetMsg_Cl_Vote *)pRawMsg;
				if(!pMsg->m_Vote)
					return;

				pPlayer->m_Vote = pMsg->m_Vote;
				pPlayer->m_VotePos = ++m_VotePos;
				m_VoteUpdate = true;
			}
		}
		else if (MsgID == NETMSGTYPE_CL_SETTEAM && !m_World.m_Paused)
		{
		}
		else if (MsgID == NETMSGTYPE_CL_SETSPECTATORMODE && !m_World.m_Paused)
		{
			CNetMsg_Cl_SetSpectatorMode *pMsg = (CNetMsg_Cl_SetSpectatorMode *)pRawMsg;

			if(pPlayer->GetTeam() != TEAM_SPECTATORS || pPlayer->m_SpectatorID == pMsg->m_SpectatorID || ClientID == pMsg->m_SpectatorID ||
				(g_Config.m_SvSpamprotection && pPlayer->m_LastSetSpectatorMode && pPlayer->m_LastSetSpectatorMode+Server()->TickSpeed()*3 > Server()->Tick()))
				return;

			pPlayer->m_LastSetSpectatorMode = Server()->Tick();
			if(pMsg->m_SpectatorID != SPEC_FREEVIEW && (!m_apPlayers[pMsg->m_SpectatorID] || m_apPlayers[pMsg->m_SpectatorID]->GetTeam() == TEAM_SPECTATORS))
				SendChatTarget(ClientID, _("无效的观战编号"));
			else
				pPlayer->m_SpectatorID = pMsg->m_SpectatorID;
		}
		else if (MsgID == NETMSGTYPE_CL_CHANGEINFO)
		{
			if(g_Config.m_SvSpamprotection && pPlayer->m_LastChangeInfo && pPlayer->m_LastChangeInfo+Server()->TickSpeed()*5 > Server()->Tick())
				return;

			CNetMsg_Cl_ChangeInfo *pMsg = (CNetMsg_Cl_ChangeInfo *)pRawMsg;
			pPlayer->m_LastChangeInfo = Server()->Tick();

			// set infos
			char aOldName[MAX_NAME_LENGTH];
			str_copy(aOldName, Server()->ClientName(ClientID), sizeof(aOldName));
			Server()->SetClientName(ClientID, pMsg->m_pName);
			if(str_comp(aOldName, Server()->ClientName(ClientID)) != 0)
			{
				char aChatText[256];
				str_format(aChatText, sizeof(aChatText), "'%s' changed name to '%s'", aOldName, Server()->ClientName(ClientID));
				SendChat(-1, CGameContext::CHAT_ALL, aChatText);
			}
			Server()->SetClientClan(ClientID, pMsg->m_pClan);
			Server()->SetClientCountry(ClientID, pMsg->m_Country);
			str_copy(pPlayer->m_TeeInfos.m_SkinName, pMsg->m_pSkin, sizeof(pPlayer->m_TeeInfos.m_SkinName));
			pPlayer->m_TeeInfos.m_UseCustomColor = pMsg->m_UseCustomColor;
			pPlayer->m_TeeInfos.m_ColorBody = pMsg->m_ColorBody;
			pPlayer->m_TeeInfos.m_ColorFeet = pMsg->m_ColorFeet;
			m_pController->OnPlayerInfoChange(pPlayer);
		}
		else if (MsgID == NETMSGTYPE_CL_EMOTICON && !m_World.m_Paused)
		{
			CNetMsg_Cl_Emoticon *pMsg = (CNetMsg_Cl_Emoticon *)pRawMsg;

			if(g_Config.m_SvSpamprotection && pPlayer->m_LastEmote && pPlayer->m_LastEmote + 25 > Server()->Tick())
				return;

			pPlayer->m_LastEmote = Server()->Tick();

			SendEmoticon(ClientID, pMsg->m_Emoticon);

			if(pMsg->m_Emoticon == EMOTICON_DOTDOT && Server()->m_LocateGame == LOCATE_GAME)
			{
				CCharacter *pChr = GetPlayerChar(ClientID);
				if(pChr && !pChr->m_Freeze)
					ScanFacility(ClientID);
			}
			else if(pMsg->m_Emoticon == EMOTICON_HEARTS)
				VehicleHandleHeartsDismount(this, ClientID);
		}
		else if (MsgID == NETMSGTYPE_CL_KILL && !m_World.m_Paused)
		{
			if(pPlayer->m_LastKill && pPlayer->m_LastKill+Server()->TickSpeed()*3 > Server()->Tick())
				return;

			pPlayer->m_LastKill = Server()->Tick();
			SendBroadcast(ClientID, BROADCAST_PRIORITY_INTERFACE, BROADCAST_DURATION_REALTIME, _("这个服务器禁止了自杀."));
		}

		ResetVotes(ClientID);
	}
	else
	{
		if(MsgID == NETMSGTYPE_CL_STARTINFO)
		{
			if(pPlayer->m_IsReady)
				return;

			CNetMsg_Cl_StartInfo *pMsg = (CNetMsg_Cl_StartInfo *)pRawMsg;
			pPlayer->m_LastChangeInfo = Server()->Tick();

			// set start infos
			Server()->SetClientName(ClientID, pMsg->m_pName);
			Server()->SetClientClan(ClientID, pMsg->m_pClan);
			Server()->SetClientCountry(ClientID, pMsg->m_Country);
			str_copy(pPlayer->m_TeeInfos.m_SkinName, pMsg->m_pSkin, sizeof(pPlayer->m_TeeInfos.m_SkinName));
			pPlayer->m_TeeInfos.m_UseCustomColor = pMsg->m_UseCustomColor;
			pPlayer->m_TeeInfos.m_ColorBody = pMsg->m_ColorBody;
			pPlayer->m_TeeInfos.m_ColorFeet = pMsg->m_ColorFeet;
			m_pController->OnPlayerInfoChange(pPlayer);

			ResetVotes(ClientID);

			// send tuning parameters to client
			SendTuningParams(ClientID);

			// client is ready to enter
			pPlayer->m_IsReady = true;
			CNetMsg_Sv_ReadyToEnter m;
			Server()->SendPackMsg(&m, MSGFLAG_VITAL|MSGFLAG_FLUSH, ClientID);
		}
	}
}

void CGameContext::ConTuneParam(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	const char *pParamName = pResult->GetString(0);
	float NewValue = pResult->GetFloat(1);

	if(pSelf->Tuning()->Set(pParamName, NewValue))
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "%s changed to %.2f", pParamName, NewValue);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "tuning", aBuf);
		pSelf->SendTuningParams(-1);
	}
	else
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "tuning", "No such tuning parameter");
}

void CGameContext::ConTuneReset(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	CTuningParams TuningParams;
	*pSelf->Tuning() = TuningParams;
	pSelf->SendTuningParams(-1);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "tuning", "Tuning reset");
}

void CGameContext::ConTuneDump(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	char aBuf[256];
	for(int i = 0; i < pSelf->Tuning()->Num(); i++)
	{
		float v;
		pSelf->Tuning()->Get(i, &v);
		str_format(aBuf, sizeof(aBuf), "%s %.2f", pSelf->Tuning()->m_apNames[i], v);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "tuning", aBuf);
	}
}

void CGameContext::ConPause(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->m_pController->TogglePause();
}

void CGameContext::ConChangeMap(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->m_pController->ChangeMap(pResult->NumArguments() ? pResult->GetString(0) : "");
}

void CGameContext::ConRestart(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(pResult->NumArguments())
		pSelf->m_pController->DoWarmup(pResult->GetInteger(0));
	else
		pSelf->m_pController->StartRound();
}

void CGameContext::ConBroadcast(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->SendBroadcast(-1, BROADCAST_PRIORITY_GAMEANNOUNCE, BROADCAST_DURATION_REALTIME, pResult->GetString(0));
}

void CGameContext::ConSay(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->SendChat(-1, CGameContext::CHAT_ALL, pResult->GetString(0));
}

void CGameContext::ConSetTeam(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int ClientID = clamp(pResult->GetInteger(0), 0, (int)MAX_CLIENTS-1);
	int Team = clamp(pResult->GetInteger(1), -1, 1);
	int Delay = pResult->NumArguments()>2 ? pResult->GetInteger(2) : 0;
	if(!pSelf->m_apPlayers[ClientID])
		return;

	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "moved client %d to team %d", ClientID, Team);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);

	pSelf->m_apPlayers[ClientID]->m_TeamChangeTick = pSelf->Server()->Tick()+pSelf->Server()->TickSpeed()*Delay*60;
	pSelf->m_apPlayers[ClientID]->SetTeam(Team);
}

void CGameContext::ConSetTeamAll(IConsole::IResult *pResult, void *pUserData)
{
}

void CGameContext::ConSwapTeams(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->SwapTeams();
}

void CGameContext::ConShuffleTeams(IConsole::IResult *pResult, void *pUserData)
{
}

void CGameContext::ConLockTeams(IConsole::IResult *pResult, void *pUserData)
{
}

void CGameContext::ConAddVote(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	const char *pDescription = pResult->GetString(0);
	const char *pCommand = pResult->GetString(1);

	if(pSelf->m_NumVoteOptions == MAX_VOTE_OPTIONS)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "maximum number of vote options reached");
		return;
	}

	// check for valid option
	if(!pSelf->Console()->LineIsValid(pCommand) || str_length(pCommand) >= VOTE_CMD_LENGTH)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "skipped invalid command '%s'", pCommand);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
		return;
	}
	while(*pDescription && *pDescription == ' ')
		pDescription++;
	if(str_length(pDescription) >= VOTE_DESC_LENGTH || *pDescription == 0)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "skipped invalid option '%s'", pDescription);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
		return;
	}

	// check for duplicate entry
	CVoteOptionServer *pOption = pSelf->m_pVoteOptionFirst;
	while(pOption)
	{
		if(str_comp_nocase(pDescription, pOption->m_aDescription) == 0)
		{
			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "option '%s' already exists", pDescription);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
			return;
		}
		pOption = pOption->m_pNext;
	}

	// add the option
	++pSelf->m_NumVoteOptions;
	int Len = str_length(pCommand);

	pOption = (CVoteOptionServer *)pSelf->m_pVoteOptionHeap->Allocate(sizeof(CVoteOptionServer) + Len);
	pOption->m_pNext = 0;
	pOption->m_pPrev = pSelf->m_pVoteOptionLast;
	if(pOption->m_pPrev)
		pOption->m_pPrev->m_pNext = pOption;
	pSelf->m_pVoteOptionLast = pOption;
	if(!pSelf->m_pVoteOptionFirst)
		pSelf->m_pVoteOptionFirst = pOption;

	str_copy(pOption->m_aDescription, pDescription, sizeof(pOption->m_aDescription));
	mem_copy(pOption->m_aCommand, pCommand, Len+1);
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "added option '%s' '%s'", pOption->m_aDescription, pOption->m_aCommand);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);

	// inform clients about added option
	CNetMsg_Sv_VoteOptionAdd OptionMsg;
	OptionMsg.m_pDescription = pOption->m_aDescription;
	pSelf->Server()->SendPackMsg(&OptionMsg, MSGFLAG_VITAL, -1);
}

void CGameContext::ConRemoveVote(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	const char *pDescription = pResult->GetString(0);

	// check for valid option
	CVoteOptionServer *pOption = pSelf->m_pVoteOptionFirst;
	while(pOption)
	{
		if(str_comp_nocase(pDescription, pOption->m_aDescription) == 0)
			break;
		pOption = pOption->m_pNext;
	}
	if(!pOption)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "option '%s' does not exist", pDescription);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
		return;
	}

	// inform clients about removed option
	CNetMsg_Sv_VoteOptionRemove OptionMsg;
	OptionMsg.m_pDescription = pOption->m_aDescription;
	pSelf->Server()->SendPackMsg(&OptionMsg, MSGFLAG_VITAL, -1);

	// TODO: improve this
	// remove the option
	--pSelf->m_NumVoteOptions;
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "removed option '%s' '%s'", pOption->m_aDescription, pOption->m_aCommand);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);

	CHeap *pVoteOptionHeap = new CHeap();
	CVoteOptionServer *pVoteOptionFirst = 0;
	CVoteOptionServer *pVoteOptionLast = 0;
	int NumVoteOptions = pSelf->m_NumVoteOptions;
	for(CVoteOptionServer *pSrc = pSelf->m_pVoteOptionFirst; pSrc; pSrc = pSrc->m_pNext)
	{
		if(pSrc == pOption)
			continue;

		// copy option
		int Len = str_length(pSrc->m_aCommand);
		CVoteOptionServer *pDst = (CVoteOptionServer *)pVoteOptionHeap->Allocate(sizeof(CVoteOptionServer) + Len);
		pDst->m_pNext = 0;
		pDst->m_pPrev = pVoteOptionLast;
		if(pDst->m_pPrev)
			pDst->m_pPrev->m_pNext = pDst;
		pVoteOptionLast = pDst;
		if(!pVoteOptionFirst)
			pVoteOptionFirst = pDst;

		str_copy(pDst->m_aDescription, pSrc->m_aDescription, sizeof(pDst->m_aDescription));
		mem_copy(pDst->m_aCommand, pSrc->m_aCommand, Len+1);
	}

	// clean up
	delete pSelf->m_pVoteOptionHeap;
	pSelf->m_pVoteOptionHeap = pVoteOptionHeap;
	pSelf->m_pVoteOptionFirst = pVoteOptionFirst;
	pSelf->m_pVoteOptionLast = pVoteOptionLast;
	pSelf->m_NumVoteOptions = NumVoteOptions;
}

void CGameContext::ConForceVote(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	const char *pType = pResult->GetString(0);
	const char *pValue = pResult->GetString(1);
	const char *pReason = pResult->NumArguments() > 2 && pResult->GetString(2)[0] ? pResult->GetString(2) : "No reason given";
	char aBuf[128] = {0};

	if(str_comp_nocase(pType, "option") == 0)
	{
		CVoteOptionServer *pOption = pSelf->m_pVoteOptionFirst;
		while(pOption)
		{
			if(str_comp_nocase(pValue, pOption->m_aDescription) == 0)
			{
				str_format(aBuf, sizeof(aBuf), "admin forced server option '%s' (%s)", pValue, pReason);
				pSelf->SendChatTarget(-1, aBuf);
				pSelf->Console()->ExecuteLine(pOption->m_aCommand, -1);
				break;
			}

			pOption = pOption->m_pNext;
		}

		if(!pOption)
		{
			str_format(aBuf, sizeof(aBuf), "'%s' isn't an option on this server", pValue);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
			return;
		}
	}
	else if(str_comp_nocase(pType, "kick") == 0)
	{
		int KickID = str_toint(pValue);
		if(KickID < 0 || KickID >= MAX_CLIENTS || !pSelf->m_apPlayers[KickID])
		{
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "Invalid client id to kick");
			return;
		}

		if (!g_Config.m_SvVoteKickBantime)
		{
			str_format(aBuf, sizeof(aBuf), "kick %d %s", KickID, pReason);
			pSelf->Console()->ExecuteLine(aBuf, -1);
		}
		else
		{
			char aAddrStr[NETADDR_MAXSTRSIZE] = {0};
			pSelf->Server()->GetClientAddr(KickID, aAddrStr, sizeof(aAddrStr));
			str_format(aBuf, sizeof(aBuf), "ban %s %d %s", aAddrStr, g_Config.m_SvVoteKickBantime, pReason);
			pSelf->Console()->ExecuteLine(aBuf, -1);
		}
	}
	else if(str_comp_nocase(pType, "spectate") == 0)
	{
		int SpectateID = str_toint(pValue);
		if(SpectateID < 0 || SpectateID >= MAX_CLIENTS || !pSelf->m_apPlayers[SpectateID] || pSelf->m_apPlayers[SpectateID]->GetTeam() == TEAM_SPECTATORS)
		{
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "Invalid client id to move");
			return;
		}

		str_format(aBuf, sizeof(aBuf), "admin moved '%s' to spectator (%s)", pSelf->Server()->ClientName(SpectateID), pReason);
		pSelf->SendChatTarget(-1, aBuf);
		str_format(aBuf, sizeof(aBuf), "set_team %d -1 %d", SpectateID, g_Config.m_SvVoteSpectateRejoindelay);
		pSelf->Console()->ExecuteLine(aBuf, -1);
	}
}

void CGameContext::ConClearVotes(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "cleared votes");
	CNetMsg_Sv_VoteClearOptions VoteClearOptionsMsg;
	pSelf->Server()->SendPackMsg(&VoteClearOptionsMsg, MSGFLAG_VITAL, -1);
	pSelf->m_pVoteOptionHeap->Reset();
	pSelf->m_pVoteOptionFirst = 0;
	pSelf->m_pVoteOptionLast = 0;
	pSelf->m_NumVoteOptions = 0;
}

void CGameContext::ConVote(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	// check if there is a vote running
	if(!pSelf->m_VoteCloseTime)
		return;

	if(str_comp_nocase(pResult->GetString(0), "yes") == 0)
		pSelf->m_VoteEnforce = CGameContext::VOTE_ENFORCE_YES;
	else if(str_comp_nocase(pResult->GetString(0), "no") == 0)
		pSelf->m_VoteEnforce = CGameContext::VOTE_ENFORCE_NO;
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "admin forced vote %s", pResult->GetString(0));
	pSelf->SendChatTarget(-1, aBuf);
	str_format(aBuf, sizeof(aBuf), "forcing vote %s", pResult->GetString(0));
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
}

void CGameContext::ConchainSpecialMotdupdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	if(pResult->NumArguments())
	{
		CGameContext *pSelf = (CGameContext *)pUserData;
		for(int i = 0; i < MAX_CLIENTS; ++i)
		{
			if(!pSelf->m_apPlayers[i])
				continue;
			if(pSelf->m_apPlayers[i]->m_TerminalMenuOpen)
				pSelf->m_TerminalMenu.SendMotd(pSelf, i, pSelf->m_apPlayers[i]->m_TerminalMenuSelection);
			else
			{
				CNetMsg_Sv_Motd Msg;
				Msg.m_pMessage = g_Config.m_SvMotd;
				pSelf->Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, i);
			}
		}
	}
}

void CGameContext::ConAbout(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext* pThis = (CGameContext*) pUserData;
	
	pThis->SendChatTarget(pResult->GetClientID(), _("{str:name} {str:version}，作者 {str:authors}"), "name", MOD_NAME, "version", MOD_VERSION, "authors", MOD_AUTHORS);
	pThis->SendChatTarget(pResult->GetClientID(), _("F3 终端 → 制作人员 可查看完整名单"));
	if(MOD_QQ_GROUP[0])
		pThis->SendChatTarget(pResult->GetClientID(), _("【官方 QQ 交流群】{str:qq}"), "qq", MOD_QQ_GROUP);
	
	if(MOD_CREDITS[0])
		pThis->SendChatTarget(pResult->GetClientID(), _("制作人员：{str:c}"), "c", MOD_CREDITS);
	if(MOD_THANKS[0])
		pThis->SendChatTarget(pResult->GetClientID(), _("鸣谢：{str:c}"), "c", MOD_THANKS);
	if(MOD_SOURCES[0])
		pThis->SendChatTarget(pResult->GetClientID(), _("源码：{str:c}"), "c", MOD_SOURCES);
}

void CGameContext::ConLanguage(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	int ClientID = pResult->GetClientID();

	const char *pLanguageCode = (pResult->NumArguments()>0) ? pResult->GetString(0) : 0x0;
	char aFinalLanguageCode[8];
	aFinalLanguageCode[0] = 0;

	if(pLanguageCode)
	{
		if(str_comp_nocase(pLanguageCode, "ua") == 0)
			str_copy(aFinalLanguageCode, "uk", sizeof(aFinalLanguageCode));
		else
		{
			for(int i=0; i<pSelf->Server()->Localization()->m_pLanguages.size(); i++)
			{
				if(str_comp_nocase(pLanguageCode, pSelf->Server()->Localization()->m_pLanguages[i]->GetFilename()) == 0)
					str_copy(aFinalLanguageCode, pLanguageCode, sizeof(aFinalLanguageCode));
			}
		}
	}
	
	if(aFinalLanguageCode[0])
	{
		pSelf->SetClientLanguage(ClientID, aFinalLanguageCode);
		const char *pLangKey = str_comp(aFinalLanguageCode, "en") == 0 ? _("英语") : _("中文");
		pSelf->SendChatTarget(ClientID, _("语言已切换为: {lstr:lang}"), "lang", pLangKey);
	}
	else
	{
		const char* pLanguage = pSelf->m_apPlayers[ClientID]->GetLanguage();
		const char* pTxtUnknownLanguage = pSelf->Server()->Localization()->Localize(pLanguage, _("未知语言"));
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "language", pTxtUnknownLanguage);	
		
		dynamic_string BufferList;
		int BufferIter = 0;
		for(int i=0; i<pSelf->Server()->Localization()->m_pLanguages.size(); i++)
		{
			if(i>0)
				BufferIter = BufferList.append_at(BufferIter, ", ");
			BufferIter = BufferList.append_at(BufferIter, pSelf->Server()->Localization()->m_pLanguages[i]->GetFilename());
		}
		
		dynamic_string Buffer;
		pSelf->Server()->Localization()->Format_L(Buffer, pLanguage, _("可用语言：{str:ListOfLanguage}"), "ListOfLanguage", BufferList.buffer(), NULL);
		
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "language", Buffer.buffer());

        pSelf->SendChatTarget(ClientID, Buffer.buffer());
    }
	
	return;
}

void CGameContext::SetClientLanguage(int ClientID, const char *pLanguage)
{
	Server()->SetClientLanguage(ClientID, pLanguage);
	if(m_apPlayers[ClientID])
	{
		m_apPlayers[ClientID]->SetLanguage(pLanguage);
	}
}

void CGameContext::ConsoleOutputCallback_Chat(const char *pStr, void *pUser)
{
	CGameContext* pThis = (CGameContext*) pUser;
	if(pThis->m_ConsoleOutput_Target >= 0 && pThis->m_ConsoleOutput_Target < MAX_CLIENTS)
		pThis->SendChatTarget(pThis->m_ConsoleOutput_Target, pStr);
}

void CGameContext::OnConsoleInit()
{
	m_pServer = Kernel()->RequestInterface<IServer>();
	m_pConsole = Kernel()->RequestInterface<IConsole>();

	m_ConsoleOutputHandle_ChatPrint = Console()->RegisterPrintCallback(3, ConsoleOutputCallback_Chat, this);
	Console()->SetPrintOutputLevel_Hard(m_ConsoleOutputHandle_ChatPrint, IConsole::OUTPUT_LEVEL_CHAT);
	
	Console()->Register("tune", "si", CFGFLAG_SERVER, ConTuneParam, this, "Tune variable to value");
	Console()->Register("tune_reset", "", CFGFLAG_SERVER, ConTuneReset, this, "Reset tuning");
	Console()->Register("tune_dump", "", CFGFLAG_SERVER, ConTuneDump, this, "Dump tuning");

	Console()->Register("pause", "", CFGFLAG_SERVER, ConPause, this, "Pause/unpause game");
	Console()->Register("change_map", "?r", CFGFLAG_SERVER|CFGFLAG_STORE, ConChangeMap, this, "Change map");
	Console()->Register("restart", "?i", CFGFLAG_SERVER|CFGFLAG_STORE, ConRestart, this, "Restart in x seconds (0 = abort)");
	Console()->Register("broadcast", "r", CFGFLAG_SERVER, ConBroadcast, this, "Broadcast message");
	Console()->Register("say", "r", CFGFLAG_SERVER, ConSay, this, "Say in chat");
	Console()->Register("set_team", "ii?i", CFGFLAG_SERVER, ConSetTeam, this, "Set team of player to team");
	Console()->Register("set_team_all", "i", CFGFLAG_SERVER, ConSetTeamAll, this, "Set team of all players to team");
	Console()->Register("swap_teams", "", CFGFLAG_SERVER, ConSwapTeams, this, "Swap the current teams");
	Console()->Register("shuffle_teams", "", CFGFLAG_SERVER, ConShuffleTeams, this, "Shuffle the current teams");
	Console()->Register("lock_teams", "", CFGFLAG_SERVER, ConLockTeams, this, "Lock/unlock teams");

	Console()->Register("add_vote", "sr", CFGFLAG_SERVER, ConAddVote, this, "Add a voting option");
	Console()->Register("remove_vote", "s", CFGFLAG_SERVER, ConRemoveVote, this, "remove a voting option");
	Console()->Register("force_vote", "ss?r", CFGFLAG_SERVER, ConForceVote, this, "Force a voting option");
	Console()->Register("clear_votes", "", CFGFLAG_SERVER, ConClearVotes, this, "Clears the voting options");
	Console()->Register("vote", "r", CFGFLAG_SERVER, ConVote, this, "Force a vote to yes/no");
	
	Console()->Register("about", "", CFGFLAG_CHAT|CFGFLAG_USER, ConAbout, this, "Show information about the mod");
	Console()->Register("language", "?s", CFGFLAG_CHAT|CFGFLAG_USER, ConLanguage, this, "Switch language");
	Console()->Register("help", "?s", CFGFLAG_CHAT|CFGFLAG_USER, ConHelp, this, "Show gameplay help");
	Console()->Register("status", "?s", CFGFLAG_CHAT|CFGFLAG_USER, ConGameStatus, this, "Show mission status (/status me for career)");

	Console()->Register("gc_status", "", CFGFLAG_SERVER, ConGcStatus, this, "Print LC economy state");
	Console()->Register("gc_set_quota", "i", CFGFLAG_SERVER, ConGcSetQuota, this, "Set gc_quota");
	Console()->Register("gc_set_money", "i", CFGFLAG_SERVER, ConGcSetMoney, this, "Set gc_money");
	Console()->Register("mapgen_now", "", CFGFLAG_SERVER, ConMapGenNow, this, "Generate expedition map in lobby");
	Console()->Register("spawn_monster", "?i", CFGFLAG_SERVER, ConSpawnMonster, this, "Spawn a monster (type 0-7)");
	
	Console()->Chain("sv_motd", ConchainSpecialMotdupdate, this);
}

void CGameContext::OnInit(/*class IKernel *pKernel*/)
{
	m_pServer = Kernel()->RequestInterface<IServer>();
	m_pConsole = Kernel()->RequestInterface<IConsole>();
	m_pStorage = Kernel()->RequestInterface<IStorage>();
	m_World.SetGameServer(this);
	m_Events.SetGameServer(this);

	for(int i = 0; i < NUM_NETOBJTYPES; i++)
		Server()->SnapSetStaticsize(i, m_NetObjHandler.GetObjSize(i));

	m_Layers.Init(Kernel());
	m_Collision.Init(&m_Layers);
	m_pScrapInfo = new CScrapInfo(this);
	m_pScrapInfo->Init();
	m_MapGen.Init(&m_Layers, &m_Collision, m_pStorage);

	LcApplyMoon(g_Config.m_GcMoon);
	LcLoadCompanyStats(&m_CycleStats, m_aCareerStats);

	// select gametype
	m_pController = new CGameController(this);

	m_VoteStart = 0;

	// create all entities from the game layer
	CMapItemLayerTilemap *pTileMap = m_Layers.GameLayer();
	CTile *pTiles = (CTile *)Kernel()->RequestInterface<IMap>()->GetData(pTileMap->m_Data);


	for(int y = 0; y < pTileMap->m_Height; y++)
	{
		for(int x = 0; x < pTileMap->m_Width; x++)
		{
			int Index = pTiles[y*pTileMap->m_Width+x].m_Index;

			if(Index >= ENTITY_OFFSET)
			{
				vec2 Pos(x*32.0f+16.0f, y*32.0f+16.0f);
				m_pController->OnEntity(Index-ENTITY_OFFSET, Pos);
			}
		}
	}
}

void CGameContext::OnShutdown()
{
	delete m_pController;
	m_pController = 0;
	Clear();
}

void CGameContext::OnSnap(int ClientID)
{
	// add tuning to demo
	CTuningParams StandardTuning;
	if(ClientID == -1 && Server()->DemoRecorder_IsRecording() && mem_comp(&StandardTuning, &m_Tuning, sizeof(CTuningParams)) != 0)
	{
		CMsgPacker Msg(NETMSGTYPE_SV_TUNEPARAMS);
		int *pParams = (int *)&m_Tuning;
		for(unsigned i = 0; i < sizeof(m_Tuning)/sizeof(int); i++)
			Msg.AddInt(pParams[i]);
		Server()->SendMsg(&Msg, MSGFLAG_RECORD|MSGFLAG_NOSEND, ClientID);
	}

	m_World.Snap(ClientID);
	m_pController->Snap(ClientID);
	m_Events.Snap(ClientID);

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(m_apPlayers[i])
			m_apPlayers[i]->Snap(ClientID);
	}
}
void CGameContext::OnPreSnap() {}
void CGameContext::OnPostSnap()
{
	m_Events.Clear();
}

bool CGameContext::IsClientReady(int ClientID)
{
	return m_apPlayers[ClientID] && m_apPlayers[ClientID]->m_IsReady ? true : false;
}

bool CGameContext::IsClientPlayer(int ClientID)
{
	return m_apPlayers[ClientID] && m_apPlayers[ClientID]->GetTeam() == TEAM_SPECTATORS ? false : true;
}

const char *CGameContext::GameType() { return m_pController && m_pController->m_pGameType ? m_pController->m_pGameType : ""; }
const char *CGameContext::Version() { return GAME_VERSION; }
const char *CGameContext::NetVersion() { return GAME_NETVERSION; }

IGameServer *CreateGameServer() { return new CGameContext; }

void CGameContext::ResetVotes(int ClientID)
{
	if(ClientID == -1)
	{
		for (int i = 0; i < MAX_PLAYER; i++)
			ResetVotes(i);
		return;
	}

	if(ClientID < 0 || ClientID >= MAX_PLAYER)
		return;

	if(!m_apPlayers[ClientID])
		return;
	
	m_aPlayerVotes[ClientID].clear();

	// send vote options
	CNetMsg_Sv_VoteClearOptions ClearMsg;
	Server()->SendPackMsg(&ClearMsg, MSGFLAG_VITAL, ClientID);

	if (m_aPlayerVotes[ClientID].size())
		return;

	if (!m_apPlayers[ClientID]->GetCharacter())
	{
		AddVote(ClientID, "null", _("☪ 死人无法操作"));
		return;
	}

	LcBuildVoteMenu(this, ClientID);
}

void CGameContext::AddVote(int To, const char *aCmd, const char *pText, ...)
{
	int Start = (To < 0 ? 0 : To);
	int End = (To < 0 ? MAX_PLAYER : To + 1);

	dynamic_string Buffer;

	va_list VarArgs;
	va_start(VarArgs, pText);

	for (int i = Start; i < End; i++)
	{
		if (m_apPlayers[i])
		{
			Buffer.clear();
			Server()->Localization()->Format_VL(Buffer, m_apPlayers[i]->GetLanguage(), pText, VarArgs);
			{
				const char *Desc = Buffer.buffer();
				const char *Cmd = aCmd;
				int ClientID = i;
				while (*Desc && *Desc == ' ')
					Desc++;
			
				if (ClientID == -2)
					return;
			
				CVoteOptions Vote;
				str_copy(Vote.m_aDescription, Desc, sizeof(Vote.m_aDescription));
				str_copy(Vote.m_aCommand, Cmd, sizeof(Vote.m_aCommand));
				m_aPlayerVotes[ClientID].add(Vote);
			
				// inform clients about added option
				CNetMsg_Sv_VoteOptionAdd OptionMsg;
				OptionMsg.m_pDescription = Vote.m_aDescription;
				Server()->SendPackMsg(&OptionMsg, MSGFLAG_VITAL, ClientID);
			}
		}
	}

	Buffer.clear();
	va_end(VarArgs);
}

void CGameContext::AddBroadcast(int ClientID, const char *pText, int Priority, int LifeSpan)
{
	if (LifeSpan > 0)
	{
		if (m_aBroadcastStates[ClientID].m_TimedPriority > Priority)
			return;

		str_copy(m_aBroadcastStates[ClientID].m_aTimedMessage, pText, sizeof(m_aBroadcastStates[ClientID].m_aTimedMessage));
		m_aBroadcastStates[ClientID].m_LifeSpanTick = LifeSpan;
		m_aBroadcastStates[ClientID].m_TimedPriority = Priority;
	}
	else
	{
		if (m_aBroadcastStates[ClientID].m_Priority > Priority)
			return;

		str_copy(m_aBroadcastStates[ClientID].m_aNextMessage, pText, sizeof(m_aBroadcastStates[ClientID].m_aNextMessage));
		m_aBroadcastStates[ClientID].m_Priority = Priority;
	}
}

void CGameContext::ConHelp(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int ClientID = pResult->GetClientID();
	if(pResult->NumArguments() > 0)
	{
		const char *pTopic = pResult->GetString(0);
		if(str_comp_nocase(pTopic, "store") == 0)
			pSelf->OpenTerminalGuidePage(ClientID, LC_PAGE_GUIDE_STORE);
		else if(str_comp_nocase(pTopic, "monsters") == 0)
			pSelf->OpenTerminalGuidePage(ClientID, LC_PAGE_GUIDE_MONSTERS);
		else if(str_comp_nocase(pTopic, "scrap") == 0)
			pSelf->OpenTerminalGuidePage(ClientID, LC_PAGE_GUIDE_SCRAP);
		else if(str_comp_nocase(pTopic, "1") == 0)
		{
			pSelf->SendChatTarget(ClientID, _("【教程 1/4】大厅：ESC 投票选路线，全员投票出发"));
			pSelf->SendChatTarget(ClientID, _("F3 终端可购买商店、查看图鉴"));
		}
		else if(str_comp_nocase(pTopic, "2") == 0)
		{
			pSelf->SendChatTarget(ClientID, _("【教程 2/4】设施内：锤子拾取废品，4 格背包"));
			pSelf->SendChatTarget(ClientID, _("重量越大移动越慢，注意班次倒计时"));
		}
		else if(str_comp_nocase(pTopic, "3") == 0)
		{
			pSelf->SendChatTarget(ClientID, _("【教程 3/4】ESC/F3：理由 1=使用物品，理由空=放下/放入飞船"));
			pSelf->SendChatTarget(ClientID, _("冻结时用队友锤子救活；登船自动修复"));
		}
		else if(str_comp_nocase(pTopic, "4") == 0)
		{
			pSelf->SendChatTarget(ClientID, _("【教程 4/4】全员登船后投票启动飞船返航"));
			pSelf->SendChatTarget(ClientID, _("截止日前达成指标，否则全进度重置"));
			pSelf->SendChatTarget(ClientID, _("/help store|monsters|scrap 查看图鉴 | /status 任务状态"));
		}
		else
			pSelf->SendChatTarget(ClientID, _("用法: /help [1-4|store|monsters|scrap]"));
		return;
	}
	pSelf->SendChatTarget(ClientID, _("- - - - - - -"));
	pSelf->SendChatTarget(ClientID, _("【致命公司 · 快速指南】"));
	pSelf->SendChatTarget(ClientID, _("你是公司雇员。在截止日前达成指标。"));
	pSelf->SendChatTarget(ClientID, _("输入 /help 1 ~ /help 4 查看分步教程"));
	pSelf->SendChatTarget(ClientID, _("按 F3 打开公司终端；ESC 可发起投票"));
	pSelf->SendChatTarget(ClientID, _("/help store 商店 | /help monsters 怪物 | /help scrap 废品"));
	pSelf->SendChatTarget(ClientID, _("指令: /help /status /about /language"));
	pSelf->SendChatTarget(ClientID, _("- - - - - - -"));
}

static void SendGameStatus(CGameContext *pSelf, int ClientID)
{
	int Days = g_Config.m_GcDays;
	int Money = g_Config.m_GcMoney;
	int Quota = g_Config.m_GcQuota;
	int Rounds = g_Config.m_GcRounds;
	char aProgress[32];
	LcFormatQuotaProgress(aProgress, sizeof(aProgress), Money, Quota);
	pSelf->SendChatTarget(ClientID, _("=== 公司状态 ==="));
	pSelf->SendChatTarget(ClientID, _("班次 {int:rounds} | 截止 {int:days}天 | 指标 {int:money}/{int:quota}"), "rounds", &Rounds, "days", &Days, "money", &Money, "quota", &Quota);
	pSelf->SendChatTarget(ClientID, aProgress);
	if(pSelf->Server()->m_LocateGame == LOCATE_GAME && g_Config.m_SvTimelimit > 0 && pSelf->m_pController)
	{
		int LimitTicks = g_Config.m_SvTimelimit * pSelf->Server()->TickSpeed() * 60 + pSelf->m_pController->ExpeditionTimeBonusSec() * pSelf->Server()->TickSpeed();
		int RemainingTicks = LimitTicks - (pSelf->Server()->Tick() - pSelf->m_pController->RoundStartTick());
		int RemainingSec = RemainingTicks > 0 ? RemainingTicks / pSelf->Server()->TickSpeed() : 0;
		pSelf->SendChatTarget(ClientID, _("设施内 | 班次剩余 {int:sec} 秒"), "sec", &RemainingSec);
		pSelf->SendChatTarget(ClientID, _("路线：{lstr:moon}"), "moon", LcMoonName(g_Config.m_GcMoon));
	}
	else
	{
		pSelf->SendChatTarget(ClientID, _("当前位置: 公司飞船"));
		pSelf->SendChatTarget(ClientID, _("已选路线: {lstr:moon}"), "moon", LcMoonName(g_Config.m_GcMoon));
	}
	if(pSelf->m_LastMapGenSeed > 0)
	{
		int Seed = pSelf->m_LastMapGenSeed;
		pSelf->SendChatTarget(ClientID, _("地图种子: {int:seed}"), "seed", &Seed);
	}
}

void CGameContext::ConGameStatus(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int ClientID = pResult->GetClientID();
	if(ClientID < 0)
		return;
	if(pResult->NumArguments() > 0 && str_comp_nocase(pResult->GetString(0), "me") == 0)
	{
		const SLcPlayerCareerStats &C = pSelf->m_aCareerStats[ClientID];
		const SLcPlayerRoundStats &R = pSelf->m_aRoundStats[ClientID];
		pSelf->SendChatTarget(ClientID, _("=== 个人生涯 ==="));
		pSelf->SendChatTarget(ClientID, _("累计上交 {int:dep} | 累计救人 {int:rev} | 累计死亡 {int:death} | 单局最高 {int:best}"),
			"dep", &C.m_TotalDeposited, "rev", &C.m_TotalRevives, "death", &C.m_TotalDeaths, "best", &C.m_BestRoundDeposit);
		pSelf->SendChatTarget(ClientID, _("本局: 上交 {int:dep} | 丢失 {int:lost} | 救人 {int:rev} | 死亡 {int:death}"),
			"dep", &R.m_Deposited, "lost", &R.m_LostOnDeath, "rev", &R.m_RevivesGiven, "death", &R.m_Deaths);
		pSelf->SendChatTarget(ClientID, _("商店购买次数 {int:n} | 全服商店总花费 {int:sp}"), "n", &pSelf->m_aStorePurchaseCount[ClientID], "sp", &pSelf->m_TotalStoreSpend);
		return;
	}
	SendGameStatus(pSelf, ClientID);
}

void CGameContext::ConGcStatus(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "gc_quota=%d gc_money=%d gc_days=%d gc_rounds=%d phase=%d seed=%d",
		g_Config.m_GcQuota, g_Config.m_GcMoney, g_Config.m_GcDays, g_Config.m_GcRounds,
		pSelf->m_pController ? (int)pSelf->m_pController->ExpeditionPhase() : -1, pSelf->m_LastMapGenSeed);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);
}

void CGameContext::ConGcSetQuota(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(pResult->NumArguments() < 1)
		return;
	g_Config.m_GcQuota = pResult->GetInteger(0);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", "gc_quota updated");
}

void CGameContext::ConGcSetMoney(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(pResult->NumArguments() < 1)
		return;
	g_Config.m_GcMoney = pResult->GetInteger(0);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", "gc_money updated");
}

void CGameContext::ConMapGenNow(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	(void)pResult;
	if(pSelf->Server()->m_LocateGame != LOCATE_LOBBY)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", "mapgen_now only works in lobby");
		return;
	}
	pSelf->GenTheMap();
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", "map generation queued");
}

void CGameContext::ConSpawnMonster(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Type = pResult->NumArguments() > 0 ? pResult->GetInteger(0) : 0;
	if(Type < 0 || Type >= NUM_MONSTER_TYPES)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", "monster type must be 0..NUM_MONSTER_TYPES-1");
		return;
	}
	pSelf->NewMonster(Type);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", "monster spawned");
}

// MapGen
void CGameContext::SaveMap(const char *path)
{
	IMap *pMap = Layers()->Map();
	if (!pMap)
		return;

	CDataFileWriter fileWrite;
	char aMapFile[512];
	str_format(aMapFile, sizeof(aMapFile), "maps/%s.map", g_Config.m_SvMapGame);

	// Map will be saved to current dir, not to ~/.ninslash/maps or to data/maps, so we need to create a dir for it
	Storage()->CreateFolder("maps", IStorage::TYPE_SAVE);

	fileWrite.SaveMap(Storage(), pMap->GetFileReader(), aMapFile);

	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "Map saved in '%s'!", aMapFile);
	Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
}

enum
{
	MAPGEN_WORKER_IDLE = 0,
	MAPGEN_WORKER_RUNNING,
	MAPGEN_WORKER_DONE,
	MAPGEN_WORKER_FAILED,
};

struct SMapGenWorkerJob
{
	volatile int m_State;
	SMapGenStaging m_Staging;
	SMapGenParams m_Params;
	char m_aTemplate[128];
	char m_aTheme[128];
	CLayers *m_pLayers;
	CCollision *m_pCollision;
	IStorage *m_pStorage;
};

static void MapGenWorkerThread(void *pUser)
{
	SMapGenWorkerJob *pJob = (SMapGenWorkerJob *)pUser;
	pJob->m_State = MAPGEN_WORKER_RUNNING;

	CMapGenRandomScope RandScope(pJob->m_Params.m_Seed);

	const int W = pJob->m_pLayers->GameLayer()->m_Width;
	const int H = pJob->m_pLayers->GameLayer()->m_Height;
	if(!pJob->m_Staging.Init(W, H))
	{
		pJob->m_State = MAPGEN_WORKER_FAILED;
		return;
	}

	CMapGen Gen;
	Gen.Init(pJob->m_pLayers, pJob->m_pCollision, pJob->m_pStorage);
	Gen.SetParams(pJob->m_Params);
	Gen.UseStaging(&pJob->m_Staging);

	if(!Gen.BeginFillMap(pJob->m_aTemplate, pJob->m_aTheme))
	{
		pJob->m_Staging.Free();
		pJob->m_State = MAPGEN_WORKER_FAILED;
		return;
	}

	while(Gen.IsGenerating())
		Gen.StepFillMap();

	if(!Gen.IsDone())
	{
		pJob->m_Staging.Free();
		pJob->m_State = MAPGEN_WORKER_FAILED;
		return;
	}

	pJob->m_State = MAPGEN_WORKER_DONE;
}

void CGameContext::CancelMapGenJob()
{
	if(m_pMapGenJob)
	{
		m_pMapGenJob->m_Staging.Free();
		delete m_pMapGenJob;
		m_pMapGenJob = 0;
	}
}

void CGameContext::GenTheMap()
{
	if(m_MapGenPending || m_MapGenActive || Server()->m_MapGenerated)
		return;

	Server()->m_MapGenerated = false;
	m_MapGenPending = true;
	m_World.m_Paused = true;
}

void CGameContext::ProcessMapGen()
{
	if(m_MapGenPending)
	{
		m_MapGenPending = false;
		CancelMapGenJob();
		m_MapGenProgressStage = 1;

		if(g_Config.m_SvMapGenRandSeed)
			g_Config.m_SvMapGenSeed = rand() % 32768;
		srand(g_Config.m_SvMapGenSeed);
		m_LastMapGenSeed = g_Config.m_SvMapGenSeed;

		m_pMapGenJob = new SMapGenWorkerJob();
		m_pMapGenJob->m_State = MAPGEN_WORKER_RUNNING;
		m_pMapGenJob->m_Params.m_Seed = g_Config.m_SvMapGenSeed;
		m_pMapGenJob->m_Params.m_GcMoon = g_Config.m_GcMoon;
		m_pMapGenJob->m_Params.m_GcRounds = g_Config.m_GcRounds;
		m_pMapGenJob->m_Params.m_MapGenLevel = g_Config.m_SvMapGenLevel;
		str_copy(m_pMapGenJob->m_aTemplate, g_Config.m_SvMapLobby, sizeof(m_pMapGenJob->m_aTemplate));
		str_copy(m_pMapGenJob->m_aTheme, g_Config.m_SvMapgenTheme, sizeof(m_pMapGenJob->m_aTheme));
		m_pMapGenJob->m_pLayers = &m_Layers;
		m_pMapGenJob->m_pCollision = &m_Collision;
		m_pMapGenJob->m_pStorage = m_pStorage;

		MapGen()->ResetStep();
		m_MapGenActive = true;
		m_MapGenApplying = false;

		thread_init(MapGenWorkerThread, m_pMapGenJob);

		SendChatTarget(-1, _("设施: {lstr:fac} | 主题: {str:theme}"), "fac", LcFacilityName(LcGetFacilityType(g_Config.m_GcMoon)), "theme", g_Config.m_SvMapgenTheme);
		return;
	}

	if(!m_MapGenActive || !m_pMapGenJob)
		return;

	if(m_pMapGenJob->m_State == MAPGEN_WORKER_RUNNING)
	{
		m_MapGenProgressStage = 2;
		return;
	}

	if(m_pMapGenJob->m_State == MAPGEN_WORKER_FAILED)
	{
		m_MapGenActive = false;
		m_MapGenFailed = true;
		m_MapGenProgressStage = 0;
		m_World.m_Paused = false;
		CancelMapGenJob();
		SendChatTarget(-1, _("地图生成失败（布局/连通性），请重试"));
		return;
	}

	if(m_pMapGenJob->m_State == MAPGEN_WORKER_DONE)
	{
		m_MapGenProgressStage = 3;
		if(!m_MapGenApplying)
		{
			MapGen()->ResetStep();
			MapGen()->UseStaging(&m_pMapGenJob->m_Staging);
			MapGen()->ResetApplyIndex();
			m_MapGenApplying = true;
		}

		if(MapGen()->ApplyStagingChunk(2048))
		{
			m_MapGenApplying = false;
			m_MapGenActive = false;
			MapGen()->ApplyThemeTilesets(g_Config.m_SvMapgenTheme);
			SaveMap("");
			Server()->m_MapGenerated = true;
			str_copy(g_Config.m_SvMap, g_Config.m_SvMapGame, sizeof(g_Config.m_SvMap));
			SendChatTarget(-1, _("地图种子: {int:seed}"), "seed", &m_LastMapGenSeed);
			MapGen()->ClearStaging();
			CancelMapGenJob();
		}
	}
}

void CGameContext::GivePlayerScrap(int ClientID, int ScrapType, int Value, int Weight)
{
	if(ClientID < 0 || ClientID >= MAX_CLIENTS || !m_apPlayers[ClientID])
		return;
	if((int)m_apPlayers[ClientID]->m_vScraps.size() >= GC_MAX_SCRAP_SLOTS)
		return;

	Scrap *pTemp = new Scrap();
	pTemp->m_ID = m_apPlayers[ClientID]->m_ItemCount++;
	pTemp->m_ScrapID = ScrapType;
	pTemp->m_Value = Value;
	pTemp->m_Weight = Weight;
	pTemp->m_InShip = false;
	m_apPlayers[ClientID]->m_vScraps.add(pTemp);
}

void CGameContext::DepositScrapInShip(vec2 Pos, const Scrap &Item)
{
	for(CScrap *pScrap = (CScrap *)m_World.FindFirst(CGameWorld::ENTTYPE_SCRAP); pScrap; pScrap = (CScrap *)pScrap->TypeNext())
	{
		if(!pScrap->GetInShip() || pScrap->GetScrapType() != Item.m_ScrapID)
			continue;
		if(distance(pScrap->m_Pos, Pos) <= GC_SHIP_SCRAP_MERGE_RADIUS)
		{
			pScrap->MergeStats(Item.m_Value, Item.m_Weight);
			pScrap->m_Pos = Pos;
			return;
		}
	}

	new CScrap(&m_World, 0, Pos, false, true, Item);
}

void CGameContext::CreditShipScrapDeposit(int ClientID, int Value)
{
	if(ClientID < 0 || ClientID >= MAX_CLIENTS || !m_apPlayers[ClientID] || Value <= 0)
		return;

	Server()->GetClientSession(ClientID)->m_TotalEarn += Value;
	LcRecordDeposit(this, ClientID, Value);
	SendChatTarget(ClientID, _("你为公司贡献了{int:v}元"), "v", &Value);
}

void CGameContext::CompactShipScrap(vec2 Center, float Radius)
{
	(void)Radius;
	for(int Type = 0; Type < NUM_SCRAPS; Type++)
	{
		CScrap *pKeep = 0;
		for(CScrap *pScrap = (CScrap *)m_World.FindFirst(CGameWorld::ENTTYPE_SCRAP); pScrap; pScrap = (CScrap *)pScrap->TypeNext())
		{
			if(!pScrap->GetInShip() || pScrap->GetScrapType() != Type)
				continue;
			if(!pKeep)
			{
				pKeep = pScrap;
				pKeep->m_Pos = Center;
				continue;
			}
			pKeep->MergeStats(pScrap->GetScrapValue(), pScrap->GetWeight());
			pScrap->Reset();
		}
	}
}

bool CGameContext::HasPendingExpeditionBonus(int ClientID) const
{
	if(ClientID < 0 || ClientID >= MAX_CLIENTS)
		return false;
	return m_aNextArmorBonus[ClientID] > 0 || m_aNextFlashBonus[ClientID] > 0 || m_aStoreBonus[ClientID].HasAny();
}

void CGameContext::ApplyExpeditionBonuses(int ClientID)
{
	if(ClientID < 0 || ClientID >= MAX_CLIENTS || !m_apPlayers[ClientID])
		return;

	if(!HasPendingExpeditionBonus(ClientID))
		return;

	CCharacter *pChr = GetPlayerChar(ClientID);
	if(m_aNextArmorBonus[ClientID] > 0 && pChr)
	{
		pChr->IncreaseArmor(m_aNextArmorBonus[ClientID]);
		m_aNextArmorBonus[ClientID] = 0;
		SendChatTarget(ClientID, _("公司配发了防护服"));
	}
	if(m_aNextFlashBonus[ClientID] > 0)
	{
		GivePlayerScrap(ClientID, SCRAP_L1_FLASHBANG, 6, 6);
		m_aNextFlashBonus[ClientID] = 0;
		SendChatTarget(ClientID, _("公司配发了闪光弹装备"));
	}

	CLcStoreBonus &B = m_aStoreBonus[ClientID];
	if(pChr)
	{
		if(B.m_ShotgunAmmo > 0)
		{
			pChr->GiveWeapon(WEAPON_SHOTGUN, B.m_ShotgunAmmo);
			SendChatTarget(ClientID, _("公司配发了散弹枪（{int:ammo}发）"), "ammo", &B.m_ShotgunAmmo);
			B.m_ShotgunAmmo = 0;
		}
		if(B.m_RifleAmmo > 0)
		{
			pChr->GiveWeapon(WEAPON_RIFLE, B.m_RifleAmmo);
			SendChatTarget(ClientID, _("公司配发了激光枪（{int:ammo}发）"), "ammo", &B.m_RifleAmmo);
			B.m_RifleAmmo = 0;
		}
		if(B.m_GrenadeAmmo > 0)
		{
			pChr->GiveWeapon(WEAPON_GRENADE, B.m_GrenadeAmmo);
			SendChatTarget(ClientID, _("公司配发了榴弹（{int:ammo}发）"), "ammo", &B.m_GrenadeAmmo);
			B.m_GrenadeAmmo = 0;
		}
		if(B.m_GunAmmo > 0)
		{
			pChr->GiveWeapon(WEAPON_GUN, B.m_GunAmmo);
			SendChatTarget(ClientID, _("公司配发了手枪（{int:ammo}发）"), "ammo", &B.m_GunAmmo);
			B.m_GunAmmo = 0;
		}
		if(B.m_NinjaAmmo != 0)
		{
			pChr->GiveWeapon(WEAPON_NINJA, B.m_NinjaAmmo);
			SendChatTarget(ClientID, _("公司配发了忍者刀"));
			B.m_NinjaAmmo = 0;
		}
		if(B.m_HealthBonus > 0)
		{
			pChr->IncreaseHealth(B.m_HealthBonus);
			SendChatTarget(ClientID, _("公司配发了生命保障（+{int:hp}）"), "hp", &B.m_HealthBonus);
			B.m_HealthBonus = 0;
		}
	}
	if(B.m_Medkit > 0)
	{
		GivePlayerScrap(ClientID, SCRAP_L2_MEDKIT, 22, 10);
		SendChatTarget(ClientID, _("公司配发了急救包"));
		B.m_Medkit = 0;
	}
	if(B.m_Whistle > 0)
	{
		GivePlayerScrap(ClientID, SCRAP_L1_WHISTLE, 8, 2);
		SendChatTarget(ClientID, _("公司配发了驱虫哨"));
		B.m_Whistle = 0;
	}
	if(B.m_Soda > 0)
	{
		GivePlayerScrap(ClientID, SCRAP_L1_SODA, 5, 3);
		SendChatTarget(ClientID, _("公司配发了能量汽水"));
		B.m_Soda = 0;
	}
	if(B.m_Megaphone > 0)
	{
		GivePlayerScrap(ClientID, SCRAP_L2_MEGAPHONE, 20, 9);
		SendChatTarget(ClientID, _("公司配发了扩音器"));
		B.m_Megaphone = 0;
	}
	if(B.m_Boombox > 0)
	{
		GivePlayerScrap(ClientID, SCRAP_L3_BOOMBOX, 87, 27);
		SendChatTarget(ClientID, _("公司配发了音响"));
		B.m_Boombox = 0;
	}
	if(B.m_Remote > 0)
	{
		GivePlayerScrap(ClientID, SCRAP_L2_REMOTE, 19, 7);
		SendChatTarget(ClientID, _("公司配发了遥控器"));
		B.m_Remote = 0;
	}
	if(B.m_Aircraft > 0)
	{
		m_aAircraftStock[ClientID] += B.m_Aircraft;
		SendChatTarget(ClientID, _("公司配发了飞行器（库存 {int:stock}）"), "stock", &m_aAircraftStock[ClientID]);
		B.m_Aircraft = 0;
	}
}

void CGameContext::TryBuyStoreItem(int ClientID, const char *pItem)
{
	int Cost = 0;
	if(str_comp(pItem, "buy_time") == 0)
		Cost = GC_STORE_TIME_COST;
	else if(str_comp(pItem, "buy_flash") == 0)
		Cost = GC_STORE_FLASH_COST;
	else if(str_comp(pItem, "buy_armor") == 0)
		Cost = GC_STORE_ARMOR_COST;
	else if(str_comp(pItem, "buy_shotgun") == 0)
		Cost = GC_STORE_SHOTGUN_COST;
	else if(str_comp(pItem, "buy_rifle") == 0)
		Cost = GC_STORE_RIFLE_COST;
	else if(str_comp(pItem, "buy_grenade") == 0)
		Cost = GC_STORE_GRENADE_COST;
	else if(str_comp(pItem, "buy_medkit") == 0)
		Cost = GC_STORE_MEDKIT_COST;
	else if(str_comp(pItem, "buy_whistle") == 0)
		Cost = GC_STORE_WHISTLE_COST;
	else if(str_comp(pItem, "buy_soda") == 0)
		Cost = GC_STORE_SODA_COST;
	else if(str_comp(pItem, "buy_gun") == 0)
		Cost = GC_STORE_GUN_COST;
	else if(str_comp(pItem, "buy_ninja") == 0)
		Cost = GC_STORE_NINJA_COST;
	else if(str_comp(pItem, "buy_health") == 0)
		Cost = GC_STORE_HEALTH_COST;
	else if(str_comp(pItem, "buy_megaphone") == 0)
		Cost = GC_STORE_MEGAPHONE_COST;
	else if(str_comp(pItem, "buy_boombox") == 0)
		Cost = GC_STORE_BOOMBOX_COST;
	else if(str_comp(pItem, "buy_remote") == 0)
		Cost = GC_STORE_REMOTE_COST;
	else if(str_comp(pItem, "buy_aircraft") == 0)
		Cost = GC_STORE_AIRCRAFT_COST;
	else
		return;

	if(g_Config.m_GcMoney < Cost)
	{
		SendChatTarget(ClientID, _("公司资金不足，无法购买"));
		return;
	}

	int PrevMoney = g_Config.m_GcMoney;
	g_Config.m_GcMoney -= Cost;
	LcLogStorePurchase(this, ClientID, pItem, Cost);
	LcPlayUiSound(this, SOUND_PICKUP_ARMOR, ClientID);
	if(str_comp(pItem, "buy_time") == 0)
	{
		m_NextExpeditionTimeBonusSec += GC_STORE_TIME_BONUS_SEC;
		SendChatTarget(ClientID, _("已购买：下趟班次 +2 分钟"));
	}
	else if(str_comp(pItem, "buy_flash") == 0)
	{
		m_aNextFlashBonus[ClientID] = 1;
		SendChatTarget(ClientID, _("已购买：下趟班次闪光弹"));
	}
	else if(str_comp(pItem, "buy_armor") == 0)
	{
		m_aNextArmorBonus[ClientID] = GC_STORE_ARMOR_BONUS;
		SendChatTarget(ClientID, _("已购买：下趟班次防护服"));
	}
	else if(str_comp(pItem, "buy_shotgun") == 0)
	{
		m_aStoreBonus[ClientID].m_ShotgunAmmo = GC_STORE_SHOTGUN_AMMO;
		SendChatTarget(ClientID, _("已购买：下趟班次散弹枪"));
	}
	else if(str_comp(pItem, "buy_rifle") == 0)
	{
		m_aStoreBonus[ClientID].m_RifleAmmo = GC_STORE_RIFLE_AMMO;
		SendChatTarget(ClientID, _("已购买：下趟班次激光枪"));
	}
	else if(str_comp(pItem, "buy_grenade") == 0)
	{
		m_aStoreBonus[ClientID].m_GrenadeAmmo = GC_STORE_GRENADE_AMMO;
		SendChatTarget(ClientID, _("已购买：下趟班次榴弹"));
	}
	else if(str_comp(pItem, "buy_medkit") == 0)
	{
		m_aStoreBonus[ClientID].m_Medkit = 1;
		SendChatTarget(ClientID, _("已购买：下趟班次急救包"));
	}
	else if(str_comp(pItem, "buy_whistle") == 0)
	{
		m_aStoreBonus[ClientID].m_Whistle = 1;
		SendChatTarget(ClientID, _("已购买：下趟班次驱虫哨"));
	}
	else if(str_comp(pItem, "buy_soda") == 0)
	{
		m_aStoreBonus[ClientID].m_Soda = 1;
		SendChatTarget(ClientID, _("已购买：下趟班次能量汽水"));
	}
	else if(str_comp(pItem, "buy_gun") == 0)
	{
		m_aStoreBonus[ClientID].m_GunAmmo = GC_STORE_GUN_AMMO;
		SendChatTarget(ClientID, _("已购买：下趟班次手枪"));
	}
	else if(str_comp(pItem, "buy_ninja") == 0)
	{
		m_aStoreBonus[ClientID].m_NinjaAmmo = -1;
		SendChatTarget(ClientID, _("已购买：下趟班次忍者刀"));
	}
	else if(str_comp(pItem, "buy_health") == 0)
	{
		m_aStoreBonus[ClientID].m_HealthBonus = GC_STORE_HEALTH_BONUS;
		SendChatTarget(ClientID, _("已购买：下趟班次生命保障"));
	}
	else if(str_comp(pItem, "buy_megaphone") == 0)
	{
		m_aStoreBonus[ClientID].m_Megaphone = 1;
		SendChatTarget(ClientID, _("已购买：下趟班次扩音器"));
	}
	else if(str_comp(pItem, "buy_boombox") == 0)
	{
		m_aStoreBonus[ClientID].m_Boombox = 1;
		SendChatTarget(ClientID, _("已购买：下趟班次音响"));
	}
	else if(str_comp(pItem, "buy_remote") == 0)
	{
		m_aStoreBonus[ClientID].m_Remote = 1;
		SendChatTarget(ClientID, _("已购买：下趟班次遥控器"));
	}
	else if(str_comp(pItem, "buy_aircraft") == 0)
	{
		m_aStoreBonus[ClientID].m_Aircraft += 1;
		SendChatTarget(ClientID, _("已购买：下趟班次飞行器（待发放 {int:count}）"), "count", &m_aStoreBonus[ClientID].m_Aircraft);
	}

	SendChatTarget(-1, _("{str:name} 购买了 {lstr:item}（-{int:cost} 币）"),
		"name", Server()->ClientName(ClientID), "item", LcStoreItemName(pItem), "cost", &Cost);
	LcCheckQuotaMilestones(this, PrevMoney);
}

void CGameContext::CreateScanLink(vec2 From, vec2 To)
{
	new CScanLink(&m_World, From, To, GC_SCAN_LINK_SEC * Server()->TickSpeed());
	CreateSound(From, SOUND_RIFLE_FIRE);
}

void CGameContext::OpenTerminalGuidePage(int ClientID, int Page)
{
	CPlayer *pPlayer = m_apPlayers[ClientID];
	if(!pPlayer || !pPlayer->GetCharacter() || pPlayer->GetCharacter()->m_Freeze)
	{
		SendChatTarget(ClientID, _("☪ 死人无法操作"));
		return;
	}

	if(!pPlayer->m_TerminalMenuOpen)
		OpenTerminalMenu(ClientID);

	pPlayer->m_TerminalMenuPage = Page;
	pPlayer->m_TerminalMenuSelection = 0;
	pPlayer->m_TerminalMenuTextScroll = 0;
	RefreshTerminalMenu(ClientID);
}

bool CGameContext::ExecutePlayerVoteCommand(int ClientID, const char *pCmd, const char *pReason)
{
	if(!pCmd || !pCmd[0] || !m_apPlayers[ClientID])
		return false;

	if(str_comp(pCmd, "qstart") == 0)
	{
		if(m_apPlayers[ClientID]->GetCharacter() && m_apPlayers[ClientID]->GetCharacter()->m_Freeze)
		{
			SendChatTarget(ClientID, _("☪ 死人无法操作"));
			return true;
		}

		if(Server()->m_LocateGame == LOCATE_LOBBY && m_CountInGame < g_Config.m_SvLessPlayerStart)
		{
			SendChatTarget(ClientID, _("需要更多员工才能出发"));
			return true;
		}

		int Need = GetNeedVoteStart();
		m_apPlayers[ClientID]->m_VoteStarted = !m_apPlayers[ClientID]->m_VoteStarted;
		m_VoteStart += m_apPlayers[ClientID]->m_VoteStarted * 2 - 1;
		if(Server()->m_LocateGame == LOCATE_LOBBY)
		{
			if(!m_apPlayers[ClientID]->m_VoteStarted)
				SendChatTarget(ClientID, _("你取消了开始游戏的投票"));
			else
				SendChatTarget(-1, _("{str:name} 投票请求开始游戏[{int:now}/{int:need}]"), "name", Server()->ClientName(ClientID), "now", &m_VoteStart, "need", &Need);
		}
		else
		{
			if(!m_apPlayers[ClientID]->m_VoteStarted)
				SendChatTarget(ClientID, _("你取消了启动飞船的投票"));
			else
				SendChatTarget(-1, _("{str:name} 投票请求启动飞船[{int:now}/{int:need}]"), "name", Server()->ClientName(ClientID), "now", &m_VoteStart, "need", &Need);
		}
		ResetVotes(-1);
		return true;
	}

	if(str_comp(pCmd, "theme metal") == 0)
	{
		str_copy(g_Config.m_SvMapgenTheme, "metal_main", sizeof(g_Config.m_SvMapgenTheme));
		SendChatTarget(-1, _("{str:name} 选择了地图主题: {str:theme}"), "name", Server()->ClientName(ClientID), "theme", g_Config.m_SvMapgenTheme);
		return true;
	}

	if(str_comp(pCmd, "theme grass") == 0)
	{
		str_copy(g_Config.m_SvMapgenTheme, "grass_main", sizeof(g_Config.m_SvMapgenTheme));
		SendChatTarget(-1, _("{str:name} 选择了地图主题: {str:theme}"), "name", Server()->ClientName(ClientID), "theme", g_Config.m_SvMapgenTheme);
		return true;
	}

	if(str_comp(pCmd, "theme desert") == 0)
	{
		str_copy(g_Config.m_SvMapgenTheme, "desert_main", sizeof(g_Config.m_SvMapgenTheme));
		SendChatTarget(-1, _("{str:name} 选择了地图主题: {str:theme}"), "name", Server()->ClientName(ClientID), "theme", g_Config.m_SvMapgenTheme);
		return true;
	}

	if(str_comp(pCmd, "theme jungle") == 0)
	{
		str_copy(g_Config.m_SvMapgenTheme, "jungle_main", sizeof(g_Config.m_SvMapgenTheme));
		SendChatTarget(-1, _("{str:name} 选择了地图主题: {str:theme}"), "name", Server()->ClientName(ClientID), "theme", g_Config.m_SvMapgenTheme);
		return true;
	}

	if(str_comp(pCmd, "theme winter") == 0)
	{
		str_copy(g_Config.m_SvMapgenTheme, "winter_main", sizeof(g_Config.m_SvMapgenTheme));
		SendChatTarget(-1, _("{str:name} 选择了地图主题: {str:theme}"), "name", Server()->ClientName(ClientID), "theme", g_Config.m_SvMapgenTheme);
		return true;
	}

	if(str_comp(pCmd, "theme outdoor") == 0 || str_comp(pCmd, "theme mines") == 0 ||
		str_comp(pCmd, "theme lab") == 0 || str_comp(pCmd, "theme ruins") == 0)
	{
		SendChatTarget(ClientID, _("该主题已移除，请使用: theme metal|grass|desert|jungle|winter"));
		return true;
	}

	if(str_comp_num(pCmd, "moon ", 5) == 0)
	{
		int Moon = str_toint(pCmd + 5);
		if(Moon < 0 || Moon >= NUM_LC_MOONS)
			return false;
		g_Config.m_GcMoon = Moon;
		LcApplyMoon(Moon);
		SendChatTarget(-1, _("{str:name} 选择了路线: {lstr:moon}"), "name", Server()->ClientName(ClientID), "moon", LcMoonName(Moon));
		return true;
	}

	if(str_comp(pCmd, "help_store") == 0)
	{
		OpenTerminalGuidePage(ClientID, LC_PAGE_GUIDE_STORE);
		return true;
	}

	if(str_comp(pCmd, "help_monsters") == 0)
	{
		OpenTerminalGuidePage(ClientID, LC_PAGE_GUIDE_MONSTERS);
		return true;
	}

	if(str_comp(pCmd, "help_scrap") == 0)
	{
		OpenTerminalGuidePage(ClientID, LC_PAGE_GUIDE_SCRAP);
		return true;
	}

	if(str_comp_num(pCmd, "buy_", 4) == 0)
	{
		TryBuyStoreItem(ClientID, pCmd);
		return true;
	}

	if(str_comp(pCmd, "scan") == 0)
	{
		ScanFacility(ClientID);
		return true;
	}

	if(str_comp(pCmd, "refresh_monsters") == 0)
		return true;

	char aBuf[64];
	for(int i = 0; i < m_apPlayers[ClientID]->m_vScraps.size(); i++)
	{
		if(!m_apPlayers[ClientID]->GetCharacter())
			break;

		str_format(aBuf, sizeof(aBuf), "scrap %d", i);
		if(str_comp(pCmd, aBuf) != 0)
			continue;

		if(str_comp(pReason, "1") == 0)
			ScrapInfo()->Call(m_apPlayers[ClientID]->m_vScraps[i]->m_ID, m_apPlayers[ClientID]->m_vScraps[i]->m_ScrapID, ClientID);
		else
		{
			CCharacter *pChr = m_apPlayers[ClientID]->GetCharacter();
			if(!m_apPlayers[ClientID]->m_vScraps[i]->m_InShip && pChr->m_InShip)
				CreditShipScrapDeposit(ClientID, m_apPlayers[ClientID]->m_vScraps[i]->m_Value);
			if(pChr->m_InShip && m_pController && m_pController->m_pShip)
			{
				vec2 DropPos = m_pController->m_pShip->m_Pos;
				DepositScrapInShip(DropPos, *m_apPlayers[ClientID]->m_vScraps[i]);
				CompactShipScrap(DropPos);
			}
			else
				new CScrap(&m_World, 0, pChr->m_Pos, false, false, *m_apPlayers[ClientID]->m_vScraps[i]);
			m_apPlayers[ClientID]->EraseScrap(m_apPlayers[ClientID]->m_vScraps[i]->m_ID);
		}

		if(m_pController && m_pController->m_pShip)
			m_pController->m_pShip->UpdateValue();
		return true;
	}

	return false;
}

void CGameContext::ToggleTerminalMenu(int ClientID)
{
	CPlayer *pPlayer = m_apPlayers[ClientID];
	if(!pPlayer)
		return;
	if(!pPlayer->GetCharacter() && Server()->m_LocateGame != LOCATE_LOBBY)
		return;

	if(Server()->Tick() < pPlayer->m_TerminalMenuToggleTick + Server()->TickSpeed() / 3)
		return;

	pPlayer->m_TerminalMenuToggleTick = Server()->Tick();

	if(pPlayer->m_TerminalMenuOpen)
	{
		CloseTerminalMenu(ClientID);
		pPlayer->m_LastMenuVoteKey = 1;
	}
	else
		OpenTerminalMenu(ClientID);
}

void CGameContext::OpenTerminalMenu(int ClientID)
{
	CPlayer *pPlayer = m_apPlayers[ClientID];
	if(!pPlayer || pPlayer->m_TerminalMenuOpen)
		return;

	if(pPlayer->GetCharacter() && pPlayer->GetCharacter()->m_Freeze)
	{
		SendChatTarget(ClientID, _("☪ 死人无法操作"));
		return;
	}

	if(!pPlayer->GetCharacter() && Server()->m_LocateGame != LOCATE_LOBBY)
		return;

	if(pPlayer->m_TerminalWelcomePending)
	{
		pPlayer->m_TerminalWelcomePending = false;
		pPlayer->m_TerminalMenuPage = LC_PAGE_WELCOME;
	}

	pPlayer->m_TerminalMenuOpen = true;
	pPlayer->m_TerminalMenuSelection = 0;
	pPlayer->m_TerminalMenuTextScroll = 0;
	pPlayer->m_TerminalMenuInputWarmup = true;
	pPlayer->m_TerminalMenuFireBlock = false;
	pPlayer->m_TerminalMenuIgnoreHookUntilTick = Server()->Tick() + Server()->TickSpeed() / 2;
	if(pPlayer->GetCharacter())
		pPlayer->GetCharacter()->ResetInput();
	m_TerminalMenu.SendMotd(this, ClientID, 0);
	pPlayer->m_TerminalMenuMotdTick = Server()->Tick();
}

void CGameContext::CloseTerminalMenu(int ClientID)
{
	CPlayer *pPlayer = m_apPlayers[ClientID];
	if(!pPlayer || !pPlayer->m_TerminalMenuOpen)
		return;

	pPlayer->m_TerminalMenuOpen = false;
	pPlayer->m_TerminalMenuPage = LC_PAGE_MAIN;
	pPlayer->m_TerminalMenuSelection = 0;
	pPlayer->m_TerminalMenuTextScroll = 0;
	pPlayer->m_TerminalMenuInputWarmup = false;
	pPlayer->m_TerminalMenuFireBlock = true;
	pPlayer->m_LastMenuVoteKey = 0;

	if(pPlayer->GetCharacter())
	{
		pPlayer->GetCharacter()->SyncDirectInput(&pPlayer->m_TerminalMenuPrevInput);
		pPlayer->GetCharacter()->OnPredictedInput(&pPlayer->m_TerminalMenuPrevInput);
	}

	CNetMsg_Sv_Motd Msg;
	Msg.m_pMessage = " ";
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, ClientID);
}

void CGameContext::RefreshTerminalMenu(int ClientID)
{
	CPlayer *pPlayer = m_apPlayers[ClientID];
	if(!pPlayer || !pPlayer->m_TerminalMenuOpen)
		return;

	if(!pPlayer->GetCharacter() && Server()->m_LocateGame != LOCATE_LOBBY)
	{
		CloseTerminalMenu(ClientID);
		return;
	}

	m_TerminalMenu.Populate(this, ClientID);
	if(pPlayer->m_TerminalMenuSelection >= m_TerminalMenu.NumActions())
		pPlayer->m_TerminalMenuSelection = maximum(0, m_TerminalMenu.NumActions() - 1);

	m_TerminalMenu.SendMotd(this, ClientID, pPlayer->m_TerminalMenuSelection);
	pPlayer->m_TerminalMenuMotdTick = Server()->Tick();
}

void CGameContext::TerminalMenuGoBack(int ClientID)
{
	CPlayer *pPlayer = m_apPlayers[ClientID];
	if(!pPlayer || !pPlayer->m_TerminalMenuOpen)
		return;

	const int Parent = LcTerminalParentPage(pPlayer->m_TerminalMenuPage);
	if(Parent < 0)
	{
		CloseTerminalMenu(ClientID);
		return;
	}

	pPlayer->m_TerminalMenuPage = Parent;
	pPlayer->m_TerminalMenuSelection = 0;
	pPlayer->m_TerminalMenuTextScroll = 0;
	RefreshTerminalMenu(ClientID);
}

namespace
{
struct CMenuInputCount
{
	int m_Presses;
};

CMenuInputCount CountMenuInput(int Prev, int Cur)
{
	CMenuInputCount Result = {0};
	Prev &= INPUT_STATE_MASK;
	Cur &= INPUT_STATE_MASK;
	if(Cur == Prev)
		return Result;
	if(Cur < Prev && Prev - Cur <= 8)
		return Result;
	int i = Prev;

	while(i != Cur)
	{
		i = (i + 1) & INPUT_STATE_MASK;
		if(i & 1)
			Result.m_Presses++;
	}

	return Result;
}
} // namespace

bool CGameContext::HandleTerminalMenuInput(int ClientID, const CNetObj_PlayerInput *pInput, const CNetObj_PlayerInput *pPrevInput)
{
	CPlayer *pPlayer = m_apPlayers[ClientID];
	if(!pPlayer || !pPlayer->m_TerminalMenuOpen || !pInput || !pPrevInput)
		return false;

	const int NextScroll = pInput->m_NextWeapon != pPrevInput->m_NextWeapon ? 1 : 0;
	const int PrevScroll = pInput->m_PrevWeapon != pPrevInput->m_PrevWeapon ? 1 : 0;
	const int FirePress = CountMenuInput(pPrevInput->m_Fire, pInput->m_Fire).m_Presses;
	const int HookPress = CountMenuInput(pPrevInput->m_Hook, pInput->m_Hook).m_Presses;

	if(!NextScroll && !PrevScroll && !FirePress && !HookPress)
		return false;

	m_TerminalMenu.Populate(this, ClientID);

	if(HookPress && !FirePress)
	{
		if(Server()->Tick() < pPlayer->m_TerminalMenuIgnoreHookUntilTick)
			return false;
		TerminalMenuGoBack(ClientID);
		return true;
	}

	const int NumInfo = m_TerminalMenu.NumInfoLines();
	const int MaxScroll = maximum(0, NumInfo - CLcTerminalMenu::TERMINAL_VISIBLE_INFO);
	const int NumActions = m_TerminalMenu.NumActions();
	const bool InfoScrollMode = LcTerminalInfoScrollPage(pPlayer->m_TerminalMenuPage);
	const bool InfoOverflow = MaxScroll > 0;
	const bool TextScrollMode = InfoScrollMode || InfoOverflow;

	if(TextScrollMode)
	{
		if(NextScroll)
		{
			if(pPlayer->m_TerminalMenuTextScroll < MaxScroll)
				pPlayer->m_TerminalMenuTextScroll++;
			else if(NumActions > 0)
				pPlayer->m_TerminalMenuSelection = (pPlayer->m_TerminalMenuSelection + 1) % NumActions;
		}
		else if(PrevScroll)
		{
			if(pPlayer->m_TerminalMenuTextScroll > 0)
				pPlayer->m_TerminalMenuTextScroll--;
			else if(NumActions > 0)
				pPlayer->m_TerminalMenuSelection = (pPlayer->m_TerminalMenuSelection - 1 + NumActions) % NumActions;
		}

		if(FirePress && NumActions > 0)
		{
			m_TerminalMenu.ExecuteAction(this, pPlayer, pPlayer->m_TerminalMenuSelection);
			m_TerminalMenu.Populate(this, ClientID);
			if(pPlayer->m_TerminalMenuSelection >= m_TerminalMenu.NumActions())
				pPlayer->m_TerminalMenuSelection = maximum(0, m_TerminalMenu.NumActions() - 1);
			RefreshTerminalMenu(ClientID);
			return true;
		}

		if(NextScroll || PrevScroll)
		{
			RefreshTerminalMenu(ClientID);
			return true;
		}

		return false;
	}

	if(NumActions <= 0)
	{
		if(NextScroll || PrevScroll)
			RefreshTerminalMenu(ClientID);
		return NextScroll || PrevScroll || HookPress != 0;
	}

	if(NextScroll)
		pPlayer->m_TerminalMenuSelection = (pPlayer->m_TerminalMenuSelection + 1) % NumActions;
	else if(PrevScroll)
		pPlayer->m_TerminalMenuSelection = (pPlayer->m_TerminalMenuSelection - 1 + NumActions) % NumActions;

	if(FirePress)
	{
		m_TerminalMenu.ExecuteAction(this, pPlayer, pPlayer->m_TerminalMenuSelection);
		m_TerminalMenu.Populate(this, ClientID);
		if(pPlayer->m_TerminalMenuSelection >= m_TerminalMenu.NumActions())
			pPlayer->m_TerminalMenuSelection = maximum(0, m_TerminalMenu.NumActions() - 1);
		RefreshTerminalMenu(ClientID);
		return true;
	}

	if(NextScroll || PrevScroll)
	{
		RefreshTerminalMenu(ClientID);
		return true;
	}

	return false;
}

void CGameContext::DeployAircraft(int ClientID)
{
	if(ClientID < 0 || ClientID >= MAX_CLIENTS || !m_apPlayers[ClientID])
		return;

	if(Server()->m_LocateGame != LOCATE_GAME)
	{
		SendChatTarget(ClientID, _("只能在设施内部署飞行器"));
		return;
	}

	if(m_aAircraftStock[ClientID] <= 0)
	{
		SendChatTarget(ClientID, _("飞行器库存不足，请先在商店购买"));
		return;
	}

	CCharacter *pChr = GetPlayerChar(ClientID);
	if(!pChr || pChr->m_Freeze || pChr->m_InShip)
	{
		SendChatTarget(ClientID, _("当前无法部署飞行器"));
		return;
	}

	vec2 SpawnPos = pChr->m_Pos + vec2(0.f, -VehicleScale(40.f));
	if(VehicleSpotBlocked(&m_World, SpawnPos))
	{
		SendChatTarget(ClientID, _("附近空间不足，无法部署飞行器"));
		return;
	}

	m_aAircraftStock[ClientID]--;
	new CAircraft(&m_World, SpawnPos, m_apPlayers[ClientID]->GetTeam());
	SendChatTarget(ClientID, _("已部署飞行器（剩余库存 {int:stock}）"), "stock", &m_aAircraftStock[ClientID]);
	RefreshTerminalMenu(ClientID);
}

void CGameContext::ClearFacilityMarkers()
{
	for(int i = 0; i < m_apHazardMarkers.size(); i++)
	{
		if(m_apHazardMarkers[i])
			m_apHazardMarkers[i]->Reset();
	}
	m_apHazardMarkers.clear();
	m_aFacilityMarkers.clear();
}

void CGameContext::BuildFacilityMarkers()
{
	ClearFacilityMarkers();

	CCollision *pCol = Collision();
	if(!pCol)
		return;

	const int MaxMarkers = 200;
	int MarkerCount = 0;

	for(int y = 1; y < pCol->GetHeight() - 1; y++)
	{
		for(int x = 1; x < pCol->GetWidth() - 1; x++)
		{
			int Reserved = pCol->GetTileReserved(x, y);
			vec2 Center = vec2((float)(x * 32 + 16), (float)(y * 32 + 16));

			if(LcIsFacilityRoomReserved(Reserved))
			{
				SLcFacilityMarker Mark;
				Mark.m_Pos = Center;
				Mark.m_Type = Reserved;
				m_aFacilityMarkers.add(Mark);
				m_apHazardMarkers.add(new CHazardMarker(&m_World, Center, Reserved, 28.f));
				continue;
			}

			if(!LcIsHazardReserved(Reserved))
				continue;
			if(Reserved == LC_HAZARD_GRASS)
				continue;
			if(Reserved == LC_HAZARD_GAS || Reserved == LC_HAZARD_TAR)
			{
				if(x > 0 && pCol->GetTileReserved(x - 1, y) == Reserved)
					continue;
				if(y > 0 && pCol->GetTileReserved(x, y - 1) == Reserved)
					continue;
			}
			if(MarkerCount >= MaxMarkers)
				continue;

			float MarkerSize = 14.f;
			if(Reserved == LC_HAZARD_GAS)
			{
				MarkerSize = 40.f;
				if(x + 1 < pCol->GetWidth() && pCol->GetTileReserved(x + 1, y) == Reserved)
					Center.x += 16.f;
				if(y + 1 < pCol->GetHeight() && pCol->GetTileReserved(x, y + 1) == Reserved)
					Center.y += 16.f;
			}
			else if(Reserved == LC_HAZARD_TAR)
				MarkerSize = 28.f;

			m_apHazardMarkers.add(new CHazardMarker(&m_World, Center, Reserved, MarkerSize));
			MarkerCount++;
		}
	}

	if(m_aFacilityMarkers.size() > 0)
	{
		int RoomCount = m_aFacilityMarkers.size();
		SendChatTarget(-1, _("设施内发现 {int:count} 处固定房间（电池房/保险丝间/发电机室）"), "count", &RoomCount);
	}
}

void CGameContext::ScanFacility(int ClientID)
{
	CCharacter *pChr = GetPlayerChar(ClientID);
	if(!pChr)
		return;

	vec2 Pos = pChr->m_Pos;
	float BestDist = 1e30f;
	const char *pBestName = 0;
	vec2 BestPos = vec2(0, 0);
	ivec2 BestGrid(0, 0);
	bool Found = false;

	for(int i = 0; i < (int)m_aFacilityMarkers.size(); i++)
	{
		float Dist = distance(Pos, m_aFacilityMarkers[i].m_Pos);
		if(Dist < BestDist)
		{
			BestDist = Dist;
			pBestName = LcFacilityRoomNameZh(m_aFacilityMarkers[i].m_Type);
			BestPos = m_aFacilityMarkers[i].m_Pos;
			BestGrid = ivec2((int)(m_aFacilityMarkers[i].m_Pos.x/32), (int)(m_aFacilityMarkers[i].m_Pos.y/32));
			Found = true;
		}
	}

	for(CEntity *pEnt = m_World.FindFirst(CGameWorld::ENTTYPE_SCRAP); pEnt; pEnt = pEnt->TypeNext())
	{
		float Dist = distance(Pos, pEnt->m_Pos);
		if(Dist < BestDist)
		{
			BestDist = Dist;
			CScrap *pScrap = (CScrap *)pEnt;
			pBestName = ScrapInfo()->GetScrapName(pScrap->GetScrapType());
			BestPos = pEnt->m_Pos;
			BestGrid = ivec2((int)(pEnt->m_Pos.x/32), (int)(pEnt->m_Pos.y/32));
			Found = true;
		}
	}

	for(int i = 0; i < MAX_MONSTERS; i++)
	{
		if(!m_apMonsters[i])
			continue;
		float Dist = distance(Pos, m_apMonsters[i]->m_Pos);
		if(Dist < BestDist)
		{
			BestDist = Dist;
			pBestName = m_apMonsters[i]->MonsterName();
			BestPos = m_apMonsters[i]->m_Pos;
			BestGrid = ivec2((int)(m_apMonsters[i]->m_Pos.x/32), (int)(m_apMonsters[i]->m_Pos.y/32));
			Found = true;
		}
	}

	if(!Found)
	{
		if(m_pController->m_pShip)
		{
			vec2 ShipPos = m_pController->m_pShip->m_Pos;
			ivec2 ShipGrid((int)(ShipPos.x/32), (int)(ShipPos.y/32));
			SendChatTarget(ClientID, _("扫描仪：着陆飞船在 [x:{int:x}, y:{int:y}]"), "x", &ShipGrid.x, "y", &ShipGrid.y);
			CreateScanLink(Pos, ShipPos);
			return;
		}
		SendChatTarget(ClientID, _("扫描仪：附近无目标"));
		return;
	}

	char aLandmark[64];
	if(m_pController && m_pController->m_pShip)
	{
		for(int i = 0; i < (int)m_aFacilityMarkers.size(); i++)
		{
			if(distance(BestPos, m_aFacilityMarkers[i].m_Pos) < 64.f)
			{
				LcFormatLandmarkLabel(aLandmark, sizeof(aLandmark), this, BestPos, m_pController->m_pShip->m_Pos, m_aFacilityMarkers[i].m_Type);
				SendChatTarget(-1, _("扫描仪：{str:landmark} [{int:x},{int:y}]"), "landmark", aLandmark, "x", &BestGrid.x, "y", &BestGrid.y);
				SendBroadcast(ClientID, BROADCAST_PRIORITY_EFFECTSTATE, Server()->TickSpeed() * 2, _("【扫描】固定房间常有高价值废品，也更容易遭遇危险，请谨慎探索。"));
				CreateScanLink(Pos, BestPos);
				if(m_pController->m_pShip)
				{
					ivec2 ShipGrid((int)(m_pController->m_pShip->m_Pos.x/32), (int)(m_pController->m_pShip->m_Pos.y/32));
					SendChatTarget(ClientID, _("扫描仪：着陆飞船在 [x:{int:x}, y:{int:y}]"), "x", &ShipGrid.x, "y", &ShipGrid.y);
				}
				return;
			}
		}
	}

	SendChatTarget(-1, _("扫描仪：{str:name} 在 [x:{int:x}, y:{int:y}]"), "name", pBestName, "x", &BestGrid.x, "y", &BestGrid.y);
	CreateScanLink(Pos, BestPos);
	if(m_pController->m_pShip)
	{
		ivec2 ShipGrid((int)(m_pController->m_pShip->m_Pos.x/32), (int)(m_pController->m_pShip->m_Pos.y/32));
		SendChatTarget(ClientID, _("扫描仪：着陆飞船在 [x:{int:x}, y:{int:y}]"), "x", &ShipGrid.x, "y", &ShipGrid.y);
	}
}

bool CGameContext::ShouldPersistPlayer(int ClientID) const
{
	(void)ClientID;
	if(Server()->m_LocateGame != LOCATE_GAME || !m_pController)
		return false;
	return m_pController->ExpeditionPhase() == LC_PHASE_EXPEDITION;
}

void CGameContext::PersistPlayer(int ClientID)
{
	CPlayer *pP = m_apPlayers[ClientID];
	CCharacter *pChr = GetPlayerChar(ClientID);
	if(!pP || !pChr)
		return;

	CLcPlayerPersist &P = m_aPlayerPersist[ClientID];
	P.Reset();
	P.m_Active = true;
	P.m_RoundId = m_pController->m_RoundId;
	P.m_X = pChr->m_Pos.x;
	P.m_Y = pChr->m_Pos.y;
	P.m_Freeze = pChr->m_Freeze;
	P.m_Hand = pP->m_Hand;
	P.m_LeekTick = pChr->m_LeekTick;
	P.m_DisconnectTick = Server()->Tick();
	for(int i = 0; i < pP->m_vScraps.size(); i++)
	{
		if(pP->m_vScraps[i])
			P.m_vScraps.add(*pP->m_vScraps[i]);
	}
	pP->m_vScraps.clear();
}

void CGameContext::RestorePersistedPlayer(int ClientID)
{
	CLcPlayerPersist &P = m_aPlayerPersist[ClientID];
	if(!P.m_Active || !m_pController)
		return;

	const int Timeout = Server()->TickSpeed() * GC_DISCONNECT_PERSIST_SEC;
	if(Server()->Tick() - P.m_DisconnectTick > Timeout || P.m_RoundId != m_pController->m_RoundId)
	{
		P.Reset();
		return;
	}

	IServer::CClientSession *pSession = Server()->GetClientSession(ClientID);
	pSession->m_X = P.m_X;
	pSession->m_Y = P.m_Y;
	pSession->m_Freeze = P.m_Freeze;
	pSession->m_RoundId = P.m_RoundId;
}

void CGameContext::FinishPersistedRestore(int ClientID)
{
	CLcPlayerPersist &P = m_aPlayerPersist[ClientID];
	if(!P.m_Active)
		return;

	CPlayer *pP = m_apPlayers[ClientID];
	CCharacter *pChr = GetPlayerChar(ClientID);
	if(!pP || !pChr)
		return;

	pP->m_Hand = P.m_Hand;
	pChr->m_LeekTick = P.m_LeekTick;
	for(int i = 0; i < P.m_vScraps.size(); i++)
	{
		Scrap *pCopy = new Scrap(P.m_vScraps[i]);
		pP->m_vScraps.add(pCopy);
	}
	SendChatTarget(ClientID, _("你已重新连入远征，背包与位置已恢复"));
	P.Reset();
}

void CGameContext::StunMonstersInRadius(vec2 Pos, float Radius, int Ticks)
{
	for(int i = 0; i < MAX_MONSTERS; i++)
	{
		CMonster *pMonster = m_apMonsters[i];
		if(!pMonster)
			continue;
		if(distance(pMonster->m_Pos, Pos) <= Radius)
			pMonster->Stun(Ticks);
	}
}

bool CGameContext::PlayerCanDamageMonster(const CMonster *pMonster) const
{
	if(!pMonster || Server()->m_LocateGame != LOCATE_GAME)
		return false;
	return pMonster->IsAttackable();
}

int CGameContext::ScaledMonsterDamage(int Dmg, const CMonster *pMonster) const
{
	if(Dmg > 0 && pMonster && pMonster->m_Freeze)
		Dmg = Dmg * GC_FROZEN_MONSTER_DAMAGE_PERCENT / 100;
	return Dmg;
}

void CGameContext::DamageMonsterFromPlayer(CMonster *pMonster, int FromClient, int Weapon, int Dmg, vec2 Force)
{
	if(!PlayerCanDamageMonster(pMonster))
		return;
	Dmg = ScaledMonsterDamage(Dmg, pMonster);
	if(Dmg > 0)
		pMonster->TakeDamage(Force, Dmg, FromClient, Weapon);
}

void CGameContext::DamageMonstersInRadius(vec2 Pos, int FromClient, int Weapon, float Radius, float InnerRadius, int MaxDmg)
{
	for(int i = 0; i < MAX_MONSTERS; i++)
	{
		CMonster *pMonster = m_apMonsters[i];
		if(!PlayerCanDamageMonster(pMonster))
			continue;

		vec2 Diff = pMonster->m_Pos - Pos;
		float Len = length(Diff);
		if(Len > Radius)
			continue;

		vec2 ForceDir(0, 1);
		if(Len > 0.0f)
			ForceDir = normalize(Diff);
		float Falloff = 1 - clamp((Len - InnerRadius) / (Radius - InnerRadius), 0.0f, 1.0f);
		int Dmg = (int)(MaxDmg * Falloff);
		if(Dmg > 0)
			DamageMonsterFromPlayer(pMonster, FromClient, Weapon, Dmg, ForceDir * (float)Dmg * 2.0f);
	}
}

void CGameContext::ScanMonsters(int ClientID)
{
	int Found = 0;
	for(int i = 0; i < MAX_MONSTERS; i++)
	{
		CMonster *pMonster = m_apMonsters[i];
		if(!pMonster)
			continue;
		Found++;
		ivec2 P = ivec2((int)(pMonster->m_Pos.x / 32), (int)(pMonster->m_Pos.y / 32));
		SendChatTarget(ClientID, _("扩音器回声: {str:name} @ [{int:x},{int:y}]"), "name", pMonster->MonsterName(), "x", &P.x, "y", &P.y);
	}
	if(!Found)
		SendChatTarget(ClientID, _("扩音器只听到风声... 附近没有怪物"));
}


// Monster

CMonster *CGameContext::GetValidMonster(int MonsterID) const
{
    if(MonsterID >= MAX_MONSTERS || MonsterID < 0)
        return 0;

    if(!m_apMonsters[MonsterID])
        return 0;

    return m_apMonsters[MonsterID];
}

bool CGameContext::IsValidPlayer(int PlayerID)
{
    if(PlayerID >= MAX_CLIENTS || PlayerID < 0)
        return false;

    if(!m_apPlayers[PlayerID])
        return false;

    return true;
}

void CGameContext::NewMonster(int Type, bool Boss)
{
	int Health = Boss ? GC_BOSS_HEALTH : 1;
	int Armor = Boss ? 4 : 1;
	for(int i = 0; i < MAX_MONSTERS; i ++)
	{
	    if(!m_apMonsters[i])
	    {
	        m_apMonsters[i] = new CMonster(&m_World, Type, i, Health, Armor, Boss);
			if(Boss)
			{
				m_apMonsters[i]->m_BossType = Type;
				SendBroadcast(-1, BROADCAST_PRIORITY_GAMEANNOUNCE, Server()->TickSpeed() * 2, _("!!! 警告: 检测到高威胁目标 ({lstr:name}) !!!"), "name", LcMonsterName(Type));
			}
			break;
	    }
	}
}

void CGameContext::HandleMonsterSpawn()
{
	for (int i = 0; i < NUM_MONSTER_TYPES; i++)
	{
		if(m_NeedSpawnTick[i] > 0)
			m_NeedSpawnTick[i]--;
		else
		{
			NewMonster(i);
			m_NeedSpawnTick[i] = BalanceMonsterSpawnInterval(Server()->TickSpeed(), m_CountAlive);
		}
	}
}

void CGameContext::Count()
{
	m_CountAlive = 0;
	m_CountInGame = 0;
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if(!GetPlayerChar(i))
			continue;

		m_CountInGame++;

		if(GetPlayerChar(i)->m_Freeze)
			continue;
		
		m_CountAlive++;
	}
}

void CGameContext::OnMonsterDeath(int MonsterID)
{
    if(!GetValidMonster(MonsterID))
        return;

    m_apMonsters[MonsterID]->Destroy();

    delete m_apMonsters[MonsterID];
    m_apMonsters[MonsterID] = 0;
}