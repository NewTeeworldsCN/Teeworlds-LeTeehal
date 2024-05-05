#pragma once

#include "entities/scrap.h"
#include "gamecontext.h"
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
    int m_ID;
    int m_ScrapID;
    int m_Value;
    int m_Weight;
};

typedef void (*FCallbackScrap)(int ClientID, int ScrapID, void *pUser);

struct ScrapInfo
{
    char m_aName[64];
    ivec2 m_Value;
    ivec2 m_Weight;
    FCallbackScrap m_pData;
};

class CScrapInfo
{
public:
    CScrapInfo(CGameContext *pGameServer);

    void Init();
    void RegisterScrap(const char aName[64], int ScrapID, ivec2 Value, ivec2 Weight, FCallbackScrap pData);

    const char *GetScrapName(int Scrap);

    void RandomScrap(int ScrapID, int &Value, int &Weight);
    void Call(int ScrapID, int ScrapType, int ClientID);

    ScrapInfo m_aScrapInfo[NUM_SCRAPS];

public:
    static void ConToothpaste(int ClientID, int ScrapID, void *pUser);
    static void ConHairbrush(int ClientID, int ScrapID, void *pUser);
    static void ConFlashbang(int ClientID, int ScrapID, void *pUser);
    static void ConPickles(int ClientID, int ScrapID, void *pUser);
    static void ConFish(int ClientID, int ScrapID, void *pUser);
    static void ConMetalsheet(int ClientID, int ScrapID, void *pUser);
    
    static void ConToy(int ClientID, int ScrapID, void *pUser);
    static void ConCube(int ClientID, int ScrapID, void *pUser);
    static void ConSign(int ClientID, int ScrapID, void *pUser);
    static void ConPill(int ClientID, int ScrapID, void *pUser);
    static void ConOldphone(int ClientID, int ScrapID, void *pUser);
    static void ConRemote(int ClientID, int ScrapID, void *pUser);
    
    static void ConMagic7ball(int ClientID, int ScrapID, void *pUser);
    static void ConShotgun(int ClientID, int ScrapID, void *pUser);
    static void ConGoldbar(int ClientID, int ScrapID, void *pUser);
    static void ConLamp(int ClientID, int ScrapID, void *pUser);
    static void ConCashRegister(int ClientID, int ScrapID, void *pUser);
    static void ConTeeth(int ClientID, int ScrapID, void *pUser);

private:
    class CGameContext *m_pGameServer;
};