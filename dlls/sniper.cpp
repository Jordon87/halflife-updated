/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "monsters.h"
#include "player.h"
#include "gamerules.h"
#include "UserMessages.h"

LINK_ENTITY_TO_CLASS(weapon_sniper, CSniper);

bool CSniper::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "357";
	p->iMaxAmmo1 = SNIPER_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = SNIPER_MAX_CLIP;
	p->iSlot = 3;
	p->iPosition = 2;
	p->iId = m_iId = WEAPON_SNIPER;
	p->iFlags = 0;
	p->iWeight = SNIPER_WEIGHT;

	return true;
}

void CSniper::Spawn()
{
	Precache();
	m_iId = WEAPON_SNIPER;
	SET_MODEL(ENT(pev), "models/w_sniper.mdl");

	m_iDefaultAmmo = SNIPER_DEFAULT_GIVE;

	FallInit(); // get ready to fall down.
}


void CSniper::Precache()
{
	PRECACHE_MODEL("models/v_sniper.mdl");
	PRECACHE_MODEL("models/w_sniper.mdl");
	PRECACHE_MODEL("models/p_sniper.mdl");

	PRECACHE_SOUND("weapons/sniper_fire1.wav");
	PRECACHE_SOUND("weapons/sniper_reload1.wav");
	PRECACHE_SOUND("weapons/zoom.wav");

	m_usSniper = PRECACHE_EVENT(1, "events/sniper.sc");
}

bool CSniper::Deploy()
{
	return DefaultDeploy("models/v_sniper.mdl", "models/p_sniper.mdl", SNIPER_DRAW, "sniper", pev->body);
}


void CSniper::Holster()
{
	m_fInReload = false; // cancel any reload in progress.

	if (m_pPlayer->m_iFOV != 0)
	{
		SecondaryAttack();
	}

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;;
}

void CSniper::SecondaryAttack()
{
	if (m_pPlayer->m_iFOV != 0)
	{
		m_pPlayer->m_iFOV = 0; // 0 means reset to default fov
	}
	else if (m_pPlayer->m_iFOV != 40)
	{
		m_pPlayer->m_iFOV = 40;
	}

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.75f;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.75f;
	
	EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_STATIC, "weapons/zoom.wav", 1, ATTN_NORM, 0, RANDOM_LONG(0, 0x96));
}

void CSniper::PrimaryAttack()
{
	if (m_iUnk_0x88 == 0)
	{
		SniperFire();
		m_iUnk_0x88 = 1;
	}
}

void CSniper::SniperFire()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound();
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.15f;
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15f;
		return;
	}

	if (m_iClip == 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.2f;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.2f;
		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;

	m_iClip--;

	PLAYBACK_EVENT_FULL(1, m_pPlayer->edict(), m_usSniper, 0.0f, g_vecZero, g_vecZero, 0.0f, 0.0f, m_iClip, m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType], 0, 0);

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	Vector vecDir;
	vecDir = m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, VECTOR_CONE_1DEGREES, 8192, BULLET_PLAYER_SNIPER, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed);

	if (0 == m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
	{
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 2.75f;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.75f;

		if (m_pPlayer->m_iFOV != 0)
		{
			SecondaryAttack();
		}
	}
	else
	{
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.5f;
	}

	if (m_iClip == 0)
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.75f;
	}
	else
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.5f;
	}
}


void CSniper::Reload()
{
	if (m_pPlayer->ammo_sniper <= 0)
		return;

	if (m_pPlayer->m_iFOV != 0)
	{
		SecondaryAttack();
	}

	if (DefaultReload(3, SNIPER_RELOAD, 2.25f))
	{
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/sniper_reload1.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 93 + RANDOM_LONG(0, 0xF));
	}
}


void CSniper::WeaponIdle()
{
	ResetEmptySound();

	m_iUnk_0x88 = 0;

	if (m_flTimeWeaponIdle < UTIL_WeaponTimeBase())
	{
		SendWeaponAnim(SNIPER_IDLE1);
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
	}
}
