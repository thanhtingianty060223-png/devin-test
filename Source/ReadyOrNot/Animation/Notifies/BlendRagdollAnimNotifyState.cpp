// Copyright Void Interactive, 2021

#include "BlendRagdollAnimNotifyState.h"
#include "Animation/RoNAnimInstance_PlayerTP.h"

UBlendRagdollAnimNotifyState::UBlendRagdollAnimNotifyState()
{
	bIsNativeBranchingPoint = true;

	TotalBlendDuration = 0.0f;
	CurrentBlendAmount = 0.0f;
	
	bWakedUpPelvis = false;
	PelvisWakeUpTime = 0.35f;

	bUsePhysicalAnimComp = true;

	PhysicalAnimData.bIsLocalSimulation = false;
	PhysicalAnimData.OrientationStrength = 1000.0f;
	PhysicalAnimData.AngularVelocityStrength = 100.0f;
	PhysicalAnimData.PositionStrength = 1000.0f;
	PhysicalAnimData.VelocityStrength = 100.0f;
	PhysicalAnimData.MaxAngularForce = 0.0f;
	PhysicalAnimData.MaxLinearForce = 0.0f;
}

void UBlendRagdollAnimNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration)
{
	AReadyOrNotCharacter* rc = Cast<AReadyOrNotCharacter>(MeshComp->GetOwner());
	if (rc)
	{
		if (rc->bBlendingAnim2Ragdoll)
			return;

		//if (rc->IsDeadOrUnconscious())
		//	return;
		NotifyTotalDuration = TotalDuration;
		ActiveAnimTime = 0.0f;

		TotalBlendDuration = 1.0f / TotalDuration;
		CurrentBlendAmount = 0.0f;
		bWakedUpPelvis = false;

		// rc->SetAppropriatePhysicsAsset(true);
		rc->GetMesh()->SetCollisionProfileName("Ragdoll", true);

		// Always enable CCD on the pelvis to reduce whole body clipping into/through walls due to velocity/framerate.
		rc->GetMesh()->SetUseCCD(true, "pelvis");

#ifdef RON_DEATHS_USE_LINEAR_BLEND

		rc->GetMesh()->SetAllBodiesBelowSimulatePhysics("pelvis", true, true);
		rc->GetMesh()->SetAllBodiesBelowPhysicsBlendWeight("pelvis", 1.0f, false, true);
#else
		/*
		if (!bUsePhysicalAnimComp)
		{
			rc->GetMesh()->SetAllBodiesBelowSimulatePhysics("pelvis", true, false);
		}
		else
		{
			if (rc->GetPhysicalAnimationComp())
			{
				rc->GetPhysicalAnimationComp()->SetSkeletalMeshComponent(rc->GetMesh());
				rc->GetPhysicalAnimationComp()->ApplyPhysicalAnimationSettingsBelow("pelvis", PhysicalAnimData, true);
				rc->GetMesh()->SetAllBodiesBelowSimulatePhysics("pelvis", true, true);
			}
		}
		*/
#endif


		rc->bBlendInPhysics = true;

		bDeadOnBegin = rc->IsDeadNotUnconscious();
		
		// debug if needed
		UE_LOG(LogTemp, Warning, TEXT("UBlendRagdollAnimNotifyState: Start Blend amount: %f"), CurrentBlendAmount);
	}
}

void UBlendRagdollAnimNotifyState::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime)
{
	AReadyOrNotCharacter* rc = Cast<AReadyOrNotCharacter>(MeshComp->GetOwner());
	if (rc)
	{
		if (rc->bBlendingAnim2Ragdoll)
			return;

		if (bDiedWhileInTick)
			return;

		rc->bBlendInPhysics = true;

		const float NewTime = FMath::Min(ActiveAnimTime + FrameDeltaTime, NotifyTotalDuration);

		//CurrentBlendAmount = FMath::FInterpConstantTo(CurrentBlendAmount, 1.0f, FrameDeltaTime, TotalBlendDuration);
		if (NewTime > ActiveAnimTime)
		{
			ActiveAnimTime = NewTime;
			CurrentBlendAmount = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, NotifyTotalDuration), FVector2D(0.0f, 1.0f), ActiveAnimTime);

#ifdef RON_DEATHS_USE_LINEAR_BLEND
			// once the threshold is reached we want to update the pelvis bone aswell!
			rc->GetMesh()->SetAllBodiesBelowPhysicsBlendWeight("pelvis", CurrentBlendAmount, false, true);
#else
			if (!bUsePhysicalAnimComp)
			{
				if (!bWakedUpPelvis && CurrentBlendAmount > PelvisWakeUpTime)
				{
					rc->GetMesh()->SetAllBodiesSimulatePhysics(true);
					bWakedUpPelvis = true;

					UE_LOG(LogTemp, Warning, TEXT("UBlendRagdollAnimNotifyState: Pelvis waked up!"));
				}

				// once the threshold is reached we want to update the pelvis bone aswell!
				if (CurrentBlendAmount > PelvisWakeUpTime)
					rc->GetMesh()->SetAllBodiesBelowPhysicsBlendWeight("pelvis", CurrentBlendAmount, false, true);
				else
					rc->GetMesh()->SetAllBodiesBelowPhysicsBlendWeight("pelvis", CurrentBlendAmount, false, false);
			}
#endif

			if (rc->IsDeadNotUnconscious())
			{
				CurrentBlendAmount = 1.0f;
				NotifyEnd(MeshComp, Animation);
				bDiedWhileInTick = !bDeadOnBegin;
				return;
			}

			UE_LOG(LogTemp, Warning, TEXT("UBlendRagdollAnimNotifyState: Current Blend amount: %f"), CurrentBlendAmount);
		}
	}
}

void UBlendRagdollAnimNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	AReadyOrNotCharacter* rc = Cast<AReadyOrNotCharacter>(MeshComp->GetOwner());
	if (!IsValid(rc))
		return;

	if (rc->bBlendingAnim2Ragdoll)
		return;
	
	if (bDiedWhileInTick)
		return;
	
	if (rc->GetCapsuleComponent())
	{
		rc->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		rc->GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
	}
	
	if (CurrentBlendAmount > 0.8f && rc)
	{
		UE_LOG(LogTemp, Warning, TEXT("UBlendRagdollAnimNotifyState: Stopping montage at %f"), CurrentBlendAmount);

		if (rc->GetCharacterMovement())
		{
			rc->GetCharacterMovement()->StopMovementImmediately();
			rc->GetCharacterMovement()->DisableMovement();
		}
		

#ifdef RON_DEATHS_USE_LINEAR_BLEND
		// stop current active montage
		MeshComp->GetAnimInstance()->StopAllMontages(0.15f);
#else
		if (!bUsePhysicalAnimComp)
		{
			// stop current active montage
			MeshComp->GetAnimInstance()->StopAllMontages(0.15f);
		}
		else
		{
			/*
			if (rc->GetPhysicalAnimationComp())
			{
				FPhysicalAnimationData NewData;
				NewData.bIsLocalSimulation = false;
				NewData.OrientationStrength = 0.0f;
				NewData.AngularVelocityStrength = 0.0f;
				NewData.PositionStrength = 0.0f;
				NewData.VelocityStrength = 0.0f;
				NewData.MaxAngularForce = 0.0f;
				NewData.MaxLinearForce = 0.0f;

				rc->GetPhysicalAnimationComp()->SetSkeletalMeshComponent(rc->GetMesh());
				rc->GetPhysicalAnimationComp()->ApplyPhysicalAnimationSettingsBelow("pelvis", NewData, true);
			}
			*/
		}
#endif
	}

	rc->bBlendInPhysics = false;
	
	FTimerDelegate Delegate;
	Delegate.BindUObject(rc, &AReadyOrNotCharacter::OnBlendRagdollAnimFinished);

	rc->GetWorld()->GetTimerManager().SetTimerForNextTick(Delegate);
}
#if WITH_EDITOR
void UBlendRagdollAnimNotifyState::OnAnimNotifyCreatedInEditor(FAnimNotifyEvent& ContainingAnimNotifyEvent)
{
	ContainingAnimNotifyEvent.MontageTickType = EMontageNotifyTickType::BranchingPoint;
}
#endif