// Copyright Void Interactive, 2017

#pragma once

#include "Characters/CyberneticCharacter.h"
#include "SuspectCharacter.generated.h"

UCLASS()
class READYORNOT_API ASuspectCharacter final : public ACyberneticCharacter
{
    GENERATED_BODY()
	
public:
	ASuspectCharacter();

	virtual bool TryApplyStunDamage(UStunDamage* InStunDamage, float& Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	// For the DUE_PROCESS achievement, we need to know if the last damage that killed us was a C2
	bool bLastDamageCauserIsC2 = false;
	
protected:
	virtual bool OnTakeDamage(float& Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	bool CheckWasFlanked(AController* Attacker);
	float MaxReactToFlankCooldownDuration = 15.0f;

	virtual void OnKilled(AReadyOrNotCharacter* InstigatorCharacter) override;
};
