// © Void Interactive, 2017

#pragma once

#include "ReadyOrNotGameState.h"
#include "Actors/Gameplay/TugOfWarMover.h"
#include "TugOfWarGS.generated.h"

UCLASS(Blueprintable, BlueprintType)
class READYORNOT_API ATugOfWarGS : public AReadyOrNotGameState
{
	GENERATED_BODY()

public:
	ATugOfWarGS();
	virtual float GetWinningScore(bool& bUsesScoring) override;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Tug of War")
	ATugOfWarMover* Mover;
};