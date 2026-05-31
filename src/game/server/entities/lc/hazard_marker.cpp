#include <cmath>

#include <game/generated/protocol.h>
#include <game/gamecore.h>
#include <game/collision.h>
#include <game/server/core/gamecontext.h>
#include <game/server/lc/hazards/hazards.h>
#include "hazard_marker.h"

static const float HAZARD_PI = 3.14159265f;

CHazardMarker::CHazardMarker(CGameWorld *pGameWorld, vec2 Pos, int Reserved, float Size)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PROJECTILE)
{
	m_Center = Pos;
	m_Reserved = Reserved;
	m_Size = Size;
	m_PulseTick = 0;
	m_Pos = Pos;
	m_ProximityRadius = (int)Size + 32;
	GameWorld()->InsertEntity(this);
}

void CHazardMarker::Reset()
{
	GameWorld()->DestroyEntity(this);
}

void CHazardMarker::Tick()
{
	if(GameWorld()->m_Paused)
		return;
	m_PulseTick++;

	if(LcIsFacilityRoomReserved(m_Reserved))
		return;

	CCollision *pCol = GameServer()->Collision();
	if(!pCol)
		return;

	int Tx = round_to_int(m_Center.x) / 32;
	int Ty = round_to_int(m_Center.y) / 32;
	if(pCol->GetTileReserved(Tx, Ty) != m_Reserved)
		Reset();
}

void CHazardMarker::TickPaused()
{
}

float CHazardMarker::PulseScale() const
{
	if(m_Reserved != LC_HAZARD_MINE && m_Reserved != LC_HAZARD_SHOCK && m_Reserved != LC_HAZARD_SPIKE)
		return 1.f;

	float Phase = (m_PulseTick % 30) / 30.f;
	return 0.82f + 0.28f * (0.5f + 0.5f * sinf(Phase * 2.f * HAZARD_PI));
}

void CHazardMarker::SnapLaser(vec2 From, vec2 To, int SnappingClient)
{
	if(SnappingClient != -1)
	{
		if(NetworkClipped(SnappingClient, From) && NetworkClipped(SnappingClient, To))
			return;
	}

	CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, Server()->SnapNewID(), sizeof(CNetObj_Laser)));
	if(!pObj)
		return;

	pObj->m_X = (int)To.x;
	pObj->m_Y = (int)To.y;
	pObj->m_FromX = (int)From.x;
	pObj->m_FromY = (int)From.y;
	pObj->m_StartTick = Server()->Tick();
}

void CHazardMarker::SnapProjectile(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

	int Weapon = WEAPON_GRENADE;
	switch(m_Reserved)
	{
	case LC_HAZARD_MINE: Weapon = WEAPON_GRENADE; break;
	case LC_HAZARD_SPIKE: Weapon = WEAPON_HAMMER; break;
	case LC_HAZARD_SHOCK: Weapon = WEAPON_GUN; break;
	default: return;
	}

	CNetObj_Projectile *pProj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, m_ID, sizeof(CNetObj_Projectile)));
	if(!pProj)
		return;

	pProj->m_Type = Weapon;
	pProj->m_VelX = 0;
	pProj->m_VelY = 0;
	pProj->m_X = (int)m_Center.x;
	pProj->m_Y = (int)m_Center.y;
	pProj->m_StartTick = Server()->Tick();
}

void CHazardMarker::SnapLaserFrame(int SnappingClient)
{
	float S = m_Size * PulseScale();
	if(LcIsFacilityRoomReserved(m_Reserved))
		S = m_Size + 8.f;

	SnapLaser(m_Center + vec2(-S, -S), m_Center + vec2(S, -S), SnappingClient);
	SnapLaser(m_Center + vec2(S, -S), m_Center + vec2(S, S), SnappingClient);
	SnapLaser(m_Center + vec2(S, S), m_Center + vec2(-S, S), SnappingClient);
	SnapLaser(m_Center + vec2(-S, S), m_Center + vec2(-S, -S), SnappingClient);

	if(m_Reserved == LC_HAZARD_MINE)
	{
		SnapLaser(m_Center + vec2(-S * 0.7f, 0.f), m_Center + vec2(S * 0.7f, 0.f), SnappingClient);
		SnapLaser(m_Center + vec2(0.f, -S * 0.7f), m_Center + vec2(0.f, S * 0.7f), SnappingClient);
	}
	else if(m_Reserved == LC_HAZARD_GAS)
	{
		SnapLaser(m_Center + vec2(-S * 0.5f, 0.f), m_Center + vec2(S * 0.5f, 0.f), SnappingClient);
		SnapLaser(m_Center + vec2(0.f, -S * 0.5f), m_Center + vec2(0.f, S * 0.5f), SnappingClient);
	}
}

void CHazardMarker::Snap(int SnappingClient)
{
	if(m_Reserved == LC_HAZARD_MINE)
	{
		SnapProjectile(SnappingClient);
		SnapLaserFrame(SnappingClient);
		return;
	}

	if(LcHazardUsesProjectileSnap(m_Reserved))
	{
		SnapProjectile(SnappingClient);
		SnapLaserFrame(SnappingClient);
		return;
	}

	SnapLaserFrame(SnappingClient);
}
