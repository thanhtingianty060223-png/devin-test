// Copyright Void Interactive, 2023

#pragma once

#include "HUD/Widgets/PreMissionPlanning.h"
#include "Info/LoadoutManager.h"
#include "ReadyOrNotLoadoutManager.generated.h"

UENUM(BlueprintType)
enum class ELoadoutMunitionSlotType : uint8
{
	TacticalSlot,
	PrimaryAmmunition,
	SecondaryAmmunition,
	GrenadeSlot
};

UCLASS()
class READYORNOT_API UReadyOrNotLoadoutManager : public UObject
{	
	GENERATED_BODY()

public:
	void Initialize(UWorld* World);
	
	// Default preset names
	const FString DEFAULT = "default";
	const FString DEFAULT_BLUE_ONE = "defaultblueone";
	const FString DEFAULT_BLUE_TWO = "defaultbluetwo";
	const FString DEFAULT_RED_ONE = "defaultredone";
	const FString DEFAULT_RED_TWO = "defaultredtwo";
	
	// Loadout
	UFUNCTION(BlueprintCallable, BlueprintPure)
	FSavedLoadout GetActiveLoadout();
	UFUNCTION(BlueprintCallable)
	void SetActiveLoadout(FSavedLoadout Loadout);
	UFUNCTION(BlueprintCallable)
	void SetActiveLoadoutByName(FString LoadoutName);
	UFUNCTION(BlueprintCallable)
	bool ItemIsInActiveLoadout(TSubclassOf<ABaseItem> Item);

	// Players
	UFUNCTION(BlueprintCallable)
	void SetActiveSwatMember(EEquippingSwat SwatMember);
	UFUNCTION(BlueprintCallable, BlueprintPure)
	EEquippingSwat GetActiveSwatMember();
	UFUNCTION(BlueprintCallable, BlueprintPure)
	FString GetActiveSwatMemberLabel();
	UFUNCTION(BlueprintCallable)
	EEquippingSwat FStringToEquippingSwat(FString Name);
	UFUNCTION(BlueprintCallable)
	EEquippingSwat NextActiveSwatMember();
	UFUNCTION(BlueprintCallable)
	EEquippingSwat PreviousActiveSwatMember();

	// Save & Load
	UFUNCTION(BlueprintCallable)
	void DoSaveActiveLoadout();
	UFUNCTION(BlueprintCallable)
	void DoSaveLoadout(EEquippingSwat SwatMember, FSavedLoadout Loadout);
	UFUNCTION(BlueprintCallable)
	void SanitizeActiveLoadout();
	FSavedLoadout GetLoadoutByName(FString LoadoutName);
	bool DeleteLoadout(FString LoadoutName);

	TArray<FName> GetUsableAmmoTypes(const ABaseWeapon* Weapon);

	// Primary
	UFUNCTION(BlueprintCallable)
	void SetActivePrimary(TSubclassOf<ABaseWeapon> Primary);
	UFUNCTION(BlueprintCallable, BlueprintPure)
	TSubclassOf<ABaseWeapon> GetActivePrimary();
	UFUNCTION(BlueprintCallable)
	void SetActivePrimarySkin(TSubclassOf<USkinComponent> Skin);
	UFUNCTION(BlueprintCallable, BlueprintPure)
	TSubclassOf<UWeaponAttachment> GetActivePrimaryAttachmentByType(EWeaponAttachmentType AttachmentType);
	UFUNCTION(BlueprintCallable, BlueprintPure)
	TArray<FName> GetPrimaryAmmoTypes();

	// Secondary
	UFUNCTION(BlueprintCallable)
	void SetActiveSecondary(TSubclassOf<ABaseWeapon> Secondary);
	UFUNCTION(BlueprintCallable, BlueprintPure)
	TSubclassOf<ABaseWeapon> GetActiveSecondary();
	UFUNCTION(BlueprintCallable)
	void SetActiveSecondarySkin(TSubclassOf<USkinComponent> Skin);
	UFUNCTION(BlueprintCallable, BlueprintPure)
	TSubclassOf<UWeaponAttachment> GetActiveSecondaryAttachmentByType(EWeaponAttachmentType AttachmentType);
	UFUNCTION(BlueprintCallable, BlueprintPure)
	TArray<FName> GetSecondaryAmmoTypes();

	// Ammunition
	UFUNCTION(BlueprintCallable, BlueprintPure)
	FText GetAmmunitionDisplayName(FName AmmunitionName);
	UFUNCTION(BlueprintCallable, BlueprintPure)
	FText GetAmmunitionCaliber(FName AmmunitionName);
	UFUNCTION(BlueprintCallable, BlueprintPure)
	FText GetAmmunitionDescription(FName AmmunitionName);

	// Long Tactical
	UFUNCTION(BlueprintCallable)
	void SetActiveLongTactical(TSubclassOf<ABaseItem> LongTactical);
	UFUNCTION(BlueprintCallable, BlueprintPure)
	TSubclassOf<ABaseItem> GetActiveLongTactical();

	// Attachments
	UFUNCTION(BlueprintCallable, BlueprintPure)
	TArray<EWeaponAttachmentType> GetAvailableAttachmentTypesByWeapon(TSubclassOf<ABaseWeapon> BaseWeapon);
	void SetActiveLoadoutFromPreset(FString LoadoutName);
	UFUNCTION(BlueprintCallable)
	void SetPrimaryAttachment(TSubclassOf<UWeaponAttachment> WeaponAttachment, EWeaponAttachmentType AttachmentType);
	UFUNCTION(BlueprintCallable)
	void SetSecondaryAttachment(TSubclassOf<UWeaponAttachment> WeaponAttachment, EWeaponAttachmentType AttachmentType);
	UFUNCTION(BlueprintCallable)
	bool AttachmentIsEquipped(TSubclassOf<UWeaponAttachment> WeaponAttachment, EWeaponAttachmentType AttachmentType);
	UFUNCTION(BlueprintCallable)
	TArray<TSubclassOf<UWeaponAttachment>>
	GetAttachmentByWeaponAndType(ABaseWeapon* Weapon, EWeaponAttachmentType Type);
	UPROPERTY(BlueprintReadOnly)
	TMap<TSubclassOf<ABaseWeapon>, FStoredWeaponAttachments> StoredAttachmentsByWeapon;
	UFUNCTION(BlueprintCallable)
	UWeaponAttachment* GetStoredAttachmentByWeaponAndType(TSubclassOf<ABaseWeapon> Weapon, EWeaponAttachmentType Type);

	// Armor		
	UFUNCTION(BlueprintCallable, BlueprintPure)
	TSubclassOf<ABaseItem> GetActiveBodyArmor();
	UFUNCTION(BlueprintCallable)
	void SetActiveBodyArmor(TSubclassOf<ABaseItem> Armor);
	UFUNCTION(BlueprintCallable, BlueprintPure)
	EArmourCoverage GetArmorCoverage();
	UFUNCTION(BlueprintCallable)
	void SetArmorCoverage(EArmourCoverage ArmorCoverage);
	UFUNCTION(BlueprintCallable, BlueprintPure)
	FText GetArmorCoverageText(EArmourCoverage Coverage);
	UFUNCTION(BlueprintCallable, BlueprintPure)
	UArmourMaterial* GetActiveArmorMaterial();
	UFUNCTION(BlueprintCallable, BlueprintPure)
	TArray<UArmourMaterial*> GetArmorMaterials();
	UFUNCTION(BlueprintCallable)
	void SetArmorMaterial(UArmourMaterial* ArmorMaterial);

	// Headwear
	UFUNCTION(BlueprintCallable, BlueprintPure)
	TSubclassOf<ABaseItem> GetActiveHeadwear();
	UFUNCTION(BlueprintCallable)
	void SetActiveHeadwear(TSubclassOf<ABaseItem> Headwear);

	// Slots
	UFUNCTION(BlueprintCallable, BlueprintPure)
	int GetSlotCount(TSubclassOf<ABaseItem> SlotItem);
	UFUNCTION(BlueprintCallable, BlueprintPure)
	int GetPreviewSlotCount(FSavedLoadout PreviewLoadout, TSubclassOf<ABaseItem> SlotItem);
	UFUNCTION(BlueprintCallable, BlueprintPure)
	int GetCurrentSlotCount();
	UFUNCTION(BlueprintCallable, BlueprintPure)
	int GetMaximumSlotCount();

	void SanitizeSlotCounts();
	
	// Primary Ammo Slots
	UFUNCTION(BlueprintCallable, BlueprintPure)
	int GetPrimarySlotCount(FName AmmoType);
	UFUNCTION(BlueprintCallable, BlueprintPure)
	int GetPreviewPrimarySlotCount(FSavedLoadout PreviewLoadout, FName AmmoType);
	UFUNCTION(BlueprintCallable)
	void IncrementPrimarySlotCount(FName AmmoType);
	UFUNCTION(BlueprintCallable)
	void DecrementPrimarySlotCount(FName AmmoType);

	// Secondary Ammo Slots
	UFUNCTION(BlueprintCallable, BlueprintPure)
	int GetSecondarySlotCount(FName AmmoType);
	UFUNCTION(BlueprintCallable, BlueprintPure)
	int GetPreviewSecondarySlotCount(FSavedLoadout PreviewLoadout, FName AmmoType);
	UFUNCTION(BlueprintCallable)
	void IncrementSecondarySlotCount(FName AmmoType);
	UFUNCTION(BlueprintCallable)
	void DecrementSecondarySlotCount(FName AmmoType);

	// Grenade Slots
	UFUNCTION(BlueprintCallable, BlueprintPure)
	int GetGrenadeSlotCount(TSubclassOf<ABaseItem> SlotItem);
	UFUNCTION(BlueprintCallable, BlueprintPure)
	int GetPreviewGrenadeSlotCount(FSavedLoadout PreviewLoadout, TSubclassOf<ABaseItem> SlotItem);
	UFUNCTION(BlueprintCallable)
	void IncrementGrenadeSlotCount(TSubclassOf<ABaseItem> SlotItem);
	UFUNCTION(BlueprintCallable)
	void DecrementGrenadeSlotCount(TSubclassOf<ABaseItem> SlotItem);

	// Tactical Slot
	UFUNCTION(BlueprintCallable, BlueprintPure)
	int GetTacticalSlotCount(TSubclassOf<ABaseItem> SlotItem);
	UFUNCTION(BlueprintCallable, BlueprintPure)
	int GetPreviewTacticalSlotCount(FSavedLoadout PreviewLoadout, TSubclassOf<ABaseItem> SlotItem);
	UFUNCTION(BlueprintCallable)
	void IncrementTacticalSlotCount(TSubclassOf<ABaseItem> SlotItem);
	UFUNCTION(BlueprintCallable)
	void DecrementTacticalSlotCount(TSubclassOf<ABaseItem> SlotItem);

	// UFUNCTION(BlueprintCallable)
	// TArray<ABaseItem*> GetLongTacticalItems();

	// New Item Access (AssetManager)
	UFUNCTION(BlueprintCallable)
	TArray<ABaseItem*> GetItemsByLoadoutCategory(ELoadoutCategory LoadoutCategory);
	UFUNCTION(BlueprintCallable)
	TArray<ABaseItem*> GetBodyArmors();
	UFUNCTION(BlueprintCallable)
	TArray<ABaseItem*> GetHeadwears();

	// Item Images
	UFUNCTION(BlueprintCallable)
	TSoftObjectPtr<UTexture2D> GetPrimaryWeaponImage(ABaseItem* Item);
	UFUNCTION(BlueprintCallable)
	TSoftObjectPtr<UTexture2D> GetSecondaryWeaponImage(ABaseItem* Item);
	UFUNCTION(BlueprintCallable)
	TSoftObjectPtr<UTexture2D> GetLongTacticalItemImage(ABaseItem* Item);
	UFUNCTION(BlueprintCallable)
	TSoftObjectPtr<UTexture2D> GetBodyArmorItemImage(ABaseItem* Item);
	UFUNCTION(BlueprintCallable)
	TSoftObjectPtr<UTexture2D> GetHeadwearItemImage(ABaseItem* Item);

	// Presets
	UFUNCTION(BlueprintCallable)
	FText GetCurrentPresetDisplayName();

	UFUNCTION(BlueprintCallable, BlueprintPure)
	TArray<FString> GetAllPresetNames() const;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FSavedLoadout GetPreset(const FString& Name);

	UFUNCTION(BlueprintCallable)
	void SavePreset(const FString& Name);
	
	UFUNCTION(BlueprintCallable)
	void ApplyPreset(const FString& Name);

	UFUNCTION(BlueprintCallable)
	void DeletePreset(const FString& Name);

	UFUNCTION(BlueprintCallable)
	void ResetLoadoutToDefault();
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPresetApplied, const FString&, Name, const FSavedLoadout&, Loadout);
	
	UPROPERTY(BlueprintAssignable)
	FOnPresetApplied OnPresetApplied;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAvailablePresetsChanged);

	UPROPERTY(BlueprintAssignable)
	FOnAvailablePresetsChanged OnAvailablePresetsChanged;
	
private:
	TMap<EEquippingSwat, FString> SwatMemberPreset = {
		{EEquippingSwat::ES_None, DEFAULT},
		{EEquippingSwat::ES_BlueOne, DEFAULT_BLUE_ONE},
		{EEquippingSwat::ES_BlueTwo, DEFAULT_BLUE_TWO},
		{EEquippingSwat::ES_RedOne, DEFAULT_RED_ONE},
		{EEquippingSwat::ES_RedTwo, DEFAULT_RED_TWO}
	};
	
	UPROPERTY()
	FSavedLoadout ActiveLoadout;
	UPROPERTY()
	EEquippingSwat ActiveSwatMember = EEquippingSwat::ES_None;
	UPROPERTY()
	class ULoadoutManager* LoadoutManager;
};
