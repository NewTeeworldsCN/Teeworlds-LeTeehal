#include "terminal_actions.h"

#include "../expedition/balance.h"
#include "../../entities/lc/monster.h"

#include <teeuniverses/components/localization.h>

static const SLcStoreItem s_aStoreItems[] =
{
	{"buy_time", "延长班次", GC_STORE_TIME_COST},
	{"buy_flash", "闪光弹", GC_STORE_FLASH_COST},
	{"buy_armor", "防护服", GC_STORE_ARMOR_COST},
	{"buy_health", "生命保障", GC_STORE_HEALTH_COST},
	{"buy_aircraft", "飞行器", GC_STORE_AIRCRAFT_COST},
	{"buy_shotgun", "散弹枪", GC_STORE_SHOTGUN_COST},
	{"buy_rifle", "激光枪", GC_STORE_RIFLE_COST},
	{"buy_grenade", "榴弹", GC_STORE_GRENADE_COST},
	{"buy_gun", "手枪", GC_STORE_GUN_COST},
	{"buy_ninja", "忍者刀", GC_STORE_NINJA_COST},
	{"buy_medkit", "急救包", GC_STORE_MEDKIT_COST},
	{"buy_whistle", "驱虫哨", GC_STORE_WHISTLE_COST},
	{"buy_soda", "汽水", GC_STORE_SODA_COST},
	{"buy_megaphone", "扩音器", GC_STORE_MEGAPHONE_COST},
	{"buy_boombox", "音响", GC_STORE_BOOMBOX_COST},
	{"buy_remote", "遥控器", GC_STORE_REMOTE_COST},
};

const SLcStoreItem *LcGetStoreItems(int *pCount)
{
	if(pCount)
		*pCount = (int)(sizeof(s_aStoreItems) / sizeof(s_aStoreItems[0]));
	return s_aStoreItems;
}

const char *LcStoreItemName(const char *pCmd)
{
	if(!pCmd)
		return "";

	for(int i = 0; i < (int)(sizeof(s_aStoreItems) / sizeof(s_aStoreItems[0])); i++)
	{
		if(str_comp(pCmd, s_aStoreItems[i].m_pCmd) == 0)
			return s_aStoreItems[i].m_pNameKey;
	}
	return pCmd;
}

const char *LcMoonRouteLabel(int Moon)
{
	switch(Moon)
	{
	case 0: return _("☞ 实验 [★]");
	case 1: return _("☞ 保障 [★★]");
	case 2: return _("☞ 誓约 [★★★]");
	case 3: return _("☞ 攻势 [★★★★]");
	case 4: return _("☞ 泰坦 [★★★★★]");
	default: return _("☞ 未知路线");
	}
}

const char *LcMonsterName(int Type)
{
	switch(Type)
	{
	case TYPE_PULLHANDLE: return _("布条怪");
	case TYPE_SATIETY: return _("囤积虫");
	case TYPE_LEEK_BOX: return _("孢子蜥");
	case TYPE_BUG: return _("蔓背怪");
	case TYPE_FEAR: return _("弹簧头");
	case TYPE_HUNTER: return _("猛禽");
	case TYPE_BOMBER: return _("爆壳虫");
	case TYPE_LEECH: return _("吸盘怪");
	case TYPE_STALKER: return _("潜追者");
	default: return _("未知怪物");
	}
}
