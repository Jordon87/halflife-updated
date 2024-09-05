#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"

LINK_ENTITY_TO_CLASS(weapon_vulcan, CVulcan);

void CVulcan::Spawn()
{
	pev->classname = MAKE_STRING("weapon_vulcan");
	Precache();
	SET_MODEL(ENT(pev), "models/w_768mmVulcan.mdl");
	m_iId = WEAPON_VULCAN;
	
	m_iDefaultAmmo = 100;

	FallInit();
}

void CVulcan::Precache()
{
	PRECACHE_MODEL("models/v_768mmVulcan.mdl");
	PRECACHE_MODEL("models/w_768mmVulcan.mdl");
	PRECACHE_MODEL("models/p_768mmVulcan.mdl"); 

	m_nShell = PRECACHE_MODEL("models/shell.mdl");

	PRECACHE_SOUND("items/9mmclip1.wav");
	PRECACHE_SOUND("hassault/hw_spin.wav");
	PRECACHE_SOUND("buttons/button8.wav");
	PRECACHE_SOUND("hassault/hw_shoot1.wav");
	PRECACHE_SOUND("hassault/hw_shoot2.wav");
	PRECACHE_SOUND("hassault/hw_shoot3.wav");
}

bool CVulcan::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "7.68mm";
	p->iMaxAmmo1 = 250;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = -1;
	p->iSlot = 2;
	p->iPosition = 4;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_VULCAN;
	p->iWeight = 15;

	return true;
}

bool CVulcan::Deploy()
{
	flSpin = 0.0f;
	m_flSpinTime = gpGlobals->time;
	m_flSoundTime = gpGlobals->time;

	return CBasePlayerWeapon::DefaultDeploy("models/v_768mmVulcan.mdl", "models/p_768mmVulcan.mdl", 0, "mp5");
}

bool CVulcan::CanHolster()
{
	return flSpin == 0.0f;
}

void CVulcan::Holster()
{
	EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "buttons/button8.wav", VOL_NORM, ATTN_NORM, 0, 200);
	CBasePlayerWeapon::Holster();
}

void CVulcan::PrimaryAttack()
{
	m_flTimeWeaponIdle = gpGlobals->time;

	if (m_pPlayer->pev->waterlevel == 3)
	{
		m_flNextPrimaryAttack = gpGlobals->time + 0.15f;
	}

	FUN_100d7849();

	if (FUN_100d7ad4())
	{
		if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		{
			m_flNextPrimaryAttack = gpGlobals->time + 0.15f;
		}

		m_pPlayer->m_iWeaponVolume = 600;
		m_pPlayer->m_iWeaponFlash = 256;

		m_pPlayer->SetAnimation(PLAYER_ATTACK1);

		SendWeaponAnim(2);

		Vector vecSrc = m_pPlayer->GetGunPosition();
		Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);
		Vector vecDir;

		m_pPlayer->FireBullets(1, vecSrc, vecAiming, VECTOR_CONE_3DEGREES, 8192, BULLET_PLAYER_MP5, 2, 0);
		--m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType];

		int iRNGSound = RANDOM_LONG(0,2);

		if (iRNGSound == 0)
		{
			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "hassault/hw_shoot1.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(0,20) + 90);
		}
		else if (iRNGSound == 1)
		{
			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "hassault/hw_shoot2.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(0, 20) + 90);
		}
		else
		{
			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "hassault/hw_shoot3.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(0, 20) + 90);
		}

		if (0 == m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
			m_pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);

		m_pPlayer->pev->punchangle.x = RANDOM_FLOAT(-1.25, 1.25) + m_pPlayer->pev->punchangle.x;
		m_pPlayer->pev->punchangle.y = RANDOM_FLOAT(-1.0, 1.0) + m_pPlayer->pev->punchangle.y;
	}
	m_flNextPrimaryAttack = (100.0f - flSpin) / 1000.0 + m_flNextPrimaryAttack + 0.033;

	if (m_flNextPrimaryAttack < gpGlobals->time)
		m_flNextPrimaryAttack = (100.0f - flSpin) / 1000.0 + gpGlobals->time + 0.033;
}

void CVulcan::SecondaryAttack()
{
	m_flTimeWeaponIdle = gpGlobals->time;

	if (m_pPlayer->pev->waterlevel == 3)
	{
		m_flNextPrimaryAttack = gpGlobals->time + 0.15f;
	}
	else
	{
		m_flNextSecondaryAttack = (100.0f - flSpin) / 1000.0 + gpGlobals->time + 0.033;
		m_flNextPrimaryAttack = m_flNextSecondaryAttack;

		if (m_flNextPrimaryAttack < gpGlobals->time)
		{
			m_flNextSecondaryAttack = (100.0f - flSpin) / 1000.0 + gpGlobals->time + 0.033;
			m_flNextPrimaryAttack = m_flNextSecondaryAttack;
		}
		FUN_100d7849();
	}
}

void CVulcan::WeaponIdle()
{
	ResetEmptySound();
	FUN_100d7849();

	if (m_flTimeWeaponIdle <= gpGlobals->time)
		m_flTimeWeaponIdle = RANDOM_FLOAT(10.0, 15.0) + gpGlobals->time;
}

void CVulcan::FUN_100d7849()
{
	if (m_flSpinTime <= gpGlobals->time)
	{
		if (m_flNextPrimaryAttack + 0.25f <= gpGlobals->time)
		{
			if ((0.0f < flSpin) && (flSpin = flSpin - 5.0f, flSpin <= 0.0f))
			{
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "buttons/button8.wav", VOL_NORM, ATTN_NORM, 0, 95);
			}
			if (flSpin < 0.0f)
				flSpin = 0.0f;
		}
		else
		{
			if (flSpin < 100.0f)
				flSpin = flSpin + 8.333333333333334;

			if (100.0f < flSpin)
				flSpin = 100.0f;
		}

		if (flSpin <= 95.0f)
		{
			if (flSpin <= 50.0)
			{
				if (flSpin <= 0.0f)
					SendWeaponAnim(6);
				else
					SendWeaponAnim(3);
			}
			else
			{
				SendWeaponAnim(4);
			}
		}
		else
		{
			SendWeaponAnim(5);
		}

		if (m_flSoundTime < gpGlobals->time)
		{
			if (0.0f < flSpin)
			{
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "hassault/hw_spin.wav", VOL_NORM, ATTN_NORM, 0, flSpin + flSpin);
			}
			m_flSoundTime = gpGlobals->time + 0.1f;
		}

		m_flSpinTime = m_flSpinTime + 0.1f;

		if (m_flSpinTime < gpGlobals->time)
			m_flSpinTime = gpGlobals->time + 0.1f;
	}
}

bool CVulcan::FUN_100d7ad4()
{
	return 0.0f < flSpin;
}

class CVulcanAmmo : public CBasePlayerAmmo
{
	void Spawn() override
	{
		Precache();
		SET_MODEL(ENT(pev),"models/w_768ammo.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache() override
	{
		PRECACHE_MODEL("models/w_768ammo.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	bool AddAmmo(CBaseEntity* pOther) override
	{
		if (pOther->GiveAmmo(100, "7.68mm", 250) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return true;
		}
		return false;
	}
};
LINK_ENTITY_TO_CLASS(ammo_768mmbox, CVulcanAmmo);