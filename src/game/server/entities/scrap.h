#pragma once

#include <game/server/entity.h>

class CScrap : public CEntity
{
    enum
    {
        NUM_ID = 4,
    };
    CWorldCore *m_pWorld;

public:
    CScrap(CGameWorld *pGameWorld, int Level, vec2 Pos, bool Random, class Scrap S);
    ~CScrap();

    virtual void Reset();
    virtual void Tick();
    virtual void TickPaused();
    virtual void Snap(int SnappingClient);

    bool Pickup(int ClientID);
    
    int GetScrapValue() { return m_ScrapValue; }
    int GetScrapType() { return m_ScrapType; }
    int GetWeight() { return m_Weight; }

    bool m_Hide;

private:
    int m_Level;
    int m_ScrapValue;
    int m_ScrapType;
    int m_Weight;
    bool m_Random;
    float m_Angle;

    int m_IDs[NUM_ID];

    vec2 m_Vel;
};