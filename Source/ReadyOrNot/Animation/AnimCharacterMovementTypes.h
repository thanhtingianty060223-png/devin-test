// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Animation/BlendSpace.h"

#include "AnimCharacterMovementTypes.generated.h"

class UAnimSequence;

UENUM(BlueprintType)
enum class EAnimCardinalDirection : uint8
{
	North,
	East,
	South,
	West
};

/**
 * Animations for a locomotion set authored with only four cardinal directions.
 * This will often be accompanied by Orientation Warping to account for diagonals.
 */
USTRUCT(BlueprintType)
struct FCardinalDirectionAnimSet
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animation)
	UAnimSequence* NorthAnim = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animation)
	UAnimSequence* EastAnim = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animation)
	UAnimSequence* SouthAnim = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animation)
	UAnimSequence* WestAnim = nullptr;
};

/**
* Animation Blendspaces for a locomotion set authored with only four cardinal directions.
* This will often be accompanied by Orientation Warping to account for diagonals.
*/
USTRUCT(BlueprintType)
struct FCardinalDirectionBlendSpaceAnimSet
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animation)
	UBlendSpace* NorthAnim = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animation)
	UBlendSpace* EastAnim = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animation)
	UBlendSpace* SouthAnim = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animation)
	UBlendSpace* WestAnim = nullptr;
};

/**
 * Snapshot of movement properties used to predict where the character will move in the future.
 * These properties are found on the UCharacterMovementComponent. They're copied (usually via Property Access) on the game thread
 * so they can be used in thread safe functions during animation update.
 */
USTRUCT(BlueprintType)
struct FAnimCharacterMovementPredictionSnapshot
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, Category = MovementPrediction)
	float GroundFriction = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = MovementPrediction)
	float BrakingFriction = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = MovementPrediction)
	float BrakingFrictionFactor = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = MovementPrediction)
	float BrakingDecelerationWalking = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = MovementPrediction)
	bool bUseSeparateBrakingFriction = false;

	UPROPERTY(BlueprintReadWrite, Category = MovementPrediction)
	float GravityZ = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = MovementPrediction)
	float CapsuleRadius = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = MovementPrediction)
	float CapsuleHalfHeight = 0.0f;
};

/**
 * A Snapshot taken of current Cardinal Direction Properties
 * Can be used for States that need static data instead of always changing, like Pre and Post Pivoting.
 * */
USTRUCT(BlueprintType)
struct FCardinalDirectionSnapshot
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = Data)
	EAnimCardinalDirection CardinalDirection;

	UPROPERTY(BlueprintReadOnly, Category = Data)
	float YawAngle;

	UPROPERTY(BlueprintReadOnly, Category = Data)
	float YawAngleDeltaNorth;

	UPROPERTY(BlueprintReadOnly, Category = Data)
	float YawAngleDeltaEast;

	UPROPERTY(BlueprintReadOnly, Category = Data)
	float YawAngleDeltaSouth;

	UPROPERTY(BlueprintReadOnly, Category = Data)
	float YawAngleDeltaWest;
};

/**
 * Snapshot of movement data commonly used to drive locomotion animations.
 * See UAnimationCharacterMovementLibrary::UpdateCharacterMovementSnapshot() for an example of how to populate this data.
 */
USTRUCT(BlueprintType)
struct FAnimCharacterMovementSnapshot
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, Category = Movement)
	FVector WorldLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category = Movement)
	FVector LastWorldLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category = Movement)
	FVector WorldVelocity = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category = Movement)
	FVector LocalVelocity = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category = Movement)
	FVector WorldAcceleration = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category = Movement)
	FVector LocalAcceleration = FVector::ZeroVector;

	/** Vector of VelocityYawAngle */
	UPROPERTY(BlueprintReadWrite, Category = Movement)
	FVector LocalVelocityDirection = FVector::ZeroVector;

	/** Vector of AccelerationYawAngle */
	UPROPERTY(BlueprintReadWrite, Category = Movement)
	FVector LocalAccelerationDirection = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category = Movement)
	EAnimCardinalDirection CurrentCardinalDirection = EAnimCardinalDirection::North;

	/** Angle (in degrees) between velocity and the character's forward vector. */
	UPROPERTY(BlueprintReadWrite, Category = Movement)
	float VelocityYawAngle = 0.0f;

	/** Angle (in degrees) between acceleration and the character's forward vector. */
	UPROPERTY(BlueprintReadWrite, Category = Movement)
	float AccelerationYawAngle = 0.0f;

	/** Clamped Angle (in degrees) between velocity and the character's forward vector. */
	UPROPERTY(BlueprintReadWrite, Category = Movement)
	float VelocityYawDeltaNorth = 0.0f;

	/** Clamped rotated -90 Angle (in degrees) between velocity and the character's forward vector. */
	UPROPERTY(BlueprintReadWrite, Category = Movement)
	float VelocityYawDeltaEast = 0.0f;
	
	/** Clamped rotated 180 Angle (in degrees) between velocity and the character's forward vector. */
	UPROPERTY(BlueprintReadWrite, Category = Movement)
	float VelocityYawDeltaSouth = 0.0f;

	/** Clamped rotated 90 Angle (in degrees) between velocity and the character's forward vector. */
	UPROPERTY(BlueprintReadWrite, Category = Movement)
	float VelocityYawDeltaWest = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = Movement)
	float Distance2DTraveledSinceLastUpdate = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = Movement)
	float Speed2D = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = Movement)
	float AccelerationSize2D = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = Movement)
	bool bIsOnGround = false;

	UPROPERTY(BlueprintReadWrite, Category = Movement)
	bool bIsMoving = false;

	UPROPERTY(BlueprintReadWrite, Category = Movement)
	bool bIsAccelerating = false;

	UPROPERTY(BlueprintReadWrite, Category = Movement)
	bool bIsMovingAndAccelerating = false;

	UPROPERTY(BlueprintReadWrite, Category = Movement)
	bool bIsMovingOrAccelerating = false;

	UPROPERTY(BlueprintReadWrite, Category = Movement)
	bool bIsNotMovingOrAccelerating = false;

	UPROPERTY(BlueprintReadWrite, Category = Movement)
	bool bIsNotMoving = false;

	UPROPERTY(BlueprintReadWrite, Category = Movement)
	bool bIsNotAccelerating = false;

	UPROPERTY(BlueprintReadWrite, Category = Movement)
	bool bIsNotMovingAndAccelerating = false;

	UPROPERTY(BlueprintReadWrite, Category = Movement)
	bool bIsMovingAndNotAccelerating = false;

	UPROPERTY(BlueprintReadWrite, Category = Movement)
	bool bAccelerationOpposesVelocity = false;

	UPROPERTY(BlueprintReadWrite, Category = Movement)
	bool bAccelerationEqualsVelocity = false;
};