// Void Interactive, 2020

#include "Animation/Notifies/AnimNotify_SpawnThrownItem.h"

void UAnimNotify_SpawnThrownItem::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::Notify(MeshComp, Animation);

	AReadyOrNotCharacter* Character = Cast<AReadyOrNotCharacter>(MeshComp->GetOwner());
	if (Character)
	{
		if (bLocalOnly && !Character->IsLocalPlayer())
			return;

		if (bNonLocalOnly && Character->IsLocalPlayer())
			return;
		
		if (ABaseItem* Item = Character->GetInventoryComponent()->GetInventoryItemOfClass(ItemClass))
		{
			const USkeletalMeshComponent* MeshCompToUse = MeshComp;

			if (const APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(Character))
				MeshCompToUse = (PlayerCharacter->IsLocalPlayer() ? PlayerCharacter->GetMesh1P() : MeshComp);

			Item->SpawnThrownItemAtTransform(MeshCompToUse->GetSocketTransform(BoneToSpawnOn), bCustomThrowDirection ? ThrowDirection.Vector() : Character->GetControlRotation().Vector());

			Character->OnItemThrown_FromAnimNotify.Broadcast(Item);
		}
	}
}
