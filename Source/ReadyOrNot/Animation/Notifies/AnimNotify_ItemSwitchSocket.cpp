// Void Interactive, 2020

#include "AnimNotify_ItemSwitchSocket.h"

void UAnimNotify_ItemSwitchSocket::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::Notify(MeshComp, Animation);
	
	if (const AReadyOrNotCharacter* Character = Cast<AReadyOrNotCharacter>(MeshComp->GetOwner()))
	{
		if (DesiredItemSocket == EItemSocket::Body)
			Character->GetInventoryComponent()->HolsterEquippedItem(true);
		else
			Character->GetInventoryComponent()->EquipHolsteredItem(true);
	}
}
