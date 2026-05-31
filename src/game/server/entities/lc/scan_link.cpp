#include <game/generated/protocol.h>
#include <game/server/core/gamecontext.h>
#include "scan_link.h"

CScanLink::CScanLink(CGameWorld *pGameWorld, vec2 From, vec2 To, int LifeTicks)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_LASER)
{
	m_From = From;
	m_To = To;
	m_LifeTicks = LifeTicks;
	m_StartTick = Server()->Tick();
	m_Pos = (From + To) * 0.5f;
	m_ProximityRadius = (int)(distance(From, To) * 0.5f) + 64;
	GameWorld()->InsertEntity(this);
}

void CScanLink::Reset()
{
	GameWorld()->DestroyEntity(this);
}

void CScanLink::Tick()
{
	if(GameWorld()->m_Paused)
		return;

	m_LifeTicks--;
	if(m_LifeTicks <= 0)
		GameWorld()->DestroyEntity(this);
}

void CScanLink::TickPaused()
{
}

void CScanLink::Snap(int SnappingClient)
{
	if(SnappingClient != -1)
	{
		if(NetworkClipped(SnappingClient, m_From) && NetworkClipped(SnappingClient, m_To))
			return;
	}

	CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_ID, sizeof(CNetObj_Laser)));
	if(!pObj)
		return;

	pObj->m_X = (int)m_To.x;
	pObj->m_Y = (int)m_To.y;
	pObj->m_FromX = (int)m_From.x;
	pObj->m_FromY = (int)m_From.y;
	pObj->m_StartTick = Server()->Tick();
}
