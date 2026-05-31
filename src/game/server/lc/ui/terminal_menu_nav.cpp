#include "terminal_menu.h"
#include "../expedition/balance.h"
#include "../../scrap/scrap_info.h"
#include "terminal_actions.h"

bool LcTerminalInfoScrollPage(int Page)
{
	return Page == LC_PAGE_TEAM || Page == LC_PAGE_HELP_TUT || Page == LC_PAGE_CREDITS
		|| Page == LC_PAGE_GUIDE_MONSTERS || Page == LC_PAGE_GUIDE_SCRAP
		|| LcTerminalIsGuideMonsterDetail(Page) || LcTerminalIsGuideStoreDetail(Page)
		|| LcTerminalIsGuideScrapDetail(Page);
}

static int GuideScrapTierPage(int ScrapID)
{
	if(ScrapID < END_SCRAP_L1)
		return LC_PAGE_GUIDE_SCRAP_L1;
	if(ScrapID < END_SCRAP_L2)
		return LC_PAGE_GUIDE_SCRAP_L2;
	return LC_PAGE_GUIDE_SCRAP_L3;
}

static int GuideStoreCategoryPage(int ItemIndex)
{
	switch(ItemIndex)
	{
	case 0:
	case 2:
	case 3:
	case 4:
		return LC_PAGE_GUIDE_STORE_SUPPLIES;
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
		return LC_PAGE_GUIDE_STORE_WEAPONS;
	default:
		return LC_PAGE_GUIDE_STORE_FIELD;
	}
}

int LcTerminalParentPage(int Page)
{
	switch(Page)
	{
	case LC_PAGE_GUIDE_STORE:
	case LC_PAGE_GUIDE_MONSTERS:
	case LC_PAGE_GUIDE_SCRAP:
		return LC_PAGE_GUIDE;
	case LC_PAGE_GUIDE_STORE_SUPPLIES:
	case LC_PAGE_GUIDE_STORE_WEAPONS:
	case LC_PAGE_GUIDE_STORE_FIELD:
		return LC_PAGE_GUIDE_STORE;
	case LC_PAGE_GUIDE_SCRAP_L1:
	case LC_PAGE_GUIDE_SCRAP_L2:
	case LC_PAGE_GUIDE_SCRAP_L3:
		return LC_PAGE_GUIDE_SCRAP;
	case LC_PAGE_SHIP_CARGO:
		return LC_PAGE_MAIN;
	case LC_PAGE_WELCOME:
	case LC_PAGE_TEAM:
	case LC_PAGE_HELP_TUT:
	case LC_PAGE_CREDITS:
		return LC_PAGE_MAIN;
	case LC_PAGE_INVENTORY_USE:
	case LC_PAGE_INVENTORY_DROP:
		return LC_PAGE_INVENTORY;
	default:
		break;
	}

	if(LcTerminalIsGuideMonsterDetail(Page))
		return LC_PAGE_GUIDE_MONSTERS;
	if(LcTerminalIsGuideScrapDetail(Page))
		return GuideScrapTierPage(Page - LC_PAGE_GUIDE_SCRAP_DETAIL_BASE);
	if(LcTerminalIsGuideStoreDetail(Page))
		return GuideStoreCategoryPage(Page - LC_PAGE_GUIDE_STORE_DETAIL_BASE);
	if(LcTerminalIsInventoryDetail(Page))
		return LC_PAGE_INVENTORY;
	if(Page == LC_PAGE_MAIN)
		return -1;
	return LC_PAGE_MAIN;
}

bool LcTerminalIsValidPage(int Page)
{
	switch(Page)
	{
	case LC_PAGE_MAIN:
	case LC_PAGE_STORE:
	case LC_PAGE_AIRCRAFT:
	case LC_PAGE_GUIDE:
	case LC_PAGE_GUIDE_STORE:
	case LC_PAGE_GUIDE_MONSTERS:
	case LC_PAGE_GUIDE_SCRAP:
	case LC_PAGE_MOON:
	case LC_PAGE_INVENTORY:
	case LC_PAGE_GUIDE_STORE_SUPPLIES:
	case LC_PAGE_THREATS:
	case LC_PAGE_GUIDE_STORE_WEAPONS:
	case LC_PAGE_GUIDE_STORE_FIELD:
	case LC_PAGE_GUIDE_SCRAP_L1:
	case LC_PAGE_GUIDE_SCRAP_L2:
	case LC_PAGE_GUIDE_SCRAP_L3:
	case LC_PAGE_SHIP_CARGO:
	case LC_PAGE_INVENTORY_USE:
	case LC_PAGE_INVENTORY_DROP:
	case LC_PAGE_WELCOME:
	case LC_PAGE_TEAM:
	case LC_PAGE_HELP_TUT:
	case LC_PAGE_CREDITS:
		return true;
	default:
		break;
	}

	if(LcTerminalIsGuideMonsterDetail(Page))
		return Page - LC_PAGE_GUIDE_MONSTER_BASE < LC_PAGE_GUIDE_MONSTER_END - LC_PAGE_GUIDE_MONSTER_BASE;
	if(LcTerminalIsGuideScrapDetail(Page))
		return Page - LC_PAGE_GUIDE_SCRAP_DETAIL_BASE < NUM_SCRAPS;
	if(LcTerminalIsGuideStoreDetail(Page))
	{
		int Count = 0;
		LcGetStoreItems(&Count);
		return Page - LC_PAGE_GUIDE_STORE_DETAIL_BASE < Count;
	}
	if(LcTerminalIsInventoryDetail(Page))
		return Page - LC_PAGE_INVENTORY_DETAIL_BASE < GC_MAX_SCRAP_SLOTS;
	return false;
}
