// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "LoadoutCategory.h"
#include "LoadoutSlotWidget.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API ULoadoutSlotWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void SetActive(bool IsActive);

	UPROPERTY(BlueprintReadWrite)
	bool IsActive;
	
	UPROPERTY(BlueprintReadWrite, meta=(ExposeOnSpawn))
	EItemCategory LoadoutSlot;

	UPROPERTY(BlueprintReadWrite, meta=(ExposeOnSpawn))
	TSubclassOf<ABaseItem> ItemData;

	UPROPERTY(BlueprintReadWrite)
	bool bIsPrimary = false;

	UPROPERTY(BlueprintReadWrite)
	bool bIsAmmunition = false;

	UPROPERTY(BlueprintReadWrite)
	FAmmoTypeData AmmoType;

	UPROPERTY(BlueprintReadWrite)
	bool bIsArmourMaterial = false;

	UPROPERTY(BlueprintReadWrite)
	UArmourMaterial *ArmourMaterialData = nullptr;

	UPROPERTY(BlueprintReadWrite)
	EWeaponAttachmentType AttachmentSlot;
	
	UPROPERTY()
	TArray<FLoadoutCategory> GearCategoryClasses;

private:
};
