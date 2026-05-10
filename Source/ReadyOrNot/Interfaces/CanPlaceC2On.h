// Copyright Void Interactive, 2021

#pragma once

#include "UObject/Interface.h"
#include "CanPlaceC2On.generated.h"

UINTERFACE(BlueprintType, Blueprintable)
class READYORNOT_API UCanPlaceC2On : public UInterface
{
	GENERATED_BODY()
};

class READYORNOT_API ICanPlaceC2On
{
	GENERATED_BODY()

public:
	// Returns true if we can place C2 on this thing now
	UFUNCTION(BlueprintNativeEvent, Category = "Interfaces|CanPlaceC2On")
	bool CanPlaceC2OnNow(class APlayerCharacter* C2Owner, class AC2Explosive* C2, FHitResult Hit);

	// Reaction when the C2 blows up
	UFUNCTION(BlueprintNativeEvent, Category = "Interfaces|CanPlaceC2On")
	void OnC2Detonated(class APlacedC2Explosive* C2);

	// C2 has started being placed, we can interrupt this if need be
	UFUNCTION(BlueprintNativeEvent, Category = "Interfaces|CanPlaceC2On")
	void C2StartPlacement(class AC2Explosive* C2);

	// C2 has stopped being placed (either it has finished being placed or canceled)
	UFUNCTION(BlueprintNativeEvent, Category = "Interfaces|CanPlaceC2On")
	void C2StopPlacement(class AC2Explosive* C2);

	// C2 was removed from the object
	UFUNCTION(BlueprintNativeEvent, Category = "Interfaces|CanPlaceC2On")
	void OnC2Removed(class APlacedC2Explosive* C2);

	UFUNCTION(BlueprintNativeEvent, Category = "Interfaces|CanPlaceC2On")
    FVector GetPlacementLocation(FHitResult TraceHit);
	
	UFUNCTION(BlueprintNativeEvent, Category = "Interfaces|CanPlaceC2On")
	FRotator GetPlacementRotation(FHitResult TraceHit);
};
