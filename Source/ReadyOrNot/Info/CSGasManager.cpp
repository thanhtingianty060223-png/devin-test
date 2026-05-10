// Copyright Void Interactive, 2023


#include "CSGasManager.h"
#include "NavigationSystem.h"
#include "NavModifierVolume.h"
#include "Actors/CoverLandmark.h"
#include "Actors/CoverLandmarkProxy.h"
#include "Actors/Environment/PepperGasCloud.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "Navigation/ReadyOrNotNavAreas.h"
#include "Navigation/ReadyOrNotNavMeshGenerator.h"
#include "Navigation/ReadyOrNotNavQueries.h"
#include "NavMesh/RecastNavMesh.h"
#include "NavMesh/RecastNavMeshGenerator.h"

DECLARE_CYCLE_STAT(TEXT("GenerateNavVolumes"), STAT_GenerateNavVolumes, STATGROUP_CSGAS)

TAutoConsoleVariable<int32> CVarRonDrawDebugGasPoints(TEXT("DrawDebugGasPoints"), 0, TEXT("Draw Debug Gas Points"));

UCSGasManager::UCSGasManager()
{
	GasGenerationQuery = nullptr;
	GasData = GetDefault<UReadyOrNotCSGasSettings>()->GasDataAsset.LoadSynchronous();
}

bool UCSGasManager::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer))
	{
		return false;
	}

	// Only want for the server
	//##UE5.3UPGRADE##
	//return Outer->GetWorld() && Outer->GetWorld()->IsServer();
	return Outer->GetWorld() && !Outer->GetWorld()->IsNetMode(ENetMode::NM_Client);
	//##UE5.3UPGRADE##
}

UCSGasManager* UCSGasManager::Get(UWorld* World)
{
	// ##UE5UPGRADE## Deprecated
	if (!World->IsNetMode(NM_ListenServer))
		return nullptr;
	
	return World->GetSubsystem<UCSGasManager>();
}

void UCSGasManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	ElapsedTimeSinceTick += DeltaSeconds;
	if (ElapsedTimeSinceTick < TickFrequency)
	{
		return;
	}
	
	AReadyOrNotGameState* GS = GetWorld()->GetGameState<AReadyOrNotGameState>();
	if (!GS)
	{
		return;
	}

	ElapsedTimeSinceTick = 0;

	if (!GasSources.Num())
	{
		return;
	}

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());

	for (AReadyOrNotCharacter* Character : GetWorld()->GetGameState<AReadyOrNotGameState>()->AllReadyOrNotCharacters)
	{
		FVector CharacterLocation = Character->GetActorLocation();
		FNavLocation CharacterNavLocation;
		
		if (ACyberneticCharacter* CyberneticCharacter = Cast<ACyberneticCharacter>(Character))
		{
			if (CyberneticCharacter->IsTakingCoverAtLandmark() && IsValid(CyberneticCharacter->CurrentCoverLandmarkInUse) && IsValid(CyberneticCharacter->CurrentCoverLandmarkInUse->EntryPoints[0]))
			{
				CharacterLocation = CyberneticCharacter->CurrentCoverLandmarkInUse->EntryPoints[0]->GetActorLocation();
			}
		}
		
		NavSys->ProjectPointToNavigation(CharacterLocation, CharacterNavLocation, FVector(50, 50, 150));
		FVector CharacterProjectedLocation = CharacterNavLocation.Location;
				
		for (auto GasSource : GasSources)
		{
			if (!GasSource->Implements<UGasSource>())
			{
				continue;
			}
			
			if (ShouldApplyGasDamage(CharacterProjectedLocation, GasSource, NavSys))
			{
				FDamageEvent DmgEvent;
				TSubclassOf<UDamageType> const ValidDamageTypeClass = GasData->DamageType ? GasData->DamageType : (TSubclassOf<UDamageType>) UStunDamage::StaticClass();
				DmgEvent.DamageTypeClass = ValidDamageTypeClass;
				Character->TakeDamage(0.1, DmgEvent, GasSource->GetInstigatorController(), GasSource);
				break;
			}
		}
	}
}

TStatId UCSGasManager::GetStatId() const
{
	return GetStatID();
}

void UCSGasManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UCSGasManager::AddGasSource(AActor* Source)
{
	if (!IsValid(Source))
		return;

	// ##UE5.3UPGRADE##
	//if (!GetWorld()->IsServer() || !Source->HasAuthority())
	if (GetWorld()->IsNetMode(NM_Client) || !Source->HasAuthority())
		return;
	// ##UE5.3UPGRADE##

	if (GasSources.Contains(Source))
		return;
	
	if (!Source->Implements<UGasSource>())
		return;

	// Check that this gas source can be successfully projected to the navmesh somewhere. If not, ignore it
	FVector ProjectedLocation;
	bool Success = IGasSource::Execute_GetGasReleaseLocation(Source, ProjectedLocation);
	if (!Success)
		return;
	
	GasSources.Emplace(Source);
	
	UpdateGas();
}

void UCSGasManager::RemoveGasSource(AActor* Source)
{
	if (!IsValid(Source))
	{
		return;
	}

	int Removed = GasSources.Remove(Source);

	if (Removed)
	{
		UpdateGas();
	}
	
}

void UCSGasManager::UpdateGas()
{
	if (bIsRunningQuery)
	{
		return;
	}
	FEnvQueryRequest GasGenerationRequest = FEnvQueryRequest(GasData->GasGenerationQuery, this);
	FEnvNamedValue SafePointsBufferDistance;
	SafePointsBufferDistance.ParamName = FName(TEXT("SafePointsBufferDistance"));
	SafePointsBufferDistance.ParamType = EAIParamType::Float;
	SafePointsBufferDistance.Value = GasData->SafePointsBufferDistance;
	GasGenerationRequest.SetNamedParam(SafePointsBufferDistance);
	
	GasGenerationRequest.Execute(EEnvQueryRunMode::AllMatching, this, &UCSGasManager::OnGasGenerationQueryFinished);
	bIsRunningQuery = true;
}

void UCSGasManager::OnGasGenerationQueryFinished(TSharedPtr<FEnvQueryResult> Result)
{	
	bIsRunningQuery = false;
	GasPoints.Reset();
	SafePoints.Reset();
	TArray<FVector> GeneratedLocations;
	Result->GetAllAsLocations(GeneratedLocations);

	for (int32 i = 0; i < GeneratedLocations.Num(); i++)
	{
		Result->GetItemScore(i) > 0 ? GasPoints.Emplace(GeneratedLocations[i]) : SafePoints.Emplace(GeneratedLocations[i]);
	}

#if !UE_BUILD_SHIPPING
	if (CVarRonDrawDebugGasPoints.GetValueOnGameThread() != 0)
	{
		for (auto GasPoint : GasPoints)
		{
			DrawDebugPoint(GetWorld(), GasPoint, 50, FColor::Red, false, 5);
		}

		for (auto SafePoint : SafePoints)
		{
			DrawDebugPoint(GetWorld(), SafePoint, 50, FColor::Blue, false, 5);
		}
	}
#endif

	GenerateNavVolumes();

	
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	ANavigationData* NavData = NavSys->GetDefaultNavDataInstance();
	ARecastNavMesh* NavMesh = Cast<ARecastNavMesh>(NavData);

	TArray<int32> AffectedTileIndices;
	float TileSize = NavMesh->TileSizeUU;
	FBox TotalNavBounds = NavSys->GetWorldBounds();

	TArray<FIntPoint> CurrentAffectedNavTiles;

	// Limit nav modifer tile checks
	int step = (GasPoints.Num() / 20) + 1;
	for (int i = 0; i < GasPoints.Num(); i++)
	{
		int32 X, Y;
		NavMesh->GetNavMeshTileXY(GasPoints[i], X, Y);
		TArray<int32> TileIndices;
		NavMesh->GetNavMeshTilesAt(X, Y, TileIndices);
				
		for (auto TileIndex : TileIndices)
		{
			if (!AffectedTileIndices.Contains(TileIndex))
			{
				AffectedTileIndices.Emplace(TileIndex);
				CurrentAffectedNavTiles.AddUnique(FIntPoint(X, Y));
			}
		}
	}
	
	// Work out which tiles were previously affected and now aren't, or weren't and now are. These changed tiles are the
	// ones that needs to be rebuilt
	TArray<FIntPoint> RemovedAffectedTiles = AffectedNavTiles.FilterByPredicate([&CurrentAffectedNavTiles](const FIntPoint& Element){ return !CurrentAffectedNavTiles.Contains(Element);});
	//TArray<FIntPoint> AddedAffectedTiles = CurrentAffectedNavTiles.FilterByPredicate([this](const FIntPoint& Element){ return !AffectedNavTiles.Contains(Element);});

	AffectedNavTiles.Reset();
	AffectedNavTiles = CurrentAffectedNavTiles;

	TArray<FIntPoint> CurrentAndRemovedAffectedTiles = CurrentAffectedNavTiles;
	CurrentAndRemovedAffectedTiles.Append(RemovedAffectedTiles);

	FReadyOrNotNavMeshGenerator* MyGenerator = static_cast<FReadyOrNotNavMeshGenerator*>(NavMesh->GetGenerator());
	MyGenerator->SetSeedDistanceForTiles(CurrentAndRemovedAffectedTiles, 0);
}

void UCSGasManager::GenerateNavVolumes()
{
	SCOPE_CYCLE_COUNTER(STAT_GenerateNavVolumes)
	
	if (!IsValid(NavVolumesContainer))
	{
		NavVolumesContainer = GetWorld()->SpawnActor(AActor::StaticClass());
	}
	
	TSet<UActorComponent*> CurrentVolumeComponents = NavVolumesContainer->GetComponents();
	int32 CompCount = CurrentVolumeComponents.Num();
	int32 ExtraNavVolumesNeeded = (GasPoints.Num() - CompCount) > 0 ? (GasPoints.Num() - CompCount) : 0;

	for (int i = 0; i < ExtraNavVolumesNeeded; i++)
	{
		UBoxComponent* Box = Cast<UBoxComponent>(NavVolumesContainer->AddComponentByClass(UBoxComponent::StaticClass(), false, FTransform::Identity, true));
		Box->AreaClass = UNavArea_CSGas::StaticClass();
		Box->bDynamicObstacle = true;
		Box->InitBoxExtent(GasData->NavModifierExtents);
		Box->SetHiddenInGame(true);		
		Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		NavVolumesContainer->FinishAddComponent(Box, false, FTransform::Identity);
	}
	
	TArray<UBoxComponent*> CurrentVolumeComponentsArray;
	NavVolumesContainer->GetComponents<UBoxComponent>(CurrentVolumeComponentsArray, false);

	for (int32 i = 0; i < GasPoints.Num(); i++)
	{
		CurrentVolumeComponentsArray[i]->Activate();
		CurrentVolumeComponentsArray[i]->SetCanEverAffectNavigation(true);
		CurrentVolumeComponentsArray[i]->SetBoxExtent(GasData->NavModifierExtents);
		CurrentVolumeComponentsArray[i]->SetWorldLocation(GasPoints[i]);
		CurrentVolumeComponentsArray[i]->SetWorldRotation(FQuat::Identity);
	}

	// Remove the unused nav modifiers, but don't delete cause they might be needed later
	// How to remove without deleting? Gonna just set extents to zero and move to origin for now
	for (int32 i = GasPoints.Num(); i < CurrentVolumeComponentsArray.Num(); i++)
	{
		CurrentVolumeComponentsArray[i]->Deactivate();
		CurrentVolumeComponentsArray[i]->SetCanEverAffectNavigation(false);
		CurrentVolumeComponentsArray[i]->SetBoxExtent(FVector::ZeroVector);
		CurrentVolumeComponentsArray[i]->SetWorldLocation(FVector::ZeroVector);
	}

	//SIZE_T size = NavVolumesContainer->GetResourceSizeBytes(EResourceSizeMode::EstimatedTotal);
	//UE_LOG(LogReadyOrNot, Warning, TEXT("Nav Volumes Size: %llu"), size);

#if !UE_BUILD_SHIPPING
	if (CVarRonDrawDebugGasPoints.GetValueOnGameThread() == 0)
		return;

	TInlineComponentArray<UBoxComponent*> Components;
	NavVolumesContainer->GetComponents<UBoxComponent>(Components);
	for (UBoxComponent* Box : Components)
	{
		Box->SetHiddenInGame(false);
	}
#endif
	
}

void UCSGasManager::GenerateSphereNavVolume(FVector Location, float Radius)
{
	
}


 void UCSGasManager::GetGasSources(TArray<AActor*> &OutGasSources)
{
	OutGasSources = GasSources;
}

void UCSGasManager::GetGasSafePoints(TArray<FVector> &OutGasSafePoints)
{
	OutGasSafePoints = SafePoints;
}

bool UCSGasManager::ProjectPepperballToNavigation(APepperProjectile* Projectile, FVector Location, FVector Normal, FVector& OutLocation)
{
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	FNavLocation ProjectedLocation;

	bool Result = NavSys->ProjectPointToNavigation(Location, ProjectedLocation, FVector(50, 50, 500));
	if (Result)
	{
		OutLocation = ProjectedLocation.Location;
		return true;
	}
	
	// Try walk back along the hit normal a bit and reproject. Maybe we've fired off the navmesh slightly, like over a table or something
	FVector NewLocation = Location + (Normal * 300);
	Result = NavSys->ProjectPointToNavigation(NewLocation, ProjectedLocation, FVector(250, 250, 500));
	if (Result)
	{
		OutLocation = ProjectedLocation.Location;
		return true;
	}
	
	return false;
}

void UCSGasManager::AddPepperballLocation(APepperProjectile* Projectile, FVector Location, FVector Normal)
{
	if (!Projectile->HasAuthority())
		return;
	
	//* How close a new pepperball needs to be to an existing potential pepper gas cloud to add to its value
	float PepperballLocationTolerance = 200;
	float PepperballLcoationToToleranceSquared = FMath::Square(PepperballLocationTolerance);

	FVector ProjectedPepperballLocation;
	bool Result = ProjectPepperballToNavigation(Projectile, Location, Normal, ProjectedPepperballLocation);
	if (!Result)
	{
		return;
	}
	
	bool bLocationAdded = false;
	for (auto& Element : PepperballLocations)
	{
		if (FVector::DistSquared(ProjectedPepperballLocation, Element.Key) <= PepperballLcoationToToleranceSquared)
		{
			PepperballLocations.Emplace((Element.Key + ProjectedPepperballLocation) / 2, Element.Value + 1);
			PepperballLocations.Remove(Element.Key);
			bLocationAdded = true;
			break;
		}
	}
	if (!bLocationAdded)
	{
		PepperballLocations.Emplace(ProjectedPepperballLocation, 1);
	}

	CalculatePepperballGasClouds(Projectile);

	if (!GetWorld()->GetTimerManager().IsTimerActive(TH_PepperballLocationUpdate))
	{
		GetWorld()->GetTimerManager().SetTimer(TH_PepperballLocationUpdate, this, &UCSGasManager::TickPepperballGasClouds, 1.0f);
	}
	
	//UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	//FNavLocation ProjectedLocation;

	/* Existing way
	//Snap it to a grid after projecting to nav
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	FNavLocation ProjectedLocation;
	NavSys->ProjectPointToNavigation(Location, ProjectedLocation, FVector(1000, 1000, 500));
	float GridResolution = 200;
	FVector GridLocation = ProjectedLocation.Location;
	//FVector GridLocation = ProjectedLocation.Location.GridSnap(GridResolution);
	GridLocation = FVector(FMath::GridSnap(GridLocation.X, GridResolution),FMath::GridSnap(GridLocation.Y, GridResolution),FMath::GridSnap(GridLocation.Z, 50.f));

	if (PepperballGridLocationsCount.Contains(GridLocation))
	{
		PepperballGridLocationsCount[GridLocation] += 1;	
	}
	else
	{
		PepperballGridLocationsCount.Emplace(GridLocation, 1);
		if (CVarRonDrawDebugGasPoints.GetValueOnGameThread() != 0)
			DrawDebugPoint(GetWorld(), GridLocation, 50, FColor::Black, false, 10);
	}

	CalculatePepperballGasClouds(Projectile);
	*/


/* Experimental stuff
	ANavigationData* NavData = NavSys->GetDefaultNavDataInstance();
	ARecastNavMesh* NavMesh = Cast<ARecastNavMesh>(NavData);

	FNavLocation ProjLoc;
	NavSys->ProjectPointToNavigation(Location, ProjLoc, FVector(100, 100, 200));
	TArray<NavNodeRef> Polys;
	NavMesh->GetPolysWithinPathingDistance(ProjLoc.Location, 800, Polys, nullptr, Projectile, nullptr);

	TArray<FVector> AllVertices;
	AllVertices.Reserve(Polys.Num() * MAX_VERTS_PER_POLY);

	TArray<int32> Triangles;
	Triangles.Reserve(Polys.Num() * MAX_VERTS_PER_POLY);
	
	for (auto Poly : Polys)
	{
		TArray<FVector> PolyVerts;
		NavMesh->GetPolyVerts(Poly, PolyVerts);
		
		for (int i = 0; i < PolyVerts.Num(); i++)
		{
			if (FVector::Distance(PolyVerts[i], ProjLoc.Location) > 600)
			{
				PolyVerts[i] = (PolyVerts[i] - ProjLoc.Location) / ((PolyVerts[i] - ProjLoc.Location).Size()) * 600;
				PolyVerts[i] = PolyVerts[i] + ProjLoc.Location;
			}
			

			DrawDebugPoint(GetWorld(), PolyVerts[i] + FVector(0, 0, 100), 50, FColor::Black, false, 10);

			if (i >= 2)
			{
				Triangles.Append({AllVertices.Num(), AllVertices.Num() + i - 1, AllVertices.Num() + i});
			}
		}

		for (int i = 0; i < PolyVerts.Num(); i++)
		{
			AllVertices.Emplace(PolyVerts[i]);
		}
		
		FVector Center;
		NavMesh->GetPolyCenter(Poly, Center);
		DrawDebugPoint(GetWorld(), Center + FVector(0, 0, 100), 50, FColor::Red, false, 10);
	}

	DrawDebugSphere(GetWorld(), ProjLoc.Location, 600, 24, FColor::White, false, 10, 0, 10);

	AActor* ProceduralMeshActor = GetWorld()->SpawnActor(AActor::StaticClass());
	UProceduralMeshComponent* ProceduralMesh = Cast<UProceduralMeshComponent>(ProceduralMeshActor->AddComponentByClass(UProceduralMeshComponent::StaticClass(), false, FTransform::Identity, false));
	ProceduralMesh->SetWorldLocation(ProjLoc.Location);

	TArray<FVector> NullArray;
	TArray<FVector2D> NullArray2D;
	TArray<FColor> NullArrayColor;
	TArray<FProcMeshTangent> NullArrayTangent;
	ProceduralMesh->CreateMeshSection(0, AllVertices, Triangles, NullArray, NullArray2D, NullArrayColor, NullArrayTangent, true);

	//ProceduralMeshActor->AddToRoot();
*/
}
	
void UCSGasManager::CalculatePepperballGasClouds(APepperProjectile* Projectile)
{
	for (TMap<FVector, int32>::TIterator It = PepperballLocations.CreateIterator(); It; ++It)
	{
		if (It.Value() >= FMath::RandRange(3, 5))
		{
			APepperGasCloud* PepperballGasSource = Cast<APepperGasCloud>(GetWorld()->SpawnActor(GasData->PepperGasCloud));
			PepperballGasSource->SetActorLocation(It.Key());
			AddGasSource(PepperballGasSource);
			OnPepperCloudSpawned(It.Key());
			
			It.RemoveCurrent();
		}
	}
}

void UCSGasManager::TickPepperballGasClouds()
{
	// Budget implementation for now, remove 2 from each location each tick
	for (TMap<FVector, int32>::TIterator It = PepperballLocations.CreateIterator(); It; ++It)
	{
		PepperballLocations[It.Key()] -= 1;
		if (PepperballLocations[It.Key()] <= 0)
		{
			It.RemoveCurrent();
		}
	}
}

bool UCSGasManager::ShouldApplyGasDamage(FVector Location, AActor* GasSource, UNavigationSystemV1* NavSys)
{
	if (!IsValid(GasSource) || !IsValid(NavSys))
		return false;
	
	float GasSourceRadius = IGasSource::Execute_GetGasRadius(GasSource) - 50;
	FVector GasSourceReleaseLocation;
	bool bValidGasSourceReleaseLocation = IGasSource::Execute_GetGasReleaseLocation(GasSource, GasSourceReleaseLocation);
	GasSourceReleaseLocation = bValidGasSourceReleaseLocation ? GasSourceReleaseLocation : GasSource->GetActorLocation();

	if (bValidGasSourceReleaseLocation)
	{
		FNavLocation GasSourceLocation;
		NavSys->ProjectPointToNavigation(GasSourceReleaseLocation, GasSourceLocation, FVector(50, 50, 150));
		FVector GasSourceProjectedLocation = GasSourceLocation.Location;
				
		if (FVector::Distance(GasSourceProjectedLocation, Location) <= GasSourceRadius)
		{
			float PathLength;
			NavSys->GetPathLength(this, Location, GasSourceProjectedLocation, PathLength, NavSys->MainNavData, UNavQuery_CSGas::StaticClass());
			if (PathLength < GasSourceRadius)
			{
				return true;
			}
		}
	}
	else if (FVector::Distance(GasSource->GetActorLocation(), Location) <= GasSourceRadius)
	{
		FHitResult Hit;
		FCollisionObjectQueryParams ObjectQueryParams = FCollisionObjectQueryParams(ECC_WorldStatic);
		GetWorld()->LineTraceSingleByObjectType(Hit, GasSourceReleaseLocation, Location, ObjectQueryParams);
		if (!Hit.bBlockingHit)
		{
			return true;
		}
	}

	return false;
}