// Void Interactive, 2020

#include "SuppressionCombatMove.h"

#include "Characters/CyberneticCharacter.h"
#include "Characters/CyberneticController.h"

#include "Info/Activities/BaseCombatActivity.h"

#include "NavigationSystem.h"

USuppressionCombatMove::USuppressionCombatMove()
{
	bRequireMagazineWeapon = true;
}

void USuppressionCombatMove::StartActivity(AAIController* Owner)
{
	Super::StartActivity(Owner);
	
	TimeUntilNextSuppressionMove = 0.0f;

	OwningController->AbortMove();
}

bool USuppressionCombatMove::OverrideFocalPoint(FVector& FocalPoint)
{
	FVector LastKnownEnemyPosition = OwningController->GetTargetingComp()->GetLastKnownEnemyPosition();

	if (OwningController->GetTrackedTarget())
	{
		LastKnownEnemyPosition = OwningController->GetFocalPointOnActor(OwningController->GetTrackedTarget());
	}
	else
	{
		if (OwningController->GetLastTrackedEnemy())
		{
			if (OwningController->GetTargetingComp()->HasLineOfSightToLastTrackedTarget())
			{
				LastKnownEnemyPosition = OwningController->GetFocalPointOnActor(OwningController->GetLastTrackedEnemy());
			}
		}
	}

	if (LastKnownEnemyPosition != FVector::ZeroVector)
	{
		FocalPoint = LastKnownEnemyPosition;
		return true;
	}
	
	return Super::OverrideFocalPoint(FocalPoint);
}

void USuppressionCombatMove::ResetData()
{
	Super::ResetData();

	bHasBrokenLOS = false;
	TimeUntilNextSuppressionMove = 0.0f;
}

#if !UE_BUILD_SHIPPING
void USuppressionCombatMove::PerformActivity_Debug(float DeltaTime)
{
	Super::PerformActivity_Debug(DeltaTime);
	
	FVector LastKnownEnemyPosition = OwningController->GetTargetingComp()->GetLastKnownEnemyPosition();

	if (OwningController->GetTrackedTarget())
		LastKnownEnemyPosition = OwningController->GetFocalPointOnActor(OwningController->GetTrackedTarget());

	DrawDebugSphere(GetWorld(), LastKnownEnemyPosition, 20.0f, 32, FColor::Cyan, false, DeltaTime);
}

void USuppressionCombatMove::GatherDebugString(FString& OutString)
{
	Super::GatherDebugString(OutString);
	
	OutString += LINE_TERMINATOR;
	OutString += AddDebugString("Has Broken LOS", bHasBrokenLOS ? "True" : "False");
	OutString += AddDebugString("Time Until Next Suppression Move", FString::Printf(TEXT("%.2f"), TimeUntilNextSuppressionMove));
	
	FVector LastKnownEnemyPosition = OwningController->GetTargetingComp()->GetLastKnownEnemyPosition();

	if (OwningController->GetTrackedTarget())
		LastKnownEnemyPosition = OwningController->GetFocalPointOnActor(OwningController->GetTrackedTarget());
	
	OutString += LINE_TERMINATOR;
	OutString += AddDebugString("Last Known Enemy Position", LastKnownEnemyPosition.ToCompactString());

	if (bHasBrokenLOS)
	{
		OutString += LINE_TERMINATOR;
		OutString += AddDebugString("Suppressing...", FString::Printf(TEXT("%.2f"), OwningCombatActivity->CurrentScriptedFireAt.TimeRemaining));
	}
}
#endif

void USuppressionCombatMove::RequestCombatMove(const float DeltaTime)
{
	Super::RequestCombatMove(DeltaTime);

	FVector LastKnownEnemyPosition = OwningController->GetTargetingComp()->GetLastKnownEnemyPosition();

	bool bCanSeeEnemy = false;
	
	if (OwningController->GetTrackedTarget())
	{
		LastKnownEnemyPosition = OwningController->GetFocalPointOnActor(OwningController->GetTrackedTarget());
		
		bCanSeeEnemy = OwningController->GetTargetingComp()->CanSeeTrackedTarget() && OwningController->GetTargetingComp()->HasLineOfSightToTrackedTarget();
	}
	else
	{
		if (OwningController->GetLastTrackedEnemy())
		{
			if (OwningController->GetTargetingComp()->HasLineOfSightToLastTrackedTarget())
			{
				LastKnownEnemyPosition = OwningController->GetLastTrackedEnemy()->GetActorLocation();
				
				bCanSeeEnemy = OwningController->GetTargetingComp()->CanSeeLastTrackedTarget() && OwningController->GetTargetingComp()->HasLineOfSightToLastTrackedTarget();
			}
		}
	}
	
	if (LastKnownEnemyPosition == FVector::ZeroVector)
	{
		#if !UE_BUILD_SHIPPING
		UnableToCombatReason = "No last known enemy position";
		#endif
		
		FinishCombatMove(false);
		return;
	}

	if (OwningController->GetTrackedTarget())
	{
		if (OwningController->GetTrackedTarget()->IsDeadOrUnconscious())
		{
			#if !UE_BUILD_SHIPPING
			UnableToCombatReason = "Tracked target is dead";
			#endif
		
			FinishCombatMove(true);
			return;
		}
	}

	if (OwningController->GetLastTrackedEnemy())
	{
		if (OwningController->GetLastTrackedEnemy()->IsDeadOrUnconscious())
		{
			#if !UE_BUILD_SHIPPING
			UnableToCombatReason = "Previous tracked target is dead";
			#endif
			
			FinishCombatMove(true);
			return;
		}
	}

	if (!GetCharacter()->GetEquippedWeapon())
	{
		#if !UE_BUILD_SHIPPING
		UnableToCombatReason = "No magazine weapon equipped";
		#endif
		
		FinishCombatMove(false);
		return;
	}
	
	if (!GetCharacter()->GetEquippedWeapon()->HasAnyAmmo())
	{
		#if !UE_BUILD_SHIPPING
		UnableToCombatReason = "No ammo left";
		#endif
		
		FinishCombatMove(false);
		return;
	}

	const bool bHasLOS = bCanSeeEnemy;

	if (!bHasLOS && !bHasBrokenLOS)
	{
		bHasBrokenLOS = true;
		
		AbortMove();
	}
	
	if (bHasLOS)
	{
		OwningCombatActivity->StopScriptedFire();
		bHasBrokenLOS = false;
	}

	if (bHasBrokenLOS)
	{
		GetCharacter()->PlayBarkOrStartConversation(VO_SUSPECTS_AND_CIVILIAN::REPLY_SHOOTING, true, 4.0f);
	}
	
	if (!OwningController->IsMoving())
	{
		TimeUntilNextSuppressionMove -= DeltaTime;

		if (TimeUntilNextSuppressionMove <= 0.0f)
		{
			//if (const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
			{
				FNavGenerationParameters Params;
				Params.Radius = 1000.0f;
				Params.NavType = ENavType::NT_Circle;
				Params.LOSType = ENavLOS::NL_RequireLOS;
				Params.MaxDistance = -1;

				TArray<FVector> GennedPoints;
				FVector GenLocation = OwningController->GetTargetingComp()->TargetLocationHistory.Num() > 0 ? OwningController->GetTargetingComp()->TargetLocationHistory.Last() : LastKnownEnemyPosition;
				uint8 i = 0;
				while (GennedPoints.Num() == 0 && i < OwningController->GetTargetingComp()->TargetLocationHistory.Num())
				{
					GenLocation = OwningController->GetTargetingComp()->TargetLocationHistory[i];
					GenerateNavigablePoints(GenLocation, Params, GennedPoints);
					i++;
				}

				GenLocationInLOS = GenLocation;

				FVector SuppressionLocation = FVector::ZeroVector;

				TArray<FVector> GennedPoints_Copy = GennedPoints;
				// Remove all points in LOS that are behind us the last known enemy position
				GennedPoints.RemoveAll([&](const FVector& Point)
				{
					const float DotProduct = FVector::DotProduct((GetCharacter()->GetActorLocation() - GenLocation).GetSafeNormal2D(), (Point - LastKnownEnemyPosition).GetSafeNormal2D());
					return DotProduct < 0.3f;
				});
				
				if (GennedPoints.Num() == 0)
				{
					GennedPoints = GennedPoints_Copy;
				}

				GennedPoints.Remove(Location);
				
				if (GennedPoints.Num() > 0)
				{
					if (GennedPoints.Num() > 1)
					{
						FVector SortLocation = GetCharacter()->GetActorLocation();
						GennedPoints.Sort([SortLocation](const FVector& Lhs, const FVector& Rhs)
						{
							return (Lhs - SortLocation).Size() < (Rhs - SortLocation).Size();
						});
						
						const int32 RandMovePt = FMath::RandRange(1, 2);
						if (GennedPoints.Num() > RandMovePt)
						{
							SuppressionLocation = GennedPoints[RandMovePt];
						}
					}
					else
					{
						SuppressionLocation	= GennedPoints[0];
					}
				}

				if (SuppressionLocation == FVector::ZeroVector)
				{
					#if !UE_BUILD_SHIPPING
					UnableToCombatReason = "Suppression Location was (0,0,0)";
					#endif
					
					FinishCombatMove(false);
					return;
				}

				DrawDebugSphere(GetWorld(), SuppressionLocation, 5.0f, 24, FColor::Yellow, false, 1.0f);
				Location = SuppressionLocation;
				
				GetCharacter()->PlayBarkOrStartConversation(VO_SUSPECTS_AND_CIVILIAN::REPLY_SHOOTING, true, 2.0f);
				//PlayAISpeech(VO_SUSPECTS_AND_CIVILIAN::SUPPRESSION, true, 5.0f);

				TimeUntilNextSuppressionMove = 5.0f;
			}
		}
		else
		{
			TimeUntilNextSuppressionFire -= DeltaTime;

			if (TimeUntilNextSuppressionFire <= 0.0f)
			{
				if (bHasBrokenLOS)
				{
					OwningCombatActivity->StopScriptedFire();

					if (!GetCharacter()->GetAIArchetype()->bSuppressionRequiresLOSOnLastKnownEnemyPosition ||
						(GetCharacter()->GetAIArchetype()->bSuppressionRequiresLOSOnLastKnownEnemyPosition && GetCharacter()->HasLineOfSightTo(GenLocationInLOS)))
					{
						OwningCombatActivity->ScriptedFireAtLocation(GenLocationInLOS, FMath::FRandRange(0.2f, 0.8f), true, 2.0f);
					}

					TimeUntilNextSuppressionFire = FMath::FRandRange(0.5f, 2.0f);
				}
			}
		}
	}
}
