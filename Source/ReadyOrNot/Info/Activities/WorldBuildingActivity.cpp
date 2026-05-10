// Copyright Void Interactive, 2021

#include "WorldBuildingActivity.h"

#include "Characters/CyberneticController.h"
#include "Characters/CyberneticCharacter.h"

#include "Info/SuspectsAndCivilianManager.h"

UWorldBuildingActivity::UWorldBuildingActivity()
{
	bAbortActivityIfCannotReachLocation = true;
	bAbortIfTrackingEnemy = false; // Dont want to immediately finish the activity, handle it here instead
	bAbortIfNotMovingForAWhile = false;

	ActivityStateMachine->AddState("Move To")
						.BindEventEnter(MAKE_DELEGATE_BINDING(this, &UWorldBuildingActivity::EnterMoveToState))
						.BindEventTick(MAKE_DELEGATE_BINDING(this, &UWorldBuildingActivity::TickMoveToState))
						.CreateTransition("Start", MAKE_DELEGATE_BINDING(this, &UWorldBuildingActivity::ShouldStart))
						.CreateTransition("Complete", MAKE_DELEGATE_BINDING(this, &UWorldBuildingActivity::ShouldEnd));
	
	ActivityStateMachine->AddState("Start")
						.BindEventEnter(MAKE_DELEGATE_BINDING(this, &UWorldBuildingActivity::EnterStartState))
						.CreateTransition("End", MAKE_DELEGATE_BINDING(this, &UWorldBuildingActivity::ShouldEnd), 3)
						.CreateTransition("AbruptEnd", MAKE_DELEGATE_BINDING(this, &UWorldBuildingActivity::ShouldEndAbruptly), 2)
						.CreateTransition("Loop", MAKE_DELEGATE_BINDING(this, &UWorldBuildingActivity::ShouldLoop), 1)
						.CreateTransition("Complete", MAKE_DELEGATE_BINDING(this, &UWorldBuildingActivity::ShouldComplete), 0);
	
	ActivityStateMachine->AddState("Loop")
						.BindEventEnter(MAKE_DELEGATE_BINDING(this, &UWorldBuildingActivity::EnterLoopState))
						.BindEventTick(MAKE_DELEGATE_BINDING(this, &UWorldBuildingActivity::TickLoopState))
						.CreateTransition("End", MAKE_DELEGATE_BINDING(this, &UWorldBuildingActivity::ShouldEnd), 0)
						.CreateTransition("AbruptEnd", MAKE_DELEGATE_BINDING(this, &UWorldBuildingActivity::ShouldEndAbruptly), 1);
						
	ActivityStateMachine->AddState("End")
						.BindEventEnter(MAKE_DELEGATE_BINDING(this, &UWorldBuildingActivity::EnterEndState))
						.CreateTransition("Complete", MAKE_DELEGATE_BINDING(this, &UWorldBuildingActivity::ShouldComplete));

	ActivityStateMachine->AddState("AbruptEnd")
						.BindEventEnter(MAKE_DELEGATE_BINDING(this, &UWorldBuildingActivity::EnterAbruptEndState))
						.CreateTransition("Complete", MAKE_DELEGATE_BINDING(this, &UWorldBuildingActivity::ShouldComplete));
	
	ActivityStateMachine->AddState("Complete")
						.BindEventEnter(MAKE_DELEGATE_BINDING(this, &UWorldBuildingActivity::EnterCompleteState));
}

void UWorldBuildingActivity::StartActivity(AAIController* Owner)
{
	Super::StartActivity(Owner);

	if (!IsSetupCorrectly())
	{
		ACTIVITY_FAILED(GetName() + " is not setup correctly");
		return;
	}
	
	OwningController->bStopDecisionMaking = true;
}

void UWorldBuildingActivity::PerformActivity(const float DeltaTime)
{
	Super::PerformActivity(DeltaTime);

	if (!OwningController || !GetCharacter())
		return;

	if (!IsSetupCorrectly())
	{
		ACTIVITY_FAILED(GetName() + " is not setup correctly");
		return;
	}
	
	bHasHolsteredWeapon = !GetCharacter()->GetEquippedItem();

	if (!OwningController->IsMoving())
	{
		TimeAtWorldBuildingLocation += DeltaTime;
	}
	
	if (!bHasPlayedStartSpeech)
	{
		bHasPlayedStartSpeech = true;
		GetCharacter()->PlayBarkOrStartConversation(StartActivitySpeech, true);
	}
	
	if (OwningController->GetAwarenessState() == EAIAwarenessState::Alerted)
	{
		bAbortNow = true;
	}
}

void UWorldBuildingActivity::FinishedActivity(const bool bSuccess)
{
	GetCharacter()->ReasonsToStandStill.Remove("World Building");
	
	GetCharacter()->StopTPMontageFromTable(TableMontageName);
	GetCharacter()->StopTPMontage(TableMontageAnim);
	GetCharacter()->StopTPMontage(MontageStart);
	GetCharacter()->StopTPMontage(MontageEnd);

	if (bShouldSurrenderFromActivity && bAbortDueToPendingSurrender)
	{
		GetCharacter()->Surrender();
	}
	else
	{
		GetCharacter()->PlayBarkOrStartConversation(FinishActivitySpeech, true);
		
		TryDrawWeapon();
	}
	
	OwningController->bStopDecisionMaking = false;
	
	Super::FinishedActivity(bSuccess);
}

void UWorldBuildingActivity::ResetData()
{
	Super::ResetData();

	TableMontageAnim = nullptr;
	
	TimeAtWorldBuildingLocation = 0.0f;
	TotalTimeUntilEnd = 0.0f;

	TimeDoingWorldBuilding = 0.0f;

	bHasHolsteredWeapon = false;
	bShouldCompleteNow = false;
	bShouldLoopNow = false;
	bHasPlayedStartSpeech = false;
	
	bLocationMatch = false;
	bRotationMatch = false;
	
	bAbortDueToPendingSurrender = false;
	bAbortNow = false;
}

bool UWorldBuildingActivity::CanFinishActivity() const
{
	return false; // Force finished
}

bool UWorldBuildingActivity::CanShoot() const
{
	return false;
}

bool UWorldBuildingActivity::OverrideFocalPoint(FVector& FocalPoint)
{
	if (GetActiveStateID() == 0)
	{
		if (HasReachedLocation(GetDestinationTolerance()))
		{
			FocalPoint = GetCharacter()->GetActorLocation() + GetRotationOffset().Vector() * 500.0f;
			return true;
		}
	}
	
	if (GetActiveStateID() > 0)
	{
		FocalPoint = GetCharacter()->GetActorLocation() + GetRotationOffset().Vector() * 500.0f;
		return true;
	}
	
	return false;
}

bool UWorldBuildingActivity::CanBePushed() const
{
	return GetActiveStateID() > 0;
}

bool UWorldBuildingActivity::CanBeCleared()
{
	return GetActiveStateID() == 0; // Is moving to the world building location?
}

bool UWorldBuildingActivity::CanOverrideActivity() const
{
	return GetActiveStateID() != 0; // Is moving to the world building location?
}

bool UWorldBuildingActivity::TryHolsterWeapon()
{
	if (!bShouldHolsterWeapon)
		return false;
	
	if (GetCharacter()->IsTableMontagePlaying("tp_holster_generic"))
	{
		return false;
	}
	
	if (const ABaseItem* EquippedItem = GetCharacter()->GetEquippedItem())
	{
		if (EquippedItem->ItemClass == EItemClass::IC_Melee)
		{
			bHasHolsteredWeapon = true;
			GetCharacter()->GetInventoryComponent()->HolsterEquippedItem(true);
		}
		else
		{
			GetCharacter()->PlayMontageFromTable("tp_holster_generic");
		}
	}

	return true;
}

bool UWorldBuildingActivity::TryDrawWeapon()
{
	if (GetCharacter()->GetEquippedItem())
		return false;
	
	GetCharacter()->StopTPMontageFromTable(TableMontageName);
	GetCharacter()->StopTPMontage(TableMontageAnim);
	GetCharacter()->StopTPMontage(MontageStart);
	GetCharacter()->StopTPMontage(MontageEnd);

	if (const ABaseItem* HolsteredItem = GetCharacter()->GetInventoryComponent()->GetHolsteredItem())
	{
		if (HolsteredItem->ItemClass == EItemClass::IC_Melee)
		{
			GetCharacter()->GetInventoryComponent()->EquipHolsteredItem(true);
			return true;
		}

		FString Animation = "tp_draw";
		
		if (HolsteredItem->ItemClass == EItemClass::IC_Pistol)
		{
			Animation += "_pistol";
		}
		else
		{
			Animation += "_rifle";
		}
		
		GetCharacter()->PlayMontageFromTable(Animation);
	}
	
	return true;
}

bool UWorldBuildingActivity::ShouldEnd() const
{
	return TimeDoingWorldBuilding >= WorldBuildingTime || bAbortNow;
}

bool UWorldBuildingActivity::ShouldEndAbruptly() const
{
	if (bOneShotAnimationDataTable)
		return false;
	
	if (!MontageAbruptEnd)
		return false;
	
	return bAbortDueToPendingSurrender || bAbortNow || GetCharacter()->IsStunned();
}

void UWorldBuildingActivity::EnterMoveToState()
{
	RequestMoveAsync();
}

void UWorldBuildingActivity::TickMoveToState(const float DeltaTime, float Uptime)
{
	if (HasReachedLocation(GetDestinationTolerance()))
	{
		if (GetCharacter()->GetEquippedItem())
			TryHolsterWeapon();
		
		// Magnetize towards the target location, slowly
		{
			const FVector StartLocation = GetCharacter()->GetActorLocation();
			const FVector EndLocation = FVector(Location.X, Location.Y, StartLocation.Z);
		
			GetCharacter()->SetActorLocation(FMath::VInterpTo(StartLocation, EndLocation, DeltaTime, 1.0f));

			if (bRequireRotationMatch)
			{
				const FRotator TargetRotation = FRotator(0.0f, GetRotationOffset().Yaw, 0.0f);

				GetCharacter()->SetActorRotation(FMath::RInterpTo(GetCharacter()->GetActorRotation(), TargetRotation, DeltaTime, 2.0f), ETeleportType::TeleportPhysics);
			}

			//ULog::Info(GetName() + " | Magnetizing..");
		}
	}
}

bool UWorldBuildingActivity::ShouldStart() const
{
	if (bAbortNow)
		return false;

	if (Location == FVector::ZeroVector)
		return false;
	
	if (HasReachedLocation(GetDestinationTolerance()))
	{
		bool bCorrectRotation = !bRequireRotationMatch;

		if (bRequireRotationMatch)
		{
			FRotator CurrentPawnRotation = GetCharacter()->GetActorRotation();
			CurrentPawnRotation.Pitch = 0.0f;
			CurrentPawnRotation.Roll = 0.0f;
			
			const FRotator TestPawnRotation = FRotator(0.0f, GetRotationOffset().Yaw, 0.0f);
			
			if (CurrentPawnRotation.Equals(TestPawnRotation, 10.0f) || !bRequireRotationMatch)
			{
				bCorrectRotation = true;
			}
		}

		if (bShouldHolsterWeapon)
		{
			return bCorrectRotation && (bHasHolsteredWeapon || !GetCharacter()->GetEquippedItem());
		}

		return bCorrectRotation;
	}

	return false;
}

void UWorldBuildingActivity::EnterStartState()
{
	USuspectsAndCivilianManager::Get(this)->PlayBarkOrStartConversation(StartActivitySpeech, GetCharacter()->GetActorLocation());

	TableMontageAnim = nullptr;

	if (bOneShotAnimationDataTable)
	{
		if (UAnimMontage* Montage = GetCharacter()->GetMontageFromTable(TableMontageName))
		{
			TableMontageAnim = Montage;
			
			GetCharacter()->Play3PMontage(Montage);

			const float Delay = Montage->GetPlayLength() - (Montage->GetDefaultBlendOutTime() + 0.2f);
			if (Delay > 0.0f)
			{
				UReadyOrNotFunctionLibrary::StartTimerForCallback(this, &UWorldBuildingActivity::CompleteNow, Delay, false);
			}
			else
			{
				CompleteNow();
			}
		}
		else
		{
			ACTIVITY_FAILED("Unable to play animation from table montage (" + TableMontageName + ")");
			return;
		}

		return;
	}

	if (MontageStart)
	{
		GetCharacter()->Play3PMontage(MontageStart);

		const float Delay = MontageStart->GetPlayLength() - (MontageStart->GetDefaultBlendOutTime() + 0.2f);
		if (Delay > 0.0f)
		{
			UReadyOrNotFunctionLibrary::StartTimerForCallback(this, &UWorldBuildingActivity::LoopNow, Delay, false);
		}
		else
		{
			LoopNow();
		}
	}


	AbortMove(true);

	GetCharacter()->ReasonsToStandStill.AddUnique("World Building");
}

void UWorldBuildingActivity::EnterEndState()
{
	if (bOneShotAnimationDataTable)
	{
		CompleteNow();
		return;
	}

	if (MontageEnd)
	{
		GetCharacter()->Play3PMontage(MontageEnd);

		const float Delay = MontageEnd->GetPlayLength() - (MontageEnd->GetDefaultBlendOutTime() + 0.2f);
		if (Delay > 0.0f)
		{
			UReadyOrNotFunctionLibrary::StartTimerForCallback(this, &UWorldBuildingActivity::CompleteNow, Delay, false);
		}
		else
		{
			CompleteNow();
		}
	}
}

void UWorldBuildingActivity::EnterAbruptEndState()
{
	if (bOneShotAnimationDataTable)
	{
		CompleteNow();
		return;
	}

	if (MontageAbruptEnd)
	{
		GetCharacter()->Play3PMontage(MontageAbruptEnd);

		const float Delay = MontageAbruptEnd->GetPlayLength() - (MontageAbruptEnd->GetDefaultBlendOutTime() + 0.2f);
		if (Delay > 0.0f)
		{
			UReadyOrNotFunctionLibrary::StartTimerForCallback(this, &UWorldBuildingActivity::CompleteNow, Delay, false);
		}
		else
		{
			CompleteNow();
		}
	}
}

void UWorldBuildingActivity::EnterCompleteState()
{
	ULog::Info(ActivityName.ToString() + " | World Building Complete");
	OwningController->FinishActivity(this, true, true);
}

void UWorldBuildingActivity::EnterLoopState()
{
}

void UWorldBuildingActivity::TickLoopState(const float DeltaTime, const float Uptime)
{
	TimeDoingWorldBuilding = Uptime;
}

bool UWorldBuildingActivity::ShouldLoop() const
{
	if (bOneShotAnimationDataTable)
		return false;

	return bShouldLoopNow;
}

bool UWorldBuildingActivity::ShouldComplete() const
{
	return bShouldCompleteNow;
}

void UWorldBuildingActivity::LoopNow()
{
	bShouldLoopNow = true;
}

void UWorldBuildingActivity::CompleteNow()
{
	bShouldCompleteNow = true;
}

void UWorldBuildingActivity::OnAIPerceptionSense_Implementation(ACyberneticController* InSenseController, FAIStimulus Stimulus, AActor*& OutOverrideSensedActor)
{
	if (const AReadyOrNotCharacter* SensedCharacter = Cast<AReadyOrNotCharacter>(OutOverrideSensedActor))
	{
		if (SensedCharacter->IsOnSWATTeam())
		{
			bAbortNow = true;
			
			if (bShouldSurrenderFromActivity)
				bAbortDueToPendingSurrender = true;
		}
	}
}

void UWorldBuildingActivity::OnAIHearingSense_Implementation(ACyberneticController* InSenseController, FAIStimulus Stimulus, AActor*& OutOverrideSensedActor)
{
	 if (OwningController->IsTagAggressiveNoise(Stimulus.Tag))
	 {
	 	bAbortNow = true;
	 }
}

bool UWorldBuildingActivity::IsSetupCorrectly() const
{
	
	if (bOneShotAnimationDataTable)
	{
		return !TableMontageName.IsEmpty();
	}

	if (Loop && MontageStart && MontageEnd)
		return true;

	if (bMoveOnly)
		return true;
	
	return false;
}

float UWorldBuildingActivity::GetDestinationTolerance() const
{
	return 50.0f;
}
