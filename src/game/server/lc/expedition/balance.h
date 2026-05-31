#ifndef GAME_SERVER_GAME_BALANCE_H
#define GAME_SERVER_GAME_BALANCE_H

#include <stdlib.h>

int LcMoonScrapMultiplierPercent();
int LcMoonMonsterMultiplierPercent();

static const int GC_STARTING_QUOTA = 280;
static const int GC_STARTING_DAYS = 4;
static const int GC_STARTING_ROUNDS = 1;

static const int GC_QUOTA_BASE_PER_ROUND = 280;
static const int GC_QUOTA_BONUS_PER_ROUND = 25;

static const int GC_SCRAP_L1_MIN = 60;
static const int GC_SCRAP_L1_PER_ROUND = 3;
static const int GC_SCRAP_L2_MAX = 30;
static const int GC_SCRAP_L2_PER_ROUND = 5;
static const int GC_SCRAP_L3_MAX = 20;
static const int GC_SCRAP_L3_PER_ROUND = 1;

inline int BalanceNextQuota(int Rounds)
{
	return Rounds * GC_QUOTA_BASE_PER_ROUND + GC_QUOTA_BONUS_PER_ROUND * Rounds;
}

inline int BalanceScrapL1Count(int Rounds)
{
	int Count = Rounds * GC_SCRAP_L1_PER_ROUND;
	Count = Count < GC_SCRAP_L1_MIN ? GC_SCRAP_L1_MIN : Count;
	return Count * LcMoonScrapMultiplierPercent() / 100;
}

inline int BalanceScrapL2Count(int Rounds)
{
	int Count = Rounds * GC_SCRAP_L2_PER_ROUND;
	Count = Count > GC_SCRAP_L2_MAX ? GC_SCRAP_L2_MAX : Count;
	return Count * LcMoonScrapMultiplierPercent() / 100;
}

inline int BalanceScrapL3Count(int Rounds)
{
	int Count = Rounds * GC_SCRAP_L3_PER_ROUND;
	Count = Count > GC_SCRAP_L3_MAX ? GC_SCRAP_L3_MAX : Count;
	return Count * LcMoonScrapMultiplierPercent() / 100;
}

static const int GC_MONSTER_SPAWN_MIN_SEC = 18;
static const int GC_MONSTER_SPAWN_MAX_SEC = 50;

static const int GC_BOSS_ROUND_INTERVAL = 5;
static const int GC_BOSS_HEALTH = 12;
static const int GC_BOSS_KILL_BONUS = 80;

static const int GC_FLASHBANG_RADIUS = 320;
static const int GC_FLASHBANG_STUN_SEC = 3;
static const int GC_DISCONNECT_PERSIST_SEC = 120;
static const int GC_SCRAP_GOLDBAR_BONUS = 25;
static const int GC_SCRAP_CASH_MIN = 10;
static const int GC_SCRAP_CASH_MAX = 40;
static const int GC_SCRAP_LAMP_ARMOR = 3;

static const int GC_SCRAP_SODA_BOOST_SEC = 12;
static const int GC_SCRAP_WHISTLE_RADIUS = 220;
static const int GC_SCRAP_WHISTLE_STUN_SEC = 3;
static const int GC_SCRAP_MEDKIT_HEAL = 5;
static const int GC_SCRAP_BOOMBOX_STUN_SEC = 4;
static const int GC_SCRAP_LUCKYCAT_BONUS_PERCENT = 200;

static const int GC_MAX_SCRAP_SLOTS = 16; // Hmm...
static const int GC_AIRCRAFT_MAX_CARGO = 16;
static const float GC_SHIP_SCRAP_MERGE_RADIUS = 72.f;

static const int GC_STORE_TIME_COST = 150;
static const int GC_STORE_TIME_BONUS_SEC = 120;
static const int GC_STORE_FLASH_COST = 80;
static const int GC_STORE_ARMOR_COST = 100;
static const int GC_STORE_ARMOR_BONUS = 3;

static const int GC_STORE_SHOTGUN_COST = 120;
static const int GC_STORE_SHOTGUN_AMMO = 8;
static const int GC_STORE_RIFLE_COST = 140;
static const int GC_STORE_RIFLE_AMMO = 5;
static const int GC_STORE_GRENADE_COST = 160;
static const int GC_STORE_GRENADE_AMMO = 3;
static const int GC_STORE_MEDKIT_COST = 90;
static const int GC_STORE_WHISTLE_COST = 70;
static const int GC_STORE_SODA_COST = 60;
static const int GC_STORE_GUN_COST = 95;
static const int GC_STORE_GUN_AMMO = 12;
static const int GC_STORE_NINJA_COST = 110;
static const int GC_STORE_HEALTH_COST = 85;
static const int GC_STORE_HEALTH_BONUS = 4;
static const int GC_STORE_MEGAPHONE_COST = 85;
static const int GC_STORE_BOOMBOX_COST = 140;
static const int GC_STORE_REMOTE_COST = 95;
static const int GC_STORE_AIRCRAFT_COST = 220;

static const int GC_HUNTER_FIRE_RANGE = 520;
static const int GC_HUNTER_FIRE_SEC = 2;
static const int GC_BOMBER_DETONATE_RANGE = 72;
static const int GC_LEECH_DRAIN_RANGE = 120;
static const int GC_LEECH_HOOK_RANGE = 800;
static const int GC_STALKER_ATTACK_SEC = 1;
static const int GC_STALKER_LUNGE_EXTRA = 40;

static const int GC_TURRET_RANGE = 640;
static const int GC_TURRET_LOSE_RANGE = 780;
static const int GC_TURRET_IDLE_ROT_DEG = 22;
static const int GC_TURRET_TRACK_ROT_DEG = 95;
static const int GC_TURRET_AIM_CONE_DEG = 16;
static const int GC_TURRET_FIRE_CONE_DEG = 5;
static const int GC_TURRET_FIRE_SEC = 1;

static const int GC_MONSTER_PIT_CHECK_TILES = 10;
static const int GC_MONSTER_BOTTOM_MARGIN_TILES = 8;
static const int GC_MONSTER_FLY_GRAVITY_CANCEL_PERCENT = 88;
static const int GC_MONSTER_WALL_CRAWL_SPEED = 220;
static const int GC_MONSTER_WALL_CLIMB_SPEED = 180;
static const int GC_MONSTER_HOOK_SEARCH_TILES = 18;

static const int GC_COILHEAD_STARE_SEC = 2;
static const int GC_COILHEAD_RANGE = 400;
static const int GC_COILHEAD_KILL_RANGE = 72;
static const int GC_COILHEAD_SOUND_SEC = 2;
static const int GC_COILHEAD_SPEED_PERCENT = 55;
static const int GC_COILHEAD_WALL_CRAWL_SPEED = 120;
static const int GC_COILHEAD_WALL_CLIMB_SPEED = 100;

static const int GC_HAZARD_GAS_DAMAGE = 1;
static const int GC_HAZARD_MINE_DAMAGE = 5;
static const int GC_HAZARD_SHOCK_DAMAGE = 2;
static const int GC_HAZARD_SPIKE_DAMAGE = 3;
static const int GC_HAZARD_GAS_TICK_SEC = 2;
static const int GC_HAZARD_SHOCK_TICK_SEC = 2;
static const int GC_BRACKEN_AMBUSH_RANGE = 280;
static const int GC_BRACKEN_GRASS_DAMAGE_MULT = 2;

static const int GC_PLAYER_HAMMER_MONSTER_DMG = 2;
static const int GC_PLAYER_SIGN_MONSTER_BONUS = 2;
static const int GC_PLAYER_GUN_MONSTER_DMG = 2;
static const int GC_PLAYER_SHOTGUN_MONSTER_DMG = 2;
static const int GC_PLAYER_NINJA_MONSTER_DMG = 6;
static const int GC_PLAYER_GRENADE_MONSTER_DMG = 8;
static const int GC_FROZEN_MONSTER_DAMAGE_PERCENT = 150;

static const int GC_SHIP_HINT_DIST = 1400;
static const int GC_SHIP_COMPASS_SEC = 25;
static const float GC_SHIP_MONSTER_REPEL_BUFFER = 40.f;
static const float GC_SHIP_MONSTER_BOUNCE = 12.f;
static const int GC_SCAN_LINK_SEC = 2;

inline int BalanceMonsterSpawnInterval(int TickSpeed, int PlayersAlive)
{
	int Range = GC_MONSTER_SPAWN_MAX_SEC - GC_MONSTER_SPAWN_MIN_SEC;
	if(Range < 1)
		Range = 1;
	int PlayerScale = PlayersAlive > 0 ? PlayersAlive : 1;
	if(PlayerScale > 8)
		PlayerScale = 8;
	int Seconds = GC_MONSTER_SPAWN_MAX_SEC - (PlayerScale - 1) * 2;
	if(Seconds < GC_MONSTER_SPAWN_MIN_SEC)
		Seconds = GC_MONSTER_SPAWN_MIN_SEC;
	return TickSpeed * (Seconds + rand() % Range) * 100 / LcMoonMonsterMultiplierPercent();
}

#endif
