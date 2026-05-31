/* 2024 TMJ */
#include "scrap_info.h"
#include "../lc/ui/guide.h"

CScrapInfo::CScrapInfo(CGameContext *pGameServer)
{
    dbg_msg("CScrapInfo", "CScrapInfo created");
    m_pGameServer = pGameServer;
}

void CScrapInfo::Init()
{
    RegisterScrap("瓶盖", SCRAP_L1_TOOTHPASTE, ivec2(1, 7), ivec2(0, 1), LCToothpaste);
    RegisterScrap("发刷", SCRAP_L1_HAIRBRUSH, ivec2(2, 6), ivec2(6, 11), LCHairbrush);
    RegisterScrap("停车标志", SCRAP_L1_FLASHBANG, ivec2(4, 8), ivec2(4, 8), LCFlashbang);
    RegisterScrap("泡菜罐", SCRAP_L1_PICKLES, ivec2(9, 14), ivec2(6, 12), LCPickles);
    RegisterScrap("玩具鱼", SCRAP_L1_FISH, ivec2(2, 7), ivec2(0, 1), LCFish);
    RegisterScrap("大型螺栓", SCRAP_L1_METALSHEET, ivec2(2, 9), ivec2(14, 20), LCMetalsheet);
    RegisterScrap("能量汽水", SCRAP_L1_SODA, ivec2(3, 8), ivec2(2, 5), LCSoda);
    RegisterScrap("驱虫哨", SCRAP_L1_WHISTLE, ivec2(5, 12), ivec2(1, 3), LCWhistle);

    RegisterScrap("机器人玩具", SCRAP_L2_TOY, ivec2(15, 20), ivec2(8, 16), LCToy);
    RegisterScrap("魔方", SCRAP_L2_CUBE, ivec2(2, 30), ivec2(2, 4), LCCube);
    RegisterScrap("路标", SCRAP_L2_SIGN, ivec2(10, 28), ivec2(28, 40), LCSign);
    RegisterScrap("药瓶", SCRAP_L2_PILL, ivec2(1, 30), ivec2(2, 4), LCPill);
    RegisterScrap("老式电话", SCRAP_L2_OLDPHONE, ivec2(19, 27), ivec2(14, 21), LCOldphone);
    RegisterScrap("遥控器", SCRAP_L2_REMOTE, ivec2(14, 24), ivec2(4, 11), LCRemote);
    RegisterScrap("急救包", SCRAP_L2_MEDKIT, ivec2(18, 32), ivec2(8, 14), LCMedkit);
    RegisterScrap("扩音器", SCRAP_L2_MEGAPHONE, ivec2(12, 28), ivec2(6, 12), LCMegaphone);

    RegisterScrap("魔法八音盒", SCRAP_L3_MAGIC7BALL, ivec2(66, 66), ivec2(2, 3), LCMagic7ball);
    RegisterScrap("猎枪", SCRAP_L3_SHOTGUN, ivec2(100, 180), ivec2(16, 16), LCShotgun);
    RegisterScrap("金条", SCRAP_L3_GOLDBAR, ivec2(100, 180), ivec2(46, 70), LCGoldbar);
    RegisterScrap("台灯", SCRAP_L3_LAMP, ivec2(67, 140), ivec2(38, 121), LCLamp);
    RegisterScrap("收银机", SCRAP_L3_CASHREGISTER, ivec2(70, 150), ivec2(53, 124), LCCashRegister);
    RegisterScrap("假牙", SCRAP_L3_TEETH, ivec2(42, 51), ivec2(2, 4), LCTeeth);
    RegisterScrap("招财猫", SCRAP_L3_LUCKYCAT, ivec2(80, 160), ivec2(4, 8), LCLuckycat);
    RegisterScrap("音响", SCRAP_L3_BOOMBOX, ivec2(55, 120), ivec2(20, 35), LCBoombox);
}

const char *CScrapInfo::GetScrapName(int Type)
{
    if(Type < 0 || Type >= NUM_SCRAPS)
        return "none";

    return m_aScrapInfo[Type].m_aName;
}

const char *CScrapInfo::GetScrapDesc(int Type)
{
    if(Type < 0 || Type >= NUM_SCRAPS)
        return "";
    return LcScrapDesc(Type);
}

const char *CScrapInfo::GetScrapDescShort(int Type)
{
    if(Type < 0 || Type >= NUM_SCRAPS)
        return "";
    return LcScrapDescShort(Type);
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

    if(MinValue > MaxValue)
    {
        int Temp = MinValue;
        MinValue = MaxValue;
        MaxValue = Temp;
    }
    if(MinWeight > MaxWeight)
    {
        int Temp = MinWeight;
        MinWeight = MaxWeight;
        MaxWeight = Temp;
    }

    Value = MinValue + (MaxValue > MinValue ? rand() % (MaxValue - MinValue + 1) : 0);
    Weight = MinWeight + (MaxWeight > MinWeight ? rand() % (MaxWeight - MinWeight + 1) : 0);
}

void CScrapInfo::Call(int ScrapID, int ScrapType, int ClientID)
{
    if(ScrapType < 0 || ScrapType >= NUM_SCRAPS)
        return;
    
    m_aScrapInfo[ScrapType].m_pData(ClientID, ScrapID, m_pGameServer);
}