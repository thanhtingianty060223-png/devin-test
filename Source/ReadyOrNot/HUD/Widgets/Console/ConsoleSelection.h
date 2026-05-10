// Copyright Void Interactive, 2022

#pragma once

#include "ConsoleSelectionItem.h"
#include "Blueprint/UserWidget.h"
#include "ConsoleSelection.generated.h"

UCLASS()
class READYORNOT_API UConsoleSelection : public UUserWidget
{
	GENERATED_BODY()

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	class UHorizontalBox* Container;

	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	class UImage* Button;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString ButtonName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	EItemCategory ItemCategory; 

	UFUNCTION()
	void ItemAdded(ABaseItem* Item);
	UFUNCTION()
	void ItemRemoved(ABaseItem* Item);
	void InitializeButton();
	UFUNCTION()
	void ItemEquipped(ABaseItem* Item);

	void AddInitialInventory();
	UConsoleSelectionItem* TryGetWidgetForItem(ABaseItem* Item) const;

	virtual bool Initialize() override;
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UInventoryComponent* GetPlayerInventory();
};
