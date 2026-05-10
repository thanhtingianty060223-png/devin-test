// Copyright Void Interactive, 2023

#pragma once

#include "CommonUserWidget.h"
#include "ItemSlot_V2.generated.h"

UCLASS()
class READYORNOT_API UItemSlot_V2 : public UCommonUserWidget
{
	GENERATED_BODY()
	
protected:
	UFUNCTION(BlueprintCallable)
	void SetItem(ABaseItem* Item);

	UFUNCTION(BlueprintCallable)
	void SetArmorMaterial(UArmourMaterial* Armor);

	UFUNCTION(BlueprintCallable)
	void SetTexts(FText Name, FText Type);

	UPROPERTY(BlueprintReadWrite)
	class ABaseItem* BaseItem;

	UPROPERTY(BlueprintReadWrite)
	class UArmourMaterial* ArmorMaterial;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	class UTextBlock* ItemName;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	class UTextBlock* ItemType;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	class UHorizontalBox* PrefixText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    FString PresetName;
};
