// Copyright Void Interactive, 2021

#include "BaseActivity.h"

#include "Actors/Items/BallisticsShield.h"

#include "Characters/CyberneticController.h"
#include "Characters/CyberneticCharacter.h"

#include "lib/ReadyOrNotFunctionLibrary.h"

#include "NavigationSystem.h"
#include "ReadyOrNotAISystem.h"
#include "Navigation/NavLinkProxy.h"

#if !UE_BUILD_SHIPPING
#include "Log.h"
#endif

TAutoConsoleVariable<int32> CVarRonToggleActivityDebugLines(TEXT("a.RonToggleActivityDebugLines"), 0, TEXT("0 = No draw all activity debug lines. 1 = Draw all activity debug lines"));
TAutoConsoleVariable<int32> CVarUseRawPath(TEXT("AI.UseRawPath"), 1, TEXT("0 = Use spline path. 1 = Only use raw path"));

#if !UE_BUILD_SHIPPING
TAutoConsoleVariable<int32> CVarNoBingChilling(TEXT("AI.NoBingChilling"), 0, TEXT("0 = No ice cream. 1 = Ice cream"));
#endif

DECLARE_CYCLE_STAT(TEXT("RoN ~ Base Activity ~ Perform Activity"), STAT_BaseActivity_PerformActivity, STATGROUP_BaseActivity);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Base Activity ~ Perform Activity (Debug)"), STAT_BaseActivity_PerformActivity_Debug, STATGROUP_BaseActivity);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Base Activity ~ Request Move Async"), STAT_BaseActivity_RequestMoveAsync, STATGROUP_BaseActivity);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Base Activity ~ Request Move Async ~ Debug Log"), STAT_BaseActivity_RequestMoveAsync_DebugLog, STATGROUP_BaseActivity);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Base Activity ~ On Path Found"), STAT_BaseActivity_OnPathFound, STATGROUP_BaseActivity);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Base Activity ~ On Path Found ~ Debug Log"), STAT_BaseActivity_OnPathFound_DebugLog, STATGROUP_BaseActivity);

UBaseActivity::UBaseActivity()
{
	ActivityName = FText::FromString(GetName());
	
	bIsProgressActivity = false;
	bFinishActivityWhenOverriden = false;
	bAbortMoveWhenActivityFinished = true;
	bAbortMoveWhenActivityOverriden = true;
	bAbortActivityIfCannotReachLocation = true;
	bResetStateMachineWhenActivityResumed = true;
	bAbortIfTrackingEnemy = false;
	bNoMoveActivity = false;
	bAllowRePathOnInvalidation = true;
	bAbortIfNotMovingForAWhile = true;
	bAbortIfSurrendered = false;

	bNoResetDataOnFinish = false;

	MoveAcceptanceRadius = 10.0f;
	
	ActivityStatus = EActivityStatus::Uninitialized;

	ActivityStateMachine = CreateDefaultSubobject<UActivityFiniteStateMachine>(*(ActivityName.ToString() + "_StateMachine"), true);
}

FString UBaseActivity::GetActiveStateName() const
{
	if (ActivityStateMachine->GetActiveState())
		return ActivityStateMachine->GetActiveState()->Name;
	
	return "None";
}

float UBaseActivity::GetActiveStateUptime() const
{
	if (ActivityStateMachine->GetActiveState())
		return ActivityStateMachine->GetActiveState()->Uptime;
	
	return 0.0f;
}

int32 UBaseActivity::GetActiveStateID() const
{
	if (ActivityStateMachine->GetActiveState())
		return ActivityStateMachine->GetActiveState()->ID;

	return -1;
}

void UBaseActivity::GatherDebugString(FString& OutString)
{
	#if !UE_BUILD_SHIPPING
    OutString += AddDebugString("Location", Location.ToCompactString());
    OutString += AddDebugString("Active State", GetActiveStateName() + " (ID: " + FString::FromInt(GetActiveStateID()) + ")");
	#endif
}

#if !UE_BUILD_SHIPPING
FString UBaseActivity::AddDebugString(FString Category, FString Text)
{
	return "\n\t" + (Category.IsEmpty() ? "" : Category + ": ") + Text;
}
#endif

bool UBaseActivity::CanTick() const
{
	if (!OwningController || !GetCharacter())
		return false;

	if (ActivityStatus == EActivityStatus::Paused)
		return false;

	//if (bPauseIfRecentlyInCombat && UReadyOrNotAISystem::WasRecentlyInCombat(RecentCombatTimeThreshold, false))
		//return false;
	
	if (bPauseIfTrackingEnemy && OwningController->GetTrackedTarget() && OwningController->IsCharacterEnemy(OwningController->GetTrackedTarget()))
		return false;
	
	if (IsActivityComplete())
		return false;

	if (GetCharacter()->IsCarried())
		return false;
	
	return HasStartedActivity() && !IsActivityComplete();
}

void UBaseActivity::SetLocation(const FVector& NewLocation, const bool bRequestMove, const FVector CustomExtent)
{
	#if WITH_EDITOR
	ensureAlwaysMsgf(!bNoMoveActivity, TEXT("%s is a no move activity. Failed to set location"), *GetName());
	#endif
	
	if (bNoMoveActivity || NewLocation == FVector::ZeroVector)
	{
		Location = FVector::ZeroVector;
		return;
	}

	if (const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		FNavLocation NewLocationProjected(NewLocation);
		const bool bSuccess = NavSys->ProjectPointToNavigation(NewLocation, NewLocationProjected, CustomExtent != FVector::ZeroVector ? CustomExtent : FVector(75.0f, 75.0f, 150.0f));
		
		if (!bSuccess && bAbortActivityIfProjectedLocationFailed)
		{
			Location = FVector::ZeroVector;
			if (OwningController)
				OwningController->FinishActivity(this, false, true);
			return;
		}

		Location = NewLocationProjected.Location;
	}
	
	if (bRequestMove) 
		RequestMoveAsync(bAllowPartialMove, false);
}

bool UBaseActivity::HasReachedLocation(const float Tolerance) const
{
	#if WITH_EDITOR
	ensureAlways(OwningController && GetCharacter());
	
	if (!OwningController || !GetCharacter())
		return false;
	#endif

	if (Location == FVector::ZeroVector)
		return true;

	uint16 Dist;
	if ((uint16)FMath::Abs(GetCharacter()->GetNavAgentLocation().Z - Location.Z) <= 70)
	{
		Dist = (uint16)FVector2D::Distance(FVector2D(Location), FVector2D(GetCharacter()->GetNavAgentLocation()));
	}
	else
	{
		Dist = (uint16)FVector::Distance(Location, GetCharacter()->GetNavAgentLocation());
	}
	
	//ULog::Info("Dist: " + GetCharacter()->GetName() + ": " + FString::FromInt(Dist));
	//ULog::Info("Tolerance: " + GetCharacter()->GetName() + ": " + FString::FromInt((int32)Tolerance));

	return Dist <= FMath::Clamp<uint16>(Tolerance, 1, 10000);
	
	// if we're not moving then increase tolerance, give a bit of time for once we're given a move to perform async queries
	//const bool bIncreasedTolerance = TimeSinceLastAsyncMove > 1.0f && !OwningController->IsMoving();
	//return Dist < FMath::Clamp(Tolerance * (bIncreasedTolerance ? 2.5f : 1.0f), 1.0f, MAX_ACTIVITY_DESTINATION_TOLERANCE);
}

void UBaseActivity::AbortMove(const bool bAbortAll, const FPathFollowingResultFlags::Type& Result)
{
	bSearchingPath = false;
	
	if (OwningController)
	{
		if (bAbortAll)
		{
			for (const auto& It : MoveRequestID)
			{
				if (OwningController->IsMovingForRequest(It.Value))
				{
					OwningController->GetPathFollowingComponent()->AbortMove(*this, Result, It.Value);

					#if WITH_EDITOR
					ULog::Info("Move (ID: " + It.Value.ToString() + ") aborted by " + GetName());
					#endif
				}
			}
			
			OwningController->GetPathFollowingComponent()->AbortMove(*this, Result);
			MoveRequestID.Empty();
			Location = FVector::ZeroVector;
			return;
		}
		
		FIntVector Key = FIntVector(Location);
		
		// Only abort our moves and not anyone elses'
		const FAIRequestID* RequestIDPtr = MoveRequestID.Find(Key);
		if (!RequestIDPtr)
		{
			Key = FIntVector(LastRequestedLocation);
			RequestIDPtr = MoveRequestID.Find(Key);
		}

		if (RequestIDPtr)
		{
			OwningController->GetPathFollowingComponent()->AbortMove(*this, Result, *RequestIDPtr);

			ULog::Info("Move (ID: " + RequestIDPtr->ToString() + ") aborted by " + GetName());
			
			MoveRequestID.Remove(Key);

			Location = FVector::ZeroVector;
		}
	}
}

void UBaseActivity::AbortLastMove(const FPathFollowingResultFlags::Type& Result)
{
	if (OwningController)
	{
		OwningController->GetPathFollowingComponent()->AbortMove(*this, Result, OwningController->LastRequestID);

		#if WITH_EDITOR
		ULog::Info("Move (ID: " + OwningController->LastRequestID.ToString() + ") aborted by " + GetName());
		#endif
	}
}

#if !UE_BUILD_SHIPPING
void UBaseActivity::ActivityFailed(const FString& InActivityFailureReason, const bool bWarn)
#else
void UBaseActivity::ActivityFailed()
#endif
{
	#if !UE_BUILD_SHIPPING
	if (GetCharacter())
	{
		if (bWarn)
			ULog::Warning(GetName() + " | " + GetCharacter()->GetName() + " | Activity Failed. Reason: " + InActivityFailureReason);
		else
			ULog::Error(GetName() + " | " + GetCharacter()->GetName() + " | Activity Failed. Reason: " + InActivityFailureReason);

		if (!bWarn)
		{
			if (CVarNoBingChilling.GetValueOnAnyThread() == 0)
			{
				GetCharacter()->PlayRawVO("", "ActivityFail", false);
			}
		}
	}
	else
	{
		if (bWarn)
			ULog::Warning(GetName() + " | Activity Failed. Reason: " + InActivityFailureReason);
		else
			ULog::Error(GetName() + " | Activity Failed. Reason: " + InActivityFailureReason);
	}
	#endif
	
	if (OwningController)
	{
		#if !UE_BUILD_SHIPPING
		OwningController->LastActivityFailedReason = InActivityFailureReason;
		#endif

		OwningController->FinishActivity(this, false, true);
	}
}

bool UBaseActivity::OverrideFocalPoint(FVector& FocalPoint)
{
	FocalPoint = FVector::ZeroVector;
	return OverrideFocalPoint_Blueprint(FocalPoint);
}

void UBaseActivity::PerformActivity(const float DeltaTime)
{
	SCOPE_CYCLE_COUNTER(STAT_BaseActivity_PerformActivity);
	
	if (!OwningController)
	{
		ACTIVITY_FAILED("No valid owning controller specified");
		return;
	}

	if (!GetCharacter())
	{
		ACTIVITY_FAILED("No valid owning pawn");
		return;
	}

	if (bAbortIfTrackingEnemy && OwningController->GetTrackedTarget() && OwningController->IsCharacterEnemy(OwningController->GetTrackedTarget()))
	{
		ACTIVITY_FAILED("Tracking enemy (bAbortIfTrackingEnemy is set to True)", true);
		return;
	}

	// todo: abort version
	if (bPauseIfRecentlyInCombat && UReadyOrNotAISystem::WasRecentlyInCombat(RecentCombatTimeThreshold, false))
	{
		ACTIVITY_FAILED("Recently in combat (bAbortIfRecentlyInCombat is set to True)", true);
		return;
	}

	if (bAbortIfSurrendered && GetCharacter()->IsSurrendered())
	{
		//ULog::Warning(GetName() + " aborted. Reason: Surrendered");
		OwningController->FinishActivity(this, false, true);
		return;
	}

	PerformActivity_Blueprint(DeltaTime);
	
	// tick timers
	ActivityStartDelay = FMath::Clamp(ActivityStartDelay - DeltaTime, 0.0f, ActivityStartDelay);
	TimeSinceLastAsyncMove += DeltaTime;
	ElapsedActivityTime += DeltaTime;
	
	// Delay before starting the activity
	if (ActivityStartDelay > 0.0f)
	{
		return;
	}

	if (bNoMoveActivity)
	{
		return;
	}

	// Failsafe, if searching path for more than 5 sec
	if (bSearchingPath && TimeSinceLastAsyncMove > 5.0f)
	{
		bSearchingPath = false;
		
		if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
		{
			NavSys->AbortAsyncFindPathRequest(LastAsyncQueryId);
		}
	}

	if (GetCharacter()->GetVelocity().IsNearlyZero(0.001f))
	{
		TimeNotMoving += DeltaTime;
	}
	else
	{
		TimeNotMoving = 0.0f;
	}

	if (!bNoMoveActivity)
	{
		if (bAbortIfNotMovingForAWhile && TimeNotMoving > 10.0f && TimeSinceLastAsyncMove > 5.0f)
		{
			if (!HasReachedLocation(GetDestinationTolerance()))
			{
				ACTIVITY_FAILED("Not moving for over 10 secs", true);
				return;
			}
		}
	}

	//if (LastRequestedLocation != FVector::ZeroVector && bLastRequestedLocationWaitingForRepath)
	//{
	//	bLastRequestedLocationWaitingForRepath = false;
	//	Location = LastRequestedLocation;
	//	RequestMoveAsync(bAllowPartialMove);
	//	return;
	//}

	const bool bZHeightIsDifferent = (uint16)FMath::Abs(GetCharacter()->GetNavAgentLocation().Z - Location.Z) >= 50;
	
	if (Location != FVector::ZeroVector && (!HasReachedLocation(GetDestinationTolerance()) || bZHeightIsDifferent || bAlwaysRequestMove))
	{
		RequestMoveAsync(bAllowPartialMove);
		return;
	}

	if (HasReachedLocation(GetDestinationTolerance()) && CanFinishActivity())
	{
		OwningController->FinishActivity(this, true);
	}
}

#if !UE_BUILD_SHIPPING
void UBaseActivity::PerformActivity_Debug(const float DeltaTime)
{
	SCOPE_CYCLE_COUNTER(STAT_BaseActivity_PerformActivity_Debug);
	
	if (CVarRonToggleActivityDebugLines.GetValueOnAnyThread() > 0)
	{
		if (Location != FVector::ZeroVector)
		{
			DrawDebugBox(GetWorld(), Location, FVector(15.0f), FColor::Purple, false, DeltaTime + 0.005f, 0, 1.25f);
			DrawDebugCircle(GetWorld(), Location, GetDestinationTolerance(), 64, FColor::White, false, DeltaTime + 0.005f, 0, 1.0f, FVector::RightVector, FVector::ForwardVector);

			FString DebugMsg = Location.ToCompactString();
			DebugMsg += LINE_TERMINATOR;
			DebugMsg += GetName();
			DebugMsg += " | ";
			DebugMsg += GetCharacter()->GetName();
			DrawDebugString(GetWorld(), Location, DebugMsg, nullptr, FColor::White, DeltaTime - 0.01f, true);
		}
	}
}
#endif

UWorld* UBaseActivity::GetWorld() const
{
	if (HasAllFlags(RF_ClassDefaultObject))
		return nullptr;

	if (OwningController)
	{
		return OwningController->GetWorld();
	}
	
	return GetOuter() ? GetOuter()->GetWorld() : nullptr;
}

void UBaseActivity::RequestMoveAsync(const bool bAllowPartialPath, bool bProjectDestination)
{
	SCOPE_CYCLE_COUNTER(STAT_BaseActivity_RequestMoveAsync);
	
	if (bSearchingPath)
		return;

	if (bNoMoveActivity)
		return;
	
	if (Location == FVector::ZeroVector)
		return;
	
	const ACyberneticCharacter* Character = GetCharacter();

	if (!OwningController || !OwningController->GetRONPathFollowingComp() || !Character)
		return;
	
	if (OwningController->ShouldPostponePathUpdates())
		return;

	if (OwningController->AsyncPathRequestIDs.Num() > 0)
		return;
	
	// Absolutely no move request when traversing through a nav link to prevent
	// paths being requested just before traversing, bugging out the AI
	/* TODO: will probably cause an issue, we'll see
	if (OwningController->GetRONPathFollowingComp()->IsCurrentSegmentNavigationLink() ||
		OwningController->GetRONPathFollowingComp()->IsUsingCustomLink())
		return;
	*/

	if (const FAIRequestID* MoveRequestPtr = MoveRequestID.Find(FIntVector(Location)))
	{
		if (OwningController->IsMovingForRequest(MoveRequestPtr->GetID()))
		{
			return;
		}
	}

	if (FIntVector(LastRequestedLocation) == FIntVector(Location) && HasReachedLocation(GetDestinationTolerance()))
		return;
	
	if (ShouldDisableMoveRequest())
		return;

	if (Character->NoPathCooldown > 0.0f)
		return;

	if (!Character->IsActiveForMovement())
		return;

	if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		if (const ANavigationData* NavData = NavSys->GetNavDataForProps(Character->GetMovementComponent()->GetNavAgentPropertiesRef()))
		{
			const FSharedConstNavQueryFilter QueryFilter = UNavigationQueryFilter::GetQueryFilter(*NavData, GetNavigationQueryOverride() ? GetNavigationQueryOverride() : OwningController->GetNavQueryFilter());
			const FVector StartLocation = Character->GetNavAgentLocation();
			const FVector EndLocation = Location;
			
			FNavLocation StartLocationProjected(StartLocation);
			FNavLocation EndLocationProjected(EndLocation);

			NavSys->ProjectPointToNavigation(StartLocation, StartLocationProjected, FVector(75.0f, 75.0f, 150.0f));

			bool bEndProjectSuccess = true;
			if (bProjectDestination)
				bEndProjectSuccess = NavSys->ProjectPointToNavigation(EndLocation, EndLocationProjected, FVector(75.0f, 75.0f, 150.0f));

			if (!bEndProjectSuccess)
			{
				if (bAbortActivityIfProjectedLocationFailed)
				{
					OwningController->FinishActivity(this, false, true);
				}
				
				return;
			}

			FNavPathQueryDelegate NavDelegate;
			NavDelegate.BindUObject(this, &UBaseActivity::OnPathFound_Internal);
			
			TimeSinceLastAsyncMove = 0.0f;
			bSearchingPath = true;
			LastRequestedLocation = Location;
			LastAsyncQueryId = NavSys->FindPathAsync(Character->GetNavAgentPropertiesRef(), ACyberneticController::CreatePathFindingQuery(QueryFilter, NavData, StartLocationProjected, EndLocationProjected, bAllowPartialPath, OwningController), NavDelegate, EPathFindingMode::Regular);

			#if !UE_BUILD_SHIPPING
			{
				SCOPE_CYCLE_COUNTER(STAT_BaseActivity_RequestMoveAsync_DebugLog);

				ULog::Info(FString::Printf(TEXT("[%s][%s][%s] Requesting Async Path to %s (ID %d)!"), *GetName(), *OwningController->GetName(), *FString(__FUNCTION__), *EndLocationProjected.Location.ToString(), LastAsyncQueryId));
				//V_LOGM(LogReadyOrNot, "[%s][%s][%s] Requesting Async Path to %s (ID %d)!", *GetName(), *OwningController->GetName(), *FString(__FUNCTION__), *EndLocationProjected.Location.ToString(), LastAsyncQueryId);
			}
			#endif
		}
	}
}

void UBaseActivity::OnPathFound(uint32 PathId, const ENavigationQueryResult::Type ResultType, const FNavPathSharedPtr NavPath)
{
	SCOPE_CYCLE_COUNTER(STAT_BaseActivity_OnPathFound);
	
	if (!NavPath.IsValid())
		return;
	
	NavPath->EnableRecalculationOnInvalidation(bAllowRePathOnInvalidation);
	NavPath->SetIgnoreInvalidation(!bAllowRePathOnInvalidation);

	// Waiting for gen.. do not remove. stops AI going WAITING <-> MOVING in a packaged game
	if (!NavPath->IsValid())
	{
		#if !UE_BUILD_SHIPPING
		TArray<FNavPathPoint> PathPoints = NavPath->GetPathPoints();
		if (CVarRonToggleActivityDebugLines.GetValueOnAnyThread() > 0)
		{
			for (int32 i = 1; i < PathPoints.Num(); i++)
			{
				DrawDebugLine(GetWorld(), PathPoints[i-1].Location + FVector(0.0f, 0.0f, 30.0f), PathPoints[i].Location + FVector(0.0f, 0.0f, 30.0f), FColor::Orange, false, 0.5f, 0, 1.25f);
			}

			//const FString DebugMessage = FString::Printf(TEXT("Is Ready: %s\nIs Up-To-Date: %s"), *FString(NavPath->IsReady() ? "true" : "false"), *FString(NavPath->IsUpToDate() ? "true" : "false"));
			//DrawDebugString(GetWorld(), PathPoints[0].Location, DebugMessage, nullptr, FColor::White, 0.5f, true);
		}
		#endif

		return;
	}

	if (NavPath->GetPathPoints().Last() == FVector::ZeroVector)
		return;
	
	/*
	if (NavPath->IsWaitingForRepath())
	{
		bLastRequestedLocationWaitingForRepath = true;
		return;
	}
	*/
	
	bLastRequestedLocationWaitingForRepath = false;

	if (ResultType != ENavigationQueryResult::Success || (NavPath->IsPartial() && !bAllowPartialMove))
	{
		// navmesh could be updating (ie. a door just opened or closed and a navlink needs to update), require a few fails
		PathFailedCount++;
		if (PathFailedCount < 50)
		{
			return;
		}
		
		#if !UE_BUILD_SHIPPING
		{
			SCOPE_CYCLE_COUNTER(STAT_BaseActivity_OnPathFound_DebugLog);
			
			ULog::Warning(FString::Printf(TEXT("[%s][%s][%s] Async Path Failed %d! (Start: %s End: %s)"), *GetName(), *OwningController->GetName(), *FString(__FUNCTION__), LastAsyncQueryId, *NavPath->GetStartLocation().ToString(), *NavPath->GetEndLocation().ToString()));
			//V_LOGM(LogReadyOrNot, "[%s][%s][%s] Async Path Failed %d! (Start: %s End: %s)", *GetName(), *OwningController->GetName(), *FString(__FUNCTION__), LastAsyncQueryId, *NavPath->GetStartLocation().ToString(), *NavPath->GetEndLocation().ToString());

			TArray<FNavPathPoint> PathPoints = NavPath->GetPathPoints();
			
			if (CVarRonToggleActivityDebugLines.GetValueOnAnyThread() > 0)
			{
				DrawDebugBox(GetWorld(), Location, FVector(35.0f), FColor::Red, false, 10.0f);
				for (int32 i = 1; i < PathPoints.Num(); i++)
				{
					DrawDebugLine(GetWorld(), PathPoints[i-1].Location + FVector(0.0f, 0.0f, 30.0f), PathPoints[i].Location + FVector(0.0f, 0.0f, 30.0f), FColor::Red, false, 2.0f, 0, 1.25f);
				}
			}
		}
		#endif

		AbortActivityOnPathNotFound();
	}
	else
	{
		PathFailedCount = 0;
		bLastRequestedLocationWaitingForRepath = false;

		RequestMoveFromPath(Location, NavPath);

		#if !UE_BUILD_SHIPPING
		{
			SCOPE_CYCLE_COUNTER(STAT_BaseActivity_OnPathFound_DebugLog);
			ULog::Info(FString::Printf(TEXT("[%s][%s][%s] Async Path Success %d!"), *GetName(), *OwningController->GetName(), *FString(__FUNCTION__), LastAsyncQueryId));
			//V_LOGM(LogReadyOrNot, "[%s][%s][%s] Async Path Success %d!", *GetName(), *OwningController->GetName(), *FString(__FUNCTION__), LastAsyncQueryId);
		}
		#endif
	}	
}

void UBaseActivity::OnPathFound_Internal(uint32 PathId, ENavigationQueryResult::Type ResultType, FNavPathSharedPtr NavPath)
{
	bSearchingPath = false;
	bLastRequestedLocationWaitingForRepath = false;
	TimeSinceLastAsyncMove = 0.0f;
	
	// Character may have been deleted between starting the path and the path being found
	if (!OwningController)
	    return;

	if (OwningController->ShouldPostponePathUpdates())
		return;

	if (!GetCharacter())
		return;

	OnPathFound(PathId, ResultType, NavPath);
}

void UBaseActivity::AbortActivityOnPathNotFound()
{
	if (bAbortActivityIfCannotReachLocation && OwningController)
	{
		OwningController->FinishActivity(this, false, true);
	}
}

void UBaseActivity::OnPathMoveCompleted(AAIController* Controller, FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	//ULog::Info(ActivityName + " | " + GetNameSafe(OwningController) + " | Move Complete: " + RequestID.ToString());

	if (MoveRequestID.Find(FIntVector(Location)))
	{
		if (Result.IsSuccess() && *MoveRequestID.Find(FIntVector(Location)) == RequestID)
		{
			if (OwningController)
			{
				if (CanFinishActivity() && TryFinishActivityOnMoveComplete())
				{
					OwningController->FinishActivity(this, true);
				}
			}
		}
	}
}

void UBaseActivity::OnPathMovePaused(AAIController* Controller, FAIRequestID RequestID)
{
	ActivityStatus = EActivityStatus::Paused;

	LastPauseRequestID = RequestID;
	AbortMove(true);
	Location = LastRequestedLocation;
	RequestMoveAsync();
}

void UBaseActivity::OnPathMoveResumed(AAIController* Controller)
{
	ActivityStatus = EActivityStatus::Started;
}

FName UBaseActivity::GetMoveStyleOverride_Implementation() const
{
	return NAME_None;
}

void UBaseActivity::OnIncapacitated(AReadyOrNotCharacter* IncapacitatedCharacter, AReadyOrNotCharacter* InstigatorCharacter)
{
}

void UBaseActivity::OnKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter)
{
	if (GetCharacter())
		GetCharacter()->OnCharacterKilled.RemoveAll(this);
}

void UBaseActivity::ResumeActivity()
{
	ActivityStatus = EActivityStatus::Started;

	if (bResetStateMachineWhenActivityResumed)
		ActivityStateMachine->Reset();
}

void UBaseActivity::ActivityOverriden(UBaseActivity* OverridingActivity)
{
	ActivityStatus = EActivityStatus::Paused;

	if (bAbortMoveWhenActivityOverriden)
		AbortMove();

	if (bResetStateMachineWhenActivityResumed)
		ActivityStateMachine->Shutdown();

	if (bFinishActivityWhenOverriden)
		OwningController->FinishActivity(this, true, true);
}

void UBaseActivity::OnClearedFromQueue()
{
}

void UBaseActivity::EquipItem(const EItemCategory InItemCategory)
{
	if (!UInteractionsData::IsPairedInteractionPlayingOn(GetCharacter()))
	{
		if (const ACyberneticCharacter* CC = GetCharacter())
		{
			// consider a shield a priamry
			if (InItemCategory == EItemCategory::IC_Primary)
			{
				if (CC->GetEquippedItem<ABallisticsShield>())
					return;
			}
			
			if (!CC->GetInventoryComponent()->IsEquippingItemOfType(InItemCategory))
				CC->GetInventoryComponent()->EquipItemOfType(InItemCategory);
		}
	}
}

void UBaseActivity::EquipItemOfClass(const TSubclassOf<ABaseItem> InItemClass)
{
	if (!UInteractionsData::IsPairedInteractionPlayingOn(GetCharacter()))
	{
		if (const ACyberneticCharacter* CC = GetCharacter())
		{
			if (!CC->GetInventoryComponent()->IsEquippingItemOfClass(InItemClass))
				CC->GetInventoryComponent()->EquipItemOfClass(InItemClass);
		}
	}
}

void UBaseActivity::EquipWeapon()
{
	if (!UInteractionsData::IsPairedInteractionPlayingOn(GetCharacter()))
	{
		if (!GetCharacter()->GetInventoryComponent()->IsEquippingItemOfType(EItemCategory::IC_Primary))
			GetCharacter()->GetInventoryComponent()->EquipItemOfType(EItemCategory::IC_Primary);
	}
}

bool UBaseActivity::InitActivity(AAIController* Owner)
{
	OwningController = Cast<ACyberneticController>(Owner);
	OwningCharacter = OwningController->GetCharacter();

	if (!OwningController)
	{
		ACTIVITY_FAILED("No valid owning controller. Aborting...");
		ActivityStatus = EActivityStatus::Uninitialized;
		return false;
	}

	if (!GetCharacter())
	{
		ACTIVITY_FAILED("No valid owning pawn. Aborting...");
		ActivityStatus = EActivityStatus::Uninitialized;
		return false;
	}

	if (IsActivityInitialized())
		return true;
	
	ActivityStatus = EActivityStatus::Initialized;

	UActivityManager::Get(this)->AllActivities.AddUnique(this);

	return true;
}

void UBaseActivity::StartActivity(AAIController* Owner)
{
	BroadcastActivityStart();

	ActivityStateMachine->Init();

	ActivityStatus = EActivityStatus::Started;

	TimeSinceLastAsyncMove = 0.0f;
	ElapsedActivityTime = 0.0f;
	TimeNotMoving = 0.0f;
	
	OriginalMoveAcceptanceRadius = MoveAcceptanceRadius;
	
	GetCharacter()->OnCharacterKilled.RemoveAll(this);
	GetCharacter()->OnCharacterKilled.AddDynamic(this, &UBaseActivity::OnKilled);
	
	GetCharacter()->OnCharacterIncapacitated.RemoveAll(this);
	GetCharacter()->OnCharacterIncapacitated.AddDynamic(this, &UBaseActivity::OnIncapacitated);

	StartActivity_Blueprint(Owner);
	
	float Cooldown = GlobalCooldown;
	if (bGlobalCooldownRandomRange)
	{
		Cooldown = FMath::FRandRange(GlobalCooldownRange.X, GlobalCooldownRange.Y);
	}

	if (Cooldown > 0.0f)
	{
		UActivityManager::Get(this)->ActivityClassGlobalCooldownMap.Add(GetClass(), Cooldown);
	}

	UActivityManager::Get(this)->OnStartActivity.Broadcast(this, OwningController);
	
	OnStartActivity.Broadcast(this, OwningController);
}

void UBaseActivity::BroadcastActivityStart()
{
	if (OnRespondToActivityStart.IsBound())
	{
		OnRespondToActivityStart.Broadcast();
		OnRespondToActivityStart.Clear();
	}
}

void UBaseActivity::PlayAISpeech(FString VoiceLine, bool bHasSharedCooldown, float cooldown)
{
	if (bHasLimitSpeechLimit && CurrentSpeechIncrement >= SpeechLimit)
		return;
		
	GetCharacter()->PlayBarkOrStartConversation(VoiceLine, bHasSharedCooldown, cooldown);
	CurrentSpeechIncrement++;
}

void UBaseActivity::FinishedActivity(bool bSuccess)
{
	if (!bNoMoveActivity && bAbortMoveWhenActivityFinished)
	{
		AbortMove(true);
	}
	
	if (ActivityStateMachine)
		ActivityStateMachine->Shutdown();
	
	ActivityStatus = EActivityStatus::Complete;
	
	OnFinishActivity.Broadcast(this, OwningController);
	OnStartActivity.Clear();
	OnRespondToActivityStart.Clear();

	GetCharacter()->OnCharacterKilled.RemoveAll(this);
	GetCharacter()->OnCharacterIncapacitated.RemoveAll(this);
	
	if (GetWorld())
		GetWorld()->GetTimerManager().ClearAllTimersForObject(this);

	if (!bNoResetDataOnFinish)
		ResetData();
}

// It is not safe to call GetCharacter() in this function
void UBaseActivity::FinishedActivity_NoOwner(bool bSuccess)
{
	if (!bNoMoveActivity &&	bAbortMoveWhenActivityFinished)
	{
		AbortMove(true);
	}
	
	if (ActivityStateMachine)
		ActivityStateMachine->Shutdown();
	
	OnFinishActivity_NoOwner.Broadcast(this, OwningController);
	OnStartActivity.Clear();
	OnRespondToActivityStart.Clear();
	ActivityStatus = EActivityStatus::Complete;
	
	if (GetWorld())
		GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
	
	if (!bNoResetDataOnFinish)
		ResetData();
}

void UBaseActivity::ResetData()
{
	TimeSinceLastAsyncMove = 0.0f;
	ElapsedActivityTime = 0.0f;
	TimeNotMoving = 0.0f;
	Location = FVector::ZeroVector;
	LastRequestedLocation = FVector::ZeroVector;
	CurrentSpeechIncrement = 0;
	bSearchingPath = false;
	MoveRequestID.Empty();
	PathFailedCount = 0;
	bLastRequestedLocationWaitingForRepath = false;
	LastPathLength = 0.0f;
	LastPauseRequestID = {};
	LastAsyncQueryId = 0;
	MoveAcceptanceRadius = OriginalMoveAcceptanceRadius;
}

float UBaseActivity::GetDestinationTolerance() const
{
	return MoveAcceptanceRadius;
}

FPathFindingResult UBaseActivity::FindPath(const FVector& InLocation)
{
	if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		if (const ANavigationData* NavData = NavSys->GetNavDataForProps(GetCharacter()->GetMovementComponent()->GetNavAgentPropertiesRef()))
		{
			const FSharedConstNavQueryFilter QueryFilter = UNavigationQueryFilter::GetQueryFilter(*NavData, GetNavigationQueryOverride() ? GetNavigationQueryOverride() : OwningController->GetNavQueryFilter());
			const FVector StartLocation = GetCharacter()->GetActorLocation();
			const FVector EndLocation = InLocation;
			
			FNavLocation StartLocationProjected(StartLocation);
			FNavLocation EndLocationProjected(EndLocation);

			NavSys->ProjectPointToNavigation(StartLocation, StartLocationProjected, FVector(75.0f, 75.0f, 200.0f));
			NavSys->ProjectPointToNavigation(EndLocation, EndLocationProjected, FVector(75.0f, 75.0f, 200.0f));
			
			const FPathFindingQuery Query = ACyberneticController::CreatePathFindingQuery(QueryFilter, NavData, StartLocationProjected, EndLocationProjected);
			
			return NavSys->FindPathSync(Query);
		}
	}

	return FPathFindingResult();
}

uint32 UBaseActivity::FindPathAsync(const FVector& InLocation)
{
	if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		if (const ANavigationData* NavData = NavSys->GetNavDataForProps(GetCharacter()->GetMovementComponent()->GetNavAgentPropertiesRef()))
		{
			const FSharedConstNavQueryFilter QueryFilter = UNavigationQueryFilter::GetQueryFilter(*NavData, GetNavigationQueryOverride() ? GetNavigationQueryOverride() : OwningController->GetNavQueryFilter());
			const FVector StartLocation = GetCharacter()->GetNavAgentLocation();
			const FVector EndLocation = InLocation;
			
			FNavLocation StartLocationProjected(StartLocation);
			FNavLocation EndLocationProjected(EndLocation);

			NavSys->ProjectPointToNavigation(StartLocation, StartLocationProjected, FVector(75.0f, 75.0f, 200.0f));
			NavSys->ProjectPointToNavigation(EndLocation, EndLocationProjected, FVector(75.0f, 75.0f, 200.0f));

			FNavPathQueryDelegate NavDelegate;
			NavDelegate.BindUObject(this, &UBaseActivity::OnAsyncPathFound_Internal);

			bSearchingPath = true;
			
			LastRequestedLocation = Location;

			LastAsyncQueryId = NavSys->FindPathAsync(GetCharacter()->GetNavAgentPropertiesRef(), ACyberneticController::CreatePathFindingQuery(QueryFilter, NavData, StartLocationProjected, EndLocationProjected, false, OwningController), NavDelegate, EPathFindingMode::Hierarchical);
			return LastAsyncQueryId;
		}
	}

	return 0;
}

void UBaseActivity::OnAsyncPathFound_Internal(uint32 PathId, ENavigationQueryResult::Type ResultType, FNavPathSharedPtr NavPath)
{
	bSearchingPath = false;
	bLastRequestedLocationWaitingForRepath = false;
	
	// Character may have been deleted between starting the path and the path being found
	if (!OwningController)
	    return;

	if (OwningController->ShouldPostponePathUpdates())
		return;

	if (!GetCharacter())
		return;

	if (NavPath.IsValid())
	{
		if (NavPath->IsPartial() && !bAllowPartialMove)
		{
			AbortActivityOnPathNotFound();
		}
	}

	OnAsyncPathFound(PathId, ResultType, NavPath);
}

void UBaseActivity::OnAsyncPathFound(uint32 PathId, ENavigationQueryResult::Type ResultType, FNavPathSharedPtr NavPath)
{
}

void UBaseActivity::RequestMoveFromPath(const FVector& InLocation, FNavPathSharedPtr NavPath)
{
	const FAIMoveRequest MoveRequest = ACyberneticController::CreateMoveRequest(InLocation, GetMoveAcceptanceRadiusOverride(), OwningController->GetNavQueryFilter(), true, true, false, false, false);
	const FAIRequestID& RequestID = OwningController->RequestMove(MoveRequest, NavPath);
	
	LastNavPath = NavPath;
	LastPathLength = NavPath->GetLength();
	LastRequestedMoveId = RequestID;

	MoveRequestID.Add(FIntVector(InLocation), RequestID);
	
	OwningController->LastRequestID = RequestID;
	OwningController->LastMoveRequest = MoveRequest;

	OwningController->GetTargetingComp()->CalculatePathedAwareness(EPathedAwareness::PA_ActivityLocation, ENavigationQueryResult::Success, NavPath);
}
