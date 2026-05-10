// Void Interactive, 2020


#include "AnimNotify_EquipLastItem.h"

void UAnimNotify_EquipLastItem::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	APlayerCharacter* pc = Cast<APlayerCharacter>(MeshComp->GetOwner());
	if (pc)
	{
		pc->GetInventoryComponent()->EquipLastEquippedItem(bInstant);
	}
}
