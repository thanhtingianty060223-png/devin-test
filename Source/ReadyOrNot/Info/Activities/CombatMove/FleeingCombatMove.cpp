// Void Interactive, 2020

#include "FleeingCombatMove.h"

#include "Actors/Environment/FlankingAvoidanceVolume.h"

#include "Characters/CyberneticCharacter.h"
#include "Characters/CyberneticController.h"

#include "Info/Activities/BaseCombatActivity.h"

#include "NavigationSystem.h"

#include "EnvironmentQuery/EnvQueryManager.h"

#include "ReadyOrNotAIConfig.h"
#include "Actors/Door.h"
#include "Navigation/ReadyOrNotNavQueries.h"

TAutoConsoleVariable<int32> CVarRonAIFleeTest(TEXT("Flee.UnlimitedFlees"), 0, TEXT("0 = Default AI Behaviour. 1 = Only try and flee"));

DECLARE_CYCLE_STAT(TEXT("RoN ~ Fleeing Combat Move ~ Find Flee Spot"), STAT_FleeingCombatMoveFindingFleeSpot, STATGROUP_CombatActivity);

UFleeingCombatMove::UFleeingCombatMove()
{
	bAbortMoveWhenActivityFinished = false;
	bAbortMoveWhenActivityOverriden = false;
}

void UFleeingCombatMove::StartActivity(AAIController* Owner)
{
	Super::StartActivity(Owner);
	
	FleesRemaining = FMath::RandRange(AI_CONFIG_GET_INT("MinFlees"), AI_CONFIG_GET_INT("MaxFlees"));

	#if !UE_BUILD_SHIPPING
	if (CVarRonAIFleeTest.GetValueOnAnyThread() > 0)
	{
		FleesRemaining = 255;
	}
	#endif

	if (FleesRemaining == 0)
	{
		#if !UE_BUILD_SHIPPING
		UnableToCombatReason = "No flees remaining";
		#endif

		return;
	}

	GetCharacter()->ReasonsToSprint.AddUnique("fleeing");
	
	GetCharacter()->Multicast_ChangeFaceEmotion(ECharacterEmotion::Afraid, 15.0f, 1.0f, 0.1f, 50);
}

void UFleeingCombatMove::FinishedActivity(bool bSuccess)
{
	Super::FinishedActivity(bSuccess);
	
	GetCharacter()->ReasonsToSprint.Remove("fleeing");
}

bool UFleeingCombatMove::ShouldDisableMoveRequest() const
{
	// Animation blocking checks whether we're stunned, but if we're trying to find a path to flee gas, we don't want to skip this async pathing because of gas stun
	const bool bPathRequestForGasFlee = CurrentFleeType == FT_Gas;
	if (GetCharacter()->IsAnimationBlocking() && !bPathRequestForGasFlee)
		return true;

	return false;
}

#if !UE_BUILD_SHIPPING
void UFleeingCombatMove::PerformActivity_Debug(const float DeltaTime)
{
	Super::PerformActivity_Debug(DeltaTime);

	if (CVarRonAIFleeTest.GetValueOnAnyThread() > 0)
	{
		FleesRemaining = 255;
	}
}

void UFleeingCombatMove::GatherDebugString(FString& OutString)
{
	Super::GatherDebugString(OutString);

	OutString += AddDebugString("Flees Remaining", FString::FromInt(FleesRemaining));
	OutString += AddDebugString("Last Flee Point", GetNameSafe(LastFleePoint));
}
#endif

float UFleeingCombatMove::GetDestinationTolerance() const
{
	return 30.0f;
}

FName UFleeingCombatMove::GetMoveStyleOverride_Implementation() const
{
	if (GetCharacter()->GetCurrentWeaponAnimType() == EAnimWeaponType::CWT_Unarmed)
	{
		return GetCharacter()->AssignedAIData->MovementStyle.UnarmedMoveStyle;
	}
	
	if (GetCharacter()->GetCurrentWeaponAnimType() == EAnimWeaponType::CWT_Pistol)
	{
		return GetCharacter()->CurMoveDataBlock.PistolMovementStyle;
	}
	
	if (GetCharacter()->GetCurrentWeaponAnimType() == EAnimWeaponType::CWT_Rifle)
	{
		return GetCharacter()->MovementStyleData.LoweredTwoHandedMoveStyle;
	}

	return NAME_None;
}

void UFleeingCombatMove::ResetData()
{
	Super::ResetData();

	FleesRemaining = 0;
	
	UsedFleePoints.Empty();
	LastExitPoints.Empty();
	PreviouslyValidLocations.Empty();
	
	LastFleePoint = nullptr;
	
	bRunningEQS = false;
}

void UFleeingCombatMove::RequestCombatMove(const float DeltaTime)
{
	Super::RequestCombatMove(DeltaTime);

	SCOPE_CYCLE_COUNTER(STAT_FleeingCombatMoveFindingFleeSpot)
	
	if (FleesRemaining == 0)
	{
		#if !UE_BUILD_SHIPPING
		UnableToCombatReason = "No flees remaining";
		#endif
		
		FinishCombatMove(false);

		return;
	}
	
	if (HasReachedLocation(GetDestinationTolerance()) && Location != FVector::ZeroVector)
	{
		#if !UE_BUILD_SHIPPING
		ULog::Info(FString::Printf(TEXT("Finished Flee Move %s (Reached Final Destination: %s)"), *GetName(), *Location.ToString()));
		#endif
		
		Location = FVector::ZeroVector;
		FinishCombatMove(true);
		return;
	}

	if (!IsDoingFleeMove())
	{
		bool bFleeSuccess = false;
		if (GetCharacter()->IsCurrentlyGassed())
		{
			bFleeSuccess = FleeGas();
			CurrentFleeType = FT_Gas;
		}
		else
		{
			bFleeSuccess = TryFlee();
			CurrentFleeType = FT_Regular;
		}
		
		if (!bFleeSuccess)
		{
			#if !UE_BUILD_SHIPPING
			ULog::Info(FString::Printf(TEXT("%s | Unable to flee using a safe route"), *GetName()));
			UnableToCombatReason = "No safe route available";
			#endif
			
			Location = FVector::ZeroVector;
			CurrentFleeType = FT_None;
			FinishCombatMove(false);
		}
	}
}

bool UFleeingCombatMove::FleeGas()
{
	if (!GetCharacter()->LastDamageEvent.Causer)
	{
		return false;
	}
	
	if (bRunningEQS)
	{
		return true;
	}

	FEnvQueryRequest EscapeGasRequest = FEnvQueryRequest(GetCharacter()->EscapeGasQuery, GetCharacter());
	EscapeGasRequest.Execute(EEnvQueryRunMode::AllMatching, this, &UFleeingCombatMove::OnEQSComplete);
	
	bRunningEQS = true;
	
	return true;
}

void UFleeingCombatMove::OnEQSComplete(TSharedPtr<FEnvQueryResult> Result)
{
	bRunningEQS = false;
	
	TArray<FVector> Locations;
	Result->GetAllAsLocations(Locations);

	if (Locations.Num())
	{
		// We take an item around 70% of best score (score linearly decreases with distance from character).
		// This way we flee to a point near the gas ending border, but still far enough away from the gas they don't accidentally
		// still register as affected by the gas when they reach the destination
		Location = Result->GetItemAsLocation(Locations.Num() / 3);
		return;
	}

	// We didn't get any safe points, fallback to basic points generation and pick one far away
	GenerateNavigablePoints(GetCharacter()->GetActorLocation(), FNavGenerationParameters(ENavType::NT_Grid, ENavLOS::NL_Any, 1000, 0.0f), Locations);

	if (!Locations.Num())
		return;
	
	FVector FurthestLocation = Locations[0];
	float FurthestLocationDistSq = FVector::DistSquared(GetCharacter()->GetActorLocation(), FurthestLocation);
	for (auto& GeneratedLocation : Locations)
	{
		float Distance = FVector::DistSquared(GeneratedLocation, GetCharacter()->GetActorLocation()); 
		if (Distance > FurthestLocationDistSq)
		{
			FurthestLocation = Location;
			FurthestLocationDistSq = Distance; 
		}
	}

	Location = FurthestLocation;
}

bool UFleeingCombatMove::TryFlee()
{
	if (IsDoingFleeMove())
		return true;
	
	if (FleesRemaining <= 0)
		return false;
	
	AThreatAwarenessActor* NearestThreat = OwningController->GetTargetingComp()->GetNearestThreat();
	if (!NearestThreat)
		return false;

	GetCharacter()->PlayBarkOrStartConversation(VO_SUSPECTS_AND_CIVILIAN::FLEEING, true, 10.0f);

	FleesRemaining--;
	
	TArray<ADoor*> Doors;
	NearestThreat->GetUniqueExtis(Doors);
	
	TArray<ADoor*> ValidDoors;
	
	for (ADoor* D : Doors)
	{
		if (D->IsLocked() || D->IsTrapLive() || D->IsIgnoredForFlee())
			continue;
		
		ValidDoors.AddUnique(D);
	}

	TArray<AThreatAwarenessActor*> ExitPoints;
	
	if (ValidDoors.Num() > 0)
	{
		const ADoor* Door = ValidDoors[FMath::RandRange(0, ValidDoors.Num() - 1)];
		
		if (!Door->FrontThreatAwarenessPoints.Contains(NearestThreat))
		{
			ExitPoints = Door->FrontThreatAwarenessPoints;
		}
		
		if (!Door->BackThreatAwarenessPoints.Contains(NearestThreat))
		{
			ExitPoints = Door->BackThreatAwarenessPoints;
		}
	}
	
	ExitPoints.Remove(nullptr);
	ExitPoints.RemoveAll([](AThreatAwarenessActor* Threat)
	{
		return !Threat || Threat->IsDoorThreat() || Threat->bIsOutside;
	});
	
	LastExitPoints = ExitPoints;
	
	if (ExitPoints.Num() > 0)
	{
		for (int32 i = 0; i < ExitPoints.Num(); i++)
		{
			AThreatAwarenessActor* ExitPoint;
			
			if (ExitPoints.Num() > 20)
			{
				ExitPoint = ExitPoints[FMath::RandRange(0, 20)];
			}
			else
			{
				ExitPoint = ExitPoints[FMath::RandRange(0, ExitPoints.Num() - 1)];
			}

			LastFleePoint = ExitPoint;
			
			if (!HasAnyOtherCombatMoveGotLocation(ExitPoint->GetActorLocation(), 200.0f))
			{
				const FVector FleePoint = ExitPoint->GetActorLocation();

				#if !UE_BUILD_SHIPPING
				//GEngine->AddOnScreenDebugMessage(-1, 30.0f, FColor::White, GetCharacter()->GetName() + " is fleeing!");
				V_LOGM(LogReadyOrNot, "Found Flee Point @ %s (%s)", *FleePoint.ToString(), *ExitPoint->GetName());
				#endif
				
				for (AThreatAwarenessActor* e : ExitPoint->PathableThreatAwarenessActors)
				{
					UsedFleePoints.AddUnique(e);
				}

				Location = FleePoint;

				return true;
			}
		}
	}
		
	return false;
}

bool UFleeingCombatMove::IsDoingFleeMove() const
{	
	if (bSearchingPath || bRunningEQS)
		return true;
	
	if (const FAIRequestID* MoveRequestPtr = MoveRequestID.Find(FIntVector(Location)))
	{
		if (OwningController->IsMovingForRequest(MoveRequestPtr->GetID()))
		{
			return true;
		}
	}
	
	if (Location != FVector::ZeroVector && !HasReachedLocation(75.0f))
		return true;

	if (OwningController->IsMoving())
		return true;
	
	return false;
}

void UFleeingCombatMove::OnPathFound(uint32 PathId, ENavigationQueryResult::Type ResultType, FNavPathSharedPtr NavPath)
{
	if (!NavPath.IsValid())
		return;
	
	NavPath->EnableRecalculationOnInvalidation(bAllowRePathOnInvalidation);
	NavPath->SetIgnoreInvalidation(!bAllowRePathOnInvalidation);

	if (!GetCharacter())
		return;
	
	if (ResultType == ENavigationQueryResult::Success)
	{
		// Gas fleeing doesn't care where the path goes as long as it's out of the gas
		if (CurrentFleeType == FT_Gas)
		{
			OnPathFoundSuccess(NavPath);
			return;
		}
		
		// Get the first 2 path points and make sure the path doesn't go past the player... otherwise if it does add it to used flee points and force a search again
		const TArray<FNavPathPoint>& PathPts = NavPath.Get()->GetPathPoints();
		for (int32 i = 1; i < PathPts.Num(); i++)
		{
			//FVector v1 = UKismetMathLibrary::FindLookAtRotation(PathPts[i-1].Location, PathPts[i].Location).Vector();

			//for (TActorIterator<AReadyOrNotCharacter>It(GetWorld()); It; ++It)
			for (AReadyOrNotCharacter* Character : GetWorld()->GetGameState<AReadyOrNotGameState>()->AllReadyOrNotCharacters)
			{
				if (Character->IsOnSWATTeam())
				{
					const float Distance = FVector::Distance(Character->GetActorLocation(), PathPts[i].Location);
					
					if (Distance < 1000.0f)
					{
						//FVector v2 = UKismetMathLibrary::FindLookAtRotation(PathPts[i-1].Location, Character->GetActorLocation()).Vector();
						
						//const bool bIsMovingTowardsThisPlayer = FVector::DotProduct(v1, v2) > 0.0f;
						
						//if (bIsMovingTowardsThisPlayer)
						{
							UsedFleePoints.AddUnique(LastFleePoint);
							FleesRemaining++;
							PathFailedCount++;
							if (PathFailedCount > 10)
							{
								Location = FVector::ZeroVector;
							}

							Location = FVector::ZeroVector;
							TryFlee();
							return;
						}
					}
				}
			}
		}
		
		OnPathFoundSuccess(NavPath);
	}
	else
	{
		PathFailedCount++;
		if (PathFailedCount > 0)
		{
			// if we can't find a path just move somewhere in the room
			AThreatAwarenessActor* NearestThreat = OwningController->GetTargetingComp()->GetNearestThreat();
			if (NearestThreat && NearestThreat->PathableThreatAwarenessActors.Num() > 0)
			{
				NearestThreat->PathableThreatAwarenessActors.Remove(nullptr);
				SetLocation(NearestThreat->PathableThreatAwarenessActors[FMath::RandRange(0, NearestThreat->PathableThreatAwarenessActors.Num() - 1)]->GetActorLocation());
			}
		}
		else
		{
			if (PathFailedCount > 30 || !PreviouslyValidLocations.Contains(Location))
			{
				Location = FVector::ZeroVector;
			}
		}
	}
}

void UFleeingCombatMove::OnPathFoundSuccess(FNavPathSharedPtr NavPath)
{
	PathFailedCount = 0;
	bLastRequestedLocationWaitingForRepath = false;

	RequestMoveFromPath(Location, NavPath);

	GetCharacter()->PlayRawVOWithCooldown(VO_SUSPECTS_AND_CIVILIAN::TELL_FLEEING, 3.0f);
		
	PreviouslyValidLocations.AddUnique(Location);
}

float UFleeingCombatMove::CalculateFleeDesire() const
{
	AThreatAwarenessActor* NearestThreat = OwningController->GetTargetingComp()->GetNearestThreat();
	if (!NearestThreat)
		return 0.0f;

	TArray<ADoor*> Exits;
	if (!NearestThreat->GetUniqueExtis(Exits))
		return 0.0f;
	
	TArray<ADoor*> ValidExits;
	
	for (ADoor* D : Exits)
	{
		if (D->IsLocked() || D->IsTrapLive() || D->IsIgnoredForFlee())
			continue;
		
		ValidExits.AddUnique(D);
	}

	if (ValidExits.Num() == 0)
		return 0.0f;

	if (FleesRemaining <= 0)
		return 0.0f;

	// Flee formula
	// The greater number of valid exits and remaining flees, the greater the desire to flee
	return 1.0f / ((float)ValidExits.Num()/(float)FleesRemaining);
}

bool UFleeingCombatMove::ShouldPerformCombatMove() const
{
	if (GetCharacter()->IsDeadOrUnconscious())
		return false;
	
	if (GetCharacter()->IsPlayingDead())
		return false;

	if (GetCharacter()->IsInRagdoll())
		return false;

	if (GetCharacter()->IsArrestedOrSurrendered())
		return false;

	if (GetCharacter()->IsStunned() && !GetCharacter()->IsOnlyStunnedWithGas())
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

TSubclassOf<UNavigationQueryFilter> UFleeingCombatMove::GetNavigationQueryOverride()
{
	if (CurrentFleeType == FT_Gas)
		return UNavQuery_GasFleeingSuspect::StaticClass();

	return nullptr;
}

TEnumAsByte<EFleeType> UFleeingCombatMove::GetFleeType()
{
	return CurrentFleeType;
}
