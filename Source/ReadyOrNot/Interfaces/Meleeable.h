// Void Interactive, 2020

#pragma once

#include "UObject/Interface.h"
#include "Meleeable.generated.h"

UINTERFACE(MinimalAPI)
class UMeleeable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class READYORNOT_API IMeleeable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|Meleeable Interface")
	void OnMelee(class AReadyOrNotCharacter* Attacker, FHitResult Hit);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|Meleeable Interface")
	UFMODEvent* GetMeleeImpactSound() const;
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|Meleeable Interface")
	UParticleSystem* GetMeleeImpactParticle() const;

	// In a networked environment, should melee cosmetic effects be played locally?
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|Meleeable Interface")
	bool ShouldPlayMeleeEffectsLocally() const;
};
