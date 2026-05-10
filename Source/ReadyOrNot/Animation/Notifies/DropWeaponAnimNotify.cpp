// Copyright Void Interactive, 2021

#include "DropWeaponAnimNotify.h"

#include "Components/InventoryComponent.h"

void UDropWeaponAnimNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (const AReadyOrNotCharacter* OwnerCharacter = Cast<AReadyOrNotCharacter>(MeshComp->GetOwner()))
	{
		if (UInventoryComponent* InventoryComponent = OwnerCharacter->GetInventoryComponent())
		{
			InventoryComponent->ThrowEquippedItem();
			InventoryComponent->ClearEquippedItem();
		}
	}
}
