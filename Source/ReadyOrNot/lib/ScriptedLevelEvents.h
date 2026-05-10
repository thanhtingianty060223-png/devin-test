// Copyright Void Interactive, 2021

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "ScriptedLevelEvents.generated.h"

UCLASS()
class READYORNOT_API UScriptedLevelEvents final : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable)
	static ACyberneticCharacter* GetCyberneticsCharacterByTag(FName Tag);

	UFUNCTION(BlueprintCallable)
	static ACyberneticController* GetCyberneticsControllerByTag(FName Tag);

	UFUNCTION(BLueprintCallable)
	static void GiveWorldBuildingActivityByTag(class ACyberneticController* Controller, FName Tag, float TimeDoingActivity = 0.0f);
};
