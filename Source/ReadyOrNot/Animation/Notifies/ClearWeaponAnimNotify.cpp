// Copyright Void Interactive, 2021

#include "ClearWeaponAnimNotify.h"

void UClearWeaponAnimNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (ABaseItem* const Item = Cast< ABaseItem>(MeshComp->GetOwner()))
	{
		Item->bHasBeenCleared = true;
		Item->ItemMesh->HideBoneByName("tag_mag_01", PBO_Term);
		Item->bNoPickup = true;
	}
}
