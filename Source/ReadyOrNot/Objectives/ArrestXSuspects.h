// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "Objectives/Objective.h"
#include "ArrestXSuspects.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API AArrestXSuspects : public AObjective
{
	GENERATED_BODY()

	AArrestXSuspects();

	UPROPERTY(EditAnywhere)
	int32 RequiredArrests = 1;

	virtual void BeginPlay() override;
	virtual void TickObjective() override;

	virtual FText GetFormattedName() override;
	virtual FText GetFormattedDescription() override;
};
