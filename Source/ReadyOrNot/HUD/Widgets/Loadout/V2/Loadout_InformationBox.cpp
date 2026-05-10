// Copyright Void Interactive, 2023

#include "HUD/Widgets/Loadout/V2/Loadout_InformationBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "lib/ReadyOrNotLoadoutManager.h"

void ULoadout_InformationBox::NativeConstruct()
{
	Super::NativeConstruct();

	gs = GetWorld()->GetGameState<AReadyOrNotGameState>();
	LoadoutFunctionLibrary = gs->LoadoutFunctionLibrary;
}

void ULoadout_InformationBox::UpdateInfoBox(ABaseItem* Item, FText CurrentItemCategory, bool IsItemWeapon)
{
	this->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	txt_Class->SetVisibility(ESlateVisibility::Collapsed);
	VB_Attachments->SetVisibility(ESlateVisibility::Collapsed);
	VB_Effects->SetVisibility(ESlateVisibility::Collapsed);

	SetCategory(CurrentItemCategory);
	txt_ItemName->SetText(Item->ItemName);
	txt_Description->SetText(Item->ItemDescription);
	
	if (IsItemWeapon)
	{
		txt_Class->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		txt_Class->SetText(FText::FromString(ItemClassEnumToString(Item->ItemClass)));
	}
}

void ULoadout_InformationBox::UpdateMaterialInfo(UArmourMaterial* ArmorMaterial, FText CurrentItemCategory)
{
	this->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	txt_Class->SetVisibility(ESlateVisibility::Collapsed);
	VB_Attachments->SetVisibility(ESlateVisibility::Collapsed);
	VB_Effects->SetVisibility(ESlateVisibility::Collapsed);
	
	SetCategory(CurrentItemCategory);
	txt_ItemName->SetText(ArmorMaterial->DisplayName);
	txt_Description->SetText(ArmorMaterial->Description);
}

void ULoadout_InformationBox::UpdateEffectsInfo(UWeaponAttachment* Attachment, FText CurrentItemCategory)
{
	this->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	VB_Effects->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	txt_Class->SetVisibility(ESlateVisibility::Collapsed);
	VB_Attachments->SetVisibility(ESlateVisibility::Collapsed);
	
	SetCategory(CurrentItemCategory);
	txt_ItemName->SetText(Attachment ? Attachment->ItemName : FText::FromString(TEXT("None")));
	TArray<FAttachmentEffects> Effects = SetEffects(Attachment);
	CreateEffectsElement(Effects);
}

void ULoadout_InformationBox::SetCategory(FText CurrentItemCategory)
{
	txt_Category->SetText(CurrentItemCategory);
}

void ULoadout_InformationBox::SetAttachments(TSubclassOf<ABaseWeapon> BaseWeapon, bool IsSecondary)
{
	//find and go through all attachment types for current weapon, add attachment for current type to list and create widget element based on list
	VB_Attachments->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	TArray<UWeaponAttachment*>	  CurrentAttachments;
	TArray<EWeaponAttachmentType> AttachmentTypes = LoadoutFunctionLibrary->GetAvailableAttachmentTypesByWeapon(BaseWeapon);
	FStoredWeaponAttachments	  WeaponAttachments = LoadoutFunctionLibrary->StoredAttachmentsByWeapon.FindRef(BaseWeapon);
	for (EWeaponAttachmentType CurrentType : AttachmentTypes)
	{
		switch (CurrentType)
		{
			case EWeaponAttachmentType::Optics:
				CurrentAttachments.Add(WeaponAttachments.ScopeAttachment.GetDefaultObject());
				break;
			case EWeaponAttachmentType::Muzzle:
				CurrentAttachments.Add(WeaponAttachments.MuzzleAttachment.GetDefaultObject());
				break;
			case EWeaponAttachmentType::Underbarrel:
				CurrentAttachments.Add(WeaponAttachments.UnderbarrelAttachment.GetDefaultObject());
				break;
			case EWeaponAttachmentType::Overbarrel:
				CurrentAttachments.Add(WeaponAttachments.OverbarrelAttachment.GetDefaultObject());
				break;
			case EWeaponAttachmentType::Stock:
				CurrentAttachments.Add(WeaponAttachments.StockAttachment.GetDefaultObject());
				break;
			case EWeaponAttachmentType::Grip:
				CurrentAttachments.Add(WeaponAttachments.GripAttachment.GetDefaultObject());
				break;
			case EWeaponAttachmentType::Illuminators:
				CurrentAttachments.Add(WeaponAttachments.IlluminatorAttachment.GetDefaultObject());
				break;
			case EWeaponAttachmentType::Ammunition:
				CurrentAttachments.Add(WeaponAttachments.AmmunitionAttachment.GetDefaultObject());
				break;
			default:
				break;
		}
	}
	CreateAttachmentElement(CurrentAttachments, AttachmentTypes);
}

TArray<FAttachmentEffects> ULoadout_InformationBox::SetEffects(UWeaponAttachment* Attachment)
{
	TArray<FAttachmentEffects> AttachmentEffects;

	if (Attachment == nullptr)
		return AttachmentEffects;
	if (!ensure(Attachment))
		return {};
	
	//all multiplier effects and whether or not we have to reverse the < for it to work correctly
	FEffectsMultiplierStruct   MultiplierArr[] = {
		  FEffectsMultiplierStruct(false, Attachment->VerticalRecoilMultiplier),
		  FEffectsMultiplierStruct(false, Attachment->HorizontalRecoilMultiplier),
		  FEffectsMultiplierStruct(true, Attachment->MuzzleVelocityMultiplier),
		  FEffectsMultiplierStruct(true, Attachment->ADS_Speed_Multiplier),
		  FEffectsMultiplierStruct(false, Attachment->SpreadMultiplier)
	};

	// Keys for stringtable lookup
	FString PosEffectsArr[] = {
		"ReducedVerticalRecoil",
		"ReducedHorizontalRecoil",
		"IncreasedMuzzleVelocity",
		"IncreasedADSSpeed",
		"IncreasedAccuracy"
	};

	FString NegEffectsArr[] = {
		"IncreasedVerticalRecoil",
		"IncreasedHorizontalRecoil",
		"ReducedMuzzleVelocity",
		"ReducedADSSpeed",
		"ReducedAccuracy"
	};

	//go through all multipliers, check that they aren't set to default value 1.0 and then add either the pos or neg effect to the list based on whether the multiplier is pos or neg 
	for (int i = 0; i < sizeof(MultiplierArr) / sizeof(FEffectsMultiplierStruct); i++)
	{
		if (MultiplierArr[i].Multiplier != 1.0f)
		{
			switch (MultiplierArr[i].bShouldReverse ? MultiplierArr[i].Multiplier > 1.0f : MultiplierArr[i].Multiplier < 1.0f)
			{
				case true:
					AttachmentEffects.Add(FAttachmentEffects(FText::FromStringTable("LoadoutInfoTable", PosEffectsArr[i]), true));
					break;
				case false:
					AttachmentEffects.Add(FAttachmentEffects(FText::FromStringTable("LoadoutInfoTable", NegEffectsArr[i]), false));
					break;
			}
		}
	}

	//add the last effects, then return the list
	if (Attachment->bShouldSupressWeapon)
		AttachmentEffects.Add(FAttachmentEffects(FText::FromStringTable("LoadoutInfoTable", "Suppressed"), true));
	if (Attachment->bHidesMuzzleFlash)
		AttachmentEffects.Add(FAttachmentEffects(FText::FromStringTable("LoadoutInfoTable", "HidesMuzzleFlash"), true));
	if (Attachment->bOverrideMuzzleFlash)
		AttachmentEffects.Add(FAttachmentEffects(FText::FromStringTable("LoadoutInfoTable", "ReducesMuzzleFlash"), true));

	return AttachmentEffects;
}
