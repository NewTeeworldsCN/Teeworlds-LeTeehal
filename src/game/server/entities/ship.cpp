/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include <engine/shared/config.h>

#include "ship.h"

CShip::CShip(CGameWorld *pGameWorld, vec2 Pos)
    : CEntity(pGameWorld, CGameWorld::ENTTYPE_LASER)
{
    m_Pos = Pos;
    m_Radius = 400;
    m_Pos.y -= float(m_Radius) / 2;

    for (int i = 0; i < NUM_ID; i++)
    {
        m_IDs[i] = Server()->SnapNewID();
        m_IDsHammer[i] = Server()->SnapNewID();
    }
    GameWorld()->InsertEntity(this);

    m_StartTick = Server()->Tick();
}

CShip::~CShip()
{
    for (int i = 0; i < NUM_ID; i++)
    {
        Server()->SnapFreeID(m_IDs[i]);
        Server()->SnapFreeID(m_IDsHammer[i]);
    }
}

void CShip::Reset()
{
    m_StartTick = Server()->Tick();
}

void CShip::Tick()
{
    for (CCharacter *pChr = (CCharacter *)GameWorld()->FindFirst(CGameWorld::ENTTYPE_CHARACTER); pChr; pChr = (CCharacter *)pChr->TypeNext())
    {
        if (!pChr->GetPlayer())
            continue;

        if (distance(pChr->m_Pos, m_Pos) <= m_Radius)
        {
            if (pChr->m_InShip == false)
            {
                GameServer()->SendBroadcast(pChr->GetPlayer()->GetCID(), BROADCAST_PRIORITY_INTERFACE, BROADCAST_DURATION_GAMEANNOUNCE, _("你现在在飞船里了，打开投票界面查看更多"));
                pChr->m_InShip = true;
                GameServer()->ResetVotes(pChr->GetPlayer()->GetCID());
            }
            pChr->m_InShip = true;
            if(pChr->m_LeekTick > 0)
            {
                pChr->m_LeekTick = -1;
                GameServer()->SendChatTarget(pChr->GetPlayer()->GetCID(), _("[生命维持系统-飞船]已为您清除了韭菜盒子病毒，您现在安全了"));
            }
        }
        else
        {
            if (pChr->m_InShip == true)
            {
                GameServer()->SendBroadcast(pChr->GetPlayer()->GetCID(), BROADCAST_PRIORITY_INTERFACE, BROADCAST_DURATION_GAMEANNOUNCE, _("你离开了飞船"));
                pChr->m_InShip = false;
                GameServer()->ResetVotes(pChr->GetPlayer()->GetCID());
            }
            pChr->m_InShip = false;
        }
    }
}

void CShip::UpdateValue()
{
    m_Value = 0;
    m_Num = 0;
    for (CScrap *pScrap = (CScrap *)GameWorld()->FindFirst(CGameWorld::ENTTYPE_SCRAP); pScrap; pScrap = (CScrap *)pScrap->TypeNext())
    {
        if(!pScrap->GetInShip())
            continue;

        m_Value += pScrap->GetScrapValue();
        m_Num++;
    }
    GameServer()->ResetVotes(-1);
}

void CShip::TickPaused()
{
    m_StartTick++;
}

void CShip::Snap(int SnappingClient)
{
    if (NetworkClipped(SnappingClient))
        return;

    vec2 Vertices[4] = {
        vec2(m_Pos.x - m_Radius, m_Pos.y - m_Radius / 2),
        vec2(m_Pos.x + m_Radius, m_Pos.y - m_Radius / 2),
        vec2(m_Pos.x + m_Radius, m_Pos.y + m_Radius),
        vec2(m_Pos.x - m_Radius, m_Pos.y + m_Radius)};

    for (int i = 0; i < NUM_ID; i++)
    {
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
        {
            float time = (Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed();
            float angle = fmodf(time*pi/2, 2.0f*pi);
            CNetObj_Projectile *pProj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, m_IDsHammer[i], sizeof(CNetObj_Projectile)));
            if (pProj)
            {
                float shiftedAngle = angle + 2.0*pi*static_cast<float>(i)/static_cast<float>(NUM_ID);

                pProj->m_X = (int)(m_Pos.x + (m_Radius/4)*cos(shiftedAngle));
                pProj->m_Y = (int)(m_Pos.y + (m_Radius/4)*sin(shiftedAngle));
                pProj->m_VelX = 0;
                pProj->m_VelY = 0;
                pProj->m_Type = WEAPON_HAMMER;
                pProj->m_StartTick = Server()->Tick();
            }
        }
    }
}
