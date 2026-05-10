// Copyright Epic Games, Inc. All Rights Reserved.

#include "AnimDistanceMatchingLibrary.h"
//#include "Animation/AnimSequence.h"
#include "AIController.h"
//#include "AnimNodes/AnimNode_SequenceEvaluator.h"
//#include "Curves/CurveFloat.h"

#include "Blueprint/AIBlueprintHelperLibrary.h"

//DEFINE_LOG_CATEGORY_STATIC(LogAnimDistanceMatchingLibrary, Verbose, All);

void UAnimDistanceMatchingLibrary::CalculateDistanceMatchingStates(float DeltaTime,
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
	bool bShowDebug)
{
	if (CurrentCharacter == nullptr)
		return;

	/* predict in-air and grounded states */
	if (CurrentMovementComponent->IsFalling() && CurrentMovementComponent->Velocity.Z > 0.0f)
	{
		if(DistanceMatchingCurrentState != EDistanceMatchingType::Jump)
		{
			DistanceMatchingCurrentState = EDistanceMatchingType::Jump;

			// ALEX this should only be cached once like the start marker 
			TakeOffMarker.Location = CurrentCharacter->GetActorLocation();
			TakeOffMarker.Time = 0.0f;

			UAnimCharacterMovementLibrary::PredictJumpApex(ApexMarker, MovementSnapshot, PredictionSnapshot, TargetWorld, bShowDebug, ActorsToIgnore, 5.0f);

#if ENABLE_DRAW_DEBUG
			if (bShowDebug)
			{
				DrawDebugSphere(TargetWorld, TakeOffMarker.Location, 16.0f, 16.0f, FColor::Green, false, 1.5f, 0, 0.3f);
				DrawDebugSphere(TargetWorld, ApexMarker.Location, 16.0f, 16.0f, FColor::Cyan, false, 1.5f, 0, 0.3f);
			}
#endif
		}
	}
	else if (DistanceMatchingCurrentState != EDistanceMatchingType::Fall && CurrentMovementComponent->IsFalling() && CurrentMovementComponent->Velocity.Z < 0.0f)
	{
		DistanceMatchingCurrentState = EDistanceMatchingType::Fall;
		UAnimCharacterMovementLibrary::PredictLandingLocation(LandingMarker, MovementSnapshot, PredictionSnapshot, TargetWorld, bShowDebug, ActorsToIgnore, CurrentMovementComponent->CurrentFloor.FloorDist, 2.0f, 5.0f);
	}
	else if (!CurrentMovementComponent->IsFalling())
	{
		bSMStopRuleset = MovementSnapshot.bIsNotMovingOrAccelerating;

		// stop prediction, we have velocity but no acceleration
		if (bSMStopRuleset) // Alex: for accurate results we need to predict as long as the character is not fully standing still
		{
			// do iterative prediction for most accurate results
			StopMarker.Location = UAnimCharacterMovementLibrary::PredictGroundMovementStopLocation(MovementSnapshot, PredictionSnapshot) + CurrentCharacter->GetActorLocation();

			if (DistanceMatchingCurrentState != EDistanceMatchingType::Stop)
			{
				DistanceMatchingCurrentState = EDistanceMatchingType::Stop;
				// additional data
				StopMarker.Speed = MovementSnapshot.Speed2D;
				StopMarker.YawAngle = MovementSnapshot.VelocityYawAngle;
			}

#if ENABLE_DRAW_DEBUG
			if (bShowDebug)
			{
				DrawDebugSphere(TargetWorld, StopMarker.Location, 16.0f, 16.0f, FColor::Red, false, -1.0f, 0, 0.3f);
			}
#endif
		}

		// pivot prediction, we have a negative dot product between acceleration and velocity, we are also still moving
		//if (MovementSnapshot.Speed2D != 0.0f && (MovementSnapshot.WorldVelocity.GetSafeNormal() | MovementSnapshot.WorldAcceleration.GetSafeNormal()) <= -(MinPivotAngle / 180.0f)) // Alex for accurate results we need to predict as long as the character's dot product is negative
		if (MovementSnapshot.Speed2D != 0.0f && (MovementSnapshot.LocalVelocity.GetSafeNormal() | MovementSnapshot.LocalAcceleration.GetSafeNormal()) < 0.0f)
		{
			if(DistanceMatchingCurrentState != EDistanceMatchingType::Pivot)
			{
				DistanceMatchingCurrentState = EDistanceMatchingType::Pivot;

				// set pivoting cardinal direction snapshot
				PivotingCardinalDirSnapShot.CardinalDirection = MovementSnapshot.CurrentCardinalDirection;
				PivotingCardinalDirSnapShot.YawAngle = MovementSnapshot.VelocityYawAngle;
				PivotingCardinalDirSnapShot.YawAngleDeltaNorth = MovementSnapshot.VelocityYawDeltaNorth;
				PivotingCardinalDirSnapShot.YawAngleDeltaEast = MovementSnapshot.VelocityYawDeltaEast;
				PivotingCardinalDirSnapShot.YawAngleDeltaSouth = MovementSnapshot.VelocityYawDeltaSouth;
				PivotingCardinalDirSnapShot.YawAngleDeltaWest = MovementSnapshot.VelocityYawDeltaWest;

				// additional data
				PivotMarker.Speed = MovementSnapshot.Speed2D;
				PivotMarker.YawAngle = MovementSnapshot.VelocityYawAngle;
			}

#if ENABLE_DRAW_DEBUG
			if (bShowDebug)
			{
				DrawDebugSphere(TargetWorld, PivotMarker.Location, 16.0f, 16.0f, FColor::Purple, false, -1.0f, 0, 0.3f);
			}
#endif

			PivotMarker.Location = UAnimCharacterMovementLibrary::PredictGroundMovementPivotLocation(MovementSnapshot, PredictionSnapshot.GroundFriction) + CurrentCharacter->GetActorLocation();
		}

		bSMStartRuleset = MovementSnapshot.bIsMovingOrAccelerating; /* for the graph to prevent getting stuck do not check the current state, neesd to be updated all the time */

		if (bSMStartRuleset)
		{
			// start prediction, we have velocity and acceleration
			if (DistanceMatchingCurrentState != EDistanceMatchingType::Start && DistanceMatchingCurrentState != EDistanceMatchingType::Pivot)
			{
				DistanceMatchingCurrentState = EDistanceMatchingType::Start;

				StartMarker.Location = CurrentCharacter->GetActorLocation();
				StartMarker.Time = 0.0f;
				StartMarker.Distance = 0.0f;

				// additional data
				StartMarker.Speed = MovementSnapshot.AccelerationSize2D; // for start look at acceleration
				StartMarker.YawAngle = MovementSnapshot.VelocityYawAngle;

#if ENABLE_DRAW_DEBUG
				if (bShowDebug)
				{
					DrawDebugSphere(TargetWorld, StartMarker.Location, 16.0f, 16.0f, FColor::Blue, false, 1.5f, 0, 0.3f);
				}
#endif
			}
		}

		// we have no speed or acceleration, idle/standing still mode!
		if (MovementSnapshot.Speed2D == 0 && MovementSnapshot.AccelerationSize2D == 0.0f)
		{
			DistanceMatchingCurrentState = EDistanceMatchingType::None;
		}
	}

	// Update distance and time to marker
	switch (DistanceMatchingCurrentState)
	{
	case EDistanceMatchingType::Start:
		StartMarker.Distance = FMath::Clamp(FVector::Distance(CurrentCharacter->GetActorLocation(), StartMarker.Location), -MAX_MATCH_VALUE, MAX_MATCH_VALUE);
		//StartMarker.Time = FMath::Clamp(StartMarker.Time + DeltaTime, 0.0f, MAX_MATCH_VALUE);
		break;
	case EDistanceMatchingType::Stop:
		StopMarker.Distance = FMath::Clamp(FVector::Distance(CurrentCharacter->GetActorLocation(), StopMarker.Location), -MAX_MATCH_VALUE, MAX_MATCH_VALUE) * -1.0f;
		//StopMarker.Time = FMath::Clamp(StopMarker.Time - DeltaTime, 0.0f, MAX_MATCH_VALUE);
		break;
	case EDistanceMatchingType::Pivot:
		PivotMarker.Distance = FMath::Clamp(FVector::Distance(CurrentCharacter->GetActorLocation(), PivotMarker.Location), -MAX_MATCH_VALUE, MAX_MATCH_VALUE) * -1.0f;
		PivotMarker.DistanceAbsolute = FMath::Clamp(FVector::Distance(CurrentCharacter->GetActorLocation(), PivotMarker.Location), -MAX_MATCH_VALUE, MAX_MATCH_VALUE);
		//PivotMarker.Time = FMath::Clamp(PivotMarker.Time - DeltaTime, 0.0f, MAX_MATCH_VALUE);
		break;
	case EDistanceMatchingType::Jump:
		TakeOffMarker.Distance = FMath::Clamp(CurrentCharacter->GetActorLocation().Z - TakeOffMarker.Location.Z, 0.0f, ApexMarker.Location.Z - TakeOffMarker.Location.Z);
		ApexMarker.Distance = FMath::Clamp(ApexMarker.Location.Z - CurrentCharacter->GetActorLocation().Z, 0.0f, ApexMarker.Location.Z - TakeOffMarker.Location.Z) * -1.0f;
		//TakeOffMarker.Time = FMath::Clamp(TakeOffMarker.Time + DeltaTime, 0.0f, MAX_MATCH_VALUE);
		//ApexMarker.Time = FMath::Clamp(ApexMarker.Time - DeltaTime, 0.0f, MAX_MATCH_VALUE);
		break;
	case EDistanceMatchingType::Fall:
		ApexMarker.Distance = FMath::Clamp(ApexMarker.Location.Z - CurrentCharacter->GetActorLocation().Z, 0.0f, ApexMarker.Location.Z - LandingMarker.Location.Z);
		LandingMarker.Distance = FMath::Clamp((CurrentCharacter->GetActorLocation().Z - LandingMarker.Location.Z) * -1.0f, -ApexMarker.Location.Z, 0.0f);
		//ApexMarker.Time = FMath::Clamp(ApexMarker.Time + DeltaTime, 0.0f, MAX_MATCH_VALUE);
		//LandingMarker.Time = FMath::Clamp(LandingMarker.Time - DeltaTime, 0.0f, MAX_MATCH_VALUE);
		break;
	case EDistanceMatchingType::None:
		//StartMarker.Distance = 0.0f;
		StopMarker.Distance = 0.0f;
		//PivotMarker.Distance = 0.0f;
		TakeOffMarker.Distance = 0.0f;
		ApexMarker.Distance = 0.0f;
		LandingMarker.Distance = 0.0f;
		//StartMarker.Time = 0.0f;
		StopMarker.Time = 0.0f;
		PivotMarker.Time = 0.0f;
		TakeOffMarker.Time = 0.0f;
		ApexMarker.Time = 0.0f;
		LandingMarker.Time = 0.0f;
		break;
	}

#if ENABLE_DRAW_DEBUG
	if (bShowDebug)
	{
		if (MovementSnapshot.Speed2D != 0.0f)
		{
			DrawDebugLine(TargetWorld, LastActorLocation, CurrentCharacter->GetActorLocation(), FColor::Cyan, false, 1.5f, 0, 0.75f);
			LastActorLocation = CurrentCharacter->GetActorLocation();
		}

		if (bShowDebug)
		{
			DrawDebugSphere(TargetWorld, CurrentCharacter->GetActorLocation(), 16.0f, 16.0f, FColor::Green, false, -1.0f, 0, 0.3f);
		}

		// draw debug capsule
		DrawDebugCapsule(TargetWorld, CurrentCharacter->GetActorLocation(), PredictionSnapshot.CapsuleHalfHeight, PredictionSnapshot.CapsuleRadius, CurrentCharacter->GetActorRotation().Quaternion(), FColor::White, false, -1.0f, 0, 0.3f);
	}
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// original functions backported from UE5 5.3
USkeleton::AnimCurveUID UAnimDistanceMatchingLibrary::GetCurveUID(const UAnimSequence* InAnimSequence, FName CurveName)
{
	if (const USkeleton* Skeleton = InAnimSequence->GetSkeleton())
	{
		const FSmartNameMapping* CurveNameMapping = Skeleton->GetSmartNameContainer(USkeleton::AnimCurveMappingName);
		if (CurveNameMapping)
		{
			return CurveNameMapping->FindUID(CurveName);
		}
	}

	return SmartName::MaxUID;
}

float UAnimDistanceMatchingLibrary::GetDistanceRange(const UAnimSequence* InAnimSequence, USkeleton::AnimCurveUID CurveUID)
{
	FAnimCurveBufferAccess BufferCurveAccess(InAnimSequence, CurveUID);
	if (BufferCurveAccess.IsValid())
	{
		const int32 NumSamples = BufferCurveAccess.GetNumSamples();
		if (NumSamples >= 2)
		{
			return (BufferCurveAccess.GetValue(NumSamples - 1) - BufferCurveAccess.GetValue(0));
		}
	}
	return 0.f;
}

// Alex: ue4 version needs curve name to use type: USkeleton::AnimCurveUID
float UAnimDistanceMatchingLibrary::GetAnimPositionFromDistance(const UAnimSequence* InAnimSequence, const float& InDistance, USkeleton::AnimCurveUID InCurveName)
{
	FAnimCurveBufferAccess BufferCurveAccess(InAnimSequence, InCurveName);

	if (BufferCurveAccess.IsValid())
	{
		const int32 NumKeys = BufferCurveAccess.GetNumSamples();
		if (NumKeys < 2)
		{
			return 0.f;
		}

		// Some assumptions: 
		// - keys have unique values, so for a given value, it maps to a single position on the timeline of the animation.
		// - key values are sorted in increasing order.

		int32 First = 1;
		int32 Last = NumKeys - 1;
		int32 Count = Last - First;

		while (Count > 0)
		{
			int32 Step = Count / 2;
			int32 Middle = First + Step;

			if (InDistance > BufferCurveAccess.GetValue(Middle))
			{
				First = Middle + 1;
				Count -= Step + 1;
			}
			else
			{
				Count = Step;
			}
		}

		const float KeyAValue = BufferCurveAccess.GetValue(First - 1);
		const float KeyBValue = BufferCurveAccess.GetValue(First);
		const float Diff = KeyBValue - KeyAValue;
		const float Alpha = !FMath::IsNearlyZero(Diff) ? ((InDistance - KeyAValue) / Diff) : 0.f;

		const float KeyATime = BufferCurveAccess.GetTime(First - 1);
		const float KeyBTime = BufferCurveAccess.GetTime(First);
		return FMath::Lerp(KeyATime, KeyBTime, Alpha);
	}

	return 0.f;
}

/**
* Advance from the current time to a new time in the animation that will result in the desired distance traveled by the authored root motion.
*/
float UAnimDistanceMatchingLibrary::GetTimeAfterDistanceTraveled(UAnimSequence* AnimSequence, float CurrentTime, float DistanceTraveled, USkeleton::AnimCurveUID CurveName, const bool bAllowLooping)
{
	float NewTime = CurrentTime;

	if (AnimSequence == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid AnimSequence passed to GetTimeAfterDistanceTraveled"));
		return 0.0f;
	}

	// Avoid infinite loops if the animation doesn't cover any distance.
	if (!FMath::IsNearlyZero(GetDistanceRange(AnimSequence, CurveName)))
	{
		float AccumulatedDistance = 0.f;
		float AccumulatedTime = 0.f;

		const float SequenceLength = AnimSequence->GetPlayLength();
		static const float StepTime = 1.f / 30.f;

		// Traverse the distance curve, accumulating animated distance until the desired distance is reached.
		while ((AccumulatedDistance < DistanceTraveled) && (bAllowLooping || (NewTime + StepTime < SequenceLength)))
		{
			const float CurrentDistance = AnimSequence->EvaluateCurveData(CurveName, NewTime);
			const float DistanceAfterStep = AnimSequence->EvaluateCurveData(CurveName, NewTime + StepTime);

			const float AnimationDistanceThisStep = DistanceAfterStep - CurrentDistance;

			if (!FMath::IsNearlyZero(AnimationDistanceThisStep))
			{
				// Keep advancing if the desired distance hasn't been reached.
				if (AccumulatedDistance + AnimationDistanceThisStep < DistanceTraveled)
				{
					FAnimationRuntime::AdvanceTime(bAllowLooping, StepTime, NewTime, SequenceLength);
					AccumulatedDistance += AnimationDistanceThisStep;
				}
				// Once the desired distance is passed, find the approximate time between samples where the distance will be reached.
				else
				{
					const float DistanceAlpha = (DistanceTraveled - AccumulatedDistance) / AnimationDistanceThisStep;
					FAnimationRuntime::AdvanceTime(bAllowLooping, DistanceAlpha * StepTime, NewTime, SequenceLength);
					AccumulatedDistance = DistanceTraveled;
					break;
				}
			}
			else
			{
				NewTime += StepTime;
			}

			AccumulatedTime += StepTime;

			// If the animation doesn't cover enough distance, we abandon the algorithm to avoid an infinite loop.
			if (AccumulatedTime >= SequenceLength)
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to advance distance of (%.2f) after (%.2f) seconds on anim sequence (%s). Aborting."),
					DistanceTraveled, AccumulatedTime, *GetNameSafe(AnimSequence));
				break;
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Anim sequence (%s) is missing a distance curve or doesn't cover enough distance for GetTimeAfterDistanceTraveled."), *GetNameSafe(AnimSequence));
	}

	return NewTime;
}

/* lyra version, needs distance travelled per frame instead of actual distance of a location!!! */
void UAnimDistanceMatchingLibrary::AdvanceTimeByDistanceMatching(float DeltaTime, UAnimSequence* Sequence, float& ExplicitTime, const float& DistanceTraveled, FName InCurveName, FVector2D InPlayRateClamp)
{
	if (!Sequence)
	{
		return;
	}

	const USkeleton::AnimCurveUID CurveUID = GetCurveUID(Sequence, InCurveName);

	if (!Sequence->HasCurveData(CurveUID, false))
	{
		return;
	}

	if (DeltaTime > 0 && DistanceTraveled > 0)
	{
		const float CurrentTime = ExplicitTime;
		const float CurrentAssetLength = Sequence->GetPlayLength();
		const bool bAllowLooping = false;

		float TimeAfterDistanceTraveled = GetTimeAfterDistanceTraveled(Sequence, CurrentTime, DistanceTraveled, CurveUID, bAllowLooping);

		// Calculate the effective playrate that would result from advancing the animation by the distance traveled.
		// Account for the animation looping.
		if (TimeAfterDistanceTraveled < CurrentTime)
		{
			TimeAfterDistanceTraveled += CurrentAssetLength;
		}
		float EffectivePlayRate = (TimeAfterDistanceTraveled - CurrentTime) / DeltaTime;

		// Clamp the effective play rate.
		if (InPlayRateClamp.X >= 0.0f && InPlayRateClamp.X < InPlayRateClamp.Y)
		{
			EffectivePlayRate = FMath::Clamp(EffectivePlayRate, InPlayRateClamp.X, InPlayRateClamp.Y);
		}

		// Advance animation time by the effective play rate.
		float NewTime = CurrentTime;
		FAnimationRuntime::AdvanceTime(bAllowLooping, EffectivePlayRate * DeltaTime, NewTime, CurrentAssetLength);

		//
		ExplicitTime = NewTime;
	}
}

void UAnimDistanceMatchingLibrary::DistanceMatchToTarget(UAnimSequence* Sequence, float& ExplicitTime, const float& DistanceToTarget, FName CurDistanceCurveName)
{
	if (!Sequence)
	{
		return;
	}

	const USkeleton::AnimCurveUID CurveUID = GetCurveUID(Sequence, CurDistanceCurveName);

	if (!Sequence->HasCurveData(CurveUID, false))
	{
		return;
	}

	// By convention, distance curves store the distance to a target as a negative value.
	const float NewTime = GetAnimPositionFromDistance(Sequence, -DistanceToTarget, CurveUID);
	ExplicitTime = NewTime;
}

void UAnimDistanceMatchingLibrary::AdvanceTimeNaturally(float DeltaSeconds, UAnimSequence* Sequence, float& ExplicitTime)
{
	if (!Sequence)
	{
		return;
	}

	float SequenceLength = Sequence->GetPlayLength();
	bool bAllowLooping = false;
	float NewTime = ExplicitTime;
	FAnimationRuntime::AdvanceTime(bAllowLooping, DeltaSeconds, NewTime, SequenceLength);
	ExplicitTime = NewTime;
}

FVector UAnimDistanceMatchingLibrary::PredictGroundMovementStopLocation(const FVector& WorldVelocity, const bool& bUseSeparateBrakingFriction, const float& BrakingFriction, const float& GroundFriction, const float& BrakingFrictionFactor, const float& BrakingDecelerationWalking)
{
	FVector PredictedStopLocation = FVector::ZeroVector;

	float ActualBrakingFriction = (bUseSeparateBrakingFriction ? BrakingFriction : GroundFriction);
	const float FrictionFactor = FMath::Max(0.f, BrakingFrictionFactor);
	ActualBrakingFriction = FMath::Max(0.f, ActualBrakingFriction * FrictionFactor);
	float BrakingDeceleration = FMath::Max(0.f, BrakingDecelerationWalking);

	const FVector WorldVelocity2D = WorldVelocity * FVector(1.f, 1.f, 0.f);
	FVector WorldVelocityDir2D;
	float Speed2D;
	WorldVelocity2D.ToDirectionAndLength(WorldVelocityDir2D, Speed2D);

	const float Divisor = ActualBrakingFriction * Speed2D + BrakingDeceleration;
	if (Divisor > 0.f)
	{
		const float TimeToStop = Speed2D / Divisor;
		PredictedStopLocation = WorldVelocity2D * TimeToStop + 0.5f * ((-ActualBrakingFriction) * WorldVelocity2D - BrakingDeceleration * WorldVelocityDir2D) * TimeToStop * TimeToStop;
	}

	return PredictedStopLocation;
}

FVector UAnimDistanceMatchingLibrary::PredictGroundMovementPivotLocation(const FVector& WorldAcceleration, const FVector& WorldVelocity, const float& GroundFriction)
{
	FVector PredictedPivotLocation = FVector::ZeroVector;

	const FVector Acceleration2D = WorldAcceleration * FVector(1.f, 1.f, 0.f);

	FVector AccelerationDir2D;
	float AccelerationSize2D;
	Acceleration2D.ToDirectionAndLength(AccelerationDir2D, AccelerationSize2D);

	const float VelocityAlongAcceleration = (WorldVelocity | AccelerationDir2D);
	if (VelocityAlongAcceleration < 0.0f)
	{
		const float SpeedAlongAcceleration = -VelocityAlongAcceleration;
		const float Divisor = AccelerationSize2D + 2.f * SpeedAlongAcceleration * GroundFriction;
		const float TimeToDirectionChange = SpeedAlongAcceleration / Divisor;

		const FVector AccelerationForce = WorldAcceleration -
			(WorldVelocity - AccelerationDir2D * WorldVelocity.Size2D()) * GroundFriction;

		PredictedPivotLocation = WorldVelocity * TimeToDirectionChange + 0.5f * AccelerationForce * TimeToDirectionChange * TimeToDirectionChange;
	}

	return PredictedPivotLocation;
}

bool UAnimDistanceMatchingLibrary::ShouldDistanceMatchStop(const FTransform& WorldTransform, const FVector& WorldVelocity, const FVector& WorldAcceleration)
{
	float LocalVelocity2D = WorldTransform.InverseTransformVectorNoScale(WorldVelocity).SizeSquared2D();
	float LocalAcceleration2D = WorldTransform.InverseTransformVectorNoScale(WorldAcceleration).SizeSquared2D();

	//UE_LOG(LogTemp, Warning, TEXT("LocalVelocity2D: %f."), LocalVelocity2D);
	//UE_LOG(LogTemp, Warning, TEXT("LocalAcceleration2D: %f."), LocalAcceleration2D);

	bool bHasVelocity = !FMath::IsNearlyEqual(LocalVelocity2D, 0.0f);
	bool bHasAcceleration = !FMath::IsNearlyEqual(LocalAcceleration2D, 0.0f);

	//UE_LOG(LogTemp, Warning, TEXT("bHasVelocity: %s."), bHasVelocity ? TEXT("True") : TEXT("False"));
	//UE_LOG(LogTemp, Warning, TEXT("bHasAcceleration: %s."), bHasAcceleration ? TEXT("True") : TEXT("False"));

	if (bHasVelocity)// && !bHasAcceleration)
	{
		return true;
	}
	else
	{
		return false;
	}
}

float UAnimDistanceMatchingLibrary::GetTimeFromDistance(UAnimSequence* Sequence, float Distance, FName CurveName)
{
	const USkeleton::AnimCurveUID CurveUID = GetCurveUID(Sequence, CurveName);
	if (!Sequence->HasCurveData(CurveUID, false))
	{
		return 0.0f;
	}

	// By convention, distance curves store the distance to a target as a negative value.
	return GetAnimPositionFromDistance(Sequence, -Distance, CurveUID);
}

bool UAnimDistanceMatchingLibrary::AccelerationOpposesVelocity(const FTransform& WorldTransform, const FVector& WorldVelocity, const FVector& WorldAcceleration)
{
	FVector LocalVelocity2D = WorldTransform.InverseTransformVectorNoScale(WorldVelocity * FVector(1.0f, 1.0f, 0.0f));
	FVector LocalAcceleration2D = WorldTransform.InverseTransformVectorNoScale(WorldAcceleration * FVector(1.0f, 1.0f, 0.0f));

	if ((LocalVelocity2D | LocalAcceleration2D) < 0.0f)
	{
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

float UAnimDistanceMatchingLibrary::GetRelevantAnimTimeRemainingFraction(UAnimInstance* AnimInstance, FName StateMachineName, FName StateName)
{
	if (!AnimInstance)
	{
		return 0.0f;
	}

	/* find the state machine index first */
	int32 MachineIndex = AnimInstance->GetStateMachineIndex(StateMachineName);
	if (MachineIndex == INDEX_NONE)
	{
		return 0.0f;
	}

	const FBakedAnimationStateMachine* MachinePtr = AnimInstance->GetStateMachineInstanceDesc(StateMachineName);
	if (!MachinePtr)
	{
		return 0.0f;
	}

	int32 StateIndex = MachinePtr->FindStateIndex(StateName);
	if (StateIndex == INDEX_NONE)
	{
		return 0.0f;
	}

	return AnimInstance->GetRelevantAnimTimeRemainingFraction(MachineIndex, StateIndex);
}

float UAnimDistanceMatchingLibrary::GetRelevantAnimTime(UAnimInstance* AnimInstance, FName StateMachineName, FName StateName)
{
	if (!AnimInstance)
	{
		return 0.0f;
	}

	/* find the state machine index first */
	int32 MachineIndex = AnimInstance->GetStateMachineIndex(StateMachineName);
	if (MachineIndex == INDEX_NONE)
	{
		return 0.0f;
	}

	const FBakedAnimationStateMachine* MachinePtr = AnimInstance->GetStateMachineInstanceDesc(StateMachineName);
	if (!MachinePtr)
	{
		return 0.0f;
	}

	int32 StateIndex = MachinePtr->FindStateIndex(StateName);
	if (StateIndex == INDEX_NONE)
	{
		return 0.0f;
	}

	return AnimInstance->GetRelevantAnimTime(MachineIndex, StateIndex);
}

float UAnimDistanceMatchingLibrary::GetStateTransitionCrossfadeDuration(UAnimInstance* AnimInstance, FName StateMachineName, FName FromStateName, FName ToStateName)
{
	if (!AnimInstance)
	{
		return 0.0f;
	}

	/* find the state machine index first */
	int32 MachineIndex = AnimInstance->GetStateMachineIndex(StateMachineName);
	if (MachineIndex == INDEX_NONE)
	{
		return 0.0f;
	}

	const FBakedAnimationStateMachine* MachinePtr = AnimInstance->GetStateMachineInstanceDesc(StateMachineName);
	if (!MachinePtr)
	{
		return 0.0f;
	}

	int32 FromStateIndex = MachinePtr->FindStateIndex(FromStateName);
	if (FromStateIndex == INDEX_NONE)
	{
		return 0.0f;
	}
	int32 ToStateIndex = MachinePtr->FindStateIndex(ToStateName);
	if (ToStateIndex == INDEX_NONE)
	{
		return 0.0f;
	}

	int32 TransitionIndex = MachinePtr->FindTransitionIndex(FromStateIndex, ToStateIndex);
	if (TransitionIndex == INDEX_NONE)
	{
		return 0.0f;
	}

	return AnimInstance->GetInstanceTransitionCrossfadeDuration(MachineIndex, TransitionIndex);
}

bool UAnimDistanceMatchingLibrary::ShouldPerformAutomaticStateTransition(float CrossfadeDuration, float SequenceLength, float SequenceCurrentTime)
{
	// if crossfade duration is 0.0 then dont bother
	if (CrossfadeDuration == 0.0f)
	{
		return false;
	}

	const float AnimTimeRemaining = SequenceLength - SequenceCurrentTime;
	return (AnimTimeRemaining <= CrossfadeDuration);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

FTransform UAnimDistanceMatchingLibrary::ExtractStartingRootMotionFromSequence(UAnimSequence* AnimSequence)
{
	// Assuming AnimSequence is valid
	if (!AnimSequence)
	{
		return FTransform(); // Return identity transform if the sequence is not valid
	}

	return AnimSequence->ExtractRootMotionFromRange(0.0f, KINDA_SMALL_NUMBER);
}

FTransform UAnimDistanceMatchingLibrary::ExtractEndingRootMotionFromSequence(UAnimSequence* AnimSequence)
{
	// Assuming AnimSequence is valid
	if (!AnimSequence)
	{
		return FTransform(); // Return identity transform if the sequence is not valid
	}

	float AnimLength = AnimSequence->GetPlayLength();
	return AnimSequence->ExtractRootMotionFromRange(AnimLength - KINDA_SMALL_NUMBER, AnimLength);
}

float UAnimDistanceMatchingLibrary::CalculateAnimScoreForDirection(const FTransform& RootMotionTransform, const FVector& Direction)
{
	FVector LastFrameDirection = RootMotionTransform.GetTranslation().GetSafeNormal();

	// Create a rotation representing the -90.0 degrees about the Z-axis
	FQuat RotationAdjustment = FQuat(FVector(0.0f, 0.0f, 1.0f), -FMath::DegreesToRadians(90.0f));

	// Apply the rotation to the last frame direction
	FVector AdjustedDirection = RotationAdjustment.RotateVector(LastFrameDirection);

	// Calculate angular difference
	float CosTheta = FMath::Clamp(FVector::DotProduct(Direction, AdjustedDirection), -1.0f, 1.0f);
	float AngleDifference = FMath::Acos(CosTheta); // In radians

	return AngleDifference;
}

UAnimSequence* UAnimDistanceMatchingLibrary::FindBestDirectionAnimation(const TArray<UAnimSequence*>& AnimSequences, const FVector& Direction, bool bUseStartingRootMotion)
{
	UAnimSequence* BestFitSequence = nullptr;
	float BestFitValue = FLT_MAX; // Initializing with a high value

	for (UAnimSequence* Sequence : AnimSequences)
	{
		// for stopping use starting root motion
		FTransform RootMotionTransform = bUseStartingRootMotion ? ExtractStartingRootMotionFromSequence(Sequence) : ExtractEndingRootMotionFromSequence(Sequence);
		float CurrentFitValue = CalculateAnimScoreForDirection(RootMotionTransform, Direction);

		if (CurrentFitValue < BestFitValue)
		{
			BestFitValue = CurrentFitValue;
			BestFitSequence = Sequence;
		}
	}

	return BestFitSequence;
}

float UAnimDistanceMatchingLibrary::CalculateAnimationSpeed(UAnimSequence* AnimSequence)
{
	// Extract the total root motion from the animation
	FTransform TotalRootMotion = AnimSequence->ExtractRootMotionFromRange(0.0f, AnimSequence->GetPlayLength());
	// Calculate the distance covered by the animation
	float Distance = TotalRootMotion.GetTranslation().Size();
	// Return speed = distance / duration
	return Distance / AnimSequence->GetPlayLength();
}