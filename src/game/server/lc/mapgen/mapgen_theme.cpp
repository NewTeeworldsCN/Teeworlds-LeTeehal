#include "mapgen_theme.h"

#include <base/system.h>

static const SLcMapgenThemeProfile s_aThemes[] =
{
	{"metal_main", "Metal", false, true, 1, 1.0f, 1.0f, 0.75f, 0, 0, 0, 0, 100},
	{"grass_main", "Grass", true, false, 1, 0.70f, 1.30f, 0.50f, 22, -8, -10, -6, 120},
	{"jungle_main", "Jungle", true, false, 1, 0.60f, 1.25f, 0.45f, 28, -4, -6, -8, 125},
	{"winter_main", "Winter", true, false, 2, 0.50f, 0.90f, 0.70f, -24, 10, 6, 10, 85},
	{"desert_main", "Mine", true, false, 2, 0.55f, 1.10f, 0.65f, -18, 4, 8, 4, 95},
};

const SLcMapgenThemeProfile *LcGetMapgenThemeProfile(const char *pTheme)
{
	if(pTheme && pTheme[0])
	{
		for(unsigned i = 0; i < sizeof(s_aThemes) / sizeof(s_aThemes[0]); i++)
		{
			if(str_comp(s_aThemes[i].m_pTheme, pTheme) == 0)
				return &s_aThemes[i];
		}
	}
	return &s_aThemes[0];
}

bool LcMapgenThemeUsesClientTileset(const char *pTheme)
{
	return LcGetMapgenThemeProfile(pTheme)->m_ClientBuiltinTileset;
}
