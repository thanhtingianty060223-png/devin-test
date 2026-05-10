// Void Interactive, 2017

#pragma once

#include "Components/BoxComponent.h"
#include "ShotDetectionVolume.generated.h"

// Shot Detection Volumes do exactly what you think they do. They listen for gunfire and fire off an event when triggered.
// There's a number of uses for this - alerting birds, setting off sprinklers, etc
UCLASS(Blueprintable, BlueprintType)
class READYORNOT_API AShotDetectionVolume : public AActor
{
	GENERATED_BODY()

public:
	AShotDetectionVolume();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shot Detection")
	UBoxComponent* Bounds;

	// NOTE: This is done on the server! 
	UFUNCTION(BlueprintImplementableEvent, Category = "Shot Detection")
	void OnShotFired(class ABaseWeapon* FiringWeapon, class APlayerCharacter* FiringPlayer);
};
