// Void Interactive, 2020

#include "Animation/Notifies/AnimNotify_MirrorDoor.h"

void UAnimNotify_MirrorDoor::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::Notify(MeshComp, Animation);

	if (const AReadyOrNotCharacter* OwningCharacter = Cast<AReadyOrNotCharacter>(MeshComp->GetOwner()))
	{
		if (bMirrorFinish)
			OwningCharacter->OnMirrorDoorFinished_FromAnimNotify.Broadcast();
		else
			OwningCharacter->OnMirrorDoorStarted_FromAnimNotify.Broadcast();
	}
}
