// Copyright Void Interactive, 2021

#pragma once

#include "CoreMinimal.h"
#include "OutOfBoundsVolume.generated.h"

/**
 * A volume used to inform the player that they are outside the playable area
 */
UCLASS()
class READYORNOT_API AOutOfBoundsVolume : public AActor
{
	GENERATED_BODY()

public:
	AOutOfBoundsVolume();

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UBoxComponent* Bounds = nullptr;
};
