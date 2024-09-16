#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "squadmonster.h"
#include "soundent.h"
#include "weapons.h"
#include "plasma.h"
#include "animation.h"

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_RACE_SUPPRESS = LAST_COMMON_SCHEDULE + 1,
	SCHED_RACE_RANGEATTACK2,
	SCHED_RACE_CLOSE_ON_ENEMY,
	SCHED_RACE_TAKE_COVER_AND_RELOAD,
	SCHED_RACE_RELOAD,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum
{
	TASK_RACE_SETUP_HIDE_ATTACK = LAST_COMMON_TASK + 1,
	TASK_RACE_GET_PATH_TO_ENEMY_CORPSE,
	TASK_RACE_UNK1,
	TASK_RACE_UNK2,
};

int iRaceMuzzleFlash;

class CRace : public CSquadMonster
{
	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];
	
	static const char* pAttackHitSounds[];
	static const char* pAttackMissSounds[];
	static const char* pAttackSounds[];
	static const char* pDieSounds[];
	static const char* pPainSounds[];
	static const char* pIdleSounds[];
	static const char* pAlertSounds[];

	void Spawn() override;
	void Precache() override;
	int IRelationship(CBaseEntity* pTarget) override;
	int ISoundMask() override;
	void TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType) override;
	void StopTalking();
	bool ShouldSpeak();
	void PrescheduleThink() override;
	void DeathSound() override;
	void AlertSound() override;
	void PainSound() override;
	void AttackSound();
	int Classify() override;
	void SetYawSpeed() override;
	void HandleAnimEvent(MonsterEvent_t* pEvent) override;

	CUSTOM_SCHEDULES;

	bool CheckMeleeAttack1(float flDot, float flDist) override;
	bool CheckRangeAttack1(float flDot, float flDist) override;
	bool CheckRangeAttack2(float flDot, float flDist) override;
	void CheckAmmo() override;
	void SetActivity(Activity NewActivity) override;
	void MonsterThink() override;
	void StartTask(Task_t* pTask) override;
	Schedule_t* GetSchedule() override;
	Schedule_t* GetScheduleOfType(int Type) override;

	bool m_fCanPlasmaAttack;
	float m_flNextHornetAttackCheck;
	float m_fCanBigplasmaAttack;
	float m_flNextBigplasmaAttackCheck;
	int m_iShotCount;
	float m_flNextPainTime;
	float m_flNextSpeakTime;
	float m_flNextWordTime;
	int m_iLastWord;
};

LINK_ENTITY_TO_CLASS(monster_race, CRace);

TYPEDESCRIPTION CRace::m_SaveData[] =
	{
		DEFINE_FIELD(CRace, m_fCanPlasmaAttack, FIELD_BOOLEAN),
		DEFINE_FIELD(CRace, m_flNextHornetAttackCheck, FIELD_TIME),
		DEFINE_FIELD(CRace, m_fCanBigplasmaAttack, FIELD_BOOLEAN),
		DEFINE_FIELD(CRace, m_flNextBigplasmaAttackCheck, FIELD_TIME),
		DEFINE_FIELD(CRace, m_iShotCount, FIELD_INTEGER),
		DEFINE_FIELD(CRace, m_flNextPainTime, FIELD_TIME),
		DEFINE_FIELD(CRace, m_flNextSpeakTime, FIELD_TIME),
		DEFINE_FIELD(CRace, m_flNextWordTime, FIELD_TIME),
		DEFINE_FIELD(CRace, m_iLastWord, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CRace, CSquadMonster);

const char* CRace::pAttackHitSounds[] =
	{
		"zombie/claw_strike1.wav",
		"zombie/claw_strike2.wav",
		"zombie/claw_strike3.wav",
};

const char* CRace::pAttackMissSounds[] =
	{
		"zombie/claw_miss1.wav",
		"zombie/claw_miss2.wav",
};

const char* CRace::pAttackSounds[] =
	{
		"buttons/latchunlocked1.wav",
		"buttons/latchunlocked2.wav",
		"weapons/plasma1.wav",
		"weapons/plasma2.wav",
		"ambience/particle_suck1.wav",
	};

const char* CRace::pDieSounds[] =
	{
		"race/race_die1.wav",
		"race/race_die2.wav",
};

const char* CRace::pPainSounds[] =
	{
		"race/race_pain1.wav",
		"race/race_pain2.wav",
		"race/race_pain3.wav",
		"race/race_pain4.wav",
		"race/race_pain5.wav",
};

const char* CRace::pIdleSounds[] =
	{
		"race/race_idle1.wav",
		"race/race_idle2.wav",
		"race/race_idle3.wav",
		"race/race_idle4.wav",
};

const char* CRace::pAlertSounds[] =
	{
		"race/race_alert1.wav",
		"race/race_alert2.wav",
};

void CRace::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/race.mdl");
	UTIL_SetSize(pev, Vector(-32,-32,0), Vector(32,32,64));

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_ALIEN;
	pev->effects = 0;
	pev->health = gSkillData.raceHealth;
	m_flFieldOfView = 0.2f;
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = 0;
	m_afCapability |= bits_CAP_SQUAD;

	m_flNextBigplasmaAttackCheck = gpGlobals->time;
	m_flNextHornetAttackCheck = gpGlobals->time;

	SetBodygroup(1,0);

	m_iShotCount = 0;
	m_cAmmoLoaded = 40;

	m_HackedGunPos = Vector(0, 0,55);

	m_flNextSpeakTime = m_flNextWordTime = gpGlobals->time + 10 + RANDOM_LONG(0, 10);


	MonsterInit();
}

void CRace::Precache()
{
	PRECACHE_MODEL("models/race.mdl");

	PRECACHE_SOUND_ARRAY(pAttackHitSounds);
	PRECACHE_SOUND_ARRAY(pAttackMissSounds);
	PRECACHE_SOUND_ARRAY(pIdleSounds);
	PRECACHE_SOUND_ARRAY(pDieSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pAlertSounds);

	gpGlobals->m_SmallPlasmaSprite = PRECACHE_MODEL("sprites/plasmasmall.spr");
	gpGlobals->m_PlasmaSprite = PRECACHE_MODEL("sprites/plasma.spr");

	PRECACHE_MODEL("sprites/xbeam3.spr");

	PRECACHE_SOUND("weapons/mortarhit.wav");
	PRECACHE_SOUND("ambience/steamburst1.wav");
	PRECACHE_SOUND("steam.wav");
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

//=========================================================
// Fail Schedule
//=========================================================
Task_t tlRaceFail[] =
	{
		{TASK_STOP_MOVING, 0},
		{TASK_SET_ACTIVITY, (float)ACT_IDLE},
		{TASK_WAIT, (float)2},
		{TASK_WAIT_PVS, (float)0},
};

Schedule_t slRaceFail[] =
	{
		{tlRaceFail,
			ARRAYSIZE(tlRaceFail),
			bits_COND_CAN_RANGE_ATTACK1 |
			bits_COND_CAN_RANGE_ATTACK2 |
				bits_COND_CAN_MELEE_ATTACK1,
			0,
			"Race Fail"},
};

//=========================================================
// Combat Fail Schedule
//=========================================================
Task_t tlRaceCombatFail[] =
	{
		{TASK_STOP_MOVING, 0},
		{TASK_SET_ACTIVITY, (float)ACT_IDLE},
		{TASK_WAIT_FACE_ENEMY, (float)2},
		{TASK_WAIT_PVS, (float)0},
};

Schedule_t slRaceCombatFail[] =
	{
		{tlRaceCombatFail,
			ARRAYSIZE(tlRaceCombatFail),
			bits_COND_CAN_RANGE_ATTACK1 |
				bits_COND_CAN_RANGE_ATTACK2,
			0,
			"Race Combat Fail"},
};

//=========================================================
// Standoff schedule. Used in combat when a monster is
// hiding in cover or the enemy has moved out of sight.
// Should we look around in this schedule?
//=========================================================
Task_t tlRaceStandoff[] =
	{
		{TASK_GET_PATH_TO_ENEMY, (float)0},
		{TASK_RUN_PATH, (float)0},
		{TASK_WAIT_FOR_MOVEMENT, (float)0},
};

Schedule_t slRaceStandoff[] =
	{
		{tlRaceStandoff,
			ARRAYSIZE(tlRaceStandoff),
			bits_COND_CAN_RANGE_ATTACK1 |
				bits_COND_CAN_MELEE_ATTACK1 |
				bits_COND_SEE_ENEMY |
				bits_COND_NEW_ENEMY |
				bits_COND_HEAR_SOUND,

			bits_SOUND_DANGER,
			"Race Standoff"}};

//=========================================================
// Suppress
//=========================================================
Task_t tlRaceSuppress[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_RANGE_ATTACK2, (float)0},
};

Schedule_t slRaceSuppress[] =
	{
		{
			tlRaceSuppress,
			ARRAYSIZE(tlRaceSuppress),
			bits_COND_ENEMY_DEAD |
				bits_COND_LIGHT_DAMAGE |
				bits_COND_HEAVY_DAMAGE |
				bits_COND_HEAR_SOUND,
			0,
			"Race Suppress",
		},
};

//=========================================================
// primary range attacks
//=========================================================
Task_t tlRaceRangeAttack1[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_RACE_UNK2, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_RACE_UNK2, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
};

Schedule_t slRaceRangeAttack1[] =
	{
		{tlRaceRangeAttack1,
			ARRAYSIZE(tlRaceRangeAttack1),
			bits_COND_NEW_ENEMY |
				bits_COND_ENEMY_DEAD |
				bits_COND_HEAVY_DAMAGE |
				bits_COND_ENEMY_OCCLUDED |
				bits_COND_HEAR_SOUND |
				bits_COND_SPECIAL1 |
				bits_COND_NO_AMMO_LOADED |
				bits_COND_CAN_MELEE_ATTACK1 |
				bits_COND_LIGHT_DAMAGE,

			0,
			"Race Range Attack1"},
};

Task_t tlRaceRangeAttack2[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_RACE_UNK1, (float)0},
		{TASK_RANGE_ATTACK2, (float)0},
};

Schedule_t slRaceRangeAttack2[] =
	{
		{tlRaceRangeAttack2,
			ARRAYSIZE(tlRaceRangeAttack2),
			bits_COND_NEW_ENEMY |
				bits_COND_ENEMY_DEAD |
				bits_COND_ENEMY_OCCLUDED |
				bits_COND_HEAR_SOUND |
				bits_COND_NO_AMMO_LOADED |
				bits_COND_HEAVY_DAMAGE,

			bits_SOUND_DANGER,
			"Race Range Attack2"},
};

Task_t tlRaceCloseOnEnemy[] =
	{
		{TASK_SET_FAIL_SCHEDULE, (float)SCHED_CHASE_ENEMY_FAILED},
		{TASK_GET_PATH_TO_ENEMY, (float)0},
		{TASK_RUN_PATH, (float)0},
		{TASK_WAIT_FOR_MOVEMENT, (float)0},
};

Schedule_t slRaceCloseOnEnemy[] =
	{
		{tlRaceCloseOnEnemy,
			ARRAYSIZE(tlRaceCloseOnEnemy),
			bits_COND_NEW_ENEMY |
				bits_COND_CAN_RANGE_ATTACK1 |
				bits_COND_CAN_MELEE_ATTACK1 |
				bits_COND_CAN_RANGE_ATTACK2 |
				bits_COND_CAN_MELEE_ATTACK2 |
				bits_COND_TASK_FAILED |
				bits_COND_HEAR_SOUND,

			bits_SOUND_DANGER,
			"Race Close On Enemy"},
};

Task_t tlRaceTakeCoverFromEnemy[] =
	{
		{TASK_SET_FAIL_SCHEDULE, (float)SCHED_RACE_RANGEATTACK2},
		{TASK_FIND_COVER_FROM_ENEMY, (float)0},
		{TASK_RUN_PATH, (float)0},
		{TASK_WAIT_FOR_MOVEMENT, (float)0},
		{TASK_FACE_ENEMY, (float)0},
};

Schedule_t slRaceTakeCoverFromEnemy[] =
	{
		{tlRaceTakeCoverFromEnemy,
			ARRAYSIZE(tlRaceTakeCoverFromEnemy),
			bits_COND_NEW_ENEMY,

			0,
			"RaceTakeCoverFromEnemy"},
};

Task_t tlRaceTakeCoverFromBestSound[] =
	{
		{TASK_SET_FAIL_SCHEDULE, (float)SCHED_TAKE_COVER_FROM_ENEMY},
		{TASK_STOP_MOVING, (float)0},
		{TASK_FIND_COVER_FROM_BEST_SOUND, (float)0},
		{TASK_RUN_PATH, (float)0},
		{TASK_WAIT, (float)0.3},
		{TASK_FACE_ENEMY, (float)179.0},
};

Schedule_t slRaceTakeCoverFromBestSound[] =
	{
		{tlRaceTakeCoverFromBestSound,
			ARRAYSIZE(tlRaceTakeCoverFromBestSound),
			0,
			0,
			"RaceTakeCoverFromBestSound"},
};

Task_t tlRaceTakeCoverAndReload[] =
	{
		{TASK_SET_FAIL_SCHEDULE, (float)SCHED_RACE_RELOAD},
		{TASK_STOP_MOVING, (float)0},
		{TASK_FIND_COVER_FROM_ENEMY, (float)0},
		{TASK_RUN_PATH, (float)0},
		{TASK_WAIT_FOR_MOVEMENT, (float)0.3},
		{TASK_FACE_ENEMY, (float)179.0},
		{TASK_PLAY_SEQUENCE, (float)ACT_RELOAD},
};

Schedule_t slRaceTakeCoverAndReload[] =
	{
		{tlRaceTakeCoverAndReload,
			ARRAYSIZE(tlRaceTakeCoverAndReload),
			bits_COND_TASK_FAILED,
			0,
			"RaceTakeCoverAndReload"},
};

Task_t tlRaceReload[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_WAIT_FOR_MOVEMENT, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_PLAY_SEQUENCE, (float)ACT_RELOAD},
};

Schedule_t slRaceReload[] =
	{
		{tlRaceReload,
			ARRAYSIZE(tlRaceReload),
			0,
			0,
			"RaceReload"},
};

Task_t tlRaceVictoryDance[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_WAIT, (float)0.2},
		{TASK_RACE_GET_PATH_TO_ENEMY_CORPSE, (float)0},
		{TASK_WALK_PATH, (float)0},
		{TASK_WAIT_FOR_MOVEMENT, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_PLAY_SEQUENCE, (float)ACT_CROUCH},
		{TASK_PLAY_SEQUENCE, (float)ACT_VICTORY_DANCE},
		{TASK_PLAY_SEQUENCE, (float)ACT_VICTORY_DANCE},
		{TASK_PLAY_SEQUENCE, (float)ACT_STAND},
		{TASK_PLAY_SEQUENCE, (float)ACT_CROUCH},
		{TASK_PLAY_SEQUENCE, (float)ACT_VICTORY_DANCE},
		{TASK_PLAY_SEQUENCE, (float)ACT_VICTORY_DANCE},
		{TASK_PLAY_SEQUENCE, (float)ACT_VICTORY_DANCE},
		{TASK_PLAY_SEQUENCE, (float)ACT_VICTORY_DANCE},
		{TASK_PLAY_SEQUENCE, (float)ACT_VICTORY_DANCE},
		{TASK_PLAY_SEQUENCE, (float)ACT_STAND},
};

Schedule_t slRaceVictoryDance[] =
	{
		{tlRaceVictoryDance,
			ARRAYSIZE(tlRaceVictoryDance),
			bits_COND_NEW_ENEMY |
				bits_COND_LIGHT_DAMAGE |
				bits_COND_HEAVY_DAMAGE,
			0,
			"RaceVictoryDance"},
};

DEFINE_CUSTOM_SCHEDULES(CRace){
	slRaceFail,
	slRaceCombatFail,
	slRaceStandoff,
	slRaceSuppress,
	slRaceRangeAttack1,
	slRaceRangeAttack2,
	slRaceCloseOnEnemy,
	slRaceTakeCoverFromEnemy,
	slRaceTakeCoverFromBestSound,
	slRaceVictoryDance,
	slRaceTakeCoverAndReload,
	slRaceReload,
};

IMPLEMENT_CUSTOM_SCHEDULES(CRace, CSquadMonster);

int CRace::IRelationship(CBaseEntity* pTarget)
{
	if (FClassnameIs(pTarget->pev, "monster_alien_grunt"))
	{
		return R_NM;
	}

	return CSquadMonster::IRelationship(pTarget);
}

int CRace::ISoundMask()
{
	return bits_SOUND_WORLD |
		   bits_SOUND_COMBAT |
		   bits_SOUND_PLAYER |
		   bits_SOUND_DANGER;
}

void CRace::TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	if (ptr->iHitgroup == 10 && (bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_CLUB)) != 0)
	{
		// hit armor
		if (pev->dmgtime != gpGlobals->time || (RANDOM_LONG(0, 10) < 1))
		{
			UTIL_Ricochet(ptr->vecEndPos, RANDOM_FLOAT(1, 2));
			pev->dmgtime = gpGlobals->time;
		}

		if (RANDOM_LONG(0, 1) == 0)
		{
			Vector vecTracerDir = vecDir;

			vecTracerDir.x += RANDOM_FLOAT(-0.3, 0.3);
			vecTracerDir.y += RANDOM_FLOAT(-0.3, 0.3);
			vecTracerDir.z += RANDOM_FLOAT(-0.3, 0.3);

			vecTracerDir = vecTracerDir * -512;

			MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, ptr->vecEndPos);
			WRITE_BYTE(TE_TRACER);
			WRITE_COORD(ptr->vecEndPos.x);
			WRITE_COORD(ptr->vecEndPos.y);
			WRITE_COORD(ptr->vecEndPos.z);

			WRITE_COORD(vecTracerDir.x);
			WRITE_COORD(vecTracerDir.y);
			WRITE_COORD(vecTracerDir.z);
			MESSAGE_END();
		}

		flDamage -= 20;
		if (flDamage <= 0)
			flDamage = 0.1; // don't hurt the monster much, but allow bits_COND_LIGHT_DAMAGE to be generated
	}
	else
	{
		SpawnBlood(ptr->vecEndPos, BloodColor(), flDamage); // a little surface blood.
		TraceBleed(flDamage, vecDir, ptr, bitsDamageType);
	}

	AddMultiDamage(pevAttacker, this, flDamage, bitsDamageType);
}

void CRace::StopTalking()
{
	m_flNextWordTime = m_flNextSpeakTime = gpGlobals->time + 10 + RANDOM_LONG(0, 10);
}

bool CRace::ShouldSpeak()
{
	if (m_flNextSpeakTime > gpGlobals->time)
	{
		// my time to talk is still in the future.
		return false;
	}

	if ((pev->spawnflags & SF_MONSTER_GAG) != 0)
	{
		if (m_MonsterState != MONSTERSTATE_COMBAT)
		{
			// if gagged, don't talk outside of combat.
			// if not going to talk because of this, put the talk time
			// into the future a bit, so we don't talk immediately after
			// going into combat
			m_flNextSpeakTime = gpGlobals->time + 3;
			return false;
		}
	}

	return true;
}

void CRace::PrescheduleThink()
{
	if (ShouldSpeak())
	{
		if (m_flNextWordTime < gpGlobals->time)
		{
			int num = -1;

			do
			{
				num = RANDOM_LONG(0, ARRAYSIZE(pIdleSounds) - 1);
			} while (num == m_iLastWord);

			m_iLastWord = num;

			// play a new sound
			EMIT_SOUND(ENT(pev), CHAN_VOICE, pIdleSounds[num], 1.0, ATTN_NORM);

			// is this word our last?
			if (RANDOM_LONG(1, 10) <= 1)
			{
				// stop talking.
				StopTalking();
			}
			else
			{
				m_flNextWordTime = gpGlobals->time + RANDOM_FLOAT(0.5, 1);
			}
		}
	}
}

void CRace::DeathSound()
{
	StopTalking();

	EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pDieSounds), 1.0, ATTN_NORM);
}

void CRace::AlertSound()
{
	StopTalking();

	EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pAlertSounds), 1.0, ATTN_NORM);
}

void CRace::PainSound()
{
	if (m_flNextPainTime > gpGlobals->time)
	{
		return;
	}

	m_flNextPainTime = gpGlobals->time + 0.6;

	StopTalking();

	EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pPainSounds), 1.0, ATTN_NORM);
}

void CRace::AttackSound()
{
	StopTalking();

	EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pAttackSounds), 1.0, ATTN_NORM);
}

int CRace::Classify()
{
	return CLASS_ALIEN_RACE;
}

void CRace::SetYawSpeed()
{
	int ys;

	switch (m_Activity)
	{
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 110;
		break;
	default:
		ys = 70;
	}

	pev->yaw_speed = ys;
}

void CRace::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	Vector vecOrigin, vecAngles;
	Vector vecUnk1, vecUnk2;

	switch (pEvent->event)
	{
	case 2:
		m_cAmmoLoaded = 40;
		ClearConditions(bits_COND_NO_AMMO_LOADED);
		break;
	case 3:
	{
		CBaseEntity* pHurt = CheckTraceHullAttack(100.f, gSkillData.agruntDmgPunch, DMG_CLUB);

		if (pHurt)
		{
			pHurt->pev->punchangle.y = -25;
			pHurt->pev->punchangle.x = 8;

			// OK to use gpGlobals without calling MakeVectors, cause CheckTraceHullAttack called it above.
			if (pHurt->IsPlayer())
			{
				// this is a player. Knock him around.
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * 250;
			}

			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pAttackHitSounds), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));

			Vector vecArmPos, vecArmAng;
			GetAttachment(0, vecArmPos, vecArmAng);
			SpawnBlood(vecArmPos, pHurt->BloodColor(), 25); // a little surface blood.
		}
		else
		{
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pAttackMissSounds), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
		}
	}
	break;

	case 4:
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/plasma1.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(0, 15) + 94);
		
		if (11 < m_iShotCount)
		{
			m_iShotCount = 0;
		}
		break;

	case 5:
	case 6:
	{
		int skilllevel = g_iSkillLevel * 300 + 800;
		GetAttachment(0, vecOrigin, vecAngles);

		vecOrigin = pev->origin + Vector(0, 0, 60);

		Vector vecForward;
		float unkLength = 0.0f;

		if (m_hEnemy != NULL)
		{
			if (g_iSkillLevel == 1)
			{
				unkLength = vecUnk1.Length() / skilllevel;

				Vector target = m_hEnemy->BodyTarget(vecOrigin);

				Vector forward = target - m_hEnemy->pev->origin + m_vecEnemyLKP - vecOrigin + (RANDOM_FLOAT(0.25, 0.75) * m_hEnemy->pev->velocity * unkLength);

				forward = gpGlobals->v_forward;
			}
			if (g_iSkillLevel == 2)
			{
				unkLength = vecUnk1.Length() / skilllevel;

				Vector target = m_hEnemy->BodyTarget(vecOrigin);

				Vector forward = target - m_hEnemy->pev->origin + m_vecEnemyLKP - vecOrigin + (RANDOM_FLOAT(0.125, 0.9375) * m_hEnemy->pev->velocity * unkLength);

				forward = gpGlobals->v_forward;
			}
			if (g_iSkillLevel == 3)
			{
				unkLength = vecUnk1.Length() / skilllevel;

				Vector target = m_hEnemy->BodyTarget(vecOrigin);

				Vector forward = target - m_hEnemy->pev->origin + m_vecEnemyLKP - vecOrigin + (RANDOM_FLOAT(-0.125, 0.125) * m_hEnemy->pev->velocity * unkLength);

				forward = gpGlobals->v_forward;
			}
		}
		else
		{
			vecForward = gpGlobals->v_forward;
		}

		vecUnk1.x = m_vecEnemyLKP.x - pev->origin.x;
		vecUnk1.y = m_vecEnemyLKP.y - pev->origin.y;
		vecUnk1.z = 0.0f;

		Vector unkNormalize = vecUnk1.Normalize();

		if (m_iShotCount / 6 % 2 == 0)
		{
			unkNormalize * (2 - m_iShotCount % 6);
		}
		else
		{
			unkNormalize*(m_iShotCount % 6 - 2);
		}

		++m_iShotCount;

		Vector unk = Vector(0, 0, -8) + unkNormalize * 24.0f;
		vecForward = unk;

		Vector angDir = UTIL_VecToAngles(vecForward);
		vecUnk2 = angDir;

		vecForward = vecForward.Normalize();

		pev->effects = EF_MUZZLEFLASH;

		if (180.0f < vecUnk2.x)
		{
			vecUnk2.x = vecUnk2.x - 360.0f;
		}

		SetBlending(0, vecUnk2.x);

		MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, vecOrigin);
		WRITE_BYTE(TE_SPRITE);
		WRITE_COORD(vecOrigin.x);
		WRITE_COORD(vecOrigin.y);
		WRITE_COORD(vecOrigin.z);
		WRITE_SHORT(iRaceMuzzleFlash);
		WRITE_BYTE(6);
		WRITE_BYTE(128);
		MESSAGE_END();

		CPlasmaBall::Shoot(pev, vecOrigin, vecForward, unkLength, gSkillData.monDmgPlasma);
		--m_cAmmoLoaded;
	}
	break;
	
	case 8:
	{
		vecUnk1 = m_vecEnemyLKP - pev->origin - Vector(0, 0, 16);
		vecUnk1 = vecUnk1.Normalize();

		pev->effects = EF_MUZZLEFLASH;
		GetAttachment(0, vecOrigin, vecAngles);

		float unkLength = vecUnk1.Length() / RANDOM_LONG(1400, 1600);

		Vector target = m_hEnemy->BodyTarget(vecOrigin);

		Vector forward = target - vecOrigin + pev->velocity * unkLength;

		Vector vecForward2;
		vecForward2.z = vecForward2.z - 32.0f;

		Vector angDir = UTIL_VecToAngles(vecForward2);
		vecUnk2 = angDir;

		if (180.0f < vecUnk2.x)
		{
			vecUnk2.x = vecUnk2.x - 360.0f;
		}

		SetBlending(0, vecUnk2.x);

		vecOrigin = vecOrigin + vecUnk1 * 32.0f;

		MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, vecOrigin);
		WRITE_BYTE(TE_SPRITE);
		WRITE_COORD(vecOrigin.x);
		WRITE_COORD(vecOrigin.y);
		WRITE_COORD(vecOrigin.z);
		WRITE_SHORT(iRaceMuzzleFlash);
		WRITE_BYTE(6);
		WRITE_BYTE(128);
		MESSAGE_END();

		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/plasma2.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(0, 15) + 64);

		CPlasmaBallBig::Shoot(pev, vecOrigin, vecForward2, unkLength, gSkillData.monDmgPlasmaBig);

		m_cAmmoLoaded -= 10;
	}
	break;

	case 11:
	{
		Vector deadOrigin, deadAngles;
		GetAttachment(0, deadOrigin, deadAngles);

		CBaseEntity* pWeapon = DropItem("weapon_plasma", deadOrigin, deadAngles);

		pWeapon->pev->velocity = Vector(RANDOM_FLOAT(200, 300), RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(-100, 100));
		pWeapon->pev->avelocity = Vector(0, RANDOM_FLOAT(200, 400), 0);
		SetBodygroup(1, 1);
	}
	break;

	default:
		CBaseMonster::HandleAnimEvent(pEvent);
		break;
	}
}

bool CRace::CheckMeleeAttack1(float flDot, float flDist)
{
	if (HasConditions(bits_COND_SEE_ENEMY) && flDist <= 100.0f && flDot >= 0.6 && m_hEnemy != NULL)
	{
		return true;
	}
	return false;
}

bool CRace::CheckRangeAttack1(float flDot, float flDist)
{
	if (gpGlobals->time < m_flNextHornetAttackCheck)
	{
		return m_fCanPlasmaAttack;
	}

	if (HasConditions(bits_COND_SEE_ENEMY) && flDist >= 100.0f && flDist <= 1024 && flDot >= 0.5 && NoFriendlyFire())
	{
		TraceResult tr;
		Vector vecArmPos, vecArmDir;

		// verify that a shot fired from the gun will hit the enemy before the world.
		// !!!LATER - we may wish to do something different for projectile weapons as opposed to instant-hit
		UTIL_MakeVectors(pev->angles);
		GetAttachment(0, vecArmPos, vecArmDir);
		//		UTIL_TraceLine( vecArmPos, vecArmPos + gpGlobals->v_forward * 256, ignore_monsters, ENT(pev), &tr);
		UTIL_TraceLine(vecArmPos, m_hEnemy->BodyTarget(vecArmPos), dont_ignore_monsters, ENT(pev), &tr);

		if (tr.flFraction == 1.0 || tr.pHit == m_hEnemy->edict())
		{
			m_flNextHornetAttackCheck = gpGlobals->time + RANDOM_FLOAT(2, 5);
			m_fCanPlasmaAttack = true;
			return m_fCanPlasmaAttack;
		}
	}

	m_flNextHornetAttackCheck = gpGlobals->time + 0.2; // don't check for half second if this check wasn't successful
	m_fCanPlasmaAttack = false;
	return m_fCanPlasmaAttack;
}

bool CRace::CheckRangeAttack2(float flDot, float flDist)
{
	if (gpGlobals->time < m_flNextBigplasmaAttackCheck)
	{
		return m_fCanBigplasmaAttack;
	}

	if (HasConditions(bits_COND_SEE_ENEMY) && flDist >= 100.0f && flDist <= 1024 && flDot >= 0.5 && NoFriendlyFire())
	{
		TraceResult tr;
		Vector vecArmPos, vecArmDir;

		// verify that a shot fired from the gun will hit the enemy before the world.
		// !!!LATER - we may wish to do something different for projectile weapons as opposed to instant-hit
		UTIL_MakeVectors(pev->angles);
		GetAttachment(0, vecArmPos, vecArmDir);
		//		UTIL_TraceLine( vecArmPos, vecArmPos + gpGlobals->v_forward * 256, ignore_monsters, ENT(pev), &tr);
		UTIL_TraceLine(vecArmPos, m_hEnemy->BodyTarget(vecArmPos), dont_ignore_monsters, ENT(pev), &tr);

		if (tr.flFraction == 1.0 || tr.pHit == m_hEnemy->edict())
		{
			m_flNextBigplasmaAttackCheck = gpGlobals->time + RANDOM_FLOAT(2, 5);
			m_fCanBigplasmaAttack = true;
			return m_fCanBigplasmaAttack;
		}
	}

	m_flNextBigplasmaAttackCheck = gpGlobals->time + 0.2; // don't check for half second if this check wasn't successful
	m_fCanBigplasmaAttack = false;
	return m_fCanBigplasmaAttack;
}

void CRace::CheckAmmo()
{
	if (m_cAmmoLoaded <= 0)
	{
		SetConditions(bits_COND_NO_AMMO_LOADED);
	}
}

void CRace::SetActivity(Activity NewActivity)
{
	int iSequence = ACTIVITY_NOT_AVAILABLE;
	void* pmodel = GET_MODEL_PTR(ENT(pev));

	switch (NewActivity)
	{
	case ACT_RUN:
		if (HasConditions(bits_COND_SEE_ENEMY) && m_fCanPlasmaAttack)
			iSequence = LookupSequence("plasma_run");
		else
			iSequence = LookupSequence("run");
		break;

	default:
		iSequence = LookupActivity(NewActivity);
		break;
	}

	m_Activity = NewActivity;

	if (iSequence > ACTIVITY_NOT_AVAILABLE)
	{
		if (pev->sequence != iSequence || !m_fSequenceLoops)
		{
			pev->frame = 0;
		}

		pev->sequence = iSequence;
		ResetSequenceInfo();
		SetYawSpeed();
	}
	else
	{
		ALERT(at_console, "%s has no sequence for act:%d\n", STRING(pev->classname), NewActivity);
		pev->sequence = 0;
	}
}

void CRace::MonsterThink()
{
	pev->nextthink = gpGlobals->time + 0.1;

	RunAI();

	float flInterval = StudioFrameAdvance();
	
	if (m_Activity == ACT_RUN && m_fSequenceFinished)
	{
		SetActivity(ACT_RUN);
	}

	if (m_MonsterState != MONSTERSTATE_SCRIPT && m_MonsterState != MONSTERSTATE_DEAD && m_Activity == ACT_IDLE && m_fSequenceFinished)
	{
		int iSequence;

		if (m_fSequenceLoops)
		{
			iSequence = LookupActivity(m_Activity);
		}
		else
		{
			iSequence = LookupActivityHeaviest(m_Activity);
		}
		if (iSequence != ACTIVITY_NOT_AVAILABLE)
		{
			pev->sequence = iSequence;
			ResetSequenceInfo();
		}
	}

	DispatchAnimEvents(flInterval);

	if (!MovementIsComplete())
	{
		Move(flInterval);
	}
}

void CRace::StartTask(Task_t* pTask)
{
	switch (pTask->iTask)
	{
	case TASK_RACE_GET_PATH_TO_ENEMY_CORPSE:
	{
		UTIL_MakeVectors(pev->angles);
		if (BuildRoute(m_vecEnemyLKP - gpGlobals->v_forward * 50, bits_MF_TO_LOCATION, NULL))
		{
			TaskComplete();
		}
		else
		{
			ALERT(at_aiconsole, "RaceGetPathToEnemyCorpse failed!!\n");
			TaskFail();
		}
	}
	break;

	case TASK_RACE_UNK1:
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "ambience/particle_suck1.wav", VOL_NORM, 0.4f, 0, RANDOM_LONG(0,15) + 94);
		break;

	case TASK_RACE_UNK2:
		if (!NoFriendlyFire())
			SetConditions(bits_COND_SPECIAL1);

		TaskComplete();
		break;

	default:
		CBaseMonster::StartTask(pTask);
		break;
	}
}

Schedule_t* CRace::GetSchedule()
{
	if (HasConditions(bits_COND_HEAR_SOUND))
	{
		CSound* pSound;
		pSound = PBestSound();

		ASSERT(pSound != NULL);
		if (pSound)
		{
			if ((pSound->m_iType & bits_SOUND_DANGER) != 0)
			{
				return GetScheduleOfType(SCHED_TAKE_COVER_FROM_BEST_SOUND);
			}
		}
	}

	switch (m_MonsterState)
	{
	case MONSTERSTATE_COMBAT:
	{
		// dead enemy
		if (HasConditions(bits_COND_ENEMY_DEAD))
		{
			// call base class, all code to handle dead enemies is centralized there.
			return GetSchedule();
		}

		if (HasConditions(bits_COND_NEW_ENEMY))
		{
			return GetScheduleOfType(SCHED_WAKE_ANGRY);
		}

		// zap player!
		if (HasConditions(bits_COND_CAN_MELEE_ATTACK1))
		{
			AttackSound(); // this is a total hack. Should be parto f the schedule
			return GetScheduleOfType(SCHED_MELEE_ATTACK1);
		}

		if (HasConditions(bits_COND_NO_AMMO_LOADED))
			return GetScheduleOfType(SCHED_RACE_TAKE_COVER_AND_RELOAD);

		if (HasConditions(bits_COND_HEAVY_DAMAGE))
		{
			if (RANDOM_LONG(1, 12) > 6 && InSquad() && !IsLeader())
				return GetScheduleOfType(SCHED_TAKE_COVER_FROM_ENEMY);

			if (RANDOM_LONG(1, 12) <= 2 || !HasConditions(bits_COND_CAN_RANGE_ATTACK2))
				return GetScheduleOfType(SCHED_SMALL_FLINCH);
			return GetScheduleOfType(SCHED_RANGE_ATTACK2);
		}

		if (!HasConditions(bits_COND_LIGHT_DAMAGE))
		{
			if (RANDOM_LONG(1, 5) < 5)
			{
				if (HasConditions(bits_COND_CAN_RANGE_ATTACK1) && !HasConditions(bits_COND_CAN_RANGE_ATTACK2))
					return GetScheduleOfType(SCHED_RACE_CLOSE_ON_ENEMY);

				if (HasConditions(bits_COND_CAN_RANGE_ATTACK1))
					return GetScheduleOfType(SCHED_RANGE_ATTACK1); 

				return GetScheduleOfType(SCHED_CHASE_ENEMY);
			
			}
		}

		if (RANDOM_LONG(1, 12) > 10)
			return GetScheduleOfType(SCHED_TAKE_COVER_FROM_ENEMY);

		if (RANDOM_LONG(1, 12) <= 8 || !HasConditions(bits_COND_CAN_RANGE_ATTACK2))
		{
			if (RANDOM_LONG(1, 5) < 5)
			{
				if (HasConditions(bits_COND_CAN_RANGE_ATTACK1) && !HasConditions(bits_COND_CAN_RANGE_ATTACK2))
					return GetScheduleOfType(SCHED_RACE_CLOSE_ON_ENEMY);

				if (HasConditions(bits_COND_CAN_RANGE_ATTACK1))
					return GetScheduleOfType(SCHED_RANGE_ATTACK1);

				return GetScheduleOfType(SCHED_CHASE_ENEMY);
			}
			if (!HasConditions(bits_COND_CAN_RANGE_ATTACK2))
				return GetScheduleOfType(SCHED_CHASE_ENEMY);
		}
		return GetScheduleOfType(SCHED_RANGE_ATTACK2);
	}
	}

	return CBaseMonster::GetSchedule();
}

Schedule_t* CRace::GetScheduleOfType(int Type)
{
	switch (Type)
	{
	case SCHED_VICTORY_DANCE:
	{
		return slRaceVictoryDance;
	}
	case SCHED_TAKE_COVER_FROM_ENEMY:
	{
		return slRaceTakeCoverFromEnemy;
	}
	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
	{
		return slRaceTakeCoverFromBestSound;
	}
	case SCHED_RANGE_ATTACK1:
	{
		return slRaceRangeAttack1;
	}
	case SCHED_RANGE_ATTACK2:
	{
		return slRaceRangeAttack2;
	}
	case SCHED_STANDOFF:
	{
		return slRaceStandoff;
	}
	case SCHED_FAIL:
	{
		if (m_hEnemy != NULL)
		{
			return slRaceCombatFail;
		}

		return slRaceFail;
	}
	case SCHED_RACE_SUPPRESS:
	{
		return slRaceSuppress;
	}
	case SCHED_RACE_RANGEATTACK2:
	{
		if (HasConditions(bits_COND_CAN_RANGE_ATTACK2))
		{
			return GetScheduleOfType(SCHED_FAIL);
		}
		else
		{
			return GetScheduleOfType(SCHED_RANGE_ATTACK2);
		}
	}
	case SCHED_RACE_CLOSE_ON_ENEMY:
	{
		return slRaceCloseOnEnemy;
	}
	case SCHED_RACE_TAKE_COVER_AND_RELOAD:
	{
		return slRaceTakeCoverAndReload;
	}
	case SCHED_RACE_RELOAD:
	{
		return slRaceReload;
	}
	default:
	{
		return CSquadMonster::GetScheduleOfType(Type);
	}
	}
}
