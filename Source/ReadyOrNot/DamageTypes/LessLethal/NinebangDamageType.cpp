// Copyright Void Interactive, 2021

#include "NinebangDamageType.h"
#include "ReadyOrNot.h"
#include "ReadyOrNotAIConfig.h"
#include "Characters/CyberneticCharacter.h"

void UNinebangDamageType::ScriptedStunEvent(class AReadyOrNotCharacter* Victim, float& Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (!Victim)
		return;

	if (Victim->IsDeadOrUnconscious())
		return;

	ACyberneticCharacter* Character = Cast<ACyberneticCharacter>(Victim);
	if (!Character)
		return;

	const float AccuracyPenalty = AI_CONFIG_GET_FLOAT("NineBangerAccuracyPenalty");
	const float StunThreshold = AccuracyPenalty * 4.0f; // Must be greater than this value for stun to apply, one less than the max 4x

	// Apply the accuracy penalty, returning if we aren't dazed enough to be stunned
	Character->StunAccuracyPenalty += AccuracyPenalty;
	if (Character->StunAccuracyPenalty < StunThreshold)
		return;

	// If we've managed to hit sufficiently, then finally apply a stun via the super
	Super::ScriptedStunEvent(Victim, Damage, DamageEvent, EventInstigator, DamageCauser);
}