// Copyright Void Interactive, 2021

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Enums.h"
#include "ReadyOrNotStatics.generated.h"

struct FStackupPattern
{
	ESquadPosition SquadPosition = ESquadPosition::SP_NONE;
	FVector2D VectorOffset = FVector2D::ZeroVector;
	float YawOffset = 0.0f;

	FStackupPattern(ESquadPosition sp, FVector2D vo, float yo)
	{
		SquadPosition = sp;
		VectorOffset = vo;
		YawOffset = yo;
	}
	
};

UCLASS()
class READYORNOT_API UReadyOrNotStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Ready Or Not Statics")
	static class AReadyOrNotLevelScript* GetReadyOrNotLevelScript();

	UFUNCTION(BlueprintPure, Category = "Ready Or Not Statics")
	static class UReadyOrNotGameInstance* GetReadyOrNotGameInstance();

	UFUNCTION(BlueprintPure, Category = "Ready Or Not Statics")
	static class AReadyOrNotGameMode* GetReadyOrNotGameMode();

	UFUNCTION(BlueprintPure, Category = "Ready Or Not Statics")
	static class AReadyOrNotGameState* GetReadyOrNotGameState();

	UFUNCTION(BlueprintPure, Category = "Ready Or Not Statics")
	static class AConversationManager* GetConversationManager();

	UFUNCTION(BlueprintPure, Category = "Ready Or Not Statics")
    static class AReadyOrNotPlayerController* GetReadyOrNotPlayerController();

	UFUNCTION(BlueprintPure, Category = "Ready Or Not Statics")
	static bool DoesMapExist(FString Map);
};
