#include "scrap-info.h"

void CScrapInfo::ConToothpaste(int ClientID, int ScrapID, void *pUser)
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
        pThis->SendChatTarget(ClientID, _("你喝下了牙膏，hmm, 草莓味的"), "health", &Health);
    }

    pP->EraseScrap(ScrapID);
}

void CScrapInfo::ConHairbrush(int ClientID, int ScrapID, void *pUser)
{
    CGameContext *pThis = (CGameContext *)pUser;

    pThis->SendChatTarget(ClientID, _("你用梳子梳了梳头发，神清气爽！"));
}

void CScrapInfo::ConFlashbang(int ClientID, int ScrapID, void *pUser)
{
    CGameContext *pThis = (CGameContext *)pUser;
    vec2 Pos = pThis->GetPlayerChar(ClientID)->m_Pos;

    pThis->SendChatTarget(ClientID, _("A1高闪来一个好吗！"));
    pThis->SendChatTarget(ClientID, _("...不好(闪光瓶已损坏)"));
}

void CScrapInfo::ConPickles(int ClientID, int ScrapID, void *pUser)
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

void CScrapInfo::ConFish(int ClientID, int ScrapID, void *pUser)
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

void CScrapInfo::ConMetalsheet(int ClientID, int ScrapID, void *pUser)
{
    CGameContext *pThis = (CGameContext *)pUser;
    CPlayer *pP = pThis->m_apPlayers[ClientID];

    int Armor = pP->m_vScraps[ScrapID]->m_Weight / 2;
    pP->GetCharacter()->IncreaseArmor(Armor);
    pThis->SendChatTarget(ClientID, _("你装备了金属板.. 防御+{int:health}"), "health", &Armor);

    pP->EraseScrap(ScrapID);
}

void CScrapInfo::ConToy(int ClientID, int ScrapID, void *pUser)
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

void CScrapInfo::ConCube(int ClientID, int ScrapID, void *pUser)
{
    CGameContext *pThis = (CGameContext *)pUser;

    CPlayer *pP = pThis->m_apPlayers[ClientID];

    pThis->SendChatTarget(ClientID, _("我是魔方大佬！"));
    
    pP->EraseScrap(ScrapID);
}

void CScrapInfo::ConSign(int ClientID, int ScrapID, void *pUser)
{
    CGameContext *pThis = (CGameContext *)pUser;
    CPlayer *pP = pThis->m_apPlayers[ClientID];

    pThis->SendChatTarget(ClientID, _("你把路标拿在手上当武器."));

    pP->m_Hand = ScrapID;
}

void CScrapInfo::ConPill(int ClientID, int ScrapID, void *pUser)
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

void CScrapInfo::ConOldphone(int ClientID, int ScrapID, void *pUser)
{
    CGameContext *pThis = (CGameContext *)pUser;
    CPlayer *pP = pThis->m_apPlayers[ClientID];

    pThis->SendChatTarget(ClientID, _("你拨打了电话... But nobody come"));
}

void CScrapInfo::ConRemote(int ClientID, int ScrapID, void *pUser)
{
    CGameContext *pThis = (CGameContext *)pUser;
    CPlayer *pP = pThis->m_apPlayers[ClientID];

    pThis->SendChatTarget(ClientID, _("你按下了遥控器... 你仿佛听到远处有人在哀号"));
}

void CScrapInfo::ConMagic7ball(int ClientID, int ScrapID, void *pUser)
{
    CGameContext *pThis = (CGameContext *)pUser;
    CPlayer *pP = pThis->m_apPlayers[ClientID];

    pThis->SendChatTarget(ClientID, _("你把魔法7号球丢了出去，忍者变身！"));

    pP->GetCharacter()->GiveNinja();

    pP->EraseScrap(ScrapID);
}

void CScrapInfo::ConShotgun(int ClientID, int ScrapID, void *pUser)
{
    CGameContext *pThis = (CGameContext *)pUser;
    CPlayer *pP = pThis->m_apPlayers[ClientID];

    pThis->SendChatTarget(ClientID, _("你装备了散弹枪，2发子弹"));

    pP->GetCharacter()->GiveWeapon(WEAPON_SHOTGUN, 2);

    pP->EraseScrap(ScrapID);
}

void CScrapInfo::ConGoldbar(int ClientID, int ScrapID, void *pUser)
{
    CGameContext *pThis = (CGameContext *)pUser;
    CPlayer *pP = pThis->m_apPlayers[ClientID];

    pThis->SendChatTarget(ClientID, _("这是金金金金金金金金条"));
}

void CScrapInfo::ConLamp(int ClientID, int ScrapID, void *pUser)
{
    CGameContext *pThis = (CGameContext *)pUser;
    CPlayer *pP = pThis->m_apPlayers[ClientID];

    pThis->SendChatTarget(ClientID, _("绚丽台灯，我想在家里摆一个"));
}

void CScrapInfo::ConCashRegister(int ClientID, int ScrapID, void *pUser)
{
    CGameContext *pThis = (CGameContext *)pUser;
    CPlayer *pP = pThis->m_apPlayers[ClientID];

    pThis->SendChatTarget(ClientID, _("这是收银机，里面有银银银银银银银和金金金金金金金"));
}

void CScrapInfo::ConTeeth(int ClientID, int ScrapID, void *pUser)
{
    CGameContext *pThis = (CGameContext *)pUser;
    
    pThis->SendChatTarget(ClientID, _("老年人用的假牙，恶心"));
}