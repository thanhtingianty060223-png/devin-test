#include "CSGasDamageType.h"
#include "ReadyOrNot.h"
#include "Characters/CyberneticCharacter.h"

void UCSGasDamageType::ScriptedStunEvent(class AReadyOrNotCharacter* Victim, float& Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// Gas damage is slightly different because it relies upon the accumulation effect.
	// We apply the damage to the accumulator and if the accumulator >= 1, THEN stun the person.

	if (!Victim)
		return;

	if (Victim->IsDeadOrUnconscious())
		return;

	if (!Victim->IsAffectedByDamageType(this))
		return;

	Victim->GasDamageAccumulated += Damage * 25.0f;
	
	if (Victim->GasDamageAccumulated >= 1.0f)
	{
		Victim->GasDamageAccumulated = 1.0f;

		if (!Victim->IsCurrentlyGassed())
		{
			if (APlayerController* Controller = Victim->GetController<APlayerController>())
			{
				Controller->ClientStartCameraShake(StunShake);
			}
		}

		Victim->StartStun(StunType, DamageCauser);
	}
}