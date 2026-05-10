// Void Interactive, 2020

#include "Animation/Notifies/AnimNotify_EquipItemOfClass.h"

void UAnimNotify_EquipItemOfClass::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::Notify(MeshComp, Animation);

	if (bAIOnly)
	{
		if (const ACyberneticCharacter* AICharacter = Cast<ACyberneticCharacter>(MeshComp->GetOwner()))
		{
			AICharacter->GetInventoryComponent()->EquipItemOfClass(ItemClass, bInstant);
		}
	}
	else
	{
		if (const AReadyOrNotCharacter* Character = Cast<AReadyOrNotCharacter>(MeshComp->GetOwner()))
		{
			Character->GetInventoryComponent()->EquipItemOfClass(ItemClass, bInstant);
		}
	}
}
