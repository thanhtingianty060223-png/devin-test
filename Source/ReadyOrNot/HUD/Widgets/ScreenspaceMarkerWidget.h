// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "ScreenspaceMarkerWidget.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UScreenspaceMarkerWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void ShowWidget();

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void HideWidget();
};
