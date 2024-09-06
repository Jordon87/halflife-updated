#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#include "customentity.h"
#include "soundent.h"
#include "skill.h"

LINK_ENTITY_TO_CLASS(weapon_plasma, CPlasma);

class CPlasmaBall : public CBaseEntity
{
public:
	void Spawn() override;
	void EXPORT Animate();
	static void Shoot(entvars_t* pevOwner, Vector vecStart, Vector vecVelocity, float a4, float fldmg);
	void EXPORT Touch(CBaseEntity* pOther) override;

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	CBeam* m_pTail;
};

LINK_ENTITY_TO_CLASS(plasmaball, CPlasmaBall);

TYPEDESCRIPTION CPlasmaBall::m_SaveData[] =
	{
		DEFINE_FIELD(CPlasmaBall, m_pTail, FIELD_CLASSPTR),
};

IMPLEMENT_SAVERESTORE(CPlasmaBall, CBaseEntity);

class CPlasmaBallBig : public CBaseEntity
{
public:
	void Spawn() override;
	void EXPORT Animate();
	static void Shoot(entvars_t* pevOwner, Vector vecStart, Vector vecVelocity, float a4, float fldmg);
	void EXPORT Touch(CBaseEntity* pOther) override;

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	CBeam* m_pTail[4];
};

LINK_ENTITY_TO_CLASS(plasmaballbig, CPlasmaBallBig);

TYPEDESCRIPTION CPlasmaBallBig::m_SaveData[] =
	{
		DEFINE_FIELD(CPlasmaBallBig, m_pTail, FIELD_CLASSPTR),
};

IMPLEMENT_SAVERESTORE(CPlasmaBallBig, CBaseEntity);

void CPlasma::Spawn()
{
	pev->classname = MAKE_STRING("weapon_plasma");
	Precache();
	SET_MODEL(ENT(pev), "models/w_plasmagun.mdl");
	m_iId = WEAPON_PLASMA;

	m_iDefaultAmmo = 60;

	FallInit();
}

void CPlasma::Precache()
{
	PRECACHE_MODEL("models/v_plasmagun.mdl");
	PRECACHE_MODEL("models/w_plasmagun.mdl");
	PRECACHE_MODEL("models/p_plasmagun.mdl");

	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND("buttons/latchunlocked1.wav");
	PRECACHE_SOUND("buttons/latchunlocked2.wav");

	PRECACHE_SOUND("weapons/plasma1.wav");
	PRECACHE_SOUND("weapons/plasma2.wav");

	PRECACHE_SOUND("ambience/particle_suck1.wav");

	PRECACHE_SOUND("ambience/steamburst1.wav");

	PRECACHE_SOUND("steam.wav");

	PRECACHE_SOUND("weapons/plasma_re.wav");

	gpGlobals->m_SmallPlasmaSprite = PRECACHE_MODEL("sprites/plasmasmall.spr");
	gpGlobals->m_PlasmaSprite = PRECACHE_MODEL("sprites/plasma.spr");

	PRECACHE_MODEL("sprites/xbeam3.spr");

	PRECACHE_SOUND("weapons/mortarhit.wav");
}

bool CPlasma::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "Plasma cell";
	p->iMaxAmmo1 = 240;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 60;
	p->iSlot = 3;
	p->iPosition = 4;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_PLASMA;
	p->iWeight = 15;

	return true;
}

bool CPlasma::Deploy()
{
	unk = false;

	return DefaultDeploy("models/v_plasmagun.mdl", "models/p_plasmagun.mdl", 2, "mp5");
}

bool CPlasma::CanHolster()
{
	return unk == 0;
}

void CPlasma::PrimaryAttack()
{
	if (m_pPlayer->pev->waterlevel == 3 || m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = gpGlobals->time + 0.15f;
	}
	else
	{
		m_pPlayer->m_iWeaponVolume = 600;
		m_pPlayer->m_iWeaponFlash = 256;

		--m_iClip;

		m_pPlayer->SetAnimation(PLAYER_ATTACK1);
		
		SendWeaponAnim(0);

		UTIL_MakeVectors(m_pPlayer->pev->v_angle);

		Vector vecSrc;
		vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 21.5 + gpGlobals->v_right * 0.2 + gpGlobals->v_up * -5;

		Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/plasma2.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(0,15) + 94);

		CPlasmaBall::Shoot(m_pPlayer->pev, vecSrc, vecAiming, RANDOM_LONG(1100, 1300), gSkillData.plrDmgPlasma);
	
		if (0 == m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
			m_pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);

		m_flNextPrimaryAttack = m_flNextPrimaryAttack + 0.06;

		if (this->m_flNextPrimaryAttack < gpGlobals->time)
		{
			m_flNextPrimaryAttack = gpGlobals->time + 0.06;
		}
		m_flTimeWeaponIdle = RANDOM_FLOAT(10, 15) + gpGlobals->time;
	}
}

void CPlasma::SecondaryAttack()
{
	if (m_pPlayer->pev->waterlevel == 3 || m_iClip < 10)
	{
		PlayEmptySound();
		m_flNextSecondaryAttack = gpGlobals->time + 0.15f;
		m_flNextPrimaryAttack = m_flNextSecondaryAttack;
	}
	else
	{
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "ambience/particle_suck1.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(0,15) + 94);
		unk = true;
		m_flNextPrimaryAttack = gpGlobals->time + 0.75f + 1.0f;
		m_flNextSecondaryAttack = gpGlobals->time + 0.75f + 1.0f;
		m_flTimeWeaponIdle = gpGlobals->time + 0.75f;
		SendWeaponAnim(1);
	}
}

void CPlasma::Reload()
{
	DefaultReload(60,4,1.5f);
}

void CPlasma::WeaponIdle()
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	if (m_flTimeWeaponIdle <= gpGlobals->time)
	{
		if (unk)
		{
			m_pPlayer->m_iWeaponVolume = 600;
			m_pPlayer->m_iWeaponFlash = 256;

			m_iClip -= 10;

			m_pPlayer->SetAnimation(PLAYER_ATTACK1);

			UTIL_MakeVectors(m_pPlayer->pev->v_angle);

			Vector vecSrc;
			vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 5.5 + gpGlobals->v_right * 0.0 + gpGlobals->v_up * -4;
		
			Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/plasma2.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(0, 15) + 64);
		
			CPlasmaBallBig::Shoot(m_pPlayer->pev, vecSrc, vecAiming, RANDOM_LONG(1400, 1600), gSkillData.plrDmgPlasma);
		
			if (0 == m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
				m_pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);

			unk = false;
		}
		m_flTimeWeaponIdle = RANDOM_FLOAT(10, 15) + gpGlobals->time;
	}
}

void CPlasmaBall::Spawn()
{
	pev->movetype = MOVETYPE_TOSS;
	pev->classname = MAKE_STRING("plasmaball");
	
	pev->solid = SOLID_BBOX;
	pev->rendermode = kRenderFxFadeSlow;
	pev->renderamt = 255;

	SET_MODEL(ENT(pev), "sprites/plasma.spr");
	pev->frame = 0;
	pev->scale = RANDOM_FLOAT(0.25, 0.4);
	pev->gravity = RANDOM_FLOAT(0.1, 0.15);

	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));

	m_pTail = CBeam::BeamCreate("sprites/xbeam3.spr", 80);
	m_pTail->PointEntInit(pev->origin, entindex());
	m_pTail->SetBrightness(255);
	m_pTail->SetScrollRate(30);
	m_pTail->SetNoise(30);
	m_pTail->SetColor(192,176,128);
	m_pTail->SetFlags(BEAM_FSHADEIN);
}

void CPlasmaBall::Animate()
{
	pev->nextthink = gpGlobals->time + 0.1f;

	pev->frame = pev->frame + 1.0f;

	pev->velocity + Vector(RANDOM_LONG(512, 1536), RANDOM_LONG(-256, 256), RANDOM_LONG(-256, 256)) * 0.125f;

	if (2.0f < pev->frame)
	{
		pev->frame = 0.0f;
	}

	Vector unk;

	if (pev->sequence == 0)
	{
		if (RANDOM_LONG(0, 2) == 0)
		{
			MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY);
			WRITE_BYTE(120);
			WRITE_COORD(pev->origin.x);
			WRITE_COORD(pev->origin.y);
			WRITE_COORD(pev->origin.z);
			WRITE_COORD(unk.x);
			WRITE_COORD(unk.y);
			WRITE_COORD(unk.z);
			WRITE_SHORT(gpGlobals->m_SmallPlasmaSprite);
			WRITE_BYTE(1);
			WRITE_BYTE(1);
			WRITE_BYTE(200);
			WRITE_BYTE(5);
			MESSAGE_END();
		}

		m_pTail->SetStartPos(pev->origin - pev->velocity * 0.0625);

		if (this->pev->waterlevel == 3)
		{
			pev->sequence = 1;
			pev->renderamt = 160.0f;
			MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY);
			WRITE_BYTE(120);
			WRITE_COORD(pev->origin.x);
			WRITE_COORD(pev->origin.y);
			WRITE_COORD(pev->origin.z);
			WRITE_COORD(0.0f);
			WRITE_COORD(0.0f);
			WRITE_COORD(1.0f);
			WRITE_SHORT(gpGlobals->m_SmallPlasmaSprite);
			WRITE_BYTE(RANDOM_LONG(3,8));
			WRITE_BYTE(RANDOM_LONG(100,250));
			WRITE_BYTE(80);
			WRITE_BYTE(5);
			MESSAGE_END();

			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "steam.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(90,110));
		
			UTIL_Remove(m_pTail);

			pev->velocity = g_vecZero;

			pev->movetype = MOVETYPE_FLY;
		}
	}
	else
	{
		pev->renderamt = pev->renderamt - 25.0f;
		pev->scale = pev->scale + 0.2f;

		if (pev->renderamt < 30.0f)
		{
			SetThink(&CBaseEntity::SUB_Remove);
			pev->nextthink = gpGlobals->time;
		}
	}
}

void CPlasmaBall::Shoot(entvars_t* pevOwner, Vector vecStart, Vector vecVelocity, float a4, float fldmg)
{
	CPlasmaBall* pPlasma = GetClassPtr((CPlasmaBall*)NULL);
	UTIL_SetOrigin(pPlasma->pev, vecStart);

	Vector angDir = UTIL_VecToAngles(vecVelocity);
	pPlasma->pev->angles = angDir;
	pPlasma->pev->velocity = vecVelocity * a4;
	pPlasma->pev->owner = ENT(pevOwner);
	pPlasma->pev->dmg = fldmg;

	pPlasma->Spawn();

	pPlasma->SetThink(&CPlasmaBall::Animate);

	pPlasma->pev->nextthink = gpGlobals->time + 0.1f;

	if (RANDOM_LONG(0, 5) == 0)
	{
		CSoundEnt::InsertSound(bits_SOUND_COMBAT, pPlasma->pev->origin, 1024, 3.0);
	}
}

void CPlasmaBall::Touch(CBaseEntity* pOther)
{
	if (pev->sequence == 0)
	{
		TraceResult tr;

		switch (RANDOM_LONG(0, 1))
		{
		case 0:
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "buttons/latchunlocked1.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_FLOAT(90.0, 110.0));
			break;

		case 1:
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "buttons/latchunlocked2.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_FLOAT(90.0, 110.0));
			break;
		}

		if (pOther->pev->takedamage == 0.0f)
		{
			UTIL_TraceLine(pev->origin, pev->origin + pev->velocity * 10.0f, dont_ignore_monsters, ENT(pev), &tr);
		
			if (tr.fStartSolid == 0)
			{
				MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY);
				WRITE_BYTE(23);
				WRITE_COORD(tr.vecEndPos.x);
				WRITE_COORD(tr.vecEndPos.y);
				WRITE_COORD(tr.vecEndPos.z);
				WRITE_SHORT(gpGlobals->m_PlasmaSprite);
				WRITE_BYTE(10);
				WRITE_BYTE(10);
				WRITE_BYTE(192);
				MESSAGE_END();
			}
			Vector unk = pev->velocity * 0.0625;
			Vector unk2 = pev->origin - unk;

			pev->origin = unk2;
			pev->velocity = g_vecZero;
			pev->movetype = MOVETYPE_FLY;
		}
		else
		{
			pOther->TakeDamage(pev, VARS(pev->owner), pev->dmg, DMG_GENERIC);

			pev->velocity = pev->velocity.Normalize() * 128.0f;
			pev->movetype = MOVETYPE_FLY;
		}
		MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY);
		WRITE_BYTE(120);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z);
		WRITE_COORD(0.0f);
		WRITE_COORD(0.0f);
		WRITE_COORD(1.0f);
		WRITE_SHORT(gpGlobals->m_SmallPlasmaSprite);
		WRITE_BYTE(RANDOM_LONG(1,3));
		WRITE_BYTE(RANDOM_LONG(50,150));
		WRITE_BYTE(80);
		WRITE_BYTE(5);
		MESSAGE_END();
		UTIL_Remove(m_pTail);
		pev->sequence = 1;
	}
}

void CPlasmaBallBig::Spawn()
{
	pev->movetype = MOVETYPE_TOSS;
	pev->classname = MAKE_STRING("plasmaballbig");

	pev->solid = SOLID_BBOX;
	pev->rendermode = kRenderFxFadeSlow;
	pev->renderamt = 255;

	SET_MODEL(ENT(pev), "sprites/plasma.spr");
	pev->frame = 0;
	pev->scale = 2.0f;
	pev->gravity = 0.5f;
	pev->sequence = 0;

	UTIL_SetSize(pev, g_vecZero, g_vecZero);

	int i;
	for (i = 0; i < 4; ++i)
	{
		m_pTail[i] = CBeam::BeamCreate("sprites/xbeam3.spr", 80);
		m_pTail[i]->PointEntInit(pev->origin, entindex());
		m_pTail[i]->SetBrightness(255);
		m_pTail[i]->SetScrollRate(30);
		m_pTail[i]->SetNoise(0);
		m_pTail[i]->SetColor(192, 176, 128);
		m_pTail[i]->SetFlags(BEAM_FSHADEIN);
	}
}

void CPlasmaBallBig::Animate()
{
	pev->nextthink = gpGlobals->time + 0.1f;

	Vector unk;

	if (pev->sequence == 0)
	{
		unk = pev->velocity + Vector(RANDOM_LONG(0, 512), RANDOM_LONG(-256, 256), RANDOM_LONG(-256, 256)) * 0.25f;
	}
	else
	{
		unk = pev->velocity + Vector(RANDOM_LONG(0, 512), RANDOM_LONG(-256, 256), RANDOM_LONG(-256, 256));
	}

	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY);
	WRITE_BYTE(120);
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z);
	WRITE_COORD(unk.x);
	WRITE_COORD(unk.y);
	WRITE_COORD(unk.z);
	WRITE_SHORT(gpGlobals->m_SmallPlasmaSprite);
	WRITE_BYTE(pev->sequence * RANDOM_LONG(2, 7) + 3);
	WRITE_BYTE(1);
	WRITE_BYTE(200);
	WRITE_BYTE(5);
	MESSAGE_END();

	if (pev->waterlevel == 3 && pev->sequence == 0)
	{
		pev->sequence = 1;
		pev->renderamt = 160.0f;

		MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY);
		WRITE_BYTE(120);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z);
		WRITE_COORD(0.0f);
		WRITE_COORD(0.0f);
		WRITE_COORD(1.0f);
		WRITE_SHORT(gpGlobals->m_SmallPlasmaSprite);
		WRITE_BYTE(RANDOM_LONG(12, 20));
		WRITE_BYTE(RANDOM_LONG(150, 250));
		WRITE_BYTE(80);
		WRITE_BYTE(5);
		MESSAGE_END();

		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "steam.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(90, 110));
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "ambience/steamburst1.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(80, 90));

		pev->velocity = g_vecZero;
		pev->movetype = MOVETYPE_FLY;
	}

	if (this->pev->sequence != 0)
	{
		pev->renderamt = pev->renderamt - 10.0f;
		pev->scale = pev->scale + 0.2f;
	}

	int i;
	for (i = 0; i < 4; ++i)
	{
		m_pTail[i]->SetStartPos(pev->origin + Vector(RANDOM_LONG(-128, 128), RANDOM_LONG(-128, 128), RANDOM_LONG(-128, 128)));
	}

	if (pev->renderamt < 30.0f)
	{
		SetThink(&CBaseEntity::SUB_Remove);

		int j;
		for (j = 0; j < 4; ++j)
		{
			UTIL_Remove(m_pTail[j]);
		}
		pev->nextthink = gpGlobals->time;
	}

	if (pev->renderamt < 230.0f)
	{
		RadiusDamage(pev->origin, pev, VARS(pev->owner), pev->dmg / 25.0f, pev->dmg * pev->scale * 0.4, 0, DMG_BLAST);
	}
}

void CPlasmaBallBig::Shoot(entvars_t* pevOwner, Vector vecStart, Vector vecVelocity, float a4, float fldmg)
{
	CPlasmaBallBig* pBigPlasma = GetClassPtr((CPlasmaBallBig*)NULL);
	UTIL_SetOrigin(pBigPlasma->pev, vecStart);

	Vector angDir = UTIL_VecToAngles(vecVelocity);
	pBigPlasma->pev->angles = angDir;
	pBigPlasma->pev->velocity = vecVelocity * a4;
	pBigPlasma->pev->owner = ENT(pevOwner);
	pBigPlasma->pev->dmg = fldmg;

	pBigPlasma->Spawn();

	pBigPlasma->SetThink(&CPlasmaBallBig::Animate);

	pBigPlasma->pev->nextthink = gpGlobals->time + 0.1f;

	if (RANDOM_LONG(0, 5) == 0)
	{
		CSoundEnt::InsertSound(bits_SOUND_COMBAT, pBigPlasma->pev->origin, 1024, 3.0);
	}
}

void CPlasmaBallBig::Touch(CBaseEntity* pOther)
{
	if (pev->sequence == 0)
	{
		TraceResult tr;

		if (pOther->pev->takedamage == 0.0f)
		{
			UTIL_TraceLine(pev->origin, pev->origin + pev->velocity * 10.0f, dont_ignore_monsters, ENT(pev), &tr);

			MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY);
			WRITE_BYTE(23);
			WRITE_COORD(tr.vecEndPos.x);
			WRITE_COORD(tr.vecEndPos.y);
			WRITE_COORD(tr.vecEndPos.z);
			WRITE_SHORT(gpGlobals->m_PlasmaSprite);
			WRITE_BYTE(120);
			WRITE_BYTE(10);
			WRITE_BYTE(96);
			MESSAGE_END();

			float dot = DotProduct(tr.vecPlaneNormal, Vector(0,0,1));

			if (0.5f < dot)
			{
				pev->movetype = MOVETYPE_FLY;
				pev->velocity = g_vecZero;
			}
		}
		else
		{
			pOther->TakeDamage(pev, VARS(pev->owner), 25.0f, DMG_BLAST);
		}
		MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY);
		WRITE_BYTE(120);
		WRITE_COORD(tr.vecEndPos.x);
		WRITE_COORD(tr.vecEndPos.y);
		WRITE_COORD(tr.vecEndPos.z);
		WRITE_COORD(tr.vecPlaneNormal.x);
		WRITE_COORD(tr.vecPlaneNormal.y);
		WRITE_COORD(tr.vecPlaneNormal.z);
		WRITE_SHORT(gpGlobals->m_SmallPlasmaSprite);
		WRITE_BYTE(10);
		WRITE_BYTE(200);
		WRITE_BYTE(80);
		WRITE_BYTE(5);
		MESSAGE_END();
		CSoundEnt::InsertSound(bits_SOUND_DANGER, pev->origin, 512, 3.0);
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/mortarhit.wav", VOL_NORM, 0.5f, 0, RANDOM_LONG(90,110));
		pev->sequence = 1;
	}
}

class CAmmoPlasmaCell : public CBasePlayerAmmo
{
	void Spawn() override
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_plasmaclip.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache() override
	{
		PRECACHE_MODEL("models/w_plasmaclip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	bool AddAmmo(CBaseEntity* pOther) override
	{
		if (pOther->GiveAmmo(60, "Plasma cell", 240) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return true;
		}
		return false;
	}
};
LINK_ENTITY_TO_CLASS(ammo_plasma_cell, CAmmoPlasmaCell);