// Copyright Void Interactive, 2022

#include "RosterScenarioSpawner.h"

#include "Actors/Door.h"
#include "Actors/ExplosiveVest.h"
#include "Actors/WorldDataGenerator.h"
#include "Actors/Items/MeleeWeapon.h"
#include "BombActor.h"

#include "Components/MirrorPortalComponent.h"

#include "Info/WorldBuildingPlacementActor.h"
#include "Info/Activities/InvestigateActivity.h"
#include "Info/Activities/PlaceTrapActivity.h"

#include "NavigationSystem.h"
#include "Navigation/ReadyOrNotNavQueries.h"

#include "ReadyOrNotAIConfig.h"

// #include "GeneratedCodeHelpers.h"
#include "Algo/RandomShuffle.h"

ARosterScenarioSpawner::ARosterScenarioSpawner()
{
}

void ARosterScenarioSpawner::DoRoster()
{
	TArray<AAISpawn*> ConfiguredSpawns;
	
	TArray<AAISpawn*> PossibleSpawns_Suspects;
	TArray<AAISpawn*> PossibleSpawns_Civilians;

	TArray<TSubclassOf<AMeleeWeapon>> MeleeWeapons;
	
	TSubclassOf<AMeleeWeapon> Knife = StaticLoadClass(AMeleeWeapon::StaticClass(), nullptr, TEXT("Blueprint'/Game/Blueprints/Items/WeaponsSuspect/Melee_Knife.Melee_Knife_C'"));
	
	if (Knife)
	{
		MeleeWeapons.Add(Knife);
	}

	// Gather and setup all spawners
	for (TActorIterator<AAISpawn> It(GetWorld()); It; ++It)
	{
		AAISpawn* Spawner = *It;
		
		if (!Spawner->bEnabled)
			continue;
		
		if (!IsSpawnLocationValid(Spawner->GetActorLocation()))
			continue;

		// Impossible to spawn
		if (Spawner->SpawnChance <= 0.0f)
			continue;

		// Clamp down to 1% min
		Spawner->SpawnChance = FMath::Clamp(Spawner->SpawnChance, 0.001f, 1.0f);

		TArray<const FSpawnData*> SpawnDataArray;
		
		if (Spawner->GetSuspectSpawnData(SpawnDataArray))
		{
			Spawner->SpawnData = *SpawnDataArray[FMath::RandRange(0, SpawnDataArray.Num() - 1)];

			PossibleSpawns_Suspects.AddUnique(Spawner);
		}
		else if (Spawner->GetCivilianSpawnData(SpawnDataArray))
		{
			Spawner->SpawnData = *SpawnDataArray[FMath::RandRange(0, SpawnDataArray.Num() - 1)];
			
			PossibleSpawns_Civilians.AddUnique(Spawner);
		}
	}

	TArray<AAISpawn*> ConfiguredObjectiveSpawns;
	
	// Configure objective spawns
	for (FObjectiveSpawn ObjSpawn : Objectives)
	{
		ObjSpawn.Spawners.Remove(nullptr);
		
		if (ObjSpawn.Spawners.Num() <= 0)
			continue;

		if (AAISpawn* Spawn = ObjSpawn.Spawners[FMath::RandRange(0, ObjSpawn.Spawners.Num() - 1)])
		{
			Spawn->SpawnData.SpawnWithTags.AddUnique(ObjSpawn.Tag);
			Spawn->SpawnData.SpawnedAI = ObjSpawn.Type;
			
			PossibleSpawns_Suspects.Remove(Spawn);
			PossibleSpawns_Civilians.Remove(Spawn);

			ConfiguredObjectiveSpawns.Add(Spawn);
		}
		
		for (AAISpawn* Spawn : ObjSpawn.Spawners)
		{
			PossibleSpawns_Suspects.Remove(Spawn);
			PossibleSpawns_Civilians.Remove(Spawn);
		}
	}

	const bool bUseSpawnGroups = AI_CONFIG_GET_BOOL("UseSpawnGroups", false);

	// Setup required suspect count
	uint8 MinSuspects = FMath::Clamp(AI_CONFIG_GET_INT("MinSuspects", 1), 1, 255);
	uint8 MaxSuspects = FMath::Clamp(AI_CONFIG_GET_INT("MaxSuspects", 0), 0, 255);

	if (MaxSuspects == 0)
	{
		MaxSuspects = PossibleSpawns_Suspects.Num();
	}
	
	if (MinSuspects > MaxSuspects)
		MinSuspects = MaxSuspects;

	// Setup required civilian count
	uint8 MinCivilians = FMath::Clamp(AI_CONFIG_GET_INT("MinCivilians", 1), 1, 255);
	uint8 MaxCivilians = FMath::Clamp(AI_CONFIG_GET_INT("MaxCivilians", 0), 0, 255);
	
	if (MaxCivilians == 0)
	{
		MaxCivilians = PossibleSpawns_Civilians.Num();
	}

	if (MinCivilians > MaxCivilians)
		MinCivilians = MaxCivilians;
	
	uint8 RequiredSuspectCount = FMath::RandRange(MinSuspects, MaxSuspects);
	uint8 RequiredCivilianCount = FMath::RandRange(MinCivilians, MaxCivilians);

	// Setup other config counts
	const uint8 MaxMeleeSuspects = FMath::Clamp(AI_CONFIG_GET_INT("MaxMeleeSuspects", 0), 0, 255);

	const uint8 MinExplosiveVestSuspects = FMath::Clamp(AI_CONFIG_GET_INT("MinExplosiveVestSuspects", 0), 0, 255);
	const uint8 MaxExplosiveVestSuspects = FMath::Clamp(AI_CONFIG_GET_INT("MaxExplosiveVestSuspects", 0), 0, 255);
	const uint8 RequiredSuspectExplosiveVests = FMath::RandRange(MinExplosiveVestSuspects, MaxExplosiveVestSuspects);
	
	const uint8 MinExplosiveVestCivilians = FMath::Clamp(AI_CONFIG_GET_INT("MinExplosiveVestCivilians", 0), 0, 255);
	const uint8 MaxExplosiveVestCivilians = FMath::Clamp(AI_CONFIG_GET_INT("MaxExplosiveVestCivilians", 0), 0, 255);
	const uint8 RequiredCivilianExplosiveVests = FMath::RandRange(MinExplosiveVestCivilians, MaxExplosiveVestCivilians);
	
	// Failsafe, clamp down to the actual amount of spawns there are if we dont have enough spawns to meet the required amount
	{
		if (PossibleSpawns_Suspects.Num() < RequiredSuspectCount)
			RequiredSuspectCount = PossibleSpawns_Suspects.Num();
		
		if (PossibleSpawns_Civilians.Num() < RequiredCivilianCount)
			RequiredCivilianCount = PossibleSpawns_Civilians.Num();
	}

	PossibleSpawns_Suspects.Remove(nullptr);
	PossibleSpawns_Civilians.Remove(nullptr);

	TArray<AAISpawn*> ConfiguredSuspectSpawns, ConfiguredCivilianSpawns;

	// Configure all suspect spawns
	{
		uint8 SpawnedSuspects = 0;
		uint8 SpawnedMeleeSuspects = 0;
		uint8 SpawnedExplosiveVestSuspects = 0;
		
		TMap<uint8, TArray<AAISpawn*>> GroupSpawnMap;

		if (bUseSpawnGroups)
		{
			// FCustomThunkTemplates::Array_Shuffle(PossibleSpawns_Suspects);

			// gurantee spawn the min amount AI for every group
			PossibleSpawns_Suspects.Sort([](const AAISpawn& Lhs, const AAISpawn& Rhs)
			{
				return Lhs.GroupID < Rhs.GroupID;
			});

			uint8 GroupIndex = 0;
			uint8 NumConfiguredForGroup = 0;
			TArray<AAISpawn*> Copy = PossibleSpawns_Suspects;
			for (AAISpawn* Spawn : Copy)
			{
				if (Spawn->GroupID < GroupIndex)
					continue;
				
				const FString MinConfigKey = "MinSuspects_Group" + FString::FromInt(Spawn->GroupID);
				const uint8 MinInGroup = FMath::Clamp(AI_CONFIG_GET_INT(MinConfigKey, 0), 0, 255);
				if (NumConfiguredForGroup == MinInGroup)
				{
					NumConfiguredForGroup = 0;
					GroupIndex = Spawn->GroupID+1;
					continue;
				}
				
				GroupSpawnMap.FindOrAdd(Spawn->GroupID).Add(Spawn);
		
				PossibleSpawns_Suspects.Remove(Spawn);
				ConfiguredSuspectSpawns.Add(Spawn);

				NumConfiguredForGroup++;
				SpawnedSuspects++;
			}
		}
		
		// FCustomThunkTemplates::Array_Shuffle(PossibleSpawns_Suspects);

		while (PossibleSpawns_Suspects.Num() > 0)
		{
			if (AAISpawn* Spawn = PossibleSpawns_Suspects[FMath::RandRange(0, PossibleSpawns_Suspects.Num() - 1)])
			{
				bool bNoRandom = false;
				if (bUseSpawnGroups)
				{
					const FString MinConfigKey = "MinSuspects_Group" + FString::FromInt(Spawn->GroupID);
					const FString MaxConfigKey = "MaxSuspects_Group" + FString::FromInt(Spawn->GroupID);
					const uint8 MinInGroup = FMath::Clamp(AI_CONFIG_GET_INT(MinConfigKey, 0), 0, 255);
					const uint8 MaxInGroup = FMath::Clamp(AI_CONFIG_GET_INT(MaxConfigKey, 0), 0, 255);
					if (MaxInGroup == 0)
					{
						PossibleSpawns_Suspects.Remove(Spawn);
						continue;
					}
					
					uint32 NumInGroup = GroupSpawnMap.FindOrAdd(Spawn->GroupID).Num();
					if (NumInGroup >= MaxInGroup)
					{
						PossibleSpawns_Suspects.Remove(Spawn);
						continue;
					}

					bNoRandom = NumInGroup < MinInGroup;
				}
				
				if (SpawnedSuspects < RequiredSuspectCount)
				{
					if (bNoRandom || FMath::FRand() <= Spawn->SpawnChance)
					{
						PossibleSpawns_Suspects.Remove(Spawn);
						ConfiguredSuspectSpawns.Add(Spawn);
						
						GroupSpawnMap.FindOrAdd(Spawn->GroupID).Add(Spawn);

						SpawnedSuspects++;
					}
				}
				else
				{
					// spawned everything possible
					break;
				}
			}
		}

		for (AAISpawn* Spawn : ConfiguredSuspectSpawns)
		{
			if (SpawnedMeleeSuspects < MaxMeleeSuspects && MeleeWeapons.Num() > 0)
			{
				Spawn->SpawnData.ForceWeaponOverride = MeleeWeapons[FMath::RandRange(0, MeleeWeapons.Num() - 1)];
				SpawnedMeleeSuspects++;
			}
			else if (SpawnedExplosiveVestSuspects < RequiredSuspectExplosiveVests && Spawn->bAllowExplosiveVestSpawn)
			{
				Spawn->SpawnData.ForceBodyArmourOverride = "ExplosiveVest";
				SpawnedExplosiveVestSuspects++;
			}
		}
	}

	// Configure all civilian spawns
	{
		uint8 SpawnedCivilians = 0;
		uint8 SpawnedExplosiveVestCivilians = 0;
		
		TMap<uint8, TArray<AAISpawn*>> GroupSpawnMap;
		
		if (bUseSpawnGroups)
		{
			// FCustomThunkTemplates::Array_Shuffle(PossibleSpawns_Civilians);
			
			// gurantee spawn the min amount AI for every group
			PossibleSpawns_Civilians.Sort([](const AAISpawn& Lhs, const AAISpawn& Rhs)
			{
				return Lhs.GroupID < Rhs.GroupID;
			});

			uint8 GroupIndex = 0;
			uint8 NumConfiguredForGroup = 0;
			TArray<AAISpawn*> Copy = PossibleSpawns_Civilians;
			for (AAISpawn* Spawn : Copy)
			{
				if (Spawn->GroupID < GroupIndex)
					continue;
				
				const FString MinConfigKey = "MinCivilians_Group" + FString::FromInt(Spawn->GroupID);
				const uint8 MinInGroup = FMath::Clamp(AI_CONFIG_GET_INT(MinConfigKey, 0), 0, 255);
				if (NumConfiguredForGroup == MinInGroup)
				{
					NumConfiguredForGroup = 0;
					GroupIndex = Spawn->GroupID+1;
					continue;
				}
				
				GroupSpawnMap.FindOrAdd(Spawn->GroupID).Add(Spawn);
		
				PossibleSpawns_Civilians.Remove(Spawn);
				ConfiguredCivilianSpawns.Add(Spawn);

				NumConfiguredForGroup++;
				SpawnedCivilians++;
			}
		}

		// FCustomThunkTemplates::Array_Shuffle(PossibleSpawns_Civilians);
		
		while (PossibleSpawns_Civilians.Num() > 0)
		{
			if (AAISpawn* Spawn = PossibleSpawns_Civilians[FMath::RandRange(0, PossibleSpawns_Civilians.Num() - 1)])
			{
				bool bNoRandom = false;
				if (bUseSpawnGroups)
				{
					const FString MinConfigKey = "MinCivilians_Group" + FString::FromInt(Spawn->GroupID);
					const FString MaxConfigKey = "MaxCivilians_Group" + FString::FromInt(Spawn->GroupID);
					const uint8 MinInGroup = FMath::Clamp(AI_CONFIG_GET_INT(MinConfigKey, 0), 0, 255);
					const uint8 MaxInGroup = FMath::Clamp(AI_CONFIG_GET_INT(MaxConfigKey, 0), 0, 255);
					if (MaxInGroup == 0)
					{
						PossibleSpawns_Civilians.Remove(Spawn);
						
						continue;
					}
					
					uint32 NumInGroup = GroupSpawnMap.FindOrAdd(Spawn->GroupID).Num();
					if (NumInGroup >= MaxInGroup)
					{
						PossibleSpawns_Civilians.Remove(Spawn);
						continue;
					}

					bNoRandom = NumInGroup < MinInGroup;
				}
				
				if (SpawnedCivilians < RequiredCivilianCount)
				{
					if (bNoRandom || FMath::FRand() <= Spawn->SpawnChance)
					{
						PossibleSpawns_Civilians.Remove(Spawn);
						ConfiguredCivilianSpawns.Add(Spawn);
						
						GroupSpawnMap.FindOrAdd(Spawn->GroupID).Add(Spawn);

						SpawnedCivilians++;
					}
				}
				else
				{
					// spawned everything possible
                	break;
				}
			}
		}
		
		for (AAISpawn* Spawn : ConfiguredCivilianSpawns)
		{
			if (SpawnedExplosiveVestCivilians < RequiredCivilianExplosiveVests && Spawn->bAllowExplosiveVestSpawn)
			{
				Spawn->SpawnData.ForceBodyArmourOverride = "CivExplosiveVest";
				SpawnedExplosiveVestCivilians++;
			}
		}
	}

	ConfiguredSpawns = ConfiguredObjectiveSpawns;
	ConfiguredSpawns += ConfiguredSuspectSpawns;
	ConfiguredSpawns += ConfiguredCivilianSpawns;
	
	UsedTrapDoors.Empty();
	
	// do the spawns
	for (AAISpawn* Spawner : ConfiguredSpawns)
	{
		DoSpawn(Spawner);
	}
}

bool ARosterScenarioSpawner::IsSpawnLocationValid(const FVector& Location) const
{
	if (Location == FVector::ZeroVector)
		return false;
	
	if (UReadyOrNotFunctionLibrary::GetCOOPMode() == ECOOPMode::CM_BombThreat)
	{
		for (TActorIterator<ABombActor> It(GetWorld()); It; ++It)
		{
			const float Distance = FVector::Distance(It->GetActorLocation(), Location);
			if (Distance < AI_CONFIG_GET_FLOAT("BTMaxDistanceFromSelectedBombs"))
			{
				return true;
			}
		}
		
		return false;
	}
	
	return true;
}

void ARosterScenarioSpawner::GetAllReachablePatrolPoints(const FVector& Location, TArray<FPatrolPoint>& OutPatrolPoints)
{
	if (AWorldDataGenerator* WorldData = AWorldDataGenerator::Get(GetWorld()))
	{
		if (WorldData->AllPatrolPoints.Num() == 0)
		{
			TArray<AThreatAwarenessActor*> OutThreats;
			OutThreats.Reserve(5000);
			for (TActorIterator<AThreatAwarenessActor>It(GetWorld()); It; ++It)
			{
				OutThreats.Add(*It);
			}
			WorldData->PlacePatrolPointsOnAllThreats(OutThreats);
		}
		
		for (const FPatrolPoint& PatrolPoint : WorldData->AllPatrolPoints)
		{
			if (UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
			{
				FNavLocation NavLocation(Location);

				if (NavSys->ProjectPointToNavigation(NavLocation.Location, NavLocation, FVector(100.0f, 100.0f, 200.0f)))
				{
					FPathFindingQuery PathFindingQuery;
					PathFindingQuery.StartLocation = NavLocation;
					PathFindingQuery.EndLocation = FVector(PatrolPoint.Location);
					PathFindingQuery.SetAllowPartialPaths(false);
					
					const TSubclassOf<UNavigationQueryFilter> FilterClass = UNavQuery_Suspect::StaticClass();
					const FSharedConstNavQueryFilter QueryFilter = UNavigationQueryFilter::GetQueryFilter(*NavSys->MainNavData, FilterClass);
					PathFindingQuery.QueryFilter = QueryFilter;

					const FPathFindingResult PathFindingResult = NavSys->FindPathSync(PathFindingQuery);

					if (PathFindingResult.Result == ENavigationQueryResult::Success)
					{
						if (!UsedPatrolPoints.Contains(PatrolPoint) && !IsPointOnUsedPatrolPoint(FVector(PatrolPoint.Location)))
						{
							OutPatrolPoints.Add(PatrolPoint);
						}
					}
				}
			}
		}
	}
}

bool ARosterScenarioSpawner::IsPointOnUsedPatrolPoint(const FVector& Location) const
{
	for (const FPatrolPoint& Pt : UsedPatrolPoints)
	{
		if (FVector::Distance(Location, FVector(Pt.Location)) < 200.0f)
			return true;
	}
	
	return false;
}

ADoor* ARosterScenarioSpawner::GetClosestDoor(const FVector& Location) const
{
	float ClosestDist = BIG_DIST;
	ADoor* ClosestDoor = nullptr;
	for (TActorIterator<ADoor>It(GetWorld()); It; ++It)
	{
		ADoor* Door = *It;
		float Dist = (Location - Door->GetActorLocation()).Size();
		FHitResult Hit;
		GetWorld()->LineTraceSingleByObjectType(Hit, Location, Door->GetDoorway()->GetComponentLocation(), FCollisionObjectQueryParams(ECC_WorldStatic));
		if (!Hit.bBlockingHit)
		{
			if (Dist < ClosestDist)
			{
				ClosestDist = Dist;
				ClosestDoor = Door;
			}
		}

	}

	// just get any door
	if (!ClosestDoor)
	{
		for (TActorIterator<ADoor>It(GetWorld()); It; ++It)
		{
			ADoor* Door = *It;
			float Dist = (Location - Door->GetActorLocation()).Size();
			FHitResult Hit;
			GetWorld()->LineTraceSingleByObjectType(Hit, Location, Door->GetDoorway()->GetComponentLocation(), FCollisionObjectQueryParams(ECC_WorldStatic));
			if (Dist < ClosestDist)
			{
				ClosestDist = Dist;
				ClosestDoor = Door;
			}

		}
	}

	return ClosestDoor;
}

void ARosterScenarioSpawner::DoSpawn(AAISpawn* Spawn)
{
	if (!Spawn->bEnabled)
		return;
	
	if (Spawn->GetSpawningTeamType(Spawn->SpawnData) == ETeamType::TT_SUSPECT)
	{
		TArray<FPatrolPoint> OutPatrolPoints;
		GetAllReachablePatrolPoints(Spawn->GetActorLocation(), OutPatrolPoints);

		// Only patrol if we don't already have patrols defined
		if (Spawn->SpawnData.ActivityRouteCollection.ActivityRoutes.Num() == 0)
		{
			// Randomize patrol points
			Algo::RandomShuffle(OutPatrolPoints);
		
			// Regular patrols
			{
				constexpr uint8 MaxPatrols = 3;
				for (int32 i = 0; i < OutPatrolPoints.Num() && i < MaxPatrols; i++)
				{
					FActivityRoute Route;

					FTransform SpawnTransform;
					SpawnTransform.SetLocation(FVector(OutPatrolPoints[i].Location));
					Route.WorldBuildingPlacementActor = GetWorld()->SpawnActorDeferred<AWorldBuildingPlacementActor>(AWorldBuildingPlacementActor::StaticClass(), SpawnTransform);
					TArray<TSubclassOf<UWorldBuildingActivity>> PossibleWorldBuildingActivities = {UInvestigateActivity::StaticClass()};
					if (UBpGameplayHelperLib::GetRoNData() && UBpGameplayHelperLib::GetRoNData()->RandomWorldBuildingActivities.Num() > 0)
					{
						PossibleWorldBuildingActivities = UBpGameplayHelperLib::GetRoNData()->RandomWorldBuildingActivities;
					}
					Route.WorldBuildingPlacementActor->Activity = PossibleWorldBuildingActivities[FMath::RandRange(0, PossibleWorldBuildingActivities.Num() - 1)];
					Route.TimeDoingActivity = 10.0f;
					Route.bMoveOnly = true;
					Route.WorldBuildingPlacementActor->FinishSpawning(SpawnTransform);
				
					if (const ADoor* Door = GetClosestDoor(FVector(OutPatrolPoints[i].Location)))
					{
						Route.WorldBuildingPlacementActor->SetActorRotation(UKismetMathLibrary::FindLookAtRotation(FVector(OutPatrolPoints[i].Location), Door->GetActorLocation()));
					}

					Spawn->SpawnData.ActivityRouteCollection.ActivityRoutes.Add(Route);
				
					UsedPatrolPoints.Add(OutPatrolPoints[i]);
				
					V_LOGM(LogReadyOrNot, "Added Patrol Route at %s to %s", *OutPatrolPoints[i].Location.ToString(), *Spawn->GetName());
				}
			}
		}


		// Traps
		{
			// Count existing trap doors so we don't exceed max traps
			uint16 ExistingTraps = 0;
			for (TActorIterator<ADoor> It(GetWorld()); It; ++It)
			{
				const ADoor* Door = *It;
				
				if (Door->GetAttachedTrap())
					ExistingTraps++;
			}
			
			// Max placable traps is limited by MaxTrapsPlaceable but also by MaxTraps, subtracting existing traps
			const int32 MaxPlaceableTraps = FMath::Min(AI_CONFIG_GET_INT("MaxTrapsPlaceable"), AI_CONFIG_GET_INT("MaxTraps") - ExistingTraps);
			const int32 MaxTrapsSuspectCanPlace = AI_CONFIG_GET_INT("MaxTrapsSuspectCanPlace");

			int32 TrapsThisSuspect = 0;

			// Use the reachable/roamer points but only keep the ones that are marked with our door tag
			TArray<FPatrolPoint> TrapPlacementPatrolPoints = OutPatrolPoints;
			TrapPlacementPatrolPoints.RemoveAll([](const FPatrolPoint& p)
			{
				return !p.Tags.Contains("DoorPatrolPoint");
			});
			
			uint16 TrapOrders = 0;
			
			for (FPatrolPoint& Point : TrapPlacementPatrolPoints)
			{
				// Break out early if we've exceeded max traps or max traps per suspect
				if (TrapOrders >= MaxPlaceableTraps || TrapsThisSuspect >= MaxTrapsSuspectCanPlace)
					break;

				ADoor* Door = GetClosestDoor(FVector(Point.Location));
				if (!Door || !Door->CanSpawnTrap() || UsedTrapDoors.Contains(Door))
					continue;

				// Don't attempt to place a trap if this point isn't on the right side of the door
				if ((Door->TrapSide == EDoorTrapSide::Front && !Door->IsPointInFrontOfDoorway(FVector(Point.Location)) ||
					(Door->TrapSide == EDoorTrapSide::Back && Door->IsPointInFrontOfDoorway(FVector(Point.Location)))))
					continue;

				const float MaxZ = FMath::Max(Spawn->GetActorLocation().Z, Door->GetActorLocation().Z);
				const float MinZ = FMath::Min(Spawn->GetActorLocation().Z, Door->GetActorLocation().Z);

				const float ZHeightDifference = MaxZ - MinZ;

				// If within range? not too high, not too low
				if (ZHeightDifference < 150.0f)
				{
					FActivityRoute Route;

					const TArray<FString> TrapTypes = AI_CONFIG_GET_STRING_ARRAY_SINGLE_LINE("TrapType");

					// Spawn the world building placement activity and set it to use the PlaceTrapActivity with our data
					FTransform SpawnTransform;
					SpawnTransform.SetLocation(FVector(Point.Location));
					
					Route.WorldBuildingPlacementActor = GetWorld()->SpawnActorDeferred<AWorldBuildingPlacementActor>(AWorldBuildingPlacementActor::StaticClass(), SpawnTransform);
					Route.WorldBuildingPlacementActor->Activity = UPlaceTrapActivity::StaticClass();
					Route.WorldBuildingPlacementActor->GenerateActivities();
					Route.WorldBuildingPlacementActor->FinishSpawning(SpawnTransform);
					
					if (UPlaceTrapActivity* PlaceTrapActivity = Cast<UPlaceTrapActivity>(Route.WorldBuildingPlacementActor->ActivityInstance))
					{
						PlaceTrapActivity->Door = Door;
						PlaceTrapActivity->TrapType = *TrapTypes[FMath::RandRange(0, TrapTypes.Num() - 1)];
					}

					// Kind of a hack using the mirror point to get center of door
					FVector DoorCenter = Door->GetFrontMirrorPoint() ? Door->GetFrontMirrorPoint()->GetComponentLocation() : Door->GetActorLocation();
					Route.WorldBuildingPlacementActor->SetActorRotation(UKismetMathLibrary::FindLookAtRotation(FVector(Point.Location), DoorCenter));

					Route.TimeDoingActivity = 10.0f;
					Spawn->SpawnData.ActivityRouteCollection.ActivityRoutes.Add(Route);

					TrapsThisSuspect++;
				}

				UsedTrapDoors.Add(Door);
				UsedPatrolPoints.Add(Point);

				TrapOrders++;

				V_LOGM(LogReadyOrNot, "Added Trap Placement Point at %s to %s", *Point.Location.ToString(), *Spawn->GetName());
			}
		}
	}

	if (Spawn->DoSpawn())
	{
		#if WITH_EDITOR
		ULog::Info(Spawn->GetName() + ": AI Spawned from roster scenario -> " + Spawn->SpawnedCharacter->GetName() + " at " + Spawn->SpawnedCharacter->GetActorLocation().ToCompactString());
		#endif
	}
}
