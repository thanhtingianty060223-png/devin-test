// Copyright Void Interactive, 2021

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffects/ReadyOrNotGameplayEffect.h"
#include "BasePlayerEffect.generated.h"

/**
 * A base class for applying a gameplay effect to a specific player character
 */
UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew)
class READYORNOT_API UBasePlayerEffect : public UReadyOrNotGameplayEffect
{
	GENERATED_BODY()

protected:
	void Initialize_Implementation(AActor* InActor) override;

	UPROPERTY(BlueprintReadOnly, Category = "Base Player Effect")
	class APlayerCharacter* PlayerCharacter;
};
