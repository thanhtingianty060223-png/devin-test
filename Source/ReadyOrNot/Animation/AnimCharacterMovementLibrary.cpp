// Copyright Epic Games, Inc. All Rights Reserved.

#include "AnimCharacterMovementLibrary.h"
#include "Animation/AnimSequenceBase.h"


#include "GameFramework/Character.h"

#include "KismetAnimationLibrary.h"

// include draw debug helpers header file
#include "DrawDebugHelpers.h"
#include "Kismet/KismetSystemLibrary.h"
#include "DisplayDebugHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogAnimCharacterMovementLibrary, Verbose, All);


float UAnimCharacterMovementLibrary::CalculateDirectionInput(const FVector& Velocity, const FRotator& BaseRotation)
{
	if (!Velocity.IsNearlyZero())
	{
		FMatrix RotMatrix = FRotationMatrix(BaseRotation);
		FVector ForwardVector = RotMatrix.GetScaledAxis(EAxis::X);
		FVector RightVector = RotMatrix.GetScaledAxis(EAxis::Y);
		FVector NormalizedVel = Velocity.GetSafeNormal2D();

		// get a cos(alpha) of forward vector vs velocity
		float ForwardCosAngle = FVector::DotProduct(ForwardVector, NormalizedVel);
		// now get the alpha and convert to degree
		float ForwardDeltaDegree = FMath::RadiansToDegrees(FMath::Acos(ForwardCosAngle));

		// depending on where right vector is, flip it
		float RightCosAngle = FVector::DotProduct(RightVector, NormalizedVel);
		if (RightCosAngle < 0)
		{
			ForwardDeltaDegree *= -1;
		}

		return ForwardDeltaDegree;
	}

	return 0.f;
}

void UAnimCharacterMovementLibrary::UpdateCharacterMovementSnapshot(const FTransform& WorldTransform, const FVector& WorldVelocity, const FVector& WorldAcceleration, bool bIsOnGround,
	float RootYawOffset, UPARAM(ref) FAnimCharacterMovementSnapshot& Snapshot)
{
	// Position

	const FVector WorldLocation = WorldTransform.GetLocation();
	Snapshot.Distance2DTraveledSinceLastUpdate = FVector::Dist2D(Snapshot.WorldLocation, WorldLocation);
	Snapshot.WorldLocation = WorldLocation;

	// Velocity

	Snapshot.WorldVelocity = WorldVelocity;
	Snapshot.LocalVelocity = WorldTransform.InverseTransformVectorNoScale(Snapshot.WorldVelocity);
	Snapshot.Speed2D = Snapshot.WorldVelocity.Size2D();

	// Acceleration

	Snapshot.WorldAcceleration = WorldAcceleration;
	Snapshot.LocalAcceleration = WorldTransform.InverseTransformVectorNoScale(Snapshot.WorldAcceleration);
	Snapshot.AccelerationSize2D = Snapshot.WorldAcceleration.Size2D();

	// Movement angle

	const FRotator Rotation = WorldTransform.GetRotation().Rotator();

	if (FMath::IsNearlyZero(Snapshot.Speed2D))
	{
		Snapshot.VelocityYawAngle = 0.0f;
		Snapshot.AccelerationYawAngle = 0.0f;

		Snapshot.LocalVelocityDirection = FVector::ZeroVector;
		Snapshot.LocalAccelerationDirection = FVector::ZeroVector;
	}
	else
	{
		Snapshot.VelocityYawAngle = CalculateDirectionInput(Snapshot.WorldVelocity, Rotation);
		Snapshot.VelocityYawAngle = FRotator::NormalizeAxis(Snapshot.VelocityYawAngle - RootYawOffset);

		// unit direction vector here
		Snapshot.LocalVelocityDirection = FRotator(0.0f, Snapshot.VelocityYawAngle, 0.0f).Vector();

		Snapshot.AccelerationYawAngle = CalculateDirectionInput(Snapshot.WorldAcceleration, Rotation);
		Snapshot.AccelerationYawAngle = FRotator::NormalizeAxis(Snapshot.AccelerationYawAngle - RootYawOffset);

		// unit direction vector here
		Snapshot.LocalAccelerationDirection = FRotator(0.0f, Snapshot.AccelerationYawAngle, 0.0f).Vector();

		// calculate cardinal direction
		Snapshot.CurrentCardinalDirection = GetCardinalDirectionFromAngle(Snapshot.CurrentCardinalDirection, Snapshot.VelocityYawAngle, 0.5f);

		// used for orientation warping!
		Snapshot.VelocityYawDeltaNorth = CalculateDirAngle(Snapshot.WorldVelocity, Rotation, FRotator(0, 0, 0), -45.0f, 45.0f, Snapshot.VelocityYawDeltaNorth);
		Snapshot.VelocityYawDeltaEast = CalculateDirAngle(Snapshot.WorldVelocity, Rotation, FRotator(0, 90, 0), -45.0f, 45.0f, Snapshot.VelocityYawDeltaEast);
		Snapshot.VelocityYawDeltaWest = CalculateDirAngle(Snapshot.WorldVelocity, Rotation, FRotator(0, -90, 0), -45.0f, 45.0f, Snapshot.VelocityYawDeltaWest);
		Snapshot.VelocityYawDeltaSouth = CalculateDirAngle(Snapshot.WorldVelocity, Rotation, FRotator(0, 180, 0), -45.0f, 45.0f, Snapshot.VelocityYawDeltaSouth);

		const FRotator Rot90(0.f, 90.f, 0.f);
		const FRotator RotNeg90(0.f, -90.f, 0.f);
		const FRotator Rot180(0.f, 180.f, 0.f);

		//if(Snapshot.VelocityYawAngle <= 0.f)
			//Rot180 = FRotator(0.f, -180.f, 0.f);

		//Snapshot.VelocityYawDeltaFwd = FMath::ClampAngle(Snapshot.VelocityYawAngle, -45.0f, 45.0f);
		//Snapshot.VelocityYawDeltaRight = FMath::ClampAngle(CalculateDirectionInput(Snapshot.WorldVelocity, FRotator(FQuat(Rotation) * FQuat(Rot90))), -45.0f, 45.0f);
		//Snapshot.VelocityYawDeltaLeft = FMath::ClampAngle(CalculateDirectionInput(Snapshot.WorldVelocity, FRotator(FQuat(Rotation) * FQuat(RotNeg90))), -45.0f, 45.0f);
		//Snapshot.VelocityYawDeltaBwd = FMath::ClampAngle(CalculateDirectionInput(Snapshot.WorldVelocity, FRotator(FQuat(Rotation) * FQuat(Rot180))), -45.0f, 45.0f);
	}

	// Movement state
	Snapshot.bIsOnGround = bIsOnGround;

	Snapshot.bIsMoving = Snapshot.Speed2D != 0 ? true : false;
	Snapshot.bIsAccelerating = Snapshot.AccelerationSize2D != 0 ? true : false;

	Snapshot.bIsMovingAndAccelerating = (Snapshot.bIsAccelerating && Snapshot.bIsMoving) ? true : false;
	Snapshot.bIsMovingOrAccelerating = (Snapshot.bIsAccelerating || Snapshot.bIsMoving) ? true : false;
	Snapshot.bIsNotMovingOrAccelerating = (!Snapshot.bIsAccelerating || !Snapshot.bIsMoving) ? true : false;
	Snapshot.bIsNotMoving = (!Snapshot.bIsMoving) ? true : false;
	Snapshot.bIsNotAccelerating = (!Snapshot.bIsAccelerating) ? true : false;
	Snapshot.bIsNotMovingAndAccelerating = (!Snapshot.bIsMoving && Snapshot.bIsAccelerating) ? true : false;
	Snapshot.bIsMovingAndNotAccelerating = (Snapshot.bIsMoving && !Snapshot.bIsAccelerating) ? true : false;

	Snapshot.bAccelerationOpposesVelocity = (Snapshot.LocalVelocity.GetSafeNormal() | Snapshot.LocalAcceleration.GetSafeNormal()) < 0.0f;
	Snapshot.bAccelerationEqualsVelocity = (Snapshot.LocalVelocity.GetSafeNormal() | Snapshot.LocalAcceleration.GetSafeNormal()) > 0.0f;

	// last values
	Snapshot.LastWorldLocation = WorldLocation;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	FString DebugString = FString::Printf(TEXT(
		"WorldVelocity(%s) | LocalVelocity(%s) | Speed2D(%.3f) | Distance2DTraveledSinceLastUpdate(%.3f)\n"
		"WorldAcceleration(%s) | LocalAcceleration(%s) | AccelerationSize2D(%.3f)\n"
		"VelocityYawAngle(%.3f) | AccelerationYawAngle(%.3f)\n"
		"bIsOnGround(%d)\n"),
		*Snapshot.WorldVelocity.ToString(), *Snapshot.LocalVelocity.ToString(), Snapshot.Speed2D, Snapshot.Distance2DTraveledSinceLastUpdate,
		*Snapshot.WorldAcceleration.ToString(), *Snapshot.LocalAcceleration.ToString(), Snapshot.AccelerationSize2D,
		Snapshot.VelocityYawAngle, Snapshot.AccelerationYawAngle,
		Snapshot.bIsOnGround);

	UE_LOG(LogAnimCharacterMovementLibrary, VeryVerbose, TEXT("%s"), *DebugString);
#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
}

EAnimCardinalDirection UAnimCharacterMovementLibrary::GetCardinalDirectionFromAngle(EAnimCardinalDirection PreviousCardinalDirection, float AngleInDegrees, float DeadZoneAngle)
{
	// Use deadzone offset to favor backpedal on S & W, and favor frontpedal on N & E
	const float AbsoluteAngle = FMath::Abs(AngleInDegrees);
	if (PreviousCardinalDirection == EAnimCardinalDirection::North)
	{
		if (AbsoluteAngle <= (45.f + DeadZoneAngle + DeadZoneAngle))
		{
			return EAnimCardinalDirection::North;
		}
		else if (AbsoluteAngle >= 135.f - DeadZoneAngle)
		{
			return EAnimCardinalDirection::South;
		}
		else if (AngleInDegrees > 0.f)
		{
			return EAnimCardinalDirection::East;
		}
		return EAnimCardinalDirection::West;
	}
	else if (PreviousCardinalDirection == EAnimCardinalDirection::South)
	{
		if (AbsoluteAngle <= 45.f + DeadZoneAngle)
		{
			return EAnimCardinalDirection::North;
		}
		else if (AbsoluteAngle >= (135.f - DeadZoneAngle - DeadZoneAngle))
		{
			return EAnimCardinalDirection::South;
		}
		else if (AngleInDegrees > 0.f)
		{
			return EAnimCardinalDirection::East;
		}
		return EAnimCardinalDirection::West;
	}

	// East and West
	if (AbsoluteAngle <= (45.f + DeadZoneAngle))
	{
		return EAnimCardinalDirection::North;
	}
	else if (AbsoluteAngle >= (135.f - DeadZoneAngle))
	{
		return EAnimCardinalDirection::South;
	}
	else if (AngleInDegrees > 0.f)
	{
		return EAnimCardinalDirection::East;
	}
	return EAnimCardinalDirection::West;
}

const UAnimSequence* UAnimCharacterMovementLibrary::SelectAnimForCardinalDirection(EAnimCardinalDirection CardinalDirection, const FCardinalDirectionAnimSet& AnimSet)
{
	switch (CardinalDirection)
	{
	case EAnimCardinalDirection::North:
		return AnimSet.NorthAnim;
	case EAnimCardinalDirection::East:
		return AnimSet.EastAnim;
	case EAnimCardinalDirection::South:
		return AnimSet.SouthAnim;
	case EAnimCardinalDirection::West:
		return AnimSet.WestAnim;
	default:
		checkNoEntry();
		return nullptr;
	}

	return nullptr;
}

FVector UAnimCharacterMovementLibrary::PredictGroundMovementStopLocation(const FAnimCharacterMovementSnapshot& MovementSnapshot, const FAnimCharacterMovementPredictionSnapshot& PredictionSnapshot)
{
	FVector PredictedStopLocation = FVector::ZeroVector;

	float ActualBrakingFriction = (PredictionSnapshot.bUseSeparateBrakingFriction ? PredictionSnapshot.BrakingFriction : PredictionSnapshot.GroundFriction);
	const float FrictionFactor = FMath::Max(0.f, PredictionSnapshot.BrakingFrictionFactor);
	ActualBrakingFriction = FMath::Max(0.f, ActualBrakingFriction * FrictionFactor);
	float BrakingDeceleration = FMath::Max(0.f, PredictionSnapshot.BrakingDecelerationWalking);

	const FVector WorldVelocity2D = MovementSnapshot.WorldVelocity * FVector(1.f, 1.f, 0.f);
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

FVector UAnimCharacterMovementLibrary::PredictGroundMovementPivotLocation(const FAnimCharacterMovementSnapshot& MovementSnapshot, float GroundFriction)
{
	FVector PredictedPivotLocation = FVector::ZeroVector;

	const FVector WorldAcceleration2D = MovementSnapshot.WorldAcceleration * FVector(1.f, 1.f, 0.f);

	FVector WorldAccelerationDir2D;
	float WorldAccelerationSize2D;
	WorldAcceleration2D.ToDirectionAndLength(WorldAccelerationDir2D, WorldAccelerationSize2D);

	const float VelocityAlongAcceleration = (MovementSnapshot.WorldVelocity | WorldAccelerationDir2D);
	if (VelocityAlongAcceleration < 0.0f)
	{
		const float SpeedAlongAcceleration = -VelocityAlongAcceleration;
		const float Divisor = WorldAccelerationSize2D + 2.f * SpeedAlongAcceleration * GroundFriction;
		const float TimeToDirectionChange = SpeedAlongAcceleration / Divisor;

		const FVector AccelerationForce = MovementSnapshot.WorldAcceleration -
			(MovementSnapshot.WorldVelocity - WorldAccelerationDir2D * MovementSnapshot.Speed2D) * GroundFriction;

		PredictedPivotLocation = MovementSnapshot.WorldVelocity * TimeToDirectionChange + 0.5f * AccelerationForce * TimeToDirectionChange * TimeToDirectionChange;
	}

	return PredictedPivotLocation;
}

/* pass rc->ReplicatedControlRotation.Vector()??? */
/*
Calculate the current direction angle from actor velocity and rotation
*/
float UAnimCharacterMovementLibrary::CalculateDirAngle(FVector CurVel, FRotator CurActorRotation, FRotator DirRotation, float ClampMin, float ClampMax, float CurDirAngle)
{
	float TargetDirAngle = 0.f;
	FVector	VelDir = CurVel;
	VelDir.Z = 0.0f;

	if (VelDir.IsNearlyZero())
	{
		TargetDirAngle = 0.f;
	}
	else
	{
		VelDir = VelDir.GetSafeNormal();
		// rotate into the given direction to produce the desired result
		// it makes sure the rotation becomes the actual forward direction so it returns values from zero
		FVector LookDir = DirRotation.RotateVector(CurActorRotation.Vector());// DirRotation.RotateVector(CurActorRotation.Vector()));

		LookDir.Z = 0.f;
		LookDir = LookDir.GetSafeNormal();

		FVector LeftDir = LookDir ^ FVector(0.f, 0.f, 1.f);
		LeftDir = LeftDir.GetSafeNormal();

		float ForwardPct = (LookDir | VelDir);
		float LeftPct = (LeftDir | VelDir);

		TargetDirAngle = FMath::Acos(ForwardPct);
		if (LeftPct > 0.f)
		{
			TargetDirAngle *= -1.f;
		}
	}

	// Move CurDirAngle towards TargetDirAngle as fast as DirRadsPerSecond allows
	float DeltaDir = FMath::FindDeltaAngleRadians(CurDirAngle, TargetDirAngle);

	float Final = FMath::UnwindRadians(CurDirAngle + DeltaDir);
	return FMath::ClampAngle(FMath::RadiansToDegrees(Final), ClampMin, ClampMax);
}

/* Jump Prediction related */
void UAnimCharacterMovementLibrary::PredictJumpPath(FPredictionResult& PredictResult, const FAnimCharacterMovementSnapshot& MovementSnapshot, const FAnimCharacterMovementPredictionSnapshot& PredictionSnapshot, UWorld* TargetWorld, bool bDrawDebugTrace, TArray<AActor*> ActorsToIgnore, const float SimulationTime, const float SimulationFrequency)
{
	const float SubstepDeltaTime = 1.0f / SimulationFrequency;

	FVector CurrentVelocity = MovementSnapshot.WorldVelocity;
	FVector TraceStart = MovementSnapshot.WorldLocation;
	FVector TraceEnd = TraceStart;
	float CurrentTime = 0.0f;

	while (CurrentTime < SimulationTime)
	{
		// Limit step to not go further than total time.
		const float PreviousTime = CurrentTime;
		const float ActualStepDeltaTime = FMath::Min(SimulationTime - CurrentTime, SubstepDeltaTime);
		CurrentTime += ActualStepDeltaTime;

		// Integrate (Velocity Verlet method)
		TraceStart = TraceEnd;
		FVector PreviousVelocity = CurrentVelocity;
		CurrentVelocity = PreviousVelocity + FVector(0.f, 0.f, PredictionSnapshot.GravityZ * ActualStepDeltaTime);
		TraceEnd = TraceStart + (PreviousVelocity + CurrentVelocity) * (0.5f * ActualStepDeltaTime);

		FHitResult HitResult;
		const EDrawDebugTrace::Type DrawDebugTrace = bDrawDebugTrace ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None;

		float TraceDrawTime = 2.0f;
		bool bHit = UKismetSystemLibrary::CapsuleTraceSingle(TargetWorld, TraceStart, TraceEnd, PredictionSnapshot.CapsuleRadius, PredictionSnapshot.CapsuleHalfHeight, TraceTypeQuery1, false, ActorsToIgnore, EDrawDebugTrace::None, HitResult, true, FLinearColor::Red, FLinearColor::Green, TraceDrawTime);

		if (bHit)
		{
			PredictResult.Location = HitResult.Location;
			PredictResult.Time = PreviousTime + ActualStepDeltaTime * HitResult.Time;

			return;
		}
	}

	PredictResult.Location = TraceEnd;
	PredictResult.Time = SimulationTime;
}

void UAnimCharacterMovementLibrary::PredictJumpApex(FPredictionResult& PredictResult, const FAnimCharacterMovementSnapshot& MovementSnapshot, const FAnimCharacterMovementPredictionSnapshot& PredictionSnapshot, UWorld* TargetWorld, bool bDrawDebugTrace, TArray<AActor*> ActorsToIgnore, float ApexSimulationFrequency)
{
	// Velocity * Sin jump angle / Gravity
	const float MaxTimeToApex = MovementSnapshot.WorldVelocity.Size() * MovementSnapshot.WorldVelocity.GetSafeNormal().Z / FMath::Abs(PredictionSnapshot.GravityZ);
	PredictJumpPath(PredictResult, MovementSnapshot, PredictionSnapshot, TargetWorld, bDrawDebugTrace, ActorsToIgnore, MaxTimeToApex, ApexSimulationFrequency);
}

void UAnimCharacterMovementLibrary::PredictLandingLocation(FPredictionResult& PredictResult, const FAnimCharacterMovementSnapshot& MovementSnapshot, const FAnimCharacterMovementPredictionSnapshot& PredictionSnapshot, UWorld* TargetWorld, bool bDrawDebugTrace, TArray<AActor*> ActorsToIgnore, float DistanceToFloor, float MaxSimulationTime, float LandingSimulationFrequency)
{
	PredictJumpPath(PredictResult, MovementSnapshot, PredictionSnapshot, TargetWorld, bDrawDebugTrace, ActorsToIgnore, MaxSimulationTime, LandingSimulationFrequency);

	PredictResult.Location += FVector(0.0f, 0.0f, DistanceToFloor);
}