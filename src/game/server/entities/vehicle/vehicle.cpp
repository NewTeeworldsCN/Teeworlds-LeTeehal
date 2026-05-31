#include "vehicle.h"
#include "aircraft.h"

#include "vehicle_util.h"
#include "vehicle_visual.h"

#include <game/server/entities/core/character.h>
#include <game/server/core/gamecontext.h>
#include <game/server/core/player.h>

CVehicle::CVehicle(CGameWorld *pGameWorld, int ObjType, vec2 Pos, int Team, int Health)
: CEntity(pGameWorld, ObjType)
{
	m_Pos = Pos;
	m_Health = Health;
	m_MaxHealth = Health;
	m_Team = Team;
	m_Driver = -1;
	m_Vel = vec2(0.f, 0.f);
	m_SpawnPos = Pos;
	m_Dead = false;
	for(int i = 0; i < 3; i++)
		m_aHealthBarIds[i] = Server()->SnapNewID();
}

CVehicle::~CVehicle()
{
	for(int i = 0; i < 3; i++)
		Server()->SnapFreeID(m_aHealthBarIds[i]);
	ForceDriverLeave();
}

CCharacter *CVehicle::DriverChar()
{
	if(m_Driver < 0)
		return 0;
	return GameServer()->GetPlayerChar(m_Driver);
}

void CVehicle::ApplyHorizontalInput(CCharacter *pDriver, int MaxSpeed, int Accel, float IdleDecay)
{
	if(!pDriver)
		return;

	if(pDriver->GetInput().m_Direction < 0)
		m_Vel.x = SaturatedAdd(-(float)MaxSpeed, (float)MaxSpeed, m_Vel.x, -(float)Accel);
	else if(pDriver->GetInput().m_Direction > 0)
		m_Vel.x = SaturatedAdd(-(float)MaxSpeed, (float)MaxSpeed, m_Vel.x, (float)Accel);
	else
		m_Vel.x *= IdleDecay;
}

void CVehicle::MoveBox(vec2 Size)
{
	vec2 NewPos = m_Pos;
	GameServer()->Collision()->MoveBox(&NewPos, &m_Vel, Size, 0.f);
	m_Pos = NewPos;
}

bool CVehicle::CanBoard(int Team) const
{
	(void)Team;
	return !m_Dead;
}

void CVehicle::TakeDamage(int Amount, int From)
{
	(void)From;
	if(Amount <= 0 || m_Dead)
		return;

	m_Health -= Amount;
	if(m_Health <= 0)
		Explode();
}

void CVehicle::SpawnShrapnel()
{
	VehicleSpawnShrapnel(GameServer(), GameWorld(), m_Pos, m_Driver >= 0 ? m_Driver : -1, m_Team);
}

void CVehicle::SnapHealthBar(int SnappingClient)
{
	if(NetworkClipped(SnappingClient) || m_Dead)
		return;

	const int Segments = 3;
	const int Filled = maximum(0, minimum(Segments, (m_Health * Segments + m_MaxHealth - 1) / maximum(1, m_MaxHealth)));
	for(int i = 0; i < Segments; i++)
	{
		if(i >= Filled)
			break;
		VehicleVisual::SnapPickup(Server(), m_aHealthBarIds[i],
			vec2(m_Pos.x + (i - 1) * VehicleScale(10.f), m_Pos.y - VehicleScale(38.f)), POWERUP_HEALTH);
	}
}

void CVehicle::Explode()
{
	if(m_Dead)
		return;

	const int Owner = m_Driver >= 0 ? m_Driver : -1;
	const vec2 ExplosionPos = m_Pos;

	m_Dead = true;
	m_Health = 0;
	ForceDriverLeave();
	GameServer()->CreateExplosion(ExplosionPos, Owner, WEAPON_GRENADE, false);
	SpawnShrapnel();
	OnBeforeDestroy();
	GameWorld()->DestroyEntity(this);
}

void CVehicle::OnDriverBoarded(CCharacter *pDriver)
{
	(void)pDriver;
}

void CVehicle::Tick()
{
	if(m_Dead)
		return;

	if(m_Driver >= 0)
	{
		CCharacter *pDriver = DriverChar();
		if(!pDriver || pDriver->m_Freeze)
		{
			ForceDriverLeave();
			return;
		}

		TickDriver(pDriver);
		VehicleSyncCharacter(pDriver, m_Pos, m_Vel, DriverOffsetY());
		pDriver->m_VehicleSeat = VEHICLE_SEAT_DRIVER;
	}
	else
	{
		TickIdle();
		if(VehicleTryAutoBoard(GameWorld(), this, m_Driver, m_Team, BoardRadius()))
		{
			if(CCharacter *pDriver = DriverChar())
			{
				pDriver->m_VehicleSeat = VEHICLE_SEAT_DRIVER;
				OnDriverBoarded(pDriver);
			}
		}
	}
}

void CVehicle::Reset()
{
	m_Dead = true;
	GameWorld()->DestroyEntity(this);
}

void CVehicle::ForceDriverLeave()
{
	if(m_Driver < 0)
		return;

	if(CCharacter *pDriver = DriverChar())
		VehicleDismount(m_Driver, m_Vel, pDriver);
	else
		m_Driver = -1;
}

bool CVehicle::IsOccupiedBy(int ClientId) const
{
	return m_Driver == ClientId;
}

void CVehicle::HandleOccupantDismount(int ClientId)
{
	if(m_Driver == ClientId)
		ForceDriverLeave();
}

void CVehicle::ForEachAircraft(CGameWorld *pWorld, CAircraft **ppOut, int Max, int *pNum)
{
	int Num = 0;
	for(CAircraft *pAircraft = (CAircraft *)pWorld->FindFirst(CGameWorld::ENTTYPE_AIRCRAFT); pAircraft; pAircraft = (CAircraft *)pAircraft->TypeNext())
	{
		if(Num < Max)
			ppOut[Num++] = pAircraft;
	}
	if(pNum)
		*pNum = Num;
}
