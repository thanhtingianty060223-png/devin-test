// Copyright Void Interactive, 2023


#include "EnvQueryGenerator_GasPoints.h"
#include "Actors/BaseGasGrenade.h"
#include "EnvironmentQuery/Contexts/EnvQueryContext_Querier.h"
#include "Interfaces/GasSource.h"
#include "NavMesh/RecastNavMesh.h"

#define LOCTEXT_NAMESPACE "EnvQueryGenerator"

#if WITH_RECAST
namespace PathGridHelpers
{
	static bool HasPath(const FRecastDebugPathfindingData& NodePool, const NavNodeRef& NodeRef)
	{
		FRecastDebugPathfindingNode SearchKey(NodeRef);
		const FRecastDebugPathfindingNode* MyNode = NodePool.Nodes.Find(SearchKey);
		return MyNode != nullptr;
	}
}
#endif

UEnvQueryGenerator_GasPoints::UEnvQueryGenerator_GasPoints(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	GenerateAround = UEnvQueryContext_Querier::StaticClass();
	GridRadius.DefaultValue = 800.0f;
	SpaceBetween.DefaultValue = 150.0f;
	SafePointsBufferDistance.DefaultValue = 300.f;
	ProjectionData.SetNavmeshOnly();
}

void UEnvQueryGenerator_GasPoints::GenerateItems(FEnvQueryInstance& QueryInstance) const
{
	UObject* BindOwner = QueryInstance.Owner.Get();
	GridRadius.BindData(BindOwner, QueryInstance.QueryID);
	SpaceBetween.BindData(BindOwner, QueryInstance.QueryID);
	SafePointsBufferDistance.BindData(BindOwner, QueryInstance.QueryID);

	float RadiusValue = GridRadius.GetValue();
	float DensityValue = SpaceBetween.GetValue();
	float SafePointsBufferDistanceValue = SafePointsBufferDistance.GetValue();

	TArray<FVector> ContextLocations;
	TArray<AActor*> ContextActors;
	QueryInstance.PrepareContext(GenerateAround, ContextActors);

	TArray<FNavLocation> GridPoints;
	//GridPoints.Reserve(ItemCount * ItemCount * ContextLocations.Num());
	//GridPoints.Reserve(ItemCount * ItemCount);

	for (int32 ContextIndex = 0; ContextIndex < ContextActors.Num(); ContextIndex++)
	{
		if (!ContextActors[ContextIndex]->Implements<UGasSource>())
		{
			continue;
		}

		FVector ActorLocation;
		IGasSource::Execute_GetGasReleaseLocation(ContextActors[ContextIndex], ActorLocation);
		float Radius = IGasSource::Execute_GetGasRadius(ContextActors[ContextIndex]) + SafePointsBufferDistanceValue;

		const int32 ItemCount = FPlatformMath::TruncToInt((Radius * 2.0f / DensityValue) + 1);
		const int32 ItemCountHalf = ItemCount / 2;
		
		for (int32 IndexX = 0; IndexX < ItemCount; ++IndexX)
		{
			for (int32 IndexY = 0; IndexY < ItemCount; ++IndexY)
			{
				const FNavLocation TestPoint = FNavLocation(ActorLocation - FVector(DensityValue * (IndexX - ItemCountHalf), DensityValue * (IndexY - ItemCountHalf), 0));
				float DistanceToPointFromCurrentContext = FVector::Distance(ActorLocation, TestPoint.Location); 
				bool bIgnorePoint = false;
				for (int32 PreviousContextsIndex = 0; PreviousContextsIndex < ContextIndex; PreviousContextsIndex++)
				{
					if (!ContextActors[PreviousContextsIndex]->Implements<UGasSource>())
					{
						continue;
					}
					float PreviousActorRadius = IGasSource::Execute_GetGasRadius(ContextActors[PreviousContextsIndex]);
					if (FVector::Distance(TestPoint.Location, ContextActors[PreviousContextsIndex]->GetActorLocation()) < (PreviousActorRadius + SafePointsBufferDistanceValue))
					{
						bIgnorePoint = true;
					}
				}
				
				if (FVector::Distance(ActorLocation, TestPoint.Location) < Radius && !bIgnorePoint)
				{
					GridPoints.Add(TestPoint);
				}
			}
		}

		// ProjectAndFilterNavPoints(GridPoints, QueryInstance);
		// TArray<FGasLocation> GasPoints;
		// GasPoints.Reserve(GridPoints.Num());
		// for (FNavLocation Point : GridPoints)
		// {
		// 	FGasLocation GasPoint = FGasLocation(Point.Location, Point.NodeRef, ContextIndex);
		// 	GasPoints.Emplace(GasPoint);
		// }
		// StoreGasNavPoints(GasPoints, QueryInstance);
	}

	ProjectAndFilterNavPoints(GridPoints, QueryInstance);
	StoreNavPoints(GridPoints, QueryInstance);
}


void UEnvQueryGenerator_GasPoints::ProjectAndFilterNavPoints(TArray<FNavLocation>& Points, FEnvQueryInstance& QueryInstance) const
{
	Super::ProjectAndFilterNavPoints(Points, QueryInstance);
	
	UObject* BindOwner = QueryInstance.Owner.Get();
	SafePointsBufferDistance.BindData(BindOwner, QueryInstance.QueryID);
	float SafePointsBufferDistanceValue = SafePointsBufferDistance.GetValue();

#if WITH_RECAST
	UObject* DataOwner = QueryInstance.Owner.Get();

	const ANavigationData* QueryNavData = FEQSHelpers::FindNavigationDataForQuery(QueryInstance);
	const ARecastNavMesh* NavMeshData = Cast<const ARecastNavMesh>(QueryNavData);
	if (NavMeshData == nullptr || DataOwner == nullptr)
	{
		return;
	}
	
	TArray<AActor*> ContextActors;
	QueryInstance.PrepareContext(GenerateAround, ContextActors);

	FSharedConstNavQueryFilter NavQueryFilter = NavigationFilter ? UNavigationQueryFilter::GetQueryFilter(*NavMeshData, DataOwner, NavigationFilter) : NavMeshData->GetDefaultQueryFilter();
	if (!NavQueryFilter.IsValid())
	{
		UE_LOG(LogEQS, Error, TEXT("%s (%d:%s) can't obtain navigation filter! NavData:%s FilterClass:%s"),
			*QueryInstance.QueryName, QueryInstance.OptionIndex, *OptionName,
			*GetNameSafe(NavMeshData), *GetNameSafe(NavigationFilter.Get()));
		return;
	}

	FSharedNavQueryFilter NavigationFilterCopy = NavQueryFilter->GetCopy();
	NavigationFilterCopy->SetMaxSearchNodes(4096);
	if (NavigationFilterCopy.IsValid())
	{
		TArray<NavNodeRef> Polys;
		TArray<FNavLocation> HitLocations;
		const FVector ProjectionExtent(ProjectionData.ExtentX, ProjectionData.ExtentX, (ProjectionData.ProjectDown + ProjectionData.ProjectUp) / 2);

		// Set up a map that specifies whether a point at index has a valid path to ANY context, not necessarily ALL contexts
		TMap<int32, bool> PointContextsPathValid;
		PointContextsPathValid.Reserve(Points.Num());
		for (int i = 0; i < Points.Num(); i++)
		{
			PointContextsPathValid.Emplace(i, false);
		}
		
		for (int32 ContextIdx = 0; ContextIdx < ContextActors.Num() && Points.Num(); ContextIdx++)
		{
			if (!ContextActors[ContextIdx]->Implements<UGasSource>())
			{
				continue;
			}
			
			FVector ContextActorLocation;
			IGasSource::Execute_GetGasReleaseLocation(ContextActors[ContextIdx], ContextActorLocation);
			float Radius = IGasSource::Execute_GetGasRadius(ContextActors[ContextIdx]) + SafePointsBufferDistanceValue;
			
			
			float CollectDistanceSq = 0.0f;
			for (int32 Idx = 0; Idx < Points.Num(); Idx++)
			{
				const float TestDistanceSq = FVector::DistSquared(Points[Idx].Location, ContextActorLocation);
				CollectDistanceSq = FMath::Max(CollectDistanceSq, TestDistanceSq);
			}

			const float MaxPathDistance = FMath::Min(FMath::Sqrt(CollectDistanceSq), Radius);

			Polys.Reset();

			FRecastDebugPathfindingData NodePoolData;
			NodePoolData.Flags = ERecastDebugPathfindingFlags::Basic;

			NavMeshData->GetPolysWithinPathingDistance(ContextActorLocation, MaxPathDistance, Polys, NavigationFilterCopy, nullptr, &NodePoolData);

			for (int32 Idx = Points.Num() - 1; Idx >= 0; Idx--)
			{
				if (PointContextsPathValid[Idx])
					continue;

				bool bHasPath = PathGridHelpers::HasPath(NodePoolData, Points[Idx].NodeRef);

				/*
				float PathLength = 100000;
				if (FVector::Distance(ContextActorLocation, Points[Idx]) < (radius * 2))
				{
					NavMeshData->CalcPathLength(ContextActorLocation, Points[Idx], PathLength, NavigationFilterCopy);
				}
				bool bHasPath = PathGridHelpers::HasPath(NodePoolData, Points[Idx].NodeRef);
				*/
				// if (!bHasPath && Points[Idx].NodeRef != INVALID_NAVNODEREF)
				// {
				// 	// try projecting it again, maybe it will match valid poly on different height
				// 	HitLocations.Reset();
				// 	FVector TestPt(Points[Idx].Location.X, Points[Idx].Location.Y, ContextLocations[ContextIdx].Z);
				//
				// 	NavMeshData->ProjectPointMulti(TestPt, HitLocations, ProjectionExtent, TestPt.Z - ProjectionData.ProjectDown, TestPt.Z + ProjectionData.ProjectUp, NavigationFilterCopy, nullptr);
				// 	for (int32 HitIdx = 0; HitIdx < HitLocations.Num(); HitIdx++)
				// 	{
				// 		const bool bHasPathTest = PathGridHelpers::HasPath(NodePoolData, HitLocations[HitIdx].NodeRef);
				// 		if (bHasPathTest)
				// 		{
				// 			Points[Idx] = HitLocations[HitIdx];
				// 			Points[Idx].Location.Z += ProjectionData.PostProjectionVerticalOffset;
				// 			bHasPath = true;
				// 			break;
				// 		}
				// 	}
				// }

				//if (PathLength < 1500)
				/*
				if (bHasPath && PathLength < (radius * 2))
				{
					PointContextsPathValid[Idx] = true;
				}
				*/
				if (bHasPath)
				{
					PointContextsPathValid[Idx] = true;
				}

			}
		}

		for (int i = Points.Num() - 1; i >= 0; i--)
		{
			if (!PointContextsPathValid[i])
			{
				Points.RemoveAt(i);
			}
		}
	}
#endif // WITH_RECAST
}

FText UEnvQueryGenerator_GasPoints::GetDescriptionTitle() const
{
	return FText::Format(LOCTEXT("GasPointsdDescriptionGenerateAroundContext", "{0}: generate around {1}"),
		Super::GetDescriptionTitle(), UEnvQueryTypes::DescribeContext(GenerateAround));
};

FText UEnvQueryGenerator_GasPoints::GetDescriptionDetails() const
{
	FText Desc = FText::Format(LOCTEXT("GasPointsDescription", "radius: {0}, space between: {1}"),
		FText::FromString(GridRadius.ToString()), FText::FromString(SpaceBetween.ToString()));

	FText ProjDesc = ProjectionData.ToText(FEnvTraceData::Brief);
	if (!ProjDesc.IsEmpty())
	{
		FFormatNamedArguments ProjArgs;
		ProjArgs.Add(TEXT("Description"), Desc);
		ProjArgs.Add(TEXT("ProjectionDescription"), ProjDesc);
		Desc = FText::Format(LOCTEXT("GasPointsDescriptionWithProjection", "{Description}, {ProjectionDescription}"), ProjArgs);
	}

	return Desc;
}

#undef LOCTEXT_NAMESPACE