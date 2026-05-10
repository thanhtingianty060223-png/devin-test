// � Void Interactive, 2020

#pragma once

#include "DamageTypes/StunDamage.h"
#include "StingerDamageType.generated.h"

UCLASS(Blueprintable, BlueprintType)
class READYORNOT_API UStingerDamageType : public UStunDamage
{
	GENERATED_BODY()

public:
	virtual void ScriptedStunEvent(class AReadyOrNotCharacter* Victim, float& Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
};