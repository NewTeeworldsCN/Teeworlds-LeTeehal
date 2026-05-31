/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_SHIP_H
#define GAME_SERVER_ENTITIES_SHIP_H

#include <game/server/entity.h>

class CShip : public CEntity
{
enum
{
    NUM_ID = 4,
};
public:
	CShip(CGameWorld *pGameWorld, vec2 Pos);
    ~CShip();

	virtual void Reset();
	virtual void Tick();
	virtual void TickPaused();
	virtual void Snap(int SnappingClient);
    
    void UpdateValue();
    int GetValue() { return m_Value; }
    int GetNum() { return m_Num; }

    int m_Value;
private:
    int m_IDs[NUM_ID];
    int m_IDsHammer[NUM_ID];
    int m_Radius;
    int m_StartTick;
    int m_Num;
};

#endif
