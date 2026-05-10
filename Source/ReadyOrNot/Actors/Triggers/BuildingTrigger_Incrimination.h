// Void Interactive, 2020

#pragma once

#include "Actors/Triggers/BuildingTrigger.h"
#include "BuildingTrigger_Incrimination.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API ABuildingTrigger_Incrimination : public ABuildingTrigger
{
	GENERATED_BODY()

public:
	ABuildingTrigger_Incrimination();
	
	// A list of every potential clue spawn that's tied to this evidence building
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Setup", meta = (DisplayAfter="SpacingPerFloor"))
	TArray<class AIncriminationClueSpawnPoint*> ClueSpawnPoints;

protected:
	void BeginPlay() override;
};
