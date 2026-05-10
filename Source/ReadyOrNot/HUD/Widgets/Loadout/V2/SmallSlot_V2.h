// Copyright Void Interactive, 2023

#pragma once
#include "CommonUserWidget.h"

#include "SmalLSlot_V2.generated.h"

UCLASS()
class READYORNOT_API USmallSlot_V2 : public UCommonUserWidget
{
	GENERATED_BODY()

protected:
	UFUNCTION(BlueprintCallable)
	void SetEquipped(bool IsEquipped);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool GetEquipped();

	UFUNCTION(BlueprintImplementableEvent)
	void OnEquipped();
	
	UFUNCTION(BlueprintCallable)
	void SetText(FText Text);

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	class UTextBlock* ItemName;

private:
	UPROPERTY()
	bool Equipped;
};
