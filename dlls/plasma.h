#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"

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