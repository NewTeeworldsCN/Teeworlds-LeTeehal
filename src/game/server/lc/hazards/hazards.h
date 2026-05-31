#ifndef GAME_SERVER_LC_HAZARDS_H
#define GAME_SERVER_LC_HAZARDS_H

enum ELcHazard
{
	LC_HAZARD_NONE = 0,
	LC_HAZARD_GAS = 1,
	LC_HAZARD_MINE = 2,
	LC_HAZARD_GRASS = 3,
	LC_HAZARD_SHOCK = 4,
	LC_HAZARD_SPIKE = 5,
	LC_HAZARD_TAR = 6,
};

enum ELcFacilityRoom
{
	LC_ROOM_NONE = 0,
	LC_ROOM_BATTERY = 10,
	LC_ROOM_FUSE = 11,
	LC_ROOM_GENERATOR = 12,
};

enum ELcFacilityType
{
	LC_FACILITY_FACTORY = 0,
	LC_FACILITY_MANSION,
	LC_FACILITY_MINES,
	LC_FACILITY_WAREHOUSE,
	LC_FACILITY_RESEARCH,
	NUM_LC_FACILITY_TYPES,
};

enum ELcHazardVisual
{
	LC_VIS_GAS = 0,
	LC_VIS_MINE,
	LC_VIS_SHOCK,
	LC_VIS_SPIKE,
	LC_VIS_TAR,
	LC_VIS_ROOM,
};

// Doodads / foreground tile indices (metal_main set)
static const int LC_TILE_DOODAD_GAS = 51;
static const int LC_TILE_DOODAD_GRASS = 84;
static const int LC_TILE_DOODAD_MINE = 100;
static const int LC_TILE_DOODAD_TAR = 101;
static const int LC_TILE_DOODAD_SHOCK = 102;
static const int LC_TILE_DOODAD_WARN = 103;
static const int LC_TILE_FG_MINE = 100;
static const int LC_TILE_FG_GAS = 103;
static const int LC_TILE_FG_SHOCK = 102;
static const int LC_TILE_FG_SPIKE = 19;
static const int LC_TILE_FG_BATTERY = 36;
static const int LC_TILE_FG_ROOM = 34;

inline bool LcIsHazardReserved(int Reserved)
{
	return Reserved >= LC_HAZARD_GAS && Reserved <= LC_HAZARD_TAR;
}

inline bool LcHazardUsesProjectileSnap(int Hazard)
{
	return Hazard == LC_HAZARD_MINE || Hazard == LC_HAZARD_SPIKE || Hazard == LC_HAZARD_SHOCK;
}

inline bool LcIsFacilityRoomReserved(int Reserved)
{
	return Reserved >= LC_ROOM_BATTERY && Reserved <= LC_ROOM_GENERATOR;
}

const char *LcHazardNameZh(int Hazard);
const char *LcFacilityRoomNameZh(int Room);

#endif
