// Void Interactive, 2020

#include "BaseCombatMoveActivity.h"

#include "Characters/CyberneticController.h"
#include "Characters/CyberneticCharacter.h"

#include "Info/Activities/BaseCombatActivity.h"

#include "NavigationSystem.h"

UBaseCombatMoveActivity::UBaseCombatMoveActivity()
{
	ActivityName = FText::FromString(GetName());
	
	bIsProgressActivity = false;
	bAbortMoveWhenActivityFinished = true;
	bAbortIfNotMovingForAWhile = false;
	bFinishActivityWhenOverriden = true;
	bHasLimitSpeechLimit = true;
	SpeechLimit = 1;
}

void UBaseCombatMoveActivity::StartActivity(AAIController* Owner)
{
	Super::StartActivity(Owner);
	
	#if !UE_BUILD_SHIPPING
	UnableToCombatReason = "";
	#endif

	if (bRequireMagazineWeapon)
	{
		if (!GetCharacter()->GetEquippedWeapon())
		{
			#if !UE_BUILD_SHIPPING
			UnableToCombatReason = "No equipped weapon";
			#endif
			
			FinishCombatMove();
			return;
		}
	}
}

void UBaseCombatMoveActivity::OnMoveInterrupted(UBaseActivity* Activity)
{
	InterruptActivity = nullptr;
	
	if (Activity)
	{
		InterruptActivity = Activity;
		
		LocationBeforeInterrupt = Location;
		Location = FVector::ZeroVector;

		#if !UE_BUILD_SHIPPING
		ULog::Info(GetName() + " interrupted by " + Activity->GetName());
		#endif
	}
}

void UBaseCombatMoveActivity::OnMoveResumed()
{
	if (InterruptActivity)
	{
		ULog::Info("Interrupt activity " + InterruptActivity->GetName() + " finished. Resuming " + GetName());
		
		InterruptActivity = nullptr;
	}
}

void UBaseCombatMoveActivity::GenerateNavigablePoints(const FVector& GenAroundLoc, const FNavGenerationParameters& NavGenerationParameters, TArray<FVector>& OutLocations)
{
	if (const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		switch (NavGenerationParameters.NavType)
		{
			case ENavType::NT_SinglePoint:
			{
				FNavLocation OutLocation;
				NavSys->GetRandomReachablePointInRadius(GenAroundLoc, NavGenerationParameters.Radius, OutLocation);
				OutLocations.Add(OutLocation.Location);
			}
			break;
		
			case ENavType::NT_Circle:
			{
				for (int32 i = 0; i < 36; i++)
				{
					FVector TestPt = GenAroundLoc + FRotator(0.0f, i*10, 0.0f).Vector() * NavGenerationParameters.Radius;
					if (NavGenerationParameters.MaxDistance > 0.0f && (TestPt - GetCharacter()->GetActorLocation()).Size() > NavGenerationParameters.MaxDistance)
						continue;
					
					FNavLocation OutNavLocation;
					if (NavSys->ProjectPointToNavigation(TestPt, OutNavLocation, FVector(50.0f, 50.0f, 200.0f)))
					{
						if (NavGenerationParameters.LOSType != ENavLOS::NL_Any)
						{
							if (NavGenerationParameters.LOSType == ENavLOS::NL_RequireLOSToMyself)
							{
								FHitResult Hit;
								GetWorld()->LineTraceSingleByObjectType(Hit, GetCharacter()->GetActorLocation(), OutNavLocation.Location, FCollisionObjectQueryParams(ECC_WorldStatic));
								//DrawDebugLine(GetWorld(), Hit.TraceStart, Hit.TraceEnd, Hit.bBlockingHit ? FColor::Red : FColor::White, 1.0f, 0, 1);
								if (Hit.bBlockingHit)
									continue;
							}
							else
							{
								FHitResult Hit;
								GetWorld()->LineTraceSingleByObjectType(Hit, GenAroundLoc, FVector(OutNavLocation.Location.X, OutNavLocation.Location.Y, GenAroundLoc.Z), FCollisionObjectQueryParams(ECC_WorldStatic));
								//DrawDebugLine(GetWorld(), Hit.TraceStart, Hit.TraceEnd, Hit.bBlockingHit ? FColor::Red : FColor::White, 1.0f, 0, 1);
								if ((Hit.bBlockingHit && NavGenerationParameters.LOSType == ENavLOS::NL_RequireLOS) || (!Hit.bBlockingHit && NavGenerationParameters.LOSType == ENavLOS::NL_BreakLOS))
									continue;
							}
						}
						
						OutLocations.Add(OutNavLocation.Location);
					}
				}
			}
			break;
			
			case ENavType::NT_Grid:
			{
				for (int32 i = 0; i < 10; i++)
				{
					for (int32 y = 0; y < 10; y++)
					{
						FVector TestPt = FVector(GenAroundLoc.X + i * 50.0f, GenAroundLoc.Y + y * 50.0f, GenAroundLoc.Z);
						FNavLocation OutNavLocation;
						if (NavSys->ProjectPointToNavigation(TestPt, OutNavLocation, FVector(50.0f, 50.0f, 100.0f)))
						{
							OutLocations.Add(OutNavLocation.Location);
						}
					}
				}
			}
			break;
			
			default:
			break;
		}
	}

	/*
	#if WITH_EDITOR
	for (const FVector& loc : OutLocations)
	{
		DrawDebugPoint(GetWorld(), loc, 10.0f, FColor::White, false, 10.0f, 0);
	}
	#endif
	*/
}

bool UBaseCombatMoveActivity::ShouldPerformCombatMove() const
{
	if (GetCharacter()->IsDeadOrUnconscious())
		return false;
	
	if (GetCharacter()->IsPlayingDead())
		return false;

	if (GetCharacter()->IsInRagdoll())
		return false;

	if (GetCharacter()->IsArrestedOrSurrendered())
		return false;

	if (GetCharacter()->IsStunned())
		return false;
	
	if (PathFailedCount > 25)
		return false;

	if (OwningCombatActivity)
	{
		if (OwningCombatActivity->IsTryingToFireAtScriptedActor())
			return false;
	}
	
	return true;
}

void UBaseCombatMoveActivity::FinishCombatMove(const bool bSuccess)
{
	if (OwningCombatActivity)
	{
		if (IsActive())
		{
			Location = FVector::ZeroVector;
			OwningCombatActivity->FinishCombatMove(bSuccess);
		}
	}
}

void UBaseCombatMoveActivity::PerformActivity(const float DeltaTime)
{
	#if WITH_EDITOR
	checkNoRecursion();
	#endif
	
	if (InterruptActivity)
		return;

	if (!ShouldPerformCombatMove()/* || GetCharacter()->IsStunned()*/)
	{
		OwningCombatActivity->FinishCombatMove(false);
		return;
	}

	if (bRequireMagazineWeapon)
	{
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
	}
	
	RequestCombatMove(DeltaTime);
	
	if (ActivityStatus != EActivityStatus::Complete)
		Super::PerformActivity(DeltaTime);
}

bool UBaseCombatMoveActivity::CanFinishActivity() const
{
	// Managed by the combat activity. Call FinishCombatMove to stop the combat move
	return false;
}

#if !UE_BUILD_SHIPPING
void UBaseCombatMoveActivity::GatherDebugString(FString& OutString)
{
	OutString = "\n\t" + GetName() + (InterruptActivity != nullptr ? " (Interrupted by: " + InterruptActivity->GetName() + ")" : "");

	OutString += AddDebugString("Location", Location.ToCompactString());

	FVector FocalPoint = FVector::ZeroVector;
	OverrideFocalPoint(FocalPoint);
	OutString += AddDebugString("Target Focal Point", FocalPoint.ToString());
	
	if (Location != FVector::ZeroVector)
	{
		OutString += AddDebugString("Distance To Destination", FString::SanitizeFloat(FVector::Distance(GetCharacter()->GetNavAgentLocation(), Location)));
		OutString += AddDebugString("Has Reached Destination", HasReachedLocation(GetDestinationTolerance()) ? "true" : "false");
	}
}
#endif

bool UBaseCombatMoveActivity::IsActive() const
{
	return OwningCombatActivity->GetCombatMoveActivity() == this;
}

bool UBaseCombatMoveActivity::HasAnyOtherCombatMoveGotLocation(const FVector& TestLocation, const float Tolerance) const
{
	for (const ACyberneticCharacter* AI : GetWorld()->GetGameState<AReadyOrNotGameState>()->AllAICharacters)
	{
		if (AI->GetCyberneticsController())
		{
			if (const UBaseCombatActivity* CombatActivity = AI->GetCyberneticsController()->GetCombatActivity())
			{
				if (const UBaseCombatMoveActivity* CombatMoveActivity = CombatActivity->GetCombatMoveActivity())
				{
					if (CombatMoveActivity != this)
					{
						return FVector::Distance(CombatActivity->GetLocation(), TestLocation) < Tolerance;
					}
				}
			}
		}
	}

	return false;
}

bool UBaseCombatMoveActivity::HasLOSToEnemy() const
{
	return OwningController->GetTargetingComp()->CanSeeTrackedTarget() || OwningController->GetTargetingComp()->HasLineOfSightToTrackedTarget();
}
