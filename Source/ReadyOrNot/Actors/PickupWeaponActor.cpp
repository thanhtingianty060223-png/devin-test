// Copyright Void Interactive, 2017

#include "PickupWeaponActor.h"
#include "ReadyOrNot.h"

APickupWeaponActor::APickupWeaponActor()
{
	RootComponent = SkeletalMesh;
	SkeletalMesh->SetVisibility(true);
}

void APickupWeaponActor::BeginPlay()
{
	Super::BeginPlay();
}

void APickupWeaponActor::ActorPickedUp(AActor* InPickupInstigator)
{
	Super::ActorPickedUp(InPickupInstigator);

	// Unequip the previous weapon which had this category, equip the new weapon in that category, and then swap to that item
	APlayerCharacter* pc = Cast<APlayerCharacter>(InPickupInstigator);
	if (!pc)
	{
		return;
	}
	
	/*
	 * TODO: Reimplement this
	 *ABaseItem* ItemActor = GetWorld()->SpawnActor<ABaseItem>(Weapon.Get());
	pc->GetInventoryComponent()->AddInventoryItem(ItemActor);

	ABaseItem* Item = Cast<ABaseItem>(ItemActor);
	ABaseWeapon* SpawnedWeapon = Cast<ABaseWeapon>(ItemActor);

	if (SpawnedWeapon != nullptr)
	{
		if (ScopeAttachment != nullptr)
		{
			SpawnedWeapon->AddAttachment(ScopeAttachment.Get());
		}

		if (MuzzleAttachment != nullptr)
		{
			SpawnedWeapon->AddAttachment(MuzzleAttachment.Get());
		}

		if (UnderbarrelAttachment != nullptr)
		{
			SpawnedWeapon->AddAttachment(UnderbarrelAttachment.Get());
		}

		if (OverbarrelAttachment != nullptr)
		{
			SpawnedWeapon->AddAttachment(OverbarrelAttachment.Get());
		}
	}

	if (bModifyLoadout)
	{
		if (bSecondaryWeapon)
		{
			pc->GetInventoryComponent()->GetSpawnedGear().Secondary = Item;
		}
		else
		{
			pc->GetInventoryComponent()->GetSpawnedGear().Primary = Item;
		}
	}
	
	pc->Holster(Item, true);

	if (bKillOnPickup)
	{
		Destroyed();
	}*/
}

