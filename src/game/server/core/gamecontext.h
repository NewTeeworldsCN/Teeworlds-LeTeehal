/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_GAMECONTEXT_H
#define GAME_SERVER_GAMECONTEXT_H

#include <engine/server.h>
#include <engine/console.h>
#include <engine/shared/memheap.h>

#include <teeuniverses/components/localization.h>

#include <game/layers.h>
#include <game/voting.h>

#include "eventhandler.h"
#include "gamecontroller.h"
#include "gameworld.h"
#include "player.h"

#include "../scrap/scrap_info.h"
#include "../lc/expedition/balance.h"

#include <engine/storage.h> // MapGen

#include "../lc/mapgen/mapgen.h"
#include "../lc/economy/player_persist.h"
#include "../lc/economy/store_bonus.h"
#include "../lc/ui/terminal_menu.h"
#include "../lc/ui/vote_menu.h"
#include "../lc/ui/gameplay_ui.h"
#include "../lc/economy/company_stats.h"

#include "../entities/lc/monster.h"

/*
	Tick
		Game Context (CGameContext::tick)
			Game World (GAMEWORLD::tick)
				Reset world if requested (GAMEWORLD::reset)
				All entities in the world (ENTITY::tick)
				All entities in the world (ENTITY::tick_defered)
				Remove entities marked for deletion (GAMEWORLD::remove_entities)
			Game Controller (GAMECONTROLLER::tick)
			All players (CPlayer::tick)


	Snap
		Game Context (CGameContext::snap)
			Game World (GAMEWORLD::snap)
				All entities in the world (ENTITY::snap)
			Game Controller (GAMECONTROLLER::snap)
			Events handler (EVENT_HANDLER::snap)
			All players (CPlayer::snap)

*/

#define BROADCAST_DURATION_REALTIME (0)
#define BROADCAST_DURATION_GAMEANNOUNCE (Server()->TickSpeed() * 2)

enum
{
	BROADCAST_PRIORITY_LOWEST = 0,
	BROADCAST_PRIORITY_WEAPONSTATE,
	BROADCAST_PRIORITY_EFFECTSTATE,
	BROADCAST_PRIORITY_GAMEANNOUNCE,
	BROADCAST_PRIORITY_SERVERANNOUNCE,
	BROADCAST_PRIORITY_INTERFACE,
};

class CGameContext : public IGameServer
{
	IServer *m_pServer;
	class IConsole *m_pConsole;
	CLayers m_Layers;
	CCollision m_Collision;
	CNetObjHandler m_NetObjHandler;
	CTuningParams m_Tuning;
	class CScrapInfo *m_pScrapInfo;
	CMapGen m_MapGen;
	IStorage *m_pStorage;

	static void ConsoleOutputCallback_Chat(const char *pStr, void *pUser);

	static void ConLanguage(IConsole::IResult *pResult, void *pUserData);
	static void ConAbout(IConsole::IResult *pResult, void *pUserData);
	static void ConTuneParam(IConsole::IResult *pResult, void *pUserData);
	static void ConTuneReset(IConsole::IResult *pResult, void *pUserData);
	static void ConTuneDump(IConsole::IResult *pResult, void *pUserData);
	static void ConPause(IConsole::IResult *pResult, void *pUserData);
	static void ConChangeMap(IConsole::IResult *pResult, void *pUserData);
	static void ConRestart(IConsole::IResult *pResult, void *pUserData);
	static void ConBroadcast(IConsole::IResult *pResult, void *pUserData);
	static void ConSay(IConsole::IResult *pResult, void *pUserData);
	static void ConSetTeam(IConsole::IResult *pResult, void *pUserData);
	static void ConSetTeamAll(IConsole::IResult *pResult, void *pUserData);
	static void ConSwapTeams(IConsole::IResult *pResult, void *pUserData);
	static void ConShuffleTeams(IConsole::IResult *pResult, void *pUserData);
	static void ConLockTeams(IConsole::IResult *pResult, void *pUserData);
	static void ConAddVote(IConsole::IResult *pResult, void *pUserData);
	static void ConRemoveVote(IConsole::IResult *pResult, void *pUserData);
	static void ConForceVote(IConsole::IResult *pResult, void *pUserData);
	static void ConClearVotes(IConsole::IResult *pResult, void *pUserData);
	static void ConVote(IConsole::IResult *pResult, void *pUserData);
	static void ConchainSpecialMotdupdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);

	static void ConHelp(IConsole::IResult *pResult, void *pUserData);
	static void ConGameStatus(IConsole::IResult *pResult, void *pUserData);
	static void ConGcStatus(IConsole::IResult *pResult, void *pUserData);
	static void ConGcSetQuota(IConsole::IResult *pResult, void *pUserData);
	static void ConGcSetMoney(IConsole::IResult *pResult, void *pUserData);
	static void ConMapGenNow(IConsole::IResult *pResult, void *pUserData);
	static void ConSpawnMonster(IConsole::IResult *pResult, void *pUserData);

	CGameContext(int Resetting);
	void Construct(int Resetting);

	bool m_Resetting;
	
	int m_ConsoleOutputHandle_ChatPrint;
	int m_ConsoleOutput_Target;
	
public:
	IServer *Server() const { return m_pServer; }
	class IConsole *Console() { return m_pConsole; }
	CCollision *Collision() { return &m_Collision; }
	CTuningParams *Tuning() { return &m_Tuning; }
	class CScrapInfo *ScrapInfo() { return m_pScrapInfo; }
	CLayers *Layers() { return &m_Layers; }
	IStorage *Storage() const { return m_pStorage; }
	CMapGen *MapGen() { return &m_MapGen; }

	CGameContext();
	~CGameContext();

	void Clear();

	CEventHandler m_Events;
	CPlayer *m_apPlayers[MAX_CLIENTS];

	CGameController *m_pController;
	CGameWorld m_World;

	// helper functions
	class CCharacter *GetPlayerChar(int ClientID);

	int m_LockTeams;

	// voting
	void StartVote(const char *pDesc, const char *pCommand, const char *pReason);
	void EndVote();
	void SendVoteSet(int ClientID);
	void SendVoteStatus(int ClientID, int Total, int Yes, int No);
	void AbortVoteKickOnDisconnect(int ClientID);

	int m_VoteCreator;
	int64 m_VoteCloseTime;
	bool m_VoteUpdate;
	int m_VotePos;
	char m_aVoteDescription[VOTE_DESC_LENGTH];
	char m_aVoteCommand[VOTE_CMD_LENGTH];
	char m_aVoteReason[VOTE_REASON_LENGTH];
	int m_NumVoteOptions;
	int m_VoteEnforce;
	enum
	{
		VOTE_ENFORCE_UNKNOWN=0,
		VOTE_ENFORCE_NO,
		VOTE_ENFORCE_YES,
	};
	CHeap *m_pVoteOptionHeap;
	CVoteOptionServer *m_pVoteOptionFirst;
	CVoteOptionServer *m_pVoteOptionLast;

	// helper functions
	void CreateDamageInd(vec2 Pos, float AngleMod, int Amount);
	void CreateExplosion(vec2 Pos, int Owner, int Weapon, bool NoDamage);
	void CreateHammerHit(vec2 Pos);
	void CreatePlayerSpawn(vec2 Pos);
	void CreateDeath(vec2 Pos, int Who);
	void CreateSound(vec2 Pos, int Sound, int Mask=-1);
	void CreateSoundGlobal(int Sound, int Target=-1);


	enum
	{
		CHAT_ALL=-2,
		CHAT_SPEC=-1,
		CHAT_RED=0,
		CHAT_BLUE=1
	};

	// network
	void SendChatTarget(int To, const char *pText, ...);
	void SendChat(int ClientID, int Team, const char *pText);
	void SendEmoticon(int ClientID, int Emoticon);
	void SendWeaponPickup(int ClientID, int Weapon);
	void SendBroadcast(int ClientID, int Priority, int LifeSpan, const char *pText, ...);
	void SetClientLanguage(int ClientID, const char *pLanguage);

	void AddBroadcast(int ClientID, const char *pText, int Priority, int LifeSpan);



	//
	void CheckPureTuning();
	void SendTuningParams(int ClientID);

	//
	void SwapTeams();

	// engine events
	virtual void OnInit();
	virtual void OnConsoleInit();
	virtual void OnShutdown();

	virtual void OnTick();
	virtual void OnPreSnap();
	virtual void OnSnap(int ClientID);
	virtual void OnPostSnap();

	virtual void OnMessage(int MsgID, CUnpacker *pUnpacker, int ClientID);

	virtual void OnClientConnected(int ClientID);
	virtual void OnClientEnter(int ClientID);
	virtual void OnClientDrop(int ClientID, const char *pReason);
	virtual void OnClientDirectInput(int ClientID, void *pInput);
	virtual void OnClientPredictedInput(int ClientID, void *pInput);

	virtual bool IsClientReady(int ClientID);
	virtual bool IsClientPlayer(int ClientID);

	virtual void OnSetAuthed(int ClientID,int Level);
	
	virtual const char *GameType();
	virtual const char *Version();
	virtual const char *NetVersion();

	struct CVoteOptions
	{
		char m_aDescription[VOTE_DESC_LENGTH] = {0};
		char m_aCommand[VOTE_CMD_LENGTH] = {0};
	};
	array<CVoteOptions> m_aPlayerVotes[MAX_CLIENTS];

	void ResetVotes(int ClientID);
	void AddVote(int To, const char *aCmd, const char *pText, ...);

	class CBroadcastState
	{
	public:
		int m_NoChangeTick;
		char m_aPrevMessage[1024];

		int m_Priority;
		char m_aNextMessage[1024];

		int m_LifeSpanTick;
		int m_TimedPriority;
		char m_aTimedMessage[1024];
	};
	CBroadcastState m_aBroadcastStates[MAX_PLAYER];

	int m_VoteStart;
	int GetNeedVoteStart()
	{
		int Majority = max(1, (m_CountInGame + 1) / 2);
		int Formula = ((int)((m_CountInGame/3)*2)) + 1;
		return max(Majority, Formula);
	}

	// MapGen
	virtual void SaveMap(const char *path);

	void GenTheMap();
	void ProcessMapGen();
	void CancelMapGenJob();
	bool m_MapGenPending;
	bool m_MapGenActive;
	bool m_MapGenApplying;
	bool m_MapGenFailed;
	int m_LastMapGenSeed;
	int m_MapGenRetryLeft;
	int m_MapGenLoadingBroadcastTick;
	int m_LastRoundBossBonus;
	SLcPlayerRoundStats m_aRoundStats[MAX_CLIENTS];
	SLcCycleStats m_CycleStats;
	SLcPlayerCareerStats m_aCareerStats[MAX_CLIENTS];
	int m_aStorePurchaseCount[MAX_CLIENTS];
	int m_TotalStoreSpend;
	int m_MapGenProgressStage;
	int m_LastQuotaMilestonePct;
	struct SMapGenWorkerJob *m_pMapGenJob;

	int m_NextExpeditionTimeBonusSec;
	int m_aNextArmorBonus[MAX_CLIENTS];
	int m_aNextFlashBonus[MAX_CLIENTS];
	CLcStoreBonus m_aStoreBonus[MAX_CLIENTS];
	int m_aAircraftStock[MAX_CLIENTS];
	CLcTerminalMenu m_TerminalMenu;

	void ToggleTerminalMenu(int ClientID);
	void OpenTerminalMenu(int ClientID);
	void CloseTerminalMenu(int ClientID);
	void RefreshTerminalMenu(int ClientID);
	bool HandleTerminalMenuInput(int ClientID, const CNetObj_PlayerInput *pInput, const CNetObj_PlayerInput *pPrevInput);
	void TerminalMenuGoBack(int ClientID);
	void DeployAircraft(int ClientID);
	void OpenTerminalGuidePage(int ClientID, int Page);
	bool ExecutePlayerVoteCommand(int ClientID, const char *pCmd, const char *pReason);

	void BuildFacilityMarkers();
	void ClearFacilityMarkers();
	bool m_FacilityMarkersBuilt;

	struct SLcFacilityMarker
	{
		vec2 m_Pos;
		int m_Type;
	};
	array<SLcFacilityMarker> m_aFacilityMarkers;
	array<class CHazardMarker*> m_apHazardMarkers;

	void TryBuyStoreItem(int ClientID, const char *pItem);
	void ScanFacility(int ClientID);
	void CreateScanLink(vec2 From, vec2 To);
	void GivePlayerScrap(int ClientID, int ScrapType, int Value, int Weight);
	void DepositScrapInShip(vec2 Pos, const Scrap &Item);
	void CompactShipScrap(vec2 Center, float Radius = GC_SHIP_SCRAP_MERGE_RADIUS);
	void CreditShipScrapDeposit(int ClientID, int Value);
	bool HasPendingExpeditionBonus(int ClientID) const;
	void ApplyExpeditionBonuses(int ClientID);

	void PersistPlayer(int ClientID);
	void RestorePersistedPlayer(int ClientID);
	void FinishPersistedRestore(int ClientID);
	bool ShouldPersistPlayer(int ClientID) const;
	void StunMonstersInRadius(vec2 Pos, float Radius, int Ticks);
	void ScanMonsters(int ClientID);
	bool PlayerCanDamageMonster(const class CMonster *pMonster) const;
	int ScaledMonsterDamage(int Dmg, const class CMonster *pMonster) const;
	void DamageMonsterFromPlayer(class CMonster *pMonster, int FromClient, int Weapon, int Dmg, vec2 Force);
	void DamageMonstersInRadius(vec2 Pos, int FromClient, int Weapon, float Radius, float InnerRadius, int MaxDmg);

	CLcPlayerPersist m_aPlayerPersist[MAX_CLIENTS];

	// Monster Neox
	CMonster *m_apMonsters[MAX_MONSTERS];
    CMonster *GetValidMonster(int MonsterID) const;
	void OnMonsterDeath(int MonsterID);
	bool IsValidPlayer(int PlayerID);

	void NewMonster(int Type, bool Boss = false);
	void HandleMonsterSpawn();
	int m_NeedSpawnTick[NUM_MONSTER_TYPES];

	int m_CountInGame;
	int m_CountAlive;

	void Count();
};

inline int CmaskAll() { return -1; }
inline int CmaskOne(int ClientID) { return 1<<ClientID; }
inline int CmaskAllExceptOne(int ClientID) { return 0x7fffffff^CmaskOne(ClientID); }
inline bool CmaskIsSet(int Mask, int ClientID) { return (Mask&CmaskOne(ClientID)) != 0; }
#endif
