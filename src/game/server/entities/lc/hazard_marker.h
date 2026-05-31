#ifndef GAME_SERVER_ENTITIES_HAZARD_MARKER_H
#define GAME_SERVER_ENTITIES_HAZARD_MARKER_H

#include <game/server/core/entity.h>

class CHazardMarker : public CEntity
{
public:
	CHazardMarker(CGameWorld *pGameWorld, vec2 Pos, int Reserved, float Size = 16.f);

	virtual void Reset();
	virtual void Tick();
	virtual void TickPaused();
	virtual void Snap(int SnappingClient);

private:
	vec2 m_Center;
	int m_Reserved;
	float m_Size;
	int m_PulseTick;

	float PulseScale() const;
	void SnapLaser(vec2 From, vec2 To, int SnappingClient);
	void SnapProjectile(int SnappingClient);
	void SnapLaserFrame(int SnappingClient);
};

#endif
