// Copyright Void Interactive, 2021

#pragma once

#include "DamageTypes/StunDamage.h"
#include "NinebangDamageType.generated.h"

UCLASS(Blueprintable, BlueprintType)
class READYORNOT_API UNinebangDamageType : public UStunDamage
{
	GENERATED_BODY()

public:
	virtual void ScriptedStunEvent(class AReadyOrNotCharacter* Victim, float& Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
};