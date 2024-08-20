#include "extdll.h"
#include "util.h"
#include "cbase.h"

class CHudToggle : public CPointEntity
{
public:
	void Spawn() override;
	bool KeyValue(KeyValueData* pkvd) override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

};

LINK_ENTITY_TO_CLASS(env_hudtoggle, CHudToggle);

void CHudToggle::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = 0;
	pev->frame = 0;
}

bool CHudToggle::KeyValue(KeyValueData* pkvd)
{
	return false;
}

void CHudToggle::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	pActivator->ToggleHud();
}
