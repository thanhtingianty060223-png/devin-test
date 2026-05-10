// Copyright Void Interactive, 2023

#pragma once

#include "CommonUserWidget.h"
#include "lib/ReadyOrNotLoadoutManager.h"
#include "LoadoutSlot_V2.generated.h"

UCLASS()
class READYORNOT_API ULoadoutSlot_V2 : public UCommonUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	
	UFUNCTION(BlueprintCallable)
	void SetEquipped(bool IsEquipped);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool GetEquipped();

	UFUNCTION(BlueprintImplementableEvent)
	void OnEquipped();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void ShowSubtext(const FText& Text, bool Show);

	UFUNCTION(BlueprintCallable)
	void SetItem(ABaseItem* Item);

	UFUNCTION(BlueprintCallable)
	void SetAttachment(UWeaponAttachment* WeaponAttachment);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	TSubclassOf<UWeaponAttachment> GetAttachmentClass();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void SetArmorMaterial(UArmourMaterial* Item);

	UFUNCTION(BlueprintCallable)
	void SetAmmoMunition(FName AmmoName, bool Secondary);

	UFUNCTION(BlueprintCallable)
	void SetTacticalMunition(ELoadoutMunitionSlotType Munition, ABaseItem* TacticalItem);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void OnSlotsUpdated();

	UFUNCTION(BlueprintCallable)
	void IncrementSlots();

	UFUNCTION(BlueprintCallable)
	void DecrementSlots();

	UFUNCTION(BlueprintCallable)
	void UpdateSlotCount();
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void RefreshInfo();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void RefreshItemImage();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void RefreshAttachmentImage();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void SetVisualState(bool Hovered, bool Pressed, bool Equipped);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void SetStyle(bool UseGamepad);

	UPROPERTY(BlueprintReadWrite)
	class ABaseItem* BaseItem;

	UPROPERTY(BlueprintReadWrite)
	TSubclassOf<ABaseItem> BaseItemClass;

	UPROPERTY(BlueprintReadWrite)
	class ABaseWeapon* BaseWeapon;

	UPROPERTY(BlueprintReadWrite)
	class ABaseArmour* BaseArmor;

	UPROPERTY(BlueprintReadWrite)
	class UArmourMaterial* ArmorMaterial;

	UPROPERTY(BlueprintReadOnly)
	FName AmmunitionName;

	UPROPERTY(BlueprintReadOnly)
	class UTexture2D* ItemImage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UTexture2D* EmptyImage;

	UPROPERTY(BlueprintReadOnly)
	FText ItemName;

	UPROPERTY(BlueprintReadOnly)
	FText ItemType;

	UPROPERTY(BlueprintReadOnly)
	FText ItemSubtext;

	UPROPERTY(BlueprintReadOnly)
	int SlotCount;

	UPROPERTY(BlueprintReadWrite)
	EWeaponAttachmentType AttachmentType;

	UPROPERTY(BlueprintReadOnly)
	class UWeaponAttachment* Attachment;

	UPROPERTY(BlueprintReadWrite)
	ELoadoutMunitionSlotType MunitionType;

	UPROPERTY(BlueprintReadWrite)
	bool bHovered = false;

	UPROPERTY(BlueprintReadWrite)
	bool bPressed = false;

	UPROPERTY(BlueprintReadWrite)
	bool bUseGamepad = false;

	UPROPERTY(BlueprintReadOnly)
	bool bAttachment = false;

private:
	UPROPERTY()
	class AReadyOrNotGameState* gs;

	UPROPERTY()
	class UReadyOrNotLoadoutManager* LoadoutFunctionLibrary;

	UPROPERTY()
	bool bEquipped = false;
};
