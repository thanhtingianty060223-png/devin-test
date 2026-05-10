// Copyright Void Interactive, 2023

#include "CustomizationWidget.h"

#include "Commander/MetaGameProfile.h"
#include "Commander/RosterManager.h"
#include "GameModes/LobbyGM.h"
#include "Info/LoadoutManager.h"

TArray<EWeaponAttachmentType> ULoadoutWeapon::GetSupportedAttachmentTypes() const
{
	if (!Class)
		return {};

	ABaseWeapon* WeaponCDO = Class->GetDefaultObject<ABaseWeapon>();
	if (!ensure(WeaponCDO))
		return {};
	
	TArray<EWeaponAttachmentType> SupportedTypes;
	if (WeaponCDO->bAcceptsScopeAttachments)
		SupportedTypes.Add(EWeaponAttachmentType::Optics);
	if (WeaponCDO->bAcceptsMuzzleAttachments)
		SupportedTypes.Add(EWeaponAttachmentType::Muzzle);
	if (WeaponCDO->bAcceptsUnderbarrelAttachments)
		SupportedTypes.Add(EWeaponAttachmentType::Underbarrel);
	if (WeaponCDO->bAcceptsOverbarrelAttachments)
		SupportedTypes.Add(EWeaponAttachmentType::Overbarrel);
	if (WeaponCDO->bAcceptsStockAttachments)
		SupportedTypes.Add(EWeaponAttachmentType::Stock);
	if (WeaponCDO->bAcceptsGripAttachments)
		SupportedTypes.Add(EWeaponAttachmentType::Grip);
	if (WeaponCDO->bAcceptsIlluminatorAttachments)
		SupportedTypes.Add(EWeaponAttachmentType::Illuminators);
	if (WeaponCDO->bAcceptsAmmunitionAttachments)
		SupportedTypes.Add(EWeaponAttachmentType::Ammunition);

	return SupportedTypes;
}

TArray<TSubclassOf<UWeaponAttachment>> ULoadoutWeapon::GetSupportedAttachmentsOfType(EWeaponAttachmentType Type) const
{
	if (!Class)
		return {};

	ABaseWeapon* WeaponCDO = Class->GetDefaultObject<ABaseWeapon>();
	if (!ensure(WeaponCDO))
		return {};

	TArray<TSubclassOf<UWeaponAttachment>> ScopeAttachments;
	switch (Type)
	{
	case EWeaponAttachmentType::Optics:
		// TODO(killo): we can probably make the attachment array UWeaponAttachment and remove this conversion
		for (const TSubclassOf<UScopedWeaponAttachment>& Attachment : WeaponCDO->AvailableScopeAttachments)
		{
			ScopeAttachments.Add(Attachment);
		}
		return ScopeAttachments;
	case EWeaponAttachmentType::Muzzle: return WeaponCDO->AvailableMuzzleAttachments;
	case EWeaponAttachmentType::Underbarrel: return WeaponCDO->AvailableUnderbarrelAttachments;
	case EWeaponAttachmentType::Overbarrel: return WeaponCDO->AvailableOverbarrelAttachments;
	case EWeaponAttachmentType::Stock: return WeaponCDO->AvailableStockAttachments;
	case EWeaponAttachmentType::Grip: return WeaponCDO->AvailableGripAttachments;
	case EWeaponAttachmentType::Illuminators: return WeaponCDO->AvailableIlluminatorAttachments;
	case EWeaponAttachmentType::Ammunition: return WeaponCDO->AvailableAmmunitionAttachments;
	default: return {};
	}
}

void UCustomizationWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!GetWorld())
		return;
	
	UReadyOrNotGameInstance* GameInstance = GetWorld()->GetGameInstance<UReadyOrNotGameInstance>();
	if (!GameInstance || !GameInstance->LoadoutManager)
		return;
	
	LoadoutManager = GameInstance->LoadoutManager;
	Profile = UBaseProfile::GetCurrentProfile();

	SetActiveSwatMember(EEquippingSwat::ES_None);
	
	CacheLoadoutItems();
	CacheCustomizationItems();

	for (TActorIterator<AReadyOrNotCharacter> It(GetWorld()); It; ++It)
	{
		AReadyOrNotCharacter* Character = *It;
		if (Character->Tags.Contains("PreviewCharacter"))
		{
			PreviewCharacter = Character;
			break;
		}
	}

	if (PreviewCharacter && PreviewCharacter->GetInventoryComponent())
	{
		PreviewCharacter->GetInventoryComponent()->OnInventoryItemsChanged.AddUObject(this, &UCustomizationWidget::HandlePreviewCharacterItemsChanged);
	}

	// Hack to wait for spawned gear
	GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [&]()
	{
		ApplyCustomization(CurrentCustomization);
	}));
}

void UCustomizationWidget::NativeDestruct()
{
	Super::NativeDestruct();

	AReadyOrNotPlayerState* PlayerState = GetOwningPlayerState<AReadyOrNotPlayerState>();
	if (PlayerState && Profile)
	{
		FSavedCustomization* Customization = Profile->Customizations.Find(EEquippingSwat::ES_None);
		if (Customization)
		{
			PlayerState->Server_SetCustomization(*Customization);
		}
	}
}

template<typename T>
T* GetLoadoutItemMatching(TSubclassOf<ABaseItem> Class, const TArray<T*>& Items)
{
	for (T* Item : Items)
	{
		if (Item->Class == Class)
			return Item;
	}
	return nullptr;
}

ULoadoutWeapon* UCustomizationWidget::GetPrimaryWeapon() const
{
	return Cast<ULoadoutWeapon>(GetLoadoutItemMatching<ULoadoutWeapon>(CurrentLoadout.Primary, CachedPrimaryWeapons));
}

ULoadoutWeapon* UCustomizationWidget::GetSecondaryWeapon() const
{
	return Cast<ULoadoutWeapon>(GetLoadoutItemMatching<ULoadoutWeapon>(CurrentLoadout.Secondary, CachedSecondaryWeapons));
}

ULoadoutEquipment* UCustomizationWidget::GetLongTactical() const
{
	return Cast<ULoadoutEquipment>(GetLoadoutItemMatching<ULoadoutEquipment>(CurrentLoadout.LongTactical, CachedLongTacticalItems));
}

ULoadoutEquipment* UCustomizationWidget::GetArmor() const
{
	return Cast<ULoadoutEquipment>(GetLoadoutItemMatching<ULoadoutEquipment>(CurrentLoadout.Armor, CachedArmorItems));
}

ULoadoutEquipment* UCustomizationWidget::GetHelmet() const
{
	return Cast<ULoadoutEquipment>(GetLoadoutItemMatching<ULoadoutEquipment>(CurrentLoadout.Helmet, CachedHelmetItems));
}

TSubclassOf<UWeaponAttachment>* GetPrimaryAttachmentSlot(FSavedLoadout& Loadout, EWeaponAttachmentType Type)
{
	switch (Type)
	{
	case EWeaponAttachmentType::Optics: return &Loadout.PrimaryScope;
	case EWeaponAttachmentType::Muzzle: return &Loadout.PrimaryMuzzle;
	case EWeaponAttachmentType::Underbarrel: return &Loadout.PrimaryUnderbarrel;
	case EWeaponAttachmentType::Overbarrel: return &Loadout.PrimaryOverbarrel;
	case EWeaponAttachmentType::Stock: return &Loadout.PrimaryStock;
	case EWeaponAttachmentType::Grip: return &Loadout.PrimaryGrip;
	case EWeaponAttachmentType::Illuminators: return &Loadout.PrimaryIlluminator;
	case EWeaponAttachmentType::Ammunition: return &Loadout.PrimaryAmmunition;
	default: return nullptr;
	}
}

TSubclassOf<UWeaponAttachment>* GetSecondaryAttachmentSlot(FSavedLoadout& Loadout, EWeaponAttachmentType Type)
{
	switch (Type)
	{
	case EWeaponAttachmentType::Optics: return &Loadout.SecondaryScope;
	case EWeaponAttachmentType::Muzzle: return &Loadout.SecondaryMuzzle;
	case EWeaponAttachmentType::Underbarrel: return &Loadout.SecondaryUnderbarrel;
	case EWeaponAttachmentType::Overbarrel: return &Loadout.SecondaryOverbarrel;
	case EWeaponAttachmentType::Stock: return &Loadout.SecondaryStock;
	case EWeaponAttachmentType::Grip: return &Loadout.SecondaryGrip;
	case EWeaponAttachmentType::Illuminators: return &Loadout.SecondaryIlluminator;
	case EWeaponAttachmentType::Ammunition: return &Loadout.SecondaryAmmunition;
	default: return nullptr;
	}
}

bool CheckAttachmentSupported(UWeaponAttachment* Attachment, ABaseWeapon* WeaponCDO)
{
	bool bAttachmentSupported = false;
	switch (Attachment->WeaponAttachmentType)
	{
	case EWeaponAttachmentType::Optics: return WeaponCDO->AvailableScopeAttachments.Contains(Attachment->GetClass());
	case EWeaponAttachmentType::Muzzle: return WeaponCDO->AvailableMuzzleAttachments.Contains(Attachment->GetClass());
	case EWeaponAttachmentType::Underbarrel: return WeaponCDO->AvailableUnderbarrelAttachments.Contains(Attachment->GetClass());
	case EWeaponAttachmentType::Overbarrel: return WeaponCDO->AvailableOverbarrelAttachments.Contains(Attachment->GetClass());
	case EWeaponAttachmentType::Stock: return WeaponCDO->AvailableStockAttachments.Contains(Attachment->GetClass());
	case EWeaponAttachmentType::Grip: return WeaponCDO->AvailableGripAttachments.Contains(Attachment->GetClass());
	case EWeaponAttachmentType::Illuminators: return WeaponCDO->AvailableIlluminatorAttachments.Contains(Attachment->GetClass());
	case EWeaponAttachmentType::Ammunition: return WeaponCDO->AvailableAmmunitionAttachments.Contains(Attachment->GetClass());
	default: return false;
	}
}

void UCustomizationWidget::SetPrimaryWeapon(ULoadoutWeapon* Item)
{
	CurrentLoadout.Primary = Item ? Item->Class : nullptr;
	PrimarySet(CurrentLoadout.Primary);
}

void UCustomizationWidget::SetSecondaryWeapon(ULoadoutWeapon* Item)
{
	CurrentLoadout.Secondary = Item ? Item->Class : nullptr;
	SecondarySet(CurrentLoadout.Secondary);
}

void UCustomizationWidget::SetLongTactical(ULoadoutEquipment* Item)
{
	CurrentLoadout.LongTactical = Item ? Item->Class : nullptr;
}

void UCustomizationWidget::SetArmor(ULoadoutEquipment* Item)
{
	CurrentLoadout.Armor = Item ? Item->Class : nullptr;
	ArmorSet(CurrentLoadout.Armor);
}

void UCustomizationWidget::SetHelmet(ULoadoutEquipment* Item)
{
	CurrentLoadout.Helmet = Item ? Item->Class : nullptr;
	HelmetSet(CurrentLoadout.Helmet);
}

void UCustomizationWidget::SetPrimaryAttachment(UWeaponAttachment* Attachment)
{
	if (!Attachment)
		return;
	
	if (!CurrentLoadout.Primary)
		return;

	ABaseWeapon* WeaponCDO = CurrentLoadout.Primary->GetDefaultObject<ABaseWeapon>();
	if (!WeaponCDO)
		return;

	UWeaponAttachment* AttachmentCDO = Attachment->GetClass()->GetDefaultObject<UWeaponAttachment>();
	if (!ensure(AttachmentCDO))
		return;
	
	const EWeaponAttachmentType AttachmentType = AttachmentCDO->WeaponAttachmentType;
	const TArray<EWeaponAttachmentType>& RemovesWeaponAttachmentTypes = AttachmentCDO->RemovesWeaponAttachmentTypes;

	bool bAttachmentSupported = CheckAttachmentSupported(Attachment, WeaponCDO);
	if (!bAttachmentSupported)
		return;
	
	for (EWeaponAttachmentType RemoveType : RemovesWeaponAttachmentTypes)
	{
		TSubclassOf<UWeaponAttachment>* AttachmentSlot = GetPrimaryAttachmentSlot(CurrentLoadout, RemoveType);
		if (AttachmentSlot)
			AttachmentSlot = nullptr;
	}

	TSubclassOf<UWeaponAttachment>* AttachmentSlot = GetPrimaryAttachmentSlot(CurrentLoadout, AttachmentType);
	if (AttachmentSlot)
		*AttachmentSlot = Attachment->GetClass();
}

void UCustomizationWidget::SetSecondaryAttachment(UWeaponAttachment* Attachment)
{
	TSubclassOf<UWeaponAttachment>* AttachmentSlot = GetSecondaryAttachmentSlot(CurrentLoadout, Attachment->WeaponAttachmentType);
	if (AttachmentSlot)
		*AttachmentSlot = Attachment->GetClass();
}

TArray<ULoadoutCustomization*> UCustomizationWidget::GetCustomizationSkins(ABaseItem* Item)
{
	if (!Item)
		return {};

	FName ItemTag = Item->CustomizationTag;
	FCompatibleSkinsArray* SkinData = ItemToSkinsMap.Find(ItemTag);
	
	if (!SkinData)
		return {};

	TArray<ULoadoutCustomization*> CompatibleFamilies;
	for (ULoadoutCustomization* CustomizationItem : SkinData->CompatibleSkins)
	{
		if (CachedCustomizationFamilies.Contains(CustomizationItem))
			CompatibleFamilies.Add(CustomizationItem);
	}
	return CompatibleFamilies;
}

TArray<ULoadoutCustomization*> UCustomizationWidget::GetCustomizationItems(ECustomizationType Type, bool bFamiliesOnly)
{
	const TArray<ULoadoutCustomization*>& SourceItems = bFamiliesOnly ? CachedCustomizationFamilies : CachedCustomizationItems;
	
	// Return all assets if "any" type is requested
	if (Type == ECustomizationType::Any)
		return SourceItems;

	FSpawnedGear SpawnedGear;
	if (PreviewCharacter && PreviewCharacter->GetInventoryComponent())
	{
		SpawnedGear = PreviewCharacter->GetInventoryComponent()->GetSpawnedGear();
	}
	
	// Ensure we return only currently available skins
	switch (Type)
	{
	case ECustomizationType::PrimarySkin: return GetCustomizationSkins(SpawnedGear.Primary);
	case ECustomizationType::SecondarySkin: return GetCustomizationSkins(SpawnedGear.Secondary);
	case ECustomizationType::ArmorSkin: return GetCustomizationSkins(SpawnedGear.Armor);
	case ECustomizationType::HeadwearSkin: return GetCustomizationSkins(SpawnedGear.Helmet);
	default: break;
	}

	// Otherwise just return the matching items
	TArray<ULoadoutCustomization*> Items;
	for (ULoadoutCustomization* LoadoutItem : SourceItems)
	{
		// Ensure the customization type matches
		if (LoadoutItem->Asset->Type != Type)
			continue;

		Items.Add(LoadoutItem);
	}

	return Items;
}

ULoadoutCustomization* UCustomizationWidget::GetEquippedCustomizationItem(ECustomizationType Type)
{
	UCustomizationDataBase* DataAsset = CurrentCustomization.GetCustomizationItem(Type);
	for (ULoadoutCustomization* LoadoutItem : CachedCustomizationItems)
	{
		if (LoadoutItem->Asset == DataAsset)
			return LoadoutItem;
	}
	return nullptr;
}

void UCustomizationWidget::SetCustomizationItem(ULoadoutCustomization* Item)
{
	if (!Item || !Item->Asset)
		return;

	// Play a sound on equip
	UFMODEvent** EquipEvent = CustomizationEquipEvents.Find(Item->GetCustomizationType());
	if (EquipEvent)
	{
		UFMODBlueprintStatics::PlayEvent2D(GetWorld(), *EquipEvent, true);
	}

	// Store the item in our current customization and save the profile
	CurrentCustomization.SetCustomizationItem(Item->Asset);
	if (Profile)
	{
		Profile->Customizations.Add(CurrentSwat, CurrentCustomization);
		Profile->SaveProfile();
	}

	// Additional customization logic if it is a skin
	bool bIsSkin = Item->GetCustomizationType() >= ECustomizationType::PrimarySkin;
	if (bIsSkin)
	{
		ABaseItem* Target = nullptr;
		UCustomizationSkin* Material = Cast<UCustomizationSkin>(Item->Asset);

		FSpawnedGear SpawnedGear;
		if (PreviewCharacter && PreviewCharacter->GetInventoryComponent())
		{
			SpawnedGear = PreviewCharacter->GetInventoryComponent()->GetSpawnedGear();
		}
		
		// Example grabbing the target actor to apply our customization to in previews
		switch (Item->GetCustomizationType())
		{
		case ECustomizationType::PrimarySkin: Target = SpawnedGear.Primary; break;
		case ECustomizationType::SecondarySkin: Target = SpawnedGear.Secondary; break;
		case ECustomizationType::ArmorSkin: Target = SpawnedGear.Armor; break;
		case ECustomizationType::HeadwearSkin: Target = SpawnedGear.Helmet; break;
		default: break;
		}
		
		// Store the skin we have last set for this particular item
		if (Target)
		{
			TSoftClassPtr<ABaseItem> SoftLoadoutItem = TSoftClassPtr<ABaseItem>(Target->GetClass());
			CurrentCustomization.PreviousSkinsMap.Add(SoftLoadoutItem, Material);
		}

		// If set, apply the customization to the preview
		if (Target && Material)
		{
			Target->Skin = Material;
			FSavedCustomization::ApplyItemCustomization(Target, Material);
		}
	}

	ApplyCustomization(CurrentCustomization);
	
	FName AnimationToPlay;
	switch (Item->GetCustomizationType())
	{
	case ECustomizationType::Character: break;
	case ECustomizationType::Voice: break;
	case ECustomizationType::Helmet: AnimationToPlay = "tp_customization_head"; break;
	case ECustomizationType::Shirt: AnimationToPlay = "tp_customization_shirt"; break;
	case ECustomizationType::Pants: AnimationToPlay = "tp_customization_pants"; break;
	case ECustomizationType::Shoes: AnimationToPlay = "tp_customization_shoes"; break;
	case ECustomizationType::Eyewear: AnimationToPlay = "tp_customization_character"; break;
	case ECustomizationType::Belt: break;
	case ECustomizationType::Gloves: /*AnimationToPlay = "tp_customization_gloves";*/ break;
	case ECustomizationType::Watch: /*AnimationToPlay = "tp_customization_watch";*/ break;
	case ECustomizationType::Tattoo: break;
	case ECustomizationType::PrimarySkin: break;
	case ECustomizationType::SecondarySkin: break;
	case ECustomizationType::ArmorSkin: AnimationToPlay = "tp_customization_armor"; break;
	case ECustomizationType::HeadwearSkin: break;
	case ECustomizationType::Any: break;
	default: ;
	}

	if (PreviewCharacter && !AnimationToPlay.IsNone())
	{
		PreviewCharacter->PlayMontageFromTable(AnimationToPlay.ToString());
	}
}

bool UCustomizationWidget::CanCustomizeType(ECustomizationType Type)
{
	if (!GetWorld())
		return true;

	// Allow all if not single player
	if (!GetWorld()->GetNetDriver())
		return true;

	// Cannot customize character or voices in commander mode
	switch (Type)
	{
	case ECustomizationType::Character: return false;
	case ECustomizationType::Voice: return false;
	default: return true;
	}
}

void UCustomizationWidget::PreviewCustomizationItem(ULoadoutCustomization* Item)
{
	if (!Item || !Item->Asset)
		return;

	// Special case where we just play a preview of the voice
	UCustomizationVoice* Voice = Cast<UCustomizationVoice>(Item->Asset);
	if (Voice)
	{
		const TArray<FString> PossibleVoicelines =
		{
			VO_SWAT_GENERAL::CALL_ARRESTING_CIVILIAN,
			VO_SWAT_GENERAL::CALL_ARRESTING_SUSPECT,
			VO_SWAT_GENERAL::CALL_ARRESTING_CIVILIAN_COMPLETE,
			VO_SWAT_GENERAL::CALL_ARRESTING_SUSPECT_COMPLETE,
			VO_SWAT_GENERAL::CALL_SUSPECT_KILLED,
			VO_SWAT_GENERAL::CALL_YELL_AT_CIVILIAN,
			VO_SWAT_GENERAL::CALL_YELL_AT_SUSPECT,
			VO_SWAT_GENERAL::CALL_REPORT_ARRESTED_CIVILIAN,
			VO_SWAT_GENERAL::CALL_REPORT_ARRESTED_SUSPECT,
			VO_SWAT_GENERAL::CALL_REPORT_INCAPACITATED_CIVILIAN,
			VO_SWAT_GENERAL::CALL_REPORT_INCAPACITATED_SUSPECT,
			VO_SWAT_GENERAL::CALL_REPORT_DEAD_CIVILIAN,
			VO_SWAT_GENERAL::CALL_REPORT_DEAD_SUSPECT,
		};
		
		if (PreviewCharacter && PossibleVoicelines.Num() > 0)
		{
			FString RandomVoiceLine = PossibleVoicelines[FMath::RandRange(0, PossibleVoicelines.Num() - 1)];
			PreviewCharacter->PlayRawVO(RandomVoiceLine, Voice->VoiceHandle.ToString(), false);
		}
		return;
	}

	// Make a copy of our customization for previewing
	FSavedCustomization PreviewCustomization = CurrentCustomization;
	PreviewCustomization.SetCustomizationItem(Item->Asset);

	ApplyCustomization(PreviewCustomization);
}

void UCustomizationWidget::ClearCustomizationPreview()
{
	CurrentCustomization.ApplyCustomization(PreviewCharacter);
}

void UCustomizationWidget::ExitCustomization()
{
	AReadyOrNotPlayerController* PlayerController = GetOwningPlayer<AReadyOrNotPlayerController>();
	if (PlayerController)
	{
		PlayerController->EscapeMenu();
	}
}

void UCustomizationWidget::SetActiveSwatMember(EEquippingSwat InSwat)
{
	CurrentSwat = InSwat;

	// Try to find existing customization
	CurrentCustomization = FSavedCustomization();
	if (Profile)
	{
		FSavedCustomization* SavedCustomization = Profile->Customizations.Find(CurrentSwat);
		if (SavedCustomization)
		{
			CurrentCustomization = *SavedCustomization;
		}
	}

	if (!GetWorld())
		return;
	
	// Practice mode customizations
	UItemData* ItemData = UBpGameplayHelperLib::GetItemData();
	if (ItemData)
	{
		FDefaultCharacterCustomization* CharacterCustomization = ItemData->DefaultCharacters.Find(CurrentSwat);
		if (CharacterCustomization)
		{
			CurrentCustomization.Character = CharacterCustomization->Character;
			CurrentCustomization.Voice = CharacterCustomization->Voice;
		}
	}

	// Commander mode customizations
	ALobbyGM* LobbyGM = GetWorld()->GetAuthGameMode<ALobbyGM>();
	if (LobbyGM && LobbyGM->RosterManager)
	{
		ERosterSquadPosition RosterSquadPosition = ERosterSquadPosition::Unassigned;
		switch (CurrentSwat)
		{
		case EEquippingSwat::ES_BlueOne: RosterSquadPosition = ERosterSquadPosition::BlueOne; break;
		case EEquippingSwat::ES_BlueTwo: RosterSquadPosition = ERosterSquadPosition::BlueTwo; break;
		case EEquippingSwat::ES_RedOne: RosterSquadPosition = ERosterSquadPosition::RedOne; break;
		case EEquippingSwat::ES_RedTwo: RosterSquadPosition = ERosterSquadPosition::RedTwo; break;
		default: ;
		}
		
		URosterCharacter* RosterCharacter = LobbyGM->RosterManager->GetSquadCharacter(RosterSquadPosition, false);
		if (RosterCharacter)
		{
			CurrentCustomization.Character = RosterCharacter->Character;
			CurrentCustomization.Voice = RosterCharacter->Voice;
		}
	}
	
	CurrentCustomization.Sanitize();
	ApplyCustomization(CurrentCustomization);
}

template<typename T>
T* CreateLoadoutItem(ABaseItem* ItemCDO)
{
	if (!ItemCDO)
		return nullptr;
	
	T* LoadoutItem = NewObject<T>();
	LoadoutItem->Class = ItemCDO->GetClass();
	LoadoutItem->Name = ItemCDO->ItemName;
	LoadoutItem->Description = ItemCDO->ItemDescription;
	LoadoutItem->Icon = ItemCDO->ItemIcon;

	return LoadoutItem;
}

void UCustomizationWidget::CacheLoadoutItems()
{
	if (!LoadoutManager)
		return;

	for (ABaseItem* ItemCDO : LoadoutManager->PrimaryWeapons)
	{
		CachedPrimaryWeapons.Add(CreateLoadoutItem<ULoadoutWeapon>(ItemCDO));
	}
	for (ABaseItem* ItemCDO : LoadoutManager->SecondaryWeapons)
	{
		CachedSecondaryWeapons.Add(CreateLoadoutItem<ULoadoutWeapon>(ItemCDO));
	}
	for (ABaseItem* ItemCDO : LoadoutManager->LongTacticalItems)
	{
		CachedLongTacticalItems.Add(CreateLoadoutItem<ULoadoutEquipment>(ItemCDO));
	}
	for (ABaseItem* ItemCDO : LoadoutManager->TacticalItems)
	{
		CachedTacticalItems.Add(CreateLoadoutItem<ULoadoutEquipment>(ItemCDO));
	}
	for (ABaseItem* ItemCDO : LoadoutManager->BodyArmors)
	{
		CachedArmorItems.Add(CreateLoadoutItem<ULoadoutEquipment>(ItemCDO));
	}
	for (ABaseItem* ItemCDO : LoadoutManager->Helmets)
	{
		CachedHelmetItems.Add(CreateLoadoutItem<ULoadoutEquipment>(ItemCDO));
	}
}

void UCustomizationWidget::CacheCustomizationItems()
{
	if (!GetWorld())
		return;

	UReadyOrNotGameInstance* GameInstance = GetGameInstance<UReadyOrNotGameInstance>();
	if (!GameInstance || !GameInstance->LoadoutManager)
		return;
	
	UMetaGameProfile* MetaGameProfile = UMetaGameProfile::GetProfile(GetWorld());

	TMap<UCustomizationDataBase*, ULoadoutCustomization*> ItemAssetMap;
	TSet<ULoadoutCustomization*> ItemsInFamilies;
	
	CachedCustomizationItems.Empty();
	for (UCustomizationDataBase* DataAsset : LoadoutManager->CustomizationAssets)
	{
		// Ignore assets set to not appear in loadout
		if (!DataAsset->bShowInLoadout)
			continue;
		
		// Ignore assets not unlocked by the player
		bool bAnyTagMissing = false;
		for (FName RequiredTag : DataAsset->RequiredTags)
		{
			if (!MetaGameProfile || !MetaGameProfile->GetProgressionTags().Contains(RequiredTag))
			{
				bAnyTagMissing = true;
				break;
			}
		}

		ULoadoutCustomization* LoadoutItem = NewObject<ULoadoutCustomization>();
		LoadoutItem->Asset = DataAsset;
		LoadoutItem->Name = DataAsset->Name;
		LoadoutItem->Description = DataAsset->Description;
		LoadoutItem->Icon = DataAsset->Icon;
		LoadoutItem->Variant = DataAsset->Variant;
		LoadoutItem->VariantIcon = DataAsset->VariantIcon;
		LoadoutItem->bLocked = bAnyTagMissing;
		LoadoutItem->RequirementsText = DataAsset->RequirementsText;
		
		CachedCustomizationItems.Add(LoadoutItem);
		ItemAssetMap.Add(DataAsset, LoadoutItem);

		if (DataAsset->Parent)
			ItemsInFamilies.Add(LoadoutItem);
		
		UCustomizationSkin* MaterialAsset = Cast<UCustomizationSkin>(DataAsset);
		if (MaterialAsset)
		{
			for (FName ItemTag : MaterialAsset->CompatibleItemTags)
			{
				FCompatibleSkinsArray& SkinData = ItemToSkinsMap.FindOrAdd(ItemTag);
				SkinData.CompatibleSkins.Add(LoadoutItem);
			}
		}
	}
	
	for (ULoadoutCustomization* LoadoutItem : CachedCustomizationItems)
	{
		if (!LoadoutItem->Asset)
			continue;

		// If items have a parent, add them as a child to the parent
		// Otherwise we add them as an item family
		if (ItemsInFamilies.Contains(LoadoutItem))
		{
			UCustomizationDataBase* Parent = LoadoutItem->Asset->Parent;
			ULoadoutCustomization** ParentItem = ItemAssetMap.Find(Parent);
			
			if (Parent && ParentItem)
			{
				(*ParentItem)->Children.Add(LoadoutItem);
				LoadoutItem->Name = (*ParentItem)->Name;
			}
		}
		else
		{
			CachedCustomizationFamilies.Add(LoadoutItem);
		}
	}
}

void UCustomizationWidget::ApplyCustomization(const FSavedCustomization& InCustomization)
{
	if (!PreviewCharacter)
		return;
	
	FSavedCustomization CustomizationCopy = InCustomization;
	
	// Force Judge in SP
	UItemData* ItemData = UBpGameplayHelperLib::GetItemData();
	if (!GetWorld()->GetNetDriver() && ItemData && CurrentSwat == EEquippingSwat::ES_None)
	{
		CustomizationCopy.Character = ItemData->DefaultCustomization.Character;
		CustomizationCopy.Voice = ItemData->DefaultCustomization.Voice;
	}

	// Apply all customizations
	CustomizationCopy.SanitizeServer(PreviewCharacter);
	CustomizationCopy.ApplyCustomization(PreviewCharacter);
	CustomizationCopy.ApplyCustomizationSkins(PreviewCharacter);

	// Some gear skins may have changed sockets, reset them here
	if (PreviewCharacter->GetInventoryComponent())
	{
		PreviewCharacter->GetInventoryComponent()->ReattachAllGear();
	}
}

void UCustomizationWidget::HandlePreviewCharacterItemsChanged()
{
	if (!PreviewCharacter)
		return;
	
	ApplyCustomization(CurrentCustomization);
}

