// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LoadoutInformationTableWidget.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API ULoadoutInformationTableWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnModifyWeaponButtonClicked, ABaseItem*, ItemToModify);
	UPROPERTY(BlueprintAssignable, Category = Events)
	FOnModifyWeaponButtonClicked OnModifyWeaponButtonClicked;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInfoPanelRemoved);
	UPROPERTY(BlueprintAssignable, Category = Events)
	FOnInfoPanelRemoved OnInfoPanelRemoved;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCancelRefresh);
	UPROPERTY(BlueprintAssignable, Category = Events)
	FOnCancelRefresh OnCancelRefresh;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInfoPanelAttachmentClicked, EWeaponAttachmentType, WeaponSlot, UWeaponAttachment *, AttachmentData);
	UPROPERTY(BlueprintAssignable, Category = Events)
	FOnInfoPanelAttachmentClicked OnInfoPanelAttachmentClicked;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInfoPanelRemoveAttachmentClicked, EWeaponAttachmentType, WeaponSlot);
	UPROPERTY(BlueprintAssignable, Category = Events)
	FOnInfoPanelRemoveAttachmentClicked OnInfoPanelRemoveAttachmentClicked;
	
	UFUNCTION()
	void RefreshPanelItemInfo(FSavedLoadout ActiveLoadout, TSubclassOf<ABaseItem> ItemClass, EItemCategory LoadoutSlot, bool bRestricted = false);

	UFUNCTION()
	void RefreshPanelAmmoInfo(FAmmoTypeData AmmoType, TSubclassOf<ABaseItem> ItemClass);

	UFUNCTION()
	void RefreshPanelArmourMaterial(UArmourMaterial *ArmourMaterial);

	
};
