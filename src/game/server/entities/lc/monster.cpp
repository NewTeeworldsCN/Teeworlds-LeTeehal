/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* Copyright � 2013 Neox.                                                                                                */
/* If you are missing that file, acquire a complete release at https://www.teeworlds.com/forum/viewtopic.php?pid=106934  */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <new>
#include <engine/shared/config.h>
#include <game/server/lc/expedition/balance.h>
#include <game/server/lc/hazards/hazards.h>
#include <game/server/lc/ui/guide.h>
#include <game/server/lc/ui/terminal_actions.h>
#include <game/server/lc/economy/company_stats.h>
#include <game/server/core/gamecontext.h>
#include <game/mapitems.h>

#include "../core/laser.h"
#include "../core/projectile.h"
#include "../core/character.h"
#include "monster.h"

CMonster::CMonster(CGameWorld *pWorld, int Type, int MonsterID, int Health, int Armor, bool Boss)
: CEntity(pWorld, CGameWorld::ENTTYPE_MONSTER)
{
    m_MonsterID = MonsterID;
    m_Type = Type;
    m_Boss = Boss;
    m_BossType = Type;
	m_ProximityRadius = Boss ? ms_PhysSize + 10 : ms_PhysSize;
    m_MaxArmor = Armor;
	m_Armor = Armor;
    m_Health = 10 + Health;
	m_MaxHealth = 10 + Health;
    m_DieTick = -1;
    m_Freeze = false;
    m_FreezeUntilTick = 0;
    m_StareAccum = 0;
    m_LastCoilSoundTick = 0;
    mem_zero(m_LastCoilStareBroadcastTick, sizeof(m_LastCoilStareBroadcastTick));
    m_LastFireTick = 0;
    m_LastHoardChatTick = 0;
    m_Exploded = false;
    m_Hidden = false;
    m_BrackenTelegraphSent = false;
    m_GrassAmbushTick = 0;
    mem_zero(&m_Ninja, sizeof(m_Ninja));
    m_Mobility = MobilityTypeFor(Type);
    m_MobilityHookActive = false;
    m_MobilityFlyActive = false;
    m_MobilityWallActive = false;
    m_MobilityHookDir = vec2(0, 0);

	Spawn();

    m_ActiveWeapon = WEAPON_HAMMER;
    switch(Type)
    {
    case TYPE_SATIETY: m_ActiveWeapon = WEAPON_GUN; break;
    case TYPE_LEEK_BOX: m_ActiveWeapon = WEAPON_NINJA; break;
    case TYPE_BUG: m_ActiveWeapon = WEAPON_SHOTGUN; break;
    case TYPE_FEAR: m_ActiveWeapon = WEAPON_RIFLE; break;
    case TYPE_HUNTER: m_ActiveWeapon = WEAPON_GUN; break;
    case TYPE_BOMBER: m_ActiveWeapon = WEAPON_GRENADE; break;
    case TYPE_LEECH: m_ActiveWeapon = WEAPON_HAMMER; break;
    case TYPE_STALKER: m_ActiveWeapon = WEAPON_NINJA; break;
    default: break;
    }

    if(Type == TYPE_STALKER)
    {
        m_Health = 8;
        m_MaxHealth = 8;
        m_Armor = 0;
        m_MaxArmor = 0;
    }

    if(Type == TYPE_SATIETY)
        Die(60*Server()->TickSpeed());

	GameWorld()->InsertEntity(this);
}

void CMonster::Reset()
{
	GameServer()->OnMonsterDeath(m_MonsterID);
}

void CMonster::Destroy()
{
    GameWorld()->DestroyEntity(this);
}

void CMonster::Spawn()
{
	if(!GameServer()->m_pController->m_aMonsterSpawnPos.size())
		return;
	m_Pos = vec2(0, 0);
	m_Core.m_Vel = vec2(0, 0);
	m_Core.m_HookPos = vec2(0, 0);
	m_Core.m_HookDir = vec2(0, 0);
	m_Core.m_HookTick = 0;
	m_Core.m_HookState = HOOK_IDLE;
	m_Core.m_HookedPlayer = -1;
	m_Core.m_Jumped = 0;
	m_Path.m_ActualDirection = rand()%2;
	if(!m_Path.m_ActualDirection)
        m_Path.m_ActualDirection = -1;

    if(m_Path.m_ActualDirection == 1)
    {
        m_Path.m_CollideLeft = true;
        m_Path.m_CollideRight = false;
    }
    else
    {
        m_Path.m_CollideLeft = false;
        m_Path.m_CollideRight = true;
    }

    float Distance = 0;
    int FurthestNum = -1;

    for(int i = 0; i < GameServer()->m_pController->m_aMonsterSpawnPos.size(); i ++)
    {
        CCharacter* pMonst = GameWorld()->ClosestCharacter(GameServer()->m_pController->m_aMonsterSpawnPos[i], 100000, this);
        if(pMonst)
        {
            float Distance2 = distance(GameServer()->m_pController->m_aMonsterSpawnPos[i], pMonst->m_Pos);
            if(Distance > Distance2)
            {
                Distance = Distance2;
                FurthestNum = i;
            }
        }
    }

    vec2 SpawnPos; // Can be replaced with m_Pos, but better to read

    if(FurthestNum == -1)
    {
        FurthestNum = GameServer()->m_pController->m_MonsterSpawnCurrentNum;
        GameServer()->m_pController->m_MonsterSpawnCurrentNum ++;
        GameServer()->m_pController->m_MonsterSpawnCurrentNum %= GameServer()->m_pController->m_aMonsterSpawnPos.size();
        bool Found = false;
        for(int i = 0; i < 4; i ++)
        {
            vec2 Pos = GameServer()->m_pController->m_aMonsterSpawnPos[FurthestNum]; // Done just because it's shorter to type
            switch(i)
            {
                case 0: Pos.x += 64.f; break;
                case 1: Pos.x -= 64.f; break;
                case 2: Pos.y += 64.f; break;
                case 3: Pos.y -= 64.f; break;
            }

            if(GameServer()->Collision()->CheckPoint(Pos))
                continue;

            CCharacter* pClosest = GameWorld()->ClosestCharacter(Pos, 500, this);
            if(pClosest)
            {
                Found = true;
                SpawnPos = Pos;
                break;
            }
        }
        if(!Found)
            SpawnPos = GameServer()->m_pController->m_aMonsterSpawnPos[FurthestNum];
    }
    else
        SpawnPos = GameServer()->m_pController->m_aMonsterSpawnPos[FurthestNum];

    m_Pos = SpawnPos;
}

bool CMonster::IsGrounded()
{
	if(GameServer()->Collision()->CheckPoint(m_Pos.x+m_ProximityRadius/2, m_Pos.y+m_ProximityRadius/2+5))
		return true;
	if(GameServer()->Collision()->CheckPoint(m_Pos.x-m_ProximityRadius/2, m_Pos.y+m_ProximityRadius/2+5))
		return true;
	return false;
}

EMobilityType CMonster::MobilityTypeFor(int Type) const
{
	switch(Type)
	{
	case TYPE_PULLHANDLE:
	case TYPE_LEECH:
	case TYPE_BUG:
		return MOBILITY_HOOK;
	case TYPE_HUNTER:
	case TYPE_SATIETY:
		return MOBILITY_FLY;
	case TYPE_FEAR:
	case TYPE_LEEK_BOX:
		return MOBILITY_WALL;
	default:
		return MOBILITY_GROUND;
	}
}

bool CMonster::NeedsMobilityAssist()
{
	CCollision *pCol = GameServer()->Collision();
	if(!pCol)
		return false;

	if(IsGrounded())
		return false;

	const float Tile = 32.f;
	const float MapBottom = (pCol->GetHeight() - GC_MONSTER_BOTTOM_MARGIN_TILES) * Tile;
	if(m_Pos.y > MapBottom)
		return true;

	if(m_Core.m_Vel.y > 120.f)
		return true;

	if(m_Path.m_StuckTick > Server()->TickSpeed())
		return true;

	if(!pCol->IntersectLine(m_Pos, m_Pos + vec2(0, Tile * GC_MONSTER_PIT_CHECK_TILES), 0x0, 0x0))
		return true;

	return false;
}

bool CMonster::FindHookAnchor(vec2 Target, vec2 *pOutDir)
{
	CCollision *pCol = GameServer()->Collision();
	if(!pCol || !pOutDir)
		return false;

	vec2 ToTarget = Target - m_Pos;
	if(length(ToTarget) < 1.f)
		ToTarget = vec2((float)m_Path.m_Direction, -1.f);
	else
		ToTarget = normalize(ToTarget);

	const float HookLen = GameServer()->m_World.m_Core.m_Tuning.m_HookLength;
	vec2 aDirs[6];
	int NumDirs = 0;
	aDirs[NumDirs++] = ToTarget;
	aDirs[NumDirs++] = vec2(0.f, -1.f);
	aDirs[NumDirs++] = normalize(vec2(ToTarget.x, -1.f));
	aDirs[NumDirs++] = normalize(vec2((float)m_Path.m_Direction, -1.f));
	aDirs[NumDirs++] = normalize(vec2(-ToTarget.x, -0.75f));
	aDirs[NumDirs++] = normalize(vec2(ToTarget.x, -0.75f));

	for(int i = 0; i < NumDirs; i++)
	{
		if(length(aDirs[i]) < 0.01f)
			continue;

		vec2 End = m_Pos + aDirs[i] * HookLen;
		vec2 HitPos;
		int Hit = pCol->IntersectLine(m_Pos, End, &HitPos, 0x0);
		if(!Hit || (Hit & CCollision::COLFLAG_NOHOOK))
			continue;

		vec2 Dir = HitPos - m_Pos;
		if(length(Dir) < 48.f)
			continue;

		*pOutDir = normalize(Dir);
		return true;
	}

	return false;
}

void CMonster::HandleMobility(CEntity *pVict)
{
	if(m_Freeze)
		return;
	if(m_Type == TYPE_FEAR && IsSeenByAnyPlayer())
		return;
	if(m_Type == TYPE_LEEK_BOX)
		return;

	CCollision *pCol = GameServer()->Collision();
	if(!pCol)
		return;

	vec2 Target = pVict ? pVict->m_Pos : m_Pos + vec2((float)m_Path.m_Direction * 400.f, -200.f);
	const bool Assist = NeedsMobilityAssist();
	const float MapBottom = (pCol->GetHeight() - GC_MONSTER_BOTTOM_MARGIN_TILES) * 32.f;

	if(m_Pos.y > MapBottom)
	{
		vec2 RescueDir;
		if(FindHookAnchor(m_Pos + vec2(0.f, -600.f), &RescueDir))
		{
			m_MobilityHookDir = RescueDir;
			m_MobilityHookActive = true;
			m_WillHook = true;
		}
		else
		{
			Spawn();
			return;
		}
	}

	switch(m_Mobility)
	{
	case MOBILITY_HOOK:
		if(Assist || (pVict && distance(m_Pos, pVict->m_Pos) > 320.f))
		{
			vec2 HookDir;
			if(FindHookAnchor(Target, &HookDir))
			{
				m_MobilityHookDir = HookDir;
				m_MobilityHookActive = true;
				m_WillHook = true;
			}
		}
		break;
	case MOBILITY_FLY:
		m_MobilityFlyActive = Assist || pVict != 0;
		break;
	case MOBILITY_WALL:
		if(Assist || !IsGrounded())
			m_MobilityWallActive = true;
		break;
	default:
		break;
	}
}


void CMonster::HandleNinja(bool IsPredicted)
{
	m_Ninja.m_CurrentMoveTime--;

	if (m_Ninja.m_CurrentMoveTime == 0)
	{
		// reset velocity
		m_Core.m_Vel = m_Ninja.m_ActivationDir*m_Ninja.m_OldVelAmount;
	}

	if (m_Ninja.m_CurrentMoveTime > 0)
	{
		// Set velocity
		m_Core.m_Vel = m_Ninja.m_ActivationDir * g_pData->m_Weapons.m_Ninja.m_Velocity;
		vec2 OldPos = m_Pos;
		GameServer()->Collision()->MoveBox(&m_Pos, &m_Core.m_Vel, vec2(m_ProximityRadius, m_ProximityRadius), 0.f);

		// reset velocity so the client doesn't predict stuff
		m_Core.m_Vel = vec2(0.f, 0.f);

		if(IsPredicted)
            return;

		// check if we Hit anything along the way
		{
			CCharacter *aEnts[MAX_CLIENTS];
			vec2 Dir = m_Pos - OldPos;
			float Radius = m_ProximityRadius * 2.0f;
			vec2 Center = OldPos + Dir * 0.5f;
			int Num = GameServer()->m_World.FindEntities(Center, Radius, (CEntity**)aEnts, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);

			for (int i = 0; i < Num; ++i)
			{
				if(aEnts[i]->m_Freeze || aEnts[i]->m_InShip)
					continue;
				// make sure we haven't Hit this object before
				bool bAlreadyHit = false;
				for (int j = 0; j < m_NumObjectsHit; j++)
				{
					if (m_apHitObjects[j] == aEnts[i])
						bAlreadyHit = true;
				}
				if (bAlreadyHit)
					continue;

				// check so we are sufficiently close
				if (distance(aEnts[i]->m_Pos, m_Pos) > (m_ProximityRadius * 2.0f))
					continue;

				// Hit a player, give him damage and stuffs...
				GameServer()->CreateSound(aEnts[i]->m_Pos, SOUND_NINJA_HIT);
				// set his velocity to fast upward (for now)
				if(m_NumObjectsHit < 10)
					m_apHitObjects[m_NumObjectsHit++] = aEnts[i];

                int Damage = g_pData->m_Weapons.m_Ninja.m_pBase->m_Damage;

				aEnts[i]->TakeDamage(vec2(0, -10.0f), Damage, aEnts[i]->GetPlayer()->GetCID(), m_ActiveWeapon);
			}

			CMonster *aMonsts[MAX_MONSTERS];
			int Num2 = GameServer()->m_World.FindEntities(Center, Radius, (CEntity**)aMonsts, MAX_MONSTERS, CGameWorld::ENTTYPE_MONSTER);

			for (int i = 0; i < Num2; ++i)
			{
			    if(aMonsts[i]->m_MonsterID == m_MonsterID)
                    continue;

				// make sure we haven't Hit this object before
				bool bAlreadyHit = false;
				for (int j = 0; j < m_NumObjectsHit; j++)
				{
					if (m_apHitObjects[j] == aMonsts[i])
						bAlreadyHit = true;
				}
				if (bAlreadyHit)
					continue;

				// check so we are sufficiently close
				if (distance(aMonsts[i]->m_Pos, m_Pos) > (m_ProximityRadius * 2.0f))
					continue;

				// Hit a player, give him damage and stuffs...
				GameServer()->CreateSound(aMonsts[i]->m_Pos, SOUND_NINJA_HIT);
				// set his velocity to fast upward (for now)
				if(m_NumObjectsHit < 10)
					m_apHitObjects[m_NumObjectsHit++] = aMonsts[i];

				aMonsts[i]->TakeDamage(vec2(0, -10.0f), g_pData->m_Weapons.m_Ninja.m_pBase->m_Damage, m_MonsterID, m_ActiveWeapon, true);
			}
		}
	}
}

void CMonster::Move()
{
    if(m_Freeze || (m_Type == TYPE_FEAR && IsSeenByAnyPlayer()))
    {
        m_Core.m_Vel = vec2(0,0);
        return;
    }
	float RampValue = VelocityRamp(length(m_Core.m_Vel)*50, GameServer()->m_World.m_Core.m_Tuning.m_VelrampStart, GameServer()->m_World.m_Core.m_Tuning.m_VelrampRange, GameServer()->m_World.m_Core.m_Tuning.m_VelrampCurvature);

	m_Core.m_Vel.x = m_Core.m_Vel.x*RampValue;

	GameServer()->Collision()->MoveBox(&m_Pos, &m_Core.m_Vel, vec2(m_ProximityRadius, m_ProximityRadius), 0);

	m_Core.m_Vel.x = m_Core.m_Vel.x*(1.0f/RampValue);
}

void CMonster::HandleCore()
{
	float PhysSize = m_ProximityRadius;

	// get ground state
	bool Grounded = IsGrounded();

	CEntity *pVict = 0;
	vec2 TargetDirection = vec2(0, 0);
	if(m_Type == TYPE_SATIETY)
		pVict = HighestValuePlayer(m_Pos, 10000.f);
	else if(m_Type == TYPE_FEAR)
		pVict = ClosestPlayer(m_Pos, 10000.f);
	else if(m_Type == TYPE_HUNTER || m_Type == TYPE_BOMBER || m_Type == TYPE_LEECH || m_Type == TYPE_STALKER)
		pVict = ClosestPlayer(m_Pos, 10000.f);
	else
		pVict = GameWorld()->ClosestCharacter(m_Pos, 10000, 0x0, false);

	if(m_Type == TYPE_FEAR && IsSeenByAnyPlayer())
	{
		m_WillJump = false;
		m_Core.m_Vel = vec2(0, 0);
		m_Core.m_HookState = HOOK_IDLE;
		m_Core.m_HookPos = m_Pos;
		m_Core.m_HookedPlayer = -1;
		return;
	}

	if(m_Type == TYPE_FEAR && pVict)
		TargetDirection = normalize(pVict->m_Pos - m_Pos);
	else if(pVict)
		TargetDirection = normalize(pVict->m_Pos - m_Pos);
	else if(m_MobilityHookActive && length(m_MobilityHookDir) > 0.01f)
		TargetDirection = normalize(m_MobilityHookDir);

	float MaxSpeed = Grounded ? GameServer()->m_World.m_Core.m_Tuning.m_GroundControlSpeed : GameServer()->m_World.m_Core.m_Tuning.m_AirControlSpeed;
	if(m_Type == TYPE_BOMBER)
		MaxSpeed *= 0.65f;
	if(m_Type == TYPE_STALKER)
		MaxSpeed *= 1.5f;
	if(m_Type == TYPE_FEAR)
		MaxSpeed *= GC_COILHEAD_SPEED_PERCENT / 100.f;
	float Accel = Grounded ? GameServer()->m_World.m_Core.m_Tuning.m_GroundControlAccel : GameServer()->m_World.m_Core.m_Tuning.m_AirControlAccel;
	float Friction = Grounded ? GameServer()->m_World.m_Core.m_Tuning.m_GroundFriction : GameServer()->m_World.m_Core.m_Tuning.m_AirFriction;

	m_Core.m_Vel.y += GameServer()->m_World.m_Core.m_Tuning.m_Gravity;

	if(m_MobilityFlyActive)
	{
		m_Core.m_Vel.y -= GameServer()->m_World.m_Core.m_Tuning.m_Gravity * (GC_MONSTER_FLY_GRAVITY_CANCEL_PERCENT / 100.f);
		if(pVict)
		{
			vec2 FlyDir = normalize(pVict->m_Pos - m_Pos);
			m_Core.m_Vel.x = SaturatedAdd(-MaxSpeed * 1.2f, MaxSpeed * 1.2f, m_Core.m_Vel.x, FlyDir.x * Accel * 0.6f);
			m_Core.m_Vel.y = SaturatedAdd(-MaxSpeed * 1.2f, MaxSpeed * 1.2f, m_Core.m_Vel.y, FlyDir.y * Accel * 0.45f);
		}
		else if(NeedsMobilityAssist())
			m_Core.m_Vel.y -= 250.f;
	}

	if(m_MobilityWallActive && !IsGrounded())
	{
		const float WallDist = (float)m_ProximityRadius + 4.f;
		const bool WallLeft = GameServer()->Collision()->CheckPoint(m_Pos + vec2(-WallDist, 0.f));
		const bool WallRight = GameServer()->Collision()->CheckPoint(m_Pos + vec2(WallDist, 0.f));
		int CrawlSpeed = GC_MONSTER_WALL_CRAWL_SPEED;
		int ClimbSpeed = GC_MONSTER_WALL_CLIMB_SPEED;
		if(m_Type == TYPE_FEAR)
		{
			CrawlSpeed = GC_COILHEAD_WALL_CRAWL_SPEED;
			ClimbSpeed = GC_COILHEAD_WALL_CLIMB_SPEED;
		}
		if(WallLeft || WallRight)
		{
			m_Core.m_Vel.y = minimum(m_Core.m_Vel.y, (float)-ClimbSpeed);
			if(WallLeft)
			{
				m_Core.m_Vel.x = maximum(m_Core.m_Vel.x, (float)CrawlSpeed);
				m_Path.m_Direction = 1;
			}
			if(WallRight)
			{
				m_Core.m_Vel.x = minimum(m_Core.m_Vel.x, (float)-CrawlSpeed);
				m_Path.m_Direction = -1;
			}
		}
	}

   // handle jump
    if(m_WillJump)
    {
        if(m_Core.m_Jumped < 2)
        {
            if(Grounded)
            {
                m_Core.m_Vel.y = -GameServer()->m_World.m_Core.m_Tuning.m_GroundJumpImpulse;
                m_Core.m_Jumped = 1;
                GameServer()->CreateSound(m_Pos, SOUND_PLAYER_JUMP);
            }
            else
            {
                m_Core.m_Vel.y = -GameServer()->m_World.m_Core.m_Tuning.m_AirJumpImpulse;
                m_Core.m_Jumped = 2;
                GameServer()->CreateSound(m_Pos, SOUND_PLAYER_AIRJUMP);
            }
        }
    }
    else
    {
        if(Grounded)
            m_Core.m_Jumped = 0;
        else if(m_Core.m_Jumped < 1)
            m_Core.m_Jumped = 1;
    }

    // handle hook
    if(m_WillHook || m_MobilityHookActive)
    {
        if(m_Core.m_HookState == HOOK_IDLE)
        {
            m_Core.m_HookState = HOOK_FLYING;
            m_Core.m_HookPos = m_Pos+TargetDirection*PhysSize*1.5f;
            m_Core.m_HookDir = TargetDirection;
            m_Core.m_HookedPlayer = -1;
            m_Core.m_HookTick = 0;
        }
    }
    else if(m_Core.m_HookedPlayer != -1 || m_Core.m_HookState == HOOK_IDLE)
    {
        m_Core.m_HookedPlayer = -1;
        m_Core.m_HookState = HOOK_IDLE;
        m_Core.m_HookPos = m_Pos;
    }

	// add the speed modification according to players wanted direction
	if(m_Path.m_Direction < 0)
		m_Core.m_Vel.x = SaturatedAdd(-MaxSpeed, MaxSpeed, m_Core.m_Vel.x, -Accel);
	if(m_Path.m_Direction > 0)
		m_Core.m_Vel.x = SaturatedAdd(-MaxSpeed, MaxSpeed, m_Core.m_Vel.x, Accel);
	if(m_Path.m_Direction == 0)
		m_Core.m_Vel.x *= Friction;

	// handle jumping
	// 1 bit = to keep track if a jump has been made on this input
	// 2 bit = to keep track if a air-jump has been made
	if(Grounded)
		m_Core.m_Jumped &= ~2;

	// do hook
	if(m_Core.m_HookState == HOOK_IDLE)
	{
		m_Core.m_HookedPlayer = -1;
		m_Core.m_HookState = HOOK_IDLE;
		m_Core.m_HookPos = m_Pos;
	}
	else if(m_Core.m_HookState >= HOOK_RETRACT_START && m_Core.m_HookState < HOOK_RETRACT_END)
	{
		m_Core.m_HookState++;
	}
	else if(m_Core.m_HookState == HOOK_RETRACT_END)
	{
		m_Core.m_HookState = HOOK_RETRACTED;
	}
	else if(m_Core.m_HookState == HOOK_FLYING)
	{
		vec2 NewPos = m_Core.m_HookPos+m_Core.m_HookDir*GameServer()->m_World.m_Core.m_Tuning.m_HookFireSpeed;
		if(distance(m_Pos, NewPos) > GameServer()->m_World.m_Core.m_Tuning.m_HookLength)
		{
			m_Core.m_HookState = HOOK_RETRACT_START;
			NewPos = m_Pos + normalize(NewPos-m_Pos) * GameServer()->m_World.m_Core.m_Tuning.m_HookLength;
		}

		// make sure that the hook doesn't go though the ground
		bool GoingToHitGround = false;
		bool GoingToRetract = false;
		int Hit = GameServer()->Collision()->IntersectLine(m_Core.m_HookPos, NewPos, &NewPos, 0);
		if(Hit)
		{
			if(Hit&CCollision::COLFLAG_NOHOOK)
				GoingToRetract = true;
			else
				GoingToHitGround = true;
		}

		// Check against other players first
        float Distance = 0.0f;
        for(int i = 0; i < MAX_CLIENTS; i++)
        {
            CCharacter *pChar = GameServer()->GetPlayerChar(i);
            if(!pChar)
                continue;

            vec2 ClosestPoint = closest_point_on_line(m_Core.m_HookPos, NewPos, pChar->m_Pos);
            if(distance(pChar->m_Pos, ClosestPoint) < PhysSize+2.0f)
            {
                if (m_Core.m_HookedPlayer == -1 || distance(m_Core.m_HookPos, pChar->m_Pos) < Distance)
                {
                    m_Core.m_HookState = HOOK_GRABBED;
                    m_Core.m_HookedPlayer = i;
                    Distance = distance(m_Core.m_HookPos, pChar->m_Pos);
                    GameServer()->CreateSound(m_Pos, SOUND_HOOK_ATTACH_PLAYER);
                }
            }
        }

		if(m_Core.m_HookState == HOOK_FLYING)
		{
			// check against ground
			if(GoingToHitGround)
				m_Core.m_HookState = HOOK_GRABBED;
			else if(GoingToRetract)
				m_Core.m_HookState = HOOK_RETRACT_START;

			m_Core.m_HookPos = NewPos;
		}
	}

	if(m_Core.m_HookState == HOOK_GRABBED)
	{
		if(m_Core.m_HookedPlayer != -1)
		{
			CCharacter *pChar = GameServer()->GetPlayerChar(m_Core.m_HookedPlayer);
			if(pChar)
				m_Core.m_HookPos = pChar->m_Pos;
			else
			{
				// release hook
				m_Core.m_HookedPlayer = -1;
				m_Core.m_HookState = HOOK_RETRACTED;
				m_Core.m_HookPos = m_Pos;
			}

			// keep players hooked for a max of 1.5sec
			//if(Server()->Tick() > hook_tick+(Server()->TickSpeed()*3)/2)
				//release_hooked();
		}
		else
            m_WillHook = false;

		// don't do this hook rutine when we are hook to a player
		if(m_Core.m_HookedPlayer == -1 && distance(m_Core.m_HookPos, m_Pos) > 46.0f)
		{
			vec2 HookVel = normalize(m_Core.m_HookPos-m_Pos)*GameServer()->m_World.m_Core.m_Tuning.m_HookDragAccel;
			// the hook as more power to drag you up then down.
			// this makes it easier to get on top of an platform
			if(HookVel.y > 0)
				HookVel.y *= 0.3f;
			else if(m_MobilityHookActive)
				HookVel.y *= 1.35f;

			// the hook will boost it's power if the player wants to move
			// in that direction. otherwise it will dampen everything abit
			if((HookVel.x < 0 && m_Path.m_Direction < 0) || (HookVel.x > 0 && m_Path.m_Direction > 0))
				HookVel.x *= 0.95f;
			else
				HookVel.x *= 0.75f;

			vec2 NewVel = m_Core.m_Vel+HookVel;

			// check if we are under the legal limit for the hook
			if(length(NewVel) < GameServer()->m_World.m_Core.m_Tuning.m_HookDragSpeed || length(NewVel) < length(m_Core.m_Vel))
				m_Core.m_Vel = NewVel; // no problem. apply

		}

		// release hook (max hook time is 1.25
		m_Core.m_HookTick++;
		if(m_Core.m_HookedPlayer != -1 && (m_Core.m_HookTick > Server()->TickSpeed()+Server()->TickSpeed()/5 || !GameServer()->GetPlayerChar(m_Core.m_HookedPlayer)))
		{
			m_Core.m_HookedPlayer = -1;
			m_Core.m_HookState = HOOK_RETRACTED;
			m_Core.m_HookPos = m_Pos;
		}
		else if(m_Core.m_HookedPlayer == -1 && m_Core.m_HookTick > Server()->TickSpeed() * 2)
		{
			m_Core.m_HookState = HOOK_RETRACT_START;
			m_MobilityHookActive = false;
			m_MobilityHookDir = vec2(0, 0);
		}
	}

	if(m_Core.m_HookState == HOOK_RETRACTED)
        m_Core.m_HookState = HOOK_IDLE;

    CCharacter *pChar = GameServer()->GetPlayerChar(m_Core.m_HookedPlayer);

    if(pChar)
    {
        float Distance = distance(m_Pos, pChar->m_Pos);
        vec2 Dir = normalize(m_Pos - pChar->m_Pos);

        if(Distance > PhysSize*1.50f) // TODO: fix tweakable variable
        {
            float Accel = GameServer()->m_World.m_Core.m_Tuning.m_HookDragAccel * (Distance/GameServer()->m_World.m_Core.m_Tuning.m_HookLength);
            float DragSpeed = GameServer()->m_World.m_Core.m_Tuning.m_HookDragSpeed;

            // add force to the hooked player
            pChar->m_Core.m_Vel.x = SaturatedAdd(-DragSpeed, DragSpeed, pChar->m_Core.m_Vel.x, Accel*Dir.x*1.5f);
            pChar->m_Core.m_Vel.y = SaturatedAdd(-DragSpeed, DragSpeed, pChar->m_Core.m_Vel.y, Accel*Dir.y*1.5f);

            // add a little bit force to the guy who has the grip
            m_Core.m_Vel.x = SaturatedAdd(-DragSpeed, DragSpeed, m_Core.m_Vel.x, -Accel*Dir.x*0.25f);
            m_Core.m_Vel.y = SaturatedAdd(-DragSpeed, DragSpeed, m_Core.m_Vel.y, -Accel*Dir.y*0.25f);
        }
    }

	// clamp the velocity to something sane
	if(length(m_Core.m_Vel) > 6000)
		m_Core.m_Vel = normalize(m_Core.m_Vel) * 6000;

    if(m_Core.m_HookState == HOOK_GRABBED && m_Core.m_HookedPlayer == -1)
    {
        if(!m_MobilityHookActive || IsGrounded())
        {
            m_Core.m_HookState = HOOK_IDLE;
            m_Core.m_HookPos = m_Pos;
            m_MobilityHookActive = false;
            m_MobilityHookDir = vec2(0, 0);
        }
    }

    if(IsGrounded())
    {
        m_MobilityHookActive = false;
        m_MobilityFlyActive = false;
        m_MobilityWallActive = false;
        m_MobilityHookDir = vec2(0, 0);
    }
}

void CMonster::HandleWeapons()
{

}

void CMonster::OnPredictedNinja() // Ninja's smart part =)
{
}

void CMonster::FireWeapon()
{
}

void CMonster::Tick()
{
	if(m_FreezeUntilTick > 0 && Server()->Tick() >= m_FreezeUntilTick)
	{
		m_FreezeUntilTick = 0;
		m_Freeze = false;
	}

	if(m_Health <= 0)
	{
		if(m_DieTick < 0)
			Die(1);
	}
	else if(m_Ninja.m_CurrentMoveTime > 0)
	{
		HandleNinja(false);
	}
	else if(m_Type == TYPE_FEAR && IsSeenByAnyPlayer())
	{
		m_WillJump = false;
		m_WillHook = false;
		m_Core.m_Vel = vec2(0, 0);
		m_Core.m_HookState = HOOK_IDLE;
		m_Core.m_HookPos = m_Pos;
		m_Core.m_HookedPlayer = -1;
		m_Path.m_Direction = 0;
		m_Path.m_ActualDirection = 0;
		if(IsGrounded())
			m_Core.m_Jumped = 0;
		HandleCoilheadStareBroadcast();
	}
	else
	{
		HandleActions();
		HandleWeapons();
		HandleCore();
		Move();
	}

    if(m_DieTick > 0)
    {
        m_DieTick--;
        if(m_DieTick == 0)
            Reset();
    }
}

void CMonster::HandleActions() // This is the monsters AI, it has been decreased because if too many calculations, it creates hard lags/crashes
{
    if(m_Freeze)
    {
        m_Core.m_Vel = vec2(0, 0); // this works like "Hey you are lose connection" hahahahhaha
        m_WillJump = false;
        return;
    }

    if(m_Type == TYPE_BUG)
    {
        m_Hidden = GameServer()->Collision()->GetHazardAt(m_Pos) == LC_HAZARD_GRASS;
    }
 
    if(m_Path.m_LastPos.x == m_Pos.x && m_Path.m_LastPos.y == m_Pos.y) // This is done because monsters are SOMETIMES stuck on a corner and can't move because of it
    {
        m_Path.m_StuckTick ++;
        if(m_Path.m_StuckTick >= Server()->TickSpeed() * 3)
        {
            ChangeDir();
            m_Path.m_StuckTick = 0;
        }
    }
    else
        m_Path.m_StuckTick = 0;

    m_Path.m_LastPos = m_Pos;

	const int DIRECTION_RIGHT = 1;
	const int DIRECTION_LEFT = -1;
	const int DIRECTION_ZERO = 0;
	const int TILES_TO_CHECK = 7;
	const int LINES_TO_CHECK = 30;
	const int JUMP_LIMIT = 11;
	//const int AIRJUMP_LIMIT = 6;
	const int TS = 32; // TS = TileSize, not TeamSpeak.
    const float MaxSpeedBase = IsGrounded() ? GameServer()->m_World.m_Core.m_Tuning.m_GroundControlSpeed : GameServer()->m_World.m_Core.m_Tuning.m_AirControlSpeed;
    float MaxSpeed = MaxSpeedBase;
    if(m_Type == TYPE_FEAR)
        MaxSpeed *= GC_COILHEAD_SPEED_PERCENT / 100.f;
    if(m_Type == TYPE_STALKER)
        MaxSpeed *= 1.5f;
    const float Accel = IsGrounded() ? GameServer()->m_World.m_Core.m_Tuning.m_GroundControlAccel : GameServer()->m_World.m_Core.m_Tuning.m_AirControlAccel;
    const float Friction = IsGrounded() ? GameServer()->m_World.m_Core.m_Tuning.m_GroundFriction : GameServer()->m_World.m_Core.m_Tuning.m_AirFriction;

	m_WillJump = false;

    CEntity *pVict;
    if(m_Type == TYPE_SATIETY)
        pVict = HighestValuePlayer(m_Pos, 10000.f);
    else if(m_Type == TYPE_LEEK_BOX)
    {
        pVict = GameServer()->m_World.ClosestCharacter(GetPos(), 240, 0x0, false);

        if(pVict)
        {
            if(((CCharacter *)pVict)->m_LeekTick <= 0 && !((CCharacter *)pVict)->m_InShip)
            {
                ((CCharacter *)pVict)->m_LeekTick = 15*Server()->TickSpeed();
                int CID = ((CCharacter *)pVict)->GetPlayer()->GetCID();
                GameServer()->SendBroadcast(CID, BROADCAST_PRIORITY_EFFECTSTATE, Server()->TickSpeed() * 2, _("[生命维持系统]请回到飞船！你已被感染韭菜盒子病毒！请保护公司财产！"));
                Die(15*Server()->TickSpeed());
                m_Freeze = true;
            }
            return;
        }
    }
    else if(m_Type == TYPE_FEAR)
    {
        if(IsSeenByAnyPlayer())
        {
            m_Path.m_Direction = 0;
            m_Path.m_ActualDirection = 0;
            m_WillJump = false;
            m_Core.m_Vel = vec2(0, 0);
            return;
        }

        pVict = ClosestPlayer(m_Pos, 10000.f);
        if(pVict)
        {
            CCharacter *pPlayer = (CCharacter *)pVict;
            float Dist = distance(m_Pos, pPlayer->m_Pos);

            if(Dist < (float)GC_COILHEAD_KILL_RANGE)
            {
                pPlayer->LCDie();
                GameServer()->CreateSound(m_Pos, SOUND_PLAYER_DIE, CmaskAll());
                GameServer()->SendChatTarget(pPlayer->GetPlayer()->GetCID(), _("弹簧头抓住了你！"));
                return;
            }

            if(Dist < (float)GC_COILHEAD_RANGE)
            {
                if(m_LastCoilSoundTick + Server()->TickSpeed() * GC_COILHEAD_SOUND_SEC <= Server()->Tick())
                {
                    GameServer()->CreateSound(m_Pos, SOUND_PLAYER_PAIN_LONG, CmaskAll());
                    m_LastCoilSoundTick = Server()->Tick();
                }
            }

            if(pPlayer->m_Pos.x > m_Pos.x + 8.f)
                m_Path.m_ActualDirection = DIRECTION_RIGHT;
            else if(pPlayer->m_Pos.x < m_Pos.x - 8.f)
                m_Path.m_ActualDirection = DIRECTION_LEFT;
            else
                m_Path.m_ActualDirection = DIRECTION_ZERO;
        }
    }
    else if(m_Type == TYPE_BOMBER)
    {
        pVict = ClosestPlayer(m_Pos, 10000.f);
        if(pVict && distance(m_Pos, pVict->m_Pos) < (float)GC_BOMBER_DETONATE_RANGE && !m_Exploded)
        {
            m_Exploded = true;
            GameServer()->CreateExplosion(m_Pos, -1, WEAPON_GRENADE, false);
            GameServer()->CreateSound(m_Pos, SOUND_GRENADE_EXPLODE, CmaskAll());
            Reset();
            return;
        }
    }
    else if(m_Type == TYPE_HUNTER)
        pVict = ClosestPlayer(m_Pos, 10000.f);
    else if(m_Type == TYPE_STALKER)
        pVict = ClosestPlayer(m_Pos, 10000.f);
    else if(m_Type == TYPE_LEECH)
        pVict = ClosestPlayer(m_Pos, (float)GC_LEECH_HOOK_RANGE);
    else if(m_Type == TYPE_BUG)
    {
        pVict = GameWorld()->ClosestCharacter(m_Pos, 10000, 0x0, false);
        if(pVict)
        {
            CCharacter *pChr = (CCharacter *)pVict;
            float Dist = distance(m_Pos, pChr->m_Pos);
            if(m_Hidden && !m_BrackenTelegraphSent && Dist < (float)(GC_BRACKEN_AMBUSH_RANGE + 120) && Dist >= (float)GC_BRACKEN_AMBUSH_RANGE)
            {
                m_BrackenTelegraphSent = true;
                GameServer()->SendChatTarget(pChr->GetPlayer()->GetCID(), _("前方草丛有异动…"));
            }
            if(m_Hidden && Dist < GC_BRACKEN_AMBUSH_RANGE)
            {
                m_Hidden = false;
                m_GrassAmbushTick = Server()->Tick() + Server()->TickSpeed() * 3;
                GameServer()->SendChatTarget(pChr->GetPlayer()->GetCID(), _("草丛中突然窜出蔓背怪！"));
            }
            if(m_Hidden)
                pVict = 0;
        }
    }
    else
        pVict = GameWorld()->ClosestCharacter(m_Pos, 10000, 0x0, false);

    if(m_Path.m_CollideRight)
        m_Path.m_ActualDirection = DIRECTION_LEFT;

    if(m_Path.m_CollideLeft)
        m_Path.m_ActualDirection = DIRECTION_RIGHT;

    for(int j = 0; j < 2; j ++)
    {
        if(GameServer()->Collision()->IntersectLine(m_Pos, vec2(m_Pos.x + TS * (j + 1), m_Pos.y), 0x0, 0x0)) // Check collision on the right
        {
            if(CanJump())
                Jump();
            vec2 At;
            bool FoundEmpty = GameServer()->Collision()->EmptyOnLine(vec2(m_Pos.x + TS, m_Pos.y), vec2(m_Pos.x + TS, m_Pos.y - TS * TILES_TO_CHECK), &At, 0x0);
            //if(!FoundEmpty) // This makes monsters going on the right and left, but not on the center, just try it by adding it back
                //FoundEmpty = GameServer()->Collision()->EmptyOnLine(vec2(m_Pos.x + TS, m_Pos.y), vec2(m_Pos.x + TS, m_Pos.y + TS * TILES_TO_CHECK), &At, 0x0);
            if(FoundEmpty && j == 1)
            {
                if(GameServer()->Collision()->CheckPoint(At + vec2(-TS, 0)))
                    FoundEmpty = false;
            }
            if(GameServer()->Collision()->IntersectLine(m_Pos, vec2(m_Pos.x + TS * (j + 1), m_Pos.y), 0x0, 0x0) && !FoundEmpty && m_Core.m_Jumped >= 2)
            {
                m_Path.m_CollideRight = true;
                m_Path.m_CollideLeft = false;
            }
            if(!FoundEmpty) // We don't need to check again
                break;
        }
        else if(GameServer()->Collision()->IntersectLine(m_Pos, vec2(m_Pos.x - TS * (j + 1), m_Pos.y), 0x0, 0x0)) // Check collision on the left
        {
            if(CanJump())
                Jump();
            vec2 At;
            bool FoundEmpty = GameServer()->Collision()->EmptyOnLine(vec2(m_Pos.x - TS, m_Pos.y), vec2(m_Pos.x - TS, m_Pos.y - TS * TILES_TO_CHECK), &At, 0x0);
            //if(!FoundEmpty) // This makes monsters going on the right and left, but not on the center, just try it by adding it back
                //FoundEmpty = GameServer()->Collision()->EmptyOnLine(vec2(m_Pos.x - TS, m_Pos.y), vec2(m_Pos.x - TS, m_Pos.y + TS * TILES_TO_CHECK), &At, 0x0);
            if(FoundEmpty && j == 1)
            {
                if(GameServer()->Collision()->CheckPoint(At + vec2(TS, 0)))
                    FoundEmpty = false;
            }
            if(GameServer()->Collision()->IntersectLine(m_Pos, vec2(m_Pos.x - TS * (j + 1), m_Pos.y), 0x0, 0x0) && !FoundEmpty && m_Core.m_Jumped >= 2)
            {
                m_Path.m_CollideRight = false;
                m_Path.m_CollideLeft = true;
            }
            if(!FoundEmpty) // We don't need to check again
                break;
        }
    }

    if(m_Path.m_CollideLeft)
    {
        if(!GameServer()->Collision()->CheckPoint(m_Pos + vec2(TS, TS))) // Check if the next tile under the monster is not a solid tile
        {
            bool KeepDirection = false;
            for(int i = 1; i < LINES_TO_CHECK+1; i ++)
            {
                // Check 30 * 30 tiles on the bottom right of the monster
                if(GameServer()->Collision()->IntersectLine(m_Pos + vec2(i * TS, 0), m_Pos + vec2(TS, LINES_TO_CHECK * TS), 0x0, 0x0))
                {
                    KeepDirection = true;
                    break;
                }
            }
            if(!KeepDirection)
                ChangeDir();
            else if(IsGrounded())
                Jump();
            else if(!GameServer()->Collision()->IntersectLine(m_Pos, m_Pos + vec2(0, TS * JUMP_LIMIT), 0x0, 0x0) && CanJump() && pVict)
                Jump();
        }

        // This is made because monsters are often stuck if "the top and the bottom are too close" and "the monster can't go to his direction (because of a wall)"
        if(m_Path.m_CollideLeft) // Check if the direction isn't changed
        {
            // Check collision on the top and right
            if(GameServer()->Collision()->CheckPoint(m_Pos + vec2(0, -TS)) && GameServer()->Collision()->CheckPoint(m_Pos + vec2(TS, 0)))
            {
                 // Check 11 tiles on the bottom (limit of a jump + airjump of a player)
                if(GameServer()->Collision()->IntersectLine(m_Pos, m_Pos + vec2(0, TS * 11), 0x0, 0x0)) // If found a solid tile, change the direction
                    ChangeDir();
            }
        }
    }
    else
    {
        if(!GameServer()->Collision()->CheckPoint(m_Pos + vec2(-TS, TS))) // Check if the next tile under the monster is not a solid tile
        {
            bool KeepDirection = false;
            for(int i = 1; i < LINES_TO_CHECK+1; i ++)
            {
                // Check 30 * 30 tiles on the bottom left of the monster
                if(GameServer()->Collision()->IntersectLine(m_Pos + vec2(i * -TS, 0), m_Pos + vec2(-TS, LINES_TO_CHECK * TS), 0x0, 0x0))
                {
                    KeepDirection = true;
                    break;
                }
            }
            if(!KeepDirection)
                ChangeDir();
            else if(IsGrounded())
                Jump();
            else if(!GameServer()->Collision()->IntersectLine(m_Pos, m_Pos + vec2(0, TS * JUMP_LIMIT), 0x0, 0x0) && CanJump() && pVict)
                Jump();
        }

        // This is made because monsters are often stuck if "the top and the bottom are too close" and "the monster can't go to his direction (because of a wall)"
        if(m_Path.m_CollideRight) // Check only if the direction isn't changed
        {
            // Check collision on the top and left
            if(GameServer()->Collision()->CheckPoint(m_Pos + vec2(0, -TS)) && GameServer()->Collision()->CheckPoint(m_Pos + vec2(-TS, 0)))
            {
                 // Check 11 tiles on the bottom (limit of a jump + airjump of a player)
                if(GameServer()->Collision()->IntersectLine(m_Pos, m_Pos + vec2(0, TS * JUMP_LIMIT), 0x0, 0x0)) // If found a solid tile, change the direction
                    ChangeDir();
            }
        }
    }

    if(pVict)
    {
        vec2 Diff = pVict->m_Pos - m_Pos;
        if(Diff.x >= 1)
        {
            m_Path.m_ActualDirection = DIRECTION_RIGHT;
            m_Path.m_CollideLeft = true;
            m_Path.m_CollideRight = false;
        }
        else if(Diff.x <= -1)
        {
            m_Path.m_ActualDirection = DIRECTION_LEFT;
            m_Path.m_CollideLeft = false;
            m_Path.m_CollideRight = true;
        }
        else
        {
            m_Path.m_ActualDirection = DIRECTION_ZERO;
            m_Path.m_CollideLeft = true;
            m_Path.m_CollideRight = false;
        }

        if(distance(pVict->m_Pos, m_Pos) < 800)
        {
            if(m_Type == TYPE_PULLHANDLE || m_Type == TYPE_BUG || m_Type == TYPE_LEECH)
            {
                Die(30*Server()->TickSpeed());
                m_WillHook = true;
                int WeightInterval = 25;
                if(m_Type == TYPE_BUG && GameServer()->Collision()->GetHazardAt(m_Pos) == LC_HAZARD_GRASS)
                    WeightInterval = 12;
                if(m_Type == TYPE_PULLHANDLE && Server()->Tick() % WeightInterval == 0)
                {
                    ((CCharacter *)pVict)->GetPlayer()->m_AddedWeight++;
                    GameServer()->ResetVotes(((CCharacter *)pVict)->GetPlayer()->GetCID());
                }
                if(m_Type == TYPE_LEECH && Server()->Tick() % 40 == 0 && distance(pVict->m_Pos, m_Pos) < (float)GC_LEECH_DRAIN_RANGE)
                {
                    CCharacter *pChr = (CCharacter *)pVict;
                    pChr->TakeDamage(vec2(0, 0.2f), 1, SnapClientID(m_MonsterID), WEAPON_HAMMER);
                    GameServer()->CreateSound(m_Pos, SOUND_PLAYER_PAIN_SHORT, CmaskAll());
                }
            }
            bool WillJump = CanJump();
            if(WillJump) // Here we check if when the monster will jump, it will still see the victim or not. If not, make like it doesn't jump
            {
                vec2 OldVel = m_Core.m_Vel; // Create a copy of the velocity

                if(IsGrounded())
                {
                    m_Core.m_Vel.y = -GameServer()->m_World.m_Core.m_Tuning.m_GroundJumpImpulse; // Simulate ground jumping
                    for(int PredictedTicks = 0; PredictedTicks < Server()->TickSpeed(); PredictedTicks ++) // Simulate ticks for 1 virtual second
                    {
                        if(PredictedTicks == Server()->TickSpeed()/2) // Simulate air jumping
                            m_Core.m_Vel.y = -GameServer()->m_World.m_Core.m_Tuning.m_AirJumpImpulse;

                        Move();

                        if(GameServer()->Collision()->IntersectLine(pVict->m_Pos, m_Pos, 0x0, 0x0)) // If the monster doesn't see the player after jumping
                        {
                            WillJump = false;
                            break;
                        }
                        m_Core.m_Vel.y += GameServer()->m_World.m_Core.m_Tuning.m_Gravity;
                        if(m_Path.m_ActualDirection < 0)
                            m_Core.m_Vel.x = SaturatedAdd(-MaxSpeed, MaxSpeed, m_Core.m_Vel.x, -Accel);
                        if(m_Path.m_ActualDirection > 0)
                            m_Core.m_Vel.x = SaturatedAdd(-MaxSpeed, MaxSpeed, m_Core.m_Vel.x, Accel);
                        if(m_Path.m_ActualDirection == 0)
                            m_Core.m_Vel.x *= Friction;
                    }
                }
                else
                {
                    m_Core.m_Vel.y = -GameServer()->m_World.m_Core.m_Tuning.m_AirJumpImpulse; // Simulate air jumping
                    for(int PredictedTicks = 0; PredictedTicks < Server()->TickSpeed()/2; PredictedTicks ++) // Simulate ticks for a half virtual second
                    {
                        Move();

                        if(GameServer()->Collision()->IntersectLine(pVict->m_Pos, m_Pos, 0x0, 0x0))
                        {
                            WillJump = false;
                            break;
                        }
                        m_Core.m_Vel.y += GameServer()->m_World.m_Core.m_Tuning.m_Gravity;
                        if(m_Path.m_ActualDirection < 0)
                            m_Core.m_Vel.x = SaturatedAdd(-MaxSpeed, MaxSpeed, m_Core.m_Vel.x, -Accel);
                        if(m_Path.m_ActualDirection > 0)
                            m_Core.m_Vel.x = SaturatedAdd(-MaxSpeed, MaxSpeed, m_Core.m_Vel.x, Accel);
                        if(m_Path.m_ActualDirection == 0)
                            m_Core.m_Vel.x *= Friction;
                    }
                }
                // The pos and the velocity have been modified, make like they aren't
                m_Pos = m_Path.m_LastPos;
                m_Core.m_Vel = OldVel;
            }
            if(WillJump)
                Jump();
        }
    }
    else
    {
        // Here we check if when the monster will jump, there will not be a collision between a player and it (so the monster has found a victim)
        bool WillJump = CanJump();
        if(WillJump)
        {
            vec2 OldVel = m_Core.m_Vel; // Create a copy of the velocity

            if(IsGrounded())
            {
                m_Core.m_Vel.y = -GameServer()->m_World.m_Core.m_Tuning.m_GroundJumpImpulse; // Simulate ground jumping
                for(int PredictedTicks = 0; PredictedTicks <= Server()->TickSpeed() * 2; PredictedTicks ++) // Simulate ticks for 2 virtual seconds
                {
                    if(PredictedTicks == Server()->TickSpeed()/2) // Simulate air jumping
                        m_Core.m_Vel.y = -GameServer()->m_World.m_Core.m_Tuning.m_AirJumpImpulse;

                    Move();

                    if(PredictedTicks % Server()->TickSpeed()/2 == 0) // Do not check always because of the lags
                    {
                        if(GameWorld()->ClosestCharacter(m_Pos, 10000, 0x0, false)) // If the monster will find a player
                        {
                            WillJump = true;
                            break;
                        }
                        else
                            WillJump = false;
                    }
                    m_Core.m_Vel.y += GameServer()->m_World.m_Core.m_Tuning.m_Gravity;
                    if(m_Path.m_ActualDirection < 0)
                        m_Core.m_Vel.x = SaturatedAdd(-MaxSpeed, MaxSpeed, m_Core.m_Vel.x, -Accel);
                    if(m_Path.m_ActualDirection > 0)
                        m_Core.m_Vel.x = SaturatedAdd(-MaxSpeed, MaxSpeed, m_Core.m_Vel.x, Accel);
                    if(m_Path.m_ActualDirection == 0)
                        m_Core.m_Vel.x *= Friction;
                }
            }
            else
            {
                m_Core.m_Vel.y = -GameServer()->m_World.m_Core.m_Tuning.m_AirJumpImpulse; // Simulate air jumping
                for(int PredictedTicks = 0; PredictedTicks <= Server()->TickSpeed(); PredictedTicks ++) // Simulate ticks 1 virtual second
                {
                    Move();

                    if(PredictedTicks % Server()->TickSpeed()/2 == 0) // Do not check always because of the lags
                    {
                        if(GameWorld()->ClosestCharacter(m_Pos, 10000, 0x0, false)) // If the monster will find a player
                        {
                            WillJump = true;
                            break;
                        }
                        else
                            WillJump = false;
                    }
                    m_Core.m_Vel.y += GameServer()->m_World.m_Core.m_Tuning.m_Gravity;
                    if(m_Path.m_ActualDirection < 0)
                        m_Core.m_Vel.x = SaturatedAdd(-MaxSpeed, MaxSpeed, m_Core.m_Vel.x, -Accel);
                    if(m_Path.m_ActualDirection > 0)
                        m_Core.m_Vel.x = SaturatedAdd(-MaxSpeed, MaxSpeed, m_Core.m_Vel.x, Accel);
                    if(m_Path.m_ActualDirection == 0)
                        m_Core.m_Vel.x *= Friction;
                }
            }
            // The pos and the velocity have been modified, make like they aren't
            m_Pos = m_Path.m_LastPos;
            m_Core.m_Vel = OldVel;
        }
        if(WillJump)
            Jump();
        m_WillHook = false;
    }

    if(pVict && m_Type == TYPE_SATIETY)
    {
        CCharacter *pChr = (CCharacter *)pVict;
        if(m_LastHoardChatTick + Server()->TickSpeed() * 8 <= Server()->Tick())
        {
            m_LastHoardChatTick = Server()->Tick();
            GameServer()->SendChatTarget(-1, _("囤积虫正在追 {str:name} 的高价值背包"), "name", Server()->ClientName(pChr->GetPlayer()->GetCID()));
        }
    }

    if(m_Type == TYPE_HUNTER && pVict)
    {
        CCharacter *pChr = (CCharacter *)pVict;
        float Dist = distance(m_Pos, pChr->m_Pos);
        if(Dist < (float)GC_HUNTER_FIRE_RANGE && !GameServer()->Collision()->IntersectLine(m_Pos, pChr->m_Pos, 0x0, 0x0))
        {
            if(m_LastFireTick + Server()->TickSpeed() * GC_HUNTER_FIRE_SEC <= Server()->Tick())
            {
                vec2 Dir = normalize(pChr->m_Pos - m_Pos);
                vec2 FirePos = m_Pos + Dir * (float)(m_ProximityRadius + 16);
                new CProjectile(GameWorld(), WEAPON_GUN,
                    SnapClientID(m_MonsterID),
                    FirePos,
                    Dir,
                    (int)(Server()->TickSpeed() * GameServer()->Tuning()->m_GunLifetime),
                    1, 0, 0, -1, WEAPON_GUN);
                GameServer()->CreateSound(m_Pos, SOUND_GUN_FIRE, CmaskAll());
                m_LastFireTick = Server()->Tick();
            }
        }
    }
    else if(m_Type == TYPE_STALKER && pVict && m_Ninja.m_CurrentMoveTime <= 0)
    {
        CCharacter *pChr = (CCharacter *)pVict;
        float Dist = distance(m_Pos, pChr->m_Pos);
        const float MeleeReach = m_ProximityRadius + pChr->m_ProximityRadius + 14.f;
        const float LungeReach = MeleeReach + (float)GC_STALKER_LUNGE_EXTRA;
        if(Dist < LungeReach &&
            m_LastFireTick + Server()->TickSpeed() * GC_STALKER_ATTACK_SEC <= Server()->Tick() &&
            !GameServer()->Collision()->IntersectLine(m_Pos, pChr->m_Pos, 0x0, 0x0))
        {
            vec2 Dir = pChr->m_Pos - m_Pos;
            if(length(Dir) < 0.01f)
                Dir = vec2(1.f, 0.f);
            else
                Dir = normalize(Dir);

            if(Dist <= MeleeReach)
            {
                m_NumObjectsHit = 0;
                GameServer()->CreateSound(m_Pos, SOUND_NINJA_FIRE);
                GameServer()->CreateSound(pChr->m_Pos, SOUND_NINJA_HIT);
                if(length(pChr->m_Pos - m_Pos) > 0.0f)
                    GameServer()->CreateHammerHit(pChr->m_Pos - Dir * m_ProximityRadius * 0.5f);
                else
                    GameServer()->CreateHammerHit(m_Pos);
                pChr->TakeDamage(vec2(0.f, -1.f) + normalize(Dir + vec2(0.f, -1.1f)) * 10.0f,
                    g_pData->m_Weapons.m_Ninja.m_pBase->m_Damage, SnapClientID(m_MonsterID), WEAPON_NINJA);
                m_LastFireTick = Server()->Tick();
            }
            else
            {
                m_NumObjectsHit = 0;
                m_Ninja.m_ActivationDir = Dir;
                m_Ninja.m_OldVelAmount = (int)length(m_Core.m_Vel);
                m_Ninja.m_CurrentMoveTime = g_pData->m_Weapons.m_Ninja.m_Movetime;
                GameServer()->CreateSound(m_Pos, SOUND_NINJA_FIRE);
                m_LastFireTick = Server()->Tick();
            }
        }
    }

    m_Path.m_Direction = m_Path.m_ActualDirection;
    HandleMobility(pVict);
}

void CMonster::TickPaused()
{
	++m_DamageTakenTick;
	++m_Ninja.m_ActivationTick;
}

void CMonster::Jump()
{
    const int Now = Server()->Tick();
    m_Path.m_LastJumpTick = Now;
    m_WillJump = true;
}

void CMonster::ChangeDir()
{
    m_Path.m_CollideRight = m_Path.m_CollideRight ? false : true;
    m_Path.m_CollideLeft = m_Path.m_CollideLeft ? false : true;
}

bool CMonster::CanJump()
{
    const int Now = Server()->Tick();
    const int WAIT_TICK = Server()->TickSpeed()/2;

    if(m_Path.m_LastJumpTick + WAIT_TICK < Now && m_Core.m_Jumped < 2)
        return true;

    return false;
}

bool CMonster::IncreaseHealth(int Amount)
{
	if(m_Health >= m_MaxHealth)
		return false;
	m_Health = clamp(m_Health+Amount, 0, m_MaxHealth);
	return true;
}

bool CMonster::IncreaseArmor(int Amount)
{
	if(m_Armor >= m_MaxArmor)
		return false;
	m_Armor = clamp(m_Armor+Amount, 0, m_MaxArmor);
	return true;
}

void CMonster::Die(int DieTick)
{
	if(DieTick < 1)
		DieTick = 1;
	const bool FirstDie = m_DieTick < 0;
	if(m_DieTick < 0 || DieTick < m_DieTick)
		m_DieTick = DieTick;
	if(FirstDie && m_Boss)
	{
		g_Config.m_GcMoney += GC_BOSS_KILL_BONUS;
		GameServer()->m_LastRoundBossBonus += GC_BOSS_KILL_BONUS;
		int Bonus = GC_BOSS_KILL_BONUS;
		GameServer()->SendChatTarget(-1, _("【公司】头目 {lstr:name} 已被清除，公司奖励 {int:bonus} 元"), "name", LcMonsterName(m_BossType), "bonus", &Bonus);
		for(int i = 0; i < MAX_CLIENTS; i++)
			if(GameServer()->m_apPlayers[i])
				LcGrantAchievement(GameServer(), i, LC_ACH_FIRST_BOSS, _("【成就】{str:name} 达成「首次清除头目」"));
	}
}

void CMonster::Stun(int Ticks)
{
	if(Ticks <= 0)
		return;
	m_FreezeUntilTick = Server()->Tick() + Ticks;
	m_Freeze = true;
}

const char *CMonster::MonsterName()
{
    return LcMonsterName(m_Type);
}

const char *CMonster::MonsterDesc()
{
    return LcMonsterDesc(m_Type);
}

const char *CMonster::MonsterDescShort()
{
    return LcMonsterDescShort(m_Type);
}

bool CMonster::TakeDamage(vec2 Force, int Dmg, int From, int Weapon, bool FromMonster, bool Drain, bool FromReflect)
{
	m_Core.m_Vel += Force;

	if(FromMonster)
        return false;

    if(FromReflect && m_Health == 1 && !m_Armor)
        return false;

    int DrainedAmount = 0;

	m_DamageTaken++;

	// create healthmod indicator
	if(Server()->Tick() < m_DamageTakenTick+25)
	{
		// make sure that the damage indicators doesn't group together
		GameServer()->CreateDamageInd(m_Pos, m_DamageTaken*0.25f, Dmg);
	}
	else
	{
		m_DamageTaken = 0;
		GameServer()->CreateDamageInd(m_Pos, 0, Dmg);
	}

	if(Dmg)
	{
		if(m_Armor)
		{
			if(Dmg > 1)
			{
				m_Health--;
				Dmg--;
			}

			if(Dmg > m_Armor)
			{
				Dmg -= m_Armor;
				m_Armor = 0;
			}
			else
			{
				m_Armor -= Dmg;
				Dmg = 0;
			}
		}

		m_Health -= Dmg;
		if(m_Health < 0)
			m_Health = 0;
	}

	m_DamageTakenTick = Server()->Tick();

	// do damage Hit sound
	if(From >= 0 && From < MAX_CLIENTS && GameServer()->m_apPlayers[From])
	{
		int Mask = CmaskOne(From);
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() == TEAM_SPECTATORS && GameServer()->m_apPlayers[i]->m_SpectatorID == From)
				Mask |= CmaskOne(i);
		}
		GameServer()->CreateSound(GameServer()->m_apPlayers[From]->m_ViewPos, SOUND_HIT, Mask);
	}

	if(DrainedAmount && Drain && GameServer()->GetPlayerChar(From))
    {
        if(DrainedAmount + GameServer()->GetPlayerChar(From)->m_Health > 10)
        {
            int ArmorToGive = DrainedAmount;
            ArmorToGive -= 10 - GameServer()->GetPlayerChar(From)->m_Health;
            GameServer()->GetPlayerChar(From)->IncreaseHealth(DrainedAmount);
            GameServer()->GetPlayerChar(From)->IncreaseArmor(ArmorToGive);
        }
        else
            GameServer()->GetPlayerChar(From)->IncreaseHealth(DrainedAmount);
    }

	// check for death
	if(m_Health <= 0)
	{
	    if(FromReflect)
        {
            m_Health = 1;
            goto here;
        }
		// set attacker's face to happy (taunt!)
		if (From >= 0 && From < MAX_CLIENTS && GameServer()->m_apPlayers[From])
		{
			CCharacter *pChr = GameServer()->m_apPlayers[From]->GetCharacter();
			if (pChr)
				pChr->SetEmote(EMOTE_HAPPY, Server()->Tick() + Server()->TickSpeed());
		}

		if(m_Type == TYPE_BOMBER && !m_Exploded)
		{
			m_Exploded = true;
			GameServer()->CreateExplosion(m_Pos, From, WEAPON_GRENADE, false);
			GameServer()->CreateSound(m_Pos, SOUND_GRENADE_EXPLODE, CmaskAll());
		}

		Die(1);

		return false;
	}

	here:

	if (Dmg > 2)
		GameServer()->CreateSound(m_Pos, SOUND_PLAYER_PAIN_LONG);
	else
		GameServer()->CreateSound(m_Pos, SOUND_PLAYER_PAIN_SHORT);

	return true;
}

CCharacter *CMonster::ClosestPlayer(vec2 Pos, float Radius)
{
	float ClosestRange = Radius * 2.f;
	CCharacter *pClosest = 0;

	for(CCharacter *pChr = (CCharacter *)GameWorld()->FindFirst(CGameWorld::ENTTYPE_CHARACTER); pChr; pChr = (CCharacter *)pChr->TypeNext())
	{
		if(!pChr->GetPlayer() || pChr->m_Freeze || pChr->m_InShip)
			continue;

		float Len = distance(Pos, pChr->m_Pos);
		if(Len < pChr->m_ProximityRadius + Radius && Len < ClosestRange)
		{
			ClosestRange = Len;
			pClosest = pChr;
		}
	}

	return pClosest;
}

bool CMonster::IsSeenByPlayer(CCharacter *pChr)
{
	if(m_Type != TYPE_FEAR || !pChr || !pChr->GetPlayer() || pChr->m_Freeze || pChr->m_InShip)
		return false;
	if(distance(pChr->m_Pos, m_Pos) > (float)GC_COILHEAD_RANGE)
		return false;
	if(GameServer()->Collision()->IntersectLine(pChr->m_Pos, m_Pos, 0x0, 0x0))
		return false;

	CPlayer *pP = pChr->GetPlayer();
	vec2 Aim = normalize(vec2((float)pP->m_LatestActivity.m_TargetX, (float)pP->m_LatestActivity.m_TargetY));
	vec2 ToMonster = m_Pos - pChr->m_Pos;
	float Len = length(ToMonster);
	if(Len < 1.f)
		return true;
	return dot(Aim, normalize(ToMonster)) > 0.55f;
}

bool CMonster::IsSeenByAnyPlayer()
{
	for(CCharacter *pChr = (CCharacter *)GameWorld()->FindFirst(CGameWorld::ENTTYPE_CHARACTER); pChr; pChr = (CCharacter *)pChr->TypeNext())
	{
		if(IsSeenByPlayer(pChr))
			return true;
	}
	return false;
}

void CMonster::HandleCoilheadStareBroadcast()
{
	const int Interval = Server()->TickSpeed() * 2;
	for(CCharacter *pChr = (CCharacter *)GameWorld()->FindFirst(CGameWorld::ENTTYPE_CHARACTER); pChr; pChr = (CCharacter *)pChr->TypeNext())
	{
		if(!IsSeenByPlayer(pChr))
			continue;

		const int CID = pChr->GetPlayer()->GetCID();
		if(m_LastCoilStareBroadcastTick[CID] + Interval > Server()->Tick())
			continue;

		m_LastCoilStareBroadcastTick[CID] = Server()->Tick();
		GameServer()->SendBroadcast(CID, BROADCAST_PRIORITY_EFFECTSTATE, BROADCAST_DURATION_GAMEANNOUNCE, _("弹簧头被你的注视定住了"));
	}
}

CCharacter *CMonster::HighestValuePlayer(vec2 Pos, float Radius)
{
	CCharacter *pBest = 0;
	int BestValue = -1;

	for(CCharacter *pChr = (CCharacter *)GameWorld()->FindFirst(CGameWorld::ENTTYPE_CHARACTER); pChr; pChr = (CCharacter *)pChr->TypeNext())
	{
		if(!pChr->GetPlayer() || pChr->m_Freeze || pChr->m_InShip)
			continue;

		if(distance(Pos, pChr->m_Pos) >= Radius)
			continue;

		int Value = pChr->GetPlayer()->GetBackpackValue();
		if(Value > BestValue)
		{
			BestValue = Value;
			pBest = pChr;
		}
	}

	return pBest;
}

void CMonster::Snap(int SnappingClient)
{
    if(NetworkClipped(SnappingClient))
		return;

    if(m_Type == TYPE_BUG && m_Hidden)
        return;

    const int ClientID = SnapClientID(m_MonsterID);

    const char *pName = MonsterName();
    const char *pSkin = "default";
    const char *pClan = "威胁";
    int BodyColor = 9830400;
    int FeetColor = 7864320;
    int Weapon = WEAPON_HAMMER;

    if(m_Boss)
    {
        pSkin = "twintri";
        pClan = "首领";
        BodyColor = 16760576;
        FeetColor = 13369344;
        Weapon = WEAPON_GRENADE;
    }
    else
    {
        switch(m_Type)
        {
        case TYPE_PULLHANDLE:
            pSkin = "default";
            pClan = "布条";
            BodyColor = 16711680;
            FeetColor = 13369344;
            Weapon = WEAPON_HAMMER;
            break;
        case TYPE_SATIETY:
            pSkin = "brownbear";
            pClan = "囤积";
            BodyColor = 65280;
            FeetColor = 32768;
            Weapon = WEAPON_GUN;
            break;
        case TYPE_LEEK_BOX:
            pSkin = "Coala";
            pClan = "孢子";
            BodyColor = 8454143;
            FeetColor = 5636095;
            Weapon = WEAPON_NINJA;
            break;
        case TYPE_BUG:
            pSkin = "twinbop";
            pClan = "蔓背";
            BodyColor = 32768;
            FeetColor = 16384;
            Weapon = WEAPON_SHOTGUN;
            break;
        case TYPE_FEAR:
            pSkin = "bluekitty";
            pClan = "弹簧";
            BodyColor = 8388736;
            FeetColor = 4194432;
            Weapon = WEAPON_RIFLE;
            break;
        case TYPE_HUNTER:
            pSkin = "limekitty";
            pClan = "猛禽";
            BodyColor = 16753920;
            FeetColor = 16711680;
            Weapon = WEAPON_GUN;
            break;
        case TYPE_BOMBER:
            pSkin = "saddo";
            pClan = "爆壳";
            BodyColor = 16744448;
            FeetColor = 13369344;
            Weapon = WEAPON_GRENADE;
            break;
        case TYPE_LEECH:
            pSkin = "cammo";
            pClan = "吸盘";
            BodyColor = 8421504;
            FeetColor = 4210752;
            Weapon = WEAPON_HAMMER;
            break;
        case TYPE_STALKER:
            pSkin = "pinky";
            pClan = "潜追";
            BodyColor = 16711935;
            FeetColor = 8388736;
            Weapon = WEAPON_NINJA;
            break;
        default:
            break;
        }
    }

    CNetObj_ClientInfo *pClientInfo = static_cast<CNetObj_ClientInfo *>(Server()->SnapNewItem(NETOBJTYPE_CLIENTINFO, ClientID, sizeof(CNetObj_ClientInfo)));
    if(!pClientInfo)
        return;

    StrToInts(&pClientInfo->m_Name0, 4, pName);
    StrToInts(&pClientInfo->m_Skin0, 6, pSkin);
    StrToInts(&pClientInfo->m_Clan0, 3, pClan);
    pClientInfo->m_Country = -1;
    pClientInfo->m_UseCustomColor = 1;
    pClientInfo->m_ColorBody = BodyColor;
    pClientInfo->m_ColorFeet = FeetColor;

    CNetObj_PlayerInfo *pPlayerInfo = static_cast<CNetObj_PlayerInfo *>(Server()->SnapNewItem(NETOBJTYPE_PLAYERINFO, ClientID, sizeof(CNetObj_PlayerInfo)));
    if(!pPlayerInfo)
        return;

    pPlayerInfo->m_Local = 0;
    pPlayerInfo->m_ClientID = ClientID;
    pPlayerInfo->m_Team = TEAM_RED;
    pPlayerInfo->m_Score = GetLifes();
    pPlayerInfo->m_Latency = 0;

    CNetObj_Character *pCharacter = static_cast<CNetObj_Character *>(Server()->SnapNewItem(NETOBJTYPE_CHARACTER, ClientID, sizeof(CNetObj_Character)));
    if(!pCharacter)
        return;

    pCharacter->m_Tick = 0;
    pCharacter->m_X = (int)m_Pos.x;
    pCharacter->m_Y = (int)m_Pos.y;
    pCharacter->m_VelX = (int)(m_Core.m_Vel.x * 256.0f);
    pCharacter->m_VelY = (int)(m_Core.m_Vel.y * 256.0f);
    pCharacter->m_HookState = m_Core.m_HookState;
    pCharacter->m_HookTick = m_Core.m_HookTick;
    pCharacter->m_HookX = (int)m_Core.m_HookPos.x;
    pCharacter->m_HookY = (int)m_Core.m_HookPos.y;
    pCharacter->m_HookDx = (int)(m_Core.m_HookDir.x * 256.0f);
    pCharacter->m_HookDy = (int)(m_Core.m_HookDir.y * 256.0f);
    pCharacter->m_HookedPlayer = m_Core.m_HookedPlayer;
    pCharacter->m_Jumped = m_Core.m_Jumped;
    pCharacter->m_Direction = m_Path.m_Direction;

    vec2 AimDir = vec2((float)m_Path.m_Direction, -1.f);
    if(length(m_Core.m_Vel) > 0.01f)
        AimDir = normalize(m_Core.m_Vel);
    else if(m_Path.m_Direction == 0)
        AimDir = vec2(1.f, 0.f);
    pCharacter->m_Angle = (int)(GetAngle(normalize(AimDir)) * 256.0f);

    pCharacter->m_PlayerFlags = 0;
    pCharacter->m_Health = m_Health;
    pCharacter->m_Armor = m_Armor;
    pCharacter->m_AmmoCount = 0;
    pCharacter->m_Weapon = Weapon;
    pCharacter->m_AttackTick = 0;

    if(m_Freeze)
        pCharacter->m_Emote = EMOTE_SURPRISE;
    else if(m_Type == TYPE_FEAR && IsSeenByAnyPlayer())
        pCharacter->m_Emote = EMOTE_BLINK;
    else if(Server()->Tick() - m_DamageTakenTick < Server()->TickSpeed() / 2)
        pCharacter->m_Emote = EMOTE_PAIN;
    else if(m_Type == TYPE_FEAR)
        pCharacter->m_Emote = EMOTE_ANGRY;
    else
        pCharacter->m_Emote = EMOTE_NORMAL;
}