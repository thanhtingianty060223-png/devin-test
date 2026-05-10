// Void Interactive, 2020

#include "Animation/Notifies/AnimNotify_OnCheckedDoor.h"


void UAnimNotify_OnCheckedDoor::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::Notify(MeshComp, Animation);

	AReadyOrNotCharacter* Character = Cast<AReadyOrNotCharacter>(MeshComp->GetOwner());
	if (Character)
	{
		Character->OnDoorChecked_FromAnimNotify.Broadcast();
	}
}
