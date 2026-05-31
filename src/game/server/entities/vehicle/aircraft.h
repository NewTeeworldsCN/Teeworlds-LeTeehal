#ifndef GAME_SERVER_ENTITIES_VEHICLE_AIRCRAFT_H
#define GAME_SERVER_ENTITIES_VEHICLE_AIRCRAFT_H

#include <base/tl/array.h>
#include <game/server/scrap/scrap_info.h>

#include "vehicle.h"

enum
{
	AIRCRAFT_PART_IDS = 2,
	AIRCRAFT_MAX_SPEED = 18,
	AIRCRAFT_ACCEL = 2,
};

class CAircraft : public CVehicle
{
	int m_PartIds[AIRCRAFT_PART_IDS];
	int m_PrevDriverJump;
	bool m_InShip;
	array<Scrap> m_aCargo;

	void TickCargo();
	void CollectNearbyScrap();
	void DropCargo(bool InShip);
	bool ShipContains(vec2 Pos);

protected:
	void TickDriver(CCharacter *pDriver);
	float DriverOffsetY() const { return VehicleScale(-8.f); }
	vec2 CollisionSize() const { return vec2(VehicleScale(48.f), VehicleScale(24.f)); }
	void TickIdle();
	void OnBeforeDestroy();

public:
	CAircraft(CGameWorld *pGameWorld, vec2 Pos, int Team = -1);
	~CAircraft();

	void Tick();
	void Snap(int SnappingClient);

	int CargoCount() const { return m_aCargo.size(); }
};

#endif
