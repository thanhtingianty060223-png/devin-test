// Copyright Void Interactive, 2023


#include "EnvQueryContext_GasSafePoints.h"

#include "EnvironmentQuery/Items/EnvQueryItemType_Point.h"
#include "Info/CSGasManager.h"

UEnvQueryContext_GasSafePoints::UEnvQueryContext_GasSafePoints()
{
}

void UEnvQueryContext_GasSafePoints::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
	UCSGasManager* GasManager = UCSGasManager::Get(GetWorld());
	if (!IsValid(GasManager))
	{
		return;
	}
	
	TArray<FVector> SafePoints;
	GasManager->GetGasSafePoints(SafePoints);
	if (!SafePoints.Num())
	{
		return;
	}
	
	UEnvQueryItemType_Point::SetContextHelper(ContextData, SafePoints);
}
