#include "extdll.h"
#include "util.h"
#include "cinematic_camera.h"
#include "const.h"
#include "game.h"


int CCinematicCamera::IsCinematic()
{
	return mp_cinematics.value;
}

CCinematicCamera* CCinematicCamera::Create()
{
	CCinematicCamera* pCam = GetClassPtr((CCinematicCamera*)NULL);
	pCam->Spawn();

	return pCam;
}

void CCinematicCamera::Spawn()
{
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_NOT;
	pev->renderamt = 0.0f;
	pev->rendermode = kRenderTransAlpha;

	SET_MODEL(ENT(pev), "models/crossbow_bolt.mdl");

	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, g_vecZero, g_vecZero);
}

void CCinematicCamera::SetViewOnTarget()
{
	if (m_pPlayer)
		return;
	
	m_pPlayer->pev->origin = m_vecOrigin = pev->origin;
	SET_VIEW(m_pTarget->edict(), edict());
}

void CCinematicCamera::SetOrigin(const Vector& origin)
{
	m_vecOrigin = origin;
}

void CCinematicCamera::SetTarget(CBaseEntity* pTarget)
{
	m_pTarget = pTarget;
}

void CCinematicCamera::SetPlayer(CBasePlayer* pPlayer)
{
	m_pPlayer = pPlayer;
}

void CCinematicCamera::SetViewOnPlayer()
{
	if (IsCinematic() == 0 && strcmp(STRING(m_pPlayer->pev->classname), "trigger_camera") != 0 && m_iDeathCam == 0)
	{
		SET_VIEW(m_pTarget->edict(), m_pTarget->edict());
	}

	pev->origin = m_vecOrigin + m_pPlayer->pev->origin;
	pev->angles = m_vecAngles + m_pPlayer->pev->angles;

	if ((m_pPlayer->IsPlayer()) && (m_vecOrigin == g_vecZero
		|| m_vecOrigin == m_pPlayer->pev->view_ofs
		|| (m_pTarget->pev->flags & FL_DUCKING) != 0))
	{
		SET_VIEW(m_pTarget->edict(), m_pTarget->edict());
	}
	else
	{
		SET_VIEW(m_pTarget->edict(), edict());
	}
}