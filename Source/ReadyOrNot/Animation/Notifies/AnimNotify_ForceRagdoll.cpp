// Void Interactive, 2022
#include "AnimNotify_ForceRagdoll.h"
#include "Characters/CyberneticController.h"

void UAnimNotify_ForceRagdoll::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::Notify(MeshComp, Animation);
	
	AReadyOrNotCharacter* rc = Cast<AReadyOrNotCharacter>(MeshComp->GetOwner());
	if (rc)
	{
		// trigger ragdoll from current frame
		if (rc->GetCharacterMovement())
		{
			rc->GetCharacterMovement()->StopMovementImmediately();
			rc->GetCharacterMovement()->DisableMovement();
		}

		if (rc->GetCapsuleComponent())
		{
			rc->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			rc->GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
		}

		rc->SetAppropriatePhysicsAsset(true);

		if (rc->GetMesh())
		{
			rc->GetMesh()->SetCollisionProfileName("Ragdoll", true);

			// Always enable CCD on the pelvis to reduce whole body clipping into/through walls due to velocity/framerate.
			rc->GetMesh()->SetUseCCD(true, "pelvis");

			// wake up all bodies
			rc->GetMesh()->SetAllBodiesBelowSimulatePhysics("pelvis", true, true);
		}

		UE_LOG(LogTemp, Warning, TEXT("UAnimNotify_ForceRagdoll: Ragdoll forced!"));
	}
}
