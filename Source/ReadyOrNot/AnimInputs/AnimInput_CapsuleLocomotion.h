// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/ObjectMacros.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AnimInput_CapsuleLocomotion.generated.h"

class APawn;

// Process animation data for a capsule driven character.
USTRUCT(BlueprintType)
struct FAnimInput_CapsuleLocomotion
{
	GENERATED_BODY()

public:

	float CalculateDirectionInput(const FVector& Velocity, const FRotator& BaseRotation) const;
	void Update(const APawn* Pawn);

protected:

	UPROPERTY(BlueprintReadWrite)
	FVector WorldVelocity;

	UPROPERTY(BlueprintReadWrite)
	FVector LocalVelocity;

	UPROPERTY(BlueprintReadWrite)
	FVector WorldAcceleration;

	UPROPERTY(BlueprintReadWrite)
	FVector LocalAcceleration;

	// Angle (in degrees) between velocity and the Pawn's forward vector.
	UPROPERTY(BlueprintReadWrite)
	float VelocityYawAngle;

	// Angle (in degrees) between acceleration and the Pawn's forward vector.
	UPROPERTY(BlueprintReadWrite)
	float AccelerationYawAngle;

	UPROPERTY(BlueprintReadWrite)
	float Speed2D;

	// Speed2D has to be above this threshold for the Pawn to be considered moving.
	UPROPERTY(EditAnywhere, Category = Tunables)
	float MovingThreshold;

	UPROPERTY(BlueprintReadWrite)
	bool bIsMoving2D;

	UPROPERTY(BlueprintReadWrite)
	bool bHasAcceleration2D;

	// If the Pawn is trying to acceleration away from velocity. Common use case is for triggering pivot transitions.
	UPROPERTY(BlueprintReadWrite)
	bool bAccelerationOpposesVelocity;

	UPROPERTY(BlueprintReadWrite)
	bool bIsOnGround;
};

UCLASS()
class UAnimInputCapsuleLocomotionBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Anim Inputs")
	static void UpdateCapsuleLocomotionAnimInput(const APawn* Pawn, UPARAM(ref) FAnimInput_CapsuleLocomotion& CapsuleLocomotionAnimInput);
};