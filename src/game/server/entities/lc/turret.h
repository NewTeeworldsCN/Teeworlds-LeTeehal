#ifndef GAME_SERVER_ENTITIES_LC_TURRET_H
#define GAME_SERVER_ENTITIES_LC_TURRET_H

#include <game/server/core/entity.h>

class CTurret : public CEntity
{
public:
	CTurret(CGameWorld *pGameWorld, vec2 Pos);

	virtual void Reset();
	virtual void Tick();
	virtual void TickPaused();
	virtual void Snap(int SnappingClient);

private:
	vec2 m_AimDir;
	int m_TargetClientId;
	int m_LastFireTick;

	void SnapLaser(vec2 From, vec2 To, int SnappingClient);
	CCharacter *FindTargetInAimCone(float Range);
	CCharacter *GetTrackedTarget();
	bool HasLineOfSight(vec2 To);
	void RotateAim(float Degrees);
};

#endif
