/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
//=========================================================
// monster template
//=========================================================
// UNDONE: Holster weapon?

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "talkmonster.h"
#include "schedule.h"
#include "defaultai.h"
#include "scripted.h"
#include "weapons.h"
#include "soundent.h"
#include "items.h"
#include "gamerules.h"
#include "animation.h"

#define SF_HEVSCI_INDEPENDENT 8

enum
{
	HEAD_HELMET = 0,
	HEAD_EINSTEIN = 1,
	HEAD_LUTHER = 2,
	HEAD_GLASSES = 3,
	HEAD_SLICK = 4,
	HEAD_WOMAN = 5
};

enum
{
	TASK_HEVSCI_HEAL_THINGY = LAST_TALKMONSTER_TASK + 1,
};

enum
{
	SCHED_HEVSCI_RELOADINCOVER = LAST_TALKMONSTER_SCHEDULE + 1,
	SCHED_HEVSCI_RELOAD,
};

class CHEVSci : public CTalkMonster
{
public:
	void Spawn() override;
	void Precache() override;
	void SetYawSpeed() override;
	int ISoundMask() override;
	void AlertSound() override;
	int Classify() override;
	void HandleAnimEvent(MonsterEvent_t* pEvent) override;
	void PrescheduleThink() override;

	void RunTask(Task_t* pTask) override;
	void StartTask(Task_t* pTask) override;
	int ObjectCaps() override { return CTalkMonster::ObjectCaps() | FCAP_IMPULSE_USE; }
	bool TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType) override;
	bool CheckRangeAttack1(float flDot, float flDist) override;

	int FUN_1004c8d9(byte a1);
	void FUN_1004c919(byte a1);
	void Reload();
	void Shotgun();
	void Gauss();
	void MP5();
	void RPG();
	void Python();

	void Touch(CBaseEntity* pOther) override;
	void DeclineFollowing() override;
	void EXPORT IgnoreUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);

	// Override these to set behavior
	Schedule_t* GetScheduleOfType(int Type) override;
	Schedule_t* GetSchedule() override;
	MONSTERSTATE GetIdealState() override;

	void DeathSound() override;
	void PainSound() override;

	void TalkInit();

	void TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType) override;
	void Killed(entvars_t* pevAttacker, int iGib) override;
	void SetActivity(Activity NewActivity) override;
	void MonsterThink() override;

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	float m_painTime;
	float m_checkAttackTime;
	bool m_lastAttackCheck;
	byte m_cClipsize;
	bool m_isDucking;
	bool m_canDuckAttack;

	CItem* m_pUnkItem;

	int m_nShell;
	int m_nShotgunShell;
	int m_nHotGlow;
	int m_nSmoke;

	// UNDONE: What is this for?  It isn't used?
	float m_flPlayerDamage; // how much pain has the player inflicted on me?

	CUSTOM_SCHEDULES;
};

LINK_ENTITY_TO_CLASS(monster_hevsci, CHEVSci);

TYPEDESCRIPTION CHEVSci::m_SaveData[] =
	{
		DEFINE_FIELD(CHEVSci, m_painTime, FIELD_TIME),
		DEFINE_FIELD(CHEVSci, m_checkAttackTime, FIELD_TIME),
		DEFINE_FIELD(CHEVSci, m_lastAttackCheck, FIELD_BOOLEAN),
		DEFINE_FIELD(CHEVSci, m_flPlayerDamage, FIELD_FLOAT),
		DEFINE_FIELD(CHEVSci, m_cClipsize, FIELD_CHARACTER),
		DEFINE_FIELD(CHEVSci, m_isDucking, FIELD_BOOLEAN),
		DEFINE_FIELD(CHEVSci, m_canDuckAttack, FIELD_BOOLEAN),
};

IMPLEMENT_SAVERESTORE(CHEVSci, CTalkMonster);

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
Task_t tlHEVSciFollow[] =
	{
		{TASK_MOVE_TO_TARGET_RANGE, (float)128}, // Move within 128 of target ent (client)
		{TASK_SET_SCHEDULE, (float)SCHED_TARGET_FACE},
};

Schedule_t slHEVSciFollow[] =
	{
		{tlHEVSciFollow,
			ARRAYSIZE(tlHEVSciFollow),
			bits_COND_NEW_ENEMY |
				bits_COND_LIGHT_DAMAGE |
				bits_COND_HEAVY_DAMAGE |
				bits_COND_HEAR_SOUND |
				bits_COND_PROVOKED,
			bits_SOUND_DANGER,
			"HEVSci Follow"},
};

Task_t tlHEVSciFaceTarget[] =
	{
		{TASK_SET_ACTIVITY, (float)ACT_IDLE},
		{TASK_FACE_TARGET, (float)0},
		{TASK_SET_ACTIVITY, (float)ACT_IDLE},
		{TASK_SET_SCHEDULE, (float)SCHED_TARGET_CHASE},
};

Schedule_t slHEVSciFaceTarget[] =
	{
		{tlHEVSciFaceTarget,
			ARRAYSIZE(tlHEVSciFaceTarget),
			bits_COND_CLIENT_PUSH |
				bits_COND_NEW_ENEMY |
				bits_COND_LIGHT_DAMAGE |
				bits_COND_HEAVY_DAMAGE |
				bits_COND_HEAR_SOUND |
				bits_COND_PROVOKED,
			bits_SOUND_DANGER,
			"HEVSci FaceTarget"},
};


Task_t tlHEVSciIdleStand[] =
	{
		{TASK_STOP_MOVING, 0},
		{TASK_SET_ACTIVITY, (float)ACT_IDLE},
		{TASK_WAIT, (float)2},			// repick IDLESTAND every two seconds.
		{TASK_TLK_HEADRESET, (float)0}, // reset head position
};

Schedule_t slHEVSciIdleStand[] =
	{
		{tlHEVSciIdleStand,
			ARRAYSIZE(tlHEVSciIdleStand),
			bits_COND_NEW_ENEMY |
				bits_COND_LIGHT_DAMAGE |
				bits_COND_HEAVY_DAMAGE |
				bits_COND_HEAR_SOUND |
				bits_COND_SMELL |
				bits_COND_PROVOKED,

			bits_SOUND_COMBAT | // sound flags - change these, and you'll break the talking code.
				//bits_SOUND_PLAYER		|
				//bits_SOUND_WORLD		|

				bits_SOUND_DANGER |
				bits_SOUND_MEAT | // scents
				bits_SOUND_CARCASS |
				bits_SOUND_GARBAGE,
			"IdleStand"},
};

Task_t tlHEVSciReloadInCover[] =
	{
		{TASK_STOP_MOVING, 0},
		{TASK_SET_FAIL_SCHEDULE, (float)SCHED_HEVSCI_RELOAD},
		{TASK_FIND_COVER_FROM_ENEMY, (float)0},
		{TASK_RUN_PATH, (float)0},
		{TASK_WAIT_FOR_MOVEMENT, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_PLAY_SEQUENCE, (float)ACT_RELOAD},
};

Schedule_t slHEVSciReloadInCover[] =
	{
		{tlHEVSciReloadInCover,
			ARRAYSIZE(tlHEVSciReloadInCover),
			bits_COND_HEAVY_DAMAGE |
				bits_COND_HEAR_SOUND,

			bits_SOUND_DANGER,
			"HEVSciReloadInCover"}};

Task_t tlHEVSciReload[] =
	{
		{TASK_STOP_MOVING, 0},
		{TASK_WAIT_FOR_MOVEMENT, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_PLAY_SEQUENCE, (float)ACT_RELOAD},
};

Schedule_t slHEVSciReload[] =
	{
		{tlHEVSciReload,
			ARRAYSIZE(tlHEVSciReload),
			bits_COND_HEAVY_DAMAGE |
				bits_COND_HEAR_SOUND,

			bits_SOUND_DANGER,
			"HEVSciReload"}};

DEFINE_CUSTOM_SCHEDULES(CHEVSci){
	slHEVSciFollow,
	slHEVSciFaceTarget,
	slHEVSciIdleStand,
	slHEVSciReloadInCover,
	slHEVSciReload,
};


IMPLEMENT_CUSTOM_SCHEDULES(CHEVSci, CTalkMonster);

void CHEVSci::StartTask(Task_t* pTask)
{
	switch (pTask->iTask)
	{
	case TASK_HEVSCI_HEAL_THINGY:
		if (MoveToLocation(ACT_RUN, 0.0f, m_pUnkItem->pev->origin) == false)
		{
			TaskFail();
			RouteClear();
		}
		break;

	default:
		CTalkMonster::StartTask(pTask);
		break;
	}
}

void CHEVSci::RunTask(Task_t* pTask)
{
	switch (pTask->iTask)
	{
	case TASK_HEVSCI_HEAL_THINGY:
	{
		float distance = (m_vecMoveGoal - pev->origin).Length2D();

		if (distance < 16.0f)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/smallmedkit1.wav", VOL_NORM, ATTN_NORM);
			pev->health = gSkillData.HEVscientistHealth;

			if (g_pGameRules->ItemShouldRespawn(m_pUnkItem) == NULL)
			{
				UTIL_Remove(m_pUnkItem);
			}
			else
			{
				m_pUnkItem->Respawn();
			}

			m_pUnkItem = NULL;
			RouteClear();
			TaskComplete();
		}
		FRefreshRoute();
		break;
	}

	default:
		CTalkMonster::RunTask(pTask);
		break;
	}
}




//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards.
//=========================================================
int CHEVSci::ISoundMask()
{
	return bits_SOUND_WORLD |
		   bits_SOUND_COMBAT |
		   bits_SOUND_CARCASS |
		   bits_SOUND_MEAT |
		   bits_SOUND_GARBAGE |
		   bits_SOUND_DANGER |
		   bits_SOUND_PLAYER;
}

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int CHEVSci::Classify()
{
	return CLASS_PLAYER_ALLY;
}

//=========================================================
// ALertSound - barney says "Freeze!"
//=========================================================
void CHEVSci::AlertSound()
{
	if (m_hEnemy != NULL)
	{
		if (FOkToSpeak())
		{
			PlaySentence("SC_ATTACK", RANDOM_FLOAT(2.8, 3.2), VOL_NORM, ATTN_IDLE);
		}
	}
}
//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CHEVSci::SetYawSpeed()
{
	int ys;

	ys = 0;

	switch (m_Activity)
	{
	case ACT_IDLE:
		ys = 70;
		break;
	case ACT_WALK:
		ys = 70;
		break;
	case ACT_RUN:
		ys = 90;
		break;
	default:
		ys = 70;
		break;
	}

	pev->yaw_speed = ys;
}


//=========================================================
// CheckRangeAttack1
//=========================================================
bool CHEVSci::CheckRangeAttack1(float flDot, float flDist)
{
	if (flDist <= 1024 && flDot >= 0.5)
	{
		if (gpGlobals->time > m_checkAttackTime)
		{
			TraceResult tr;

			Vector shootOrigin = pev->origin + Vector(0, 0, 55);
			CBaseEntity* pEnemy = m_hEnemy;
			Vector shootTarget = ((pEnemy->BodyTarget(shootOrigin) - pEnemy->pev->origin) + m_vecEnemyLKP);
			UTIL_TraceLine(shootOrigin, shootTarget, dont_ignore_monsters, ENT(pev), &tr);
			m_checkAttackTime = gpGlobals->time + 1;
			if (tr.flFraction == 1.0 || (tr.pHit != NULL && CBaseEntity::Instance(tr.pHit) == pEnemy))
			{
				m_lastAttackCheck = true;
				Vector shootOrigin2 = pev->origin + Vector(0, 0, 35);
				Vector shootTarget2 = ((pEnemy->BodyTarget(shootOrigin) - pEnemy->pev->origin) + m_vecEnemyLKP);
				UTIL_TraceLine(shootOrigin2, shootTarget2, dont_ignore_monsters, ENT(pev), &tr);
				m_checkAttackTime = gpGlobals->time + 1;

				if (tr.flFraction == 1.0 || (tr.pHit != NULL && CBaseEntity::Instance(tr.pHit) == pEnemy))
					m_canDuckAttack = true;
			}
			else
			{
				m_lastAttackCheck = false;
				m_canDuckAttack = false;
			}
			m_checkAttackTime = gpGlobals->time + 1.5;
		}
		return m_lastAttackCheck;
	}
	return false;
}

int CHEVSci::FUN_1004c8d9(byte a1)
{
	int a2;

	if (m_cClipsize == -1 || a1 <= m_cAmmoLoaded)
	{
		a2 = 1;
	}
	else
	{
		a2 = 0;
	}

	return a2;
}

void CHEVSci::FUN_1004c919(byte a1)
{
	if (m_cClipsize != -1)
		m_cAmmoLoaded -= a1;
}

void CHEVSci::Reload()
{
	if (m_cClipsize != -1)
		m_cAmmoLoaded = m_cClipsize;

}

void CHEVSci::Shotgun()
{
	if (FUN_1004c8d9(1) != 0)
	{
		Vector vecOrigin, vecAngles;
		UTIL_MakeVectors(pev->angles);

		GetAttachment(3, vecOrigin, vecAngles);

		Vector vecShootDir = ShootAtEnemy(vecOrigin);

		Vector angDir = UTIL_VecToAngles(vecShootDir);
		SetBlending(0, angDir.x);

		Vector vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40, 90) + gpGlobals->v_up * RANDOM_FLOAT(75, 200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
		EjectBrass(vecOrigin  - vecAngles * 24, vecShellVelocity, pev->angles.y, m_nShotgunShell, TE_BOUNCE_SHOTSHELL);
		FireBullets(gSkillData.hgruntShotgunPellets, vecOrigin, vecShootDir, VECTOR_CONE_15DEGREES, 2048, BULLET_PLAYER_BUCKSHOT, 0); // shoot +-7.5 degrees

		FUN_1004c919(1);

		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/sbarrel1.wav", 1, ATTN_NORM, 0, 100);

		CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 384, 0.3);
	}
}

void CHEVSci::Gauss()
{
	if (FUN_1004c8d9(1) != 0)
	{
		Vector vecOrigin, vecAngles;
		UTIL_MakeVectors(pev->angles);

		GetAttachment(3, vecOrigin, vecAngles);

		Vector originDir = ShootAtEnemy(vecOrigin);

		Vector gaussVelocity = Vector(RANDOM_FLOAT(-0.05, 0.05), RANDOM_FLOAT(-0.05, 0.05), RANDOM_FLOAT(-0.05, 0.05));

		vecAngles = originDir + gaussVelocity;

		Vector vecDest = vecOrigin + vecAngles * 8192;

		TraceResult tr;

		UTIL_MakeVectors(pev->angles);

		Vector angDir = UTIL_VecToAngles(vecAngles);
		SetBlending(0, angDir.x);

		pev->effects = EF_MUZZLEFLASH;

		UTIL_TraceLine(vecOrigin, vecDest, dont_ignore_monsters, ENT(pev), &tr);
	
		CBaseEntity* pEntity = CBaseEntity::Instance(tr.pHit);

		if (!pEntity || pev->takedamage == 0.0f)
		{
			MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, tr.vecEndPos);
			WRITE_COORD(tr.vecEndPos.x);
			WRITE_COORD(tr.vecEndPos.y);
			WRITE_COORD(tr.vecEndPos.z);
			WRITE_SHORT(m_nHotGlow);
			WRITE_BYTE(TE_BEAMDISK);
			WRITE_BYTE(TE_EXPLOSION);
			WRITE_BYTE(200);
			MESSAGE_END();

			MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, tr.vecEndPos);
			WRITE_BYTE(TE_SPRITETRAIL);
			WRITE_COORD(tr.vecEndPos.x);
			WRITE_COORD(tr.vecEndPos.y);
			WRITE_COORD(tr.vecEndPos.z);
			WRITE_COORD(tr.vecEndPos.x + tr.vecPlaneNormal.x);
			WRITE_COORD(tr.vecEndPos.y + tr.vecPlaneNormal.y);
			WRITE_COORD(tr.vecEndPos.z + tr.vecPlaneNormal.z);
			WRITE_SHORT(m_nHotGlow);
			WRITE_BYTE(TE_BEAMENTS);
			WRITE_BYTE(TE_TRACER);
			WRITE_BYTE(RANDOM_LONG(1,2));
			WRITE_BYTE(TE_LAVASPLASH);
			WRITE_BYTE(TE_BEAMDISK);
			MESSAGE_END();
		}
		else
		{
			ClearMultiDamage();
			pEntity->TraceAttack(pev, gSkillData.plrDmgGauss, vecOrigin, &tr, DMG_BULLET);
			ApplyMultiDamage(pev, pev);
		}

		FUN_1004c919(1);

		MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, tr.vecEndPos);
		WRITE_BYTE(TE_BEAMENTPOINT);
		WRITE_SHORT(entindex() + 4096);
		WRITE_COORD(tr.vecEndPos.x);
		WRITE_COORD(tr.vecEndPos.y);
		WRITE_COORD(tr.vecEndPos.z);
		WRITE_SHORT(m_nSmoke);
		WRITE_BYTE(TE_BEAMPOINTS);
		WRITE_BYTE(TE_BEAMPOINTS);
		WRITE_BYTE(TE_BEAMENTPOINT);
		WRITE_BYTE(TE_LAVASPLASH);
		WRITE_BYTE(TE_BEAMPOINTS);
		WRITE_BYTE(255);
		WRITE_BYTE(128);
		WRITE_BYTE(TE_BEAMPOINTS);
		WRITE_BYTE(128);
		WRITE_BYTE(TE_BEAMPOINTS);
		MESSAGE_END();

		int pitchShift = RANDOM_LONG(0, 20);

		// Only shift about half the time
		if (pitchShift > 10)
			pitchShift = 0;
		else
			pitchShift -= 5;

		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/gauss2.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(0, 31) + 85);
		CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 384, 0.3);
	}
}

void CHEVSci::MP5()
{
	if (FUN_1004c8d9(1) != 0)
	{
		Vector vecOrigin, vecAngles;
		UTIL_MakeVectors(pev->angles);

		GetAttachment(3, vecOrigin, vecAngles);

		Vector vecShootDir = ShootAtEnemy(vecOrigin);

		Vector angDir = UTIL_VecToAngles(vecAngles);
		SetBlending(0, angDir.x);

		pev->effects = EF_MUZZLEFLASH;

		Vector vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40, 90) + gpGlobals->v_up * RANDOM_FLOAT(75, 200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
		EjectBrass(vecOrigin - vecAngles * 24, vecShellVelocity, pev->angles.y, m_nShell, TE_BOUNCE_SHELL);
		FireBullets(1, vecOrigin, vecShootDir, VECTOR_CONE_10DEGREES, 2048, BULLET_MONSTER_MP5);
	
		FUN_1004c919(1);

		int pitchShift = RANDOM_LONG(0, 10);

		pitchShift -= 5;

		switch (RANDOM_LONG(0, 2))
		{
		case 0:
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/hks1.wav", VOL_NORM, ATTN_NORM, 0, pitchShift + 100);
			break;
		case 1:
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/hks2.wav", VOL_NORM, ATTN_NORM, 0, pitchShift + 100);
			break;
		case 2:
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/hks3.wav", VOL_NORM, ATTN_NORM, 0, pitchShift + 100);
			break;
		}
		CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 384, 0.3);
	}
}

void CHEVSci::RPG()
{
	if (FUN_1004c8d9(1) != 0)
	{
		Vector vecOrigin, vecAngles;
		UTIL_MakeVectors(pev->angles);

		GetAttachment(1, vecOrigin, vecAngles);

		if (m_hEnemy != NULL)
		{
			if (pev->velocity.Length() > 64.0f)
			{
				m_vecEnemyLKP - vecOrigin;
				Vector unk = m_hEnemy->pev->velocity*((pev->velocity.Length() / 2000.0) + 0.4);
				Vector vecShootDir = ShootAtEnemy(vecOrigin);

				vecShootDir + unk / 2.0f;
			}
			else
			{
				Vector vecShootDir = ShootAtEnemy(vecOrigin);
			}

			Vector angDir = UTIL_VecToAngles(vecAngles);

			angDir.x = angDir.x * -1.0f;

			SetBlending(0, angDir.x);

			CBaseEntity* pRocket = CBaseEntity::Create("rpg_rocket", vecOrigin, angDir, edict());
		
			FUN_1004c919(1);

			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/rocketfire1.wav", 0.9f, ATTN_NORM);
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "weapons/glauncher.wav", 0.7f, ATTN_NORM);

			CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 384, 0.3);
		}
	}
}

void CHEVSci::Python()
{
	if (FUN_1004c8d9(1) != 0)
	{
		Vector vecOrigin, vecAngles;
		UTIL_MakeVectors(pev->angles);

		GetAttachment(2, vecOrigin, vecAngles);

		Vector vecShootDir = ShootAtEnemy(vecOrigin);

		Vector angDir = UTIL_VecToAngles(vecShootDir);
		SetBlending(0, angDir.x);

		pev->effects = EF_MUZZLEFLASH;

		Vector vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(150, 240) + gpGlobals->v_up * RANDOM_FLOAT(120, 300) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
		EjectBrass(vecOrigin - vecAngles * 24, vecShellVelocity, pev->angles.y, m_nShell, TE_BOUNCE_SHELL);
		FireBullets(1, vecOrigin, vecShootDir, VECTOR_CONE_5DEGREES, 4096, BULLET_PLAYER_357);

		FUN_1004c919(1);

		switch (RANDOM_LONG(0, 1))
		{
		case 0:
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/357_shot1.wav", VOL_NORM, ATTN_NORM);
			break;
		case 1:
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/357_shot2.wav", VOL_NORM, ATTN_NORM);
			break;
		}
		CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 384, 0.3);
	}
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CHEVSci::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	Vector weaponOrigin, weaponAngles;

	switch (pEvent->event)
	{
	case 1:
		Shotgun();
		break;

	case 2:
		Gauss();
		break;

	case 3:
		MP5();
		break;
	
	case 4:
		RPG();
		break;
	
	case 5:
		Python();
		break;

	case 6:
		Reload();
		break;

	case 7:
		GetAttachment(0, weaponOrigin, weaponAngles);
		switch (pev->weapons)
		{
		case 2:
			DropItem("weapon_shotgun", weaponOrigin, weaponAngles);
			break;

		case 4:
			DropItem("weapon_gauss", weaponOrigin, weaponAngles);
			break;

		case 8:
			DropItem("weapon_mp5", weaponOrigin, weaponAngles);
			break;

		case 16:
			DropItem("weapon_rpg", weaponOrigin, weaponAngles);
			break;

		case 32:
			DropItem("weapon_357", weaponOrigin, weaponAngles);
			break;
		}
		SetBodygroup(2,0);
		break;

	default:
		CTalkMonster::HandleAnimEvent(pEvent);
	}
}

void CHEVSci::PrescheduleThink()
{
	if (pev->skin != 0)
	{
		pev->skin = 0;
	}

	if (RANDOM_LONG(0, 36) == 0)
	{
		pev->skin = 1;
	}

	CTalkMonster::PrescheduleThink();
}

//=========================================================
// Spawn
//=========================================================
void CHEVSci::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/HEVsci.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = gSkillData.HEVscientistHealth;
	pev->view_ofs = Vector(0, 0, 73);  // position of the eyes relative to monster's origin.
	m_flFieldOfView = VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState = MONSTERSTATE_NONE;

	if (((((pev->weapons != 1) && (pev->weapons != 2)) && (pev->weapons != 4)) &&
	((pev->weapons != 8 && (pev->weapons != 16)))) && (pev->weapons != 32))
	{
		pev->weapons = 1;
	}

	SetBlending(0, RANDOM_LONG(-20,0));

	if ((pev->body < 1) || (5 < pev->body))
	{
		pev->body = 0;
	}

	SetBodygroup(1, pev->body);

	switch (this->pev->weapons)
	{
	case 1:
		m_cClipsize = -1;
		break;
	
	case 2:
		m_cClipsize = 8;
		m_cAmmoLoaded = 8;
		SetBodygroup(2, 1);
		break;
	
	case 4:
		m_cClipsize = -1;
		m_cAmmoLoaded = 1;
		SetBodygroup(2, 2);
		break;
	
	case 8:
		m_cClipsize = 36;
		m_cAmmoLoaded = 36;
		SetBodygroup(2, 3);
		break;
	
	case 16:
		m_cClipsize = 1;
		m_cAmmoLoaded = 1;
		SetBodygroup(2, 4);
		break;
	
	case 32:
		m_cClipsize = 6;
		m_cAmmoLoaded = 6;
		SetBodygroup(2, 5);
		break;
	}

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;
	m_isDucking = false;


	MonsterInit();
	SetUse(&CHEVSci::FollowerUse);
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CHEVSci::Precache()
{
	PRECACHE_MODEL("models/HEVsci.mdl");

	PRECACHE_SOUND("scientist/sci_pain1.wav");
	PRECACHE_SOUND("scientist/sci_pain2.wav");
	PRECACHE_SOUND("scientist/sci_pain3.wav");
	PRECACHE_SOUND("scientist/sci_pain4.wav");
	PRECACHE_SOUND("scientist/sci_pain5.wav");
	PRECACHE_SOUND("scientist/scream21.wav");
	PRECACHE_SOUND("scientist/sci_pain7.wav");
	PRECACHE_SOUND("scientist/sci_dragoff.wav");

	PRECACHE_SOUND("weapons/sbarrel1.wav");
	PRECACHE_SOUND("weapons/gauss2.wav");

	m_nHotGlow = PRECACHE_MODEL("sprites/hotglow.spr");
	m_nSmoke = PRECACHE_MODEL("sprites/smoke.spr");

	PRECACHE_SOUND("weapons/hks1.wav");
	PRECACHE_SOUND("weapons/hks2.wav");
	PRECACHE_SOUND("weapons/hks3.wav");
	PRECACHE_SOUND("weapons/glauncher.wav");
	PRECACHE_SOUND("weapons/rocketfire1.wav");

	UTIL_PrecacheOther("rpg_rocket");

	PRECACHE_SOUND("weapons/357_shot1.wav");
	PRECACHE_SOUND("weapons/357_shot2.wav");

	m_nShell = PRECACHE_MODEL("models/shell.mdl");
	m_nShotgunShell = PRECACHE_MODEL("models/shotgunshell.mdl");

	TalkInit();
	CTalkMonster::Precache();
}

// Init talk data
void CHEVSci::TalkInit()
{

	CTalkMonster::TalkInit();

	// scientists speach group names (group names are in sentences.txt)

	m_szGrp[TLK_ANSWER] = "SC_ANSWER";
	m_szGrp[TLK_QUESTION] = "SC_QUESTION";
	m_szGrp[TLK_IDLE] = "SC_IDLE";
	m_szGrp[TLK_STARE] = "SC_STARE";
	m_szGrp[TLK_USE] = "SC_OK";
	m_szGrp[TLK_UNUSE] = "SC_WAIT";
	m_szGrp[TLK_STOP] = "SC_STOP";

	m_szGrp[TLK_NOSHOOT] = "SC_SCARED";
	m_szGrp[TLK_HELLO] = "SC_HELLO";

	m_szGrp[TLK_PLHURT1] = "!SC_CUREA";
	m_szGrp[TLK_PLHURT2] = "!SC_CUREB";
	m_szGrp[TLK_PLHURT3] = "!SC_CUREC";

	m_szGrp[TLK_PHELLO] = "SC_PHELLO";
	m_szGrp[TLK_PIDLE] = "SC_PIDLE";
	m_szGrp[TLK_PQUESTION] = "SC_PQUEST";

	m_szGrp[TLK_SMELL] = "SC_SMELL";

	m_szGrp[TLK_WOUND] = "SC_WOUND";
	m_szGrp[TLK_MORTAL] = "SC_MORTAL";

	// get voice for head
	switch (pev->body)
	{
	case HEAD_EINSTEIN:
		m_voicePitch = 100;
		break;
	case HEAD_LUTHER:
		m_voicePitch = 95;
		break;
	case HEAD_GLASSES:
		m_voicePitch = 105;
		break;
	case HEAD_SLICK:
		m_voicePitch = 100;
		break;
	case HEAD_WOMAN:
		m_voicePitch = 125;
		break;
	default:
		m_voicePitch = 90;
		break;
	}
}

bool CHEVSci::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	if ((pev->spawnflags & SF_HEVSCI_INDEPENDENT) != 0)
	{
		flDamage = 0.0f;
	}

	// make sure friends talk about it if player hurts talkmonsters...
	bool ret = CTalkMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
	if (!IsAlive() || pev->deadflag == DEAD_DYING)
		return ret;

	if (m_MonsterState != MONSTERSTATE_PRONE && (pevAttacker->flags & FL_CLIENT) != 0)
	{
		m_flPlayerDamage += flDamage;

		// This is a heurstic to determine if the player intended to harm me
		// If I have an enemy, we can't establish intent (may just be crossfire)
		if (m_hEnemy == NULL)
		{
			// If the player was facing directly at me, or I'm already suspicious, get mad
			if ((m_afMemory & bits_MEMORY_SUSPICIOUS) != 0 || IsFacing(pevAttacker, pev->origin))
			{
				// Alright, now I'm pissed!
				PlaySentence("SC_MAD", 4, VOL_NORM, ATTN_NORM);

				Remember(bits_MEMORY_PROVOKED);
				StopFollowing(true);
			}
			else
			{
				// Hey, be careful with that
				PlaySentence("SC_SHOT", 4, VOL_NORM, ATTN_NORM);
				Remember(bits_MEMORY_SUSPICIOUS);
			}
		}
		else if (!(m_hEnemy->IsPlayer()) && pev->deadflag == DEAD_NO)
		{
			PlaySentence("SC_SHOT", 4, VOL_NORM, ATTN_NORM);
		}
	}

	return ret;
}


//=========================================================
// PainSound
//=========================================================
void CHEVSci::PainSound()
{
	if ((pev->spawnflags & SF_HEVSCI_INDEPENDENT) != 0)
		return;

	if (gpGlobals->time < m_painTime)
		return;

	m_painTime = gpGlobals->time + RANDOM_FLOAT(0.5, 0.75);

	switch (RANDOM_LONG(0, 2))
	{
	case 0:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "scientist/sci_pain1.wav", 1, ATTN_NORM, 0, GetVoicePitch());
		break;
	case 1:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "scientist/sci_pain2.wav", 1, ATTN_NORM, 0, GetVoicePitch());
		break;
	case 2:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "scientist/sci_pain3.wav", 1, ATTN_NORM, 0, GetVoicePitch());
		break;
	case 3:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "scientist/sci_pain4.wav", 1, ATTN_NORM, 0, GetVoicePitch());
		break;
	case 4:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "scientist/sci_pain5.wav", 1, ATTN_NORM, 0, GetVoicePitch());
		break;
	}
}

//=========================================================
// DeathSound
//=========================================================
void CHEVSci::DeathSound()
{
	switch (RANDOM_LONG(0, 2))
	{
	case 0:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "scientist/scream21.wav", 1, ATTN_NORM, 0, GetVoicePitch());
		break;
	case 1:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "scientist/sci_pain7.wav", 1, ATTN_NORM, 0, GetVoicePitch());
		break;
	case 2:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "scientist/sci_dragoff.wav", 1, ATTN_NORM, 0, GetVoicePitch());
		break;
	}
}


void CHEVSci::TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	switch (ptr->iHitgroup)
	{
	case HITGROUP_CHEST:
	case HITGROUP_STOMACH:
		if ((bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_BLAST)) != 0)
		{
			flDamage = flDamage / 2;
		}
		break;
	case 10:
		if ((bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_CLUB)) != 0)
		{
			flDamage -= 20;
			if (flDamage <= 0)
			{
				UTIL_Ricochet(ptr->vecEndPos, 1.0);
				flDamage = 0.01;
			}
		}
		// always a head shot
		ptr->iHitgroup = HITGROUP_HEAD;
		break;
	}

	CTalkMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
}


void CHEVSci::Killed(entvars_t* pevAttacker, int iGib)
{
	SetUse(NULL);
	CTalkMonster::Killed(pevAttacker, iGib);
}

void CHEVSci::SetActivity(Activity NewActivity)
{
	int iSequence = ACTIVITY_NOT_AVAILABLE;

	TraceResult tr;

	if (m_isDucking)
	{
		UTIL_TraceHull(pev->origin + Vector(-16.0, -16.0, 37.0), pev->origin + Vector(16.0, 16.0, 71.0), dont_ignore_monsters, head_hull, edict(), &tr);

		CBaseEntity* pEntity = CBaseEntity::Instance(tr.pHit);
		if (tr.pHit != NULL || tr.pHit == edict() || FClassnameIs(pEntity->pev, "worldspawn"))
		{
			m_isDucking = false;
		}
		else
		{
			if (pEntity)
			{
				ALERT(at_aiconsole, " HEVSci hit %s\n", &gpGlobals->pStringBase[pEntity->pev->classname]);
			}
		}
	}

	switch (NewActivity)
	{
	default:
		iSequence = LookupActivity(NewActivity);
		break;
	
	case ACT_IDLE:
	case ACT_SIGNAL3:
		SetBlending(0, RANDOM_LONG(-20, 0));
	
		if (m_isDucking)
		{
			iSequence = LookupSequence("crouch_aim_gauss");
		}
		else
		{
			if ((pev->weapons & 1) != 0)
			{
				iSequence = LookupSequence("look_idle");
			}
			if ((pev->weapons & 2) != 0)
			{
				iSequence = LookupSequence("ref_aim_shotgun");
			}
			if ((pev->weapons & 4) != 0)
			{
				iSequence = LookupSequence("ref_aim_gauss");
			}
			if ((pev->weapons & 8) != 0)
			{
				iSequence = LookupSequence("ref_aim_mp5");
			}
			if ((pev->weapons & 16) != 0)
			{
				iSequence = LookupSequence("ref_aim_rpg");
			}
			if ((pev->weapons & 32) != 0)
			{
				iSequence = LookupSequence("ref_aim_python");
			}

			int idleRNG = RANDOM_LONG(0, 3);

			if (idleRNG == 0)
			{
				iSequence = LookupSequence("look_idle");
			}
			else if (idleRNG == 1)
			{
				iSequence = LookupSequence("deep_idle");
			}
		}
		break;
	
	case ACT_WALK:
	case ACT_RUN:
		if (!m_isDucking)
		{
			iSequence = LookupActivity(NewActivity);
		}
		else
		{
			iSequence = LookupSequence("crouch_aim_gauss");
		}
		break;
	
	case ACT_RANGE_ATTACK1:
		if ((pev->weapons & 2) != 0)
		{
			if (m_canDuckAttack)
			{
				iSequence = LookupSequence("crouch_shoot_shotgun");
				m_isDucking = true;
			}
			else
			{
				iSequence = LookupSequence("ref_shoot_shotgun");
			}
		}
	
		if ((pev->weapons & 4) != 0)
		{
			if (m_canDuckAttack)
			{
				iSequence = LookupSequence("crouch_shoot_gauss");
				m_isDucking = true;
			}
			else
			{
				iSequence = LookupSequence("ref_shoot_gauss");
			}
		}
	
		if ((pev->weapons & 8) != 0)
		{
			if (m_canDuckAttack)
			{
				iSequence = LookupSequence("crouch_shoot_MP5");
				m_isDucking = true;
			}
			else
			{
				iSequence = LookupSequence("ref_shoot_MP5");
			}
		}
	
		if ((pev->weapons & 16) != 0)
		{
			if (m_canDuckAttack)
			{
				iSequence = LookupSequence("crouch_shoot_RPG");
				m_isDucking = true;
			}
			else
			{
				iSequence = LookupSequence("ref_shoot_RPG");
			}
		}
	
		if ((pev->weapons & 32) != 0)
		{
			pev->framerate = RANDOM_FLOAT(0.8, 1.5);
	
			if (m_canDuckAttack)
			{
				iSequence = LookupSequence("crouch_shoot_python");
				m_isDucking = true;
			}
			else
			{
				iSequence = LookupSequence("ref_shoot_python");
			}
		}
		break;
	
	case ACT_RELOAD:
		m_isDucking = true;
		iSequence = LookupActivity(NewActivity);
		break;
	}

	if (m_isDucking)
	{
		UTIL_SetSize(pev, Vector(-16.0, -16.0, 0.0), Vector(16.0, 16.0, 36.0));
	}
	else
	{
		UTIL_SetSize(pev, Vector(-16.0, -16.0, 0.0), Vector(16.0, 16.0, 72.0));
	}

	if (NewActivity == ACT_RANGE_ATTACK1
		|| NewActivity == ACT_RELOAD
		|| NewActivity == ACT_DIESIMPLE
		|| NewActivity == ACT_DIEBACKWARD
		|| NewActivity == ACT_DIEFORWARD
		|| NewActivity == ACT_DIEVIOLENT
		|| !m_isDucking)
	{
		m_Activity = NewActivity;
	}

	// Set to the desired anim, or default anim if the desired is not present
	if (iSequence > ACTIVITY_NOT_AVAILABLE)
	{
		if (pev->sequence != iSequence || !m_fSequenceLoops)
		{
			pev->frame = 0;
		}

		pev->sequence = iSequence; // Set to the reset anim (if it's there)
		ResetSequenceInfo();
		SetYawSpeed();
	}
	else
	{
		// Not available try to get default anim
		ALERT(at_console, "%s has no sequence for act:%d\n", STRING(pev->classname), NewActivity);
		pev->sequence = 0; // Set to the reset anim (if it's there)
	}
}

void CHEVSci::MonsterThink()
{
	pev->nextthink = gpGlobals->time + 0.1;

	RunAI();

	float flInterval = StudioFrameAdvance();	
	if (m_MonsterState != MONSTERSTATE_SCRIPT && m_MonsterState != MONSTERSTATE_DEAD && m_Activity == ACT_IDLE && m_fSequenceFinished)
	{
		int iSequence = ACTIVITY_NOT_AVAILABLE;

		if (m_fSequenceLoops)
		{
			if (m_Activity == ACT_IDLE)
				SetActivity(ACT_IDLE);
			else
				iSequence = LookupActivity(m_Activity);
		}
		else
		{
			iSequence = LookupActivityHeaviest(m_Activity);
		}
		if (iSequence != ACTIVITY_NOT_AVAILABLE)
		{
			pev->sequence = iSequence; // Set to new anim (if it's there)
			ResetSequenceInfo();
		}
	}

	DispatchAnimEvents(flInterval);

	if (!MovementIsComplete())
	{
		Move(flInterval);
	}
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

Schedule_t* CHEVSci::GetScheduleOfType(int Type)
{
	Schedule_t* psched;

	switch (Type)
	{
	case SCHED_IDLE_STAND:
		// call base class default so that scientist will talk
		// when standing during idle
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
		{
			// just look straight ahead.
			return slHEVSciIdleStand;
		}
		else
			return psched;
		
		break;

	// Hook these to make a looping schedule
	case SCHED_TARGET_FACE:
		// call base class default so that barney will talk
		// when 'used'
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
			return slHEVSciFaceTarget; // override this for different target face behavior
		else
			return psched;

		break;

	case SCHED_TARGET_CHASE:
		return slHEVSciFollow;
		break;

	case SCHED_RANGE_ATTACK1:
		psched = CTalkMonster::GetScheduleOfType(Type);

		if ((pev->weapons & 1) == 0)
		{
			return psched;
		}
		else
		{
			return GetScheduleOfType(SCHED_TAKE_COVER_FROM_ENEMY);
		}
		break;

	case SCHED_HEVSCI_RELOADINCOVER:
		return slHEVSciReloadInCover;
		break;

	case SCHED_HEVSCI_RELOAD:
		return slHEVSciReload;
		break;
	}

	return CTalkMonster::GetScheduleOfType(Type);
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t* CHEVSci::GetSchedule()
{
	if (HasConditions(bits_COND_HEAR_SOUND))
	{
		CSound* pSound;
		pSound = PBestSound();

		ASSERT(pSound != NULL);
		if (pSound && (pSound->m_iType & bits_SOUND_DANGER) != 0)
			return GetScheduleOfType(SCHED_TAKE_COVER_FROM_BEST_SOUND);
	}
	if (HasConditions(bits_COND_ENEMY_DEAD) && FOkToSpeak())
	{
		PlaySentence("SC_KILL", 4, VOL_NORM, ATTN_NORM);
	}

	if (FUN_1004c8d9(1) == 0)
	{
		return GetScheduleOfType(SCHED_HEVSCI_RELOADINCOVER);
	}

	switch (m_MonsterState)
	{
	case MONSTERSTATE_COMBAT:
	{
		ClearConditions(bits_COND_LIGHT_DAMAGE);

		// dead enemy
		if (!HasConditions(bits_COND_ENEMY_DEAD))
		{
			if (HasConditions(bits_COND_HEAVY_DAMAGE))
				return GetScheduleOfType(SCHED_TAKE_COVER_FROM_ENEMY);
			if (HasConditions(bits_COND_ENEMY_OCCLUDED))
				return GetScheduleOfType(SCHED_CHASE_ENEMY);
		}
	}
	break;

	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_IDLE:
		if (HasConditions(bits_COND_HEAVY_DAMAGE))
		{
			// flinch if hurt
			return GetScheduleOfType(SCHED_SMALL_FLINCH);
		}

		if (m_hEnemy == NULL && IsFollowing())
		{
			if (!m_hTargetEnt->IsAlive())
			{
				// UNDONE: Comment about the recently dead player here?
				StopFollowing(false);
				break;
			}
			else
			{
				if (HasConditions(bits_COND_CLIENT_PUSH))
				{
					return GetScheduleOfType(SCHED_MOVE_AWAY_FOLLOW);
				}
				return GetScheduleOfType(SCHED_TARGET_FACE);
			}
		}

		if (HasConditions(bits_COND_CLIENT_PUSH))
		{
			return GetScheduleOfType(SCHED_MOVE_AWAY);
		}

		if (FUN_1004c8d9(m_cClipsize) == 0)
		{
			return GetScheduleOfType(SCHED_HEVSCI_RELOAD);
		}

		// try to say something about smells
		TrySmellTalk();
		break;
	}

	return CTalkMonster::GetSchedule();
}

MONSTERSTATE CHEVSci::GetIdealState()
{
	return CTalkMonster::GetIdealState();
}

void CHEVSci::Touch(CBaseEntity* pOther)
{
	if ((pev->spawnflags & SF_HEVSCI_INDEPENDENT) == 0)
	{
		CTalkMonster::Touch(pOther);
	}
	else
	{
		if (!FOkToSpeak())
			DeclineFollowing();
	}
}

void CHEVSci::DeclineFollowing()
{
	PlaySentence("SC_POK", 2, VOL_NORM, ATTN_NORM);
}

void CHEVSci::IgnoreUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	DeclineFollowing();
}


