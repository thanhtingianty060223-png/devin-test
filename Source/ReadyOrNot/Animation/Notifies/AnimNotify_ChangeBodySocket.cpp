// Void Interactive, 2020


#include "AnimNotify_ChangeBodySocket.h"

#include "Components/InventoryComponent.h"

void UAnimNotify_ChangeBodySocket::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{

	AReadyOrNotCharacter* Character = Cast<AReadyOrNotCharacter>(MeshComp->GetOwner());
	if (Character)
	{
		for (ABaseItem* Item : Character->GetInventoryComponent()->GetInventoryItems())
		{
			if (Item && Item != Character->GetEquippedItem())
			{
				if (Item->ContainsItemCategory(ItemCategory))
				{
					if (Socket.IsNone())
					{
						Socket = Item->GetClass()->GetDefaultObject<ABaseItem>()->BodySocket;
					}
					Item->AttachToComponent(Character->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, Socket);
				}
			
			}
		}
	}
}
