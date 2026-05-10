// Void Interactive, 2020

#include "DuelingCombatMove.h"

#include "Characters/CyberneticCharacter.h"
#include "Characters/CyberneticController.h"

#include "Info/Activities/BaseCombatActivity.h"

#include "NavigationSystem.h"
#include "Animation/MoveStyle/RoNMoveStyleComponent.h"

UDuelingCombatMove::UDuelingCombatMove()
{
	bAbortMoveWhenActivityFinished = false;
	bRequireMagazineWeapon = true;
}

void UDuelingCombatMove::StartActivity(AAIController* Owner)
{
	Super::StartActivity(Owner);

	GetCharacter()->ReasonsToSprint.AddUnique("dueling");
}

void UDuelingCombatMove::FinishedActivity(bool bSuccess)
{
	Super::FinishedActivity(bSuccess);
	
	GetCharacter()->ReasonsToSprint.Remove("dueling");
}

void UDuelingCombatMove::RequestCombatMove(const float DeltaTime)
{
	Super::RequestCombatMove(DeltaTime);

	const ABaseMagazineWeapon* EquippedWeapon = GetCharacter()->GetEquippedWeapon();
	if (!EquippedWeapon)
	{
		#if !UE_BUILD_SHIPPING
		UnableToCombatReason = "No equipped weapon";
		#endif
		
		FinishCombatMove(false);
		return;
	}

	if (!EquippedWeapon->HasAnyAmmo())
	{
		#if !UE_BUILD_SHIPPING
		UnableToCombatReason = "No ammo left";
		#endif
		
		FinishCombatMove(false);
		return;
	}
	
	// Must be strafing to duel
	const bool bIsMoveStyleStrafe = GetCharacter()->IsStrafing();
	if (!bIsMoveStyleStrafe)
	{
		#if !UE_BUILD_SHIPPING
		UnableToCombatReason = "Current move style (" + GetCharacter()->MoveStyle->ActiveMoveStyle.Name.ToString() + ") is not a strafe move style";
		#endif
		
		FinishCombatMove(false);
		return;
	}

	const float TimeSinceLastSeenEnemy = OwningController->GetTargetingComp()->GetTimeSinceLastSeenEnemy();
	//const float TimeTrackingTarget = OwningController->GetTargetingComp()->GetTimeTrackingTarget();
	const FVector LastKnownEnemyPosition = OwningController->GetTargetingComp()->GetLastKnownEnemyPosition();

	if (TimeSinceLastSeenEnemy > 9.0f && LastKnownEnemyPosition != FVector::ZeroVector)
	{
		#if !UE_BUILD_SHIPPING
		UnableToCombatReason = "Time since last seen enemy > 9 seconds";
		#endif
		
		StoredLocations.Empty();
		bInvertedVelocity = !bInvertedVelocity;
		FinishCombatMove(false);
		return;
	}

	const AReadyOrNotCharacter* TrackedTarget = OwningController->GetTrackedTarget();
	if (!TrackedTarget)
	{
		#if !UE_BUILD_SHIPPING
		UnableToCombatReason = "Not tracking a target";
		#endif
		
		FinishCombatMove(false);
		return;
	}
	
	#if !UE_BUILD_SHIPPING
	UnableToCombatReason = "";
	#endif

	if (TimeUntilNextVelocityUpdate <= 0.0f)
	{
		LastVelocityInput = TrackedTarget->GetVelocity();
		TimeUntilNextVelocityUpdate = 1.0f;
		VelocityInputScale = 0.0f;
	}
	else
	{
		TimeUntilNextVelocityUpdate -= DeltaTime;
		VelocityInputScale += 0.5f * DeltaTime;
	}

	if (CanFollowTargetVelocity())
	{
		FHitResult Hit;
		GetWorld()->LineTraceSingleByObjectType(Hit, GetCharacter()->GetActorLocation(), GetCharacter()->GetActorLocation() + LastVelocityInput * (bInvertedVelocity ? -1.0f : 1.0f) * 1.0f, FCollisionObjectQueryParams(ECC_WorldStatic));

		#if !UE_BUILD_SHIPPING
		DrawDebugLine(GetWorld(), GetCharacter()->GetActorLocation(), GetCharacter()->GetActorLocation() + LastVelocityInput * (bInvertedVelocity ? -1.0f : 1.0f) * 1.0f, Hit.IsValidBlockingHit() ? FColor::Red : FColor::White, false, OwningController->GetActorTickInterval(), 0, 2.0f);
		#endif
		
		if (!Hit.IsValidBlockingHit())
		{
			if (const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
			{
				FNavLocation OutLocation;
				if (NavSys->ProjectPointToNavigation(Hit.TraceEnd, OutLocation, FVector(10.0f, 10.0f, 200.0f)))
				{
					if (FVector::Distance(GetCharacter()->GetActorLocation(), TrackedTarget->GetActorLocation()) < 500.0f)
					{
						FVector Direction = -(TrackedTarget->GetActorLocation() - GetCharacter()->GetActorLocation()).GetSafeNormal();
						SetLocation(GetCharacter()->GetNavAgentLocation() + Direction * 200.0f);
					}
					else
					{
						FVector Direction = LastVelocityInput.GetSafeNormal() * (bInvertedVelocity ? -1.0f : 1.0f);
						SetLocation(GetCharacter()->GetNavAgentLocation() + Direction * 200.0f);
					}

					return;
				}
			}
		}
	}

	TimeUntilNextMove -= DeltaTime;
	if (TimeUntilNextMove <= 0.0f || StoredLocations.Num() == 0)
	{
		StoredLocations.Empty();
		TimeUntilNextMove = 6.0f;
		
		const float DistToEnemy = (LastKnownEnemyPosition - GetCharacter()->GetActorLocation()).Size();

		GenerateNavigablePoints(LastKnownEnemyPosition, FNavGenerationParameters(ENavType::NT_Circle, ENavLOS::NL_Any, FMath::Clamp(DistToEnemy + 100.0f, 500.0f, 2000.0f), 0.0f), StoredLocations);
	}

	if (!OwningController->IsMoving())
	{
		Location = FetchNextDuelLocation();
	}
}

float UDuelingCombatMove::GetDestinationTolerance() const
{
	return 50.0f;
}

void UDuelingCombatMove::ResetData()
{
	Super::ResetData();
	
	TimeUntilNextMove = 0.0f;
	TimeUntilNextVelocityUpdate = 0.0f;
	
	bInvertedVelocity = false;
	bMovingUp = false;

	LastVelocityInput = FVector::ZeroVector;

	StoredLocations.Empty();
}

void UDuelingCombatMove::OnPathFound(uint32 PathId, ENavigationQueryResult::Type ResultType, FNavPathSharedPtr NavPath)
{
	if (!OwningController)
		return;
	
	if (ResultType == ENavigationQueryResult::Success)
	{
		const TArray<FNavPathPoint> PathPoints = NavPath->GetPathPoints();
		if (PathPoints.Num() > 2)
		{
			FVector PathPoint1 = PathPoints[1];
			
			// Don't allow paths that go towards instigator within their danger zone, too risky to traverse
			bool bAnyPathPointGoesTowardsInstigator = false;

			{
				// Is this path point inside of the instigator's danger radius
				const FVector DirectionToPathPoint = (PathPoint1 - GetCharacter()->GetActorLocation()).GetSafeNormal2D();
				const FVector DirectionToInstigator = (OwningController->GetTargetingComp()->GetLastKnownEnemyPosition() - GetCharacter()->GetActorLocation()).GetSafeNormal2D();
					
				float PathPointDotProduct = FVector::DotProduct(DirectionToPathPoint, DirectionToInstigator);
				if (PathPointDotProduct > 0.8f)
				{
					bAnyPathPointGoesTowardsInstigator = true;
				}
			}

			if (bAnyPathPointGoesTowardsInstigator)
			{
				/*
				for (int32 i = 1; i < PathPoints.Num(); i++)
				{
					DrawDebugLine(GetWorld(), PathPoints[i-1].Location + FVector(0.0f, 0.0f, 30.0f), PathPoints[i].Location + FVector(0.0f, 0.0f, 30.0f), FColor::Orange, false, 1.0f, 0, 1.25f);
				}*/

				ULog::Warning("Duel path goes towards enemy");
				return;
			}
		}
	}
	
	Super::OnPathFound(PathId, ResultType, NavPath);
}

FVector UDuelingCombatMove::FetchNextDuelLocation()
{
	FVector SortLocation = GetCharacter()->GetActorLocation();
	StoredLocations.Sort([SortLocation](const FVector& Lhs, const FVector& Rhs)
	{
		return (Lhs - SortLocation).Size() < (Rhs - SortLocation).Size();
	});

	// only select locations that we can see the enemy from
	StoredLocations.RemoveAll([&](const FVector& Element)
	{
		FVector EyesViewPoint;
		FRotator EyesViewDirection;
		GetCharacter()->GetActorEyesViewPoint(EyesViewPoint, EyesViewDirection);
		
		FCollisionQueryParams CollisionParams = UReadyOrNotFunctionLibrary::CreateCollisionQueryParams(GetCharacter(), OwningController->GetTargetingComp()->GetLastKnownEnemy());
		CollisionParams.AddIgnoredActor(OwningController->GetTrackedTarget());
		CollisionParams.AddIgnoredActor(OwningController->GetLastTrackedEnemy());
		FVector ViewPoint = Element;
		ViewPoint.Z = EyesViewPoint.Z;
		
		const bool bHit = GetWorld()->LineTraceTestByChannel(ViewPoint, OwningController->GetTargetingComp()->GetLastKnownEnemyPosition(), ECC_Visibility, CollisionParams);
		
		//DrawDebugLine(GetWorld(), ViewPoint, OwningController->GetTargetingComp()->GetLastKnownEnemyPosition(), LOSHit.bBlockingHit ? FColor::Red : FColor::Green, false, 1.0f);
		return bHit;
	});

	if (StoredLocations.Num() > 0)
	{
		if (StoredLocations.Num() > 1)
		{
			const int32 RandMovePt = FMath::RandRange(1, 2);
			if (StoredLocations.Num() > RandMovePt)
			{
				PlayAISpeech(VO_SUSPECTS_AND_CIVILIAN::DUELING, true, 5.0f);
				
				return StoredLocations[RandMovePt];
			}
		}
		else
		{
			PlayAISpeech(VO_SUSPECTS_AND_CIVILIAN::DUELING, true, 5.0f);
			
			return StoredLocations[0];
		}
	}
	
	return Location;
}

bool UDuelingCombatMove::CanFollowTargetVelocity() const
{
	// Don't follow player velocity when reloading
	if (OwningCombatActivity->IsReloading())
		return false;

	return LastVelocityInput.Size() > 1.0f;
}
