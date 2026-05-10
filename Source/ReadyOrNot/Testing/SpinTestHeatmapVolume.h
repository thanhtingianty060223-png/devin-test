// Void Interactive, 2020

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Volume.h"
#include "SpinTestHeatmapVolume.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API ASpinTestHeatmapVolume : public AVolume
{
	GENERATED_BODY()

	ASpinTestHeatmapVolume();

	UFUNCTION(CallInEditor, Category = "AVisualize")
	void VisualizeHeatMapIfExists();

	UFUNCTION(CallInEditor, Category = "AVisualize")
	void FlushVisualization();

	float GetMinFPSAtSpot(FVector Location, TArray<FString> Data, int32 index);
	
};
