#include "scrap_info.h"
#include "../lc/expedition/balance.h"
#include "../entities/lc/monster.h"
#include <engine/shared/config.h>

void CScrapInfo::LCToothpaste(int ClientID, int ScrapID, void *pUser)
{
    CGameContext *pThis = (CGameContext *)pUser;
    CPlayer *pP = pThis->m_apPlayers[ClientID];

    int Health = (rand() % 3) - 1;
    if(Health > 0)
    {
        pP->GetCharacter()->IncreaseHealth(Health);
        pThis->SendChatTarget(ClientID, _("你喝下了牙膏，血量+{int:health}"), "health", &Health);
    }
    else if(Health < 0)
    {
        pP->GetCharacter()->TakeDamage(vec2(0, 0.3), -Health, ClientID, WEAPON_NINJA);
        pThis->SendChatTarget(ClientID, _("你喝下了牙膏，过期了.. 血量{int:health}"), "health", &Health);
    }
    else
    {
        pThis->SendChatTarget(ClientID, _("你喝下了牙膏，嗯，草莓味的"));
    }

    pP->EraseScrap(ScrapID);
}

void CScrapInfo::LCHairbrush(int ClientID, int ScrapID, void *pUser)
{
    CGameContext *pThis = (CGameContext *)pUser;
    CPlayer *pP = pThis->m_apPlayers[ClientID];

    pThis->SendChatTarget(ClientID, _("你用梳子梳了梳头发，神清气爽！"));
    pP->EraseScrap(ScrapID);
}

void CScrapInfo::LCFlashbang(int ClientID, int ScrapID, void *pUser)
{
    CGameContext *pThis = (CGameContext *)pUser;
    CPlayer *pP = pThis->m_apPlayers[ClientID];
    CCharacter *pChr = pP->GetCharacter();
    if(!pChr)
        return;

    vec2 Pos = pChr->m_Pos;
    pThis->StunMonstersInRadius(Pos, (float)GC_FLASHBANG_RADIUS, GC_FLASHBANG_STUN_SEC * pThis->Server()->TickSpeed());
    pThis->CreateSound(Pos, SOUND_GRENADE_EXPLODE);
    pThis->SendChatTarget(-1, _("{str:name} 使用了闪光弹！"), "name", pThis->Server()->ClientName(ClientID));
    pP->EraseScrap(ScrapID);
}

void CScrapInfo::LCPickles(int ClientID, int ScrapID, void *pUser)
{
    CGameContext *pThis = (CGameContext *)pUser;
    CPlayer *pP = pThis->m_apPlayers[ClientID];

    int Health = (rand() % 8) - 1;
    if(Health > 0)
    {
        pP->GetCharacter()->IncreaseHealth(Health);
        pThis->SendChatTarget(ClientID, _("你吃下了酸黄瓜.. 血量+{int:health}"), "health", &Health);
    }
    else if(Health < 0)
    {
        pP->GetCharacter()->TakeDamage(vec2(0, 0.3), -Health, ClientID, WEAPON_NINJA);
        pThis->SendChatTarget(ClientID, _("你吃下了酸黄瓜.. 靠，这根被男娘用过,血量{int:health}"), "health", &Health);
    }
    else
    {
        pThis->SendChatTarget(ClientID, _("你吃下了酸黄瓜.. 靠，这根用过了"));
    }

    pP->EraseScrap(ScrapID);
}

void CScrapInfo::LCFish(int ClientID, int ScrapID, void *pUser)
{
    CGameContext *pThis = (CGameContext *)pUser;
    CPlayer *pP = pThis->m_apPlayers[ClientID];

    int Health = (rand() % 6) - 3;
    if(Health > 0)
    {
        pP->GetCharacter()->IncreaseHealth(Health);
        pThis->SendChatTarget(ClientID, _("你使用了塑料鱼.. 爽！血量+{int:health}"), "health", &Health);
    }
    else if(Health < 0)
    {
        pP->GetCharacter()->TakeDamage(vec2(0, 0.3), -Health, ClientID, WEAPON_NINJA);
        pThis->SendChatTarget(ClientID, _("你使用了塑料鱼.. 啊啊，磨到了，血量{int:health}"), "health", &Health);
    }
    else
    {
        pThis->SendChatTarget(ClientID, _("你使用了塑料鱼.. 有点硬"));
    }

    pP->EraseScrap(ScrapID);
}

void CScrapInfo::LCMetalsheet(int ClientID, int ScrapID, void *pUser)
{
    CGameContext *pThis = (CGameContext *)pUser;
    CPlayer *pP = pThis->m_apPlayers[ClientID];
    
    int Armor = rand()%7+1;
    pP->GetCharacter()->IncreaseArmor(Armor);
    pThis->SendChatTarget(ClientID, _("你装备了金属板.. 防御+{int:health}"), "health", &Armor);

    pP->EraseScrap(ScrapID);
}

void CScrapInfo::LCSoda(int ClientID, int ScrapID, void *pUser)
{
    CGameContext *pThis = (CGameContext *)pUser;
    CPlayer *pP = pThis->m_apPlayers[ClientID];
    CCharacter *pChr = pP->GetCharacter();
    if(!pChr)
        return;

    pChr->m_SpeedBoostUntilTick = pThis->Server()->Tick() + GC_SCRAP_SODA_BOOST_SEC * pThis->Server()->TickSpeed();
    int Sec = GC_SCRAP_SODA_BOOST_SEC;
    pThis->SendChatTarget(ClientID, _("咕嘟咕嘟！{int:sec}秒内移动加速"), "sec", &Sec);
    pP->EraseScrap(ScrapID);
}

void CScrapInfo::LCWhistle(int ClientID, int ScrapID, void *pUser)
{
    CGameContext *pThis = (CGameContext *)pUser;
    CPlayer *pP = pThis->m_apPlayers[ClientID];
    CCharacter *pChr = pP->GetCharacter();
    if(!pChr)
        return;

    vec2 Pos = pChr->m_Pos;
    pThis->StunMonstersInRadius(Pos, (float)GC_SCRAP_WHISTLE_RADIUS, GC_SCRAP_WHISTLE_STUN_SEC * pThis->Server()->TickSpeed());
    pThis->CreateSound(Pos, SOUND_PLAYER_SPAWN);
    pThis->SendChatTarget(ClientID, _("尖锐哨声让附近怪物僵住了！"));
    pP->EraseScrap(ScrapID);
}

void CScrapInfo::LCToy(int ClientID, int ScrapID, void *pUser)
{
    CGameContext *pThis = (CGameContext *)pUser;
    CPlayer *pP = pThis->m_apPlayers[ClientID];

    int Health = (rand() % 8) - 3;
    if(Health > 0)
    {
        pP->GetCharacter()->IncreaseHealth(Health);
        pThis->SendChatTarget(ClientID, _("你使用了机器人玩具.. 爽！血量+{int:health}"), "health", &Health);
    }
    else if(Health < 0)
    {
        pP->GetCharacter()->TakeDamage(vec2(0, 0.3), -Health, ClientID, WEAPON_NINJA);
        pThis->SendChatTarget(ClientID, _("你使用了机器人玩具.. 啊啊，磨到了，血量{int:health}"), "health", &Health);
    }
    else
    {
        pThis->SendChatTarget(ClientID, _("你使用了机器人玩具.. 有点硬"));
    }

    pP->EraseScrap(ScrapID);
}

void CScrapInfo::LCCube(int ClientID, int ScrapID, void *pUser)
{
    CGameContext *pThis = (CGameContext *)pUser;
    CPlayer *pP = pThis->m_apPlayers[ClientID];
    CCharacter *pChr = pP->GetCharacter();
    if(!pChr)
        return;

    int Health = (rand() % 6) + 1;
    pChr->IncreaseHealth(Health);
    pThis->SendChatTarget(ClientID, _("魔方旋转出奇迹，血量+{int:health}"), "health", &Health);
    pP->EraseScrap(ScrapID);
}

void CScrapInfo::LCSign(int ClientID, int ScrapID, void *pUser)
{
    CGameContext *pThis = (CGameContext *)pUser;
    CPlayer *pP = pThis->m_apPlayers[ClientID];

    pThis->SendChatTarget(ClientID, _("你把路标拿在手上当武器."));

    pP->m_Hand = SCRAP_L2_SIGN;

    pP->EraseScrap(ScrapID);
}

void CScrapInfo::LCPill(int ClientID, int ScrapID, void *pUser)
{
    CGameContext *pThis = (CGameContext *)pUser;
    CPlayer *pP = pThis->m_apPlayers[ClientID];

    int Health = (rand() % 10) - 3;
    if(Health > 0)
    {
        pP->GetCharacter()->IncreaseHealth(Health);
        pThis->SendChatTarget(ClientID, _("你吃下了药.. 血量+{int:health}"), "health", &Health);
    }
    else if(Health < 0)
    {
        pP->GetCharacter()->TakeDamage(vec2(0, 0.3), -Health, ClientID, WEAPON_NINJA);
        pThis->SendChatTarget(ClientID, _("你吃下了药.. 过期了，血量{int:health}"), "health", &Health);
    }
    else
    {
        pThis->SendChatTarget(ClientID, _("你吃下了药.. 怎么是糖??"));
    }

    pP->EraseScrap(ScrapID);
}

void CScrapInfo::LCOldphone(int ClientID, int ScrapID, void *pUser)
{
    CGameContext *pThis = (CGameContext *)pUser;
    CPlayer *pP = pThis->m_apPlayers[ClientID];
    CCharacter *pChr = pP->GetCharacter();
    if(!pChr)
        return;

    ivec2 P = ivec2((int)(pChr->m_Pos.x/32), (int)(pChr->m_Pos.y/32));
    pThis->SendChatTarget(-1, _("电话那头传来 {str:name} 的坐标 [x:{int:x}, y:{int:y}]"), "name", pThis->Server()->ClientName(ClientID), "x", &P.x, "y", &P.y);
    pP->EraseScrap(ScrapID);
}

void CScrapInfo::LCRemote(int ClientID, int ScrapID, void *pUser)
{
    CGameContext *pThis = (CGameContext *)pUser;
    CPlayer *pP = pThis->m_apPlayers[ClientID];

    pThis->NewMonster(rand() % NUM_MONSTER_TYPES);
    pThis->NewMonster(rand() % NUM_MONSTER_TYPES);
    pThis->SendChatTarget(-1, _("{str:name} 按下了遥控器... 远处传来两阵异响"), "name", pThis->Server()->ClientName(ClientID));
    pP->EraseScrap(ScrapID);
}

void CScrapInfo::LCMedkit(int ClientID, int ScrapID, void *pUser)
{
    CGameContext *pThis = (CGameContext *)pUser;
    CPlayer *pP = pThis->m_apPlayers[ClientID];
    CCharacter *pChr = pP->GetCharacter();
    if(!pChr)
        return;

    int Heal = GC_SCRAP_MEDKIT_HEAL;
    pChr->IncreaseHealth(Heal);
    if(pChr->m_Freeze)
    {
        pChr->m_Freeze = false;
        pThis->SendChatTarget(ClientID, _("急救包解除了冻结，血量+{int:health}"), "health", &Heal);
    }
    else
    {
        pThis->SendChatTarget(ClientID, _("急救包包扎完毕，血量+{int:health}"), "health", &Heal);
    }
    pP->EraseScrap(ScrapID);
}

void CScrapInfo::LCMegaphone(int ClientID, int ScrapID, void *pUser)
{
    CGameContext *pThis = (CGameContext *)pUser;
    CPlayer *pP = pThis->m_apPlayers[ClientID];

    pThis->ScanMonsters(ClientID);
    pP->EraseScrap(ScrapID);
}

void CScrapInfo::LCMagic7ball(int ClientID, int ScrapID, void *pUser)
{
    CGameContext *pThis = (CGameContext *)pUser;
    CPlayer *pP = pThis->m_apPlayers[ClientID];

    pThis->SendChatTarget(ClientID, _("你把魔法7号球丢了出去，忍者变身！"));

    pP->GetCharacter()->GiveNinja();

    pP->EraseScrap(ScrapID);
}

void CScrapInfo::LCShotgun(int ClientID, int ScrapID, void *pUser)
{
    CGameContext *pThis = (CGameContext *)pUser;
    CPlayer *pP = pThis->m_apPlayers[ClientID];

    pThis->SendChatTarget(ClientID, _("你装备了散弹枪，2发子弹"));

    pP->GetCharacter()->GiveWeapon(WEAPON_SHOTGUN, 2);

    pP->EraseScrap(ScrapID);
}

void CScrapInfo::LCGoldbar(int ClientID, int ScrapID, void *pUser)
{
    CGameContext *pThis = (CGameContext *)pUser;
    CPlayer *pP = pThis->m_apPlayers[ClientID];

    int Bonus = GC_SCRAP_GOLDBAR_BONUS;
    g_Config.m_GcMoney += Bonus;
    pThis->SendChatTarget(ClientID, _("金条化作公司资金 +{int:money}元"), "money", &Bonus);
    pP->EraseScrap(ScrapID);
}

void CScrapInfo::LCLamp(int ClientID, int ScrapID, void *pUser)
{
    CGameContext *pThis = (CGameContext *)pUser;
    CPlayer *pP = pThis->m_apPlayers[ClientID];
    CCharacter *pChr = pP->GetCharacter();
    if(!pChr)
        return;

    int Armor = GC_SCRAP_LAMP_ARMOR;
    pChr->IncreaseArmor(Armor);
    pThis->SendChatTarget(ClientID, _("台灯照亮前路，防御+{int:armor}"), "armor", &Armor);
    pP->EraseScrap(ScrapID);
}

void CScrapInfo::LCCashRegister(int ClientID, int ScrapID, void *pUser)
{
    CGameContext *pThis = (CGameContext *)pUser;
    CPlayer *pP = pThis->m_apPlayers[ClientID];

    int Bonus = GC_SCRAP_CASH_MIN + rand() % (GC_SCRAP_CASH_MAX - GC_SCRAP_CASH_MIN + 1);
    g_Config.m_GcMoney += Bonus;
    pThis->SendChatTarget(ClientID, _("收银机吐出了 {int:money}元"), "money", &Bonus);
    pP->EraseScrap(ScrapID);
}

void CScrapInfo::LCTeeth(int ClientID, int ScrapID, void *pUser)
{
    CGameContext *pThis = (CGameContext *)pUser;
    CPlayer *pP = pThis->m_apPlayers[ClientID];
    CCharacter *pChr = pP->GetCharacter();
    if(!pChr)
        return;

    pChr->TakeDamage(vec2(0, 0.3), 1, ClientID, WEAPON_NINJA);
    pThis->SendChatTarget(ClientID, _("恶心！你咬到了自己的舌头"));
    pP->EraseScrap(ScrapID);
}

void CScrapInfo::LCLuckycat(int ClientID, int ScrapID, void *pUser)
{
    CGameContext *pThis = (CGameContext *)pUser;
    CPlayer *pP = pThis->m_apPlayers[ClientID];

    pP->m_ScrapValueBonusPercent = GC_SCRAP_LUCKYCAT_BONUS_PERCENT;
    pThis->SendChatTarget(ClientID, _("招财猫摇了摇爪子... 下一件捡到的废品价值翻倍！"));
    pP->EraseScrap(ScrapID);
}

void CScrapInfo::LCBoombox(int ClientID, int ScrapID, void *pUser)
{
    CGameContext *pThis = (CGameContext *)pUser;
    CPlayer *pP = pThis->m_apPlayers[ClientID];
    CCharacter *pChr = pP->GetCharacter();
    if(!pChr)
        return;

    vec2 Pos = pChr->m_Pos;
    int StunTicks = GC_SCRAP_BOOMBOX_STUN_SEC * pThis->Server()->TickSpeed();
    for(int i = 0; i < MAX_MONSTERS; i++)
    {
        CMonster *pMonster = pThis->m_apMonsters[i];
        if(pMonster)
            pMonster->Stun(StunTicks);
    }
    pThis->CreateSound(Pos, SOUND_GRENADE_EXPLODE);
    pThis->SendChatTarget(-1, _("{str:name} 的音响震彻全图！怪物们暂时动弹不得"), "name", pThis->Server()->ClientName(ClientID));
    pP->EraseScrap(ScrapID);
}