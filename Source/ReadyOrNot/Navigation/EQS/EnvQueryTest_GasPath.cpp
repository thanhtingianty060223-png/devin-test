// Copyright Void Interactive, 2023


#include "EnvQueryTest_GasPath.h"

#include "EnvQueryGenerator_GasPoints.h"
#include "AI/Navigation/NavAgentInterface.h"
#include "Engine/World.h"
#include "NavigationData.h"
#include "NavigationSystem.h"
#include "Actors/BaseGasGrenade.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_VectorBase.h"
#include "EnvironmentQuery/Contexts/EnvQueryContext_Querier.h"

#define LOCTEXT_NAMESPACE "EnvQueryGenerator"

UEnvQueryTest_GasPath::UEnvQueryTest_GasPath(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	Context = UEnvQueryContext_Querier::StaticClass();
	Cost = EEnvTestCost::High;
	ValidItemType = UEnvQueryItemType_VectorBase::StaticClass();
	TestMode = EEnvTestPathfinding::PathExist;
	PathFromContext.DefaultValue = true;
	SkipUnreachable.DefaultValue = true;
	FloatValueMin.DefaultValue = 1000.0f;
	FloatValueMax.DefaultValue = 1000.0f;
	CalculatePathLengthToAllContexts.DefaultValue = false;
	SafePointsBufferDistance.DefaultValue = 300.f;

	SetWorkOnFloatValues(TestMode != EEnvTestPathfinding::PathExist);
}

void UEnvQueryTest_GasPath::RunTest(FEnvQueryInstance& QueryInstance) const
{
	UObject* QueryOwner = QueryInstance.Owner.Get();
	BoolValue.BindData(QueryOwner, QueryInstance.QueryID);
	PathFromContext.BindData(QueryOwner, QueryInstance.QueryID);
	SkipUnreachable.BindData(QueryOwner, QueryInstance.QueryID);
	FloatValueMin.BindData(QueryOwner, QueryInstance.QueryID);
	FloatValueMax.BindData(QueryOwner, QueryInstance.QueryID);
	CalculatePathLengthToAllContexts.BindData(QueryOwner, QueryInstance.QueryID);
	SafePointsBufferDistance.BindData(QueryOwner, QueryInstance.QueryID);

	bool bWantsPath = BoolValue.GetValue();
	bool bPathToItem = PathFromContext.GetValue();
	bool bDiscardFailed = SkipUnreachable.GetValue();
	float MinThresholdValue = FloatValueMin.GetValue();
	float MaxThresholdValue = FloatValueMax.GetValue();
	bool bCalculatePathLengthToAllContexts = CalculatePathLengthToAllContexts.GetValue();
	float SafePointsBufferDistanceValue = SafePointsBufferDistance.GetValue();

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(QueryInstance.World);
	if (NavSys == nullptr || QueryOwner == nullptr)
	{
		return;
	}

	ANavigationData* NavData = FindNavigationData(*NavSys, QueryOwner);
	if (NavData == nullptr)
	{
		return;
	}

	TArray<AActor*> ContextActors;
	if (!QueryInstance.PrepareContext(Context, ContextActors))
	{
		return;
	}

	EPathFindingMode::Type PFMode(EPathFindingMode::Regular);
	FSharedConstNavQueryFilter NavFilter = UNavigationQueryFilter::GetQueryFilter(*NavData, QueryOwner, FilterClass);

	TArray<FVector> ContextActorsProjectedLocations;
	ContextActorsProjectedLocations.Reserve(ContextActors.Num());
	for (AActor* ContextActor : ContextActors)
	{
		FNavLocation ProjLoc;
		NavSys->ProjectPointToNavigation(ContextActor->GetActorLocation(), ProjLoc, FVector(1000, 1000, 500), 0, NavFilter);
		ContextActorsProjectedLocations.Emplace(ProjLoc.Location);
	}

	if (GetWorkOnFloatValues())
	{
		FFindPathSignature FindPathFunc;
		FindPathFunc.BindUObject(this, TestMode == EEnvTestPathfinding::PathLength ?
			(bPathToItem ? &UEnvQueryTest_GasPath::FindPathLengthTo : &UEnvQueryTest_GasPath::FindPathLengthFrom) :
			(bPathToItem ? &UEnvQueryTest_GasPath::FindPathCostTo : &UEnvQueryTest_GasPath::FindPathCostFrom) );

		// Initialize a map of whether given point has already been marked as a gas point
		// If point has already been marked gas by previous context, no need to recalc path length
		TMap<int32, bool> PointMarkedValidGas;
		PointMarkedValidGas.Reserve(QueryInstance.Items.Num());
		for (int32 i = 0; i < QueryInstance.Items.Num(); i++)
		{
			PointMarkedValidGas.Emplace(i, false);
		}
		
		NavData->BeginBatchQuery();
		for (FEnvQueryInstance::ItemIterator It(this, QueryInstance); It; ++It)
		{
			const FVector ItemLocation = GetItemLocation(QueryInstance, It.GetIndex());
			for (int32 ContextIndex = 0; ContextIndex < ContextActors.Num(); ContextIndex++)
			{
				// If we aren't a gas source, really shouldn't be here
				if (!ContextActors[ContextIndex]->Implements<UGasSource>())
				{
					continue;
				}
				
				if (!bCalculatePathLengthToAllContexts && PointMarkedValidGas[It.GetIndex()])
				{
					continue;
				}
				FVector	ContextActorLocation;
				IGasSource::Execute_GetGasReleaseLocation(ContextActors[ContextIndex], ContextActorLocation);
				
				// Don't need to check path length or cost if point is too far away from this context actor anyway
				float ContextGasRadius = IGasSource::Execute_GetGasRadius(ContextActors[ContextIndex]);
				float BoundsCheckRadius = ContextGasRadius + SafePointsBufferDistanceValue;
				float DistanceBetweenLocations = FVector::Distance(ItemLocation, ContextActorLocation);
				if (DistanceBetweenLocations > BoundsCheckRadius)
				{
					continue;
				}
				
				const float PathValue = FindPathFunc.Execute(ItemLocation, ContextActorLocation, PFMode, *NavData, *NavSys, NavFilter, QueryOwner);
				// Reduce the radius here slightly for what is considered gas. Would rather have less gas than no safe points
				bool bPointIsGas = PathValue <= (ContextGasRadius - 100);
				
				if (bCalculatePathLengthToAllContexts)
				{
					It.SetScore(TestPurpose, FilterType, PathValue, MinThresholdValue, MaxThresholdValue);			
				}
				else
				{
					It.SetScore(TestPurpose, FilterType, bPointIsGas, MinThresholdValue, MaxThresholdValue);	
				}
				
				PointMarkedValidGas[It.GetIndex()] = bPointIsGas;

				if (bDiscardFailed && PathValue >= BIG_NUMBER)
				{
					It.ForceItemState(EEnvItemStatus::Failed);
				}
			}
		}
		NavData->FinishBatchQuery();
	}
	else
	{
		NavData->BeginBatchQuery();
		if (bPathToItem)
		{
			for (FEnvQueryInstance::ItemIterator It(this, QueryInstance); It; ++It)
			{
				const FVector ItemLocation = GetItemLocation(QueryInstance, It.GetIndex());
				for (int32 ContextIndex = 0; ContextIndex < ContextActors.Num(); ContextIndex++)
				{
					FVector ContextActorLocation = ContextActors[ContextIndex]->GetActorLocation();
					const bool bFoundPath = TestPathTo(ItemLocation, ContextActorLocation, PFMode, *NavData, *NavSys, NavFilter, QueryOwner);
					It.SetScore(TestPurpose, FilterType, bFoundPath, bWantsPath);
				}
			}
		}
		else
		{
			for (FEnvQueryInstance::ItemIterator It(this, QueryInstance); It; ++It)
			{
				const FVector ItemLocation = GetItemLocation(QueryInstance, It.GetIndex());
				for (int32 ContextIndex = 0; ContextIndex < ContextActors.Num(); ContextIndex++)
				{
					FVector ContextActorLocation = ContextActors[ContextIndex]->GetActorLocation();
					const bool bFoundPath = TestPathFrom(ItemLocation, ContextActorLocation, PFMode, *NavData, *NavSys, NavFilter, QueryOwner);
					It.SetScore(TestPurpose, FilterType, bFoundPath, bWantsPath);
				}
			}
		}
		NavData->FinishBatchQuery();
	}
}

FText UEnvQueryTest_GasPath::GetDescriptionTitle() const
{
	FString ModeDesc[] = { TEXT("PathExist"), TEXT("PathCost"), TEXT("PathLength") };

	FString DirectionDesc = PathFromContext.IsDynamic() ?
		FString::Printf(TEXT("%s, direction: %s"), *UEnvQueryTypes::DescribeContext(Context).ToString(), *PathFromContext.ToString()) :
		FString::Printf(TEXT("%s %s"), PathFromContext.DefaultValue ? TEXT("from") : TEXT("to"), *UEnvQueryTypes::DescribeContext(Context).ToString());

	return FText::FromString(FString::Printf(TEXT("%s: %s"), *ModeDesc[TestMode], *DirectionDesc));
}

FText UEnvQueryTest_GasPath::GetDescriptionDetails() const
{
	FText DiscardDesc = LOCTEXT("DiscardUnreachable", "discard unreachable");
	FText Desc2;
	if (SkipUnreachable.IsDynamic())
	{
		Desc2 = FText::Format(FText::FromString("{0}: {1}"), DiscardDesc, FText::FromString(SkipUnreachable.ToString()));
	}
	else if (SkipUnreachable.DefaultValue)
	{
		Desc2 = DiscardDesc;
	}

	FText TestParamDesc = GetWorkOnFloatValues() ? DescribeFloatTestParams() : DescribeBoolTestParams("existing path");
	if (!Desc2.IsEmpty())
	{
		return FText::Format(FText::FromString("{0}\n{1}"), Desc2, TestParamDesc);
	}

	return TestParamDesc;
}

#if WITH_EDITOR
void UEnvQueryTest_GasPath::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UEnvQueryTest_Pathfinding,TestMode))
	{
		SetWorkOnFloatValues(TestMode != EEnvTestPathfinding::PathExist);
	}
}
#endif

void UEnvQueryTest_GasPath::PostLoad()
{
	Super::PostLoad();
	
	SetWorkOnFloatValues(TestMode != EEnvTestPathfinding::PathExist);
}

#undef LOCTEXT_NAMESPACE
