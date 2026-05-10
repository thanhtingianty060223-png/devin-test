// © Void Interactive, 2017

#pragma once

#include "ReadyOrNotGameState.h"
#include "FirefightGS.generated.h"

UCLASS(Blueprintable, BlueprintType)
class READYORNOT_API AFirefightGS : public AReadyOrNotGameState
{
	GENERATED_BODY()

public:
	virtual float GetWinningScore(bool& bUsesScoring) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objectives")
		FText FreeTextLocalized;
};