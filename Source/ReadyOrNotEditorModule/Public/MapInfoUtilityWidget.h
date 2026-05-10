// Copyright Void Interactive, 2019

#pragma once

#include "CoreUtilityWidget.h"
#include "ReadyOrNot/Data/LevelData.h"
#include "MapInfoUtilityWidget.generated.h"

UCLASS()
class READYORNOTEDITORMODULE_API UMapInfoUtilityWidget : public UCoreUtilityWidget
{
	GENERATED_BODY()

	UFUNCTION(BlueprintPure)
	FString GetDebugString();

	UFUNCTION(BlueprintPure)
	FString GetLevelDetails();

	UFUNCTION(BlueprintPure)
	UTexture2D* GetLevelForegroundImage();

	UFUNCTION(BlueprintPure)
	UTexture2D* GetLevelBackgroundImage();

	UFUNCTION(BlueprintPure)
	FString GetAITableNames();

	UFUNCTION(BlueprintPure)
	FLevelDataLookupTable GetLevelData();

	UFUNCTION(BlueprintCallable)
	void RegenWorld();
};
