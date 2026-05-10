// Copyright Void Interactive, 2024

#include "BaseArmour.h"

ABaseArmour::ABaseArmour()
{
}

float ABaseArmour::CalculateDurabilityDamageFactor(float Damage, const FAmmoTypeData* AmmoTypeData)
{
	if (!AmmoTypeData)
		return 1.0f;

	float MaxDamage = AmmoTypeData->Damage;
	if (const FRichCurve* DamageCurve = AmmoTypeData->DamageOverRangeCurve.GetRichCurveConst())
	{
		if (DamageCurve->GetNumKeys() > 0)
			MaxDamage = DamageCurve->Eval(0.0f);
	}

	if (Damage == 0.0f || MaxDamage == 0.0f)
		return 0.0f;

	float DurabilityDamageFactor = FMath::Clamp(Damage / MaxDamage, 0.0f, 1.0f);
	return DurabilityDamageFactor;
}
