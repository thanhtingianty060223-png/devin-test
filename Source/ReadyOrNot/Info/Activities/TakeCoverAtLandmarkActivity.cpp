// Void Interactive, 2020

#include "Info/Activities/TakeCoverAtLandmarkActivity.h"

#include "ReadyOrNotAIConfig.h"

#include "Actors/CoverLandmark.h"
#include "Actors/CoverLandmarkProxy.h"

#include "Characters/CyberneticCharacter.h"
#include "Characters/CyberneticController.h"
#include "Components/MoraleComponent.h"
#include "Components/InteractableComponent.h"
#include "Info/ReadyOrNotSignificanceManager.h"

TAutoConsoleVariable<int32> CVarCoverLandmarkImmediateNoLookExit(TEXT("CoverLandmark.ImmediateNoLookExit"), 0, TEXT("0 = Don't exit immediately when no-one is looking (use timer instead). 1 = Exit immediately when no-one is looking"));
TAutoConsoleVariable<int32> CVarCoverLandmarkInstantEntry(TEXT("CoverLandmark.InstantEntry"), 0, TEXT("0 = Don't exit immediately when no-one is looking (use timer instead). 1 = Exit immediately when no-one is looking"));
TAutoConsoleVariable<int32> CVarCoverLandmarkInstantExit(TEXT("CoverLandmark.InstantExit"), 0, TEXT("0 = Don't exit immediately when no-one is looking (use timer instead). 1 = Exit immediately when no-one is looking"));

UTakeCoverAtLandmarkActivity::UTakeCoverAtLandmarkActivity()
{
	ActivityName = FText::FromStringTable("SwatCommandTable", "TakeCoverAtLandmark");
	bIsProgressActivity = false;
	bAbortMoveWhenActivityFinished = true;
	bAbortMoveWhenActivityOverriden = true;

	ActivityStateMachine->AddState("Move To Landmark")
						.BindEventEnter(MAKE_DELEGATE_BINDING(this, &UTakeCoverAtLandmarkActivity::EnterMoveToLandmarkState))
						.BindEventTick(MAKE_DELEGATE_BINDING(this, &UTakeCoverAtLandmarkActivity::TickMoveToLandmarkState))
						.CreateTransition("Enter Landmark", MAKE_DELEGATE_BINDING(this, &UTakeCoverAtLandmarkActivity::CanEnterLandmark));

	ActivityStateMachine->AddState("Enter Landmark")
						.BindEventEnter(MAKE_DELEGATE_BINDING(this, &UTakeCoverAtLandmarkActivity::Enter_EnterLandmark_State))
						.BindEventTick(MAKE_DELEGATE_BINDING(this, &UTakeCoverAtLandmarkActivity::Tick_EnterLandmark_State))
						.CreateTransition("Wait", MAKE_DELEGATE_BINDING(this, &UTakeCoverAtLandmarkActivity::ShouldWait));

	ActivityStateMachine->AddState("Wait")
						.BindEventEnter(MAKE_DELEGATE_BINDING(this, &UTakeCoverAtLandmarkActivity::Enter_Wait_State))
						.BindEventTick(MAKE_DELEGATE_BINDING(this, &UTakeCoverAtLandmarkActivity::Tick_Wait_State))
						.CreateTransition("Exit Landmark", MAKE_DELEGATE_BINDING(this, &UTakeCoverAtLandmarkActivity::CanExitLandmark))
						.CreateTransition("Abrupt Exit", MAKE_DELEGATE_BINDING(this, &UTakeCoverAtLandmarkActivity::CanAbruptlyExit));

	ActivityStateMachine->AddState("Exit Landmark")
						.BindEventEnter(MAKE_DELEGATE_BINDING(this, &UTakeCoverAtLandmarkActivity::Enter_ExitLandmark_State))
						.BindEventTick(MAKE_DELEGATE_BINDING(this, &UTakeCoverAtLandmarkActivity::Tick_ExitLandmark_State));
	
	ActivityStateMachine->AddState("Abrupt Exit")
						.BindEventEnter(MAKE_DELEGATE_BINDING(this, &UTakeCoverAtLandmarkActivity::Enter_AbruptExit_State))
						.BindEventTick(MAKE_DELEGATE_BINDING(this, &UTakeCoverAtLandmarkActivity::Tick_AbruptExit_State));
}

// Called from anim notify
void UTakeCoverAtLandmarkActivity::Notify_OnProxyUse()
{
	if (ChosenEntryProxy)
		ChosenEntryProxy->OnProxyUse(GetActiveStateName() == "Enter Landmark");

	if (CoverLandmark && CoverLandmark->IdlePoint)
		CoverLandmark->IdlePoint->OnProxyUse(GetActiveStateName() == "Wait");

	if (ChosenExitProxy)
		ChosenExitProxy->OnProxyUse(GetActiveStateName() == "Exit Landmark");
}

void UTakeCoverAtLandmarkActivity::Broadcast_OnProxyEnd(const bool bSuccess)
{
	if (ChosenEntryProxy)
		ChosenEntryProxy->OnProxyEnd(bSuccess);
	
	if (CoverLandmark && CoverLandmark->IdlePoint)
		CoverLandmark->IdlePoint->OnProxyEnd(bSuccess);

	if (ChosenExitProxy)
		ChosenExitProxy->OnProxyEnd(bSuccess);
}

void UTakeCoverAtLandmarkActivity::TeleportWeapon()
{
	const ABaseItem* EquippedItem = GetCharacter()->GetEquippedItem();
	if (!EquippedItem)
		return;
	
	GetCharacter()->ThrowEquippedItem();

	if (ChosenExitProxy)
		EquippedItem->GetItemMesh()->SetWorldLocation(ChosenExitProxy->GetActorLocation(), false, nullptr, ETeleportType::TeleportPhysics);
}

void UTakeCoverAtLandmarkActivity::StartActivity(AAIController* Owner)
{
	//WaitDuration = InitialWaitDuration;

	// If none was specified, find the closest one
	if (!CoverLandmark)
	{
		CoverLandmark = FindClosestCoverLandmark();
	}

	if (!CoverLandmark)
	{
		ACTIVITY_FAILED("Cover Landmark not found");
		return;
	}

	if (!CoverLandmark->bEnabled)
	{
		ACTIVITY_FAILED(CoverLandmark->GetName() + " is disabled", true);
		return;
	}

	bool bBypassChecks = false;
	#if !UE_BUILD_SHIPPING
	if (const UAIArchetypeData* Archetype = GetCharacter()->GetAIArchetype())
	{
		bBypassChecks = Archetype->Name == "DEBUG Hide Only";
	}
	#endif

	if (!bBypassChecks)
	{
		if (!CoverLandmark->AllowedTeamsForCover.Contains(GetCharacter()->GetTeam()))
		{
			ACTIVITY_FAILED("Can't use Cover Landmark. " + CoverLandmark->GetName() + " does not allow " + RON_ENUM_TO_STRING(ETeamType, GetCharacter()->GetTeam()), true);
			return;
		}

		if (CoverLandmark->IsCooldownActiveFor(OwningController))
		{
			ACTIVITY_FAILED("Can't use Cover Landmark. " + CoverLandmark->GetName() + " has a cooldown active for " + Owner->GetName(), true);
			return;
		}
	}
	
	if (CoverLandmark->OccupiedByController && CoverLandmark->OccupiedByController != Owner)
	{
		ACTIVITY_FAILED(CoverLandmark->GetName() + " is occupied by " + Owner->GetName(), true);
		return;
	}

	if (CoverLandmark->EntryPoints.Num() == 0)
	{
		ACTIVITY_FAILED("No entry points available for " + CoverLandmark->GetName());
		return;
	}

	if (CoverLandmark->ExitPoints.Num() == 0)
	{
		ACTIVITY_FAILED("No exit points available for " + CoverLandmark->GetName());
		return;
	}
	
	if (!CoverLandmark->IdlePoint)
	{
		ACTIVITY_FAILED("No idle point specified for " + CoverLandmark->GetName());
		return;
	}
	
	if (!ChosenEntryProxy)
	{
		ChosenEntryProxy = CoverLandmark->EntryPoints[FMath::RandRange(0, CoverLandmark->EntryPoints.Num() - 1)];
		
		if (!ChosenEntryProxy)
		{
			ACTIVITY_FAILED("No entry point specified");
			return;
		}
	}
	
	if (!ChosenExitProxy)
	{
		ChosenExitProxy = CoverLandmark->ExitPoints[FMath::RandRange(0, CoverLandmark->ExitPoints.Num() - 1)];

		if (!ChosenExitProxy)
		{
			ACTIVITY_FAILED("No exit point specified");
			return;
		}
	}
	
	// Determine which anims we should play (left or right) for Entry and Exit points
	{
		// Entry
		{
			if (CoverLandmark->Entry.bForwardOnly)
			{
				if (CoverLandmark->Entry.bFromTable)
				{
					EntryAnim = GetCharacter()->GetMontageFromTable(CoverLandmark->Entry.ForwardAnimRowName);
				}
				else
				{
					EntryAnim = CoverLandmark->Entry.ForwardAnim;
				}
			}
			else
			{
				if (CoverLandmark->Entry.bFromTable)
				{
					EntryAnim = GetCharacter()->GetMontageFromTable(ChosenEntryProxy->EntryDirection == ECoverLandmarkAnimDirection::Left ? CoverLandmark->Entry.LeftAnimRowName : CoverLandmark->Entry.RightAnimRowName);
				}
				else
				{
					EntryAnim = ChosenEntryProxy->EntryDirection == ECoverLandmarkAnimDirection::Left ? CoverLandmark->Entry.LeftAnim : CoverLandmark->Entry.RightAnim;
				}
			}
		}

		// Exit
		{
			if (CoverLandmark->Exit.bForwardOnly)
			{
				if (CoverLandmark->Exit.bFromTable)
				{
					ExitAnim = GetCharacter()->GetMontageFromTable(CoverLandmark->Exit.ForwardAnimRowName);
				}
				else
				{
					ExitAnim = CoverLandmark->Exit.ForwardAnim;
				}
			}

			else
			{
				if (CoverLandmark->Exit.bFromTable)
				{
					ExitAnim = GetCharacter()->GetMontageFromTable(ChosenExitProxy->ExitDirection == ECoverLandmarkAnimDirection::Left ? CoverLandmark->Exit.LeftAnimRowName : CoverLandmark->Exit.RightAnimRowName);
				}
				else
				{
					ExitAnim = ChosenExitProxy->ExitDirection == ECoverLandmarkAnimDirection::Left ? CoverLandmark->Exit.LeftAnim : CoverLandmark->Exit.RightAnim;
				}
			}
		}

		// Loop
		{
			if (CoverLandmark->Loop.bForwardOnly)
			{
				if (CoverLandmark->Loop.bFromTable)
				{
					LoopEntryAnim = GetCharacter()->GetMontageFromTable(CoverLandmark->Loop.ForwardAnimRowName);
					LoopExitAnim = GetCharacter()->GetMontageFromTable(CoverLandmark->Loop.ForwardAnimRowName);
				}
				else
				{
					LoopEntryAnim = CoverLandmark->Loop.ForwardAnim;
					LoopExitAnim = CoverLandmark->Loop.ForwardAnim;
				}
			}

			else
			{
				if (CoverLandmark->Loop.bFromTable)
				{
					LoopEntryAnim = GetCharacter()->GetMontageFromTable(ChosenEntryProxy->EntryDirection == ECoverLandmarkAnimDirection::Left ? CoverLandmark->Loop.LeftAnimRowName : CoverLandmark->Loop.RightAnimRowName);
					LoopExitAnim = GetCharacter()->GetMontageFromTable(ChosenExitProxy->ExitDirection == ECoverLandmarkAnimDirection::Left ? CoverLandmark->Loop.LeftAnimRowName : CoverLandmark->Loop.RightAnimRowName);
				}
				else
				{
					LoopEntryAnim = ChosenExitProxy->EntryDirection == ECoverLandmarkAnimDirection::Left ? CoverLandmark->Loop.LeftAnim : CoverLandmark->Loop.RightAnim;
					LoopExitAnim = ChosenExitProxy->ExitDirection == ECoverLandmarkAnimDirection::Left ? CoverLandmark->Loop.LeftAnim : CoverLandmark->Loop.RightAnim;
				}
			}
		}
	}

	if (!EntryAnim || !ExitAnim)
	{
		ACTIVITY_FAILED("Missing animations from " + CoverLandmark->GetName());
		return;
	}
	
	for (const TSoftObjectPtr<AStaticMeshActor>& MeshActor : CoverLandmark->IgnoredMeshActors)
	{
		IgnoredMeshActors.AddUnique(MeshActor.LoadSynchronous());
	}
	
	CoverLandmark->OccupiedByController = Owner;
	CoverLandmark->CurrentSwatWithLineOfSight = nullptr;
	
	GetCharacter()->Rep_HidingAnimState = {};
	GetCharacter()->CurrentCoverLandmarkInUse = CoverLandmark;
	UReadyOrNotSignificanceManager::UnregisterActorWithSignificanceManager(GetCharacter());

	EntryAnimTime = EntryAnim->GetPlayLength() - (EntryAnim->GetDefaultBlendOutTime() + 0.05f);
	ExitAnimTime = ExitAnim->GetPlayLength() - (ExitAnim->GetDefaultBlendOutTime() + 0.05f);
	
	BindExitEvents();

	GetCharacter()->ReasonsToSprint.AddUnique("hiding");

	Super::StartActivity(Owner);
}

void UTakeCoverAtLandmarkActivity::ResumeActivity()
{
	Super::ResumeActivity();

	AddIgnoredActors();
	
	BindExitEvents();

	if (CoverLandmark)
		CoverLandmark->OccupiedByController = OwningController;
}

void UTakeCoverAtLandmarkActivity::ActivityOverriden(UBaseActivity* OverridingActivity)
{
	Super::ActivityOverriden(OverridingActivity);

	RemoveIgnoredActors();

	UnbindExitEvents();

	if (CoverLandmark)
		CoverLandmark->OccupiedByController = nullptr;
	
	GetCharacter()->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
#if defined(PLATFORM_XB1) || defined(PLATFORM_PS4)
	GetCharacter()->GetCharacterMovement()->SetMovementMode(MOVE_NavWalking);
#else
	GetCharacter()->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
#endif
}

void UTakeCoverAtLandmarkActivity::PerformActivity(const float DeltaTime)
{
	OwningController->FinishActivity(this, bShouldExitNow, false);

	if (ActivityStatus != EActivityStatus::Complete)
		Super::PerformActivity(DeltaTime);
}

void UTakeCoverAtLandmarkActivity::FinishedActivity(const bool bSuccess)
{
	OwningController->bStopDecisionMaking = false;
	OwningController->bDisableSensePerception = false;
	
	GetCharacter()->TimeHiding = 0.0f;
	
	GetCharacter()->ReasonsToSprint.Remove("hiding");
	
	GetCharacter()->GetMesh()->SetCastCapsuleIndirectShadow(true);

	EnableProxy();
	
	if (CoverLandmark)
	{
		if (bSuccess)
		{
			//if (!CoverLandmark->bAllowAbruptExit)
				Broadcast_OnProxyEnd(!bMoveToExit);

			CoverLandmark->AddCooldownFor(OwningController, CoverLandmark->CooldownAfterUse);
		}

		RemoveIgnoredActors();

		CoverLandmark->OccupiedByController = nullptr;
		CoverLandmark->LastUsedByController = OwningController;

		if (CoverLandmark->bAllowAbruptExit)
		{
			GetCharacter()->Rep_HidingAnimState.bIsHiding = false;
			GetCharacter()->Rep_HidingAnimState.bLooping = false;
		}
		else
		{
			GetCharacter()->Rep_HidingAnimState = {};
		}
	}
	else
	{
		GetCharacter()->Rep_HidingAnimState = {};
	}

	GetCharacter()->CurrentCoverLandmarkInUse = nullptr;
	GetCharacter()->LastCoverLandmarkUsed = CoverLandmark;
	UReadyOrNotSignificanceManager::RegisterActorWithSignificanceManager(GetCharacter());

	GetCharacter()->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
#if defined(PLATFORM_XB1) || defined(PLATFORM_PS4)
	GetCharacter()->GetCharacterMovement()->SetMovementMode(MOVE_NavWalking);
#else
	GetCharacter()->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
#endif

	GetCharacter()->SetActorHiddenInGame(false);

	if (bShouldSurrender)
		GetCharacter()->Surrender();

	GetCharacter()->TimeSinceAtLastCoverLandmark = 0.0f;
	GetCharacter()->bIsExitingLandmark = false;

	UnbindExitEvents();

	Super::FinishedActivity(bSuccess);
}

void UTakeCoverAtLandmarkActivity::FinishedActivity_NoOwner(const bool bSuccess)
{
	EnableProxy();
	
	if (CoverLandmark)
	{
		if (bSuccess)
		{
			Broadcast_OnProxyEnd(true);
			CoverLandmark->AddCooldownFor(OwningController, CoverLandmark->CooldownAfterUse);
		}

		CoverLandmark->OccupiedByController = nullptr;
		CoverLandmark->LastUsedByController = OwningController;
	}
	
	Super::FinishedActivity_NoOwner(bSuccess);
}

bool UTakeCoverAtLandmarkActivity::CanFinishActivity() const
{
	if (!CoverLandmark)
		return true;

	return GetActiveStateName() == "Exit Landmark" && !GetCharacter()->Rep_HidingAnimState.bIsHiding;
}

bool UTakeCoverAtLandmarkActivity::CanShoot() const
{
	return GetActiveStateID() == 0; // Moving to the landmark
}

bool UTakeCoverAtLandmarkActivity::ShouldForceStrafe() const
{
	return GetActiveStateID() == 0; // Moving to the landmark
}

bool UTakeCoverAtLandmarkActivity::ShouldForceNoStrafe() const
{
	return GetActiveStateID() > 0; // Waiting at the landmark
}

bool UTakeCoverAtLandmarkActivity::CanOverrideActivity() const
{
	return GetActiveStateID() == 0; // Move To Landmark state
}

bool UTakeCoverAtLandmarkActivity::OverrideFocalPoint(FVector& FocalPoint)
{
	if (GetActiveStateID() == 0) // Move To Landmark state
	{
		if (!ChosenEntryProxy)
			return false;

		if (HasReachedEntryLocation(100.0f))
		{
			FocalPoint = ChosenEntryProxy->GetActorLocation() + ChosenEntryProxy->GetActorRotation().Vector() * 500.0f;
			FocalPoint.Z = GetCharacter()->GetActorLocation().Z;
			return true;
		}
		
		const float DistanceToLandmark = FVector::Distance(GetCharacter()->GetActorLocation(), ChosenEntryProxy->GetActorLocation());

		if (DistanceToLandmark < 500.0f || HasReachedLocation(150.0f))
		{
			const FCollisionQueryParams CollisionQueryParams = GetCharacter()->GetCollisionQueryParameters();
			
			FVector TraceStart = GetCharacter()->GetActorLocation();
			FVector TraceEnd = ChosenEntryProxy->GetActorLocation();
			TraceEnd.Z = TraceStart.Z;

			const bool bHasLOS = !GetWorld()->LineTraceTestByChannel(TraceStart, TraceEnd, ECC_Visibility, CollisionQueryParams);

			if (bHasLOS)
			{
				FocalPoint = ChosenEntryProxy->GetActorLocation();
				FocalPoint.Z = TraceStart.Z;
			}

			return true;
		}

		if (OwningController->GetTrackedTarget())
		{
			FocalPoint = OwningController->GetTrackedTarget()->GetActorLocation();
			return true;
		}
		
		return false;
	}
	
	FocalPoint = FVector::ZeroVector;
	return true;
}

void UTakeCoverAtLandmarkActivity::EnterMoveToLandmarkState()
{
	// Move to entry point of landmark
	SetLocation(ChosenEntryProxy->GetActorLocation(), true);
}

void UTakeCoverAtLandmarkActivity::TickMoveToLandmarkState(float DeltaTime, float Uptime)
{
	GetCharacter()->Rep_HidingAnimState.bIsHiding = false;
	GetCharacter()->Rep_HidingAnimState.bLooping = false;

	if (!OwningController->IsMoving())
	{
		SetLocation(ChosenEntryProxy->GetActorLocation(), true);
	}
	
	if (HasReachedEntryLocation(50.0f))
	{
    	GetCharacter()->SetActorRotation(ChosenEntryProxy->GetActorRotation() + FRotator(0.0f, CoverLandmark->Entry.AnimYawOffset, 0.0f));
	}

	// An exit event (yell, stun, etc..) was triggered, abort activity
	if (bShouldExitNow || IsAnySWATLookingAtLandmark())
	{
		OwningController->FinishActivity(this, false, true);
		return;
	}
}

void UTakeCoverAtLandmarkActivity::Enter_EnterLandmark_State()
{
	OwningController->bStopDecisionMaking = true;
	OwningController->bDisableSensePerception = true;
	
	CoverLandmark->OccupiedByController = OwningController;
	CoverLandmark->bClearedBySwat = false;
	bIsHiding = true;

	AddIgnoredActors();
	
	// Don't try to move back
	SetLocation(FVector::ZeroVector);

	GetCharacter()->Play3PMontage(EntryAnim);

	UpdateHidingState();
	
	GetCharacter()->Rep_HidingAnimState.bLooping = true;
	
	DisableProxy();

	GetCharacter()->GetMesh()->SetCastCapsuleIndirectShadow(false);

	#if !UE_BUILD_SHIPPING
	if (CVarCoverLandmarkInstantEntry.GetValueOnAnyThread() > 0)
	{
		GetCharacter()->Multicast_Stop3PMontage(EntryAnim, 0.0f);
		GetCharacter()->SetActorLocation(FVector(CoverLandmark->IdlePoint->GetActorLocation().X, CoverLandmark->IdlePoint->GetActorLocation().Y, GetCharacter()->GetActorLocation().Z));
		GetCharacter()->SetActorRotation(CoverLandmark->IdlePoint->GetActorRotation());
	}
	#endif
}

void UTakeCoverAtLandmarkActivity::Tick_EnterLandmark_State(const float DeltaTime, const float Uptime)
{
	SetLocation(FVector::ZeroVector);

	AbortMove();
	
	TimeEnteringLandmark = Uptime;
	TimeWaiting = 0.0f;
	bIsHiding = true;

	GetCharacter()->Rep_HidingAnimState.bLooping = true;

	if (!ChosenEntryProxy || !EntryAnim)
		return;
	
	if (!EntryAnim->HasRootMotion())
	{
		GetCharacter()->SetActorLocation(FMath::VInterpConstantTo(GetCharacter()->GetActorLocation(), FVector(CoverLandmark->IdlePoint->GetActorLocation().X, CoverLandmark->IdlePoint->GetActorLocation().Y, GetCharacter()->GetActorLocation().Z), DeltaTime, 75.0f));

		const FRotator Target = FMath::Lerp(ChosenEntryProxy->GetActorRotation() + FRotator(0.0f, CoverLandmark->Entry.AnimYawOffset, 0.0f), CoverLandmark->IdlePoint->GetActorRotation(), FMath::Clamp(Uptime/(EntryAnimTime/1.5f), 0.0f, 1.0f));
		GetCharacter()->SetActorRotation(Target);
		//GetCharacter()->SetActorRotation(FMath::RInterpConstantTo(GetCharacter()->GetActorRotation(), Target, DeltaTime, 200.0f));
		
		return;
	}
	
	if (LoopEntryAnim)
	{
		if (GetCharacter()->Is3PMontagePlaying(EntryAnim))
		{
			GetCharacter()->SetActorLocation(FMath::VInterpConstantTo(GetCharacter()->GetActorLocation(), FVector(ChosenEntryProxy->GetActorLocation().X, ChosenEntryProxy->GetActorLocation().Y, GetCharacter()->GetActorLocation().Z), DeltaTime, 75.0f));
			
			const FRotator Target = FMath::Lerp(ChosenEntryProxy->GetActorRotation() + FRotator(0.0f, CoverLandmark->Entry.AnimYawOffset, 0.0f), CoverLandmark->IdlePoint->GetActorRotation(), FMath::Clamp(Uptime/(EntryAnimTime/1.5f), 0.0f, 1.0f));
			GetCharacter()->SetActorRotation(Target);
			//GetCharacter()->SetActorRotation(FMath::RInterpConstantTo(GetCharacter()->GetActorRotation(), Target, DeltaTime, 200.0f));
			
			return;
		}

		// Play entry loop anim over and over
		if (!GetCharacter()->Is3PMontagePlaying(LoopEntryAnim))
		{
			GetCharacter()->Play3PMontage(LoopEntryAnim);
		}
	}
}

void UTakeCoverAtLandmarkActivity::Enter_Wait_State()
{
	GetCharacter()->Rep_HidingAnimState.bLooping = true;

	GetCharacter()->GetCharacterMovement()->StopMovementImmediately();

	GetCharacter()->Multicast_ChangeFaceEmotion(ECharacterEmotion::Afraid, 30.0f, 1.0f, 0.1f, 10);

	EnableProxy();
	
	GetCharacter()->GetMesh()->SetCastCapsuleIndirectShadow(false);
}

void UTakeCoverAtLandmarkActivity::Tick_Wait_State(const float DeltaTime, const float Uptime)
{
	SetLocation(FVector::ZeroVector);

	AbortMove();

	//if (CoverLandmark->bCharacterHiddenInWaitingState)
	//{
	//	GetCharacter()->SetActorHiddenInGame(true);
	//}
	
	TimeWaiting = Uptime;
	TimeEnteringLandmark = 0.0f;
	bIsHiding = true;

	GetCharacter()->Rep_HidingAnimState.bLooping = true;

	// Set to no collision so that SetActorLocation doesn't move us out of the cover object
	GetCharacter()->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	GetCharacter()->GetCharacterMovement()->DisableMovement();
	
	GetCharacter()->TimeHiding = TimeWaiting;
	GetCharacter()->TimeSinceLastSeenCharacterWhilstHiding += DeltaTime;
	GetCharacter()->TimeSinceSeenCharacterNotLookingWhilstHiding += DeltaTime;
	//LOG_NUMBER(GetCharacter()->TimeSinceLastSeenCharacterWhilstHiding);

	TimeSinceLastVisionTrace += DeltaTime;
	if (TimeSinceLastVisionTrace > 0.5f)
	{
		TimeSinceLastVisionTrace = 0.0f;

		CoverLandmark->CurrentSwatWithLineOfSight = nullptr;
		GetCharacter()->CharacterSeenWhilstHiding = nullptr;
		for (AReadyOrNotCharacter* Character : GetWorld()->GetGameState<AReadyOrNotGameState>()->AllReadyOrNotCharacters)
		{
			if (IsValid(Character) && Character->IsActive())
			{
				// some optimizations
				if (FVector::Distance(Character->GetActorLocation(), CoverLandmark->GetActorLocation()) < 2000.0f &&
					FVector::DotProduct(GetCharacter()->GetActorForwardVector(), (Character->GetActorLocation() - CoverLandmark->GetActorLocation()).GetSafeNormal2D()) > -0.5f)
				{
					FHitResult Hit;
					FCollisionQueryParams CollisionQueryParams = UReadyOrNotFunctionLibrary::CreateCollisionQueryParams(GetCharacter(), Character);
					CollisionQueryParams.AddIgnoredActor(CoverLandmark);
					CollisionQueryParams.AddIgnoredActors((TArray<AActor*>)IgnoredMeshActors);

					bool bHasLOS = !GetWorld()->LineTraceSingleByChannel(Hit, CoverLandmark->GetActorLocation(), Character->GetActorLocation(), ECC_Visibility, CollisionQueryParams);
					//DrawDebugLine(GetWorld(), CoverLandmark->GetActorLocation(), Character->GetActorLocation(), bHasLOS ? FColor::Green : FColor::Red, false, 0.5f);
					if (bHasLOS)
					{
						GetCharacter()->bHasEverSeenCharacterWhilstHiding = true;
						GetCharacter()->TimeSinceLastSeenCharacterWhilstHiding = 0.0f;
						GetCharacter()->CharacterSeenWhilstHiding = Character;
						CoverLandmark->CurrentSwatWithLineOfSight = Character;
						break;
					}
				}
			}
		}
		
		if (AReadyOrNotCharacter* Character = CoverLandmark->CurrentSwatWithLineOfSight)
		{
			const FVector DirectionToLandmark = (CoverLandmark->GetActorLocation() - Character->GetActorLocation()).GetSafeNormal2D();
			const float DotProduct = FVector::DotProduct(Character->GetActorForwardVector(), DirectionToLandmark);

			if (DotProduct > 0.7f)
			{
				GetCharacter()->TimeSinceSeenCharacterNotLookingWhilstHiding = 0.0f;
			}
		}
	}

	if (CoverLandmark->IdlePoint)
	{
		//const FVector TestLocation = FVector(GetCharacter()->GetActorLocation().X, GetCharacter()->GetActorLocation().Y, CoverLandmark->IdlePoint->GetActorLocation().Z);
		
		//FTransform BoundsTransformTest;
		//BoundsTransformTest.SetLocation(CoverLandmark->IdlePoint->GetActorLocation() + CoverLandmark->IdleTriggerBoxTransform.GetLocation());
		//BoundsTransformTest.SetRotation(CoverLandmark->IdleTriggerBoxTransform.GetRotation());
		//BoundsTransformTest.SetScale3D(CoverLandmark->IdleTriggerBoxTransform.GetScale3D());

		//if (!UKismetMathLibrary::IsPointInBoxWithTransform(TestLocation, BoundsTransformTest, CoverLandmark->IdleTriggerBoxExtent))
		//{
			//GetCharacter()->SetActorLocation(FMath::VInterpConstantTo(GetCharacter()->GetActorLocation(), FVector(CoverLandmark->IdlePoint->GetActorLocation().X, CoverLandmark->IdlePoint->GetActorLocation().Y, GetCharacter()->GetActorLocation().Z), DeltaTime, 75.0f));
			GetCharacter()->SetActorRotation(CoverLandmark->IdlePoint->GetActorRotation());
		//}
	}

	if (CoverLandmark->bAllowAbruptExit && IsAnySWATLookingAtUs())
	{
		// Move to exit state
			bMoveToExit = true;
		/*
		if (GetCharacter()->IsCivilian()) // civilians should always exit, suspects can just shoot
		{
		}
		else
		{
			OwningController->FinishActivity(this, false, true);
		}*/
		
		return;
	}
		
	if (IsAnySWATLookingAtLandmark())
	{
		//WaitDuration = SWATSeenLandmarkWaitDuration;
		//TimeSwatNotLookingAtLandmark = 0.0f;
	}
	else
	{
		#if !UE_BUILD_SHIPPING
		if (CVarCoverLandmarkImmediateNoLookExit.GetValueOnAnyThread() > 0)
			bShouldExitNow = true;
		#endif
		
		//TimeSwatNotLookingAtLandmark += DeltaTime;
	}
}

void UTakeCoverAtLandmarkActivity::Enter_AbruptExit_State()
{
#if defined(PLATFORM_XB1) || defined(PLATFORM_PS4)
	GetCharacter()->GetCharacterMovement()->SetMovementMode(MOVE_NavWalking);
#else
	GetCharacter()->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
#endif
	
	GetCharacter()->bIsExitingLandmark = true;

	GetCharacter()->SetActorHiddenInGame(false);
	
	AddIgnoredActors();
	
	// Turn collision back on
	GetCharacter()->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	// Reset cooldown
	CoverLandmark->AddCooldownFor(OwningController, CoverLandmark->CooldownAfterUse);

	Location = FVector::ZeroVector;
	//Location = ChosenExitProxy->GetActorLocation();
	//SetLocation(ChosenExitProxy->GetActorLocation(), true);
	
	GetCharacter()->Rep_HidingAnimState.bIsHiding = false;
	GetCharacter()->Rep_HidingAnimState.bLooping = false;
}

void UTakeCoverAtLandmarkActivity::Tick_AbruptExit_State(float DeltaTime, float Uptime)
{
	//if (HasReachedLocation())
	//{
	//	OwningController->FinishActivity(this, true, true);
	//}
	//else
	{
		//if (!OwningController->IsMoving())
		//{
		//	SetLocation(ChosenExitProxy->GetActorLocation(), true);
		//}
		
		const FVector TestLocation = FVector(GetCharacter()->GetActorLocation().X, GetCharacter()->GetActorLocation().Y, ChosenExitProxy->GetActorLocation().Z);

		FTransform BoundsTransformTest;
		BoundsTransformTest.SetLocation(ChosenExitProxy->GetActorLocation() + CoverLandmark->ExitTriggerBoxTransform.GetLocation());
		BoundsTransformTest.SetRotation(CoverLandmark->ExitTriggerBoxTransform.GetRotation());
		BoundsTransformTest.SetScale3D(CoverLandmark->ExitTriggerBoxTransform.GetScale3D());

		const float Dot = FVector::DotProduct((CoverLandmark->IdlePoint->GetActorLocation() - ChosenExitProxy->GetActorLocation()).GetSafeNormal2D(), (GetCharacter()->GetActorLocation() - ChosenExitProxy->GetActorLocation()).GetSafeNormal2D());
		//LOG_NUMBER(Dot);
		if (UKismetMathLibrary::IsPointInBoxWithTransform(TestLocation, BoundsTransformTest, CoverLandmark->ExitTriggerBoxExtent) ||
			Dot < 0.0f)
		{
			OwningController->FinishActivity(this, true, true);
		}
		else
		{
			const FVector& Direction = (ChosenExitProxy->GetActorLocation() - CoverLandmark->IdlePoint->GetActorLocation()).GetSafeNormal2D();
			GetCharacter()->AddMovementInput(Direction);
		}
	}

	//if (Uptime > 1.0f)
	//	OwningController->FinishActivity(this, true, true);
}

void UTakeCoverAtLandmarkActivity::Enter_ExitLandmark_State()
{
	TimeWaiting = 0.0f;
	bIsHiding = false;
	
	GetCharacter()->TimeHiding = 0.0f;
	
	GetCharacter()->bIsExitingLandmark = true;

	GetCharacter()->SetActorHiddenInGame(false);
	
	AddIgnoredActors();
	
	// Turn collision back on
	GetCharacter()->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

#if defined(PLATFORM_XB1) || defined(PLATFORM_PS4)
	GetCharacter()->GetCharacterMovement()->SetMovementMode(MOVE_NavWalking);
#else
	GetCharacter()->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
#endif

	// Reset cooldown
	CoverLandmark->AddCooldownFor(OwningController, CoverLandmark->CooldownAfterUse);
	
	if (CoverLandmark->IdlePoint)
		GetCharacter()->SetActorRotation(CoverLandmark->IdlePoint->GetActorRotation());

	if (LoopExitAnim)
	{
		GetCharacter()->Play3PMontage(LoopExitAnim);
	}
	else
	{
		PlayExitLandmarkAnim();
	}

	GetCharacter()->TimeSinceAtLastCoverLandmark = 0.0f;
	
	CoverLandmark->LastUsedByController = OwningController;
	
	DisableProxy();

	#if !UE_BUILD_SHIPPING
	if (CVarCoverLandmarkInstantExit.GetValueOnAnyThread() > 0)
	{
		GetCharacter()->Multicast_Stop3PMontage(EntryAnim, 0.0f);
		GetCharacter()->SetActorLocation(FVector(ChosenExitProxy->GetActorLocation().X, ChosenExitProxy->GetActorLocation().Y, GetCharacter()->GetActorLocation().Z));
		GetCharacter()->SetActorRotation(ChosenExitProxy->GetActorRotation());
		UpdateHidingState();
	}
	#endif
}

void UTakeCoverAtLandmarkActivity::Tick_ExitLandmark_State(const float DeltaTime, const float Uptime)
{
	SetLocation(FVector::ZeroVector);

	AbortMove();

	GetCharacter()->SetActorHiddenInGame(false);
	
	TimeWaiting = 0.0f;
	TimeEnteringLandmark = 0.0f;
	bIsHiding = false;
	
	GetCharacter()->bIsExitingLandmark = true;

	if (!ChosenExitProxy)
		return;

	if (!LoopExitAnim)
		return;
	
	const FVector TestLocation = FVector(GetCharacter()->GetActorLocation().X, GetCharacter()->GetActorLocation().Y, CoverLandmark->IdlePoint->GetActorLocation().Z);

	FTransform BoundsTransformTest;
	BoundsTransformTest.SetLocation(ChosenExitProxy->GetActorLocation() + CoverLandmark->ExitTriggerBoxTransform.GetLocation());
	BoundsTransformTest.SetRotation(CoverLandmark->ExitTriggerBoxTransform.GetRotation());
	BoundsTransformTest.SetScale3D(CoverLandmark->ExitTriggerBoxTransform.GetScale3D());

	// Has reached the exit point?
	if (UKismetMathLibrary::IsPointInBoxWithTransform(TestLocation, BoundsTransformTest, CoverLandmark->ExitTriggerBoxExtent))
	{
		if (!bPlayedExitAnim)
		{
			bPlayedExitAnim = true;
			
			PlayExitLandmarkAnim();
		}
	}
	else
	{
		// Loop the exit anim over and over
		if (!GetCharacter()->Is3PMontagePlaying(LoopExitAnim))
		{
			GetCharacter()->Play3PMontage(LoopExitAnim);
		}
	}
	
	if (bPlayedExitAnim)
	{
		GetCharacter()->SetActorLocation(FMath::VInterpConstantTo(GetCharacter()->GetActorLocation(), FVector(ChosenExitProxy->GetActorLocation().X, ChosenExitProxy->GetActorLocation().Y, GetCharacter()->GetActorLocation().Z), DeltaTime, 75.0f));
	}
	else
	{
		if (!LoopExitAnim->HasRootMotion()) // Failsafe if given animation doesnt have root motion
		{
			GetCharacter()->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			GetCharacter()->SetActorLocation(FMath::VInterpConstantTo(GetCharacter()->GetActorLocation(), FVector(ChosenExitProxy->GetActorLocation().X, ChosenExitProxy->GetActorLocation().Y, GetCharacter()->GetActorLocation().Z), DeltaTime, 75.0f), false, nullptr, ETeleportType::TeleportPhysics);

			const FRotator Target = FMath::Lerp(CoverLandmark->IdlePoint->GetActorRotation() + FRotator(0.0f, CoverLandmark->Exit.AnimYawOffset, 0.0f), ChosenExitProxy->GetActorRotation(), Uptime/ExitAnimTime);
			GetCharacter()->SetActorRotation(FMath::RInterpConstantTo(GetCharacter()->GetActorRotation(), Target, DeltaTime, 75.0f));
		}
	}
}

bool UTakeCoverAtLandmarkActivity::CanEnterLandmark() const
{
	if (!CoverLandmark)
		return false;

	const FVector FocalPoint = ChosenEntryProxy->GetActorLocation() + ChosenEntryProxy->GetActorRotation().Vector() * 200.0f;

	const float DotProduct = FVector::DotProduct((FocalPoint - GetCharacter()->GetActorLocation()).GetSafeNormal2D(), GetCharacter()->GetActorForwardVector());
	const bool bIsFacingIdlePoint = DotProduct > 0.985f;
	
	return HasReachedEntryLocation(GetDestinationTolerance()) && bIsFacingIdlePoint;
}

bool UTakeCoverAtLandmarkActivity::CanExitLandmark() const
{
	if (bShouldExitNow)
		return true;

	//if (TimeWaiting > WaitDuration && TimeSwatNotLookingAtLandmark > AI_CONFIG_GET_FLOAT("CoverLandmarkNoLookExitTime", 5.0f))
		//return true;
	
	return false;
}

bool UTakeCoverAtLandmarkActivity::ShouldWait() const
{
	if (LoopEntryAnim)
	{
		const FVector TestLocation = FVector(GetCharacter()->GetActorLocation().X, GetCharacter()->GetActorLocation().Y, CoverLandmark->IdlePoint->GetActorLocation().Z);
		
		FTransform BoundsTransformTest;
		BoundsTransformTest.SetLocation(CoverLandmark->IdlePoint->GetActorLocation() + CoverLandmark->IdleTriggerBoxTransform.GetLocation());
		BoundsTransformTest.SetRotation(CoverLandmark->IdleTriggerBoxTransform.GetRotation());
		BoundsTransformTest.SetScale3D(CoverLandmark->IdleTriggerBoxTransform.GetScale3D());

		return UKismetMathLibrary::IsPointInBoxWithTransform(TestLocation, BoundsTransformTest, CoverLandmark->IdleTriggerBoxExtent);
	}

	return TimeEnteringLandmark > EntryAnimTime;
}

bool UTakeCoverAtLandmarkActivity::CanAbruptlyExit() const
{
	return bMoveToExit;
}

bool UTakeCoverAtLandmarkActivity::HasReachedEntryLocation(const float Tolerance) const
{
	if (!OwningController || !GetCharacter() || !ChosenEntryProxy)
		return false;

	const float ZHeightDifference = FMath::Abs(ChosenEntryProxy->GetActorLocation().Z - GetCharacter()->GetActorLocation().Z);
	if (ZHeightDifference > 100.0f)
		return false;

	const float Dist = FVector::Distance(ChosenEntryProxy->GetActorLocation(), FVector(GetCharacter()->GetActorLocation().X, GetCharacter()->GetActorLocation().Y, ChosenEntryProxy->GetActorLocation().Z));
	//LOG_NUMBER(Dist);
	return Dist < Tolerance;
}

void UTakeCoverAtLandmarkActivity::PlayExitLandmarkAnim()
{
	bIsHiding = false;

	GetCharacter()->Rep_HidingAnimState.bLooping = false;

	GetCharacter()->Play3PMontage(ExitAnim);
	
	// Fixes little snap in animation when bIsHiding is immediately set
	{
		if (ExitAnimTime > 0.0f)
		{
			UReadyOrNotFunctionLibrary::StartTimerForCallback(this, &UTakeCoverAtLandmarkActivity::UpdateHidingState, ExitAnimTime);
		}
		else
		{
			UpdateHidingState();
		}
	}
}

void UTakeCoverAtLandmarkActivity::UpdateHidingState()
{
	GetCharacter()->Rep_HidingAnimState.bIsHiding = bIsHiding;
}

void UTakeCoverAtLandmarkActivity::BindExitEvents()
{
	// whenever these events fire, exit cover landmark immediately (or with a chance to not exit everytime these events are broadcasted)
	
	GetCharacter()->OnCharacterTakeDamage.AddDynamic(this, &UTakeCoverAtLandmarkActivity::OnTakeDamage);
	GetCharacter()->OnStunnedEvent.AddDynamic(this, &UTakeCoverAtLandmarkActivity::OnStunned);
	GetCharacter()->OnHeardOfficerYell.AddDynamic(this, &UTakeCoverAtLandmarkActivity::OnHeardYell);

	for (TActorIterator<APlayerCharacter> It(GetWorld()); It; ++It)
	{
		APlayerCharacter* PlayerCharacter = *It;
		PlayerCharacter->OnWeaponFire.AddDynamic(this, &UTakeCoverAtLandmarkActivity::OnEnemyWeaponFire);
	}
}

void UTakeCoverAtLandmarkActivity::UnbindExitEvents()
{
	GetCharacter()->OnCharacterTakeDamage.RemoveAll(this);
	GetCharacter()->OnStunnedEvent.RemoveAll(this);
	GetCharacter()->OnHeardOfficerYell.RemoveAll(this);

	for (TActorIterator<APlayerCharacter> It(GetWorld()); It; ++It)
	{
		APlayerCharacter* PlayerCharacter = *It;
		PlayerCharacter->OnWeaponFire.RemoveAll(this);
	}
}

void UTakeCoverAtLandmarkActivity::AddIgnoredActors()
{
	if (CoverLandmark->bDisableCollision)
	{
		GetCharacter()->MoveIgnoreActorAdd(CoverLandmark->CoverObject.LoadSynchronous());
	}

	for (const TSoftObjectPtr<AStaticMeshActor>& MeshActor : CoverLandmark->IgnoredMeshActors)
	{
		GetCharacter()->MoveIgnoreActorAdd(MeshActor.LoadSynchronous());
	}
	
	for (ABlockingVolume* V : Cast<AReadyOrNotLevelScript>(GetWorld()->GetLevelScriptActor())->BlockingVolumesInLevel)
		GetCharacter()->MoveIgnoreActorAdd(V);
	
	GetCharacter()->MoveIgnoreActorAdd(CoverLandmark->IdlePoint);
	GetCharacter()->MoveIgnoreActorAdd(ChosenEntryProxy);
	GetCharacter()->MoveIgnoreActorAdd(ChosenExitProxy);
	
	GetCharacter()->GetInteractableComponent_Implementation()->IgnoreInteractionBlockingActors.AddUnique(CoverLandmark->CoverObject.LoadSynchronous());
	GetCharacter()->GetInteractableComponent_Implementation()->IgnoreInteractionBlockingActors.AddUnique(CoverLandmark->IdlePoint);
	GetCharacter()->GetInteractableComponent_Implementation()->IgnoreInteractionBlockingActors.AddUnique(ChosenEntryProxy);
	GetCharacter()->GetInteractableComponent_Implementation()->IgnoreInteractionBlockingActors.AddUnique(ChosenExitProxy);
}

void UTakeCoverAtLandmarkActivity::RemoveIgnoredActors()
{
	GetCharacter()->MoveIgnoreActorRemove(CoverLandmark->CoverObject.LoadSynchronous());
	GetCharacter()->MoveIgnoreActorRemove(CoverLandmark->IdlePoint);
	GetCharacter()->MoveIgnoreActorRemove(ChosenEntryProxy);
	GetCharacter()->MoveIgnoreActorRemove(ChosenExitProxy);

	for (const TSoftObjectPtr<AStaticMeshActor>& MeshActor : CoverLandmark->IgnoredMeshActors)
	{
		GetCharacter()->MoveIgnoreActorRemove(MeshActor.LoadSynchronous());
	}
	
	for (ABlockingVolume* V : Cast<AReadyOrNotLevelScript>(GetWorld()->GetLevelScriptActor())->BlockingVolumesInLevel)
		GetCharacter()->MoveIgnoreActorRemove(V);
	
	GetCharacter()->GetInteractableComponent_Implementation()->IgnoreInteractionBlockingActors.Remove(CoverLandmark->CoverObject.LoadSynchronous());
	GetCharacter()->GetInteractableComponent_Implementation()->IgnoreInteractionBlockingActors.Remove(CoverLandmark->IdlePoint);
	GetCharacter()->GetInteractableComponent_Implementation()->IgnoreInteractionBlockingActors.Remove(ChosenEntryProxy);
	GetCharacter()->GetInteractableComponent_Implementation()->IgnoreInteractionBlockingActors.Remove(ChosenExitProxy);
}

void UTakeCoverAtLandmarkActivity::EnableProxy()
{
	if (ChosenEntryProxy)
		ChosenEntryProxy->EnableProxyInteraction();

	if (CoverLandmark && CoverLandmark->IdlePoint)
		CoverLandmark->IdlePoint->EnableProxyInteraction();

	if (ChosenExitProxy)
		ChosenExitProxy->EnableProxyInteraction();
}

void UTakeCoverAtLandmarkActivity::DisableProxy()
{
	if (ChosenEntryProxy)
		ChosenEntryProxy->DisableProxyInteraction();

	if (CoverLandmark && CoverLandmark->IdlePoint)
		CoverLandmark->IdlePoint->DisableProxyInteraction();

	if (ChosenExitProxy)
		ChosenExitProxy->DisableProxyInteraction();
}

float UTakeCoverAtLandmarkActivity::GetDestinationTolerance() const
{
	return 25.0f;
}

FName UTakeCoverAtLandmarkActivity::GetMoveStyleOverride_Implementation() const
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

void UTakeCoverAtLandmarkActivity::OnKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter)
{
	Super::OnKilled(InstigatorCharacter, KilledCharacter);

	if (GetCharacter())
		GetCharacter()->bDiedWhilstHiding = true;
	
	TeleportWeapon();
	
	Broadcast_OnProxyEnd(false);
}

void UTakeCoverAtLandmarkActivity::OnIncapacitated(AReadyOrNotCharacter* IncapacitatedCharacter, AReadyOrNotCharacter* InstigatorCharacter)
{
	Super::OnIncapacitated(IncapacitatedCharacter, InstigatorCharacter);
	
	if (GetCharacter())
		GetCharacter()->bDiedWhilstHiding = true;
	
	TeleportWeapon();
	
	Broadcast_OnProxyEnd(false);
}

void UTakeCoverAtLandmarkActivity::OnPathFound(uint32 PathId, ENavigationQueryResult::Type ResultType, FNavPathSharedPtr NavPath)
{
	Super::OnPathFound(PathId, ResultType, NavPath);

	if (!GetCharacter())
		return;

	if (GetCharacter()->bIsExitingLandmark)
		return;
	
	if (!NavPath.IsValid())
		return;

	bool bAnyPathPointGoesTowardsInstigator = false;

	const TArray<FNavPathPoint>& PathPoints = NavPath->GetPathPoints();
	if (PathPoints.Num() == 0)
		return;

	if (AReadyOrNotGameState* GS = GetWorld()->GetGameState<AReadyOrNotGameState>())
	{
		for (const AReadyOrNotCharacter* Character : GS->AllReadyOrNotCharacters)
		{
			if (bAnyPathPointGoesTowardsInstigator)
				break;
			
			if (IsValid(Character) && Character->IsOnSWATTeam())
			{
				for (int32 i = 1; i < PathPoints.Num(); i++)
				{
					FVector PathPoint = PathPoints[i].Location;

					// Is this path point inside of the instigator's danger radius
					const float Distance = FVector::Distance(PathPoint, Character->GetActorLocation());
					constexpr float DangerRadius = 400.0f;
					
					if (Distance < DangerRadius)
					{
						const FVector DirectionToPathPoint = (PathPoint - GetCharacter()->GetActorLocation()).GetSafeNormal2D();
						const FVector DirectionToInstigator = (Character->GetActorLocation() - PathPoint).GetSafeNormal2D();
						
						const float PathPointDotProduct = FVector::DotProduct(DirectionToPathPoint, DirectionToInstigator);
						
						if (PathPointDotProduct > 0.8f)
						{
							bAnyPathPointGoesTowardsInstigator = true;
							break;
						}
					}
				}
			}
		}
	}

	if (bAnyPathPointGoesTowardsInstigator)
	{
		ACTIVITY_FAILED("Unable to get to " + GetNameSafe(CoverLandmark) + ". Path goes towards swat", true);
	}
}

void UTakeCoverAtLandmarkActivity::OnStunned(AReadyOrNotCharacter* StunnedCharacter, float Duration, EStunType StunType, AActor* DamageCauser)
{
	if (bShouldExitNow)
		return;
	
	bShouldExitNow = true;
	bShouldSurrender = true;

	#if !UE_BUILD_SHIPPING
	ULog::Info(OwningController->GetName() + " | Exiting landmark due to stun");
	#endif
}

void UTakeCoverAtLandmarkActivity::OnHeardYell(AReadyOrNotCharacter* Shouter, const bool bLOS)
{
	if (GetActiveStateName() != "Wait")
		return;

	if (bShouldExitNow)
		return;

	const FVector DirectionToEnemy = (GetCharacter()->GetActorLocation() - Shouter->GetActorLocation()).GetSafeNormal2D();
	const float ForwardDotProduct = FVector::DotProduct(DirectionToEnemy, Shouter->GetActorForwardVector());

	// Is the enemy yelling in the general direction to us?
	if (ForwardDotProduct > 0.975f && FVector::Distance(Shouter->GetActorLocation(), GetCharacter()->GetActorLocation()) < 1000.0f)
	{
		//const float CurrentMorale = FMath::Clamp(OwningController->GetMoraleComp()->GetMorale(), 0.35f, 1.0f);
		const float ExitChance = AI_CONFIG_GET_FLOAT("CoverLandmarkExitChanceOnYell", 0.5f);
		//const float ExitChance = CurrentMorale;

		const bool bLOSToLandmark = Shouter->HasLineOfSightTo(CoverLandmark->GetActorLocation());
		
		bShouldExitNow = bLOSToLandmark && (bLOS || FMath::FRand() < ExitChance);
		bShouldSurrender = false;

		#if !UE_BUILD_SHIPPING
		if (bShouldExitNow)
			ULog::Info(OwningController->GetName() + " | Exiting landmark due to yell");
		#endif
	}
}

void UTakeCoverAtLandmarkActivity::OnEnemyWeaponFire(AReadyOrNotCharacter* Character, ABaseMagazineWeapon* Weapon, FVector FireDirection)
{
	if (!OwningController || !GetCharacter() || !Character)
		return;

	if (GetActiveStateName() != "Wait")
		return;

	if (bShouldExitNow)
		return;

	const FVector DirectionToEnemy = (GetCharacter()->GetActorLocation() - Character->GetActorLocation()).GetSafeNormal2D();
	const float ForwardDotProduct = FVector::DotProduct(DirectionToEnemy, FireDirection);

	// Is the enemy firing in the general direction to us?
	if (ForwardDotProduct > 0.9f)
	{
		FiredAtCount++;

		// The closer the fire direction, the less shots we need to fire in order to make us exit
		// ##UE5UPGRADE## FMath
		const int32 RequiredCount = FMath::GetMappedRangeValueClamped(FVector2D(0.85f, 1.0f), FVector2D(20, 5), ForwardDotProduct);

		if (FiredAtCount > RequiredCount)
		{
			const float CurrentMorale = FMath::Clamp(OwningController->GetMoraleComp()->GetMorale(), 0.4f, 1.0f);
			//LOG_NUMBER(CurrentMorale);
			//const float ExitChance = AI_CONFIG_GET_FLOAT("CoverLandmarkExitChanceWhenFiredAt", 0.5f);
			const float ExitChance = CurrentMorale;

			bShouldExitNow = FMath::FRand() < ExitChance;
			bShouldSurrender = bShouldExitNow;

			#if !UE_BUILD_SHIPPING
			if (bShouldExitNow)
				ULog::Info(OwningController->GetName() + " | Exiting landmark due to being fired at");
			#endif
		}
	}
}

void UTakeCoverAtLandmarkActivity::OnTakeDamage(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* DamagedCharacter, AActor* DamageCauser, float Damage, float HealthRemaining)
{
	if (GetActiveStateName() != "Wait")
		return;

	if (bShouldExitNow)
		return;
	
	bShouldExitNow = true;
	bShouldSurrender = bShouldExitNow;

	#if !UE_BUILD_SHIPPING
	ULog::Info(OwningController->GetName() + " | Exiting landmark due to damage");
	#endif
}

ACoverLandmark* UTakeCoverAtLandmarkActivity::FindClosestCoverLandmark() const
{
	bool bBypassChecks = false;
	#if !UE_BUILD_SHIPPING
	if (const UAIArchetypeData* Archetype = GetCharacter()->GetAIArchetype())
	{
		bBypassChecks = Archetype->Name == "DEBUG Hide Only";
	}
	#endif
				
	return UReadyOrNotFunctionLibrary::FindClosestActor<ACoverLandmark>(GetWorld(), GetCharacter()->GetActorLocation(), [&](const ACoverLandmark* Landmark, const float Distance)
	{
		if (!Landmark->bEnabled)
			return false;

		if (!bBypassChecks)
		{
			if (!Landmark->AllowedTeamsForCover.Contains(GetCharacter()->GetTeam()))
				return false;

			if (Landmark->IsCooldownActiveFor(OwningController))
				return false;
		}
		
		const float MaxZ = FMath::Max(GetCharacter()->GetActorLocation().Z, Landmark->GetActorLocation().Z);
		const float MinZ = FMath::Min(GetCharacter()->GetActorLocation().Z, Landmark->GetActorLocation().Z);
			
		const float ZHeightDifference = MaxZ - MinZ;

		// Don't consider landmarks that are above or below us.
		// Most likely not near us, due to being on another floor of a building for example
		if (ZHeightDifference > 150.0f)
			return false;

		// Too far to consider a viable landmark
		if (Distance > 3000.0f)
			return false;
		
		return true;
	});
}

bool UTakeCoverAtLandmarkActivity::IsAnySWATLookingAtLandmark() const
{
	if (!CoverLandmark)
		return false;

	// Civilians dont care
	if (GetCharacter()->IsCivilian())
		return false;
	
	if (AReadyOrNotGameState* GS = GetWorld()->GetGameState<AReadyOrNotGameState>())
	{
		for (AReadyOrNotCharacter* Character : GS->AllReadyOrNotCharacters)
		{
			if (IsValid(Character) && Character->IsOnSWATTeam())
			{
				FHitResult Hit;
				FCollisionQueryParams CollisionQueryParams = UReadyOrNotFunctionLibrary::CreateCollisionQueryParams(Character, GetCharacter());
				const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Character->GetMesh()->GetSocketLocation("head_end"), CoverLandmark->GetActorLocation(), ECC_WorldStatic, CollisionQueryParams);
				
				if (bHit)
					continue;
				
				const FVector DirectionToLandmark = (CoverLandmark->GetActorLocation() - Character->GetActorLocation()).GetSafeNormal2D();
				const float DotProduct = FVector::DotProduct(Character->GetActorForwardVector(), DirectionToLandmark);

				if (DotProduct > 0.7f)
				{
					return true;
				}
			}
		}
	}

	return false;
}

bool UTakeCoverAtLandmarkActivity::IsAnySWATLookingAtUs() const
{
	if (!CoverLandmark)
		return false;

	if (AReadyOrNotGameState* GS = GetWorld()->GetGameState<AReadyOrNotGameState>())
	{
		for (AReadyOrNotCharacter* Character : GS->AllReadyOrNotCharacters)
		{
			if (IsValid(Character) && Character->IsOnSWATTeam())
			{
				FHitResult Hit;
				FCollisionQueryParams CollisionQueryParams = UReadyOrNotFunctionLibrary::CreateCollisionQueryParams(Character, GetCharacter());
				CollisionQueryParams.bTraceComplex = true;
				const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Character->GetMesh()->GetSocketLocation("head_end"), GetCharacter()->GetMesh()->GetSocketLocation("head"), ECC_WorldStatic, CollisionQueryParams);
				
				if (bHit)
					continue;
				
				return true;
			}
		}
	}

	return false;
}

void UTakeCoverAtLandmarkActivity::ResetData()
{
	Super::ResetData();
	
	CoverLandmark = nullptr;
	ChosenEntryProxy = nullptr;
	ChosenExitProxy = nullptr;

	//WaitDuration = InitialWaitDuration;

	//TimeSwatNotLookingAtLandmark = 0.0f;
	TimeEnteringLandmark = 0.0f;
	EntryAnimTime = 0.0f;
	ExitAnimTime = 0.0f;
	TimeWaiting = 0.0f;
	TimeSinceLastVisionTrace = 0.0f;

	FiredAtCount = 0;
	
	bIsHiding = false;
	bShouldExitNow = false;
	bPlayedExitAnim = false;
	bShouldSurrender = false;
	bMoveToExit = false;
	
	EntryAnim = nullptr;
	ExitAnim = nullptr;
	LoopEntryAnim = nullptr;
	LoopExitAnim = nullptr;

	IgnoredMeshActors.Empty();
}

void UTakeCoverAtLandmarkActivity::AbortCoverNow()
{
	bShouldExitNow = true;
}

bool UTakeCoverAtLandmarkActivity::IsMovingToLandmark() const
{
	return GetActiveStateID() == 0;
}

bool UTakeCoverAtLandmarkActivity::IsExitingLandmark() const
{
	return GetActiveStateID() >= 3;
}
