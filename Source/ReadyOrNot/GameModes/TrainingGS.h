// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "ReadyOrNotGameState.h"
#include "TrainingGS.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API ATrainingGS : public AReadyOrNotGameState
{
	GENERATED_BODY()

protected:
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;

private:
	bool bFinishedLoading = false;
};
