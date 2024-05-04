#pragma once

#include "entities/scrap.h"
#include <vector>
#include <algorithm>

enum
{
    SCRAP_L1_TOOTHPASTE = 0, // 牙刷
    SCRAP_L1_HAIRBRUSH,      // 梳子
    SCRAP_L1_FLASHBANG,      // 闪光弹
    SCRAP_L1_PICKLES,        // 酸黄瓜
    SCRAP_L1_FISH,           // 塑料鱼
    SCRAP_L1_METALSHEET,     // 金属板
    END_SCRAP_L1,

    SCRAP_L2_TOY = END_SCRAP_L1, // 机器人玩具
    SCRAP_L2_CUBE,               // 魔方
    SCRAP_L2_SIGN,               // 路牌
    SCRAP_L2_PILL,               // 药瓶
    SCRAP_L2_OLDPHONE,           // 老式电话
    SCRAP_L2_REMOTE,             // 遥控器
    END_SCRAP_L2,

    SCRAP_L3_MAGIC7BALL = END_SCRAP_L2, // 魔法7号球
    SCRAP_L3_SHOTGUN,                   // 散弹枪
    SCRAP_L3_GOLDBAR,                   // 金条
    SCRAP_L3_LAMP,                      // 绚丽台灯
    SCRAP_L3_CASHREGISTER,              // 收银机
    SCRAP_L3_TEETH,                     // 假牙
    END_SCRAP_L3,

    NUM_SCRAPS = END_SCRAP_L3,

    SCRAP_LEVEL1 = 0,
    SCRAP_LEVEL2,
    SCRAP_LEVEL3,
};

struct Scrap
{
    Scrap() { };
    int m_ScrapID;
    int m_Value;
    int m_Weight;
};

struct ScrapInfo
{
    char m_aName[64];
    ivec2 m_Value;
    ivec2 m_Weight;
};

class CScrapInfo
{
public:
    CScrapInfo();

    void Init();
    void RegisterScrap(const char aName[64], int ScrapID, ivec2 Value, ivec2 Weight);

    const char *GetScrapName(int Scrap);

    void RandomScrap(int ScrapID, int &Value, int &Weight);

    ScrapInfo m_aScrapInfo[NUM_SCRAPS];
};