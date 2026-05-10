// Copyright Void Interactive, 2023


#include "EnvQueryContext_NavProjectedQuerier.h"

#include "NavigationData.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Point.h"

UEnvQueryContext_NavProjectedQuerier::UEnvQueryContext_NavProjectedQuerier()
{
}

void UEnvQueryContext_NavProjectedQuerier::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
	AActor* QueryOwner = Cast<AActor>(QueryInstance.Owner.Get());
	const ANavigationData* NavData = FEQSHelpers::FindNavigationDataForQuery(QueryInstance);
	if (!NavData)
	{
		return;
	}
	
	FNavLocation ProjectedLocation;
	bool Result = NavData->ProjectPoint(QueryOwner->GetActorLocation(), ProjectedLocation, FVector(50, 50, 150));
	if (!Result)
	{
		return;
	}
		
	UEnvQueryItemType_Point::SetContextHelper(ContextData, ProjectedLocation.Location);
}