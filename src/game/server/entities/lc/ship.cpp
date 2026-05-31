/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/generated/protocol.h>
#include <game/server/core/gamecontext.h>
#include <game/server/lc/ui/gameplay_ui.h>
#include <game/server/lc/expedition/balance.h>
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

        if (distancebox(vec2(m_Radius, m_Radius), pChr->m_Pos, m_Pos))
        {
            if (pChr->m_InShip == false)
            {
                GameServer()->SendBroadcast(pChr->GetPlayer()->GetCID(), BROADCAST_PRIORITY_INTERFACE, BROADCAST_DURATION_GAMEANNOUNCE, _("你现在在飞船里了，打开投票界面查看更多"));
                pChr->m_InShip = true;
                LcPlayUiSound(GameServer(), SOUND_WEAPON_SWITCH, pChr->GetPlayer()->GetCID());
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

bool CShip::Contains(vec2 Pos) const
{
    return distancebox(vec2((float)m_Radius, (float)m_Radius), Pos, m_Pos);
}

bool CShip::Overlaps(vec2 Pos, float Margin) const
{
    vec2 Delta = Pos - m_Pos;
    return fabs(Delta.x) < (float)m_Radius + Margin && fabs(Delta.y) < (float)m_Radius + Margin;
}

bool CShip::RepelEntity(vec2 *pPos, vec2 *pVel, float PhysRadius) const
{
    if(!pPos)
        return false;

    const float Margin = PhysRadius + GC_SHIP_MONSTER_REPEL_BUFFER;
    vec2 Delta = *pPos - m_Pos;
    float AbsX = fabs(Delta.x);
    float AbsY = fabs(Delta.y);

    if(AbsX >= (float)m_Radius + Margin || AbsY >= (float)m_Radius + Margin)
        return false;

    float OverX = (float)m_Radius + Margin - AbsX;
    float OverY = (float)m_Radius + Margin - AbsY;
    if(OverX <= 0.f || OverY <= 0.f)
        return false;

    vec2 Push(0.f, 0.f);
    if(OverX < OverY)
        Push.x = Delta.x >= 0.f ? OverX : -OverX;
    else
        Push.y = Delta.y >= 0.f ? OverY : -OverY;

    *pPos += Push;

    if(pVel)
    {
        vec2 Out = *pPos - m_Pos;
        if(length(Out) > 0.01f)
            Out = normalize(Out);
        else if(length(Push) > 0.01f)
            Out = normalize(Push);
        else
            Out = vec2(1.f, 0.f);

        float Inward = dot(*pVel, -Out);
        if(Inward > 0.f)
            *pVel += Out * (Inward + GC_SHIP_MONSTER_BOUNCE);
        else
            *pVel += Out * (GC_SHIP_MONSTER_BOUNCE * 0.35f);
    }

    return true;
}

void CShip::RecalculateValue()
{
    m_Value = 0;
    m_Num = 0;
    for(CScrap *pScrap = (CScrap *)GameWorld()->FindFirst(CGameWorld::ENTTYPE_SCRAP); pScrap; pScrap = (CScrap *)pScrap->TypeNext())
    {
        if(!pScrap->GetInShip())
            continue;

        m_Value += pScrap->GetScrapValue();
        m_Num++;
    }
}

void CShip::UpdateValue()
{
    UpdateValue(-1);
}

void CShip::UpdateValue(int NotifyClientID)
{
    RecalculateValue();
    if(NotifyClientID >= 0)
    {
        GameServer()->ResetVotes(NotifyClientID);
        return;
    }

    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        CCharacter *pChr = GameServer()->GetPlayerChar(i);
        if(pChr && pChr->m_InShip)
            GameServer()->ResetVotes(i);
    }
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
        vec2(m_Pos.x - m_Radius, m_Pos.y - m_Radius),
        vec2(m_Pos.x + m_Radius, m_Pos.y - m_Radius),
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
