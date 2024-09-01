#pragma once

#include "cbase.h"
#include "player.h"

class CCinematicCamera : public CBaseEntity
{
public:
	int IsCinematic();
	static CCinematicCamera* Create();
	void Spawn() override;
	void SetViewOnTarget();
	void SetOrigin(const Vector& origin);
	void SetTarget(CBaseEntity *pTarget);
	void SetPlayer(CBasePlayer *pPlayer);
	void SetViewOnPlayer();

	int m_iDeathCam;
private:
	CBaseEntity* m_pTarget;
	CBasePlayer* m_pPlayer;
	Vector m_vecOrigin;
	Vector m_vecAngles;
};