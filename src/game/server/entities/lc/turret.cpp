#include <cmath>

#include <base/math.h>
#include <engine/shared/config.h>
#include <game/gamecore.h>
#include <game/generated/protocol.h>
#include <game/server/core/gamecontext.h>
#include <game/server/lc/expedition/balance.h>

#include "../core/character.h"
#include "../core/projectile.h"
#include "turret.h"

static const float TURRET_PI = 3.14159265f;

static float NormalizeAngle(float Rad)
{
	while(Rad > TURRET_PI)
		Rad -= 2.f * TURRET_PI;
	while(Rad < -TURRET_PI)
		Rad += 2.f * TURRET_PI;
	return Rad;
}

static float AimDot(vec2 Aim, vec2 To)
{
	float Len = length(To);
	if(Len < 1.f)
		return 1.f;
	return dot(Aim, To / Len);
}

CTurret::CTurret(CGameWorld *pGameWorld, vec2 Pos)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_LASER)
{
	m_Pos = Pos;
	m_AimDir = vec2(1.f, 0.f);
	m_TargetClientId = -1;
	m_LastFireTick = 0;
	m_ProximityRadius = 32;
	GameWorld()->InsertEntity(this);
}

void CTurret::Reset()
{
	GameWorld()->DestroyEntity(this);
}

bool CTurret::HasLineOfSight(vec2 To)
{
	CCollision *pCol = GameServer()->Collision();
	if(!pCol)
		return false;
	return !pCol->IntersectLine(m_Pos, To, 0x0, 0x0);
}

CCharacter *CTurret::GetTrackedTarget()
{
	if(m_TargetClientId < 0)
		return 0;
	CCharacter *pChr = GameServer()->GetPlayerChar(m_TargetClientId);
	if(!pChr || pChr->m_Freeze || pChr->m_InShip)
		return 0;
	if(distance(m_Pos, pChr->m_Pos) > (float)GC_TURRET_LOSE_RANGE)
		return 0;
	if(!HasLineOfSight(pChr->m_Pos))
		return 0;
	return pChr;
}

CCharacter *CTurret::FindTargetInAimCone(float Range)
{
	const float AcquireDot = cosf(GC_TURRET_AIM_CONE_DEG * TURRET_PI / 180.f);
	CCharacter *pBest = 0;
	float BestDist = Range;

	for(CCharacter *pChr = (CCharacter *)GameWorld()->FindFirst(CGameWorld::ENTTYPE_CHARACTER); pChr; pChr = (CCharacter *)pChr->TypeNext())
	{
		if(!pChr->GetPlayer() || pChr->m_Freeze || pChr->m_InShip)
			continue;

		vec2 To = pChr->m_Pos - m_Pos;
		float Dist = length(To);
		if(Dist > Range || Dist < 32.f)
			continue;
		if(AimDot(m_AimDir, To) < AcquireDot)
			continue;
		if(!HasLineOfSight(pChr->m_Pos))
			continue;

		if(Dist < BestDist)
		{
			BestDist = Dist;
			pBest = pChr;
		}
	}

	return pBest;
}

void CTurret::RotateAim(float Degrees)
{
	float Rad = Degrees * TURRET_PI / 180.f;
	float Cur = atan2f(m_AimDir.y, m_AimDir.x);
	Cur = NormalizeAngle(Cur + Rad);
	m_AimDir = vec2(cosf(Cur), sinf(Cur));
}

void CTurret::Tick()
{
	if(GameWorld()->m_Paused)
		return;
	if(GameServer()->Server()->m_LocateGame != LOCATE_GAME)
		return;

	CCharacter *pTarget = GetTrackedTarget();
	if(!pTarget)
	{
		m_TargetClientId = -1;
		RotateAim((float)GC_TURRET_IDLE_ROT_DEG / (float)GameServer()->Server()->TickSpeed());
		pTarget = FindTargetInAimCone((float)GC_TURRET_RANGE);
		if(pTarget)
			m_TargetClientId = pTarget->GetPlayer()->GetCID();
	}
	else
	{
		vec2 To = pTarget->m_Pos - m_Pos;
		float TargetAngle = atan2f(To.y, To.x);
		float CurAngle = atan2f(m_AimDir.y, m_AimDir.x);
		float Diff = NormalizeAngle(TargetAngle - CurAngle);
		float MaxStep = GC_TURRET_TRACK_ROT_DEG * TURRET_PI / 180.f / (float)GameServer()->Server()->TickSpeed();
		if(fabs(Diff) <= MaxStep)
			m_AimDir = normalize(To);
		else
			RotateAim((Diff > 0.f ? 1.f : -1.f) * GC_TURRET_TRACK_ROT_DEG / (float)GameServer()->Server()->TickSpeed());

		const float FireDot = cosf(GC_TURRET_FIRE_CONE_DEG * TURRET_PI / 180.f);
		if(AimDot(m_AimDir, To) >= FireDot &&
			m_LastFireTick + GameServer()->Server()->TickSpeed() * GC_TURRET_FIRE_SEC <= GameServer()->Server()->Tick())
		{
			vec2 FirePos = m_Pos + m_AimDir * 24.f;
			new CProjectile(GameWorld(), WEAPON_GUN, MAX_CLIENTS - 1, FirePos, m_AimDir,
				(int)(GameServer()->Server()->TickSpeed() * GameServer()->Tuning()->m_GunLifetime),
				1, 0, 0.f, -1, WEAPON_GUN);
			GameServer()->CreateSound(m_Pos, SOUND_GUN_FIRE, CmaskAll());
			m_LastFireTick = GameServer()->Server()->Tick();
		}
	}
}

void CTurret::TickPaused()
{
}

void CTurret::SnapLaser(vec2 From, vec2 To, int SnappingClient)
{
	if(NetworkClipped(SnappingClient, From) && NetworkClipped(SnappingClient, To))
		return;

	CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, Server()->SnapNewID(), sizeof(CNetObj_Laser)));
	if(!pObj)
		return;

	pObj->m_X = (int)To.x;
	pObj->m_Y = (int)To.y;
	pObj->m_FromX = (int)From.x;
	pObj->m_FromY = (int)From.y;
	pObj->m_StartTick = Server()->Tick();
}

void CTurret::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

	const float Base = 14.f;
	SnapLaser(m_Pos + vec2(-Base, -Base), m_Pos + vec2(Base, -Base), SnappingClient);
	SnapLaser(m_Pos + vec2(Base, -Base), m_Pos + vec2(Base, Base), SnappingClient);
	SnapLaser(m_Pos + vec2(Base, Base), m_Pos + vec2(-Base, Base), SnappingClient);
	SnapLaser(m_Pos + vec2(-Base, Base), m_Pos + vec2(-Base, -Base), SnappingClient);

	vec2 BarrelEnd = m_Pos + m_AimDir * 52.f;
	SnapLaser(m_Pos + m_AimDir * 8.f, BarrelEnd, SnappingClient);
}
