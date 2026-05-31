/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* Copyright � 2013 Neox.                                                                                                */
/* If you are missing that file, acquire a complete release at https://www.teeworlds.com/forum/viewtopic.php?pid=106934  */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef GAME_SERVER_ENTITIES_MONSTER_H
#define GAME_SERVER_ENTITIES_MONSTER_H

#include <game/server/core/entity.h>
#include <game/generated/protocol.h>
#include <engine/shared/protocol.h>
#include <game/gamecore.h>

enum
{
    TYPE_PULLHANDLE = 0,
	TYPE_SATIETY,
	TYPE_LEEK_BOX,
	TYPE_BUG,
	TYPE_FEAR,
	TYPE_HUNTER,
	TYPE_BOMBER,
	TYPE_LEECH,
	TYPE_STALKER,
	NUM_MONSTER_TYPES,
};

enum EMobilityType
{
	MOBILITY_GROUND = 0,
	MOBILITY_HOOK,
	MOBILITY_FLY,
	MOBILITY_WALL,
};

class CCharacter;

class CMonster : public CEntity
{
public:
	//monster's size
	static const int ms_PhysSize = 28;

	static int SnapClientID(int MonsterID) { return MAX_CLIENTS - MAX_MONSTERS + MonsterID; }

	CMonster(CGameWorld *pWorld, int Type, int MonsterID, int Health, int Armor, bool Boss = false);

	virtual void Reset();
	virtual void Tick();
	virtual void TickPaused();
	virtual void Snap(int SnappingClient);

	bool IsGrounded();

	void HandleNinja(bool IsPredicted = false);

	void FireWeapon();

	void Die(int Killer);
	bool TakeDamage(vec2 Force, int Dmg, int From, int Weapon, bool FromMonster = false, bool Drain = true, bool FromReflect = false);

	bool IncreaseHealth(int Amount);
	bool IncreaseArmor(int Amount);

	vec2 GetPos() const { return m_Pos; }
	void Destroy();
	void SetPerfectDir();
	void HandleWeapons();
	void Move();
	void HandleCore();
	void HandleActions();
	void Jump();
	bool CanJump();
	void ChangeDir();
	void Spawn();
	const char *MonsterName();
	const char *MonsterDesc();
	const char *MonsterDescShort();
	void OnPredictedNinja();
	void Stun(int Ticks);
	bool IsAttackable() const { return !(m_Type == TYPE_BUG && m_Hidden); }
	int MonsterType() const { return m_Type; }

	void HandleMobility(CEntity *pVict);
	bool NeedsMobilityAssist();
	bool FindHookAnchor(vec2 Target, vec2 *pOutDir);
	EMobilityType MobilityTypeFor(int Type) const;
    
    int GetLifes() { return m_Armor + m_Health; }
	
	int m_DieTick;
	bool m_Freeze;
	int m_FreezeUntilTick;
	bool m_Hidden;
	int m_GrassAmbushTick;
	int m_StareAccum;
	int m_LastCoilSoundTick;
	int m_LastCoilStareBroadcastTick[MAX_CLIENTS];
	int m_LastFireTick;
	int m_LastHoardChatTick;
	bool m_Exploded;
	bool m_Boss;
	int m_BossType;
	bool m_BrackenTelegraphSent;
	bool m_StalkerCharging;
	bool m_StalkerChargeMelee;
	int m_StalkerChargeUntilTick;
	int m_LastStalkerChargeSoundTick;
	vec2 m_StalkerChargeDir;

private:
	CCharacter *ClosestPlayer(vec2 Pos, float Radius);
	CCharacter *HighestValuePlayer(vec2 Pos, float Radius);
	bool IsSeenByPlayer(CCharacter *pChr);
	bool IsSeenByAnyPlayer();
	void HandleCoilheadStareBroadcast();
	void ApplyShipBarrier();
	void StartStalkerCharge(CCharacter *pChr, float Dist, float MeleeReach);
	void TickStalkerCharge();
	void FinishStalkerCharge();
	void CancelStalkerCharge();

	CEntity *m_apHitObjects[10];
	int m_NumObjectsHit;
	int m_ActiveWeapon;
	int m_ReloadTimer;
	int m_DamageTaken;

	int m_DamageTakenTick;

	int m_Health;
	int m_Armor;
	int m_MaxArmor;
	int m_MaxHealth;

	// ninja
	struct
	{
		vec2 m_ActivationDir;
		int m_ActivationTick;
		int m_CurrentMoveTime;
		int m_OldVelAmount;
	} m_Ninja;

	int m_Type;
	int m_MonsterID;
	EMobilityType m_Mobility;
	bool m_MobilityHookActive;
	bool m_MobilityFlyActive;
	bool m_MobilityWallActive;
	vec2 m_MobilityHookDir;

    bool m_WillFire;
    bool m_WillJump;
    bool m_WillHook;

    vec2 m_Dir;

    struct
    {
        vec2 m_Vel;

        vec2 m_HookPos;
        vec2 m_HookDir;
        int m_HookTick;
        int m_HookState;
        int m_HookedPlayer;

        int m_Jumped;
    }m_Core;

	struct
	{
        int m_Direction;
        int m_ActualDirection;

        bool m_CollideRight;
        bool m_CollideLeft;

        int m_LastJumpTick;

        vec2 m_LastPos;
        int m_StuckTick;
	}m_Path;
};

#endif
