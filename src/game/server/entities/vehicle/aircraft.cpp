#include <game/server/core/gamecontext.h>
#include <game/server/core/gamecontroller.h>
#include <game/server/lc/expedition/balance.h>
#include <game/server/entities/core/character.h>
#include <game/server/entities/lc/scrap.h>
#include <game/server/entities/lc/ship.h>

#include "aircraft.h"
#include "vehicle_util.h"
#include "vehicle_visual.h"

CAircraft::CAircraft(CGameWorld *pGameWorld, vec2 Pos, int Team)
: CVehicle(pGameWorld, CGameWorld::ENTTYPE_AIRCRAFT, Pos, Team, 50)
{
	m_PrevDriverJump = 0;
	m_InShip = false;
	for(int i = 0; i < AIRCRAFT_PART_IDS; i++)
		m_PartIds[i] = Server()->SnapNewID();
	GameWorld()->InsertEntity(this);
}

CAircraft::~CAircraft()
{
	for(int i = 0; i < AIRCRAFT_PART_IDS; i++)
		Server()->SnapFreeID(m_PartIds[i]);
}

bool CAircraft::ShipContains(vec2 Pos)
{
	CShip *pShip = GameServer()->m_pController ? GameServer()->m_pController->m_pShip : 0;
	if(!pShip)
		return false;
	return pShip->Contains(Pos);
}

void CAircraft::DropCargo(bool InShip)
{
	if(m_aCargo.size() == 0)
		return;

	vec2 DropPos = m_Pos;
	CShip *pShip = 0;
	if(InShip)
	{
		pShip = GameServer()->m_pController ? GameServer()->m_pController->m_pShip : 0;
		if(pShip)
			DropPos = pShip->m_Pos;
	}

	for(int i = 0; i < m_aCargo.size(); i++)
	{
		if(InShip && pShip)
		{
			const int Driver = GetDriver();
			if(Driver >= 0 && !m_aCargo[i].m_InShip)
				GameServer()->CreditShipScrapDeposit(Driver, m_aCargo[i].m_Value);
			GameServer()->DepositScrapInShip(DropPos, m_aCargo[i]);
		}
		else
			new CScrap(GameWorld(), 0, DropPos, false, false, m_aCargo[i]);
	}
	m_aCargo.clear();

	if(InShip && pShip)
	{
		GameServer()->CompactShipScrap(DropPos);
		pShip->UpdateValue();
	}
}

void CAircraft::CollectNearbyScrap()
{
	if(m_aCargo.size() >= GC_AIRCRAFT_MAX_CARGO)
		return;

	CScrap *apScrap[32];
	const float Radius = VehicleScale(56.f);
	int Num = GameWorld()->FindEntities(m_Pos, Radius, (CEntity **)apScrap, 32, CGameWorld::ENTTYPE_SCRAP);
	for(int i = 0; i < Num && m_aCargo.size() < GC_AIRCRAFT_MAX_CARGO; i++)
	{
		CScrap *pScrap = apScrap[i];
		if(!pScrap || pScrap->GetInShip())
			continue;

		const float PickupDist = (float)pScrap->GetWeight() * 2.f + 16.f;
		if(distance(pScrap->m_Pos, m_Pos) > PickupDist)
			continue;

		Scrap Item;
		pScrap->ExportScrap(Item);
		const vec2 HitPos = pScrap->m_Pos;
		m_aCargo.add(Item);
		pScrap->Reset();
		GameServer()->CreateHammerHit(HitPos);
	}
}

void CAircraft::TickCargo()
{
	CollectNearbyScrap();

	const bool InShip = ShipContains(m_Pos);
	if(InShip && !m_InShip)
		DropCargo(true);
	m_InShip = InShip;
}

void CAircraft::TickDriver(CCharacter *pDriver)
{
	ApplyHorizontalInput(pDriver, AIRCRAFT_MAX_SPEED, AIRCRAFT_ACCEL, 0.92f);
	if(pDriver)
	{
		const CNetObj_PlayerInput &Input = pDriver->GetInput();
		if(Input.m_Hook & 1)
		{
			if(Input.m_TargetY < 0)
				m_Vel.y = SaturatedAdd(-(float)AIRCRAFT_MAX_SPEED, (float)AIRCRAFT_MAX_SPEED, m_Vel.y, -(float)AIRCRAFT_ACCEL);
			else
				m_Vel.y = SaturatedAdd(-(float)AIRCRAFT_MAX_SPEED, (float)AIRCRAFT_MAX_SPEED, m_Vel.y, (float)AIRCRAFT_ACCEL);
		}

		pDriver->TickVehicleWeapon();

		if(VehicleInputPressed(m_PrevDriverJump, Input.m_Jump))
			ForceDriverLeave();
		m_PrevDriverJump = Input.m_Jump & INPUT_STATE_MASK;
	}
	VehicleApplyFriction(m_Vel, 0.96f);
	MoveBox(CollisionSize());
}

void CAircraft::TickIdle()
{
	VehicleApplyFriction(m_Vel, 0.95f);
	MoveBox(CollisionSize());
}

void CAircraft::Tick()
{
	CVehicle::Tick();
	TickCargo();
}

void CAircraft::OnBeforeDestroy()
{
	DropCargo(ShipContains(m_Pos));
}

void CAircraft::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient) || m_Dead)
		return;

	VehicleVisual::SnapAircraft(Server(), m_ID, m_PartIds, AIRCRAFT_PART_IDS, m_Pos);
	SnapHealthBar(SnappingClient);
}
