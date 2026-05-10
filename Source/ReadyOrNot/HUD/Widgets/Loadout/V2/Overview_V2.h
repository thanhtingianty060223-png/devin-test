// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "Overview_V2.generated.h"

UCLASS()
class READYORNOT_API UOverview_V2 : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void SavePresets(FSavedLoadout loadout, bool isNPC);
	
	UPROPERTY(BlueprintReadWrite)
	FString ActiveLoadoutName;

protected:
	UPROPERTY(BlueprintReadOnly, Transient, meta=(BindWidgetAnim))
	UWidgetAnimation* SwitchCharacter;
};
