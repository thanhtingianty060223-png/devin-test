// Copyright Void Interactive, 2017

#pragma once

#include "CoreMinimal.h"
#include "SteamworksIntegration.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API USteamworksIntegration : public UObject
{

	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = Steamworks)
	static bool IsSteamEnabled();
	
	UE_DEPRECATED(4.27, "Please use UGameFeatureLibrary Instead")
	UFUNCTION(BlueprintPure, Category = Steamworks)
	static bool IsDLCInstalled(int32 AppID);

	UE_DEPRECATED(4.27, "Please use UGameFeatureLibrary Instead")
	UFUNCTION(BlueprintPure, Category = Steamworks)
	static bool IsDLCInstalledEnum(EGameVersionRestriction GameFeature);
	
	UFUNCTION(BlueprintPure, Category = Steamworks)
	static int32 GetDLCNumber(EGameVersionRestriction InDLC);
	
	UFUNCTION(BlueprintPure, Category = Steamworks)
	static EGameVersionRestriction GetDLCENum(int32 InDLC);
};
