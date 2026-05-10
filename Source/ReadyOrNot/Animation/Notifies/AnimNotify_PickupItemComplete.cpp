// Void Interactive, 2020

#include "AnimNotify_PickupItemComplete.h"

#include "Characters/CyberneticCharacter.h"
#include "Characters/CyberneticController.h"

void UAnimNotify_PickupItemComplete::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (const AReadyOrNotCharacter* Character = Cast<AReadyOrNotCharacter>(MeshComp->GetOwner()))
	{
		Character->OnPickupItem_FromAnimNotify.Broadcast();
	}
}
