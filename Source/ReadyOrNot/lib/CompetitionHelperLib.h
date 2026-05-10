// Copyright Void Interactive, 2021

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CompetitionHelperLib.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UCompetitionHelperLib : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	static void AddKill(int32 EventID, FString SteamID, FString SteamName, FString KilledPlayer);
	static void AddDeath(int32 EventID, FString SteamID, FString SteamName, FString KilledBy);
	static void AddArrest(int32 EventID, FString SteamID, FString SteamName, FString ArrestedPlayer);
	static void AddScore(int32 EventID, FString SteamID, FString SteamName, float Score);
	static void AddWin(int32 EventID, FString SteamID, FString SteamName, FString ServerName, FString ServerMode);
	
};
