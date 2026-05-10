// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "EnvQueryContext_GasSafePoints.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UEnvQueryContext_GasSafePoints : public UEnvQueryContext
{
	GENERATED_BODY()

	UEnvQueryContext_GasSafePoints();
 
	virtual void ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const override;
};
