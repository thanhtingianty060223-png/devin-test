// Void Interactive, 2020

#include "AnimNotify_WedgeDoor.h"

#include "Characters/CyberneticController.h"

void UAnimNotify_WedgeDoor::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::Notify(MeshComp, Animation);

	if (const AReadyOrNotCharacter* OwningCharacter = Cast<AReadyOrNotCharacter>(MeshComp->GetOwner()))
	{
		if (bWedgeDeployFinished)
			OwningCharacter->OnEndDoorWedgePlacement_FromAnimNotify.Broadcast();
		else
			OwningCharacter->OnStartDoorWedgePlacement_FromAnimNotify.Broadcast();
	}
}
