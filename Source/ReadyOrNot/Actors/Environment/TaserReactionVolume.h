// � Void Interactive, 2020

#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "TaserReactionVolume.generated.h"

// When a taser probe hits a person, it does a radial trace for Taser Reaction Volumes.
// These are scriptable components that we can use for different things (such as tasing people underwater)
UCLASS(Blueprintable, BlueprintType)
class READYORNOT_API ATaserReactionVolume : public AActor
{
	GENERATED_BODY()

public:
	ATaserReactionVolume();

	// Called when we have hit someone.
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = Environmental)
	void OnTaserStunDelivered(class AReadyOrNotCharacter* Character, class ATaser* Taser);

	// The thing that determines the bounds of this volume.
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Environmental)
		UBoxComponent* Bounds;
};