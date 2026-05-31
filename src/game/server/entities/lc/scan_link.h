#ifndef GAME_SERVER_ENTITIES_SCAN_LINK_H
#define GAME_SERVER_ENTITIES_SCAN_LINK_H

#include <game/server/core/entity.h>

class CScanLink : public CEntity
{
public:
	CScanLink(CGameWorld *pGameWorld, vec2 From, vec2 To, int LifeTicks);

	virtual void Reset();
	virtual void Tick();
	virtual void TickPaused();
	virtual void Snap(int SnappingClient);

private:
	vec2 m_From;
	vec2 m_To;
	int m_LifeTicks;
	int m_StartTick;
};

#endif
