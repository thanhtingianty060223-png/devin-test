// Void Interactive, 2020

#pragma once

#include "CoreMinimal.h"
#include "Actors/Environment/ReadyOrNotTriggerVolume.h"
#include "LobbyFiringRangeArea.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API ALobbyFiringRangeArea : public AReadyOrNotTriggerVolume
{
	GENERATED_BODY()

	ALobbyFiringRangeArea();

	public:
	static bool IsInFiringRange(AActor* Actor);
	
};
