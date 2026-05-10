// Void Interactive, 2020

#pragma once

#include "Actors/ActorSpawnPoint.h"
#include "IncriminationClueSpawnPoint.generated.h"

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType, HideCategories=("Rendering", "Collision", "Input", "Actor", "HLOD", "LOD", "Cooking"))
class READYORNOT_API AIncriminationClueSpawnPoint : public AActorSpawnPoint
{
	GENERATED_BODY()

public:
	AIncriminationClueSpawnPoint();

	void StartClueTimer();
	void StopClueTimer();
    
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup")
	TSubclassOf<class AIncriminationClue> IncriminationClueClass = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup")
	TSubclassOf<AActor> ClueFlareClass = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup", meta = (ClampMin = 1, UIMin = 1, EditCondition = "IncriminationClueClass != nullptr"))
    uint8 ClueNumber = 1;

	// The amount of time (in seconds) until this clue's location is revealed, if not found
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup", meta = (EditCondition = "IncriminationClueClass != nullptr"))
    float ShowObjectiveMarkerIn = 120.0f;
		
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup|UI", meta = (EditCondition = "IncriminationClueClass != nullptr"))
    FText ClueName = FText::FromString("Clue");
    
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup|UI", meta = (EditCondition = "IncriminationClueClass != nullptr"))
    FText ClueFoundMessage = FText::FromString("Clue Found");
	
protected:
    void BeginPlay() override;

	void Tick(float DeltaTime) override;

	void OnClueTimerExpired();

	void SpawnClueFlare();

private:
	FTimerHandle TH_ClueTimerExpiry;

	uint8 bClueTimerStarted : 1;
};
