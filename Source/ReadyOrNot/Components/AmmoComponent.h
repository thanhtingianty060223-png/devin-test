// Copyright Void Interactive, 2021

#pragma once

#include "CoreMinimal.h"
#include "ResourceComponent.h"
#include "AmmoComponent.generated.h"

/**
 * A reusable component that can be placed on any actor that requires the concept of ammo without having to define and implement one yourself.
 * It is also network replicated.
 * @author Ali
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class READYORNOT_API UAmmoComponent : public UResourceComponent
{
	GENERATED_BODY()

public:	
	UAmmoComponent();

	// Retreives the ammo usage/depletion rate
	UFUNCTION(BlueprintPure, Category = "Ammo")
	FORCEINLINE float GetAmmoUsagePerSecond() const { return AmmoUsagePerSecond; }
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ammo", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float AmmoUsagePerSecond = 10.0f;
};
