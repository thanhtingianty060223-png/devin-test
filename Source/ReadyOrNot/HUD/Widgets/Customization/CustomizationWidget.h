// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "Data/CustomizationData.h"
#include "HUD/Widgets/PreMissionPlanning.h"
#include "CustomizationWidget.generated.h"

class ULoadoutManager;

USTRUCT()
struct FCompatibleSkinsArray
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<ULoadoutCustomization*> CompatibleSkins;
};

UCLASS(BlueprintType)
class READYORNOT_API ULoadoutItem : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
	FText Name;

	UPROPERTY(BlueprintReadOnly)
	FText Description;
	
	UPROPERTY(BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> Icon;
};

UCLASS(BlueprintType)
class READYORNOT_API ULoadoutEquipment : public ULoadoutItem
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TSubclassOf<ABaseItem> Class;
};

UCLASS(BlueprintType)
class READYORNOT_API ULoadoutWeapon : public ULoadoutEquipment
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	TArray<EWeaponAttachmentType> GetSupportedAttachmentTypes() const;
	
	UFUNCTION(BlueprintCallable)
	TArray<TSubclassOf<UWeaponAttachment>> GetSupportedAttachmentsOfType(EWeaponAttachmentType Type) const;
};

UCLASS(BlueprintType)
class READYORNOT_API ULoadoutCustomization : public ULoadoutItem
{
	GENERATED_BODY()

public:
	UPROPERTY()
	UCustomizationDataBase* Asset;

	UPROPERTY(BlueprintReadOnly)
	FText Variant;

	UPROPERTY(BlueprintReadOnly)
	TSoftObjectPtr<UMaterialInterface> VariantIcon;
	
	UPROPERTY(BlueprintReadOnly)
	bool bLocked;

	UPROPERTY(BlueprintReadOnly)
	FText RequirementsText;

	UPROPERTY(BlueprintReadOnly)
	TArray<ULoadoutCustomization*> Children;
	
	UFUNCTION(BlueprintCallable)
	ECustomizationType GetCustomizationType() const { return Asset ? Asset->Type : ECustomizationType::Any; }
};

/**
 * 
 */
UCLASS(Abstract)
class READYORNOT_API UCustomizationWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

	friend class ULoadoutEquipmentItem;
	
protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;
	
	/*
	 *	Loadout
	 */
	UFUNCTION(BlueprintCallable)
	const TArray<ULoadoutWeapon*>& GetPrimaryWeapons() { return CachedPrimaryWeapons; }
	
	UFUNCTION(BlueprintCallable)
	const TArray<ULoadoutWeapon*>& GetSecondaryWeapons() { return CachedSecondaryWeapons; }

	UFUNCTION(BlueprintCallable)
	const TArray<ULoadoutEquipment*>& GetLongTacticalItems() { return CachedLongTacticalItems; }

	UFUNCTION(BlueprintCallable)
	const TArray<ULoadoutEquipment*>& GetTacticalItems() { return CachedTacticalItems; }

	UFUNCTION(BlueprintCallable)
	const TArray<ULoadoutEquipment*>& GetArmorItems() { return CachedArmorItems; }
	
	UFUNCTION(BlueprintCallable)
	const TArray<ULoadoutEquipment*>& GetHelmetItems() { return CachedHelmetItems; }
	
	UFUNCTION(BlueprintCallable)
	ULoadoutWeapon* GetPrimaryWeapon() const;

	UFUNCTION(BlueprintCallable)
	ULoadoutWeapon* GetSecondaryWeapon() const;
	
	UFUNCTION(BlueprintCallable)
	ULoadoutEquipment* GetLongTactical() const;

	UFUNCTION(BlueprintCallable)
	ULoadoutEquipment* GetArmor() const;

	UFUNCTION(BlueprintCallable)
	ULoadoutEquipment* GetHelmet() const;
	
	UFUNCTION(BlueprintCallable)
	void SetPrimaryWeapon(ULoadoutWeapon* Item);

	UFUNCTION(BlueprintCallable)
	void SetSecondaryWeapon(ULoadoutWeapon* Item);

	UFUNCTION(BlueprintCallable)
	void SetLongTactical(ULoadoutEquipment* Item);

	UFUNCTION(BlueprintCallable)
	void SetArmor(ULoadoutEquipment* Item);

	UFUNCTION(BlueprintCallable)
	void SetHelmet(ULoadoutEquipment* Item);
	
	UFUNCTION(BlueprintCallable)
	void SetPrimaryAttachment(UWeaponAttachment* Attachment);

	UFUNCTION(BlueprintCallable)
	void SetSecondaryAttachment(UWeaponAttachment* Attachment);
	
	/*
	 *	Customization
	 */
	TArray<ULoadoutCustomization*> GetCustomizationSkins(ABaseItem* Item);
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
	TArray<ULoadoutCustomization*> GetCustomizationItems(ECustomizationType Type, bool bFamiliesOnly);
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
	ULoadoutCustomization* GetEquippedCustomizationItem(ECustomizationType Type);
	
	UFUNCTION(BlueprintCallable)
	void SetCustomizationItem(ULoadoutCustomization* Item);
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool CanCustomizeType(ECustomizationType Type);

	UFUNCTION(BlueprintCallable)
	void PreviewCustomizationItem(ULoadoutCustomization* Item);
	
	UFUNCTION(BlueprintCallable)
	void ClearCustomizationPreview();

	UPROPERTY(EditDefaultsOnly)
	TMap<ECustomizationType, UFMODEvent*> CustomizationEquipEvents;
	
	/*
	 * General
	 */
	UFUNCTION(BlueprintCallable)
	void ExitCustomization();
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
	EEquippingSwat GetActiveSwatMember() const { return CurrentSwat; }

	UFUNCTION(BlueprintCallable)
	void SetActiveSwatMember(EEquippingSwat InSwat);
	
private:
	UPROPERTY(Transient)
	ULoadoutManager* LoadoutManager;

	UPROPERTY(Transient)
	class UBaseProfile* Profile;
	
	EEquippingSwat CurrentSwat;
	
	UPROPERTY(Transient)
	FSavedLoadout CurrentLoadout;

	UPROPERTY(Transient)
	AReadyOrNotCharacter* PreviewCharacter;
	
	UPROPERTY(Transient)
	TArray<ULoadoutWeapon*> CachedPrimaryWeapons;

	UPROPERTY(Transient)
	TArray<ULoadoutWeapon*> CachedSecondaryWeapons;

	UPROPERTY(Transient)
	TArray<ULoadoutEquipment*> CachedLongTacticalItems;

	UPROPERTY(Transient)
	TArray<ULoadoutEquipment*> CachedTacticalItems;

	UPROPERTY(Transient)
	TArray<ULoadoutEquipment*> CachedArmorItems;
	
	UPROPERTY(Transient)
	TArray<ULoadoutEquipment*> CachedHelmetItems;
	
	void CacheLoadoutItems();

	UPROPERTY(Transient)
	FSavedCustomization CurrentCustomization;

	UPROPERTY(Transient)
	TArray<ULoadoutCustomization*> CachedCustomizationItems;

	UPROPERTY(Transient)
	TArray<ULoadoutCustomization*> CachedCustomizationFamilies;
	
	UPROPERTY(Transient)
	TMap<FName, FCompatibleSkinsArray> ItemToSkinsMap;
	
	void CacheCustomizationItems();

	void ApplyCustomization(const FSavedCustomization& InCustomization);
	
	void HandlePreviewCharacterItemsChanged();
	
public:
	/*
	 * temp -killo
	 */
	UFUNCTION(BlueprintImplementableEvent)
	void PrimarySet(TSubclassOf<ABaseItem> Class);
	
	UFUNCTION(BlueprintImplementableEvent)
	void SecondarySet(TSubclassOf<ABaseItem> Class);
	
	UFUNCTION(BlueprintImplementableEvent)
	void ArmorSet(TSubclassOf<ABaseItem> Class);
	
	UFUNCTION(BlueprintImplementableEvent)
	void HelmetSet(TSubclassOf<ABaseItem> Class);
	
	UPROPERTY(Transient, BlueprintReadWrite)
	ABaseItem* CustomizationTargetPrimaryWeapon;

	UPROPERTY(Transient, BlueprintReadWrite)
	ABaseItem* CustomizationTargetSecondaryWeapon;

	UPROPERTY(Transient, BlueprintReadWrite)
	ABaseItem* CustomizationTargetArmor;

	UPROPERTY(Transient, BlueprintReadWrite)
	ABaseItem* CustomizationTargetHelmet;
	/*
	* temp -killo
	*/
};
