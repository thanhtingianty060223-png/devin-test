// Copyright Void Interactive, 2023

#pragma once

#include "CommonUserWidget.h"
#include "TacticalSlot_V2.generated.h"

// UENUM(BlueprintType)
// enum ELoadoutTacticalSlotType
// {
// 	TacticalSlot = 0,
// 	PrimaryAP = 1,
// 	PrimaryJHP = 2,
// 	SecondaryAP = 3,
// 	SecondaryJHP = 4,
// };

UCLASS()
class READYORNOT_API UTacticalSlot_V2 : public UCommonUserWidget
{
	GENERATED_BODY()

// public:
// 	UPROPERTY(BlueprintReadOnly, EditAnywhere)
// 	TEnumAsByte<ELoadoutTacticalSlotType> SlotType = TacticalSlot; 
// 	
// 	UPROPERTY(BlueprintReadOnly, EditAnywhere)
// 	TSubclassOf<ABaseItem> SlotItem;
// 
// 	UPROPERTY(EditAnywhere)
// 	FText SlotName;
// 	 
// protected:
// 	virtual void NativePreConstruct() override;
// 	virtual void NativeConstruct() override;
// 
// 	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
// 	class UTextBlock* SlotNameWidget;
// 
// 	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
// 	class UTextBlock* SlotCountWidget;
// 
// 	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
// 	class UTextBlock* ItemNameWidget;
// 
// private:
// 	UPROPERTY()
// 	AReadyOrNotPlayerController* PlayerController;
};
