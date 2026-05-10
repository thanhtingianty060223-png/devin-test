// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameFeatureLibrary.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UGameFeatureLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UGameFeatureLibrary();
	/**
	 * Checks if a feature is enabled, either via DLC or the installed game version
	 * @param InFeature Game Feature to check
	 * @return True if enabled, false if not
	 */
	UFUNCTION(BlueprintPure, Category = "Game Features")
	static bool IsFeatureEnabled(EGameFeature InFeature);
	
	/**
	 * Checks if the user has a particular game version or DLC
	 * @param InFeature Game Feature to check
	 * @return True if enabled, false if not
	 */
	UFUNCTION(BlueprintPure, Category = "Game Features")
	static bool IsGameVersionEnabled(EGameVersionRestriction InFeature);

	/**
	 * @brief Is the game the full name (excluding any DLCs)
	 * @return true if full game, false if demo
	 */
	UFUNCTION(BlueprintPure, Category = "Game Features")
	static bool IsFullGame();

	/**
	 * @brief Is the game running in Demo Mode
	 * @return true if demo, false if not
	 */
	UFUNCTION(BlueprintPure, Category = "Game Features")
	static bool IsGameDemo();

private:
	static TMap<EGameVersionRestriction, bool> Cache;
	static TMap<EGameFeature, EGameVersionRestriction> FeatureRestrictions;
};
