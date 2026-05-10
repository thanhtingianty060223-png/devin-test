// © Void Interactive, 2017

#pragma once

#include "ReadyOrNotGameState.h"
#include "ArrestAndRescueGS.generated.h"

/**
 *
 */
UCLASS()
class READYORNOT_API AArrestAndRescueGS : public AReadyOrNotGameState
{
	GENERATED_BODY()

public:
	AArrestAndRescueGS();

	UPROPERTY(BlueprintReadOnly, Replicated)
	int32 BlueRespawnWaves = 1;
	UPROPERTY(BlueprintReadOnly, Replicated)
	int32 RedRespawnWaves = 1;


};
