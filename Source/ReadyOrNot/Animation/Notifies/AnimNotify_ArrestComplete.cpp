// Copyright Void Interactive, 2023

#include "Animation/Notifies/AnimNotify_ArrestComplete.h"

void UAnimNotify_ArrestComplete::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::Notify(MeshComp, Animation);

	if (const AReadyOrNotCharacter* Character = MeshComp->GetOwner<AReadyOrNotCharacter>())
	{
		if (AZipcuffs* Zipcuffs = Character->GetInventoryComponent()->GetInventoryItemOfClass_Native<AZipcuffs>(AZipcuffs::StaticClass(), false))
		{
			Zipcuffs->Server_ArrestComplete();
		}
	}
}
