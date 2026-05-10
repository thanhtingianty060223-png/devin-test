// Void Interactive, 2020

#include "ArrestTargetActivity.h"

#include "Actors/PairedInteractionDriver.h"

#include "Characters/CyberneticController.h"
#include "Characters/CyberneticCharacter.h"

#include "Info/ReadyOrNotSignificanceManager.h"

UArrestTargetActivity::UArrestTargetActivity()
{
	ActivityName = FText::FromStringTable("SwatCommandTable", "Restrain");
	bIsProgressActivity = true;
	bAbortIfTrackingEnemy = true;
	bPauseIfTrackingEnemy = true;
	bPauseIfRecentlyInCombat = true;
	bAbortActivityIfCannotReachLocation = true;

	ActivityStateMachine->AddState("Move To")
						.BindEventEnter(MAKE_DELEGATE_BINDING(this, &UArrestTargetActivity::EnterMoveToStage))
						.CreateTransition("Arrest", MAKE_DELEGATE_BINDING(this, &UArrestTargetActivity::CanArrest));
	
	ActivityStateMachine->AddState("Arrest")
						.BindEventEnter(MAKE_DELEGATE_BINDING(this, &UArrestTargetActivity::EnterArrestStage))
						.BindEventTick(MAKE_DELEGATE_BINDING(this, &UArrestTargetActivity::TickArrestStage));
}

void UArrestTargetActivity::ActivityOverriden(UBaseActivity* OverridingActivity)
{
	Super::ActivityOverriden(OverridingActivity);

	UnbindEvents();
}

void UArrestTargetActivity::ResumeActivity()
{
	Super::ResumeActivity();

	BindEvents();

	CheckEdgeCases();
}

bool UArrestTargetActivity::CanBeCleared()
{
	return false;
}

bool UArrestTargetActivity::CheckEdgeCases()
{
	if (!ArrestTarget)
	{
		ACTIVITY_FAILED("No arrest target specified");
		return false;
	}

	if (OwningController->GetTrackedTarget() && OwningController->IsCharacterEnemy(OwningController->GetTrackedTarget()))
	{
		ACTIVITY_FAILED("Tracking Enemy", true);
		return false;
	}

	if (!ArrestTarget->CanArrest())
	{
		ACTIVITY_FAILED("Arrest target can't be arrested", true);
		return false;
	}

	if (!HasReachedLocation(GetDestinationTolerance()) && (ArrestTarget->IsArrested() || ArrestTarget->IsArrestedAndDead()))
	{
		ACTIVITY_FAILED("Arrest target is arrested", true);
		return false;
	}
	
	if (!ArrestTarget->GetMesh())
	{
		ACTIVITY_FAILED("Target does not have a valid mesh");
		return false;
	}
	
	if (!ArrestInteraction)
	{
		ACTIVITY_FAILED("No arrest interaction data found");
		return false;
	}
	
	if (BestArrestLocation == FVector::ZeroVector)
	{
		const FVector Direction = (ArrestTarget->GetNavAgentLocation() - GetCharacter()->GetNavAgentLocation()).GetSafeNormal();
		BestArrestLocation = ArrestTarget->GetNavAgentLocation() + Direction * -100.0f;
	}
	
	return true;
}

void UArrestTargetActivity::StartActivity(AAIController* Owner)
{	
	Super::StartActivity(Owner);
	
	BindEvents();
}

void UArrestTargetActivity::PerformActivity(const float DeltaTime)
{
	Super::PerformActivity(DeltaTime);
	
	// if the player got to them before us
	if (ArrestTarget)
	{
		if (ArrestTarget->IsBeingArrested() &&
			ArrestTarget->LastCharacterMakingArrest &&
			ArrestTarget->LastCharacterMakingArrest != GetCharacter())
		{
			OwningController->FinishActivity(this, true, true);
		}
	}
}

void UArrestTargetActivity::FinishedActivity(const bool bSuccess)
{
	Super::FinishedActivity(bSuccess);

	UnbindEvents();

	EquipWeapon();
	
	if (ArrestTarget)
	{
		UReadyOrNotSignificanceManager::RegisterActorWithSignificanceManager(ArrestTarget);
	}
}

bool UArrestTargetActivity::CanFinishActivity() const
{
	// Must be force finished
	return false;
}

bool UArrestTargetActivity::CanShoot() const
{
	return true;
}

void UArrestTargetActivity::RequestMoveFromPath(const FVector& InLocation, FNavPathSharedPtr NavPath)
{
	Super::RequestMoveFromPath(InLocation, NavPath);

	if (!bCalledOutMove)
	{
		bCalledOutMove = true;
		
		if (ArrestTarget)
		{
			GetCharacter()->PlayRawVO(ArrestTarget->IsSuspect() ? VO_SWAT_GENERAL::CALL_ARRESTING_SUSPECT_MOVE : VO_SWAT_GENERAL::CALL_ARRESTING_CIVILIAN_MOVE);
		}
	}
}

float UArrestTargetActivity::GetDestinationTolerance() const
{
	return 30.0f;
}

#if !UE_BUILD_SHIPPING
void UArrestTargetActivity::PerformActivity_Debug(float DeltaTime)
{
	Super::PerformActivity_Debug(DeltaTime);
	
	//DrawDebugString(GetWorld(), BestArrestLocation, "Arrest Location", nullptr, FColor::White, DeltaTime);
	//DrawDebugBox(GetWorld(), BestArrestLocation, FVector(15.0f), FColor::Yellow, false, DeltaTime, 0);
}
#endif

void UArrestTargetActivity::EnterMoveToStage()
{
	if (ArrestTarget)
	{
		ArrestInteraction = ChooseRandomArrestInteraction();

		BestArrestLocation = TryFindReachableLocationToArrestFrom();

		UReadyOrNotSignificanceManager::UnregisterActorWithSignificanceManager(ArrestTarget);
	}
	else
	{
		ACTIVITY_FAILED("No arrest target specified");
		return;
	}
	
	if (!CheckEdgeCases())
		return;

	SetLocation(BestArrestLocation, true);
	
	ProgressState = FText::FromStringTable("SwatCommandTable", "PreparingForRestrain");
}

void UArrestTargetActivity::EnterArrestStage()
{
}

void UArrestTargetActivity::TickArrestStage(const float DeltaTime, const float Uptime)
{
	if (UInteractionsData::IsPairedInteractionPlayingOn(GetCharacter()))
	{
		return;
	}
	
	if (GetCharacter()->CurrentlyArresting == ArrestTarget)
	{
		bAbortIfTrackingEnemy = false;
		return;
	}
	
	if (ArrestTarget->IsArrested() || ArrestTarget->IsArrestedAndDead())
	{
		GetCharacter()->Server_ReportToTOC_Implementation(ArrestTarget);
		OwningController->FinishActivity(this, true, true);
		return;
	}

	if (!CheckEdgeCases())
		return;
	
	ProgressState = FText::FromStringTable("SwatCommandTable", "RestrainingSuspect");
	
	if (ArrestTarget->IsCivilian())
		ProgressState = FText::FromStringTable("SwatCommandTable", "RestrainingCivilian");

	if (ArrestTarget->GetController<ACyberneticController>())
	{
		ArrestTarget->GetController<ACyberneticController>()->AbortMove();
	}

	GetCharacter()->DoArrestWithZipcuffs(ArrestTarget);
}

bool UArrestTargetActivity::CanArrest() const
{
	if (!ArrestTarget)
		return false;

	if (BestArrestLocation == FVector::ZeroVector)
		return false;
		
	return HasReachedLocation(GetDestinationTolerance());
}

bool UArrestTargetActivity::OverrideFocalPoint(FVector& FocalPoint)
{
	if (ArrestTarget && FVector::Distance(ArrestTarget->GetActorLocation(), GetCharacter()->GetActorLocation()) < 500.0f)
	{
		FocalPoint = ArrestTarget->GetActorLocation();
		return true;
	}
	
	return false;
}

bool UArrestTargetActivity::ShouldForceStrafe() const
{
	return !HasReachedLocation(150.0f);
}

bool UArrestTargetActivity::ShouldForceNoStrafe() const
{
	return HasReachedLocation(150.0f);
}

bool UArrestTargetActivity::CanOverrideActivity() const
{
	return false;
}

void UArrestTargetActivity::ResetData()
{
	Super::ResetData();

	ArrestTarget = nullptr;
	ArrestInteraction = nullptr;
	BestArrestLocation = FVector::ZeroVector;
	bCalledOutMove = false;
}

void UArrestTargetActivity::OnArresterKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter)
{
	ACTIVITY_FAILED("Arrest Target is dead", true);
}

void UArrestTargetActivity::BindEvents()
{
	UnbindEvents();
	
	GetCharacter()->OnCharacterKilled.AddDynamic(this, &UArrestTargetActivity::OnArresterKilled);
}

void UArrestTargetActivity::UnbindEvents()
{
	GetCharacter()->OnCharacterKilled.RemoveAll(this);
}

FVector UArrestTargetActivity::TryFindReachableLocationToArrestFrom() const
{
	if (!ArrestTarget || !ArrestInteraction)
		return FVector::ZeroVector;

	TArray<FVector> OutArrestLocations;
	FHitResult Hit, LOSTest;
	FCollisionObjectQueryParams CollisionObjectQueryParams;
	CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_DOOR);
	CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_DOORWAY);
	
	const FVector Start = ArrestTarget->GetMesh()->GetBoneLocation("spine_1");
	//const FVector End =  Start + (ArrestTarget->GetActorForwardVector() * -1.0f) * ArrestInteraction->RelativePosOffsetToDriver.Y + ArrestTarget->GetActorRightVector() * ArrestInteraction->RelativePosOffsetToDriver.X;

	// first, try behind the AI to test for clear space. if not trace in a circle until a clear space is found
	if (!ArrestTarget->IsDeadOrUnconscious() && !ArrestTarget->IsInRagdoll())
	{
		GetWorld()->LineTraceSingleByObjectType(LOSTest, Start, Start + ArrestTarget->GetActorForwardVector() * -200.0f, CollisionObjectQueryParams);
		if (!LOSTest.bBlockingHit)
		{
			FVector BoxStart = Start + ArrestTarget->GetActorForwardVector() * -200.0f + FVector(0.0f, 0.0f, 30.0f);
			BoxStart.Z += 30.0f;
			
			UKismetSystemLibrary::BoxTraceSingleForObjects(this, BoxStart, BoxStart, FVector(50.0f, 50.0f, 50.0f), FRotator::ZeroRotator, {UEngineTypes::ConvertToObjectType(ECC_WorldStatic), UEngineTypes::ConvertToObjectType(ECC_WorldDynamic), UEngineTypes::ConvertToObjectType(ECC_DOORWAY)}, false, {ArrestTarget}, EDrawDebugTrace::ForOneFrame, Hit, true);
			DrawDebugLine(GetWorld(), Hit.TraceStart, Hit.TraceEnd, Hit.bBlockingHit ? FColor::Red : FColor::Green, false, 1.0f, 0, 1.0f);
			if (!Hit.bBlockingHit)
			{
				FVector BestDirection = UKismetMathLibrary::FindLookAtRotation(Start, Hit.TraceEnd).Vector();
				FVector IdealLocation = ArrestTarget->GetActorLocation() + BestDirection * 150.0f + BestDirection *  -ArrestInteraction->RelativePosOffsetToDriver.X;
				IdealLocation.Z = Start.Z;

				return IdealLocation;
			}
		}
	}
	
	const float Angle = 45.0f;
	const int32 NumberOfTraces = 360/Angle;
	for (int32 i = 0; i < NumberOfTraces; i++)
	{
		GetWorld()->LineTraceSingleByObjectType(LOSTest, Start, Start + FRotator(0.0f, i * Angle - 45.0f, 0.0f).Vector() * ArrestInteraction->RelativePosOffsetToDriver.Y * 1.25f, CollisionObjectQueryParams);
		DrawDebugLine(GetWorld(), LOSTest.TraceStart, LOSTest.TraceEnd, LOSTest.bBlockingHit ? FColor::Red : FColor::Green, false, 1.0f, 0, 1.0f);
		if (!LOSTest.bBlockingHit)
		{
			FVector BoxStart = Start + FRotator(0.0f, i * Angle - 45.0f, 0.0f).Vector() * ArrestInteraction->RelativePosOffsetToDriver.Y  + FVector(0.0f, 0.0f, 30.0f);
			BoxStart.Z += 30.0f;
			UKismetSystemLibrary::BoxTraceSingleForObjects(this, BoxStart, BoxStart , FVector(50.0f, 50.0f, 50.0f), FRotator::ZeroRotator, {UEngineTypes::ConvertToObjectType(ECC_WorldStatic), UEngineTypes::ConvertToObjectType(ECC_WorldDynamic), UEngineTypes::ConvertToObjectType(ECC_DOORWAY)}, false, {}, EDrawDebugTrace::ForOneFrame, Hit, true);
			DrawDebugLine(GetWorld(), Hit.TraceStart, Hit.TraceEnd, Hit.bBlockingHit ? FColor::Red : FColor::Green, false, 1.0f, 0, 1.0f);
			if (!Hit.bBlockingHit)
			{
				//V_LOGM(LogReadyOrNot, "Best Direction Dist: %f", BestHit.Distance);
				FVector BestDirection = UKismetMathLibrary::FindLookAtRotation(Start, Hit.TraceEnd).Vector();
				//FVector IdealLocation = Start + BestDirection.ForwardVector * ArrestInteraction->RelativePosOffsetToDriver.Y + BestDirection.RightVector * ArrestInteraction->RelativePosOffsetToDriver.X;
				FVector IdealLocation = ArrestTarget->GetActorLocation() + BestDirection * ArrestInteraction->RelativePosOffsetToDriver.Y + BestDirection *  -ArrestInteraction->RelativePosOffsetToDriver.X;
				IdealLocation.Z = Start.Z;
				OutArrestLocations.Add(IdealLocation);
			}		
		}		
	}

	if (OutArrestLocations.Num() == 0)
	{
		return FVector::ZeroVector;
	}
	
	// get closest
	float ClosestDist = BIG_DIST;
	FVector ClosestLoc = FVector::ZeroVector;
	for (FVector loc : OutArrestLocations)
	{
		float Dist = (loc - GetCharacter()->GetActorLocation()).SizeSquared();
		if (Dist < ClosestDist)
		{
			ClosestDist = Dist;
			ClosestLoc = loc;
		}
	}
	
	return ClosestLoc;
}

UInteractionsData* UArrestTargetActivity::ChooseRandomArrestInteraction() const
{
	UInteractionsData* ChosenInteractionData = nullptr;
	
	if (AZipcuffs* Zipcuffs = GetCharacter()->GetInventoryComponent()->GetInventoryItemOfClass_Native<AZipcuffs>(AZipcuffs::StaticClass()))
	{
		if (ArrestTarget->IsSuspect())
		{
			const int32 Index = FMath::RandRange(0, Zipcuffs->StandingArrestInteractionSuspects.Num() - 1);
			if (Zipcuffs->StandingArrestInteractionSuspects.IsValidIndex(Index))
			{
				ChosenInteractionData = Zipcuffs->StandingArrestInteractionSuspects[Index];
			}
		}
		else
		{
			const int32 Index = FMath::RandRange(0, Zipcuffs->StandingArrestInteractionCivilians.Num() - 1);
			if (Zipcuffs->StandingArrestInteractionCivilians.IsValidIndex(Index))
			{
				ChosenInteractionData = Zipcuffs->StandingArrestInteractionCivilians[FMath::RandRange(0, Zipcuffs->StandingArrestInteractionCivilians.Num() - 1)];
			}
		}
		
		Zipcuffs->ForcedInteractionData = ChosenInteractionData;
	}

	return ChosenInteractionData;
}
