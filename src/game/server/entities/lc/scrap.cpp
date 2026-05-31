#include <game/generated/protocol.h>
#include <game/server/core/gamecontext.h>
#include <base/math.h>
#include <game/server/lc/expedition/balance.h>
#include <game/server/scrap/scrap_info.h>

#include "scrap.h"

void Rotate(vec2 *vertex, float x_orig, float y_orig, float angle)
{
    // FUCK THIS MATH
    float s = sin(angle);
    float c = cos(angle);

    vertex->x -= x_orig;
    vertex->y -= y_orig;

    float xnew = vertex->x * c - vertex->y * s;
    float ynew = vertex->x * s + vertex->y * c;

    vertex->x = xnew + x_orig;
    vertex->y = ynew + y_orig;
}

CScrap::CScrap(CGameWorld *pGameWorld, int Level, vec2 Pos, bool Random, bool InShip, Scrap S) : CEntity(pGameWorld, CGameWorld::ENTTYPE_SCRAP)
{
    m_Pos = Pos;
    m_Level = Level;
    m_Random = Random;
    m_InShip = InShip;
    if(InShip)
        S.m_InShip = true;

    switch (Level)
    {
    case ENTITY_SCRAP_L1:
        m_ScrapType = rand()%END_SCRAP_L1;
        break;

    case ENTITY_SCRAP_L2:
        m_ScrapType = (rand()%(END_SCRAP_L2-END_SCRAP_L1))+END_SCRAP_L1;
        break;

    case ENTITY_SCRAP_L3:
        m_ScrapType = (rand()%(END_SCRAP_L3-END_SCRAP_L2))+END_SCRAP_L2;
        break;
    
    default:
        m_ScrapType = (rand()%(END_SCRAP_L2-END_SCRAP_L1))+END_SCRAP_L1;
        break;
    }
    if(Random)
        GameServer()->ScrapInfo()->RandomScrap(m_ScrapType, m_ScrapValue, m_Weight);
    else
    {
        m_ScrapType = S.m_ScrapID;
        m_ScrapValue = S.m_Value;
        m_Weight = S.m_Weight;
        m_WasInShip = S.m_InShip;
    }
    m_Angle = rand() % 360;
    m_Vel = vec2(0, 0);
    m_pWorld = &GameWorld()->m_Core;
    for (int i = 0; i < NUM_ID; i++)
        m_IDs[i] = Server()->SnapNewID();

    GameWorld()->InsertEntity(this);
}

CScrap::~CScrap()
{
    for (int i = 0; i < NUM_ID; i++)
        Server()->SnapFreeID(m_IDs[i]);
}

void CScrap::Reset()
{
    GameWorld()->DestroyEntity(this);
}

void CScrap::Tick()
{
    if(GameWorld()->m_Paused || GameServer()->m_pController->IsGameOver())
        return;

    vec2 NewPos;
    CCharacter *TargetChr = GameServer()->m_World.IntersectCharacter(m_Pos, vec2(m_Pos.x+12, m_Pos.y-12), m_Weight, NewPos);
    if(TargetChr && TargetChr->GetPlayer())
    {
        int ClientID = TargetChr->GetPlayer()->GetCID();

        GameServer()->SendBroadcast(ClientID, BROADCAST_PRIORITY_EFFECTSTATE, BROADCAST_DURATION_GAMEANNOUNCE, _("废品:{str:Name}\n价值:{int:Value}\n重量:{int:Weight}\n使用锤子捡起物品"), 
            "Name", GameServer()->ScrapInfo()->GetScrapName(m_ScrapType), 
            "Value", &m_ScrapValue, "Weight", &m_Weight);
    }
}

void CScrap::ExportScrap(Scrap &Out) const
{
    Out.m_ID = 0;
    Out.m_ScrapID = m_ScrapType;
    Out.m_Value = m_ScrapValue;
    Out.m_Weight = m_Weight;
    Out.m_InShip = false;
}

void CScrap::MergeStats(int Value, int Weight)
{
    m_ScrapValue += Value;
    m_Weight += Weight;
}

bool CScrap::Pickup(int ClientID)
{
    if(ClientID > MAX_CLIENTS || ClientID < 0 || !GameServer()->m_apPlayers[ClientID])
        return false;
    
    if((int)GameServer()->m_apPlayers[ClientID]->m_vScraps.size() >= GC_MAX_SCRAP_SLOTS)
    {
        int MaxSlots = GC_MAX_SCRAP_SLOTS;
        GameServer()->SendChatTarget(ClientID, _("背包已满！最多携带 {int:max} 件废品"), "max", &MaxSlots);
        return false;
    }

    Scrap *Temp = new Scrap();
    Temp->m_ID = GameServer()->m_apPlayers[ClientID]->m_ItemCount;
    GameServer()->m_apPlayers[ClientID]->m_ItemCount++;
    Temp->m_ScrapID = m_ScrapType;
    int Value = m_ScrapValue;
    CPlayer *pPlayer = GameServer()->m_apPlayers[ClientID];
    if(pPlayer->m_ScrapValueBonusPercent != 100)
    {
        Value = m_ScrapValue * pPlayer->m_ScrapValueBonusPercent / 100;
        pPlayer->m_ScrapValueBonusPercent = 100;
        GameServer()->SendChatTarget(ClientID, _("招财猫生效！这件废品价值翻倍为 {int:value}元"), "value", &Value);
    }
    Temp->m_Value = Value;
    Temp->m_Weight = m_Weight;
    Temp->m_InShip = m_WasInShip;
    GameServer()->m_apPlayers[ClientID]->m_vScraps.add(Temp);
    GameServer()->ResetVotes(ClientID);
    return true;
}

void CScrap::TickPaused()
{
}

void CScrap::Snap(int SnappingClient)
{
    if (NetworkClipped(SnappingClient))
        return;

    int Radius = clamp(m_Weight, 5, 50);
    vec2 Vertices[4] = {
        vec2(m_Pos.x - (Radius * 2 + 4), m_Pos.y - (Radius * 2 + 4)),
        vec2(m_Pos.x + (Radius * 2 + 4), m_Pos.y - (Radius * 2 + 4)),
        vec2(m_Pos.x + (Radius * 2 + 4), m_Pos.y + (Radius * 2 + 4)),
        vec2(m_Pos.x - (Radius * 2 + 4), m_Pos.y + (Radius * 2 + 4))};
    
    if(GetInShip())
    {
        CScrap *pClosestScraps[16];
        int Num = GameWorld()->FindEntities(m_Pos, (Radius * 2 + 4), (CEntity **)pClosestScraps, 16, CGameWorld::ENTTYPE_SCRAP);
        for (int i = 0; i < Num; i++)
        {
            if (pClosestScraps[i] && pClosestScraps[i]->GetWeight() > GetWeight())
            {
                return;
            }
        }
    }

    if(!GameWorld()->m_Paused)
    {
        float Spin = ((float)m_ScrapValue) / 64.f;
        if(m_ScrapType == SCRAP_L3_BOOMBOX)
            Spin *= 2.f;
        m_Angle += Spin;
    }

    for (int i = 0; i < 4; i++)
        Rotate(&Vertices[i], m_Pos.x, m_Pos.y, m_Angle);

    int CenterWeapon = WEAPON_RIFLE;
    int LaserOffset = 0;
    int LaserStart = Server()->Tick();
    bool UsePickupRing = false;
    int PickupType = POWERUP_WEAPON;
    int PickupSubtype = WEAPON_HAMMER;

    switch(m_ScrapType)
    {
    case SCRAP_L1_SODA:
        CenterWeapon = WEAPON_GUN;
        PickupType = POWERUP_WEAPON;
        PickupSubtype = WEAPON_GUN;
        UsePickupRing = true;
        break;
    case SCRAP_L1_WHISTLE:
        CenterWeapon = WEAPON_HAMMER;
        LaserOffset = 6;
        break;
    case SCRAP_L2_MEDKIT:
        CenterWeapon = WEAPON_SHOTGUN;
        PickupType = POWERUP_HEALTH;
        PickupSubtype = 0;
        UsePickupRing = true;
        break;
    case SCRAP_L2_MEGAPHONE:
        CenterWeapon = WEAPON_GUN;
        LaserOffset = 10;
        LaserStart = Server()->Tick() - 6;
        break;
    case SCRAP_L3_LUCKYCAT:
        CenterWeapon = WEAPON_GRENADE;
        PickupType = POWERUP_ARMOR;
        PickupSubtype = 0;
        UsePickupRing = true;
        break;
    case SCRAP_L3_BOOMBOX:
        CenterWeapon = WEAPON_GRENADE;
        LaserOffset = 14;
        LaserStart = Server()->Tick() - 3;
        break;
    default:
        break;
    }

    {
        CNetObj_Projectile *pProj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, m_ID, sizeof(CNetObj_Projectile)));
        if (pProj)
        {
            pProj->m_Type = CenterWeapon;
            pProj->m_VelX = 0;
            pProj->m_VelY = 0;
            pProj->m_X = (int)m_Pos.x;
            pProj->m_Y = (int)m_Pos.y;
            pProj->m_StartTick = Server()->Tick();
        }
    }

    if(UsePickupRing)
    {
        for (int i = 0; i < NUM_ID; i++)
        {
            CNetObj_Pickup *pPickup = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, m_IDs[i], sizeof(CNetObj_Pickup)));
            if(!pPickup)
                return;
            pPickup->m_X = (int)Vertices[i].x;
            pPickup->m_Y = (int)Vertices[i].y;
            pPickup->m_Type = PickupType;
            pPickup->m_Subtype = PickupSubtype;
        }
    }
    else
    {
        for (int i = 0; i < NUM_ID; i++)
        {
            CNetObj_Laser *pLaser = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_IDs[i], sizeof(CNetObj_Laser)));
            if (pLaser)
            {
                int Pos1 = ((i + 1) >= 4) ? 0 : (i + 1);
                pLaser->m_X = Vertices[Pos1].x + LaserOffset;
                pLaser->m_Y = Vertices[Pos1].y + LaserOffset;
                pLaser->m_FromX = Vertices[i].x + LaserOffset;
                pLaser->m_FromY = Vertices[i].y + LaserOffset;
                pLaser->m_StartTick = LaserStart;
            }
        }
    }
}