// Void Interactive, 2020

#include "MoveToPlayerSpawn.h"

#include "NavigationSystem.h"
#include "ReadyOrNotGameMode.h"
#include "Characters/CyberneticCharacter.h"

void UMoveToPlayerSpawn::StartActivity(AAIController* Owner)
{
	Super::StartActivity(Owner);

	GetCharacter()->ReasonsToSprint.AddUnique("moving to player spawn");
}

void UMoveToPlayerSpawn::FinishedActivity(const bool bSuccess)
{
	Super::FinishedActivity(bSuccess);

	GetCharacter()->ReasonsToSprint.Remove("moving to player spawn");
}

void UMoveToPlayerSpawn::RequestCombatMove(const float DeltaTime)
{
	Super::RequestCombatMove(DeltaTime);

	if (UReadyOrNotStatics::GetReadyOrNotGameMode())
	{
		FVector SpawnLoc = UReadyOrNotStatics::GetReadyOrNotGameMode()->LastPlayerSpawnPoint.GetLocation();
		if (Location == FVector::ZeroVector)
		{
			if (UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
			{
				FNavLocation NavLocation;
				NavSys->GetRandomReachablePointInRadius(SpawnLoc, 650.0f, NavLocation);
				Location = NavLocation.Location;
			}
		}
	}

	if (HasReachedLocation() && bBeArrestedOnceReachedLocation && !GetCharacter()->IsArrested())
	{
		for (TActorIterator<AZipcuffs>It(GetWorld()); It; ++It)
		{
			GetCharacter()->ArrestComplete(It->GetOwnerCharacter(), *It);
			if (It->StandingArrestInteractionCivilians.Num() > 0)
			{
				GetCharacter()->Play3PMontage(It->StandingArrestInteractionCivilians[0]->SlaveMontage);
			}
			GetCharacter()->ReportToTOC_Implementation(It->GetOwnerCharacter(), false);
			break;
		}
	}
}

void UMoveToPlayerSpawn::OnPathFound(uint32 PathId, ENavigationQueryResult::Type ResultType, FNavPathSharedPtr NavPath)
{
	Super::OnPathFound(PathId, ResultType, NavPath);
	
	if (ResultType != ENavigationQueryResult::Success)
	{
		Location = FVector::ZeroVector;
	}
}
