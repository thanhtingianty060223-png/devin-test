// Void Interactive, 2020

#pragma once

#include "CoreMinimal.h"

#include "Engine/TriggerBox.h"
#include "ReadyOrNotTriggerVolume.generated.h"

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class READYORNOT_API AReadyOrNotTriggerVolume : public ATriggerBox
{
	GENERATED_BODY()

	protected:
	AReadyOrNotTriggerVolume();
	virtual void BeginPlay() override;

	void GenerateOverlappingActors();

	// Classes this volume should test for overlaps - Caution Adding AActor* as performance may suffer
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TSubclassOf<AActor>> OverlappingClasses;

	// Actors that we should test for overlaps (either statically placed in the world or added OnActorSpawned)
	UPROPERTY(VisibleAnywhere)
	TArray<AActor*> TestActors;

	int32 TestIdx;
	
	UFUNCTION()
	void OnActorSpawned(AActor* Actor);

public:
	bool IsOverlappingActor(AActor* Actor);
	bool IsOverlappingAllActorsOfType();
	int32 GetOverlappingCount();
	
};
