#include "vehicle_util.h"

#include "vehicle.h"
#include "aircraft.h"

#include <engine/shared/protocol.h>
#include <game/generated/protocol.h>
#include <game/server/entities/core/character.h>
#include <game/server/entities/core/projectile.h>
#include <game/server/core/gamecontext.h>
#include <game/server/core/gameworld.h>
#include <game/server/core/player.h>

bool VehicleInputPressed(int Prev, int Cur)
{
	Prev &= INPUT_STATE_MASK;
	Cur &= INPUT_STATE_MASK;
	return ((Prev ^ Cur) & Cur) != 0;
}

bool VehicleSpotBlocked(CGameWorld *pWorld, vec2 Pos, float Radius)
{
	for(CAircraft *pAircraft = (CAircraft *)pWorld->FindFirst(CGameWorld::ENTTYPE_AIRCRAFT); pAircraft; pAircraft = (CAircraft *)pAircraft->TypeNext())
	{
		if(!pAircraft->IsDead() && distance(pAircraft->m_Pos, Pos) < Radius)
			return true;
	}
	return false;
}

void VehicleClearOccupant(CCharacter *pChr)
{
	if(!pChr)
		return;

	if(pChr->m_OnVehicle)
		pChr->m_VehicleDismountTick = pChr->Server()->Tick();

	pChr->m_OnVehicle = false;
	pChr->m_VehicleSeat = VEHICLE_SEAT_NONE;
}

void VehicleResetCharacterHook(CCharacter *pChr)
{
	if(!pChr)
		return;

	pChr->m_Core.m_HookState = HOOK_IDLE;
	pChr->m_Core.m_HookedPlayer = -1;
	pChr->m_Core.m_HookTick = 0;
	pChr->m_Core.m_HookPos = pChr->m_Core.m_Pos;
	pChr->m_Core.m_HookDir = vec2(0.f, -1.f);
	pChr->m_Core.m_Input.m_Hook = 0;
}

void VehicleDismount(int &Owner, vec2 &Vel, CCharacter *pChr)
{
	if(Owner < 0 || !pChr)
		return;

	VehicleClearOccupant(pChr);
	pChr->m_Core.m_Vel = Vel;
	Owner = -1;
	Vel *= 0.5f;
}

void VehicleSyncCharacter(CCharacter *pChr, vec2 Pos, vec2 Vel, float RiderOffsetY)
{
	if(!pChr)
		return;

	pChr->m_Core.m_Pos = vec2(Pos.x, Pos.y + RiderOffsetY);
	pChr->m_Core.m_Vel = Vel;
	pChr->m_Pos = pChr->m_Core.m_Pos;
	VehicleResetCharacterHook(pChr);
}

void VehicleApplyGravity(CGameContext *pGS, vec2 &Vel)
{
	if(!pGS)
		return;

	Vel.y += pGS->Tuning()->m_Gravity;
}

void VehicleApplyFriction(vec2 &Vel, float Friction)
{
	Vel.x *= Friction;
	Vel.y *= Friction;
}

void VehicleApplyFlyingVertical(CGameContext *pGS, vec2 &Vel, CCharacter *pDriver, int MaxSpeed, int Accel)
{
	VehicleApplyGravity(pGS, Vel);
	if(!pDriver)
		return;

	const CNetObj_PlayerInput &Input = pDriver->GetInput();
	if(Input.m_Hook & 1)
		Vel.y = SaturatedAdd(-(float)MaxSpeed, (float)MaxSpeed, Vel.y, -(float)Accel);
}

bool VehicleTryAutoBoard(CGameWorld *pWorld, CEntity *pVehicle, int &Owner, int Team, float BoardRadius)
{
	CCharacter *pChr = pWorld->ClosestCharacter(pVehicle->m_Pos, BoardRadius, pVehicle);
	if(!pChr || pChr->m_OnVehicle || pChr->m_Freeze || pChr->m_InShip)
		return false;

	CVehicle *pVehicleCast = static_cast<CVehicle *>(pVehicle);
	if(!pVehicleCast->CanBoard(pChr->GetPlayer()->GetTeam()))
		return false;
	if(Team >= 0 && pChr->GetPlayer()->GetTeam() != Team)
		return false;

	CGameContext *pGS = pWorld->GameServer();
	if(pGS->Server()->Tick() < pChr->m_VehicleDismountTick + pGS->Server()->TickSpeed())
		return false;

	pChr->m_OnVehicle = true;
	pChr->m_VehicleSeat = VEHICLE_SEAT_DRIVER;
	VehicleResetCharacterHook(pChr);
	pChr->UpdateTuningParam();
	Owner = pChr->GetPlayer()->GetCID();
	return true;
}

void VehicleHandleHeartsDismount(CGameContext *pGS, int ClientId)
{
	CCharacter *pChr = pGS->GetPlayerChar(ClientId);
	if(!pChr || !pChr->m_OnVehicle)
		return;

	for(CAircraft *pAircraft = (CAircraft *)pGS->m_World.FindFirst(CGameWorld::ENTTYPE_AIRCRAFT); pAircraft; pAircraft = (CAircraft *)pAircraft->TypeNext())
	{
		if(pAircraft->IsOccupiedBy(ClientId))
			pAircraft->HandleOccupantDismount(ClientId);
	}
}

void VehicleOnCharacterDie(CGameContext *pGS, int ClientId)
{
	for(CAircraft *pAircraft = (CAircraft *)pGS->m_World.FindFirst(CGameWorld::ENTTYPE_AIRCRAFT); pAircraft; pAircraft = (CAircraft *)pAircraft->TypeNext())
	{
		if(pAircraft->IsOccupiedBy(ClientId))
			pAircraft->HandleOccupantDismount(ClientId);
	}
}

CVehicle *VehicleFindByOccupant(CGameWorld *pWorld, int ClientId)
{
	for(CAircraft *pAircraft = (CAircraft *)pWorld->FindFirst(CGameWorld::ENTTYPE_AIRCRAFT); pAircraft; pAircraft = (CAircraft *)pAircraft->TypeNext())
	{
		if(!pAircraft->IsDead() && pAircraft->IsOccupiedBy(ClientId))
			return pAircraft;
	}
	return 0;
}

void VehicleSpawnShrapnel(CGameContext *pGS, CGameWorld *pWorld, vec2 Pos, int Owner, int Team)
{
	(void)Team;
	const int Pellets = 12;
	for(int i = 0; i < Pellets; i++)
	{
		const float Angle = (float)i / (float)Pellets * 2.f * pi;
		const vec2 Dir = vec2(cosf(Angle), sinf(Angle));
		const float Speed = mix((float)pGS->Tuning()->m_ShotgunSpeeddiff, 1.0f, 0.85f);
		new CProjectile(pWorld, WEAPON_SHOTGUN, Owner, Pos, Dir * Speed,
			(int)(pGS->Server()->TickSpeed() * pGS->Tuning()->m_ShotgunLifetime), 1, 0, 0, -1, WEAPON_SHOTGUN);
	}
	pGS->CreateSound(Pos, SOUND_SHOTGUN_FIRE);
}
