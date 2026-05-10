// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Templates/SubclassOf.h"
#include "NavFilters/NavigationQueryFilter.h"
#include "AI/Navigation/NavigationTypes.h"
#include "NavigationSystemTypes.h"
#include "EnvironmentQuery/EnvQueryContext.h"
#include "DataProviders/AIDataProvider.h"
#include "EnvironmentQuery/EnvQueryTest.h"
#include "EnvironmentQuery/Tests/EnvQueryTest_Pathfinding.h"
#include "EnvQueryTest_GasPath.generated.h"

class ANavigationData;
class UNavigationSystemV1;


UCLASS()
class READYORNOT_API UEnvQueryTest_GasPath : public UEnvQueryTest_Pathfinding
{
	GENERATED_UCLASS_BODY()
	
	virtual void RunTest(FEnvQueryInstance& QueryInstance) const override;

	virtual FText GetDescriptionTitle() const override;
	virtual FText GetDescriptionDetails() const override;

	/** Whether to calculate path length to all contexts and set score to shortest distance. If false, score is only 1/0 */
	UPROPERTY(EditDefaultsOnly, Category=Pathfinding)
	FAIDataProviderBoolValue CalculatePathLengthToAllContexts;

	/** Added radius to gas source radius in which to generate safe points for AI to move to*/
	UPROPERTY(EditDefaultsOnly, Category=Generator)
	FAIDataProviderFloatValue SafePointsBufferDistance;

#if WITH_EDITOR
	/** update test properties after changing mode */
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	virtual void PostLoad() override;
};
