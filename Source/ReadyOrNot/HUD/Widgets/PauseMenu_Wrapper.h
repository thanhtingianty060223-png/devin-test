// Copyright Void Interactive, 2023

#pragma once

#include "CommonUserWidget.h"
#include "PauseMenu_Wrapper.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPauseMenuClosedDelegate);

UCLASS()
class READYORNOT_API UPauseMenu_Wrapper : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void OpenPauseMenu();

	UFUNCTION(BlueprintCallable)
	void ClosePauseMenu();

	UPROPERTY(BlueprintReadWrite, meta=(BindWidget))
	class UCommonActivatableWidgetStack* WidgetStack;

	UPROPERTY(BlueprintReadWrite)
	class UCommonActivatableWidget* PauseMenu;

	UPROPERTY(BlueprintAssignable)
	FOnPauseMenuClosedDelegate OnPauseMenuClosed;
};
