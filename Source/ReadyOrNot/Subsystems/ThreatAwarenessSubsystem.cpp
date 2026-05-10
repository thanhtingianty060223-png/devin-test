// Copyright Void Interactive, 2023

#include "ThreatAwarenessSubsystem.h"

#include "Info/SWATManager.h"

void UThreatAwarenessSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	FWorldDelegates::OnWorldInitializedActors.AddUObject(this, &UThreatAwarenessSubsystem::OnWorldInitialized);
	FWorldDelegates::OnPostWorldCleanup.AddUObject(this, &UThreatAwarenessSubsystem::OnPostWorldCleanup);
}

void UThreatAwarenessSubsystem::Deinitialize()
{
	Super::Deinitialize();

	FWorldDelegates::OnWorldInitializedActors.RemoveAll(this);
	FWorldDelegates::OnPostWorldCleanup.RemoveAll(this);

	ShutdownThreatAwarenessSystem();
}

UThreatAwarenessSubsystem* UThreatAwarenessSubsystem::Get(const UObject* WorldContext)
{
	return WorldContext->GetWorld()->GetSubsystem<UThreatAwarenessSubsystem>();
}

void UThreatAwarenessSubsystem::OnWorldGenerated()
{
	RemoveAllThreatPoints();

	// Find all threat awareness actors in current world and add them to the octree
	TArray<FThreatAwarenessData> ThreatPointsToAdd;
	ThreatPointsToAdd.Reserve(2000);
	
	for (TActorIterator<AThreatAwarenessActor> It(GetWorld()); It; ++It)
	{
		AThreatAwarenessActor* TAA = *It;
		ThreatPointsToAdd.AddUnique({TAA, TAA->GetActorLocation(), TAA->GetThreatLevel()});

		AllThreatActors.AddUnique(TAA);
	}

	AddThreatPoints(ThreatPointsToAdd);
}

void UThreatAwarenessSubsystem::OnWorldInitialized(const UWorld::FActorsInitializedParams& Params)
{
	if (Params.World)
	{
		Params.World->OnLevelsChanged().AddUObject(this, &UThreatAwarenessSubsystem::InitializeThreatAwarenessSystem);
		Params.World->OnWorldBeginPlay.AddUObject(this, &UThreatAwarenessSubsystem::InitializeThreatAwarenessSystem);
	}
}

void UThreatAwarenessSubsystem::OnPostWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
	ShutdownThreatAwarenessSystem();
}

void UThreatAwarenessSubsystem::InitializeThreatAwarenessSystem()
{
	if (!GetWorld()->IsGameWorld() || GetWorld()->bIsTearingDown)
		return;
	
	#if !UE_BUILD_SHIPPING
	ULog::StartDebugTimer("Threat Awareness System Initialized");
	#endif

	ThreatAwarenessOctree = MakeShareable(new FThreatAwarenessOctree(FVector::ZeroVector, 32000));

	bDrawThreatPoints = false;
	bDrawThreatRoomNames = false;
	bDrawThreatAwarenessOctree = false;

	// Find all threat awareness actors in current world and add them to the octree
	TArray<FThreatAwarenessData> ThreatPointsToAdd;
	ThreatPointsToAdd.Reserve(2000);
	AllThreatActors.Empty(2000);
	
	for (TActorIterator<AThreatAwarenessActor> It(GetWorld()); It; ++It)
	{
		AThreatAwarenessActor* TAA = *It;
		ThreatPointsToAdd.AddUnique({TAA, TAA->GetActorLocation(), TAA->GetThreatLevel()});

		AllThreatActors.AddUnique(TAA);
	}

	AddThreatPoints(ThreatPointsToAdd);

	#if !UE_BUILD_SHIPPING
	ULog::StopDebugTimer(true);
	#endif
}

void UThreatAwarenessSubsystem::ShutdownThreatAwarenessSystem()
{
	AllThreatActors.Empty(2000);
	
	if (ThreatAwarenessOctree)
	{
		ThreatAwarenessOctree->Destroy();
		ThreatAwarenessOctree = nullptr;
	}

	bDrawThreatPoints = false;
	bDrawThreatAwarenessOctree = false;
}

void UThreatAwarenessSubsystem::AddThreatPoint(const FThreatAwarenessData& InThreatPoint)
{
	FScopeLock ScopeLock{&Mutex};

	if (ThreatAwarenessOctree)
		ThreatAwarenessOctree->AddThreatPoint(InThreatPoint);
}

void UThreatAwarenessSubsystem::AddThreatPoints(const TArray<FThreatAwarenessData>& InThreatPoints)
{
	FScopeLock ScopeLock{&Mutex};

	if (ThreatAwarenessOctree)
	{
		for (const FThreatAwarenessData& ThreatPoint : InThreatPoints)
		{
			ThreatAwarenessOctree->AddThreatPoint(ThreatPoint);
		}

		ThreatAwarenessOctree->ShrinkElements();
	}
}

void UThreatAwarenessSubsystem::RemoveThreatPoint(AThreatAwarenessActor* InThreatPoint)
{
	FScopeLock ScopeLock{&Mutex};

	if (ThreatAwarenessOctree)
	{
		ThreatAwarenessOctree->RemoveThreatPoint(InThreatPoint);
	}
}

void UThreatAwarenessSubsystem::RemoveThreatPoints(const TArray<AThreatAwarenessActor*>& InThreatPoints)
{
	FScopeLock ScopeLock{&Mutex};

	if (ThreatAwarenessOctree)
	{
		for (AThreatAwarenessActor* ThreatPoint : InThreatPoints)
		{
			ThreatAwarenessOctree->RemoveThreatPoint(ThreatPoint);
		}

		ThreatAwarenessOctree->ShrinkElements();
	}
}

void UThreatAwarenessSubsystem::RemoveAllThreatPoints()
{
	FScopeLock ScopeLock{&Mutex};

	AllThreatActors.Empty(2000);
	
	if (ThreatAwarenessOctree)
	{
		ThreatAwarenessOctree->RemoveAllThreatPoints();
	}
}

bool UThreatAwarenessSubsystem::FindThreatPoints(TArray<FThreatAwarenessDataOctreeElement>& OutThreatPoints, const FSphere& InQuerySphere)
{
	FScopeLock ScopeLock{&Mutex};

	if (ThreatAwarenessOctree)
		return ThreatAwarenessOctree->FindThreatPoints(OutThreatPoints, InQuerySphere);

	return false;
}

bool UThreatAwarenessSubsystem::FindThreatPoints(TArray<FThreatAwarenessDataOctreeElement>& OutThreatPoints, const FBox& InQueryBox)
{
	FScopeLock ScopeLock{&Mutex};

	if (ThreatAwarenessOctree)
		return ThreatAwarenessOctree->FindThreatPoints(OutThreatPoints, InQueryBox);
	
	return false;
}

void UThreatAwarenessSubsystem::GetThreatsForLocation(TArray<AThreatAwarenessActor*>& OutThreats, FVector Location, float MinDistance, bool bRequireLOS, FName RoomName)
{
	OutThreats.Empty(100);
	
	TArray<FThreatAwarenessDataOctreeElement> ThreatPoints;
	ThreatPoints.Reserve(100);
	
	const float ClampedXY = FMath::Clamp(MinDistance, 1.0f, 10000.0f);
	const float ClampedZ = FMath::Clamp(MinDistance, 1.0f, 10000.0f);
	FindThreatPoints(ThreatPoints, FBox::BuildAABB(Location, FVector(ClampedXY, ClampedXY, ClampedZ)));

	if (ThreatPoints.Num() > 0)
	{
		if (bRequireLOS)
		{
			FCollisionObjectQueryParams CollisionObjectQueryParams;
			CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
			FCollisionQueryParams CollisionQueryParams;
			CollisionQueryParams.AddIgnoredActors((TArray<AActor*>)GetWorld()->GetGameState<AReadyOrNotGameState>()->AllReadyOrNotCharacters);
			CollisionQueryParams.AddIgnoredActors((TArray<AActor*>)GetWorld()->GetGameState<AReadyOrNotGameState>()->AllDoors);
			CollisionQueryParams.AddIgnoredActors((TArray<AActor*>)GetWorld()->GetGameState<AReadyOrNotGameState>()->AllItems);
			
			for (const FThreatAwarenessDataOctreeElement& Element : ThreatPoints)
			{
				AThreatAwarenessActor* TAA = Element.Data->ThreatAwarenessActor.Get();
				
				if (RoomName != NAME_None && TAA->OwningRoom != RoomName)
				{
					continue;
				}
				
				if (GetWorld()->LineTraceTestByObjectType(Element.Data->Location, Location, CollisionObjectQueryParams, CollisionQueryParams))
				{
					continue;
				}

				OutThreats.Add(TAA);
			}
		}
		else
		{
			for (const FThreatAwarenessDataOctreeElement& Element : ThreatPoints)
			{
				AThreatAwarenessActor* TAA = Element.Data->ThreatAwarenessActor.Get();
				
				if (RoomName != NAME_None && TAA->OwningRoom != RoomName)
				{
					continue;
				}
				
				OutThreats.Add(TAA);
			}
		}
	}
}

AThreatAwarenessActor* UThreatAwarenessSubsystem::GetNearestThreatForLocation(FVector Location, float MaxDistance, float MaxZHeight, bool bRequireLOS, TArray<EThreatLevel> ExcludeThreats, TArray<AThreatAwarenessActor*> ExcludeThreatActors, FName RoomName)
{
	TArray<FThreatAwarenessDataOctreeElement> ThreatPoints;
	ThreatPoints.Reserve(100);
	
	const float ClampedXY = FMath::Clamp(MaxDistance, 1.0f, 10000.0f);
	const float ClampedZ = FMath::Clamp(MaxZHeight, 1.0f, 10000.0f);
	FindThreatPoints(ThreatPoints, FBox::BuildAABB(Location, FVector(ClampedXY, ClampedXY, ClampedZ)));

	if (ThreatPoints.Num() > 0)
	{
		ThreatPoints.Sort([Location](const FThreatAwarenessDataOctreeElement& Lhs, const FThreatAwarenessDataOctreeElement& Rhs)
		{
			return FVector::DistSquared(Lhs.Data->Location, Location) < FVector::DistSquared(Rhs.Data->Location, Location);
		});

		if (bRequireLOS)
		{
			FCollisionObjectQueryParams CollisionObjectQueryParams;
			CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
			FCollisionQueryParams CollisionQueryParams;
			CollisionQueryParams.AddIgnoredActors((TArray<AActor*>)GetWorld()->GetGameState<AReadyOrNotGameState>()->AllReadyOrNotCharacters);
			CollisionQueryParams.AddIgnoredActors((TArray<AActor*>)GetWorld()->GetGameState<AReadyOrNotGameState>()->AllDoors);
			CollisionQueryParams.AddIgnoredActors((TArray<AActor*>)GetWorld()->GetGameState<AReadyOrNotGameState>()->AllItems);
			
			for (const FThreatAwarenessDataOctreeElement& Element : ThreatPoints)
			{
				AThreatAwarenessActor* TAA = Element.Data->ThreatAwarenessActor.Get();
				
				if ((ExcludeThreats.Contains(Element.Data->ThreatLevel) ||
					ExcludeThreatActors.Contains(TAA)) ||
					(RoomName != NAME_None && TAA->OwningRoom != RoomName))
				{
					continue;
				}
				
				if (GetWorld()->LineTraceTestByObjectType(Element.Data->Location, Location, CollisionObjectQueryParams, CollisionQueryParams))
				{
					continue;
				}
				
				return TAA;
			}
		}
		else
		{
			for (const FThreatAwarenessDataOctreeElement& Element : ThreatPoints)
			{
				AThreatAwarenessActor* TAA = Element.Data->ThreatAwarenessActor.Get();
				
				if ((ExcludeThreats.Contains(Element.Data->ThreatLevel) ||
					ExcludeThreatActors.Contains(TAA)) ||
					(RoomName != NAME_None && TAA->OwningRoom != RoomName))
				{
					continue;
				}
				
				return TAA;
			}
		}
	}
	
	return nullptr;
}

AThreatAwarenessActor* UThreatAwarenessSubsystem::GetFurthestThreatForLocation(FVector Location, float MaxDistance, float MaxZHeight, bool bRequireLOS, TArray<EThreatLevel> ExcludeThreats, TArray<AThreatAwarenessActor*> ExcludeThreatActors, FName RoomName)
{
	TArray<FThreatAwarenessDataOctreeElement> ThreatPoints;
	ThreatPoints.Reserve(100);
	
	const float ClampedXY = FMath::Clamp(MaxDistance, 1.0f, 10000.0f);
	const float ClampedZ = FMath::Clamp(MaxZHeight, 1.0f, 10000.0f);
	FindThreatPoints(ThreatPoints, FBox::BuildAABB(Location, FVector(ClampedXY, ClampedXY, ClampedZ)));

	if (ThreatPoints.Num() > 0)
	{
		ThreatPoints.Sort([Location](const FThreatAwarenessDataOctreeElement& Lhs, const FThreatAwarenessDataOctreeElement& Rhs)
		{
			return FVector::DistSquared(Lhs.Data->Location, Location) > FVector::DistSquared(Rhs.Data->Location, Location);
		});

		if (bRequireLOS)
		{
			FCollisionObjectQueryParams CollisionObjectQueryParams;
			CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
			FCollisionQueryParams CollisionQueryParams;
			CollisionQueryParams.AddIgnoredActors((TArray<AActor*>)GetWorld()->GetGameState<AReadyOrNotGameState>()->AllReadyOrNotCharacters);
			CollisionQueryParams.AddIgnoredActors((TArray<AActor*>)GetWorld()->GetGameState<AReadyOrNotGameState>()->AllDoors);
			CollisionQueryParams.AddIgnoredActors((TArray<AActor*>)GetWorld()->GetGameState<AReadyOrNotGameState>()->AllItems);
			
			for (const FThreatAwarenessDataOctreeElement& Element : ThreatPoints)
			{
				AThreatAwarenessActor* TAA = Element.Data->ThreatAwarenessActor.Get();
				
				if ((ExcludeThreats.Contains(Element.Data->ThreatLevel) ||
					ExcludeThreatActors.Contains(TAA)) ||
					(RoomName != NAME_None && TAA->OwningRoom != RoomName))
				{
					continue;
				}
				
				if (GetWorld()->LineTraceTestByObjectType(Element.Data->Location, Location, CollisionObjectQueryParams, CollisionQueryParams))
				{
					continue;
				}
				
				return TAA;
			}
		}
		else
		{
			for (const FThreatAwarenessDataOctreeElement& Element : ThreatPoints)
			{
				AThreatAwarenessActor* TAA = Element.Data->ThreatAwarenessActor.Get();
				
				if ((ExcludeThreats.Contains(Element.Data->ThreatLevel) ||
					ExcludeThreatActors.Contains(TAA)) ||
					(RoomName != NAME_None && TAA->OwningRoom != RoomName))
				{
					continue;
				}
				
				return TAA;
			}
		}
	}
	
	return nullptr;
}

AThreatAwarenessActor* UThreatAwarenessSubsystem::GetNearestHighestThreat(const TArray<AThreatAwarenessActor*> InThreats, FVector Location)
{
	if (Location == FVector::ZeroVector || InThreats.Num() == 0)
		return nullptr;

	float ClosestDistance = FLT_MAX;
	EThreatLevel HighestThreatLevel = EThreatLevel::TL_None;
	for (const AThreatAwarenessActor* TAA : InThreats)
	{
		if (TAA->GetThreatLevel() > HighestThreatLevel)
		{
			HighestThreatLevel = TAA->GetThreatLevel();
		}
	}
	
	AThreatAwarenessActor* HighestThreat = nullptr;
	for (AThreatAwarenessActor* TAA : InThreats)
	{
		const float Distance = FVector::Distance(Location, TAA->GetActorLocation());
		
		if (TAA->GetThreatLevel() == HighestThreatLevel &&
			Distance < ClosestDistance)
		{
			ClosestDistance = Distance;
			HighestThreat = TAA;
		}
	}

	return HighestThreat;
}

AThreatAwarenessActor* UThreatAwarenessSubsystem::GetFurthestHighestThreat(const TArray<AThreatAwarenessActor*> InThreats, FVector Location, float MinDistance)
{
	if (Location == FVector::ZeroVector || InThreats.Num() == 0)
		return nullptr;
		
	AThreatAwarenessActor* HighestThreat = nullptr;

	float FurthestDistance = MinDistance;
	EThreatLevel HighestThreatLevel = EThreatLevel::TL_None;
	for (AThreatAwarenessActor* TAA : InThreats)
	{
		const float Distance = FVector::Distance(Location, TAA->GetActorLocation());
		if (Distance < MinDistance)
			continue;
		
		if (TAA->GetThreatLevel() >= HighestThreatLevel ||
			Distance > FurthestDistance)
		{
			HighestThreatLevel = TAA->GetThreatLevel();
			FurthestDistance = Distance;
			HighestThreat = TAA;
		}
	}

	return HighestThreat;
}

TArray<AThreatAwarenessActor*> UThreatAwarenessSubsystem::GetThreatsFromLocationBeyondRadius(const TArray<AThreatAwarenessActor*> InThreats, FVector Location, float MinDistance)
{
	if (Location == FVector::ZeroVector || InThreats.Num() == 0)
		return {};
	
	TArray<AThreatAwarenessActor*> HighestThreats;

	for (AThreatAwarenessActor* TAA : InThreats)
	{
		const float Distance = FVector::Distance(Location, TAA->GetActorLocation());
		
		if (Distance > MinDistance)
		{
			HighestThreats.AddUnique(TAA);
		}
	}

	return HighestThreats;
}

FBox UThreatAwarenessSubsystem::CreateThreatSearchArea(const FVector& InThreatLocation, const FVector& InSearchBoxExtent)
{
	const FBoxCenterAndExtent SearchBox = FBoxCenterAndExtent(InThreatLocation, InSearchBoxExtent);
	return SearchBox.GetBox();
}

AThreatAwarenessActor* UThreatAwarenessSubsystem::FindClosestThreatPoint(const TArray<FThreatAwarenessDataOctreeElement>& InThreatPoints, const FVector& InTestLocation, ThreatSearchPredicate Predicate)
{
	if (InThreatPoints.Num() == 0)
		return nullptr;
	
	float ClosestDistance = FLT_MAX;
	AThreatAwarenessActor* ClosestThreatPoint = nullptr;

	for (const FThreatAwarenessDataOctreeElement& ThreatPoint : InThreatPoints)
	{
		const float Distance = FVector::Distance(ThreatPoint.Data->Location, InTestLocation);
		if (Distance < ClosestDistance && Predicate(ThreatPoint))
		{
			ClosestDistance = Distance;
			ClosestThreatPoint = ThreatPoint.Data->ThreatAwarenessActor.Get();
		}
	}
	
	return ClosestThreatPoint;
}

AThreatAwarenessActor* UThreatAwarenessSubsystem::FindFurthestThreatPoint(const TArray<FThreatAwarenessDataOctreeElement>& InThreatPoints, const FVector& InTestLocation, ThreatSearchPredicate Predicate)
{
	if (InThreatPoints.Num() == 0)
		return nullptr;
	
	float FurthestDistance = 0.0f;
	AThreatAwarenessActor* FurthestThreatPoint = nullptr;

	for (const FThreatAwarenessDataOctreeElement& ThreatPoint : InThreatPoints)
	{
		const float Distance = FVector::Distance(ThreatPoint.Data->Location, InTestLocation);
		if (Distance > FurthestDistance && Predicate(ThreatPoint))
		{
			FurthestDistance = Distance;
			FurthestThreatPoint = ThreatPoint.Data->ThreatAwarenessActor.Get();
		}
	}
	
	return FurthestThreatPoint;
}

AThreatAwarenessActor* UThreatAwarenessSubsystem::FindFurthestHighestThreatPoint(const TArray<FThreatAwarenessDataOctreeElement>& InThreatPoints, const FVector& InTestLocation, ThreatSearchPredicate Predicate)
{
	if (InThreatPoints.Num() == 0)
		return nullptr;
	
	TArray<const FThreatAwarenessDataOctreeElement*> ThreatPointsInLOS;
	
	for (const FThreatAwarenessDataOctreeElement& ThreatPoint : InThreatPoints)
	{
		// has line of sight to threat?
		FHitResult Hit;
		FCollisionQueryParams CollisionQueryParams;
		CollisionQueryParams.AddIgnoredActors((TArray<AActor*>)USWATManager::Get(UBpGameplayHelperLib::GetWorldStatic())->SwatAI);
		if (!UBpGameplayHelperLib::GetWorldStatic()->LineTraceSingleByChannel(Hit, InTestLocation, ThreatPoint.Data->Location, ECC_Visibility, CollisionQueryParams))
		{
			ThreatPointsInLOS.Add(&ThreatPoint);
		}
	}
	
	EThreatLevel HighestThreatLevel = EThreatLevel::TL_None;
	AThreatAwarenessActor* FurthestThreatPoint = nullptr;
	float FurthestDistance = 0.0f;
	for (const FThreatAwarenessDataOctreeElement* ThreatPoint : ThreatPointsInLOS)
	{
		if (ThreatPoint->Data->ThreatLevel > HighestThreatLevel)
		{
			FurthestDistance = 0.0f;
		}
		
		const float Distance = FVector::Distance(ThreatPoint->Data->Location, InTestLocation);
		
		if (Distance > FurthestDistance && ThreatPoint->Data->ThreatLevel >= HighestThreatLevel && Predicate(*ThreatPoint))
		{
			FurthestDistance = Distance;
			FurthestThreatPoint = ThreatPoint->Data->ThreatAwarenessActor.Get();
			HighestThreatLevel = ThreatPoint->Data->ThreatLevel;
		}
	}
	
	return FurthestThreatPoint;
}

void UThreatAwarenessSubsystem::Tick(const float DeltaTime)
{
#if !UE_BUILD_SHIPPING
	if (ThreatAwarenessOctree && GetWorld() && GetWorld()->WorldType != EWorldType::Editor)
	{
		TArray<FBatchedLine> OctreeLines;
		
		if (bDrawThreatAwarenessOctree)
		{
			OctreeLines.Reserve(128);
			
			// Go through the nodes of the octree and draw them
			// ##UE5UPGRADE## Compatibility
			ThreatAwarenessOctree->FindNodesWithPredicate([&](FThreatAwarenessOctree::FNodeIndex NodeIndex, FThreatAwarenessOctree::FNodeIndex CurrentNodeIndex, FBoxCenterAndExtent CurrentBounds)
			{
				// Draw Node Bounds
				const FVector Center = CurrentBounds.Center;
				const FVector Extent = CurrentBounds.Extent;
				
				const FVector Box = Extent;
				const FColor Color = FColor::Blue;
				const float Thickness = 5.0f;

				// DrawDebugBox is slower, batch them instead
				//DrawDebugBox(GetWorld(), Center, MaxExtent, FColor::Blue, false, 1.0f, 0, 10.0f);
				
				OctreeLines.Add({Center + FVector( Box.X,  Box.Y,  Box.Z), Center + FVector( Box.X, -Box.Y, Box.Z), Color, 0.5f, Thickness, 0});
				OctreeLines.Add({Center + FVector( Box.X, -Box.Y,  Box.Z), Center + FVector(-Box.X, -Box.Y, Box.Z), Color, 0.5f, Thickness, 0});
				OctreeLines.Add({Center + FVector(-Box.X, -Box.Y,  Box.Z), Center + FVector(-Box.X,  Box.Y, Box.Z), Color, 0.5f, Thickness, 0});
				OctreeLines.Add({Center + FVector(-Box.X,  Box.Y,  Box.Z), Center + FVector( Box.X,  Box.Y, Box.Z), Color, 0.5f, Thickness, 0});

				OctreeLines.Add({Center + FVector( Box.X,  Box.Y, -Box.Z), Center + FVector( Box.X, -Box.Y, -Box.Z), Color, 0.5f, Thickness, 0});
				OctreeLines.Add({Center + FVector( Box.X, -Box.Y, -Box.Z), Center + FVector(-Box.X, -Box.Y, -Box.Z), Color, 0.5f, Thickness, 0});
				OctreeLines.Add({Center + FVector(-Box.X, -Box.Y, -Box.Z), Center + FVector(-Box.X,  Box.Y, -Box.Z), Color, 0.5f, Thickness, 0});
				OctreeLines.Add({Center + FVector(-Box.X,  Box.Y, -Box.Z), Center + FVector( Box.X,  Box.Y, -Box.Z), Color, 0.5f, Thickness, 0});

				OctreeLines.Add({Center + FVector( Box.X,  Box.Y,  Box.Z), Center + FVector( Box.X,  Box.Y, -Box.Z), Color, 0.5f, Thickness, 0});
				OctreeLines.Add({Center + FVector( Box.X, -Box.Y,  Box.Z), Center + FVector( Box.X, -Box.Y, -Box.Z), Color, 0.5f, Thickness, 0});
				OctreeLines.Add({Center + FVector(-Box.X, -Box.Y,  Box.Z), Center + FVector(-Box.X, -Box.Y, -Box.Z), Color, 0.5f, Thickness, 0});
				OctreeLines.Add({Center + FVector(-Box.X,  Box.Y,  Box.Z), Center + FVector(-Box.X,  Box.Y, -Box.Z), Color, 0.5f, Thickness, 0});
				
				return true;
			},
			// ##UE5UPGRADE## Compatibility
			[&](FThreatAwarenessOctree::FNodeIndex NodeIndex, FThreatAwarenessOctree::FNodeIndex CurrentNodeIndex, FBoxCenterAndExtent Bounds)
			{
			});
		}

		TArray<FBatchedLine> ThreatPointLines;
		if (bDrawThreatPoints)
		{
			TArray<FThreatAwarenessDataOctreeElement> ThreatPoints;
			FindThreatPoints(ThreatPoints, FBoxCenterAndExtent(FVector::ZeroVector, FVector(32000.0f)).GetBox());

			ThreatPointLines.Reserve(ThreatPoints.Num());
			for (const FThreatAwarenessDataOctreeElement& ThreatPoint : ThreatPoints)
			{
				FColor ThreatColor = ThreatPoint.Data->ThreatLevel == EThreatLevel::TL_High ? FColor::Orange : ThreatPoint.Data->ThreatLevel == EThreatLevel::TL_Medium ? FLinearColor(0.027321, 0.917207, 0.138398).ToFColor(true) : FColor::Cyan;
				
				if (ThreatPoint.Data->ThreatLevel >= EThreatLevel::TL_Extreme)
					ThreatColor = FColor::Red;
				
				ThreatPointLines.Add({ThreatPoint.Data->Location, ThreatPoint.Data->Location, ThreatColor, DeltaTime + 0.1f, 10.0f, 0});
				
				if (bDrawThreatRoomNames)
				{
					if (const APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0))
					{
						if (FVector::Distance(ThreatPoint.Data->Location, CameraManager->GetCameraLocation()) < 1000.0f)
						{
							const FVector DirectionToCoverPoint = (ThreatPoint.Data->Location - CameraManager->GetCameraLocation()).GetSafeNormal();
							const float DotProduct = FVector::DotProduct(CameraManager->GetCameraRotation().Vector(), DirectionToCoverPoint);
							
							if (DotProduct > 0.5f)
							{
								DrawDebugString(GetWorld(), ThreatPoint.Data->Location, ThreatPoint.Data->ThreatAwarenessActor->OwningRoom.ToString(), nullptr, FColor::White, DeltaTime, true);
							}
						}
					}
				}
			}
		}

		if (ULineBatchComponent* LineBatcher = GetWorld()->ForegroundLineBatcher)
		{
			if (ThreatPointLines.Num() > 0)
				LineBatcher->DrawLines(ThreatPointLines);
			
			if (OctreeLines.Num() > 0)
				LineBatcher->DrawLines(OctreeLines);
		}
	}
#endif
}

bool UThreatAwarenessSubsystem::IsTickable() const
{
	return IsValid(this) && !HasAnyFlags(RF_BeginDestroyed | RF_FinishDestroyed | RF_ClassDefaultObject);
}

TStatId UThreatAwarenessSubsystem::GetStatId() const
{
	return GetStatID();
}
