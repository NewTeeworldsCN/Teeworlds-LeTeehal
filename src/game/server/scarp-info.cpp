/* 2024 TMJ */
#include "scrap-info.h"

CScrapInfo::CScrapInfo(CGameContext *pGameServer)
{
    dbg_msg("CScrapInfo", "CScrapInfo created");
    m_pGameServer = pGameServer;
}

void CScrapInfo::Init()
{
    RegisterScrap("牙膏", SCRAP_L1_TOOTHPASTE, ivec2(7, 14), ivec2(0, 1), ConToothpaste);
    RegisterScrap("梳子", SCRAP_L1_HAIRBRUSH, ivec2(8, 12), ivec2(6, 11), ConHairbrush);
    RegisterScrap("闪光弹", SCRAP_L1_FLASHBANG, ivec2(20, 24), ivec2(4, 8), ConFlashbang);
    RegisterScrap("酸黄瓜", SCRAP_L1_PICKLES, ivec2(9, 14), ivec2(6, 12), ConPickles);
    RegisterScrap("塑料鱼", SCRAP_L1_FISH, ivec2(6, 15), ivec2(0, 1), ConFish);
    RegisterScrap("金属板", SCRAP_L1_METALSHEET, ivec2(12, 14), ivec2(14, 20), ConMetalsheet);

    RegisterScrap("机器人玩具", SCRAP_L2_TOY, ivec2(31, 31), ivec2(8, 16), ConToy);
    RegisterScrap("魔方", SCRAP_L2_CUBE, ivec2(2, 30), ivec2(2, 4), ConCube);
    RegisterScrap("路牌", SCRAP_L2_SIGN, ivec2(17, 28), ivec2(28, 40), ConSign);
    RegisterScrap("药瓶", SCRAP_L2_PILL, ivec2(1, 30), ivec2(2, 4), ConPill);
    RegisterScrap("老式电话", SCRAP_L2_OLDPHONE, ivec2(19, 27), ivec2(14, 21), ConOldphone);
    RegisterScrap("遥控器", SCRAP_L2_REMOTE, ivec2(14, 24), ivec2(4, 11), ConRemote);

    RegisterScrap("魔法7号球", SCRAP_L3_MAGIC7BALL, ivec2(66, 66), ivec2(2, 3), ConMagic7ball);
    RegisterScrap("散弹枪", SCRAP_L3_SHOTGUN, ivec2(100, 180), ivec2(16, 16), ConShotgun);
    RegisterScrap("金条", SCRAP_L3_GOLDBAR, ivec2(100, 180), ivec2(46, 70), ConGoldbar);
    RegisterScrap("绚丽台灯", SCRAP_L3_LAMP, ivec2(67, 140), ivec2(38, 121), ConLamp);
    RegisterScrap("收银机", SCRAP_L3_CASHREGISTER, ivec2(70, 150), ivec2(53, 124), ConCashRegister);
    RegisterScrap("假牙", SCRAP_L3_TEETH, ivec2(42, 51), ivec2(2, 4), ConTeeth);
}

const char *CScrapInfo::GetScrapName(int Type)
{
    if(Type < 0 || Type >= NUM_SCRAPS)
        return "none";

    return m_aScrapInfo[Type].m_aName;
}

void CScrapInfo::RegisterScrap(const char aName[64], int ScrapID, ivec2 Value, ivec2 Weight, FCallbackScrap pData)
{
    if (ScrapID < 0 || ScrapID >= NUM_SCRAPS)
        return;

    str_copy(m_aScrapInfo[ScrapID].m_aName, aName, sizeof(m_aScrapInfo[ScrapID].m_aName));
    m_aScrapInfo[ScrapID].m_Value = Value;
    m_aScrapInfo[ScrapID].m_Weight = Weight;
    m_aScrapInfo[ScrapID].m_pData = pData;
}

void CScrapInfo::RandomScrap(int ScrapID, int &Value, int &Weight)
{
    int MinValue = m_aScrapInfo[ScrapID].m_Value.x;
    int MaxValue = m_aScrapInfo[ScrapID].m_Value.y;

    int MinWeight = m_aScrapInfo[ScrapID].m_Weight.x;
    int MaxWeight = m_aScrapInfo[ScrapID].m_Weight.y;

    std::random_device RandomDevice;
    std::mt19937_64 RandomEngine(RandomDevice());
    std::uniform_int_distribution<int> DistributionValue(MinValue, MaxValue);
    Value = DistributionValue(RandomEngine);

    std::uniform_int_distribution<int> DistributionWeight(MinWeight, MaxWeight);
    Weight = DistributionWeight(RandomEngine);
}

void CScrapInfo::Call(int ScrapID, int ScrapType, int ClientID)
{
    if(ScrapType < 0 || ScrapType > NUM_SCRAPS)
        return;
    
    m_aScrapInfo[ScrapType].m_pData(ClientID, ScrapID, m_pGameServer);
}