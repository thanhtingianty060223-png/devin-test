// Copyright Void Interactive, 2022

#pragma once

#include "GameFramework/Actor.h"
#include "BloodPool.generated.h"

/**
 * Blood pools get automatically spawned on the death of a character
 */
UCLASS(Blueprintable, BlueprintType)
class READYORNOT_API ABloodPool : public AActor
{
	GENERATED_BODY()

	ABloodPool();

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Blood and Gore")
	UDecalComponent* Decal;
};
