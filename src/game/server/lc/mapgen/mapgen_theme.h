#ifndef GAME_SERVER_LC_MAPGEN_THEME_H
#define GAME_SERVER_LC_MAPGEN_THEME_H

struct SLcMapgenThemeProfile
{
	const char *m_pTheme;
	const char *m_pAutomapConfig;
	bool m_ClientBuiltinTileset;
	bool m_GeneratePlatforms; // metal_main only: air platforms, boxes, fences
	int m_BackgroundTile;
	float m_BoxMul;
	float m_AirPlatformMul;
	float m_FenceSkipChance;
	int m_GrassHazardBonus;
	int m_GasHazardBonus;
	int m_MineHazardBonus;
	int m_ShockHazardBonus;
	int m_OrganicRoomMul; // percent, 100 = default
};

const SLcMapgenThemeProfile *LcGetMapgenThemeProfile(const char *pTheme);
bool LcMapgenThemeUsesClientTileset(const char *pTheme);

#endif
