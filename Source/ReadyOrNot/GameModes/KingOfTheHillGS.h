// © Void Interactive, 2017

#pragma once

#include "ReadyOrNotGameState.h"
#include "Actors/Gameplay/TugOfWarMover.h"
#include "KingOfTheHillGS.generated.h"

UCLASS(Blueprintable, BlueprintType)
class READYORNOT_API AKingOfTheHillGS : public AReadyOrNotGameState
{
	GENERATED_BODY()

public:
	AKingOfTheHillGS();
	virtual float GetWinningScore(bool& bUsesScoring) override;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Tug of War")
	ATugOfWarMover* Mover;
};