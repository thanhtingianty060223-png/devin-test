// � Void Interactive, 2020

#pragma once

#include "DamageTypes/StunDamage.h"
#include "PepperballDamageType.generated.h"

UCLASS(Blueprintable, BlueprintType)
class READYORNOT_API UPepperballDamageType : public UStunDamage
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Cybernetics)
		float StunDurationHeadshot = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Cybernetics)
		float StunDurationLeg = 0.5f;

	virtual void ScriptedStunEvent(class AReadyOrNotCharacter* Victim, float& Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
};