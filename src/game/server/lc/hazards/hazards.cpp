#include "hazards.h"

const char *LcHazardNameZh(int Hazard)
{
	switch(Hazard)
	{
	case LC_HAZARD_GAS: return "毒气区";
	case LC_HAZARD_MINE: return "地雷";
	case LC_HAZARD_GRASS: return "草丛";
	case LC_HAZARD_SHOCK: return "漏电区";
	case LC_HAZARD_SPIKE: return "尖刺陷阱";
	case LC_HAZARD_TAR: return "粘性地面";
	default: return "危险区";
	}
}

const char *LcFacilityRoomNameZh(int Room)
{
	switch(Room)
	{
	case LC_ROOM_BATTERY: return "电池房";
	case LC_ROOM_FUSE: return "保险丝间";
	case LC_ROOM_GENERATOR: return "发电机室";
	default: return "设施";
	}
}
