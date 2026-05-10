// Void Interactive, 2020

#include "AnimNotify_ToggleEquipmentVis.h"

//#include "Components/InventoryComponent.h"

void UAnimNotify_ToggleEquipmentVis::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	/*
	APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(MeshComp->GetOwner());
	if (PlayerCharacter)
	{
		switch(InventroyVis)
		{
			
			case EToggleInventoryVis::TIV_None: break;
			case EToggleInventoryVis::TIV_HideAll:
				for (ABaseItem* Item : PlayerCharacter->GetInventoryComponent()->GetInventoryItems())
				{
					if (Item)
					{
						Item->bForceInvisible = true;
					}
				}
				break;
			case EToggleInventoryVis::TIV_ShowAll:
				for (ABaseItem* Item : PlayerCharacter->GetInventoryComponent()->GetInventoryItems())
				{
					if (Item)
					{
						Item->bForceInvisible = false;
					}
				}
				break;
			case EToggleInventoryVis::TIV_HideEquipped:
				if (PlayerCharacter->GetEquippedItem())
				{
					PlayerCharacter->GetEquippedItem()->bForceInvisible = true;
				}
				break;
			case EToggleInventoryVis::TIV_ShowEquipped:
				for (ABaseItem* Item : PlayerCharacter->GetInventoryComponent()->GetInventoryItems())
				{
					if (Item)
					{
						Item->bForceInvisible = false;
					}
				}
				break;
			default: ;
		}
	}
	*/
}
