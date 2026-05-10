// Void Interactive, 2020



#include "SwatAutomationManager.h"

#include "NavigationSystem.h"
#include "Activities/Team/TeamBreachAndClearActivity.h"
#include "Actors/Door.h"
#include "Characters/CyberneticController.h"
#include "Characters/AI/SWATCharacter.h"
#include "SWATManager.h"
#include "Activities/Team/DoorInteractionActivity.h"

ASwatAutomationManager::ASwatAutomationManager()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ASwatAutomationManager::StartAutomation()
{
	bRunningAutomation = true;
	TArray<AActor*> OutActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ADoor::StaticClass(), OutActors);
	for (AActor* a : OutActors)
	{
		Doors.AddUnique(Cast<ADoor>(a));
	}
}

void ASwatAutomationManager::StopAutomation()
{
	bRunningAutomation = false;
}

void ASwatAutomationManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bRunningAutomation)
	{
		if (IsSquadReadyForNextCommand())
		{
			FVector OutCommandLocation;
			CurrentDoor = FindClosestPathableDoor(OutCommandLocation);
			if (CurrentDoor)
			{
				if (CurrentDoor->IsLocked())
				{
					USWATManager::Get(this)->GivePickLockCommand(CurrentDoor, ETeamType::TT_SQUAD, OutCommandLocation);
				}
				else
				{
					USWATManager::Get(this)->GiveBreachAndClearCommand(CurrentDoor, EDoorBreachType::Open, ETeamType::TT_SQUAD, OutCommandLocation);
					//SwatManager->GiveOpenAndClearCommand(CurrentDoor, ETeamType::TT_SQUAD, OutCommandLocation, (OutCommandLocation - FVector(CurrentDoor->GetActorLocation().X, CurrentDoor->GetActorLocation().Y, OutCommandLocation.Z)).GetSafeNormal());
					BreachedDoors.Add(CurrentDoor);
					BreachedDoors.Add(CurrentDoor->GetSubDoor());
				}
			}
		} else
		{
			ASWATCharacter* SquadMember = GetSquadMember();
			if (CurrentDoor && SquadMember)
			{
				DrawDebugLine(GetWorld(), SquadMember->GetActorLocation(), CurrentDoor->GetDoorway()->GetComponentLocation(), FColor::Green, false, 0.1f, 0, 2);
				DrawDebugBox(GetWorld(), CurrentDoor->GetDoorway()->GetComponentLocation(), FVector(100.0f), FColor::Green, false, 0.1f, 0, 2);
			}
		}
	}	
}

ADoor* ASwatAutomationManager::FindClosestPathableDoor(FVector& CommandLocation)
{
	ASWATCharacter* SquadMember = GetSquadMember();
	if (!SquadMember || !SquadMember->GetCyberneticsController())
		return nullptr;
	
	FVector SwatLocation = SquadMember->GetActorLocation();
	Doors.Sort([SwatLocation](const ADoor& Lhs, const ADoor& Rhs)
	{
		return (Lhs.GetActorLocation() - SwatLocation).Size() < (Rhs.GetActorLocation() - SwatLocation).Size();
	});

	// Test Front Side
	for (ADoor* Door : Doors)
	{
		if (BreachedDoors.Contains(Door))
			continue;

		if (Door->GetRootComponent()->ComponentTags.Contains("SkipAutoBreach"))
			continue;
		
		if (UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
		{
			FPathFindingQuery PathFindingQuery;
			FNavLocation NavSwatLocation, NavDoorLocation;
			NavSys->ProjectPointToNavigation(SwatLocation + FVector(0.0f, 0.0f, -70.0f), NavSwatLocation, FVector(50.0f, 50.0f, 50.0f));
			NavSys->ProjectPointToNavigation(Door->GetDoorway()->GetComponentLocation() + FVector(0.0f, 0.0f, -70.0f) + Door->GetActorForwardVector() * 100.0f, NavDoorLocation, FVector(50.0f, 50.0f, 50.0f));
			PathFindingQuery.StartLocation = NavSwatLocation.Location == FVector::ZeroVector ? SwatLocation : NavSwatLocation;
			PathFindingQuery.EndLocation = NavDoorLocation.Location == FVector::ZeroVector ? Door->GetDoorway()->GetComponentLocation() + FVector(0.0f, 0.0f, -70.0f) + Door->GetActorForwardVector() * 100.0f : NavDoorLocation.Location;
			PathFindingQuery.SetAllowPartialPaths(false);
			FSharedConstNavQueryFilter QueryFilter = UNavigationQueryFilter::GetQueryFilter(*NavSys->MainNavData, SquadMember->GetCyberneticsController()->GetNavQueryFilter());
			PathFindingQuery.QueryFilter = QueryFilter;
			FPathFindingResult PathFindingResult = NavSys->FindPathSync(PathFindingQuery, EPathFindingMode::Regular);
			if (PathFindingResult.Result == ENavigationQueryResult::Success)
			{
				CommandLocation = NavDoorLocation.Location;
				DrawDebugLine(GetWorld(), PathFindingQuery.StartLocation, PathFindingQuery.EndLocation, FColor::Green, false, 5.0f);
				return Door;
			} else
			{
				DrawDebugLine(GetWorld(), PathFindingQuery.StartLocation, PathFindingQuery.EndLocation, FColor::Red, false, 5.0f);
			}
		}
	}

	// Test Back Side
	for (ADoor* Door : Doors)
	{
		if (BreachedDoors.Contains(Door))
			continue;
		
		if (UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
		{
			FPathFindingQuery PathFindingQuery;
			FNavLocation NavSwatLocation, NavDoorLocation;
			NavSys->ProjectPointToNavigation(SwatLocation + FVector(0.0f, 0.0f, -70.0f), NavSwatLocation, FVector(50.0f, 50.0f, 50.0f));
			NavSys->ProjectPointToNavigation(Door->GetDoorway()->GetComponentLocation() + FVector(0.0f, 0.0f, -70.0f) + Door->GetActorForwardVector() * -100.0f, NavDoorLocation, FVector(50.0f, 50.0f, 50.0f));
			PathFindingQuery.StartLocation = NavSwatLocation.Location == FVector::ZeroVector ? SwatLocation : NavSwatLocation;
			PathFindingQuery.EndLocation  = NavDoorLocation.Location == FVector::ZeroVector ? Door->GetDoorway()->GetComponentLocation() + FVector(0.0f, 0.0f, -70.0f) + Door->GetActorForwardVector() * 100.0f : NavDoorLocation.Location;
			PathFindingQuery.SetAllowPartialPaths(false);
			FSharedConstNavQueryFilter QueryFilter = UNavigationQueryFilter::GetQueryFilter(*NavSys->MainNavData, SquadMember->GetCyberneticsController()->GetNavQueryFilter());
			PathFindingQuery.QueryFilter = QueryFilter;
			FPathFindingResult PathFindingResult = NavSys->FindPathSync(PathFindingQuery, EPathFindingMode::Regular);
			if (PathFindingResult.Result == ENavigationQueryResult::Success)
			{
				CommandLocation = NavDoorLocation.Location;
				DrawDebugLine(GetWorld(), PathFindingQuery.StartLocation, PathFindingQuery.EndLocation, FColor::Green, false, 5.0f);
				return Door;
			} else
			{
				DrawDebugLine(GetWorld(), PathFindingQuery.StartLocation, PathFindingQuery.EndLocation, FColor::Red, false, 5.0f);
			}
		}
	}
	
	return nullptr;
}

ASWATCharacter* ASwatAutomationManager::GetSquadMember()
{
	return *TActorIterator<ASWATCharacter>(GetWorld());
}

bool ASwatAutomationManager::IsSquadReadyForNextCommand()
{
	for (TActorIterator<ACyberneticController>It(GetWorld()); It; ++It)
	{
		ACyberneticController* Controller = *It;
		if (Controller->IsSWAT())
		{
			if (Controller->IsMoving())
			{
				return false;
			}

			if (Controller->GetCurrentActivity<UTeamBreachAndClearActivity>() && Controller->GetCurrentActivity<UTeamBreachAndClearActivity>()->SharedData->CommandLocation ==  FVector::ZeroVector)
			{
				return true;
			}
			
			if (Controller->GetCurrentActivity<UDoorInteractionActivity>())
			{
				return !Controller->GetCurrentActivity<UDoorInteractionActivity>()->Door->IsLocked();
			}
			
			if (Controller->GetCurrentActivity<UTeamBreachAndClearActivity>()
				&& !Controller->GetCurrentActivity<UTeamBreachAndClearActivity>()->HasBreached())
			{
				return false;
			}
		}
	}
	return true;
}
