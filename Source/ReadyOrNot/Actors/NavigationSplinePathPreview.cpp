// Void Interactive, 2020

#include "Actors/NavigationSplinePathPreview.h"

#include "Components/NavSplinePathRenderingComponent.h"

#include "NavigationSystem.h"
#include "Navigation/NavMeshSplinePath.h"

TArray<FNavPathPoint> FilterPathPoints2(const TArray<FNavPathPoint>& InPathPoints, const float DistanceThreshold)
{
	TArray<FNavPathPoint> FilteredPathPoints;
		
	// Filter out small path points
	if (InPathPoints.Num() > 2)
	{
		for (int32 k = 1; k < InPathPoints.Num(); k+=2)
		{
			if (InPathPoints.IsValidIndex(k+1))
			{
				if (FVector::Distance(InPathPoints[k-1].Location, InPathPoints[k].Location) < FMath::Abs(DistanceThreshold) ||
					FVector::Distance(InPathPoints[k].Location, InPathPoints[k+1].Location) < FMath::Abs(DistanceThreshold))
				{
					FilteredPathPoints.AddUnique(InPathPoints[k-1]);
					FilteredPathPoints.AddUnique(InPathPoints[k+1]);
					continue;
				}
					
				FilteredPathPoints.AddUnique(InPathPoints[k-1]);
				FilteredPathPoints.AddUnique(InPathPoints[k]);
				FilteredPathPoints.AddUnique(InPathPoints[k+1]);
			}
			else
			{
				FilteredPathPoints.AddUnique(InPathPoints[k]);
			}
		}
	}
	else
	{
		FilteredPathPoints = InPathPoints;
	}

	return FilteredPathPoints;
}

ANavigationSplinePathPreview::ANavigationSplinePathPreview()
{
	PrimaryActorTick.bCanEverTick = true;

	bIsEditorOnlyActor = true;

#if WITH_EDITORONLY_DATA
	// ##UE5UPGRADE## Previously used Loft_Spline texture from UE¤ that has been removed from UE5	
	static ConstructorHelpers::FObjectFinder<UTexture2D> Billboard_Icon(TEXT("Texture2D'/Engine/EditorResources/AI/S_NavLink.S_NavLink'"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> StartBillboard_Icon(TEXT("Texture2D'/Engine/EditorResources/Ai_Spawnpoint.Ai_Spawnpoint'"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> EndBillboard_Icon(TEXT("Texture2D'/Engine/EditorResources/Goal_Waypoint.Goal_Waypoint'"));
	
	BillboardComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard Component"));
	SetRootComponent(BillboardComponent);
	BillboardComponent->SetMobility(EComponentMobility::Static);
	BillboardComponent->SetCanEverAffectNavigation(false);
	BillboardComponent->SetWorldScale3D(FVector(4.0f));
	BillboardComponent->bIsScreenSizeScaled = true;
	BillboardComponent->SetSprite(Billboard_Icon.Object);

	StartBillboardComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("Start Billboard Component"));
	StartBillboardComponent->SetupAttachment(GetRootComponent());
	StartBillboardComponent->SetRelativeLocation(FVector::ZeroVector);
	StartBillboardComponent->SetMobility(EComponentMobility::Static);
	StartBillboardComponent->SetCanEverAffectNavigation(false);
	StartBillboardComponent->bReceiveMobileCSMShadows = false;
	StartBillboardComponent->bIsScreenSizeScaled = true;
	StartBillboardComponent->ScreenSize = 0.0035f;
	StartBillboardComponent->SetSprite(StartBillboard_Icon.Object);
	
	EndBillboardComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("End Billboard Component"));
	EndBillboardComponent->SetupAttachment(GetRootComponent());
	EndBillboardComponent->SetRelativeLocation(FVector::ZeroVector);
	EndBillboardComponent->SetMobility(EComponentMobility::Static);
	EndBillboardComponent->SetCanEverAffectNavigation(false);
	EndBillboardComponent->bReceiveMobileCSMShadows = false;
	EndBillboardComponent->bIsScreenSizeScaled = true;
	EndBillboardComponent->ScreenSize = 0.0035f;
	EndBillboardComponent->SetSprite(EndBillboard_Icon.Object);
	
	DebugRenderingComponent = CreateDefaultSubobject<UNavSplinePathRenderingComponent>(TEXT("Debug Rendering Component"));
	DebugRenderingComponent->SetupAttachment(GetRootComponent());
	DebugRenderingComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
#endif

	bDrawRawPath = true;
	bDrawSmoothedPath = true;
}

void ANavigationSplinePathPreview::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	#if WITH_EDITOR
	if (GetWorld()->WorldType != EWorldType::Editor &&
		GetWorld()->WorldType != EWorldType::EditorPreview)
	{
		return;
	}
	
	if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		FNavLocation StartLocationProjected(StartBillboardComponent->GetComponentLocation());
		FNavLocation EndLocationProjected(EndBillboardComponent->GetComponentLocation());

		NavSys->ProjectPointToNavigation(StartBillboardComponent->GetComponentLocation(), StartLocationProjected);
		NavSys->ProjectPointToNavigation(EndBillboardComponent->GetComponentLocation(), EndLocationProjected);
		
		FPathFindingQuery PathFindingQuery(GetWorld(), *NavSys->MainNavData, StartLocationProjected.Location, EndLocationProjected.Location);
		PathFindingQuery.SetAllowPartialPaths(false);
		//PathFindingQuery.PathInstanceToFill = NavSys->MainNavData->CreatePathInstance<FNavMeshSplinePath>(PathFindingQuery);

		const FPathFindingResult PathFindingResult = NavSys->FindPathSync(PathFindingQuery);
		const FNavPathSharedPtr& Path = PathFindingResult.Path;

		if (!Path.IsValid())
		{
			return;
		}

		if (Path->IsPartial())
		{
			return;
		}

		const TArray<FNavPathPoint>& PathPoints = Path->GetPathPoints();

		if (PathPoints.Num() <= 1)
		{
			return;
		}

		// Draw raw path points
		{
			if (bDrawRawPath)
			{
				for (int32 i = 1; i < PathPoints.Num(); i++)
					DrawDebugLine(GetWorld(), PathPoints[i-1].Location + FVector(0.0f, 0.0f, 30.0f), PathPoints[i].Location + FVector(0.0f, 0.0f, 30.0f), FColor::Red, false, DeltaTime + 0.025f, 0, PathLineThickness);
			}

			if (bDrawRawPathPoints)
			{
				for (int32 i = 0; i < PathPoints.Num(); i++)
					DrawDebugPoint(GetWorld(), PathPoints[i].Location + FVector(0.0f, 0.0f, 30.0f), PathLineThickness * 5.0f, FColor::Yellow, false, DeltaTime + 0.025f, 0);
			}
		}
		
		// Generate spline based version of raw path points
		{
			TArray<FNavPathPoint> SplinePathPoints;

			SplinePathPoints = FilterPathPoints2(PathPoints, PathPointDistanceThreshold);
			
			// Filter out small path points
			/*
			if (PathPoints.Num() > 2)
			{
				for (int32 k = 1; k < PathPoints.Num(); k+=2)
				{
					if (PathPoints.IsValidIndex(k+1))
					{
						if (FVector::Distance(PathPoints[k-1].Location, PathPoints[k].Location) < FMath::Abs(PathPointDistanceThreshold) ||
							FVector::Distance(PathPoints[k].Location, PathPoints[k+1].Location) < FMath::Abs(PathPointDistanceThreshold))
						{
							SplinePathPoints.AddUnique(PathPoints[k-1]);
							SplinePathPoints.AddUnique(PathPoints[k+1]);
							continue;
						}
						
						SplinePathPoints.AddUnique(PathPoints[k-1]);
						SplinePathPoints.AddUnique(PathPoints[k]);
						SplinePathPoints.AddUnique(PathPoints[k+1]);
					}
					else
					{
						SplinePathPoints.AddUnique(PathPoints[k]);
					}
				}

				//FilterPathPoints(PathPoints, SplinePathPoints, PathPointDistanceThreshold);
			}
			else
			{
				SplinePathPoints = PathPoints;
			}
			*/

			if (SplinePathPoints.Num() > 0)
			{
				FVector OldKeyPos = SplinePathPoints[0];

				SplineCurves = {};
				constexpr EInterpCurveMode CurveMode = CIM_CurveAutoClamped;
				for (int32 i = 0; i < SplinePathPoints.Num(); i++)
				{
					SplineCurves.Position.AddPoint((float)i, SplinePathPoints[i].Location);
					SplineCurves.Position.Points[i].InterpMode = CurveMode;
					SplineCurves.Rotation.AddPoint((float)i, FQuat::Identity);
					SplineCurves.Rotation.Points[i].InterpMode = CurveMode;
					SplineCurves.Scale.AddPoint((float)i, FVector::OneVector);
					SplineCurves.Scale.Points[i].InterpMode = CurveMode;
				}
				
				SplineCurves.UpdateSpline(false, false);

				const auto GetCurveLocation_World = [&](float InKey)
				{
					FVector Location = SplineCurves.Position.Eval(InKey);

					Location = NavSys->MainNavData->GetActorTransform().TransformPosition(Location);

					return Location;
				};

				// Draw spline path
				{
	                for (int32 j = 0; j < SplinePathPoints.Num(); j++)
	                {
	                    FVector NewKeyPos = SplinePathPoints[j];

						if (bDrawSmoothedPathPoints)
						{
							DrawDebugPoint(GetWorld(), NewKeyPos, 20.0f, FColor::Purple, false, DeltaTime + 0.025f, 0);
						}
	    
	                    if (j > 0)
	                    {
	                        // Find position on first keyframe.
	                        FVector PreviousKeyPos = OldKeyPos;
	    
	                        // Then draw a line for each substep.
	                        for (int32 StepIdx = 1; StepIdx <= PathPointSubStep; StepIdx++)
	                        {
	                            const float Alpha = (float)StepIdx / (float)PathPointSubStep;
	                            const float Key = (j-1) + Alpha;
	                            const FVector NewPos = GetCurveLocation_World(Key);

                        		if (bDrawSmoothedPath)
									DrawDebugLine(GetWorld(), PreviousKeyPos, NewPos, FColor::Green, false, DeltaTime + 0.025f, 0, PathLineThickness);

								if (bDrawSmoothedPathPointsDetail)
                        			DrawDebugPoint(GetWorld(), NewPos, 5.0f, FColor::White, false, DeltaTime + 0.025f, 0);

	                            PreviousKeyPos = NewPos;
	                        }
						}
	    
						OldKeyPos = NewKeyPos;
					}
				}
			}
		}
		
		if (!HasAnyFlags(RF_ClassDefaultObject))
		{
			DebugRenderingComponent->MarkRenderStateDirty();
		}
	}
	#endif
}

bool ANavigationSplinePathPreview::ShouldTickIfViewportsOnly() const
{
	return true;
}

/*
void ANavigationSplinePathPreview::FilterPathPoints(const TArray<FNavPathPoint>& InPathPoints, TArray<FNavPathPoint>& OutFilteredPathPoints, const float MinDistanceBetweenPoints)
{
	uint16 BaseIndex = 0;
	uint16 CurrentIndex = 1;
	uint16 Skipped = 0;
	
	OutFilteredPathPoints.Empty(InPathPoints.Num());
	OutFilteredPathPoints.AddUnique(InPathPoints[0]);
	
	while (CurrentIndex < InPathPoints.Num())
	{
		if (InPathPoints.IsValidIndex(CurrentIndex))
		{
			if (FVector::Distance(InPathPoints[BaseIndex].Location, InPathPoints[CurrentIndex].Location) > FMath::Abs(MinDistanceBetweenPoints))
			{
				OutFilteredPathPoints.AddUnique(InPathPoints[CurrentIndex-(Skipped+1)]);
				BaseIndex = CurrentIndex;
				Skipped = 0;
			}
			else
			{
				Skipped++;
			}
		}
		
		CurrentIndex++;
	}
	
	OutFilteredPathPoints.AddUnique(InPathPoints[InPathPoints.Num()-1]);
}
*/