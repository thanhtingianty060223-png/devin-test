// Void Interactive, 2018
// Author: Alexander Mijalkovski

// Handle Face Animation

#include "Animation/ReadyOrNotFaceAnimInstance.h"
#include "ReadyOrNotCharacter.h"

DECLARE_CYCLE_STAT_EXTERN(TEXT("RoN_Face_Anim_NativeUpdate"), STAT_NativeRoNFaceAnimUpdate, STATGROUP_Anim, );
DEFINE_STAT(STAT_NativeRoNFaceAnimUpdate);

UReadyOrNotFaceAnimInstance::UReadyOrNotFaceAnimInstance(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UReadyOrNotFaceAnimInstance::NativeInitializeAnimation()
{
}

// main func for tick calculations
void UReadyOrNotFaceAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
//	SCOPE_CYCLE_COUNTER(STAT_NativeRoNAnimUpdate);

	AReadyOrNotCharacter* CurChar = Cast<AReadyOrNotCharacter>(TryGetPawnOwner());

	// fallback just incase
	if (CurChar == nullptr)
	{
		return;
	}

	// used to drive the head skeleton
	BodyDriverMesh = CurChar->GetMesh();

	/*
	APlayerCharacter* CurPlayerChar = Cast<APlayerCharacter>(TryGetPawnOwner());

	// fallback just incase
	if (CurPlayerChar == nullptr)
	{
		return;
	}
	*/

	// ron specifics for AI
	if (const ACyberneticCharacter* CurCyberneticChar = Cast<ACyberneticCharacter>(CurChar))
	{
		//UE_LOG(LogTemp, Warning, TEXT("Cybernetic Char is valid!"));

		float EyesYawLimit = 50.0f;
		float EyesPitchLimit = 50.0f;

		FocalTargetLookRotation = UKismetMathLibrary::RInterpTo(FocalTargetLookRotation, CurCyberneticChar->GetLookAtRotation(EyesYawLimit, EyesPitchLimit), DeltaSeconds, 12.0f); // eyes should be rather fast

		// the pose system covers 45 degrees
		EyeTargetLookLeft = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, -EyesYawLimit), FVector2D(0.0f, 1.0f), FocalTargetLookRotation.Yaw);
		EyeTargetLookRight = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, EyesYawLimit), FVector2D(0.0f, 1.0f), FocalTargetLookRotation.Yaw);
		EyeTargetLookUp = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, EyesPitchLimit), FVector2D(0.0f, 1.0f), FocalTargetLookRotation.Pitch);
		EyeTargetLookDown = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, -EyesPitchLimit), FVector2D(0.0f, 1.0f), FocalTargetLookRotation.Pitch);

		bool bEyeLimitsReached = false;
		if (FMath::Abs(FocalTargetLookRotation.Yaw) >= 35.0f || FMath::Abs(FocalTargetLookRotation.Pitch) >= 35.0f)
		{
			bEyeLimitsReached = true;
		}

		if (bEyeLimitsReached)
		{
			float HeadYawLimit = 22.5f;
			float HeadPitchLimit = 22.5f;
			HeadLookRotation = UKismetMathLibrary::RInterpTo(HeadLookRotation, CurCyberneticChar->GetLookAtRotation(HeadYawLimit, HeadPitchLimit), DeltaSeconds, 3.0f); // head is slower then eyes
		}
		else
		{
			HeadLookRotation = UKismetMathLibrary::RInterpTo(HeadLookRotation, FRotator::ZeroRotator, DeltaSeconds, 4.0f); // head is slower then eyes
		}
	}

	// tick this last
	NativeLastTick(DeltaSeconds);
}

APlayerCharacter* UReadyOrNotFaceAnimInstance::TryGetAnyOwner()
{
	if (Cast<APlayerCharacter>(TryGetPawnOwner()))
	{
		return Cast<APlayerCharacter>(TryGetPawnOwner());
	} 

	if (GetOwningActor())
	{
		APlayerCharacter* tempChar = Cast<APlayerCharacter>(GetOwningActor()->GetOwner());
		if (tempChar)
			return tempChar;
	}
	return nullptr;
}

void UReadyOrNotFaceAnimInstance::NativeLastTick(float DeltaSeconds)
{
}

UPoseAsset* UReadyOrNotFaceAnimInstance::GetFaceROM() const
{
	UPoseAsset* PoseAsset = DefaultFaceROMData;

	// cast character
	AReadyOrNotCharacter* OwnerCharacter = Cast<AReadyOrNotCharacter>(TryGetPawnOwner());
	if (OwnerCharacter && OwnerCharacter->GetCurrentFaceROM())
	{
		PoseAsset = OwnerCharacter->GetCurrentFaceROM();
	}

	return PoseAsset;
}
