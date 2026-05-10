// Void Interactive, 2022
// Author: Alexander Mijalkovski

// Base Anim instance for anything global that all sub-classes will share

#include "Animation/ReadyOrNotAnimInstance.h"
#include "ReadyOrNot.h"
#include "ReadyOrNotCharacter.h"
#include "Characters/AI/SuspectCharacter.h"
#include "Runtime/Engine/Classes/Kismet/KismetMathLibrary.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimCurveCompressionCodec_UniformIndexable.h"
#include "Actors/Items/C2Explosive.h"

DECLARE_CYCLE_STAT_EXTERN(TEXT("RoN_Anim_NativeUpdate"), STAT_NativeRoNAnimUpdate, STATGROUP_Anim,);
DEFINE_STAT(STAT_NativeRoNAnimUpdate);

#define printfloat(variable)                if (GEngine) //GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Cyan, FString::Printf(TEXT(#variable ": %f"), variable), false)

UReadyOrNotAnimInstance::UReadyOrNotAnimInstance(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	//set any default values for your variables here
	//DirectionUpdateRate = 0.3f;

	MovementJogThreshold = 180.0f;
	JumpRecoveryTime = 0.3f;
}

void UReadyOrNotAnimInstance::NativeInitializeAnimation()
{
}

// main func for tick calculations
void UReadyOrNotAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	SCOPE_CYCLE_COUNTER(STAT_NativeRoNAnimUpdate);

	Super::NativeUpdateAnimation(DeltaSeconds);

	AReadyOrNotCharacter* CurChar = Cast<AReadyOrNotCharacter>(TryGetAnyOwner());

	// fallback just incase
	if (CurChar == nullptr)
	{
		return;
	}

	CarryArrestedAnimState = CurChar->Rep_CarryArrestedAnimState;
	bIsCarried = CurChar->IsCarried();
	bIsCarrying = CurChar->IsCarrying() || CurChar->IsDropping();
	
	HeadLeadAmount = FMath::FInterpTo(HeadLeadAmount, FMath::Abs(FMath::Cos(GetWorld()->GetTimeSeconds())), DeltaSeconds, 10.0f);

	if (UCharacterMovementComponent* CharacterMovementComponent = CurChar->GetCharacterMovement())
	{
		Velocity = CurChar->GetVelocity();
		Speed = CurChar->GetVelocity().Size2D();
		SpeedHorizontal = FVector2D(Velocity.X, Velocity.Y).Size();
		SpeedVertical = Velocity.Z;
		bIsInAir = CharacterMovementComponent->IsFalling();
		bIsMoving = Speed > 1.0f;
		MaxSpeed = CharacterMovementComponent->GetMaxSpeed();
		ActorTransform = CurChar->GetActorTransform();
		VelocityLocalNormalized = UKismetMathLibrary::InverseTransformDirection(ActorTransform, Velocity);
		VelocityLocalNormalized.Normalize();
		Direction = FMath::RadiansToDegrees(FMath::Atan2(VelocityLocalNormalized.Y, VelocityLocalNormalized.X));
		HeadLookRotation = UKismetMathLibrary::RInterpTo(HeadLookRotation, GetLookAtRotation(), DeltaSeconds, 1.0f);

		/*
		// calculate proper current direction
		ForwardDirAngle = CalculateDirAngle(DeltaSeconds, Velocity, CurChar->GetActorRotation(), FRotator(0,0,0), ForwardDirAngle);
		BackwardDirAngle = CalculateDirAngle(DeltaSeconds, Velocity, CurChar->GetActorRotation(), FRotator(0,180,0), BackwardDirAngle);
		LeftDirAngle = CalculateDirAngle(DeltaSeconds, Velocity, CurChar->GetActorRotation(), FRotator(0, -90, 0), LeftDirAngle);
		RightDirAngle = CalculateDirAngle(DeltaSeconds, Velocity, CurChar->GetActorRotation(), FRotator(0, 90, 0), RightDirAngle);

		// make sure to clamp
		ForwardDirDeg = FMath::Clamp(FMath::RadiansToDegrees(ForwardDirAngle), -45.0f, 45.0f);
		BackwardDirDeg = FMath::Clamp(FMath::RadiansToDegrees(BackwardDirAngle), -45.0f, 45.0f);
		LeftDirDeg = FMath::Clamp(FMath::RadiansToDegrees(LeftDirAngle), -45.0f, 45.0f);
		RightDirDeg = FMath::Clamp(FMath::RadiansToDegrees(RightDirAngle), -45.0f, 45.0f);

		StrafeForwardDir = FMath::FInterpTo(StrafeForwardDir, ForwardDirDeg, DeltaSeconds, 8.0f);
		StrafeBackwardDir = FMath::FInterpTo(StrafeBackwardDir, BackwardDirDeg, DeltaSeconds, 8.0f);
		StrafeLeftDir = FMath::FInterpTo(StrafeLeftDir, LeftDirDeg, DeltaSeconds, 8.0f);
		StrafeRightDir = FMath::FInterpTo(StrafeRightDir, RightDirDeg, DeltaSeconds, 8.0f);
		//StrafeForwardDir = FMath::Clamp(ForwardDirDeg, -45.0f, 45.0f); // extra for strafe anim blending
		//StrafeBackwardDir = FMath::Clamp(BackwardDirDeg, -45.0f, 45.0f); // extra for strafe anim blending

		CalculateDirectionCase(DeltaSeconds, ForwardDirAngle);
		*/

		// new simplified
		if(!FMath::IsNearlyZero(Speed))
			CalculateDirectionExtended(DeltaSeconds, Velocity, CurChar->GetActorRotation());

		DirAngleDegrees = FMath::RadiansToDegrees(DirAngle);
	}

	APlayerCharacter* CurPlayerChar = Cast<APlayerCharacter>(TryGetPawnOwner());

	// fallback just incase
	if (CurPlayerChar)
	{
		SprintAlpha = CurPlayerChar->IsSprinting() ? UKismetMathLibrary::FInterpTo(SprintAlpha, 1.0f, DeltaSeconds, 8.0f) : UKismetMathLibrary::FInterpTo(SprintAlpha, 0.0f, DeltaSeconds, 8.0f);
		SprintFPAlpha = CurPlayerChar->IsSprinting() ? UKismetMathLibrary::FInterpTo(SprintAlpha, 1.0f, DeltaSeconds, 4.0f) : UKismetMathLibrary::FInterpTo(SprintAlpha, 0.0f, DeltaSeconds, 4.0f);
		// moved to character for proper replication
		bIsStopping = CurPlayerChar->bIsStopping;
	}

	if (Speed >= 5)
	{
		// fall back to run speed if no valid speed scale
		float SpeedScale = CurPlayerChar ? CurPlayerChar->GetRunSpeed()  : 1.0f;
		if (WalkSpeedDatabase.IsValidIndex(CurrentDirection))
		{
			SpeedScale = WalkSpeedDatabase[CurrentDirection];
		}
		
		//MovementAlpha = FMath::FInterpTo(MovementAlpha, FMath::Clamp(UKismetMathLibrary::NormalizeToRange(Speed, 0.0f, SpeedScale), 0.35f, 1.20f), DeltaSeconds, 7.5f);
		MovementAlpha = FMath::FInterpTo(MovementAlpha, 1.0f, DeltaSeconds, 2.5f);
	}
	else
	{
		MovementAlpha = FMath::FInterpTo(MovementAlpha, 0.0f, DeltaSeconds, 7.5f);
	}

	if (Speed > MovementJogThreshold)
	{
		// fall back to run speed if no valid speed scale
		float SpeedScale = CurPlayerChar ? CurPlayerChar->GetRunSpeed() : 1.0f;
		if (JogSpeedDatabase.IsValidIndex(CurrentDirection))
		{
			SpeedScale = JogSpeedDatabase[CurrentDirection];
		}
		 
		//MovementJogAlpha = FMath::FInterpTo(MovementJogAlpha, FMath::Clamp(UKismetMathLibrary::NormalizeToRange(Speed, 0.0f, SpeedScale), 0.65f, 1.20f), DeltaSeconds, 5.5f);
		MovementJogAlpha = FMath::FInterpTo(MovementJogAlpha, 1.0f, DeltaSeconds, 3.0f);
	}
	else
	{
		MovementJogAlpha = FMath::FInterpTo(MovementJogAlpha, 0.0f, DeltaSeconds, 3.0f);
	}



	if (CurChar->bIsCrouched)
		bCrouching = true;
	else
		bCrouching = false;



	CalculateJump(DeltaSeconds);


	//
	//
	//
	/* moved up! */
	// cast item
	//
	ABaseItem* CurItem = CurChar ? Cast<ABaseItem>(CurChar->GetEquippedItem()) : nullptr;

	if (CurItem)
	{
		CurMotionBlock = CurItem->ActiveMotionBlock;
		bItemOneHanded = CurItem->bIsOneHandedItem;
		if (CurItem->bDeployable)
		{
			bIsDeployableEquipped = true;
		}
		else
		{
			bIsDeployableEquipped = false;
		}
	}

	ABaseMagazineWeapon* MagWeapon = Cast<ABaseMagazineWeapon>(CurItem);
	if (MagWeapon)
	{
		if (MagWeapon->WeaponType == EWeaponType::WT_PistolsLethal || MagWeapon->WeaponType == EWeaponType::WT_PistolsNonLethal)
			bIsPistol = true;
		else
			bIsPistol = false;

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		if (MagWeapon->WeaponType == EWeaponType::WT_Rifles || MagWeapon->WeaponType == EWeaponType::WT_Shotgun || MagWeapon->WeaponType == EWeaponType::WT_SubmachineGun || MagWeapon->WeaponType == EWeaponType::WT_PrimaryNonLethal)
			bIsRifle = true;
		else
			bIsRifle = false;
	}

	AC2Explosive* C2Charge = Cast<AC2Explosive>(CurItem);

	if (C2Charge)
	{
		bIsC2Charge = true;
	}
	else
	{
		bIsC2Charge = false;
	}

	if (CurChar)
	{
		// team logic
		AReadyOrNotGameState* gs = Cast<AReadyOrNotGameState>(GetWorld()->GetGameState());

		if (gs)
			bIsTeamMLO = (gs->bPvPMode && CurChar->GetTeam() == ETeamType::TT_SERT_RED) ? true : false;
	}
	else
	{
		bIsTeamMLO = false;
	}
	
	bIsItem = CurMotionBlock == EMotionBlockType::MB_Item ? true : false;

	// needs proper testing!
	if (bItemOneHanded || (bIsTeamMLO && (CurPlayerChar && CurPlayerChar->IsSprinting()) && (bIsRifle || bIsPistol) ))
		LeftHandIKAlpha = FMath::FInterpTo(LeftHandIKAlpha, 0.0f, DeltaSeconds, 9.0f);
	else
		LeftHandIKAlpha = FMath::FInterpTo(LeftHandIKAlpha, 1.0f, DeltaSeconds, 9.0f);

	// org: 95, 148, 180, crouch org: 86
	bLevel1MovementTrigger = (Speed > 150.0f) ? true : false;
	bLevel2MovementTrigger = (Speed > 200.0f) ? true : false;
	bLevel3MovementTrigger = (Speed > 235.0f) ? true : false;
	bCrouchLevel1MovementTrigger = (Speed > 120.0f) ? true : false;


	DeltaRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurChar->GetActorRotation(), CachedRotation).Yaw / DeltaSeconds;
	CachedRotation = CurChar->GetActorRotation();

	// rotation rate check
	bRotationRateReached = FMath::Abs(DeltaRotation) > 1300.0f ? true : false;

	//UE_LOG(LogTemp, Warning, TEXT("DELTA ROT IS: %f"), DeltaRotation);

	// moved here from fp graph sub-class
	/* we perform the damping now in the tp graph at the correct places where movement is done to prevent important stance changes to be excluded like crouching */
	if (CurPlayerChar)
	{
		if (CurPlayerChar->bIsPelvisFPMovementBobActive && CurPlayerChar->IsLocallyControlled() && CurPlayerChar->GetFirstPersonCameraComponent())
		{
			PelvisMovementBobAlpha = FMath::FInterpTo(PelvisMovementBobAlpha, CurPlayerChar->PelvisFPMovementDamping, DeltaSeconds, 35.0f);
		}
		else
		{
			PelvisMovementBobAlpha = 0.0f;
		}
	}

	// tick this last
	NativeLastTick(DeltaSeconds);
}

AReadyOrNotCharacter* UReadyOrNotAnimInstance::TryGetAnyOwner()
{
	if (Cast<AReadyOrNotCharacter>(TryGetPawnOwner()))
	{
		return Cast<AReadyOrNotCharacter>(TryGetPawnOwner());
	} 

	if (GetOwningActor())
	{
		AReadyOrNotCharacter* tempChar = Cast<AReadyOrNotCharacter>(GetOwningActor()->GetOwner());
		if (tempChar)
			return tempChar;
	}
	return nullptr;
}

void UReadyOrNotAnimInstance::NativeLastTick(float DeltaSeconds)
{

}

FRotator UReadyOrNotAnimInstance::GetBaseAimRotation()
{
	APlayerCharacter* pc = Cast<APlayerCharacter>(TryGetPawnOwner());
	if (pc)
	{
		return pc->Server_BaseAimRotation;
	}
	return FRotator::ZeroRotator;
}


/*
Calculate the current direction angle from actor velocity and rotation
*/
float UReadyOrNotAnimInstance::CalculateDirAngle(float DeltaTime, FVector CurVel, FRotator CurActorRotation, FRotator DirRotation, float CurDirAngle)
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
		AReadyOrNotCharacter* rc = Cast<AReadyOrNotCharacter>(GetOwningActor());
		// rotate into the given direction to produce the desired result
		// it makes sure the rotation becomes the actual forward direction so it returns values from zero
		FVector LookDir = DirRotation.RotateVector(rc->ReplicatedControlRotation.Vector());// DirRotation.RotateVector(CurActorRotation.Vector()));

		/*
		if (!bIsForward)
			LookDir = -CurActorRotation.Vector(); // get negative
		*/

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
	return FMath::UnwindRadians(CurDirAngle + DeltaDir);

	/*
	if (DeltaDir != 0.f)
	{
		//FLOAT MaxDelta = DeltaSeconds * DirDegreesPerSecond * (PI / 180.f);
		//DeltaDir = FMath::Clamp<float>(DeltaDir, -MaxDelta, MaxDelta);
		return FMath::UnwindRadians(CurDirAngle + DeltaDir);
	}

	return 0.0f;
	*/
}

/*
Get current direction and set case so blend by enum works perfectly
*/
void UReadyOrNotAnimInstance::CalculateDirectionCase(float DeltaTime, float CurDirAngle)
{
	if (CurDirAngle < -0.75f*PI) // Back
	{
		CurrentDirection = EMoveDirection::Type::B;
	}
	else if (CurDirAngle < -0.25f*PI) // Left
	{
		CurrentDirection = EMoveDirection::Type::L;
	}
	else if (CurDirAngle < 0.25f*PI) // Forward
	{
		CurrentDirection = EMoveDirection::Type::F;
	}
	else if (CurDirAngle < 0.75f*PI) // Right
	{
		CurrentDirection = EMoveDirection::Type::R;
	}
	else // Back
	{
		CurrentDirection = EMoveDirection::Type::B;
	}

	/*
	float TimeSeconds = UGameplayStatics::GetTimeSeconds(GetWorld());

	if (TimeSeconds >= DirectionNextUpdateTime)
	{
		DirectionNextUpdateTime = TimeSeconds + DirectionUpdateRate;
	}
	*/

	/*
	
	*/


	/*
	if (CurDirAngle < -0.875f*PI) // Back
	{
		CurrentDirection = EMoveDirection::Type::B;
	}
	else if (CurDirAngle < -0.625f*PI) // Back-Left
	{
		CurrentDirection = EMoveDirection::Type::B;
	}
	else if (CurDirAngle < -0.375f*PI) // Left
	{
		CurrentDirection = EMoveDirection::Type::L;
	}
	else if (CurDirAngle < -0.125f*PI) // Forward-Left
	{
		CurrentDirection = EMoveDirection::Type::F;
	}
	else if (CurDirAngle < 0.125f*PI) // Forward
	{
		CurrentDirection = EMoveDirection::Type::F;
	}
	else if (CurDirAngle < 0.375f*PI) // Forward-Right
	{
		CurrentDirection = EMoveDirection::Type::F;
	}
	else if (CurDirAngle < 0.625f*PI) // Right
	{
		CurrentDirection = EMoveDirection::Type::R;
	}
	else if (CurDirAngle < 0.875f*PI) // Back-Right
	{
		CurrentDirection = EMoveDirection::Type::B;
	}
	else // Back
	{
		CurrentDirection = EMoveDirection::Type::B;
	}
	*/
}


FRotator UReadyOrNotAnimInstance::GetLookAtRotation()
{

	FRotator LookAtRotation = FRotator::ZeroRotator;
	ACyberneticCharacter* OwningAiCharacter = Cast<ACyberneticCharacter>(TryGetPawnOwner());
	if (OwningAiCharacter)
	{
		if (OwningAiCharacter->IsDeadOrUnconscious())
			return FRotator::ZeroRotator;

		FVector EyesLocation;
		FRotator EyesRotation;
		OwningAiCharacter->GetActorEyesViewPoint(EyesLocation, EyesRotation);

		FVector v1 = UKismetMathLibrary::FindLookAtRotation(EyesLocation, OwningAiCharacter->Rep_HeadFocalPoint).Vector();
		FVector v2 = OwningAiCharacter->GetActorForwardVector();
		
		float DotProduct2D = FVector2D::DotProduct(FVector2D(v1.X, v1.Y), FVector2D(v2.X, v2.Y));
		if (DotProduct2D > 0.0f)
		{
			LookAtRotation = UKismetMathLibrary::FindLookAtRotation(EyesLocation, OwningAiCharacter->Rep_HeadFocalPoint).GetNormalized();
		}



		if (LookAtRotation != FRotator::ZeroRotator)
        {
        	LookAtRotation = UKismetMathLibrary::NormalizedDeltaRotator(LookAtRotation,  OwningAiCharacter->GetActorRotation() + FRotator(-OwningAiCharacter->AimOffset.X, OwningAiCharacter->AimOffset.Y, 0.0f));
        	LookAtRotation.Yaw = FMath::Clamp(LookAtRotation.Yaw, -35.0f, 35.0f);
        	LookAtRotation.Pitch = FMath::Clamp(LookAtRotation.Pitch, -30.0f, 30.0f);
			LookAtRotation.Roll = LookAtRotation.Pitch * -1.0f;
			LookAtRotation.Pitch = 0;
        	return LookAtRotation;
        }
		
		//UE_LOG(LogTemp, Warning, TEXT("desiredOrientation %f"), desiredOrientation);
	}
	else
	{
		APlayerCharacter* pc = Cast<APlayerCharacter>(TryGetPawnOwner());
		if (pc)
		{
			LookAtRotation = pc->FreeLookCache;
			LookAtRotation.Pitch = LookAtRotation.Pitch * -1;
			return LookAtRotation.GetNormalized();
		}
	}
	
	return FRotator::ZeroRotator;
}

// handy function to retrieve the specific curve from a animation asset
FAnimCurveBufferAccess UReadyOrNotAnimInstance::GetAnimationDataCurveBuffer(UAnimSequence* Animation, FName CurveName)
{
	// new code, working as of 4.24!
	// Curves need the Uniform Indexable compression for it to work!
	checkf(Animation != nullptr, TEXT("Invalid Animation Sequence ptr"));
	FSmartName CurveSmartName;

	Animation->GetSkeleton()->GetSmartNameByName(USkeleton::AnimCurveMappingName, CurveName, CurveSmartName);
	FAnimCurveBufferAccess CurveBuffer = FAnimCurveBufferAccess(Animation, CurveSmartName.UID);
	return CurveBuffer;
}

// perform binary search on given distance and output correct play length
float UReadyOrNotAnimInstance::FindPositionFromDistanceCurve(FAnimCurveBufferAccess DistanceCurve, const float& Distance)
{
	const int32 NumKeys = DistanceCurve.GetNumSamples();
	if (NumKeys < 2)
	{
		return 0.f;
	}

	int32 first = 1;
	int32 last = NumKeys - 1;
	int32 count = last - first;

	while (count > 0)
	{
		int32 step = count / 2;
		int32 middle = first + step;

		if (Distance > DistanceCurve.GetValue(middle))
		{
			first = middle + 1;
			count -= step + 1;
		}
		else
		{
			count = step;
		}
	}

	float KeyA = DistanceCurve.GetValue(first - 1);
	float KeyB = DistanceCurve.GetValue(first);
	const float Diff = KeyB - KeyA;
	const float Alpha = !FMath::IsNearlyZero(Diff) ? ((Distance - KeyA) / Diff) : 0.f;
	return FMath::Lerp(DistanceCurve.GetTime(first -1), DistanceCurve.GetTime(first), Alpha);
}


float UReadyOrNotAnimInstance::EvalAnimCurveBuffer(FAnimCurveBufferAccess Curve, float InTime)
{
	// Remap time if extrapolation is present and compute offset value to use if cycling 
	float CycleValueOffset = 0;
	
	// Might ne needed???
	//RemapTimeValue(InTime, CycleValueOffset);

	const int32 NumKeys = Curve.GetNumSamples();

	// If the default value hasn't been initialized, use the incoming default value
	float InterpVal = 10.0f;

	if (NumKeys == 0)
	{
		// If no keys in curve, return the Default value.
	}
	else if (NumKeys < 2 || (InTime <= Curve.GetTime(0)))
	{
		if (NumKeys > 1)
		{
			float DT = Curve.GetTime(1) - Curve.GetTime(0);

			if (FMath::IsNearlyZero(DT))
			{
				InterpVal = Curve.GetValue(0);
			}
			else
			{
				float DV = Curve.GetValue(1) - Curve.GetValue(0);
				float Slope = DV / DT;

				InterpVal = Slope * (InTime - Curve.GetTime(0)) + Curve.GetValue(0);
			}
		}
	}
	else if (InTime < Curve.GetTime(NumKeys - 1))
	{
		// perform a lower bound to get the second of the interpolation nodes
		int32 first = 1;
		int32 last = NumKeys - 1;
		int32 count = last - first;

		while (count > 0)
		{
			int32 step = count / 2;
			int32 middle = first + step;

			if (InTime >= Curve.GetTime(middle))
			{
				first = middle + 1;
				count -= step + 1;
			}
			else
			{
				count = step;
			}
		}

		InterpVal = EvalForTwoKeys(Curve, first - 1, first, InTime);
	}
	else
	{
		float DT = Curve.GetTime(NumKeys - 2)- Curve.GetTime(NumKeys - 1);

		if (FMath::IsNearlyZero(DT))
		{
			InterpVal = Curve.GetValue(NumKeys - 1);
		}
		else
		{
			float DV = Curve.GetValue(NumKeys - 2) - Curve.GetValue(NumKeys - 1);
			float Slope = DV / DT;

			InterpVal = Slope * (InTime - Curve.GetTime(NumKeys - 1)) + Curve.GetValue(NumKeys - 1);
		}
	}

	return InterpVal + CycleValueOffset;
}

float UReadyOrNotAnimInstance::EvalForTwoKeys(FAnimCurveBufferAccess Curve, int32 Key1, int32 Key2, const float InTime)
{
	const float Diff = Curve.GetTime(Key2) - Curve.GetTime(Key1);

	if (Diff > 0.f)
	{
		const float Alpha = (InTime - Curve.GetTime(Key1)) / Diff;
		const float P0 = Curve.GetValue(Key1);
		const float P3 = Curve.GetValue(Key2);

		return FMath::Lerp(P0, P3, Alpha);
	}
	else
	{
		return Curve.GetValue(Key1);
	}
}

// added jump calcs
void UReadyOrNotAnimInstance::CalculateJump(float DeltaTime)
{
	// just to be safe, plus get pawn
	AReadyOrNotCharacter* OwningCharacter = Cast<AReadyOrNotCharacter>(TryGetPawnOwner());
	if (OwningCharacter == nullptr)
	{
		return;
	}

	if (!OwningCharacter->GetCharacterMovement())
		return;

	// run jump code only when being in the falling state
	if (OwningCharacter->GetCharacterMovement()->IsFalling())
	{
		bIsFalling = true;
		float FallingVelocity = OwningCharacter->GetCharacterMovement()->Velocity.Z;

		// predict the landing point
		FHitResult Hit(1.f);

		// go 6 seconds into the future	
		float TraceTime = 6.0f;
		FVector Grav(0.0f, 0.0f, OwningCharacter->GetCharacterMovement()->GetGravityZ());

		// calculate how far to trace.  want to trace N seconds into the future, where
		// N is the blend-in time of the preland anim.  Using kinematic equation d=vt*.5at^2.
		FVector HowFar = OwningCharacter->GetCharacterMovement()->Velocity * TraceTime + 0.5f * Grav * FMath::Square(TraceTime);

		// we need to consider the crouched capsule because when jumping in crouch it stays the same.
		//float CurrentEyeHeight = bCrouching ? OwningCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 5.0f : OwningCharacter->BaseEyeHeight + 5.0f;
		float CurrentEyeHeight = OwningCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 1.0f;

		// Line trace from bottom of cylinder to find how long until we hit the ground
		FVector TraceStart = OwningCharacter->GetActorLocation() + FVector(0, 0, -CurrentEyeHeight);

		GWorld->LineTraceSingleByChannel(Hit, TraceStart, TraceStart + HowFar, ECollisionChannel::ECC_WorldStatic);

		// get the distance when to toggle the landing phase
		//float LandToggleDistance = FMath::Abs(JumpLandCurve->Evaluate(0.0f)); todo hook up to anim curve?
		float DistanceFromGround = ((OwningCharacter->GetActorLocation() - FVector(0, 0, CurrentEyeHeight)) - Hit.Location).Size();

		// When going down, keep checking for time we are going to hit the ground.
		if (FallingVelocity < 0.f)
		{
			if (DistanceFromGround < 50.0f && !bHasPrelanded) // use small hardcoded value for now
			{
				bHasPrelanded = true;
			}
		}
	}
	else
	{
		if (bIsFalling)
		{
			// tell system to switch to recovery phase
			// reset recovery anim time back to zero
			JumpRecoveryAnimTime = 0.0f;

			// run timer to blend out of recovery strength
			OwningCharacter->GetWorldTimerManager().SetTimer(JumpRecoveryHandle, this, &UReadyOrNotAnimInstance::JumpRecoveryDone, JumpRecoveryTime, false);
			bJumpRecoveryActive = true;

			// reset jumping bool
			bIsFalling = false;

			//bHasPrelanded = false;

			// to be safe in some cases it got stuck otherwise
			bHasPrelanded = true;
		}
	}

	if (bJumpRecoveryActive)
	{
		// reset recovery animation start time
		JumpRecoveryAnimTime += DeltaTime;
		JumpRecoveryStrength = FMath::FInterpTo(JumpRecoveryStrength, 1.0f, DeltaTime, 13.0f);
	}
	else
	{
		JumpRecoveryStrength = FMath::FInterpTo(JumpRecoveryStrength, 0.0f, DeltaTime, 13.0f);
	}
}

void UReadyOrNotAnimInstance::JumpRecoveryDone()
{
	// just to be safe, plus get pawn
	AReadyOrNotCharacter* OwningCharacter = Cast<AReadyOrNotCharacter>(TryGetPawnOwner());
	if (OwningCharacter == nullptr)
	{
		return;
	}

	// clear timer
	OwningCharacter->GetWorldTimerManager().ClearTimer(JumpRecoveryHandle);
	bJumpRecoveryActive = false;
	bHasPrelanded = false;
}


/* rewritten to work better */
void UReadyOrNotAnimInstance::CalculateDirectionExtended(float DeltaTime, FVector CurVel, FRotator CurActorRotation)
{
	float Speed2D = CurVel.Size2D();

	float TargetDirAngle = 0.f;
	FVector	VelDir = CurVel;
	VelDir.Z = 0.0f;

	if (VelDir.IsNearlyZero())
	{
		TargetDirAngle = 0.f;
	}
	else
	{
		// TODO Optimize: Calculating this again for each AnimNode is inefficient
		VelDir = VelDir.GetSafeNormal();

		FVector LookDir = CurActorRotation.Vector();
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

	// Move DirAngle towards TargetDirAngle as fast as DirRadsPerSecond allows
	float DeltaDir = FMath::FindDeltaAngleRadians(DirAngle, TargetDirAngle);
	if (DeltaDir != 0.f)
	{
		//FLOAT MaxDelta = DeltaSeconds * DirDegreesPerSecond * (PI / 180.f);
		//DeltaDir = FMath::Clamp<float>(DeltaDir, -MaxDelta, MaxDelta);
		DirAngle = FMath::UnwindRadians(DirAngle + DeltaDir);
	}

	if (!FMath::IsNearlyZero(Speed2D))
	{
		if (DirAngle < -0.875f * PI) // Back
		{
			CurrentDirectionExt = EMoveDirectionExt::Type::B;
		}
		else if (DirAngle < -0.625f * PI) // Back-Left
		{
			CurrentDirectionExt = EMoveDirectionExt::Type::BL;
		}
		else if (DirAngle < -0.375f * PI) // Left
		{
			CurrentDirectionExt = EMoveDirectionExt::Type::L;
		}
		else if (DirAngle < -0.125f * PI) // Forward-Left
		{
			CurrentDirectionExt = EMoveDirectionExt::Type::FL;
		}
		else if (DirAngle < 0.125f * PI) // Forward
		{
			CurrentDirectionExt = EMoveDirectionExt::Type::F;
		}
		else if (DirAngle < 0.375f * PI) // Forward-Right
		{
			CurrentDirectionExt = EMoveDirectionExt::Type::FR;
		}
		else if (DirAngle < 0.625f * PI) // Right
		{
			CurrentDirectionExt = EMoveDirectionExt::Type::R;
		}
		else if (DirAngle < 0.875f * PI) // Back-Right
		{
			CurrentDirectionExt = EMoveDirectionExt::Type::BR;
		}
		else // Back
		{
			CurrentDirectionExt = EMoveDirectionExt::Type::B;
		}
	}
	
}

TEnumAsByte<EMoveDirectionExt::Type> UReadyOrNotAnimInstance::GetCurrentDirectionExtFromYawAngle(float YawAngle)
{
	float CurDirAngle = FMath::DegreesToRadians(YawAngle);

	if (CurDirAngle < -0.875f * PI) // Back
	{
		return EMoveDirectionExt::Type::B;
	}
	else if (CurDirAngle < -0.625f * PI) // Back-Left
	{
		return EMoveDirectionExt::Type::BL;
	}
	else if (CurDirAngle < -0.375f * PI) // Left
	{
		return EMoveDirectionExt::Type::L;
	}
	else if (CurDirAngle < -0.125f * PI) // Forward-Left
	{
		return EMoveDirectionExt::Type::FL;
	}
	else if (CurDirAngle < 0.125f * PI) // Forward
	{
		return EMoveDirectionExt::Type::F;
	}
	else if (CurDirAngle < 0.375f * PI) // Forward-Right
	{
		return EMoveDirectionExt::Type::FR;
	}
	else if (CurDirAngle < 0.625f * PI) // Right
	{
		return EMoveDirectionExt::Type::R;
	}
	else if (CurDirAngle < 0.875f * PI) // Back-Right
	{
		return EMoveDirectionExt::Type::BR;
	}
	else // Back
	{
		return EMoveDirectionExt::Type::B;
	}

	return EMoveDirectionExt::Type::F;
}

TEnumAsByte<EMoveDirectionExt::Type> UReadyOrNotAnimInstance::GetOppositeDirectionExt(TEnumAsByte<EMoveDirectionExt::Type> CurrentDir)
{
	if (CurrentDir == EMoveDirectionExt::Type::B) // back to forward
		return EMoveDirectionExt::Type::F;

	if (CurrentDir == EMoveDirectionExt::Type::F) // forward to back
		return EMoveDirectionExt::Type::B;

	if (CurrentDir == EMoveDirectionExt::Type::L) // left to right
		return EMoveDirectionExt::Type::R;

	if (CurrentDir == EMoveDirectionExt::Type::R) // right to left
		return EMoveDirectionExt::Type::L;

	if (CurrentDir == EMoveDirectionExt::Type::FL) // forward left to backward right
		return EMoveDirectionExt::Type::BR;

	if (CurrentDir == EMoveDirectionExt::Type::FR) // forward right to backward left
		return EMoveDirectionExt::Type::BL;

	if (CurrentDir == EMoveDirectionExt::Type::BL) // backward left to forward right
		return EMoveDirectionExt::Type::FR;

	if (CurrentDir == EMoveDirectionExt::Type::BR) // backward right to forward left
		return EMoveDirectionExt::Type::FL;

	return EMoveDirectionExt::Type::F;
}

float UReadyOrNotAnimInstance::GetWeightFromSlot(FName SlotName)
{
	return GetSlotMontageGlobalWeight(SlotName);
}

float UReadyOrNotAnimInstance::GetWeightFromSlotInversed(FName SlotName)
{
	return GetSlotMontageGlobalWeight(SlotName) * -1.0f;
}