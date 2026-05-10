// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AnimCharacterMovementTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "AnimCharacterMovementLibrary.generated.h"

class UAnimSequence;

// Maximum distance or time value to prevent float overflow.
#define MAX_MATCH_VALUE (1000.0f)
// Minimum value to determine if character moving or accelerating.
#define MOVEMENT_THRESHOLD (0.00001f)

/* useful structs to work with the results of this component */


USTRUCT(BlueprintType)
struct FPredictionResult
{
	GENERATED_BODY()

	/** Predicted marker location. */
	UPROPERTY(BlueprintReadOnly)
	FVector Location;

	/** Distance to marker location. */
	UPROPERTY(BlueprintReadOnly)
	float Distance;

	/** Distance to marker location. */
	UPROPERTY(BlueprintReadOnly)
	float DistanceAbsolute;

	/** Time to marker location. */
	UPROPERTY(BlueprintReadOnly)
	float Time;

	/** Speed when Prediction Result was created. */
	UPROPERTY(BlueprintReadOnly)
	float Speed;

	/** Yaw Angle when Prediction Result was created. */
	UPROPERTY(BlueprintReadOnly)
	float YawAngle;

	FPredictionResult()
		: Location(ForceInitToZero)
		, Distance(0.0f)
		, DistanceAbsolute(0.0f)
		, Time(0.0f)
		, Speed(0.0f)
		, YawAngle(0.0f)
	{
	}
};

/**
  * Library of common techniques for driving character locomotion animations. 
  */
UCLASS()
class UAnimCharacterMovementLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	static float CalculateDirectionInput(const FVector& Velocity, const FRotator& BaseRotation);

	/**
	 * Populate a snapshot struct with movement data that's commonly used by animation graph logic.
	 * To avoid performance costs from calling this in the Event Graph on the game thread, it's recommended to call it in BlueprintThreadSafeUpdateAnimation and
 	 * use the Property Access system to access the input parameters (Property Access will handle copying the inputs at the right time in the frame).
	 * @param WorldTransform - The transform of the character in world space.
	 * @param WorldVelocity - The velocity of the character in world space.
	 * @param WorldAcceleration - The acceleration of the character in world space.
	 * @param bIsOnGround - If the character is on the ground.
	 * @param RootYawOffset - Offset being applied to the root bone in the animation graph (e.g. for countering capsule rotation). Set to zero if not needed.
	 * @param Snapshot - The snapshot to write to. This is typically a member variable of the animation blueprint.
	 */
	UFUNCTION(BlueprintCallable, Category = "Animation Character Movement", meta = (BlueprintThreadSafe))
	static void UpdateCharacterMovementSnapshot(const FTransform& WorldTransform, const FVector& WorldVelocity, const FVector& WorldAcceleration, bool bIsOnGround,
		float RootYawOffset, UPARAM(ref) FAnimCharacterMovementSnapshot& Snapshot);

	/**
	 * Calculate the closest cardinal direction to the direction the character is currently moving.
	 * @param PreviousCardinalDirection - The cardinal direction from the previous frame. Typically the animation blueprint holds a EAnimCardinalDirection variable.
	 * @param DirectionAngleInDegrees - The direction that the character is currently moving. FAnimCharacterMovementSnapshot.VelocityYawAngle is a commonly used input for this.
	 * @param DirectionDeadZoneAngle - Deadzone to prevent flickering between directions at angle boundaries.
	 * @return The resulting cardinal direction.
	 */
	UFUNCTION(BlueprintPure, Category = "Animation Character Movement", meta = (BlueprintThreadSafe))
	static EAnimCardinalDirection GetCardinalDirectionFromAngle(EAnimCardinalDirection PreviousCardinalDirection, float DirectionAngleInDegrees, float DeadZoneAngle);

	/* calculate directional angle with offset, useful for orientation warping to get 45s on left, right and backward*/
	UFUNCTION(BlueprintPure, Category = "Animation Character Movement", meta = (BlueprintThreadSafe))
	static float CalculateDirAngle(FVector CurVel, FRotator CurActorRotation, FRotator DirRotation, float ClampMin, float ClampMax, float CurDirAngle);

	/**
	 * Select an animation to play based on the cardinal direction calculated by GetCardinalDirectionFromAngle(). For example, this can pick a start animation
	 * to play based on the character's movement direction.	    
	 * @param CardinalDirection - The closest cardinal direction to the character's movement direction.
	 * @param AnimSet - The set of animations to choose from.
	 * @return The animation to play.
	 */
	UFUNCTION(BlueprintPure, Category = "Animation Character Movement", meta = (BlueprintThreadSafe))
	static const UAnimSequence* SelectAnimForCardinalDirection(EAnimCardinalDirection CardinalDirection, const FCardinalDirectionAnimSet& AnimSet);

	/**
	 * Predict where the character will stop based on its current movement properties and parameters from the movement component.
	 * This uses prediction logic that is heavily tied to the UCharacterMovementComponent.
	 * @param MovementSnapshot - Snapshot of current movement properties.
	 * @param PredictionSnapshot - Snapshot of parameters needed to predict how the movement component will move. Because this is a thread safe function, it's
	 *	recommended to populate these fields via the Property Access system.
	 * @return The predicted stop position in local space to the character. The size of this vector will be the distance to stop.
	 */
	UFUNCTION(BlueprintPure, Category = "Animation Character Movement", meta = (BlueprintThreadSafe))
	static FVector PredictGroundMovementStopLocation(const FAnimCharacterMovementSnapshot& MovementSnapshot, const FAnimCharacterMovementPredictionSnapshot& PredictionSnapshot);

	/**
	 * Predict where the character will change direction during a pivot based on its current movement properties and parameters from the movement component.
	 * This uses prediction logic that is heavily tied to the UCharacterMovementComponent.
	 * @param MovementSnapshot - Snapshot of current movement properties.
	 * @param GroundFriction - Value from the movement component. Because this is a thread safe function, it's recommended to populate this field via the Property Access system.
	 * @return The predicted pivot position in local space to the character. The size of this vector will be the distance to the pivot.
	 */
	UFUNCTION(BlueprintPure, Category = "Animation Character Movement", meta = (BlueprintThreadSafe))
	static FVector PredictGroundMovementPivotLocation(const FAnimCharacterMovementSnapshot& MovementSnapshot, float GroundFriction);

	/**
	* Predict the arc of a jump path affected by gravity with collision checks along the arc.
	*
	* @param PredictResult			Output result of the prediction (location and time).
	* @param SimulationTime			Maximum simulation time for the jump path prediction.
	* @param SimulationFrequency	Determines size of each sub-step in the simulation (chopping up SimulationTime).
	*/
	static void PredictJumpPath(FPredictionResult& PredictResult, const FAnimCharacterMovementSnapshot& MovementSnapshot, const FAnimCharacterMovementPredictionSnapshot& PredictionSnapshot, UWorld* TargetWorld, bool bDrawDebugTrace, TArray<AActor*> ActorsToIgnore, const float SimulationTime = 2.0f, const float SimulationFrequency = 10.0f);

	/** Predict the jump apex location and time to it. */
	static void PredictJumpApex(FPredictionResult& PredictResult, const FAnimCharacterMovementSnapshot& MovementSnapshot, const FAnimCharacterMovementPredictionSnapshot& PredictionSnapshot, UWorld* TargetWorld, bool bDrawDebugTrace, TArray<AActor*> ActorsToIgnore, float ApexSimulationFrequency = 5.0f);

	// DistanceToFloor = MovementComponent->CurrentFloor.FloorDist;

	/** Predict the jump landing location and time to it. */
	static void PredictLandingLocation(FPredictionResult& PredictResult, const FAnimCharacterMovementSnapshot& MovementSnapshot, const FAnimCharacterMovementPredictionSnapshot& PredictionSnapshot, UWorld* TargetWorld, bool bDrawDebugTrace, TArray<AActor*> ActorsToIgnore, float DistanceToFloor, float MaxSimulationTime = 2.0f, float LandingSimulationFrequency = 5.0f);
};