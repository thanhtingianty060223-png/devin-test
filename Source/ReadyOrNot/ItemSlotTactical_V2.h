// Copyright Void Interactive, 2023

#pragma once

#include "CommonUserWidget.h"
#include "ItemSlotTactical_V2.generated.h"

UENUM(BlueprintType)
enum ELoadoutTacticalSlotType
{
	TacticalSlot = 0,
	PrimaryAmmunition = 1,
	SecondaryAmmunition = 2,
	GrenadeSlot = 3, 
};

UCLASS()
class READYORNOT_API UItemSlotTactical_V2 : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TEnumAsByte<ELoadoutTacticalSlotType> SlotType = TacticalSlot; 
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSubclassOf<ABaseItem> SlotItem;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName SlotAmmunitionName;

	UPROPERTY(EditAnywhere)
	FText SlotName;

	UPROPERTY()
	FCustomWidgetNavigationDelegate NavigationDelegate;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FText GetItemName();

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FText GetItemDescription();

protected:
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	class UTextBlock* ItemName;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	class UTextBlock* ItemType;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	class UTextBlock* ItemCount;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	class UButton* LeftArrow;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	class UButton* RightArrow;
	
	virtual void NativePreConstruct() override;

	UFUNCTION(BlueprintCallable)
	UWidget* OnNavigateLeft(EUINavigation NavigationRule);

	UFUNCTION(BlueprintCallable)
	UWidget* OnNavigateRight(EUINavigation NavigationRule);

	UFUNCTION(BlueprintCallable)
	void Refresh();

	UFUNCTION(BlueprintImplementableEvent)
	void OnSlotsUpdated();

};
