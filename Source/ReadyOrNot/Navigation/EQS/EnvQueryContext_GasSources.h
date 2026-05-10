// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "EnvQueryContext_GasSources.generated.h"

UCLASS()
class UEnvQueryContext_GasSources : public UEnvQueryContext
{
	GENERATED_BODY()

	UEnvQueryContext_GasSources();
 
	virtual void ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const override;
};