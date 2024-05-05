#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include <base/math.h>

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

CScrap::CScrap(CGameWorld *pGameWorld, int Level, vec2 Pos, bool Random, Scrap S) : CEntity(pGameWorld, CGameWorld::ENTTYPE_SCRAP)
{
    m_Pos = Pos;
    m_Level = Level;
    m_Hide = false;
    m_Random = Random;
    m_ScrapType = Level - (SCRAP_L1 - 1) + (rand()%6);
    if(Random)
        GameServer()->ScrapInfo()->RandomScrap(m_ScrapType, m_ScrapValue, m_Weight);
    else
    {
        m_ScrapType = S.m_ScrapID;
        m_ScrapValue = S.m_Value;
        m_Weight = S.m_Weight;
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
    if(m_Random)
        GameServer()->ScrapInfo()->RandomScrap(m_ScrapType, m_ScrapValue, m_Weight);
    else
        GameWorld()->DestroyEntity(this);
    m_Hide = false;
}

void CScrap::Tick()
{
    if(!m_Random && m_Hide)
        Reset();

    if(m_Hide || GameWorld()->m_Paused || GameServer()->m_pController->IsGameOver())
        return;

    vec2 NewPos;
    CCharacter *TargetChr = GameServer()->m_World.IntersectCharacter(m_Pos, vec2(m_Pos.x+12, m_Pos.y-12), m_Weight, NewPos);
    if(TargetChr && TargetChr->GetPlayer())
    {
        int ClientID = TargetChr->GetPlayer()->GetCID();

        GameServer()->SendBroadcast(ClientID, 200, 10, 
            _("废品:{str:Name}\n价值:{int:Value}\n重量:{int:Weight}\n使用锤子捡起物品"), 
            "Name", GameServer()->ScrapInfo()->GetScrapName(m_ScrapType), 
            "Value", &m_ScrapValue, "Weight", &m_Weight);
    }
}

bool CScrap::Pickup(int ClientID)
{
    if(ClientID > MAX_CLIENTS || ClientID < 0 || !GameServer()->m_apPlayers[ClientID])
        return false;
    
    Scrap *Temp = new Scrap();
    Temp->m_ID = GameServer()->m_apPlayers[ClientID]->m_vScraps.size();
    Temp->m_ScrapID = m_ScrapType;
    Temp->m_Value = m_ScrapValue;
    Temp->m_Weight = m_Weight;
    GameServer()->m_apPlayers[ClientID]->m_vScraps.add(Temp);
    GameServer()->ResetVotes(ClientID);
    return true;
}

void CScrap::TickPaused()
{
    // pass
}

void CScrap::Snap(int SnappingClient)
{
    if (NetworkClipped(SnappingClient) || m_Hide)
        return;

    {
        CNetObj_Projectile *pProj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, m_ID, sizeof(CNetObj_Projectile)));
        if (pProj)
        {
            pProj->m_Type = WEAPON_RIFLE;
            pProj->m_VelX = 0;
            pProj->m_VelY = 0;
            pProj->m_X = (int)m_Pos.x;
            pProj->m_Y = (int)m_Pos.y;
            pProj->m_StartTick = Server()->Tick();
        }
    }

    vec2 Vertices[4] = {
        vec2(m_Pos.x - (m_Weight * 2 + 4), m_Pos.y - (m_Weight * 2 + 4)),
        vec2(m_Pos.x + (m_Weight * 2 + 4), m_Pos.y - (m_Weight * 2 + 4)),
        vec2(m_Pos.x + (m_Weight * 2 + 4), m_Pos.y + (m_Weight * 2 + 4)),
        vec2(m_Pos.x - (m_Weight * 2 + 4), m_Pos.y + (m_Weight * 2 + 4))};

    m_Angle += ((float)m_ScrapValue) / 48.f;

    for (int i = 0; i < 4; i++)
    {
        Rotate(&Vertices[i], m_Pos.x, m_Pos.y, m_Angle);
    }

    for (int i = 0; i < NUM_ID; i++)
    {
        CNetObj_Laser *pLaser = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_IDs[i], sizeof(CNetObj_Laser)));
        if (pLaser)
        {
            int Pos1 = ((i + 1) >= 4) ? 0 : (i + 1);
            pLaser->m_X = Vertices[Pos1].x;
            pLaser->m_Y = Vertices[Pos1].y;
            pLaser->m_FromX = Vertices[i].x;
            pLaser->m_FromY = Vertices[i].y;
            pLaser->m_StartTick = Server()->Tick();
        }
    }
}