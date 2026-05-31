#ifndef GAME_SERVER_LC_MOONS_H
#define GAME_SERVER_LC_MOONS_H

#include "../hazards/hazards.h"

enum ELcMoon
{
	LC_MOON_EXPERIMENTATION = 0,
	LC_MOON_ASSURANCE,
	LC_MOON_VOW,
	LC_MOON_OFFENSE,
	LC_MOON_TITAN,
	NUM_LC_MOONS,
};

struct SLcMoonConfig
{
	const char *m_pName;
	const char *m_pTheme;
	int m_MapGenLevel;
	int m_TimelimitMin;
	int m_ScrapPercent;
	int m_MonsterPercent;
	int m_HazardStars;
	ELcFacilityType m_FacilityType;
};

const SLcMoonConfig *LcGetMoon(int Moon);
void LcApplyMoon(int Moon);
const char *LcMoonName(int Moon);
int LcMoonScrapMultiplierPercent();
int LcMoonMonsterMultiplierPercent();
const char *LcFacilityName(ELcFacilityType Type);
bool LcFacilityUsesGridLayout(ELcFacilityType Type);
ELcFacilityType LcGetFacilityType(int Moon);

#endif
