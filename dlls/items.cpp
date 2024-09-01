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
/*

===== items.cpp ========================================================

  functions governing the selection/use of weapons for players

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#include "skill.h"
#include "items.h"
#include "gamerules.h"
#include "UserMessages.h"

class CWorldItem : public CBaseEntity
{
public:
	bool KeyValue(KeyValueData* pkvd) override;
	void Spawn() override;
	int m_iType;
};

LINK_ENTITY_TO_CLASS(world_items, CWorldItem);

bool CWorldItem::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "type"))
	{
		m_iType = atoi(pkvd->szValue);
		return true;
	}

	return CBaseEntity::KeyValue(pkvd);
}

void CWorldItem::Spawn()
{
	CBaseEntity* pEntity = NULL;

	switch (m_iType)
	{
	case 44: // ITEM_BATTERY:
		pEntity = CBaseEntity::Create("item_battery", pev->origin, pev->angles);
		break;
	case 42: // ITEM_ANTIDOTE:
		pEntity = CBaseEntity::Create("item_antidote", pev->origin, pev->angles);
		break;
	case 43: // ITEM_SECURITY:
		pEntity = CBaseEntity::Create("item_security", pev->origin, pev->angles);
		break;
	case 45: // ITEM_BODYARMOUR:
		pEntity = CBaseEntity::Create("item_bodyarmour", pev->origin, pev->angles);
		break;
	}

	if (!pEntity)
	{
		ALERT(at_console, "unable to create world_item %d\n", m_iType);
	}
	else
	{
		pEntity->pev->target = pev->target;
		pEntity->pev->targetname = pev->targetname;
		pEntity->pev->spawnflags = pev->spawnflags;
	}

	REMOVE_ENTITY(edict());
}


void CItem::Spawn()
{
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;
	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 16));
	SetTouch(&CItem::ItemTouch);

	if (DROP_TO_FLOOR(ENT(pev)) == 0)
	{
		ALERT(at_error, "Item %s fell out of level at %f,%f,%f", STRING(pev->classname), pev->origin.x, pev->origin.y, pev->origin.z);
		UTIL_Remove(this);
		return;
	}
}

void CItem::ItemTouch(CBaseEntity* pOther)
{
	// if it's not a player, ignore
	if (!pOther->IsPlayer())
	{
		return;
	}

	CBasePlayer* pPlayer = (CBasePlayer*)pOther;

	// ok, a player is touching this item, but can he have it?
	if (!g_pGameRules->CanHaveItem(pPlayer, this))
	{
		// no? Ignore the touch.
		return;
	}

	if (MyTouch(pPlayer))
	{
		SUB_UseTargets(pOther, USE_TOGGLE, 0);
		SetTouch(NULL);

		// player grabbed the item.
		g_pGameRules->PlayerGotItem(pPlayer, this);
		if (g_pGameRules->ItemShouldRespawn(this) == GR_ITEM_RESPAWN_YES)
		{
			Respawn();
		}
		else
		{
			UTIL_Remove(this);
		}
	}
	else if (gEvilImpulse101)
	{
		UTIL_Remove(this);
	}
}

CBaseEntity* CItem::Respawn()
{
	SetTouch(NULL);
	pev->effects |= EF_NODRAW;

	UTIL_SetOrigin(pev, g_pGameRules->VecItemRespawnSpot(this)); // blip to whereever you should respawn.

	SetThink(&CItem::Materialize);
	pev->nextthink = g_pGameRules->FlItemRespawnTime(this);
	return this;
}

void CItem::Materialize()
{
	if ((pev->effects & EF_NODRAW) != 0)
	{
		// changing from invisible state to visible.
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "items/suitchargeok1.wav", 1, ATTN_NORM, 0, 150);
		pev->effects &= ~EF_NODRAW;
		pev->effects |= EF_MUZZLEFLASH;
	}

	SetTouch(&CItem::ItemTouch);
}

class CItemBodyArmour : public CItem
{
	void Spawn() override
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_kevlar.mdl");
		CItem::Spawn();
	}
	void Precache() override
	{
		PRECACHE_MODEL("models/w_kevlar.mdl");
		PRECACHE_SOUND("player/kevlar_zipper.wav");
	}
	bool MyTouch(CBasePlayer* pPlayer) override
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, "player/kevlar_zipper.wav", 1, ATTN_NORM, 0, RANDOM_LONG(100,150));

		pPlayer->SetHasSuit(true);
		pPlayer->pev->armorvalue = 100.0f;
		return true;
	}
};

LINK_ENTITY_TO_CLASS(item_bodyarmour, CItemBodyArmour);



class CItemBattery : public CItem
{
	void Spawn() override
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_battery.mdl");
		CItem::Spawn();
	}
	void Precache() override
	{
		PRECACHE_MODEL("models/w_battery.mdl");
		PRECACHE_SOUND("items/gunpickup2.wav");
	}
	bool MyTouch(CBasePlayer* pPlayer) override
	{
		return false;
	}
};

LINK_ENTITY_TO_CLASS(item_battery, CItemBattery);

class CItemFlashLight : public CItem
{
	void Spawn() override
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_flashlight.mdl");
		CItem::Spawn();
	}
	void Precache() override
	{
		PRECACHE_MODEL("models/w_flashlight.mdl");
		PRECACHE_SOUND("items/gunpickup2.wav");
	}
	bool MyTouch(CBasePlayer* pPlayer) override
	{
		if (pPlayer->pev->deadflag != DEAD_NO)
			return false;

		if ((pPlayer->pev->weapons & (1ULL << WEAPON_FLASHLIGHT)) != 0)
		{
			return false;
		}

		pPlayer->SetWeaponBit(WEAPON_FLASHLIGHT);

		EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, "items/gunpickup2.wav", 1, ATTN_NORM, 0, 100);
		pPlayer->m_flFlashLightTime = gpGlobals->time;
		return true;
	}
};

LINK_ENTITY_TO_CLASS(item_flashlight, CItemFlashLight);

class CItemAntidote : public CItem
{
	void Spawn() override
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_antidote.mdl");
		CItem::Spawn();
	}
	void Precache() override
	{
		PRECACHE_MODEL("models/w_antidote.mdl");
	}
	bool MyTouch(CBasePlayer* pPlayer) override
	{
		pPlayer->m_rgItems[ITEM_ANTIDOTE] += 1;
		return true;
	}
};

LINK_ENTITY_TO_CLASS(item_antidote, CItemAntidote);


class CItemSecurity : public CItem
{
	void Spawn() override
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_security.mdl");
		CItem::Spawn();
	}
	void Precache() override
	{
		PRECACHE_MODEL("models/w_security.mdl");
	}
	bool MyTouch(CBasePlayer* pPlayer) override
	{
		pPlayer->m_rgItems[ITEM_SECURITY] += 1;
		return true;
	}
};

LINK_ENTITY_TO_CLASS(item_security, CItemSecurity);
