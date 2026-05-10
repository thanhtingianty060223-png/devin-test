// Copyright Void Interactive, 2023

#pragma once

#include "CustomizationData.h"
#include "Engine/DataAsset.h"
#include "Data/PlayableCharacterData.h"
#include "Enums.h"
#include "ItemData.generated.h"

USTRUCT(BlueprintType)
struct FWeaponData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Description)
	TArray<EWeaponType> IncludedInWeaponCategories;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Description)
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Description)
	TSoftObjectPtr<UTexture2D> Image = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Description)
	FText WeaponType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Description)
	EItemClass ItemClass = EItemClass::IC_NoClass;

	// the weapon class (go find the blueprint)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Description)
	TSubclassOf<class ABaseWeapon> Blueprint;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Points)
	float PointsAvailable = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Skins)
	TSoftObjectPtr<UTexture2D> FactorySkinImage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Skins)
	TArray<TSubclassOf<class USkinComponent>> AvailableSkins;

	UPROPERTY(BlueprintReadOnly, Category = Skins)
	class TSubclassOf<USkinComponent> CurrentSkin;
};

USTRUCT(BlueprintType)
struct FGrenadeData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Camo)
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Camo)
	TSoftObjectPtr<UTexture2D> Image = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Camo)
	EItemClass ItemClass = EItemClass::IC_NoClass;

	// the weapon class (go find the blueprint)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Camo)
	TSubclassOf<class ABaseGrenade> Blueprint;
};

USTRUCT(BlueprintType)
struct FDeviceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Camo)
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Camo)
	TSoftObjectPtr<UTexture2D> Image = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Carousel)
	TSoftObjectPtr<UTexture2D> CarouselImage1 = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Carousel)
	TSoftObjectPtr<UTexture2D> CarouselImage2 = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Carousel)
	TSoftObjectPtr<UTexture2D> CarouselImage3 = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Details)
	FText EffectiveRange;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Details)
	FText Use;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Details)
	FText Effects;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Details)
	FText Risk;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Details)
	FText ToMitigate;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Details)
	EItemClass ItemClass = EItemClass::IC_NoClass;
	
	// No longer used
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Details, meta = (ClampMin = 0, UIMin = 0, ClampMax = 8, UIMax = 8))
	int32 MaxInInventory = 4;

	// How many of this item to grant per slot allocated in the loadout
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0, UIMin = 0))
	int32 ItemsPerSlot = 1;

	// the weapon class (go find the blueprint)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Camo)
	TSubclassOf<class ABaseItem> Blueprint;
};

USTRUCT(BlueprintType)
struct FArmourData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Armour)
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Armour)
	TArray<FText> ProtectsAgainstText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Armour)
	TSoftObjectPtr<UTexture2D> Image = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Armour)
	EItemClass ItemClass = EItemClass::IC_NoClass;
	
	// the weapon class (go find the blueprint)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Armour)
	TSubclassOf<class ABaseItem> Blueprint;
};

USTRUCT(BlueprintType)
struct FUniformData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Camo)
	FText Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Camo)
	TSoftObjectPtr<UTexture2D> Image = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Camo)
	EItemClass ItemClass = EItemClass::IC_NoClass;

	// the weapon class (go find the blueprint) // TODO: Make unfirom seleciton??
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Camo)
	TSubclassOf<class ABaseItem> Blueprint;
};

USTRUCT(BlueprintType)
struct FCharacterData
{
	GENERATED_BODY()

	// the handle name of this character data (so the cycler lines uloadingp properly)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Character)
	FName Handle;

	// The character name (used for the icons)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Character)
	FText Name;

	// The character title (Used for the icons)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Character)
	FText Title;

	// The icon
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Character)
	TSoftObjectPtr<UTexture2D> CharacterIcon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Details)
	EItemClass ItemClass = EItemClass::IC_NoClass;
	
	// the character class (go find the blueprint)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Character)
	TSoftObjectPtr<class UPlayableCharacterData> Blueprint;
};

USTRUCT()
struct FDefaultCharacterCustomization
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	UCustomizationCharacter* Character;

	UPROPERTY(EditAnywhere)
	UCustomizationVoice* Voice;

	UPROPERTY(EditAnywhere)
	UCustomizationSkin* ArmorSkin;
};

/**
 * 
 */
UCLASS(BlueprintType)
class READYORNOT_API UItemData : public UDataAsset
{
	GENERATED_BODY()

public:
	UItemData();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Equipment, meta = (TitleProperty = "Name"))
	TArray<FWeaponData> BluePVPUniquePrimaryWeapons;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Equipment, meta = (TitleProperty = "Name"))
	TArray<FWeaponData> RedPVPUniquePrimaryWeapons;

private: // NOTE(killo): do not remove, we are not using itemdata for authoritative lists of items anymore
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deprecated", meta = (TitleProperty = "Name", AllowPrivateAccess = true))
	TArray<FWeaponData> PrimaryWeapons;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deprecated", meta = (TitleProperty = "Name", AllowPrivateAccess = true))
	TArray<FWeaponData> SecondaryWeapons;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deprecated", meta = (TitleProperty = "Name", AllowPrivateAccess = true))
	TArray<FDeviceData> LongTacticalItems;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deprecated", meta = (TitleProperty = "Name", AllowPrivateAccess = true))
	TArray<FDeviceData> TacticalItems;
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Defaults)
	TSubclassOf<class ABaseItem> NullItem = nullptr;

	// used to allow the player to select no attachment in the slot if desiered.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Defaults)
	TSubclassOf<class UWeaponAttachment> NullPrimaryScopeAttachment;

	// used to allow the player to select no attachment in the slot if desiered.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Defaults)
	TSubclassOf<class UWeaponAttachment> NullMuzzleAttachment;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Defaults)
	TSubclassOf<class UWeaponAttachment> NullOverbarrelAttachment;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Defaults)
	TSubclassOf<class UWeaponAttachment> NullUnderbarrelAttachment;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Defaults)
	TSubclassOf<class UWeaponAttachment> NullStockAttachment;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Defaults)
	TSubclassOf<class UWeaponAttachment> NullGripAttachment;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Defaults)
	TSubclassOf<class UWeaponAttachment> NullIlluminatorAttachment;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Defaults)
	TSubclassOf<class UWeaponAttachment> NullAmmunitionAttachment;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Default Given Items")
	TArray<TSubclassOf<ABaseItem>> DefaultItemsGivenToPlayer;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Description)
	TSubclassOf<class USkinComponent> FactorySkin;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Default Loadouts", meta = (TitleProperty = "Name"))
	TArray<FSavedLoadout> DefaultLoadouts;

	UPROPERTY(EditAnywhere, Category="Default Customization")
	FSavedCustomization DefaultCustomization;

	UPROPERTY(EditAnywhere, Category="Default Customization")
	TMap<EEquippingSwat, FDefaultCharacterCustomization> DefaultCharacters;

	UPROPERTY(EditAnywhere, Category="Default Customization")
	FSavedCustomization TrailerCustomization;

	UPROPERTY(EditAnywhere, Category="Default Customization")
	TArray<UCustomizationCharacter*> TrailerCharacters;
	
private: // NOTE(killo): do not remove, we are not using itemdata for lists of items anymore
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deprecated", meta = (TitleProperty = "Name", AllowPrivateAccess = true))
	TArray<FArmourData> BodySelection;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deprecated", meta = (TitleProperty = "Name", AllowPrivateAccess = true))
	TArray<FArmourData> HeadSelection;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deprecated", meta = (AllowPrivateAccess = true))
	TArray<TSoftObjectPtr<UArmourMaterial>> ArmourMaterials;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Uniform)
	TArray<TSubclassOf<class USkinComponent>> UniformSelection;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Character)
	TArray<FCharacterData> CharacterSelection;

	// use this image if no image in the returning data..
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Defaults)
	TSoftObjectPtr<UTexture2D> DefaultItemImage = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Personnel)
	TArray<FText> PersonnelNames;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Personnel)
	TArray<FText> PersonnelDescriptions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Defaults)
	int AttachmentPointsBase = 0;
};
