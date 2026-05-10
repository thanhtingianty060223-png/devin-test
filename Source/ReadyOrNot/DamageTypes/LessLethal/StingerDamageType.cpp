#include "StingerDamageType.h"
#include "ReadyOrNot.h"

void UStingerDamageType::ScriptedStunEvent(class AReadyOrNotCharacter* Victim, float& Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	Super::ScriptedStunEvent(Victim, Damage, DamageEvent, EventInstigator, DamageCauser);
}