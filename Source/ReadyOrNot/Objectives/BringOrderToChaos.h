// Copyright Void Interactive, 2021

#pragma once

#include "CoreMinimal.h"
#include "Objectives/Objective.h"
#include "BringOrderToChaos.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API ABringOrderToChaos : public AObjective
{
	GENERATED_BODY()

	ABringOrderToChaos();

	virtual void TickObjective() override;
};
