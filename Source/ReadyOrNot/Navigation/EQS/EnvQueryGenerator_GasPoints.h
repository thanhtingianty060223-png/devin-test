// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "DataProviders/AIDataProvider.h"
#include "EnvironmentQuery/EnvQueryContext.h"
#include "EnvironmentQuery/Generators/EnvQueryGenerator_ProjectedPoints.h"
#include "EnvQueryGenerator_GasPoints.generated.h"

/**
 * 
 */

struct FGasLocation : public FNavLocation
{
	/** ID (Index) of the context querier that this point was generated around*/
	uint32 QuerierId;

	FGasLocation() : QuerierId(0) {}
	FGasLocation(const FVector& InLocation, NavNodeRef InNodeRef = INVALID_NAVNODEREF, uint32 InQuerierId = 0)
		: FNavLocation(InLocation, InNodeRef), QuerierId(InQuerierId) {}
};

UCLASS(meta = (DisplayName = "Points: Gas"))
class READYORNOT_API UEnvQueryGenerator_GasPoints : public UEnvQueryGenerator_ProjectedPoints
{
	GENERATED_BODY()
	
	UEnvQueryGenerator_GasPoints(const FObjectInitializer& ObjectInitializer);

	/** Radius of circle to generate the grid in */
	UPROPERTY(EditDefaultsOnly, Category=Generator, meta=(DisplayName="GridRadius"))
	FAIDataProviderFloatValue GridRadius;

	/** generation density */
	UPROPERTY(EditDefaultsOnly, Category=Generator)
	FAIDataProviderFloatValue SpaceBetween;

	/** context */
	UPROPERTY(EditDefaultsOnly, Category=Generator)
	TSubclassOf<UEnvQueryContext> GenerateAround;

	/** navigation filter to use in pathfinding */
	UPROPERTY(EditDefaultsOnly, Category = Generator)
	TSubclassOf<UNavigationQueryFilter> NavigationFilter;

	/** Added radius to gas source radius in which to generate safe points for AI to move to*/
	UPROPERTY(EditDefaultsOnly, Category=Generator)
	FAIDataProviderFloatValue SafePointsBufferDistance;

	virtual void GenerateItems(FEnvQueryInstance& QueryInstance) const override;

	virtual void ProjectAndFilterNavPoints(TArray<FNavLocation>& Points, FEnvQueryInstance& QueryInstance) const override;

	virtual FText GetDescriptionTitle() const override;
	virtual FText GetDescriptionDetails() const override;
};
