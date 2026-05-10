// Void Interactive, 2020

#include "CivilianFleeCombatMove.h"

#include "Characters/CyberneticCharacter.h"
#include "Characters/CyberneticController.h"

#include "Info/Activities/WorldBuildingActivity.h"

FName UCivilianFleeCombatMove::GetMoveStyleOverride_Implementation() const
{
	return "male01_civilian_cowering";
}

void UCivilianFleeCombatMove::RequestCombatMove(const float DeltaTime)
{
	if (!GetCharacter())
		return;

	GetCharacter()->bIsFleeing = true;

	//if (OwningController->GetActivity<UTakeCoverActivity>())
	//	return;

	/*
	if (OwningController->HasBeenExposedToAggressiveNoise(1.0f, 1500.0f) && !OwningController->HasActivityType(UWorldBuildingActivity::StaticClass()))
	{
		bTryFindHidingSpot = true;
	}

	if (!OwningController->HasActivityType(UWorldBuildingActivity::StaticClass()))
	{
		if (bTryFindHidingSpot)
		{
			if (TryFindNearestHidingSpot())
			{
				bFoundHidingSpot = true;
			}
		}
	
		PlayAISpeech(VO_SUSPECTS_AND_CIVILIAN::BARK_COVER_TRANSITION, true, 5.0f);
	}
	*/
}

bool UCivilianFleeCombatMove::TryFindNearestHidingSpot()
{
	if (!GetCharacter())
		return false;

	//V_LOGM(LogReadyOrNot, "%s has %d Hiding Spots", *GetCharacter()->GetName(), CachedHidingSpots.Num());

	/*
	if (CachedHidingSpots.Num() > 0)
	{
		FVector EnemyLoc = OwningController->GetTargetingComp()->GetLastKnownEnemyPosition();
		CachedHidingSpots.Sort([EnemyLoc](const FVector& Lhs, const FVector& Rhs)
		{
			return (Lhs - EnemyLoc).Size() > (Rhs - EnemyLoc).Size();
		});
		const FVector HidingSpotLocation = CachedHidingSpots[0];
		CachedHidingSpots.RemoveAt(0);

		
		const FVector Start = OwningController->GetTargetingComp()->GetLastKnownEnemyPosition() + UKismetMathLibrary::FindLookAtRotation(OwningController->GetTargetingComp()->GetLastKnownEnemyPosition(), HidingSpotLocation).Vector() * 200.0f;
		const FVector End = HidingSpotLocation + FVector(0.0f, 0.0f, 70.0f);
		FHitResult Hit;
		GetWorld()->LineTraceSingleByObjectType(Hit, Start, End, FCollisionObjectQueryParams(ECC_WorldStatic));
		if (Hit.bBlockingHit)
		{
			if (!HasAnyOtherCombatMoveGotLocation(HidingSpotLocation))
			{
				Location = HidingSpotLocation;
			
				return true;
			}
		}
	}
	else
	{
		// CachedHidingSpots = GetCharacter()->GetSpawnData()->HidingSpots;
		// if (CachedHidingSpots.Num() == 0)
		// {
		// 	OwningController->GetCombatActivity<UBaseCombatActivity>()->StartRunningCombatMove(USupressionCombatMove::StaticClass());
		// }
	}	
	*/
	
	return false;
}

void UCivilianFleeCombatMove::OnPathFound(uint32 PathId, ENavigationQueryResult::Type ResultType, FNavPathSharedPtr NavPath)
{
	Super::OnPathFound(PathId, ResultType, NavPath);
	/*
	bSearchingPath = false;
	
	// Character may have been deleted between starting the path and the path being found
	if (!OwningController)
		return;

	if (!GetCharacter())
		return;

	if (GetCharacter()->IsMovementLocked())
		return;
	 
	//Super::OnPathFound(PathId, ResultType, NavPath);
	
	if (ResultType != ENavigationQueryResult::Success)
	{
		TryFindNearestHidingSpot();
	}
	else
	{
		if (!GetCharacter()->IsInRagdoll() && !GetCharacter()->IsArrestedOrSurrendered() && !GetCharacter()->IsStunned() && !GetCharacter()->IsDeadOrUnconscious())
		{
			//PlayMontageFromTableSync("tp_civ_cower");

			// pick a random one for now
			if (GetCharacter()->CivilianCowerActivities.Num() != 0)
			{
				const int32 CurrentSelection = FMath::RandRange(0, GetCharacter()->CivilianCowerActivities.Num() - 1);

				if (GetCharacter()->CivilianCowerActivities.IsValidIndex(CurrentSelection) && !OwningController->HasActivityType(UWorldBuildingActivity::StaticClass()))
				{
					if (const TSubclassOf<UWorldBuildingActivity> ActivityClass = GetCharacter()->CivilianCowerActivities[CurrentSelection])
					{
						UWorldBuildingActivity* ActivityInst = NewObject<UWorldBuildingActivity>(OwningController, ActivityClass);
						ActivityInst->SetLocation(Location);
						ActivityInst->SetRotation(UKismetMathLibrary::FindLookAtRotation(Location, GetCharacter()->GetActorLocation()));
						ActivityInst->MaxActivityTime = GetCharacter()->CivilianCowerActivityDuration; // the cowering activities should loop instead of this until the civilian gets interrupted by something??
						//ActivityInst->bAbortIfTrackingEnemy = false;
						ActivityInst->bAbortDueToAggressiveNoise = false;
						ActivityInst->OverrideDestinationTolerance = -250.0f;
						ActivityInst->bRequireRotationMatch = false;
						
						OwningController->AddActivity(ActivityInst);
						
						bFoundHidingSpot = false;
						bTryFindHidingSpot = false;
					}
				}
			}
		}
	}
	*/
}
