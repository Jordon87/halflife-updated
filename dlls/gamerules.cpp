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
//=========================================================
// GameRules.cpp
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "teamplay_gamerules.h"
#include "skill.h"
#include "game.h"
#include "UserMessages.h"

extern edict_t* EntSelectSpawnPoint(CBaseEntity* pPlayer);

CBasePlayerItem* CGameRules::FindNextBestWeapon(CBasePlayer* pPlayer, CBasePlayerItem* pCurrentWeapon)
{
	if (pCurrentWeapon != nullptr && !pCurrentWeapon->CanHolster())
	{
		// can't put this gun away right now, so can't switch.
		return nullptr;
	}

	const int currentWeight = pCurrentWeapon != nullptr ? pCurrentWeapon->iWeight() : -1;

	CBasePlayerItem* pBest = nullptr; // this will be used in the event that we don't find a weapon in the same category.

	int iBestWeight = -1; // no weapon lower than -1 can be autoswitched to

	for (int i = 0; i < MAX_ITEM_TYPES; i++)
	{
		for (auto pCheck = pPlayer->m_rgpPlayerItems[i]; pCheck; pCheck = pCheck->m_pNext)
		{
			// don't reselect the weapon we're trying to get rid of
			if (pCheck == pCurrentWeapon)
			{
				continue;
			}

			if ((pCheck->iFlags() & ITEM_FLAG_NOAUTOSWITCHTO) != 0)
			{
				continue;
			}

			if (pCheck->iWeight() > -1 && pCheck->iWeight() == currentWeight)
			{
				// this weapon is from the same category.
				if (pCheck->CanDeploy())
				{
					if (pPlayer->SwitchWeapon(pCheck))
					{
						return pCheck;
					}
				}
			}
			else if (pCheck->iWeight() > iBestWeight)
			{
				//ALERT ( at_console, "Considering %s\n", STRING( pCheck->pev->classname ) );
				// we keep updating the 'best' weapon just in case we can't find a weapon of the same weight
				// that the player was using. This will end up leaving the player with his heaviest-weighted
				// weapon.
				if (pCheck->CanDeploy())
				{
					// if this weapon is useable, flag it as the best
					iBestWeight = pCheck->iWeight();
					pBest = pCheck;
				}
			}
		}
	}

	// if we make it here, we've checked all the weapons and found no useable
	// weapon in the same catagory as the current weapon.

	// if pBest is nullptr, we didn't find ANYTHING. Shouldn't be possible- should always
	// at least get the crowbar, but ya never know.

	return pBest;
}

bool CGameRules::GetNextBestWeapon(CBasePlayer* pPlayer, CBasePlayerItem* pCurrentWeapon, bool alwaysSearch)
{
	if (auto pBest = FindNextBestWeapon(pPlayer, pCurrentWeapon); pBest != nullptr)
	{
		pPlayer->SwitchWeapon(pBest);
		return true;
	}

	return false;
}

//=========================================================
//=========================================================
bool CGameRules::CanHaveAmmo(CBasePlayer* pPlayer, const char* pszAmmoName, int iMaxCarry)
{
	int iAmmoIndex;

	if (pszAmmoName)
	{
		iAmmoIndex = pPlayer->GetAmmoIndex(pszAmmoName);

		if (iAmmoIndex > -1)
		{
			if (pPlayer->AmmoInventory(iAmmoIndex) < iMaxCarry)
			{
				// player has room for more of this type of ammo
				return true;
			}
		}
	}

	return false;
}

//=========================================================
//=========================================================
edict_t* CGameRules::GetPlayerSpawnSpot(CBasePlayer* pPlayer)
{
	edict_t* pentSpawnSpot = EntSelectSpawnPoint(pPlayer);

	pPlayer->pev->origin = VARS(pentSpawnSpot)->origin + Vector(0, 0, 1);
	pPlayer->pev->v_angle = g_vecZero;
	pPlayer->pev->velocity = g_vecZero;
	pPlayer->pev->angles = VARS(pentSpawnSpot)->angles;
	pPlayer->pev->punchangle = g_vecZero;
	pPlayer->pev->fixangle = 1;

	return pentSpawnSpot;
}

//=========================================================
//=========================================================
bool CGameRules::CanHavePlayerItem(CBasePlayer* pPlayer, CBasePlayerItem* pWeapon)
{
	// only living players can have items
	if (pPlayer->pev->deadflag != DEAD_NO)
		return false;

	if (pWeapon->pszAmmo1())
	{
		if (!CanHaveAmmo(pPlayer, pWeapon->pszAmmo1(), pWeapon->iMaxAmmo1()))
		{
			// we can't carry anymore ammo for this gun. We can only
			// have the gun if we aren't already carrying one of this type
			if (pPlayer->HasPlayerItem(pWeapon))
			{
				return false;
			}
		}
	}
	else
	{
		// weapon doesn't use ammo, don't take another if you already have it.
		if (pPlayer->HasPlayerItem(pWeapon))
		{
			return false;
		}
	}

	// note: will fall through to here if GetItemInfo doesn't fill the struct!
	return true;
}

//=========================================================
// load the SkillData struct with the proper values based on the skill level.
//=========================================================
void CGameRules::RefreshSkillData()
{
	int iSkill;

	iSkill = (int)CVAR_GET_FLOAT("skill");
	g_iSkillLevel = iSkill;

	if (iSkill < 1)
	{
		iSkill = 1;
	}
	else if (iSkill > 3)
	{
		iSkill = 3;
	}

	gSkillData.iSkillLevel = iSkill;

	ALERT(at_console, "\nGAME SKILL LEVEL:%d\n", iSkill);

	gSkillData.barneyHealth = 35.0f;
	gSkillData.bullsquidDmgBite = 25.0f;
	gSkillData.slaveDmgClawrake = 25.0f;
	gSkillData.agruntDmgPunch = 20.0f;
	gSkillData.controllerHealth = 60.0f;
	gSkillData.bullsquidHealth = 40.0f;
	gSkillData.bullsquidDmgWhip = 35.0f;
	gSkillData.scientistHeal = 25.0f;
	gSkillData.bullsquidDmgSpit = 10.0f;
	gSkillData.bigmommaRadiusBlast = 250.0f;
	gSkillData.plrDmgSniper = 60.0f;
	gSkillData.gargantuaHealth = 800.0f;
	gSkillData.gargantuaDmgSlash = 30.0f;
	gSkillData.gargantuaDmgFire = 5.0f;
	gSkillData.gargantuaDmgStomp = 100.0f;
	gSkillData.hassassinHealth = 50.0f;
	gSkillData.headcrabHealth = 10.0f;
	gSkillData.headcrabDmgBite = 20.0f;
	gSkillData.hgruntHealth = 50.0f;
	gSkillData.hgruntDmgKick = 10.0f;
	gSkillData.houndeyeHealth = 20.0f;
	gSkillData.houndeyeDmgBlast = 15.0f;
	gSkillData.slaveHealth = 30.0f;
	gSkillData.slaveDmgClaw = 10.0f;
	gSkillData.slaveDmgZap = 10.0f;
	gSkillData.ichthyosaurHealth = 200.0f;
	gSkillData.leechHealth = 2.0f;
	gSkillData.leechDmgBite = 2.0f;
	gSkillData.nihilanthHealth = 800.0f;
	gSkillData.nihilanthZap = 30.0f;
	gSkillData.scientistHealth = 20.0f;
	gSkillData.snarkHealth = 2.0f;
	gSkillData.snarkDmgBite = 10.0f;
	gSkillData.snarkDmgPop = 5.0f;
	gSkillData.zombieHealth = 50.0f;
	gSkillData.zombieDmgOneSlash = 20.0f;
	gSkillData.zombieDmgBothSlash = 40.0f;
	gSkillData.turretHealth = 50.0f;
	gSkillData.miniturretHealth = 40.0f;
	gSkillData.sentryHealth = 40.0f;
	gSkillData.monDmg12MM = 10.0f;
	gSkillData.monDmg9MM = 5.0f;
	gSkillData.healthkitCapacity = 15.0f;
	gSkillData.monHead = 1.0f;
	gSkillData.monChest = 1.0f;
	gSkillData.monStomach = 1.0f;
	gSkillData.monLeg = 1.0f;
	gSkillData.monArm = 1.0f;
	gSkillData.plrDmgCrowbar = 10.0f;
	gSkillData.plrDmgPipe = 20.0f;
	gSkillData.plrDmg9MM = 8.0f;
	gSkillData.plrDmg357 = 40.0f;
	gSkillData.plrDmgMP5 = 5.0f;
	gSkillData.plrDmgCrossbowClient = 10.0f;
	gSkillData.plrDmgCrossbowMonster = 50.0f;
	gSkillData.plrDmgM203Grenade = 100.0f;
	gSkillData.plrDmgBuckshot = 5.0f;
	gSkillData.plrDmgHornet = 10.0f;
	gSkillData.monDmgHornet = 10.0f;
	gSkillData.plrDmgRPG = 100.0f;
	gSkillData.plrDmgHandGrenade = 100.0f;
	gSkillData.plrDmgSatchel = 150.0f;
	gSkillData.plrDmgTripmine = 150.0f;
	gSkillData.plrHead = 1.0f;
	gSkillData.plrChest = 1.0f;
	gSkillData.plrStomach = 1.0f;
	gSkillData.plrLeg = 1.0f;
	gSkillData.plrArm = 1.0f;

	switch ( gSkillData.iSkillLevel )
	{
	  case 1:
	    gSkillData.agruntHealth = 60.0f;
	    gSkillData.agruntDmgPunch = 10.0f;
	    gSkillData.apacheHealth = 150.0f;
	    gSkillData.bullsquidDmgWhip = 25.0f;
	    gSkillData.bigmommaHealthFactor = 1.0f;
	    gSkillData.bigmommaDmgSlash = 50.0f;
	    gSkillData.bigmommaDmgBlast = 100.0f;
	    gSkillData.bullsquidDmgBite = 15.0f;
	    gSkillData.gargantuaDmgSlash = 10.0f;
	    gSkillData.gargantuaDmgFire = 3.0f;
	    gSkillData.gargantuaDmgStomp = 50.0f;
	    gSkillData.hassassinHealth = 30.0f;
	    gSkillData.headcrabDmgBite = 15.0f;
	    gSkillData.hgruntDmgKick = 5.0f;
	    gSkillData.hgruntShotgunPellets = 3.0f;
	    gSkillData.hgruntGrenadeSpeed = 400.0f;
	    gSkillData.houndeyeDmgBlast = 10.0f;
	    gSkillData.slaveDmgClaw = 8.0f;
	    gSkillData.ichthyosaurDmgShake = 20.0f;
	    gSkillData.controllerDmgZap = 20.0f;
	    gSkillData.controllerSpeedBall = 650.0f;
	    gSkillData.controllerDmgBall = 3.0f;
	    gSkillData.zombieDmgOneSlash = 15.0f;
	    gSkillData.zombieDmgBothSlash = 30.0f;
	    gSkillData.monDmg12MM = 8.0f;
	    gSkillData.monDmgMP5 = 6.0f;
	    gSkillData.monDmgHornet = 10.0f;
	    gSkillData.healthchargerCapacity = 50.0f;
	    break;
	  case 2:
	    gSkillData.bigmommaDmgSlash = 60.0f;
	    gSkillData.agruntHealth = 90.0f;
	    gSkillData.apacheHealth = 250.0f;
	    gSkillData.controllerDmgZap = 25.0f;
	    gSkillData.bigmommaHealthFactor = 1.5f;
	    gSkillData.bigmommaDmgBlast = 120.0f;
	    gSkillData.hgruntShotgunPellets = 5.0f;
	    gSkillData.hgruntGrenadeSpeed = 600.0f;
	    gSkillData.ichthyosaurDmgShake = 35.0f;
	    gSkillData.controllerSpeedBall = 800.0f;
	    gSkillData.controllerDmgBall = 4.0f;
	    gSkillData.monDmgMP5 = 8.0f;
	    gSkillData.monDmgHornet = 13.0f;
	    gSkillData.healthchargerCapacity = 40.0f;
	    break;
	  case 3:
	    gSkillData.slaveHealth = 60.0f;
	    gSkillData.turretHealth = 60.0f;
	    gSkillData.agruntHealth = 120.0f;
	    gSkillData.apacheHealth = 400.0f;
	    gSkillData.healthchargerCapacity = 25.0f;
	    gSkillData.bigmommaHealthFactor = 2.0f;
	    gSkillData.bigmommaDmgSlash = 70.0f;
	    gSkillData.bigmommaDmgBlast = 160.0f;
	    gSkillData.bigmommaRadiusBlast = 275.0f;
	    gSkillData.gargantuaHealth = 1000.0f;
	    gSkillData.hgruntHealth = 80.0f;
	    gSkillData.hgruntShotgunPellets = 6.0f;
	    gSkillData.hgruntGrenadeSpeed = 800.0f;
	    gSkillData.houndeyeHealth = 30.0f;
	    gSkillData.slaveDmgZap = 15.0f;
	    gSkillData.ichthyosaurHealth = 400.0f;
	    gSkillData.ichthyosaurDmgShake = 50.0f;
	    gSkillData.controllerHealth = 100.0f;
	    gSkillData.controllerDmgZap = 35.0f;
	    gSkillData.controllerSpeedBall = 1000.0f;
	    gSkillData.controllerDmgBall = 5.0f;
	    gSkillData.nihilanthHealth = 1000.0f;
	    gSkillData.nihilanthZap = 50.0f;
	    gSkillData.zombieHealth = 100.0f;
	    gSkillData.miniturretHealth = 50.0f;
	    gSkillData.sentryHealth = 50.0f;
	    gSkillData.monDmgMP5 = 10.0f;
	    gSkillData.monDmg9MM = 8.0f;
	    gSkillData.monDmgHornet = 16.0f;
	    gSkillData.healthkitCapacity = 10.0f;
	    break;
	}
}

//=========================================================
// instantiate the proper game rules object
//=========================================================

CGameRules* InstallGameRules()
{
	SERVER_COMMAND("exec game.cfg\n");
	SERVER_EXECUTE();

	if (1 == sv_busters.value)
	{
		g_teamplay = false;
		return new CMultiplayBusters;
	}
	else if (0 == gpGlobals->deathmatch)
	{
		// generic half-life
		g_teamplay = false;
		return new CHalfLifeRules;
	}
	else
	{
		if (teamplay.value > 0)
		{
			// teamplay

			g_teamplay = true;
			return new CHalfLifeTeamplay;
		}
		if ((int)gpGlobals->deathmatch == 1)
		{
			// vanilla deathmatch
			g_teamplay = false;
			return new CHalfLifeMultiplay;
		}
		else
		{
			// vanilla deathmatch??
			g_teamplay = false;
			return new CHalfLifeMultiplay;
		}
	}
}
