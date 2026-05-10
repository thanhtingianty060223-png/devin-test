// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

//#include "SequenceEvaluatorLibrary.h"
#include "Animation/AnimCurveTypes.h"

#include "Kismet/BlueprintFunctionLibrary.h"

// needed for curve access
#include "Animation/AnimCurveCompressionCodec_UniformIndexable.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "AnimCharacterMovementLibrary.h"
#include "AnimDistanceMatchingLibrary.generated.h"

class UAnimSequence;

UENUM(BlueprintType)
enum class EDistanceMatchingType : uint8
{
	Start,
	Stop,
	Pivot,
	Jump,
	Fall,
	None,
};

UENUM(BlueprintType)
enum class ETransitionAnimationType : uint8
{
	Start,
	Stop,
	Pivot,
	None,
};

/**
 * A animation used for Distance Matching
 * Uses additional properties:
 * Animation = The animation to use
 * Distance Curve Name = The curve name to lookup when Distance Matching
 * bEnableDistanceLimit = Should we use a specified distance limit
 * Distance Limit = The actual Distance Limit, when reached play rest of Animation normally
 * Blend out Time = If set do not use automatic transitions but try to get out of the motion at this time
 * */
USTRUCT(BlueprintType)
struct FDistanceMatchAnimation
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AnimationReference)
	UAnimSequence* Animation = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DistanceMatchProperties)
	FName DistanceCurveName = "DistanceCurve";
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DistanceMatchProperties)
	bool bEnableDistanceLimit = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DistanceMatchProperties)
	float DistanceLimit = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DistanceMatchProperties)
	float BlendOutTime = 0.0f;
};


/**
* A set of Distance Match Animations for Cardinal Blending
* */
USTRUCT(BlueprintType)
struct FDistanceMatchCardinalAnimSet
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animation)
	FDistanceMatchAnimation NorthAnim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animation)
	FDistanceMatchAnimation EastAnim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animation)
	FDistanceMatchAnimation SouthAnim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animation)
	FDistanceMatchAnimation WestAnim;
};

/**
 * Library of techniques for driving animations by distance metrics rather than by time.
 * These techniques can be effective at compensating for differences between character movement and authored motion in the animations.
 * Distance Matching effectively changes the playrate of the animation to keep the feet from sliding. It's common to clamp the resulting
 * play rate to avoid animations playing too slow or too fast and to use techniques such as Stride Warping to make up the difference.
 */
UCLASS()
class UAnimDistanceMatchingLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Advance the sequence evaluator forward by distance traveled rather than time. A distance curve is required on the animation that
	 * describes the distance traveled by the root bone in the animation. See UDistanceCurveModifier.
	 * @param UpdateContext - The update context provided in the anim node function.
	 * @param SequenceEvaluator - The sequence evaluator node to operate on.
	 * @param DistanceTraveled - The distance traveled by the character since the last animation update. See FAnimCharacterMovementSnapshot.Distance2DTraveledSinceLastUpdate.
	 * @param CachedDistanceCurve - Optimized access to curve data. This will typically be a FDistanceCurve variable on the animation blueprint. 
	 * @param PlayRateAdjustmentCurve - Optional curve that can be used to clamp how much the animation can be advanced in an update (to avoid play back being
	 *	too fast or too slow). The X axis in the curve should be the resulting play rate from distance matching. The Y axis should be the clamped play rate.
	 * @param PlayRateClamp - Only used if PlayRateAdjustmentCurve is not set. A simple clamp instead of a curve.
	 */
	/*
	UFUNCTION(BlueprintCallable, Category = "Distance Matching", meta=(BlueprintThreadSafe))
	static FSequenceEvaluatorReference AdvanceTimeByDistanceMatching(const FAnimUpdateContext& UpdateContext, const FSequenceEvaluatorReference& SequenceEvaluator,
		float DistanceTraveled, const FDistanceCurve& CachedDistanceCurve, const UCurveFloat* PlayRateAdjustmentCurve, FVector2D PlayRateClamp = FVector2D(0.75f, 1.25f));
	*/
	/**
	 * Set the time of the sequence evaluator to the point in the animation where the distance curve matches the DistanceToTarget input.
	 * A common use case is to achieve stops without foot sliding by, each frame, selecting the point in the animation that matches the distance the character has remaining until it stops.
	 * Note that because this technique sets the time of the animation by distance remaining, it doesn't respect phase of any previous animation (e.g. from a jog cycle).
	 * @param SequenceEvaluator - The sequence evaluator node to operate on.
	 * @param DistanceToTarget - The distance remaining to a target (e.g. a stop or pivot point).
	 * @param CachedDistanceCurve - Optimized access to curve data. This will typically be a FDistanceCurve variable on the animation blueprint. 
	 */
	/*
	UFUNCTION(BlueprintCallable, Category = "Distance Matching", meta=(BlueprintThreadSafe))
	static FSequenceEvaluatorReference DistanceMatchToTarget(const FSequenceEvaluatorReference& SequenceEvaluator,
		float DistanceToTarget, const FDistanceCurve& CachedDistanceCurve);
	*/
	
	/* Distance Matching for Locomotion and Jumping tied into a single function, returns PredictionResults to be used directly in Anim Graph */
	UFUNCTION(BlueprintCallable, Category = "Distance Matching", meta = (BlueprintThreadSafe))
	static void CalculateDistanceMatchingStates(float DeltaTime, 
		ACharacter* CurrentCharacter, 
		UCharacterMovementComponent* CurrentMovementComponent, 
		EDistanceMatchingType& DistanceMatchingCurrentState, 
		const FAnimCharacterMovementSnapshot& MovementSnapshot, 
		const FAnimCharacterMovementPredictionSnapshot& PredictionSnapshot, 
		UWorld* TargetWorld,
		float MinPivotAngle,
		TArray<AActor*> ActorsToIgnore,
		FVector& LastActorLocation,
		FCardinalDirectionSnapshot& PivotingCardinalDirSnapShot,
		FPredictionResult& StartMarker, 
		FPredictionResult& StopMarker, 
		FPredictionResult& PivotMarker, 
		FPredictionResult& TakeOffMarker, 
		FPredictionResult& ApexMarker, 
		FPredictionResult& LandingMarker,
		bool& bSMStartRuleset,
		bool& bSMStopRuleset,
		bool bIsAICharacter,
		bool bShowDebug = true);

	/* UE5 Function backports */
	static USkeleton::AnimCurveUID GetCurveUID(const UAnimSequence* InAnimSequence, FName CurveName);
	static float GetDistanceRange(const UAnimSequence* InAnimSequence, USkeleton::AnimCurveUID CurveUID);
	static float GetAnimPositionFromDistance(const UAnimSequence* InAnimSequence, const float& InDistance, USkeleton::AnimCurveUID InCurveName);
	static float GetTimeAfterDistanceTraveled(UAnimSequence* AnimSequence, float CurrentTime, float DistanceTraveled, USkeleton::AnimCurveUID CurveName, const bool bAllowLooping);
	static void AdvanceTimeByDistanceMatching(float DeltaSeconds, UAnimSequence* Sequence, float& ExplicitTime, const float& DistanceTraveled, FName InCurveName, FVector2D InPlayRateClamp);
	static void DistanceMatchToTarget(UAnimSequence* Sequence, float& ExplicitTime, const float& DistanceToTarget, FName CurDistanceCurveName);
	static void AdvanceTimeNaturally(float DeltaSeconds, UAnimSequence* Sequence, float& ExplicitTime);

	static float GetTimeFromDistance(UAnimSequence* Sequence, float Distance, FName CurveName);

	static bool ShouldDistanceMatchStop(const FTransform& WorldTransform, const FVector& WorldVelocity, const FVector& WorldAcceleration);
	static FVector PredictGroundMovementStopLocation(const FVector& WorldVelocity, const bool& bUseSeparateBrakingFriction, const float& BrakingFriction, const float& GroundFriction, const float& BrakingFrictionFactor, const float& BrakingDecelerationWalking);
	static FVector PredictGroundMovementPivotLocation(const FVector& WorldAcceleration, const FVector& WorldVelocity, const float& GroundFriction);
	static bool AccelerationOpposesVelocity(const FTransform& WorldTransform, const FVector& WorldVelocity, const FVector& WorldAcceleration);

	static float GetRelevantAnimTimeRemainingFraction(UAnimInstance* AnimInstance, FName StateMachineName, FName StateName);
	static float GetRelevantAnimTime(UAnimInstance* AnimInstance, FName StateMachineName, FName StateName);

	static float GetStateTransitionCrossfadeDuration(UAnimInstance* AnimInstance, FName StateMachineName, FName FromStateName, FName ToStateName);
	static bool ShouldPerformAutomaticStateTransition(float CrossfadeDuration, float SequenceLength, float SequenceCurrentTime);

	static float FlipAngle(float OriginalAngle)
	{
		float flippedAngle = OriginalAngle + 180.0f;
		// Normalize to [-180, 180)
		while (flippedAngle < -180.0f) flippedAngle += 360.0f;
		while (flippedAngle >= 180.0f) flippedAngle -= 360.0f;
		return flippedAngle;
	}

	static FTransform ExtractStartingRootMotionFromSequence(UAnimSequence* AnimSequence);
	static FTransform ExtractEndingRootMotionFromSequence(UAnimSequence* AnimSequence);
	static float CalculateAnimScoreForDirection(const FTransform& RootMotionTransform, const FVector& Direction);
	static UAnimSequence* FindBestDirectionAnimation(const TArray<UAnimSequence*>& AnimSequences, const FVector& Direction, bool bUseStartingRootMotion);
	static float CalculateAnimationSpeed(UAnimSequence* AnimSequence);	
};