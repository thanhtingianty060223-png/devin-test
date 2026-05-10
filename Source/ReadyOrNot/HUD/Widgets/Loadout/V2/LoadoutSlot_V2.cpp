// Copyright Void Interactive, 2023

#include "HUD/Widgets/Loadout/V2/LoadoutSlot_V2.h"
#include "lib/ReadyOrNotLoadoutManager.h"

#include "Components/TextBlock.h"

void ULoadoutSlot_V2::NativeConstruct()
{
	Super::NativeConstruct();
	const AReadyOrNotGameState* GameState = Cast<AReadyOrNotGameState>(GetWorld()->GetGameState());
	LoadoutFunctionLibrary = GameState->LoadoutFunctionLibrary;
	ItemImage = EmptyImage;
	RefreshInfo();
}

void ULoadoutSlot_V2::SetEquipped(bool IsEquipped)
{
	bEquipped = IsEquipped;
	OnEquipped();
}

bool ULoadoutSlot_V2::GetEquipped()
{
	return bEquipped;
}

void ULoadoutSlot_V2::SetItem(ABaseItem* Item)
{
	BaseItem = Item;
	bAttachment = false;
	if (BaseItem)
	{
		if (ABaseWeapon* Weapon = Cast<ABaseWeapon>(BaseItem))
		{
			BaseWeapon = Weapon;
		}
		else if (ABaseArmour* Armor = Cast<ABaseArmour>(BaseItem))
		{
			BaseArmor = Armor;
		}

		ItemName = BaseItem->ItemName;
		ItemImage = BaseItem->ItemIcon.LoadSynchronous();
		BaseItemClass = BaseItem->GetClass();
	}

	RefreshInfo();
	RefreshItemImage();
	
	//if (BaseItem)
	//{
	//	ItemName = BaseItem->ItemName;
	//	ItemType = FText::FromString(ItemClassEnumToString(BaseItem->ItemClass));
	//}
}

void ULoadoutSlot_V2::SetAttachment(UWeaponAttachment* WeaponAttachment)
{
	Attachment = WeaponAttachment;
	bAttachment = true;
	if (Attachment)
	{
		ItemName = Attachment->ItemName;
		ItemImage = WeaponAttachment->UIElements.AttachmentIcon.LoadSynchronous();
		//AttachmentType = Attachment->WeaponAttachmentType;
	}

	RefreshAttachmentImage();
	RefreshInfo();
}

TSubclassOf<UWeaponAttachment> ULoadoutSlot_V2::GetAttachmentClass()
{
	return Attachment ? Attachment->GetClass() : nullptr;
}

void ULoadoutSlot_V2::SetAmmoMunition(FName AmmoName, bool Secondary)
{
	AmmunitionName = AmmoName;
	const AReadyOrNotGameState* GameState = Cast<AReadyOrNotGameState>(GetWorld()->GetGameState());
	if (GameState)
	{
		if (Secondary)
		{
			MunitionType = ELoadoutMunitionSlotType::SecondaryAmmunition;
			SlotCount = GameState->LoadoutFunctionLibrary->GetSecondarySlotCount(AmmunitionName);
		}
		else
		{
			MunitionType = ELoadoutMunitionSlotType::PrimaryAmmunition;
			SlotCount = GameState->LoadoutFunctionLibrary->GetPrimarySlotCount(AmmunitionName);
		}
	}
	const UDataTable* AmmoDataTable = UBpGameplayHelperLib::GetAmmoLookupDataTable();
	FAmmoTypeData* AmmoData = AmmoDataTable->FindRow<FAmmoTypeData>(AmmunitionName, "Ammo Lookup Table");
	if (AmmoData)
	{
		ItemName = AmmoData->AmmoVariety;
		ItemImage = AmmoData->LoadoutIcon;
		ItemSubtext = AmmoData->AmmoCaliber;
	}

	OnSlotsUpdated();
}

void ULoadoutSlot_V2::SetTacticalMunition(ELoadoutMunitionSlotType Munition, ABaseItem* TacticalItem)
{
	MunitionType = Munition;
	BaseItem = TacticalItem;
	if (BaseItem)
	{
		BaseItemClass = BaseItem->GetClass();
		ItemName = BaseItem->ItemName;

		const AReadyOrNotGameState* GameState = Cast<AReadyOrNotGameState>(GetWorld()->GetGameState());
		//if (ItemData->TacticalItems.ContainsByPredicate([&](const FDeviceData& WeaponData)
		//{
		//	if (WeaponData.Blueprint == BaseItemClass)
		//	{
		//		ItemImage = WeaponData.Image.LoadSynchronous();
		//		return true;
		//	}
		//	return false;
		//}))
		if (GameState)
		{
			if (MunitionType == ELoadoutMunitionSlotType::TacticalSlot)
			{
				//GameState->LoadoutFunctionLibrary->IncrementTacticalSlotCount(BaseItemClass);
				SlotCount = GameState->LoadoutFunctionLibrary->GetSlotCount(BaseItemClass);
			}
			else if (MunitionType == ELoadoutMunitionSlotType::GrenadeSlot)
			{
				//GameState->LoadoutFunctionLibrary->IncrementGrenadeSlotCount(BaseItemClass);
				SlotCount = GameState->LoadoutFunctionLibrary->GetSlotCount(BaseItemClass);
			}
		}
	}

	OnSlotsUpdated();
}

void ULoadoutSlot_V2::IncrementSlots()
{
	const AReadyOrNotGameState* GameState = Cast<AReadyOrNotGameState>(GetWorld()->GetGameState());
	if (GameState)
	{
		if (MunitionType == ELoadoutMunitionSlotType::PrimaryAmmunition)
		{
			GameState->LoadoutFunctionLibrary->IncrementPrimarySlotCount(AmmunitionName);
			SlotCount = GameState->LoadoutFunctionLibrary->GetPrimarySlotCount(AmmunitionName);
		}
		else if (MunitionType == ELoadoutMunitionSlotType::SecondaryAmmunition)
		{
			GameState->LoadoutFunctionLibrary->IncrementSecondarySlotCount(AmmunitionName);
			SlotCount = GameState->LoadoutFunctionLibrary->GetSecondarySlotCount(AmmunitionName);
		}
		else if (MunitionType == ELoadoutMunitionSlotType::TacticalSlot)
		{
			GameState->LoadoutFunctionLibrary->IncrementTacticalSlotCount(BaseItemClass);
			SlotCount = GameState->LoadoutFunctionLibrary->GetSlotCount(BaseItemClass);
		}
		else if (MunitionType == ELoadoutMunitionSlotType::GrenadeSlot)
		{
			GameState->LoadoutFunctionLibrary->IncrementGrenadeSlotCount(BaseItemClass);
			SlotCount = GameState->LoadoutFunctionLibrary->GetSlotCount(BaseItemClass);
		}
	}

	OnSlotsUpdated();
}

void ULoadoutSlot_V2::DecrementSlots()
{
	const AReadyOrNotGameState* GameState = Cast<AReadyOrNotGameState>(GetWorld()->GetGameState());
	if (GameState)
	{
		if (MunitionType == ELoadoutMunitionSlotType::PrimaryAmmunition)
		{
			GameState->LoadoutFunctionLibrary->DecrementPrimarySlotCount(AmmunitionName);
			SlotCount = GameState->LoadoutFunctionLibrary->GetPrimarySlotCount(AmmunitionName);
		}
		else if (MunitionType == ELoadoutMunitionSlotType::SecondaryAmmunition)
		{
			GameState->LoadoutFunctionLibrary->DecrementSecondarySlotCount(AmmunitionName);
			SlotCount = GameState->LoadoutFunctionLibrary->GetSecondarySlotCount(AmmunitionName);
		}
		else if (MunitionType == ELoadoutMunitionSlotType::TacticalSlot)
		{
			GameState->LoadoutFunctionLibrary->DecrementTacticalSlotCount(BaseItemClass);
			SlotCount = GameState->LoadoutFunctionLibrary->GetSlotCount(BaseItemClass);
		}
		else if (MunitionType == ELoadoutMunitionSlotType::GrenadeSlot)
		{
			GameState->LoadoutFunctionLibrary->DecrementGrenadeSlotCount(BaseItemClass);
			SlotCount = GameState->LoadoutFunctionLibrary->GetSlotCount(BaseItemClass);
		}
	}

	OnSlotsUpdated();
}

void ULoadoutSlot_V2::UpdateSlotCount()
{
	AReadyOrNotGameState* GameState = Cast<AReadyOrNotGameState>(GetWorld()->GetGameState());
	if (!GameState || !GameState->LoadoutFunctionLibrary)
		return;

	UReadyOrNotLoadoutManager* LoadoutManager = GameState->LoadoutFunctionLibrary;
	
	if (MunitionType == ELoadoutMunitionSlotType::PrimaryAmmunition)
	{
		SlotCount = LoadoutManager->GetPrimarySlotCount(AmmunitionName);
	}
	else if (MunitionType == ELoadoutMunitionSlotType::SecondaryAmmunition)
	{
		SlotCount = LoadoutManager->GetSecondarySlotCount(AmmunitionName);
	}
	else if (MunitionType == ELoadoutMunitionSlotType::TacticalSlot)
	{
		SlotCount = LoadoutManager->GetSlotCount(BaseItemClass);
	}
	else if (MunitionType == ELoadoutMunitionSlotType::GrenadeSlot)
	{
		SlotCount = LoadoutManager->GetSlotCount(BaseItemClass);
	}
}

//void ULoadoutSlot_V2::SetArmorMaterial(UArmourMaterial* Armor)
//{
//	ArmorMaterial = Armor;
//	if (ArmorMaterial)
//	{
//		ItemName = ArmorMaterial->DisplayName.ToUpper();
//		ItemType = FText::FromString("ARMOR MATERIAL");
//	}
//	RefreshItemInfo();
//}

//void ULoadoutSlot_V2::RefreshItemInfo()
//{
//	// Maybe this should go in the loadout function lib.
//	// if (ABaseWeapon* Weapon = Cast<ABaseWeapon>(BaseItem))
//	// {
//	// 	if (Weapon->MuzzleAttachment && Weapon->MuzzleAttachment->bShouldSupressWeapon)
//	// 	{
//	// 		ShowInfo(FText::FromString("SUPPRESSED"), true);
//	// 	}
//	// 	else
//	// 	{
//	// 		ShowInfo(FText::FromString(""), false);
//	// 	}
//	// }
//	if (ASWATArmour* Armour = Cast<ASWATArmour>(BaseItem))
//	{
//		ShowInfo(FText::Format(FText::FromString("{0} SLOTS"), Armour->TotalSlots), true);
//	}
//	else
//	{
//		ShowInfo(FText::FromString(""), false);
//	}
//}
