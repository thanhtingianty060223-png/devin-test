// Copyright Void Interactive, 2023

#include "ReadyOrNotLoadoutManager.h"

#include "Commander/MetaGameProfile.h"
#include "EnvironmentQuery/EnvQueryDebugHelpers.h"
#include "Info/LoadoutManager.h"

void UReadyOrNotLoadoutManager::Initialize(UWorld* World)
{
	if (!ensure(World))
		return;

	UReadyOrNotGameInstance* GameInstance = World->GetGameInstance<UReadyOrNotGameInstance>();
	if (!GameInstance)
		return;

	LoadoutManager = GameInstance->LoadoutManager;
}

FSavedLoadout UReadyOrNotLoadoutManager::GetActiveLoadout()
{
	return ActiveLoadout;
}

void UReadyOrNotLoadoutManager::SanitizeActiveLoadout()
{
	UBpGameplayHelperLib::SanitizeLoadout(ActiveLoadout);
	SanitizeSlotCounts();
}

FSavedLoadout UReadyOrNotLoadoutManager::GetLoadoutByName(FString LoadoutName)
{
	FSavedLoadout loadout;
	UBpGameplayHelperLib::LoadLoadout(loadout, LoadoutName);
	return loadout;
}

bool UReadyOrNotLoadoutManager::DeleteLoadout(FString LoadoutName)
{
	if(LoadoutName == "")
	{
		return true;
	}
	
	if(ActiveLoadout.Name == LoadoutName)
	{
		// TODO: Show error message, can't delete active preset
		return false;
	}
	
	UBpGameplayHelperLib::DeleteLoadout(LoadoutName);
	return true;
}

TArray<FName> UReadyOrNotLoadoutManager::GetUsableAmmoTypes(const ABaseWeapon* Weapon)
{
	if (!Weapon)
		return {};

	UDataTable* AmmoDataTable = Weapon->GetAmmoDataTable();
	if (!AmmoDataTable)
		return {};
	
	TArray<FName> AmmoTypes = Weapon->GetAmmunitionTypes();
	TArray<FName> UsableAmmoTypes;
	for (FName AmmoType : AmmoTypes)
	{
		FAmmoTypeData* AmmoTypeData = AmmoDataTable->FindRow<FAmmoTypeData>(AmmoType, "GetPrimaryAmmoTypes");
		if (!AmmoTypeData)
			continue;

		if (AmmoTypeData->bIsUsableByPlayer)
			UsableAmmoTypes.Add(AmmoType);
	}
		
	return UsableAmmoTypes;
}

void UReadyOrNotLoadoutManager::SetActiveLoadout(FSavedLoadout Loadout)
{
	ActiveLoadout = Loadout;
}

void UReadyOrNotLoadoutManager::SetActivePrimary(TSubclassOf<ABaseWeapon> Primary)
{
	ActiveLoadout.Primary = Primary;
	
	TArray<FName> AmmoTypes = GetPrimaryAmmoTypes();
	if (AmmoTypes.Num() > 0)
	{
		for (int32 i = 0; i < ActiveLoadout.PrimaryAmmoSlots.Num(); i++)
		{
			FName AmmoType = ActiveLoadout.PrimaryAmmoSlots[i];
			if (!AmmoTypes.Contains(AmmoType))
			{
				ActiveLoadout.PrimaryAmmoSlots[i] = AmmoTypes[0];
			}
		}
	}
	else
	{
		ActiveLoadout.PrimaryAmmoSlots.Empty();
		ActiveLoadout.PrimaryAmmoSlotsCount = 0;
	}
}

TSubclassOf<ABaseWeapon> UReadyOrNotLoadoutManager::GetActivePrimary()
{
	if (ActiveLoadout.Primary)
	{
		return ActiveLoadout.Primary.Get();
	}
	return nullptr;
}

TArray<FName> UReadyOrNotLoadoutManager::GetPrimaryAmmoTypes()
{
	const ABaseWeapon* BaseWeapon = Cast<ABaseWeapon>(ActiveLoadout.Primary.GetDefaultObject());
	return GetUsableAmmoTypes(BaseWeapon);
}

void UReadyOrNotLoadoutManager::SetActiveSecondary(TSubclassOf<ABaseWeapon> Secondary)
{
	ActiveLoadout.Secondary = Secondary;

	TArray<FName> AmmoTypes = GetSecondaryAmmoTypes();
	if (AmmoTypes.Num() > 0)
	{
		for (int32 i = 0; i < ActiveLoadout.SecondaryAmmoSlots.Num(); i++)
		{
			FName AmmoType = ActiveLoadout.SecondaryAmmoSlots[i];
			if (!AmmoTypes.Contains(AmmoType))
			{
				ActiveLoadout.SecondaryAmmoSlots[i] = AmmoTypes[0];
			}
		}
	}
	else
	{
		ActiveLoadout.SecondaryAmmoSlotsCount = 0;
	}
}

TSubclassOf<ABaseWeapon> UReadyOrNotLoadoutManager::GetActiveSecondary()
{
	if (ActiveLoadout.Secondary)
	{
		return ActiveLoadout.Secondary.Get();
	}
	return nullptr;
}

void UReadyOrNotLoadoutManager::SetActiveSecondarySkin(TSubclassOf<USkinComponent> Skin)
{
	ActiveLoadout.SecondarySkin = Skin;
}

TSubclassOf<UWeaponAttachment> UReadyOrNotLoadoutManager::GetActiveSecondaryAttachmentByType(
	EWeaponAttachmentType AttachmentType)
{
	if (AttachmentType == EWeaponAttachmentType::Optics)
	{
		return ActiveLoadout.SecondaryScope;
	}
	if (AttachmentType == EWeaponAttachmentType::Muzzle)
	{
		return ActiveLoadout.SecondaryMuzzle;
	}
	if (AttachmentType == EWeaponAttachmentType::Overbarrel)
	{
		return ActiveLoadout.SecondaryOverbarrel;
	}
	if (AttachmentType == EWeaponAttachmentType::Underbarrel)
	{
		return ActiveLoadout.SecondaryUnderbarrel;
	}
	if (AttachmentType == EWeaponAttachmentType::Stock)
	{
		return ActiveLoadout.SecondaryStock;
	}
	if (AttachmentType == EWeaponAttachmentType::Grip)
	{
		return ActiveLoadout.SecondaryGrip;
	}
	if (AttachmentType == EWeaponAttachmentType::Illuminators)
	{
		return ActiveLoadout.SecondaryIlluminator;
	}
	if (AttachmentType == EWeaponAttachmentType::Ammunition)
	{
		return ActiveLoadout.SecondaryAmmunition;
	}
	return nullptr;
}

TArray<FName> UReadyOrNotLoadoutManager::GetSecondaryAmmoTypes()
{
	const ABaseWeapon* BaseWeapon = Cast<ABaseWeapon>(ActiveLoadout.Secondary.GetDefaultObject());
	return GetUsableAmmoTypes(BaseWeapon);
}

FText UReadyOrNotLoadoutManager::GetAmmunitionDisplayName(FName AmmunitionName)
{
	const UDataTable* AmmoDataTable = UBpGameplayHelperLib::GetAmmoLookupDataTable();
	FAmmoTypeData* Row = AmmoDataTable->FindRow<FAmmoTypeData>(AmmunitionName, "Ammo Lookup Table");
	return Row->AmmoVariety;
}

FText UReadyOrNotLoadoutManager::GetAmmunitionCaliber(FName AmmunitionName)
{
	const UDataTable* AmmoDataTable = UBpGameplayHelperLib::GetAmmoLookupDataTable();
	FAmmoTypeData* Row = AmmoDataTable->FindRow<FAmmoTypeData>(AmmunitionName, "Ammo Lookup Table");
	return Row->AmmoCaliber;
}

FText UReadyOrNotLoadoutManager::GetAmmunitionDescription(FName AmmunitionName)
{
	const UDataTable* AmmoDataTable = UBpGameplayHelperLib::GetAmmoLookupDataTable();
	FAmmoTypeData* Row = AmmoDataTable->FindRow<FAmmoTypeData>(AmmunitionName, "Ammo Lookup Table");
	return Row->AmmoDescription;
}

void UReadyOrNotLoadoutManager::SetActiveLongTactical(TSubclassOf<ABaseItem> LongTactical)
{
	ActiveLoadout.LongTactical = LongTactical;
}

TSubclassOf<ABaseItem> UReadyOrNotLoadoutManager::GetActiveLongTactical()
{
	return ActiveLoadout.LongTactical;
}

TSubclassOf<UWeaponAttachment> UReadyOrNotLoadoutManager::GetActivePrimaryAttachmentByType(
	EWeaponAttachmentType AttachmentType)
{
	if (AttachmentType == EWeaponAttachmentType::Optics)
	{
		return ActiveLoadout.PrimaryScope;
	}
	if (AttachmentType == EWeaponAttachmentType::Muzzle)
	{
		return ActiveLoadout.PrimaryMuzzle;
	}
	if (AttachmentType == EWeaponAttachmentType::Overbarrel)
	{
		return ActiveLoadout.PrimaryOverbarrel;
	}
	if (AttachmentType == EWeaponAttachmentType::Underbarrel)
	{
		return ActiveLoadout.PrimaryUnderbarrel;
	}
	if (AttachmentType == EWeaponAttachmentType::Stock)
	{
		return ActiveLoadout.PrimaryStock;
	}
	if (AttachmentType == EWeaponAttachmentType::Grip)
	{
		return ActiveLoadout.PrimaryGrip;
	}
	if (AttachmentType == EWeaponAttachmentType::Illuminators)
	{
		return ActiveLoadout.PrimaryIlluminator;
	}
	if (AttachmentType == EWeaponAttachmentType::Ammunition)
	{
		return ActiveLoadout.PrimaryAmmunition;
	}
	return nullptr;
}

TArray<EWeaponAttachmentType> UReadyOrNotLoadoutManager::GetAvailableAttachmentTypesByWeapon(
	TSubclassOf<ABaseWeapon> BaseWeapon)
{
	const ABaseWeapon* DefaultObject = BaseWeapon.GetDefaultObject();
	TArray<EWeaponAttachmentType> AvailableAttachmentTypes;

	if (DefaultObject->bAcceptsScopeAttachments)
	{
		AvailableAttachmentTypes.Add(EWeaponAttachmentType::Optics);
	}
	if (DefaultObject->bAcceptsMuzzleAttachments)
	{
		AvailableAttachmentTypes.Add(EWeaponAttachmentType::Muzzle);
	}
	if (DefaultObject->bAcceptsUnderbarrelAttachments)
	{
		AvailableAttachmentTypes.Add(EWeaponAttachmentType::Underbarrel);
	}
	if (DefaultObject->bAcceptsOverbarrelAttachments)
	{
		AvailableAttachmentTypes.Add(EWeaponAttachmentType::Overbarrel);
	}
	if (DefaultObject->bAcceptsStockAttachments)
	{
		AvailableAttachmentTypes.Add(EWeaponAttachmentType::Stock);
	}
	if (DefaultObject->bAcceptsGripAttachments)
	{
		AvailableAttachmentTypes.Add(EWeaponAttachmentType::Grip);
	}
	if (DefaultObject->bAcceptsIlluminatorAttachments)
	{
		AvailableAttachmentTypes.Add(EWeaponAttachmentType::Illuminators);
	}
	if (DefaultObject->bAcceptsAmmunitionAttachments)
	{
		AvailableAttachmentTypes.Add(EWeaponAttachmentType::Ammunition);
	}

	return AvailableAttachmentTypes;
}

void UReadyOrNotLoadoutManager::SetActiveLoadoutFromPreset(FString LoadoutName)
{
	auto characterLookOverride = ActiveLoadout.CharacterLookOverride;
	UBpGameplayHelperLib::LoadLoadout(ActiveLoadout, FString(LoadoutName));
	ActiveLoadout.CharacterLookOverride = characterLookOverride;
	SwatMemberPreset[ActiveSwatMember] = LoadoutName;
}

void UReadyOrNotLoadoutManager::SetActiveLoadoutByName(FString LoadoutName)
{
	UBpGameplayHelperLib::LoadLoadout(ActiveLoadout, FString(LoadoutName));
}

bool UReadyOrNotLoadoutManager::ItemIsInActiveLoadout(TSubclassOf<ABaseItem> Item)
{
	bool InLoadout = false;
	if (ActiveLoadout.Primary == Item)
	{
		InLoadout = true;
	}
	else if (ActiveLoadout.Secondary == Item)
	{
		InLoadout = true;
	}
	else if (ActiveLoadout.Armor == Item)
	{
		InLoadout = true;
	}
	else if (ActiveLoadout.Helmet == Item)
	{
		InLoadout = true;
	}
	return InLoadout;
}

void UReadyOrNotLoadoutManager::SetPrimaryAttachment(TSubclassOf<UWeaponAttachment> WeaponAttachment,
                                                     EWeaponAttachmentType AttachmentType)
{
	if (AttachmentType == EWeaponAttachmentType::Optics)
	{
		ActiveLoadout.PrimaryScope = WeaponAttachment;
	}
	else if (AttachmentType == EWeaponAttachmentType::Muzzle)
	{
		ActiveLoadout.PrimaryMuzzle = WeaponAttachment;
	}
	else if (AttachmentType == EWeaponAttachmentType::Overbarrel)
	{
		ActiveLoadout.PrimaryOverbarrel = WeaponAttachment;
	}
	else if (AttachmentType == EWeaponAttachmentType::Underbarrel)
	{
		ActiveLoadout.PrimaryUnderbarrel = WeaponAttachment;
	}
	else if (AttachmentType == EWeaponAttachmentType::Stock)
	{
		ActiveLoadout.PrimaryStock = WeaponAttachment;
	}
	else if (AttachmentType == EWeaponAttachmentType::Grip)
	{
		ActiveLoadout.PrimaryGrip = WeaponAttachment;
	}
	else if (AttachmentType == EWeaponAttachmentType::Illuminators)
	{
		ActiveLoadout.PrimaryIlluminator = WeaponAttachment;
	}
	else if (AttachmentType == EWeaponAttachmentType::Ammunition)
	{
		ActiveLoadout.PrimaryAmmunition = WeaponAttachment;
	}
}

void UReadyOrNotLoadoutManager::SetSecondaryAttachment(TSubclassOf<UWeaponAttachment> WeaponAttachment,
                                                       EWeaponAttachmentType AttachmentType)
{
	if (AttachmentType == EWeaponAttachmentType::Optics)
	{
		ActiveLoadout.SecondaryScope = WeaponAttachment;
	}
	else if (AttachmentType == EWeaponAttachmentType::Muzzle)
	{
		ActiveLoadout.SecondaryMuzzle = WeaponAttachment;
	}
	else if (AttachmentType == EWeaponAttachmentType::Overbarrel)
	{
		ActiveLoadout.SecondaryOverbarrel = WeaponAttachment;
	}
	else if (AttachmentType == EWeaponAttachmentType::Underbarrel)
	{
		ActiveLoadout.SecondaryUnderbarrel = WeaponAttachment;
	}
	else if (AttachmentType == EWeaponAttachmentType::Stock)
	{
		ActiveLoadout.SecondaryStock = WeaponAttachment;
	}
	else if (AttachmentType == EWeaponAttachmentType::Grip)
	{
		ActiveLoadout.SecondaryGrip = WeaponAttachment;
	}
	else if (AttachmentType == EWeaponAttachmentType::Illuminators)
	{
		ActiveLoadout.SecondaryIlluminator = WeaponAttachment;
	}
	else if (AttachmentType == EWeaponAttachmentType::Ammunition)
	{
		ActiveLoadout.SecondaryAmmunition = WeaponAttachment;
	}
}

bool UReadyOrNotLoadoutManager::AttachmentIsEquipped(TSubclassOf<UWeaponAttachment> WeaponAttachment,
                                                     EWeaponAttachmentType AttachmentType)
{
	return false;
}

void UReadyOrNotLoadoutManager::SetActiveSwatMember(EEquippingSwat SwatMember)
{
	DoSaveActiveLoadout();
	ActiveSwatMember = SwatMember;
	UBpGameplayHelperLib::LoadLoadout(ActiveLoadout, SwatMemberPreset[SwatMember]);
}

EEquippingSwat UReadyOrNotLoadoutManager::GetActiveSwatMember()
{
	return ActiveSwatMember;
}

FString UReadyOrNotLoadoutManager::GetActiveSwatMemberLabel()
{
	if (ActiveSwatMember == EEquippingSwat::ES_RedOne)
	{
		return "ALPHA";
	}
	if (ActiveSwatMember == EEquippingSwat::ES_RedTwo)
	{
		return "BRAVO";
	}
	if (ActiveSwatMember == EEquippingSwat::ES_BlueOne)
	{
		return "CHARLIE";
	}
	if (ActiveSwatMember == EEquippingSwat::ES_BlueTwo)
	{
		return "DELTA";
	}
	return "SELF";
}

EEquippingSwat UReadyOrNotLoadoutManager::FStringToEquippingSwat(FString Name)
{
	if (Name.Equals("ALPHA"))
	{
		return EEquippingSwat::ES_RedOne;
	}
	if (Name.Equals("BRAVO"))
	{
		return EEquippingSwat::ES_RedTwo;
	}
	if (Name.Equals("CHARLIE"))
	{
		return EEquippingSwat::ES_BlueOne;
	}
	if (Name.Equals("DELTA"))
	{
		return EEquippingSwat::ES_BlueTwo;
	}
	return EEquippingSwat::ES_None;
}

EEquippingSwat UReadyOrNotLoadoutManager::NextActiveSwatMember()
{
	if (ActiveSwatMember == EEquippingSwat::ES_None)
	{
		ActiveSwatMember = EEquippingSwat::ES_RedOne;
	}
	else if (ActiveSwatMember == EEquippingSwat::ES_RedOne)
	{
		ActiveSwatMember = EEquippingSwat::ES_RedTwo;
	}
	else if (ActiveSwatMember == EEquippingSwat::ES_RedTwo)
	{
		ActiveSwatMember = EEquippingSwat::ES_BlueOne;
	}
	else if (ActiveSwatMember == EEquippingSwat::ES_BlueOne)
	{
		ActiveSwatMember = EEquippingSwat::ES_BlueTwo;
	}
	else if (ActiveSwatMember == EEquippingSwat::ES_BlueTwo)
	{
		ActiveSwatMember = EEquippingSwat::ES_None;
	}
	return ActiveSwatMember;
}

EEquippingSwat UReadyOrNotLoadoutManager::PreviousActiveSwatMember()
{
	if (ActiveSwatMember == EEquippingSwat::ES_None)
	{
		ActiveSwatMember = EEquippingSwat::ES_BlueTwo;
	}
	else if (ActiveSwatMember == EEquippingSwat::ES_BlueTwo)
	{
		ActiveSwatMember = EEquippingSwat::ES_BlueOne;
	}
	else if (ActiveSwatMember == EEquippingSwat::ES_BlueOne)
	{
		ActiveSwatMember = EEquippingSwat::ES_RedTwo;
	}
	else if (ActiveSwatMember == EEquippingSwat::ES_RedTwo)
	{
		ActiveSwatMember = EEquippingSwat::ES_RedOne;
	}
	else if (ActiveSwatMember == EEquippingSwat::ES_RedOne)
	{
		ActiveSwatMember = EEquippingSwat::ES_None;
	}
	return ActiveSwatMember;
}

void UReadyOrNotLoadoutManager::DoSaveActiveLoadout()
{
	DoSaveLoadout(ActiveSwatMember, ActiveLoadout);
}

void UReadyOrNotLoadoutManager::DoSaveLoadout(EEquippingSwat SwatMember, FSavedLoadout Loadout)
{
	UBpGameplayHelperLib::SanitizeLoadout(Loadout);
	if (Loadout.Primary && Loadout.Secondary)
	{
		V_LOGM(LogReadyOrNot, "Saving loadout Primary %s Secondary %s %d", *Loadout.Primary->GetName(),
		       *Loadout.Secondary->GetName(), SwatMember)
	}
	UBpGameplayHelperLib::SaveLoadout(Loadout, Loadout.Name);
}

void UReadyOrNotLoadoutManager::SetActivePrimarySkin(TSubclassOf<USkinComponent> Skin)
{
	ActiveLoadout.PrimarySkin = Skin;
}

TSubclassOf<ABaseItem> UReadyOrNotLoadoutManager::GetActiveBodyArmor()
{
	return ActiveLoadout.Armor;
}

void UReadyOrNotLoadoutManager::SetActiveBodyArmor(TSubclassOf<ABaseItem> Armor)
{
	ActiveLoadout.Armor = Armor;
	SanitizeSlotCounts();
}

TSubclassOf<ABaseItem> UReadyOrNotLoadoutManager::GetActiveHeadwear()
{
	return ActiveLoadout.Helmet;
}

void UReadyOrNotLoadoutManager::SetActiveHeadwear(TSubclassOf<ABaseItem> Headwear)
{
	ActiveLoadout.Helmet = Headwear;
}

int UReadyOrNotLoadoutManager::GetSlotCount(TSubclassOf<ABaseItem> SlotItem)
{
	int SlotCount = GetGrenadeSlotCount(SlotItem);
	if (SlotCount == 0)
	{
		SlotCount = GetTacticalSlotCount(SlotItem);
	}
	return SlotCount;
}

int UReadyOrNotLoadoutManager::GetPreviewSlotCount(FSavedLoadout PreviewLoadout, TSubclassOf<ABaseItem> SlotItem)
{
	int SlotCount = GetPreviewGrenadeSlotCount(PreviewLoadout, SlotItem);
	if (SlotCount == 0)
	{
		SlotCount = GetPreviewTacticalSlotCount(PreviewLoadout, SlotItem);
	}
	return SlotCount;
}

int UReadyOrNotLoadoutManager::GetCurrentSlotCount()
{
	int32 CurrentSlots = ActiveLoadout.GrenadeSlotsCount + ActiveLoadout.TacticalSlotsCount +
		ActiveLoadout.PrimaryAmmoSlotsCount + ActiveLoadout.SecondaryAmmoSlotsCount;

	if (const ASWATArmour* SWATArmourCDO = Cast<ASWATArmour>(ActiveLoadout.Armor.GetDefaultObject()))
	{
		CurrentSlots = FMath::Min(CurrentSlots, SWATArmourCDO->TotalSlots);
	}
	return CurrentSlots;
}

int UReadyOrNotLoadoutManager::GetMaximumSlotCount()
{
	int32 MaxSlots = 10;
	if (const ASWATArmour* SWATArmourCDO = Cast<ASWATArmour>(ActiveLoadout.Armor.GetDefaultObject()))
	{
		MaxSlots = SWATArmourCDO->TotalSlots;
	}
	return MaxSlots;
}

void UReadyOrNotLoadoutManager::SanitizeSlotCounts()
{
	ASWATArmour* SWATArmourCDO = Cast<ASWATArmour>(ActiveLoadout.Armor.GetDefaultObject());
	if (!SWATArmourCDO)
		return;
	
	ActiveLoadout.PrimaryAmmoSlotsCount = ActiveLoadout.PrimaryAmmoSlots.Num();
	ActiveLoadout.SecondaryAmmoSlotsCount = ActiveLoadout.SecondaryAmmoSlots.Num();
	ActiveLoadout.GrenadeSlotsCount = ActiveLoadout.GrenadeSlots.Num();
	ActiveLoadout.TacticalSlotsCount = ActiveLoadout.TacticalSlots.Num();

	int32 Slots = ActiveLoadout.GrenadeSlotsCount + ActiveLoadout.TacticalSlotsCount +
		ActiveLoadout.PrimaryAmmoSlotsCount + ActiveLoadout.SecondaryAmmoSlotsCount;
	
	// Take off slots in reverse order of importance
	int32 Remainder = FMath::Max(0, Slots - SWATArmourCDO->TotalSlots);
	if (Remainder > 0)
	{
		int32 TacticalSlotsCount = ActiveLoadout.TacticalSlotsCount;
		ActiveLoadout.TacticalSlotsCount = FMath::Max(0, TacticalSlotsCount - Remainder);
		
		Remainder -= TacticalSlotsCount;
	}
	if (Remainder > 0)
	{
		int32 GrenadeSlotsCount = ActiveLoadout.GrenadeSlotsCount;
		ActiveLoadout.GrenadeSlotsCount = FMath::Max(0, GrenadeSlotsCount - Remainder);

		Remainder -= GrenadeSlotsCount;
	}
	if (Remainder > 0)
	{
		int32 SecondaryAmmoCount = ActiveLoadout.SecondaryAmmoSlotsCount;
		ActiveLoadout.SecondaryAmmoSlotsCount = FMath::Max(0, SecondaryAmmoCount - Remainder);

		Remainder -= SecondaryAmmoCount;
	}
	if (Remainder > 0)
	{
		int32 PrimaryAmmoCount = ActiveLoadout.PrimaryAmmoSlotsCount;
		ActiveLoadout.PrimaryAmmoSlotsCount = FMath::Max(0, PrimaryAmmoCount - Remainder);

		Remainder -= PrimaryAmmoCount;
	}
}

int UReadyOrNotLoadoutManager::GetPrimarySlotCount(FName AmmoType)
{
	int32 ItemCount = 0;
	for (int32 i = 0; i < ActiveLoadout.PrimaryAmmoSlots.Num(); i++)
	{
		// Ignore entries that go beyond our current count, this helps us preserve slots when changing armor types
		if (i >= ActiveLoadout.PrimaryAmmoSlotsCount)
			break;

		if (AmmoType == ActiveLoadout.PrimaryAmmoSlots[i])
			ItemCount++;
	}
	return ItemCount;
}

int UReadyOrNotLoadoutManager::GetPreviewPrimarySlotCount(FSavedLoadout PreviewLoadout, FName AmmoType)
{
	int32 ItemCount = 0;
	for (int32 i = 0; i < PreviewLoadout.PrimaryAmmoSlots.Num(); i++)
	{
		// Ignore entries that go beyond our current count, this helps us preserve slots when changing armor types
		if (i >= PreviewLoadout.PrimaryAmmoSlotsCount)
			break;

		if (AmmoType == PreviewLoadout.PrimaryAmmoSlots[i])
			ItemCount++;
	}
	return ItemCount;
}


void UReadyOrNotLoadoutManager::IncrementPrimarySlotCount(FName AmmoType)
{
	if (GetMaximumSlotCount() > GetCurrentSlotCount())
	{
		ActiveLoadout.PrimaryAmmoSlots.Add(AmmoType);
		ActiveLoadout.PrimaryAmmoSlotsCount = ActiveLoadout.PrimaryAmmoSlots.Num();
	}
}

void UReadyOrNotLoadoutManager::DecrementPrimarySlotCount(FName AmmoType)
{
	if (ActiveLoadout.PrimaryAmmoSlots.Contains(AmmoType))
	{
		ActiveLoadout.PrimaryAmmoSlots.RemoveSingle(AmmoType);
		ActiveLoadout.PrimaryAmmoSlotsCount = ActiveLoadout.PrimaryAmmoSlots.Num();
	}
}

int UReadyOrNotLoadoutManager::GetSecondarySlotCount(FName AmmoType)
{
	int32 ItemCount = 0;
	for (int32 i = 0; i < ActiveLoadout.SecondaryAmmoSlots.Num(); i++)
	{
		// Ignore entries that go beyond our current count, this helps us preserve slots when changing armor types
		if (i >= ActiveLoadout.SecondaryAmmoSlotsCount)
			break;

		if (AmmoType == ActiveLoadout.SecondaryAmmoSlots[i])
			ItemCount++;
	}
	return ItemCount;
}

int UReadyOrNotLoadoutManager::GetPreviewSecondarySlotCount(FSavedLoadout PreviewLoadout, FName AmmoType)
{
	int32 ItemCount = 0;
	for (int32 i = 0; i < PreviewLoadout.SecondaryAmmoSlots.Num(); i++)
	{
		// Ignore entries that go beyond our current count, this helps us preserve slots when changing armor types
		if (i >= PreviewLoadout.SecondaryAmmoSlotsCount)
			break;

		if (AmmoType == PreviewLoadout.SecondaryAmmoSlots[i])
			ItemCount++;
	}
	return ItemCount;
}

void UReadyOrNotLoadoutManager::IncrementSecondarySlotCount(FName AmmoType)
{
	if (GetMaximumSlotCount() > GetCurrentSlotCount())
	{
		ActiveLoadout.SecondaryAmmoSlots.Add(AmmoType);
		ActiveLoadout.SecondaryAmmoSlotsCount = ActiveLoadout.SecondaryAmmoSlots.Num();
	}
}

void UReadyOrNotLoadoutManager::DecrementSecondarySlotCount(FName AmmoType)
{
	if (ActiveLoadout.SecondaryAmmoSlots.Contains(AmmoType))
	{
		ActiveLoadout.SecondaryAmmoSlots.RemoveSingle(AmmoType);
		ActiveLoadout.SecondaryAmmoSlotsCount = ActiveLoadout.SecondaryAmmoSlots.Num();
	}
}

int UReadyOrNotLoadoutManager::GetGrenadeSlotCount(TSubclassOf<ABaseItem> SlotItem)
{
	int32 ItemCount = 0;
	for (int32 i = 0; i < ActiveLoadout.GrenadeSlots.Num(); i++)
	{
		// Ignore entries that go beyond our current count, this helps us preserve slots when changing armor types
		if (i >= ActiveLoadout.GrenadeSlotsCount)
			break;

		if (SlotItem == ActiveLoadout.GrenadeSlots[i])
			ItemCount++;
	}
	return ItemCount;
}

int UReadyOrNotLoadoutManager::GetPreviewGrenadeSlotCount(FSavedLoadout PreviewLoadout, TSubclassOf<ABaseItem> SlotItem)
{
	int32 ItemCount = 0;
	for (int32 i = 0; i < PreviewLoadout.GrenadeSlots.Num(); i++)
	{
		// Ignore entries that go beyond our current count, this helps us preserve slots when changing armor types
		if (i >= PreviewLoadout.GrenadeSlotsCount)
			break;

		if (SlotItem == PreviewLoadout.GrenadeSlots[i])
			ItemCount++;
	}
	return ItemCount;
}

void UReadyOrNotLoadoutManager::IncrementGrenadeSlotCount(TSubclassOf<ABaseItem> SlotItem)
{
	if (GetMaximumSlotCount() > GetCurrentSlotCount())
	{
		ActiveLoadout.GrenadeSlots.Add(SlotItem);
		ActiveLoadout.GrenadeSlotsCount = ActiveLoadout.GrenadeSlots.Num();
	}
}

void UReadyOrNotLoadoutManager::DecrementGrenadeSlotCount(TSubclassOf<ABaseItem> SlotItem)
{
	if (ActiveLoadout.GrenadeSlots.Contains(SlotItem))
	{
		ActiveLoadout.GrenadeSlots.RemoveSingle(SlotItem);
		ActiveLoadout.GrenadeSlotsCount = ActiveLoadout.GrenadeSlots.Num();
	}
}

int UReadyOrNotLoadoutManager::GetTacticalSlotCount(TSubclassOf<ABaseItem> SlotItem)
{
	int32 ItemCount = 0;
	for (int32 i = 0; i < ActiveLoadout.TacticalSlots.Num(); i++)
	{
		// Ignore entries that go beyond our current count, this helps us preserve slots when changing armor types
		if (i >= ActiveLoadout.TacticalSlotsCount)
			break;

		if (SlotItem == ActiveLoadout.TacticalSlots[i])
			ItemCount++;
	}
	return ItemCount;
}

int UReadyOrNotLoadoutManager::GetPreviewTacticalSlotCount(FSavedLoadout PreviewLoadout, TSubclassOf<ABaseItem> SlotItem)
{
	int32 ItemCount = 0;
	for (int32 i = 0; i < PreviewLoadout.TacticalSlots.Num(); i++)
	{
		// Ignore entries that go beyond our current count, this helps us preserve slots when changing armor types
		if (i >= PreviewLoadout.TacticalSlotsCount)
			break;

		if (SlotItem == PreviewLoadout.TacticalSlots[i])
			ItemCount++;
	}
	return ItemCount;
}

void UReadyOrNotLoadoutManager::IncrementTacticalSlotCount(TSubclassOf<ABaseItem> SlotItem)
{
	if (GetMaximumSlotCount() > GetCurrentSlotCount())
	{
		ActiveLoadout.TacticalSlots.Add(SlotItem);
		ActiveLoadout.TacticalSlotsCount = ActiveLoadout.TacticalSlots.Num();
	}
}

void UReadyOrNotLoadoutManager::DecrementTacticalSlotCount(TSubclassOf<ABaseItem> SlotItem)
{
	if (ActiveLoadout.TacticalSlots.Contains(SlotItem))
	{
		ActiveLoadout.TacticalSlots.RemoveSingle(SlotItem);
		ActiveLoadout.TacticalSlotsCount = ActiveLoadout.TacticalSlots.Num();
	}
}

TArray<ABaseItem*> UReadyOrNotLoadoutManager::GetItemsByLoadoutCategory(ELoadoutCategory LoadoutCategory)
{
	if (!LoadoutManager)
		return {};
	
	TArray<ABaseItem*> LoadoutItems;
	if (LoadoutCategory == ELoadoutCategory::Primary)
	{
		LoadoutItems = LoadoutManager->PrimaryWeapons;
	}
	else if (LoadoutCategory == ELoadoutCategory::Secondary)
	{
		LoadoutItems = LoadoutManager->SecondaryWeapons;
	}
	else if (LoadoutCategory == ELoadoutCategory::LongTactical)
	{
		LoadoutItems = LoadoutManager->LongTacticalItems;
	}
	else if (LoadoutCategory == ELoadoutCategory::TacticalDevice)
	{
		LoadoutItems = LoadoutManager->TacticalItems;
	}
	return LoadoutItems;
}

TArray<ABaseItem*> UReadyOrNotLoadoutManager::GetBodyArmors()
{
	if (!LoadoutManager)
		return {};
	
	return LoadoutManager->BodyArmors;
}

TArray<ABaseItem*> UReadyOrNotLoadoutManager::GetHeadwears()
{
	if (!LoadoutManager)
		return {};
	
	return LoadoutManager->Helmets;
}

TSoftObjectPtr<UTexture2D> UReadyOrNotLoadoutManager::GetPrimaryWeaponImage(ABaseItem* Item)
{
	return Item->ItemIcon;
}

TSoftObjectPtr<UTexture2D> UReadyOrNotLoadoutManager::GetSecondaryWeaponImage(ABaseItem* Item)
{
	return Item->ItemIcon;
}

TSoftObjectPtr<UTexture2D> UReadyOrNotLoadoutManager::GetLongTacticalItemImage(ABaseItem* Item)
{
	return Item->ItemIcon;
}

TSoftObjectPtr<UTexture2D> UReadyOrNotLoadoutManager::GetBodyArmorItemImage(ABaseItem* Item)
{
	return Item->ItemIcon;
}

TSoftObjectPtr<UTexture2D> UReadyOrNotLoadoutManager::GetHeadwearItemImage(ABaseItem* Item)
{
	return Item->ItemIcon;
}

TArray<TSubclassOf<UWeaponAttachment>> UReadyOrNotLoadoutManager::GetAttachmentByWeaponAndType(
	ABaseWeapon* Weapon, EWeaponAttachmentType Type)
{
	if (Type == EWeaponAttachmentType::Optics)
	{
		return TArray<TSubclassOf<UWeaponAttachment>>(Weapon->AvailableScopeAttachments);
	}
	if (Type == EWeaponAttachmentType::Muzzle)
	{
		return TArray<TSubclassOf<UWeaponAttachment>>(Weapon->AvailableMuzzleAttachments);
	}
	if (Type == EWeaponAttachmentType::Underbarrel)
	{
		return TArray<TSubclassOf<UWeaponAttachment>>(Weapon->AvailableUnderbarrelAttachments);
	}
	if (Type == EWeaponAttachmentType::Overbarrel)
	{
		return TArray<TSubclassOf<UWeaponAttachment>>(Weapon->AvailableOverbarrelAttachments);
	}
	if (Type == EWeaponAttachmentType::Stock)
	{
		return TArray<TSubclassOf<UWeaponAttachment>>(Weapon->AvailableStockAttachments);
	}
	if (Type == EWeaponAttachmentType::Grip)
	{
		return TArray<TSubclassOf<UWeaponAttachment>>(Weapon->AvailableGripAttachments);
	}
	if (Type == EWeaponAttachmentType::Illuminators)
	{
		return TArray<TSubclassOf<UWeaponAttachment>>(Weapon->AvailableIlluminatorAttachments);
	}
	if (Type == EWeaponAttachmentType::Ammunition)
	{
		return TArray<TSubclassOf<UWeaponAttachment>>(Weapon->AvailableAmmunitionAttachments);
	}
	return TArray<TSubclassOf<UWeaponAttachment>>();
}

UWeaponAttachment* UReadyOrNotLoadoutManager::GetStoredAttachmentByWeaponAndType(TSubclassOf<ABaseWeapon> Weapon, EWeaponAttachmentType Type){
	FStoredWeaponAttachments AttachmentStruct = StoredAttachmentsByWeapon.FindRef(Weapon);
	if (AttachmentStruct.bIsEmpty)
		return nullptr;

	switch (Type)
	{
		case EWeaponAttachmentType::Null:
			return nullptr;
			break;
		case EWeaponAttachmentType::Optics:
			return AttachmentStruct.ScopeAttachment.GetDefaultObject();
			break;
		case EWeaponAttachmentType::Muzzle:
			return AttachmentStruct.MuzzleAttachment.GetDefaultObject();
			break;
		case EWeaponAttachmentType::Underbarrel:
			return AttachmentStruct.UnderbarrelAttachment.GetDefaultObject();
			break;
		case EWeaponAttachmentType::Overbarrel:
			return AttachmentStruct.OverbarrelAttachment.GetDefaultObject();
			break;
		case EWeaponAttachmentType::Stock:
			return AttachmentStruct.StockAttachment.GetDefaultObject();
			break;
		case EWeaponAttachmentType::Grip:
			return AttachmentStruct.GripAttachment.GetDefaultObject();
			break;
		case EWeaponAttachmentType::Illuminators:
			return AttachmentStruct.IlluminatorAttachment.GetDefaultObject();
			break;
		case EWeaponAttachmentType::Ammunition:
			return AttachmentStruct.AmmunitionAttachment.GetDefaultObject();
			break;
		default:
			return nullptr;
			break;
	}
}

EArmourCoverage UReadyOrNotLoadoutManager::GetArmorCoverage()
{
	return ActiveLoadout.ArmourCoverage;
}

void UReadyOrNotLoadoutManager::SetArmorCoverage(EArmourCoverage ArmorCoverage)
{
	ActiveLoadout.ArmourCoverage = ArmorCoverage;
}

FText UReadyOrNotLoadoutManager::GetArmorCoverageText(EArmourCoverage Coverage)
{
	if (Coverage == EArmourCoverage::AC_None)
	{
		return FText::FromString("NO ARMOR");
	}
	if (Coverage == EArmourCoverage::AC_FrontOnly)
	{
		return FText::FromString("FRONT ONLY");
	}
	if (Coverage == EArmourCoverage::AC_FrontBack)
	{
		return FText::FromString("FRONT & BACK");
	}
	return FText::FromString("FRONT, BACK & SIDES");
}

UArmourMaterial* UReadyOrNotLoadoutManager::GetActiveArmorMaterial()
{
	return ActiveLoadout.ArmourMaterial;
}

TArray<UArmourMaterial*> UReadyOrNotLoadoutManager::GetArmorMaterials()
{
	if (!LoadoutManager)
		return {};
	
	return LoadoutManager->ArmorMaterials;
}

void UReadyOrNotLoadoutManager::SetArmorMaterial(UArmourMaterial* ArmorMaterial)
{
	ActiveLoadout.ArmourMaterial = ArmorMaterial;
}

FText UReadyOrNotLoadoutManager::GetCurrentPresetDisplayName()
{
	return FText::FromString(GetActiveLoadout().PresetName);
}

TArray<FString> UReadyOrNotLoadoutManager::GetAllPresetNames() const
{
	UMetaGameProfile* MetaGameProfile = UMetaGameProfile::GetProfile(UBpGameplayHelperLib::GetWorldStatic());
	if (!MetaGameProfile)
		return {};

	TArray<FString> Keys;
	MetaGameProfile->LoadoutPresetMap.GetKeys(Keys);
	
	return Keys;
}

FSavedLoadout UReadyOrNotLoadoutManager::GetPreset(const FString& Name)
{
	if (Name.IsEmpty())
		return FSavedLoadout();
	
	UMetaGameProfile* MetaGameProfile = UMetaGameProfile::GetProfile(UBpGameplayHelperLib::GetWorldStatic());
	if (!MetaGameProfile)
		return FSavedLoadout();

	FSavedLoadout* SavedLoadout = MetaGameProfile->LoadoutPresetMap.Find(Name);
	if (!SavedLoadout)
		return FSavedLoadout();

	FString* SwatMemberPresetName = SwatMemberPreset.Find(ActiveSwatMember);
	if (!SwatMemberPresetName)
		return FSavedLoadout();
	
	SavedLoadout->Name = *SwatMemberPresetName;
	return *SavedLoadout;
}

void UReadyOrNotLoadoutManager::ApplyPreset(const FString& Name)
{
	FSavedLoadout SavedLoadout = GetPreset(Name);
	if (SavedLoadout.PresetName.IsEmpty())
		return;

	SetActiveLoadout(SavedLoadout);
	OnPresetApplied.Broadcast(Name, SavedLoadout);
}

void UReadyOrNotLoadoutManager::DeletePreset(const FString& Name)
{
	if (Name.IsEmpty())
		return;

	UMetaGameProfile* MetaGameProfile = UMetaGameProfile::GetProfile(UBpGameplayHelperLib::GetWorldStatic());
	if (!MetaGameProfile)
		return;

	int32 Removed = MetaGameProfile->LoadoutPresetMap.Remove(Name);
	MetaGameProfile->SaveProfile();
	
	if (Removed > 0)
		OnAvailablePresetsChanged.Broadcast();
}

void UReadyOrNotLoadoutManager::SavePreset(const FString& Name)
{
	if (Name.IsEmpty())
		return;

	UMetaGameProfile* MetaGameProfile = UMetaGameProfile::GetProfile(UBpGameplayHelperLib::GetWorldStatic());
	if (!MetaGameProfile)
		return;

	FSavedLoadout PresetLoadout = ActiveLoadout;
	PresetLoadout.PresetName = Name;

	bool bIsNew = !MetaGameProfile->LoadoutPresetMap.Contains(Name);
	
	MetaGameProfile->LoadoutPresetMap.Add(Name, PresetLoadout);
	MetaGameProfile->SaveProfile();

	if (bIsNew)
		OnAvailablePresetsChanged.Broadcast();
}

void UReadyOrNotLoadoutManager::ResetLoadoutToDefault()
{
	FString* SwatMemberDefaultLoadoutName = SwatMemberPreset.Find(ActiveSwatMember);
	if (!SwatMemberDefaultLoadoutName)
		return;
	
	FSavedLoadout DefaultLoadout;
	UBpGameplayHelperLib::LoadDefaultLoadout(DefaultLoadout, *SwatMemberDefaultLoadoutName);

	SetActiveLoadout(DefaultLoadout);
	OnPresetApplied.Broadcast(DefaultLoadout.PresetName, DefaultLoadout);
}
