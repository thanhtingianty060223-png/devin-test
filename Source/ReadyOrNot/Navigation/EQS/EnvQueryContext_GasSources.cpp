// Copyright Void Interactive, 2023


#include "EnvQueryContext_GasSources.h"

#include "EnvironmentQuery/Items/EnvQueryItemType_Actor.h"
#include "Info/CSGasManager.h"

UEnvQueryContext_GasSources::UEnvQueryContext_GasSources()
{
}

void UEnvQueryContext_GasSources::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
	UCSGasManager* GasManager = UCSGasManager::Get(GetWorld());
	if (!IsValid(GasManager))
	{
		return;
	}
	
	TArray<AActor*> GasSources;
	GasManager->GetGasSources(GasSources);
	if (!GasSources.Num())
	{
		return;
	}
	
	UEnvQueryItemType_Actor::SetContextHelper(ContextData, GasSources);
}
