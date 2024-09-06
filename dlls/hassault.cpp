#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "soundent.h"
#include "weapons.h"
#include "animation.h"

enum
{
	SCHED_HASSUALT_LINEOFFIRE2 = LAST_COMMON_SCHEDULE + 1,
};

	class CHAssault : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	void StartTask(Task_t* pTask) override;
	void RunTask(Task_t* pTask) override;
	int ISoundMask() override;
	int Classify() override;
	void SetYawSpeed() override;
	bool CheckRangeAttack1(float flDot, float flDist) override;
	int FUN_10048c1e();
	bool FUN_10048c66();
	void FUN_10048c9a();
	void FUN_10048e32(int a1);
	void FUN_10048f34();
	void HandleAnimEvent(MonsterEvent_t* pEvent) override;
	void AlertSound() override;
	void PainSound() override;
	void DeathSound() override;
	void TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType) override;
	void Killed(entvars_t* pevAttacker, int iGib) override;
	void SetActivity(Activity NewActivity) override;
	Schedule_t* GetScheduleOfType(int Type) override;
	Schedule_t* GetSchedule() override;

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	float m_painTime;
	float m_checkAttackTime;
	int m_lastAttackCheck;
	float m_nextSpinTime;
	char m_chGunstate;
	float m_flStatetime;

	Vector unk;
	Vector unk2;

	CUSTOM_SCHEDULES;
};

LINK_ENTITY_TO_CLASS(monster_human_assault, CHAssault);

TYPEDESCRIPTION CHAssault::m_SaveData[] =
	{
		DEFINE_FIELD(CHAssault, m_painTime, FIELD_TIME),
		DEFINE_FIELD(CHAssault, m_checkAttackTime, FIELD_TIME),
		DEFINE_FIELD(CHAssault, m_lastAttackCheck, FIELD_BOOLEAN),
		DEFINE_FIELD(CHAssault, m_chGunstate, FIELD_CHARACTER),
		DEFINE_FIELD(CHAssault, m_flStatetime, FIELD_FLOAT),
		DEFINE_FIELD(CHAssault, m_nextSpinTime, FIELD_TIME),
};

IMPLEMENT_SAVERESTORE(CHAssault, CBaseMonster);

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
Task_t tlHAssaultFail[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_SET_ACTIVITY, (float)ACT_IDLE},
		{TASK_WAIT, (float)2},
		{TASK_WAIT_PVS, (float)3},
};

Schedule_t slHAssaultFail[] =
	{
		{tlHAssaultFail,
			ARRAYSIZE(tlHAssaultFail),
			bits_COND_CAN_RANGE_ATTACK1,
			0,
			"HAssault Fail"},
};

Task_t tlHAssaultCombatFail[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_SET_ACTIVITY, (float)ACT_IDLE},
		{TASK_WAIT, (float)2},
		{TASK_WAIT_PVS, (float)3},
};

Schedule_t slHAssaultCombatFail[] =
	{
		{tlHAssaultCombatFail,
			ARRAYSIZE(tlHAssaultCombatFail),
			bits_COND_CAN_RANGE_ATTACK1,
			0,
			"HAssault Combat Fail"},
};

Task_t tlHAssaultEstablishLineOfFire[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_FACE_IDEAL, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
};

Schedule_t slHAssaultEstablishLineOfFire[] =
	{
		{tlHAssaultEstablishLineOfFire,
			ARRAYSIZE(tlHAssaultEstablishLineOfFire),
			bits_COND_NEW_ENEMY |
				bits_COND_ENEMY_DEAD |
				bits_COND_HEAR_SOUND |
				bits_COND_HEAVY_DAMAGE,

			bits_SOUND_DANGER,
			"HAssaultEstablishLineOfFire"},
};

Task_t tlHAssaultEstablishLineOfFire2[] =
	{
		{TASK_GET_PATH_TO_ENEMY, (float)0},
		{TASK_RUN_PATH, (float)0},
		{TASK_WAIT_FOR_MOVEMENT, (float)0},
};

Schedule_t slHAssaultEstablishLineOfFire2[] =
	{
		{tlHAssaultEstablishLineOfFire2,
			ARRAYSIZE(tlHAssaultEstablishLineOfFire2),
			bits_COND_NEW_ENEMY |
				bits_COND_ENEMY_DEAD |
				bits_COND_HEAR_SOUND |
				bits_COND_CAN_RANGE_ATTACK1,

			bits_SOUND_DANGER,
			"HAssaultEstablishLineOfFire"},
};

DEFINE_CUSTOM_SCHEDULES(CHAssault){
	slHAssaultFail,
	slHAssaultCombatFail,
	slHAssaultEstablishLineOfFire,
	slHAssaultEstablishLineOfFire2,
};

IMPLEMENT_CUSTOM_SCHEDULES(CHAssault, CBaseMonster);

void CHAssault::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/hassault.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = gSkillData.hassaultHealth;
	pev->view_ofs = Vector(0, 0, 50);
	m_flFieldOfView = 0.2f;
	m_MonsterState = MONSTERSTATE_NONE;
	unk = Vector(0,0,0);
	
	pev->body = 0;

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;
	SetBodygroup(1,0);
	MonsterInit();
}

void CHAssault::Precache()
{
	PRECACHE_MODEL("models/hassault.mdl");

	PRECACHE_SOUND("hassault/hw_shoot1.wav");
	PRECACHE_SOUND("hassault/hw_shoot2.wav");
	PRECACHE_SOUND("hassault/hw_shoot3.wav");

	PRECACHE_SOUND("hassault/hw_spinup.wav");
	PRECACHE_SOUND("hassault/hw_spin.wav");
	PRECACHE_SOUND("hassault/hw_spindown.wav");

	PRECACHE_SOUND("hassault/assault_die1.wav");
	PRECACHE_SOUND("hassault/assault_die2.wav");
	PRECACHE_SOUND("hassault/assault_die3.wav");

	PRECACHE_SOUND("hassault/assault_pain1.wav");
	PRECACHE_SOUND("hassault/assault_pain2.wav");
	PRECACHE_SOUND("hassault/assault_pain3.wav");

	PRECACHE_SOUND("hassault/assault_alert1.wav");
	PRECACHE_SOUND("hassault/assault_alert2.wav");

	CBaseMonster::Precache();
}

void CHAssault::StartTask(Task_t* pTask)
{
	CBaseMonster::StartTask(pTask);
}

void CHAssault::RunTask(Task_t* pTask)
{
	if (m_nextSpinTime < gpGlobals->time)
	{
		FUN_10048c9a();
		m_nextSpinTime = gpGlobals->time + 0.1f;
	}
	CBaseMonster::RunTask(pTask);
}

int CHAssault::ISoundMask()
{
	return bits_SOUND_WORLD |
		   bits_SOUND_COMBAT |
		   bits_SOUND_PLAYER |
		   bits_SOUND_DANGER;
}

int CHAssault::Classify()
{
	return CLASS_HUMAN_MILITARY;
}

void CHAssault::SetYawSpeed()
{
	int ys;

	switch (m_Activity)
	{
	case ACT_IDLE:
		ys = 70;
		break;
	case ACT_WALK:
		ys = 70;
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 110;
		break;
	case ACT_RANGE_ATTACK1:
	default:
		ys = 70;
		break;
	}

	pev->yaw_speed = ys;
}

bool CHAssault::CheckRangeAttack1(float flDot, float flDist)
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
				m_lastAttackCheck = true;
			else
				m_lastAttackCheck = false;
			m_checkAttackTime = gpGlobals->time + 1.5;
		}
		return m_lastAttackCheck;
	}
	return false;
}

int CHAssault::FUN_10048c1e()
{
	return m_chGunstate == 2 || m_chGunstate == 1;
}

bool CHAssault::FUN_10048c66()
{
	return m_chGunstate == 0;
}

void CHAssault::FUN_10048c9a()
{
	m_flStatetime = m_flStatetime + 0.1f;

	switch (m_chGunstate)
	{
	case 0:
		break;

	case 1:
		if (m_flStatetime < 0.15f)
		{
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "hassault/hw_spinup.wav", VOL_NORM, 0.4f);
		}

		if (0.7f <= m_flStatetime)
		{
			FUN_10048e32(2);
		}
		break;

	case 2:
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "hassault/hw_spin.wav", VOL_NORM, 0.4f);
		if (7.5f <= m_flStatetime)
		{
			FUN_10048e32(3);
		}
		break;

	case 3:
		if (m_flStatetime < 0.15f)
		{
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "hassault/hw_spindown.wav", VOL_NORM, 0.4f);
		}
			
		if (0.8f <= m_flStatetime)
		{
			FUN_10048e32(0);
		}
		break;
	}
}

void CHAssault::FUN_10048e32(int a1)
{
	switch (a1)
	{
	case 0:
		m_chGunstate = 0;
		m_flStatetime = 0.0f;
		break;

	case 1:
		if (FUN_10048c66())
		{
			m_chGunstate = 1;
			m_flStatetime = 0.0f;
			pev->framerate = 0.1f;
		}
		break;

	case 2:
		if (m_chGunstate == 1)
		{
			m_chGunstate = 2;
			m_flStatetime = 0.0f;
		}
		break;
	
	case 3:
		if (m_chGunstate == 2)
		{
			m_chGunstate = 3;
			m_flStatetime = 0.0f;
		}
		break;
	}
}

void CHAssault::FUN_10048f34()
{
	if (FUN_10048c1e())
	{
		if (pev->framerate < 2.0f)
			pev->framerate = pev->framerate + 0.1f;

		if (m_chGunstate == 2)
			m_flStatetime = 0.0f;

		Vector vecOrigin, vecAngles;
		GetAttachment(0, vecOrigin, vecAngles);

		Vector vecShootDir = ShootAtEnemy(vecOrigin);
		vecAngles = vecShootDir;

		if (unk.Length() < 0.015625 && unk2.Length() < 0.015625)
		{
			unk2 = Vector(RANDOM_FLOAT(-0.015,0.015), RANDOM_FLOAT(-0.075, 0.075), RANDOM_FLOAT(-0.075, 0.075));
		}

		Vector unk3 = unk2 - unk * 0.125;
		
		Vector unk4 = unk3 + Vector(RANDOM_FLOAT(-0.00125, 0.00125), RANDOM_FLOAT(-0.00125, 0.00125), RANDOM_FLOAT(-0.00125, 0.00125));
		unk2 = unk4;

		if (unk2.Length() > 0.125)
		{
			unk2 = unk2.Normalize() * 0.125;
		}

		Vector unk5 = unk + unk2;
		unk = unk5;

		Vector angDir = UTIL_VecToAngles(vecShootDir);
		SetBlending(0, angDir.x);
		pev->effects = EF_MUZZLEFLASH;

		FireBullets(1, vecOrigin, vecShootDir, VECTOR_CONE_1DEGREES, 1024, BULLET_MONSTER_768MM);
	
		switch (RANDOM_LONG(0, 8))
		{
		case 0:
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "hassault/hw_shoot1.wav", VOL_NORM, ATTN_NORM);
			break;
		case 1:
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "hassault/hw_shoot2.wav", VOL_NORM, ATTN_NORM);
			break;
		case 2:
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "hassault/hw_shoot3.wav", VOL_NORM, ATTN_NORM);
			break;
		}

		CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 384, 0.3);
	}
	else
	{
		FUN_10048e32(1);
	}
}

void CHAssault::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{
	case 1:
	case 4:
	case 7:
		FUN_10048f34();
		break;

	default:
		CBaseMonster::HandleAnimEvent(pEvent);
		break;

	case 10:
		Vector weaponOrigin = pev->origin + Vector(0, 0, 36);
		Vector weaponAngles(0, RANDOM_LONG(0,360), 0);

		CBaseEntity* pWeapon = DropItem("weapon_vulcan", weaponOrigin, weaponAngles);

		pWeapon->pev->velocity = Vector(RANDOM_FLOAT(200,300), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100));
		pWeapon->pev->avelocity = Vector(0,RANDOM_FLOAT(10,30), 20);
		SetBodygroup(1, 1);
		break;
	}
}

void CHAssault::AlertSound()
{
	switch (RANDOM_LONG(0, 1))
	{
	case 0:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "hassault/assault_alert1.wav", VOL_NORM, ATTN_NORM, 0, 100);
		break;
	case 1:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "hassault/assault_alert2.wav", VOL_NORM, ATTN_NORM, 0, 100);
		break;
	}
}

void CHAssault::PainSound()
{
	if (gpGlobals->time < m_painTime)
		return;

	m_painTime = gpGlobals->time + RANDOM_FLOAT(0.5, 0.75);

	switch (RANDOM_LONG(0, 2))
	{
	case 0:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "hassault/assault_pain1.wav", 1, ATTN_NORM, 0, 100);
		break;
	case 1:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "hassault/assault_pain2.wav", 1, ATTN_NORM, 0, 100);
		break;
	case 2:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "hassault/assault_pain3.wav", 1, ATTN_NORM, 0, 100);
		break;
	}
}

void CHAssault::DeathSound()
{
	switch (RANDOM_LONG(0, 2))
	{
	case 0:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "hassault/assault_die1.wav", 1, ATTN_NORM, 0, 100);
		break;
	case 1:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "hassault/assault_die2.wav", 1, ATTN_NORM, 0, 100);
		break;
	case 2:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "hassault/assault_die3.wav", 1, ATTN_NORM, 0, 100);
		break;
	}
}

void CHAssault::TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
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

	CBaseMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
}

void CHAssault::Killed(entvars_t* pevAttacker, int iGib)
{
	m_chGunstate = 3;
	m_flStatetime = 0.0f;
	DeathSound();
	CBaseMonster::Killed(pevAttacker, iGib);
}

void CHAssault::SetActivity(Activity NewActivity)
{
	int iSequence;
	void* pmodel = GET_MODEL_PTR(ENT(pev));

	if (NewActivity == ACT_RANGE_ATTACK1)
		iSequence = LookupSequence("attack");
	else
		iSequence = LookupActivity(NewActivity);

	m_Activity = NewActivity; // Go ahead and set this so it doesn't keep trying when the anim is not present

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

Schedule_t* CHAssault::GetScheduleOfType(int Type)
{
	switch (Type)
	{
	case SCHED_RANGE_ATTACK1:
		return slHAssaultEstablishLineOfFire;

	case SCHED_FAIL:
		if (m_hEnemy == NULL)
			return slHAssaultFail;
		else
			return slHAssaultCombatFail;

	case SCHED_HASSUALT_LINEOFFIRE2:
		return slHAssaultEstablishLineOfFire2;

	default:
		return CBaseMonster::GetScheduleOfType(Type);

	}
}

Schedule_t* CHAssault::GetSchedule()
{
	switch (m_MonsterState)
	{
	case MONSTERSTATE_COMBAT:
		if (HasConditions(bits_COND_NEW_ENEMY))
		{
			AlertSound();
		}

		if (HasConditions(bits_COND_ENEMY_DEAD))
		{
			return CBaseMonster::GetSchedule();
		}

		if (HasConditions(bits_COND_LIGHT_DAMAGE) || HasConditions(bits_COND_HEAVY_DAMAGE) && !HasMemory(bits_MEMORY_FLINCHED))
		{
			return GetScheduleOfType(SCHED_SMALL_FLINCH);
		}

		if (HasConditions(bits_COND_CAN_RANGE_ATTACK1))
		{
			return GetScheduleOfType(SCHED_RANGE_ATTACK1);
		}
		else
		{
			return GetScheduleOfType(SCHED_HASSUALT_LINEOFFIRE2);
		}
	}

	return CBaseMonster::GetSchedule();
}
