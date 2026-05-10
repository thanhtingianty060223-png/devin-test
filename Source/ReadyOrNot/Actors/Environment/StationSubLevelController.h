// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "StationSubLevelController.generated.h"

UCLASS(HideCategories=("Rendering", "Replication", "Collision", "Input", "Actor", "LOD", "Cooking"))
class READYORNOT_API AStationSubLevelController : public AActor
{
	GENERATED_BODY()

public:
	AStationSubLevelController();
	
	virtual void BeginPlay() override;

private:
	// Level that this sub level controller will attempt to load if all checks pass
	UPROPERTY(EditAnywhere, Category="SubLevel Controller", meta=(GetOptions="GetStreamingLevelOptions"))
	FName LevelToLoad;

	/*
	 * Once this level has been completed, this sub level controller will load the specified level.
	 * This will always be loaded if left as 'None'
	 */
	UPROPERTY(EditAnywhere, Category="SubLevel Controller", meta=(GetOptions="GetCampaignLevelOptions", EditCondition="!bIsMultiplayerOnly"))
	FName EnableAfterCompleting;

	/*
	 * Once this level has been completed, this sub level controller will no longer load the specified level
	 * This is ignored if left as 'None'
	 */
	UPROPERTY(EditAnywhere, Category="SubLevel Controller", meta=(GetOptions="GetCampaignLevelOptions", EditCondition="!bIsMultiplayerOnly"))
	FName DisableAfterCompleting;

	/*
	 * Toggle this on if you want this level to always be loaded in multiplayer
	 */
	UPROPERTY(EditAnywhere, Category="SubLevel Controller")
	bool bIsMultiplayerOnly = false;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	UBillboardComponent* BillboardComponent;
#endif

	void LoadSubLevel();
	int32 GetCurrentCampaignIndex(const UCampaignData* CampaignData) const;
	
	UFUNCTION(CallInEditor)
	TArray<FString> GetStreamingLevelOptions() const;
	
	UFUNCTION(CallInEditor)
	TArray<FString> GetCampaignLevelOptions() const;
};
