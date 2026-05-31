#include <engine/shared/config.h>
#include <base/system.h>
#include <teeuniverses/components/localization.h>
#include "moons.h"

static const SLcMoonConfig s_aMoons[NUM_LC_MOONS] =
{
	{ _("实验"), "metal_main", 1, 7, 75, 65, 1, LC_FACILITY_FACTORY },
	{ _("保障"), "grass_main", 2, 5, 100, 100, 2, LC_FACILITY_MANSION },
	{ _("誓约"), "metal_main", 3, 4, 130, 130, 3, LC_FACILITY_RESEARCH },
	{ _("攻势"), "desert_main", 4, 3, 160, 160, 4, LC_FACILITY_MINES },
	{ _("泰坦"), "winter_main", 5, 2, 180, 180, 5, LC_FACILITY_WAREHOUSE },
};

static int s_ScrapPercent = 100;
static int s_MonsterPercent = 100;

const SLcMoonConfig *LcGetMoon(int Moon)
{
	if(Moon < 0 || Moon >= NUM_LC_MOONS)
		return &s_aMoons[LC_MOON_ASSURANCE];
	return &s_aMoons[Moon];
}

void LcApplyMoon(int Moon)
{
	const SLcMoonConfig *pMoon = LcGetMoon(Moon);
	str_copy(g_Config.m_SvMapgenTheme, pMoon->m_pTheme, sizeof(g_Config.m_SvMapgenTheme));
	g_Config.m_SvMapGenLevel = pMoon->m_MapGenLevel;
	g_Config.m_SvTimelimit = pMoon->m_TimelimitMin;
	s_ScrapPercent = pMoon->m_ScrapPercent;
	s_MonsterPercent = pMoon->m_MonsterPercent;
}

const char *LcMoonName(int Moon)
{
	return LcGetMoon(Moon)->m_pName;
}

int LcMoonScrapMultiplierPercent()
{
	return s_ScrapPercent;
}

int LcMoonMonsterMultiplierPercent()
{
	return s_MonsterPercent;
}

ELcFacilityType LcGetFacilityType(int Moon)
{
	return LcGetMoon(Moon)->m_FacilityType;
}

const char *LcFacilityName(ELcFacilityType Type)
{
	switch(Type)
	{
	case LC_FACILITY_MANSION: return _("宅邸");
	case LC_FACILITY_MINES: return _("矿道");
	case LC_FACILITY_WAREHOUSE: return _("仓储");
	case LC_FACILITY_RESEARCH: return _("研究区");
	default: return _("工厂");
	}
}

bool LcFacilityUsesGridLayout(ELcFacilityType Type)
{
	return Type == LC_FACILITY_WAREHOUSE || Type == LC_FACILITY_RESEARCH || Type == LC_FACILITY_MANSION;
}
