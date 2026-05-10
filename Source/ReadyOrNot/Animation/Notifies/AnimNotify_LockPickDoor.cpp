// Void Interactive, 2020

#include "Animation/Notifies/AnimNotify_LockPickDoor.h"

void UAnimNotify_LockPickDoor::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::Notify(MeshComp, Animation);

	if (const AReadyOrNotCharacter* OwnerCharacter = Cast<AReadyOrNotCharacter>(MeshComp->GetOwner()))
	{
		if (bLockPickFinished)
			OwnerCharacter->OnDoorLockPickEnd_FromAnimNotify.Broadcast();
		else
			OwnerCharacter->OnDoorLockPickBegin_FromAnimNotify.Broadcast();
			
	}
}
