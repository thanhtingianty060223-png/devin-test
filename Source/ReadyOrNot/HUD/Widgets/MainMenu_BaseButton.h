// Copyright Void Interactive, 2023

#pragma once

#include "CommonButtonFMOD.h"
#include "MainMenu_BaseButton.generated.h"

UCLASS()
class READYORNOT_API UMainMenu_BaseButton : public UCommonButtonFMOD
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Main Menu Button")
	FText ButtonText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Main Menu Button")
	FSlateColor NormalTextColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Main Menu Button")
	FSlateColor HoveredTextColor;

	// 	UFUNCTION(BlueprintImplementableEvent)
	// 	void OnSelectedChanged(bool Selected);
	// 
	// protected:
	// 	UFUNCTION(BlueprintCallable, BlueprintPure)
	// 	bool GetSelected();
	// 	UFUNCTION(BlueprintCallable)
	// 	void SetSelected(bool Selected);
	// 
	// private:
	// 	uint8 bSelected : 1;
};
