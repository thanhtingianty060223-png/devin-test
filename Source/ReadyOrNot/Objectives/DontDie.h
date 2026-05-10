// Copyright Void Interactive, 2021

#pragma once

#include "CoreMinimal.h"
#include "Objectives/Objective.h"
#include "DontDie.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API ADontDie : public AObjective
{
	GENERATED_BODY()
	ADontDie();

	virtual void TickObjective() override;
};
