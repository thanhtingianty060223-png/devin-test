#include "FlashbangDamageType.h"
#include "ReadyOrNot.h"
#include "Actors/Items/BallisticsShield.h"

void UFlashbangDamageType::ScriptedStunEvent(class AReadyOrNotCharacter* Victim, float& Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	Super::ScriptedStunEvent(Victim, Damage, DamageEvent, EventInstigator, DamageCauser);
}