// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "EnvQueryContext_NavProjectedQuerier.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UEnvQueryContext_NavProjectedQuerier : public UEnvQueryContext
{
	GENERATED_BODY()

	UEnvQueryContext_NavProjectedQuerier();

	virtual void ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const override;
};
