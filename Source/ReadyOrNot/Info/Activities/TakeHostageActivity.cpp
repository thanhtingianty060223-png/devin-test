// Copyright Void Interactive, 2021

#include "TakeHostageActivity.h"

#include "BaseCombatActivity.h"
#include "Actors/PairedInteractionDriver.h"
#include "Actors/Items/MeleeWeapon.h"
#include "Animation/MoveStyle/RoNMoveStyleComponent.h"

#include "Characters/PlayerCharacter.h"
#include "Characters/CyberneticController.h"
#include "Characters/CyberneticCharacter.h"

#include "ReadyOrNotAIConfig.h"

UTakeHostageActivity::UTakeHostageActivity()
{
	ActivityStateMachine->AddState("Move To")
						.BindEventEnter(MAKE_DELEGATE_BINDING(this, &UTakeHostageActivity::EnterMoveToState))
						.BindEventTick(MAKE_DELEGATE_BINDING(this, &UTakeHostageActivity::TickMoveToState))
						.CreateTransition("Begin Take", MAKE_DELEGATE_BINDING(this, &UTakeHostageActivity::CanStartHostageTake));

	ActivityStateMachine->AddState("Begin Take")
						.BindEventEnter(MAKE_DELEGATE_BINDING(this, &UTakeHostageActivity::EnterBeginHostageTakeState))
						.BindEventTick(MAKE_DELEGATE_BINDING(this, &UTakeHostageActivity::TickBeginHostageTakeState))
						.CreateTransition("Taking", MAKE_DELEGATE_BINDING(this, &UTakeHostageActivity::CanIdle));
	
	ActivityStateMachine->AddState("Taking")
						.BindEventEnter(MAKE_DELEGATE_BINDING(this, &UTakeHostageActivity::EnterTakingState))
						.BindEventTick(MAKE_DELEGATE_BINDING(this, &UTakeHostageActivity::TickTakingState))
						.BindEventExit(MAKE_DELEGATE_BINDING(this, &UTakeHostageActivity::EndTakingState))
						.CreateTransition("End Take", MAKE_DELEGATE_BINDING(this, &UTakeHostageActivity::CanEndHostageTake));
						//.CreateTransition("Turn", MAKE_DELEGATE_BINDING(this, &UTakeHostageActivity::ShouldTurn));
	
	//ActivityStateMachine->AddState("Turn")
	//					.BindEventEnter(MAKE_DELEGATE_BINDING(this, &UTakeHostageActivity::EnterTurnState))
	//					.BindEventTick(MAKE_DELEGATE_BINDING(this, &UTakeHostageActivity::TickTurnState))
	//					.CreateTransition("Taking", MAKE_DELEGATE_BINDING(this, &UTakeHostageActivity::CanIdle));
	
	ActivityStateMachine->AddState("End Take")
						.BindEventEnter(MAKE_DELEGATE_BINDING(this, &UTakeHostageActivity::EnterEndHostageTakeState))
						.BindEventTick(MAKE_DELEGATE_BINDING(this, &UTakeHostageActivity::TickEndHostageTakeState));
}

void UTakeHostageActivity::StartActivity(AAIController* Owner)
{
	Super::StartActivity(Owner);

	if (GetCharacter()->IsSurrendered() || GetCharacter()->IsSurrenderComplete())
	{
		return;
	}

	if (!Hostage)
	{
		ACTIVITY_FAILED("No hostage character");
		return;
	}

	if (!Hostage->IsActive())
	{
		ACTIVITY_FAILED("Hostage is not active");
		return;
	}

	if (GetCharacter() == Hostage)
	{
		ACTIVITY_FAILED("You cannot take yourself hostage", false);
		return;
	}
	
	if (GetCharacter()->IsOnSWATTeam() || Hostage->IsOnSWATTeam())
	{
		ACTIVITY_FAILED("Cannot take SWAT hostage", false);
		return;
	}

	if (Hostage->IsBeingTakenHostage())
	{
		ACTIVITY_FAILED(Hostage->GetName() + " is already being taken hostage by " + Hostage->TakenHostageBy->GetName(), false);
		return;
	}
	
	HostageInteractions = UInteractionsData::GetInteractionCollection("HostageTake");

	#if WITH_EDITOR
	ensureAlways(HostageInteractions.Find("Start") != nullptr);
	ensureAlways(HostageInteractions.Find("End") != nullptr);
	ensureAlways(HostageInteractions.Find("Idle") != nullptr);
	ensureAlways(HostageInteractions.Find("Idle_Knife") != nullptr);
	ensureAlways(HostageInteractions.Find("HostageKill") != nullptr);
	ensureAlways(HostageInteractions.Find("HostageKill_Knife") != nullptr);
	ensureAlways(HostageInteractions.Find("SuspectKill") != nullptr);
	ensureAlways(HostageInteractions.Find("HostageExternalKill") != nullptr);
	//ensureAlways(HostageInteractions.Find("Turn90Left") != nullptr);
	//ensureAlways(HostageInteractions.Find("Turn90Right") != nullptr);
	#endif

	if (!HostageInteractions.Find("Start") ||
		!HostageInteractions.Find("End") ||
		!HostageInteractions.Find("Idle") ||
		!HostageInteractions.Find("Idle_Knife") ||
		!HostageInteractions.Find("HostageKill") ||
		!HostageInteractions.Find("HostageKill_Knife") ||
		!HostageInteractions.Find("SuspectKill") ||
		!HostageInteractions.Find("HostageExternalKill"))
		//!HostageInteractions.Find("Turn90Left") ||
		//!HostageInteractions.Find("Turn90Right"))
	{
		ACTIVITY_FAILED("Missing hostage interaction data");
		return;
	}

	EntryAnimTime = HostageInteractions["Start"]->DriverMontage->GetPlayLength() - (HostageInteractions["Start"]->DriverMontage->GetDefaultBlendOutTime() + 0.05f);
	ExitAnimTime = HostageInteractions["End"]->DriverMontage->GetPlayLength() - (HostageInteractions["End"]->DriverMontage->GetDefaultBlendOutTime() + 0.05f);

	Hostage->TakenHostageBy = GetCharacter();

	Hostage->OnCharacterKilled.RemoveAll(this);
	Hostage->OnCharacterKilled.AddDynamic(this, &UTakeHostageActivity::OnHostageKilled);
	
	Hostage->OnCharacterIncapacitated.RemoveAll(this);
	Hostage->OnCharacterIncapacitated.AddDynamic(this, &UTakeHostageActivity::OnHostageKilled);

	GetCharacter()->OnSensedCharacter.RemoveAll(this);
	GetCharacter()->OnSensedCharacter.AddDynamic(this, &UTakeHostageActivity::OnSensedCharacter);
	
	GetCharacter()->OnHeardOfficerYell.RemoveAll(this);
	GetCharacter()->OnHeardOfficerYell.AddDynamic(this, &UTakeHostageActivity::OnHeardYell);

	GetCharacter()->OnStunnedEvent.RemoveAll(this);
	GetCharacter()->OnStunnedEvent.AddDynamic(this, &UTakeHostageActivity::OnStunned);

	GetCharacter()->OnCharacterTakeDamage.RemoveAll(this);
	GetCharacter()->OnCharacterTakeDamage.AddDynamic(this, &UTakeHostageActivity::OnTakeDamage);

	GetCharacter()->ScoringComponent->RevokeAllPenalties();
	
	GetCharacter()->TimeSinceLastAggressiveForce = 0.0f;
}

void UTakeHostageActivity::PerformActivity(float DeltaTime)
{
	Super::PerformActivity(DeltaTime);

	if (Hostage && GetActiveStateID() == 2)
	{
		Hostage->Rep_FocalPoint = GetCharacter()->Rep_FocalPoint;
	}
}

void UTakeHostageActivity::FinishedActivity(bool bSuccess)
{
	Super::FinishedActivity(bSuccess);

	if (APairedInteractionDriver* Interaction = UInteractionsData::IsPairedInteractionPlayingOn(GetCharacter()))
	{
		Interaction->EndInteraction();
	}
	
	if (Hostage)
	{
		Hostage->TakenHostageBy = nullptr;
		Hostage->Rep_HoleTraversalAnimState = {};
		
		Hostage->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		
		Hostage->ReasonsToStandStill.Remove("taken hostage");
		Hostage->MoveStyle->ClearOverrideMoveStyle();

		if (!Hostage->IsDeadOrUnconscious())
		{
			GetCharacter()->MoveIgnoreActorRemove(Hostage);
			Hostage->MoveIgnoreActorRemove(GetCharacter());
		}
		
		Hostage->OnCharacterKilled.RemoveAll(this);
	}
	
	OwningController->GetCombatActivity()->TimeSpentWithWeaponUp = 0.0f;

	GetCharacter()->ReasonsToStandStill.Remove("taking hostage");
	GetCharacter()->MoveStyle->ClearOverrideMoveStyle();
	
	GetCharacter()->AccuracyNerfPercentage = 1.0f;
	
	GetCharacter()->Rep_TakeHostageAnimState = {};
	
	GetCharacter()->OnSensedCharacter.RemoveAll(this);
	GetCharacter()->OnSensedCharacter.RemoveAll(this);
	GetCharacter()->OnHeardOfficerYell.RemoveAll(this);
	GetCharacter()->OnStunnedEvent.RemoveAll(this);
	GetCharacter()->OnCharacterTakeDamage.RemoveAll(this);
}

bool UTakeHostageActivity::CanFinishActivity() const
{
	return false;
}

bool UTakeHostageActivity::CanShoot() const
{
	if (bIsTurning)
		return false;

	if (GetActiveStateID() == 1 || GetActiveStateID() == 3)
		return false;
	
	return true;
}

bool UTakeHostageActivity::OverrideFocalPoint(FVector& FocalPoint)
{
	if (GetActiveStateID() == 0)
	{
		if (Location != FVector::ZeroVector && HasReachedLocation(500.0f))
		{
			FocalPoint = OwningController->GetFocalPointOnActor(Hostage);
			return true;
		}
	}

	if (GetActiveStateID() == 2) // Taking state
	{
		if (OwningController->GetTrackedTarget())
		{
			FocalPoint = OwningController->GetFocalPointOnActor(OwningController->GetTrackedTarget());
			return true;
		}
		
		if (OwningController->GetLastTrackedEnemy())
		{
			FocalPoint = OwningController->GetFocalPointOnActor(OwningController->GetLastTrackedEnemy());
			return true;
		}
		
		if (LOCAL_PLAYER)
		{
			FocalPoint = OwningController->GetFocalPointOnActor(LocalPlayer);
			return true;
		}
	}
	
	FocalPoint = FVector::ZeroVector;
	return false;
}

float UTakeHostageActivity::GetDestinationTolerance() const
{
	return 50.0f;
}

bool UTakeHostageActivity::ShouldForceStrafe() const
{
	return true;
}

bool UTakeHostageActivity::CanOverrideActivity() const
{
	return false;
}

bool UTakeHostageActivity::CanBeOverridenBy(UBaseActivity* InOverridingActivity)
{
	return false;
}

void UTakeHostageActivity::ResetData()
{
	Super::ResetData();

	LastEnemySensed = nullptr;

	bKillHostageNow = false;
	bSurrenderHostageNow = false;
	bIsLooping = false;
	bShouldTurnNow = false;
	bRightTurn = false;
	bIsTurning = false;

	TimeEnteringHostageTake = 0.0f;
	EntryAnimTime = 0.0f;
	ExitAnimTime = 0.0f;
	
	DriverMoveStyleOverride = NAME_None;
	SlaveMoveStyleOverride = NAME_None;

	EntryPoint = FVector::ZeroVector;

	VOCooldown = 0.0f;
}

bool UTakeHostageActivity::HasReachedEntryLocation(const float Tolerance) const
{
	if (!OwningController || !GetCharacter())
		return false;

	const float ZHeightDifference = FMath::Abs(EntryPoint.Z - GetCharacter()->GetActorLocation().Z);
	if (ZHeightDifference > 100.0f)
		return false;

	const float Dist = FVector::Distance(EntryPoint, FVector(GetCharacter()->GetActorLocation().X, GetCharacter()->GetActorLocation().Y, EntryPoint.Z));
	//LOG_NUMBER(Dist);
	return Dist < Tolerance;
}

void UTakeHostageActivity::OverrideMoveStyle()
{
	GetCharacter()->MoveStyle->SetOverrideMoveStyleByName(DriverMoveStyleOverride);
	Hostage->MoveStyle->SetOverrideMoveStyleByName(SlaveMoveStyleOverride);
}

void UTakeHostageActivity::ClearMoveStyleOverride()
{
	GetCharacter()->MoveStyle->ClearOverrideMoveStyle();
	Hostage->MoveStyle->ClearOverrideMoveStyle();
}

void UTakeHostageActivity::OnTakeDamage(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* DamagedCharacter, AActor* DamageCauser, float Damage, float HealthRemaining)
{
	bKillHostageNow = true;
}

void UTakeHostageActivity::OnHostageKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter)
{
	Hostage->ReasonsToStandStill.Remove("taken hostage");
	Hostage->MoveStyle->ClearOverrideMoveStyle();
	Hostage->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	Hostage->Rep_TakeHostageAnimState = {};

	/*
	if (APairedInteractionDriver* Interaction = UInteractionsData::IsPairedInteractionPlayingOn(GetCharacter()))
	{
		Interaction->OnSlaveInteractionFinished();
	}
	*/
	
	// Only run when externally killed
	if (bKillHostageNow || InstigatorCharacter == GetCharacter())
		return;
	
	if (APairedInteractionDriver* Interaction = UInteractionsData::IsPairedInteractionPlayingOn(GetCharacter()))
	{
		Interaction->EndInteraction();
	}
	
	if (GetCharacter() && GetCharacter()->IsActive())
	{
		GetCharacter()->Play3PMontage(HostageInteractions["HostageExternalKill"]->DriverMontage);
	}

	UReadyOrNotFunctionLibrary::StartTimerForCallback(this, &UTakeHostageActivity::ForceFinish, ExitAnimTime);
	
	ForceStop();
}

void UTakeHostageActivity::OnHostageTakeStartComplete_Driver(AActor* Actor)
{
	Location = FVector::ZeroVector;
	
	if (ACyberneticCharacter* DriverCharacter = Cast<ACyberneticCharacter>(Actor))
	{
		bIsLooping = true;
		DriverCharacter->Rep_TakeHostageAnimState.bIsLooping = true;
		Hostage->Rep_TakeHostageAnimState.bIsLooping = true;

		// Keep ignoring
		DriverCharacter->MoveIgnoreActorAdd(Hostage);
		Hostage->MoveIgnoreActorAdd(DriverCharacter);
	}

	GetCharacter()->PlayRawVO(VO_SUSPECTS_AND_CIVILIAN::BARK_HOSTAGE);
}

void UTakeHostageActivity::OnHostageTakeStartComplete_Slave(AActor* Actor)
{
	Location = FVector::ZeroVector;
	
	if (ACyberneticCharacter* SlaveCharacter = Cast<ACyberneticCharacter>(Actor))
	{
		SlaveCharacter->Rep_TakeHostageAnimState.bIsLooping = true;

		// Keep ignoring
		if (GetCharacter())
			GetCharacter()->MoveIgnoreActorAdd(SlaveCharacter);
		
		SlaveCharacter->MoveIgnoreActorAdd(GetCharacter());
	}
}

void UTakeHostageActivity::OnHostageTakeEndComplete_Driver(AActor* Actor)
{
	if (ACyberneticCharacter* DriverCharacter = Cast<ACyberneticCharacter>(Actor))
	{
		bIsLooping = false;
		DriverCharacter->Rep_TakeHostageAnimState.bIsLooping = false;
		Hostage->Rep_TakeHostageAnimState.bIsLooping = false;
		
		DriverCharacter->MoveIgnoreActorRemove(Hostage);
		Hostage->MoveIgnoreActorRemove(DriverCharacter);

		OwningController->FinishActivity(this, true, true);
	}
}

void UTakeHostageActivity::OnHostageTakeEndComplete_Slave(AActor* Actor)
{
	if (ACyberneticCharacter* SlaveCharacter = Cast<ACyberneticCharacter>(Actor))
	{
		GetCharacter()->Rep_TakeHostageAnimState.bIsLooping = false;
		SlaveCharacter->Rep_TakeHostageAnimState.bIsLooping = false;

		// Keep ignoring
		GetCharacter()->MoveIgnoreActorRemove(SlaveCharacter);
		SlaveCharacter->MoveIgnoreActorRemove(GetCharacter());
	}
}

void UTakeHostageActivity::OnHostageTakeTurnComplete_Driver(AActor* Actor)
{
	bIsTurning = false;
	bShouldTurnNow = false;
}

void UTakeHostageActivity::OnHostageTakeTurnComplete_Slave(AActor* Actor)
{
}

void UTakeHostageActivity::OnHostageTakeKillComplete_Driver(AActor* Actor)
{
	if (ACyberneticCharacter* DriverCharacter = Cast<ACyberneticCharacter>(Actor))
	{
		bIsLooping = false;
		DriverCharacter->Rep_TakeHostageAnimState = {};
		Hostage->Rep_TakeHostageAnimState = {};
		
		OwningController->FinishActivity(this, true, true);
	}
}

void UTakeHostageActivity::OnHostageTakeKillComplete_Slave(AActor* Actor)
{
	if (ACyberneticCharacter* SlaveCharacter = Cast<ACyberneticCharacter>(Actor))
	{
		GetCharacter()->Rep_TakeHostageAnimState = {};
		SlaveCharacter->Rep_TakeHostageAnimState = {};

		// Keep ignoring
		GetCharacter()->MoveIgnoreActorAdd(SlaveCharacter);
		SlaveCharacter->MoveIgnoreActorAdd(GetCharacter());

		if (SlaveCharacter->IsDeadOrUnconscious() || SlaveCharacter->IsIncapacitated())
			SlaveCharacter->EnableRagdoll();
	}
}

void UTakeHostageActivity::EnterMoveToState()
{
	EntryPoint = Hostage->GetActorLocation() + Hostage->GetActorForwardVector() * -95.0f;
	//EntryPoint = Hostage->GetActorLocation() + (GetCharacter()->GetActorLocation() - Hostage->GetActorLocation()).GetSafeNormal() * 95.0f;
	SetLocation(EntryPoint);
}

void UTakeHostageActivity::TickMoveToState(float DeltaTime, float Uptime)
{
	if (!OwningController->IsMoving())
	{
		EntryPoint = Hostage->GetActorLocation() + Hostage->GetActorForwardVector() * -95.0f;
		if (!HasReachedEntryLocation(35.0f) || Location == FVector::ZeroVector)
		{
			SetLocation(EntryPoint, true);
		}
	}
}

bool UTakeHostageActivity::CanStartHostageTake() const
{
	FVector FocalPoint = Hostage->GetActorLocation() + Hostage->GetActorForwardVector() * 500.0f;
	//FVector FocalPoint = Hostage->GetActorLocation() + -(GetCharacter()->GetActorLocation() - Hostage->GetActorLocation()).GetSafeNormal() * 500.0f;
	FocalPoint.Z = GetCharacter()->GetMesh()->GetSocketLocation("head").Z;

	const float DotProduct = FVector::DotProduct((FocalPoint - GetCharacter()->GetActorLocation()).GetSafeNormal2D(), GetCharacter()->GetActorForwardVector());
	const bool bIsFacingEntry = DotProduct > 0.95f;
	//LOG_NUMBER(DotProduct);
	
	return Location != FVector::ZeroVector && HasReachedLocation(35.0f) && bIsFacingEntry;
}

void UTakeHostageActivity::EnterBeginHostageTakeState()
{
	Location = FVector::ZeroVector;

	AbortMove();
	
	if (GetCharacter()->GetEquippedItem<AMeleeWeapon>())
	{
		DriverMoveStyleOverride = "male01_suspect_knife_hostage_driver";
		SlaveMoveStyleOverride = "male01_shared_unarmed_hostage_knife_slave";
	}
	else
	{
		DriverMoveStyleOverride = "male01_suspect_pistol_hostage_driver";
		SlaveMoveStyleOverride = "male01_shared_unarmed_hostage_slave";
	}
	
	const FRoNMovementStyle* HostageDriverMoveStyle = GetCharacter()->MoveStyle->GetMovementStyleByName(DriverMoveStyleOverride);
	const FRoNMovementStyle* HostageSlaveMoveStyle = GetCharacter()->MoveStyle->GetMovementStyleByName(SlaveMoveStyleOverride);

	UAimOffsetBlendSpace* AimOffset_Master = HostageDriverMoveStyle->TurnData.AimOffset;
	UAimOffsetBlendSpace* AimOffset_Slave = HostageSlaveMoveStyle->TurnData.AimOffset;
	
	FTakeHostageAnimState TakeHostageAnimState_Master;
	TakeHostageAnimState_Master.bIsLooping = false;
	TakeHostageAnimState_Master.bIsTakingHostage = true;
	TakeHostageAnimState_Master.LoopAnim = GetCharacter()->HostageMasterIdleLoop;
	TakeHostageAnimState_Master.AimOffset = AimOffset_Master;
	
	FTakeHostageAnimState TakeHostageAnimState_Slave;
	TakeHostageAnimState_Slave.bIsLooping = false;
	TakeHostageAnimState_Slave.bIsTakingHostage = true;
	TakeHostageAnimState_Slave.LoopAnim = GetCharacter()->HostageSlaveIdleLoop;
	TakeHostageAnimState_Slave.AimOffset = AimOffset_Slave;

	GetCharacter()->Rep_TakeHostageAnimState = TakeHostageAnimState_Master;
	Hostage->Rep_TakeHostageAnimState = TakeHostageAnimState_Slave;
	
	Hostage->AttachToComponent(GetCharacter()->GetMesh(), FAttachmentTransformRules::KeepWorldTransform);

	if (APairedInteractionDriver* HostageTakeBeginDriver = GetCharacter()->PlayPairedInteraction(HostageInteractions["Start"], GetCharacter(), Hostage, nullptr))
	{
		HostageTakeBeginDriver->Event_OnDriverInteractionFinished.AddDynamic(this, &UTakeHostageActivity::OnHostageTakeStartComplete_Driver);
		HostageTakeBeginDriver->Event_OnSlaveInteractionFinished.AddDynamic(this, &UTakeHostageActivity::OnHostageTakeStartComplete_Slave);
	}

	UReadyOrNotFunctionLibrary::StartTimerForCallback(this, &UTakeHostageActivity::OverrideMoveStyle, 0.5f);
}

void UTakeHostageActivity::TickBeginHostageTakeState(float DeltaTime, float Uptime)
{
	Location = FVector::ZeroVector;
}

bool UTakeHostageActivity::CanIdle() const
{
	return bIsLooping && !bIsTurning;
}

void UTakeHostageActivity::EnterTakingState()
{
	Location = FVector::ZeroVector;
	
	GetCharacter()->ReasonsToStandStill.AddUnique("taking hostage");
	Hostage->ReasonsToStandStill.AddUnique("taken hostage");
	
	GetCharacter()->AccuracyNerfPercentage = AI_CONFIG_GET_FLOAT("HostageTakeAccuracyNerf", 6.0f);

	FName InteractionName = "Idle";
	if (GetCharacter()->GetEquippedItem<AMeleeWeapon>())
		InteractionName = "Idle_Knife";
	
	GetCharacter()->PlayPairedInteraction(HostageInteractions[InteractionName], GetCharacter(), Hostage, nullptr);
	
	bEverHadLOSOnTrackedTargetUponEntering = OwningController->GetTargetingComp()->HasLineOfSightToTrackedTarget();

	if (LOCAL_PLAYER)
	{
		if (!bEverHadLOSOnTrackedTargetUponEntering)
			bEverHadLOSOnTrackedTargetUponEntering = GetCharacter()->HasLineOfSightToCharacter(LocalPlayer);
	}
}

void UTakeHostageActivity::EndTakingState()
{
	GetCharacter()->AccuracyNerfPercentage = 1.0f;
	
	if (APairedInteractionDriver* Interaction = UInteractionsData::IsPairedInteractionPlayingOn(GetCharacter()))
	{
		Interaction->EndInteraction();
	}
}

void UTakeHostageActivity::TickTakingState(float DeltaTime, float Uptime)
{
	Location = FVector::ZeroVector;
	
	AbortMove();

	if (const ABaseMagazineWeapon* EquippedWeapon = GetCharacter()->GetEquippedWeapon())
	{
		if (!EquippedWeapon->HasAmmo())
		{
			bSurrenderHostageNow = true;
			return;
		}
	}

	VOCooldown -= DeltaTime;
	if (VOCooldown <= 0.0f)
	{
		VOCooldown = FMath::FRandRange(5.0f, 15.0f);
		GetCharacter()->PlayRawVO(VO_SUSPECTS_AND_CIVILIAN::BARK_HOSTAGE);
	}

	if (Uptime > TimeToSurrenderHostage)
	{
		bKillHostageNow = true;
		return;
	}

	if (LastEnemySensed && OwningController->GetTrackedTarget() == LastEnemySensed)
	{
		bEverHadLOSOnTrackedTargetUponEntering = true;
		if (LastEnemySensed->IsActive() && LastEnemySensed->IsPlayerControlled())
		{
			const FVector DirectionToEnemy = (LastEnemySensed->GetActorLocation() - GetCharacter()->GetActorLocation()).GetSafeNormal2D();
			
			const float ForwardDotProduct = FVector::DotProduct(DirectionToEnemy, GetCharacter()->GetActorForwardVector());
			//const float RightDotProduct = FVector::DotProduct(DirectionToEnemy, GetCharacter()->GetActorRightVector());

			// Only when in front
			if (ForwardDotProduct > 0.0f)
			{
				const float DistanceToLastSensedEnemy = FVector::Distance(GetCharacter()->GetActorLocation(), LastEnemySensed->GetActorLocation());
				
				if (DistanceToLastSensedEnemy < 400.0f)
				{
					bKillHostageNow = true;
					return;
				}
			}
		}
	}
	else
	{
		if (LOCAL_PLAYER)
		{
			if (!bEverHadLOSOnTrackedTargetUponEntering)
			{
				FRotator NewRotation = UKismetMathLibrary::RInterpTo(GetCharacter()->GetActorRotation(), (LocalPlayer->GetActorLocation() - GetCharacter()->GetActorLocation()).Rotation(), DeltaTime, 3.0f);
		
				GetCharacter()->SetActorRotation(NewRotation);
			}
			else
			{
				const FVector DirectionToEnemy = (LocalPlayer->GetActorLocation() - GetCharacter()->GetActorLocation()).GetSafeNormal2D();
				
				const float ForwardDotProduct = FVector::DotProduct(DirectionToEnemy, GetCharacter()->GetActorForwardVector());
		
				// Surrender hostage if behind us
				if (ForwardDotProduct < 0.0f)
				{
					bSurrenderHostageNow = true;
				}
			}
		}
	}
}

bool UTakeHostageActivity::CanEndHostageTake() const
{
	return bKillHostageNow || bSurrenderHostageNow;
}

void UTakeHostageActivity::EnterTurnState()
{
	bIsTurning = true;
	
	UInteractionsData* TurnInteraction = bRightTurn ? HostageInteractions["Turn90Right"] : HostageInteractions["Turn90Left"];
	
	if (APairedInteractionDriver* HostageTakeEndDriver = GetCharacter()->PlayPairedInteraction(TurnInteraction, GetCharacter(), Hostage, nullptr))
	{
		HostageTakeEndDriver->Event_OnDriverInteractionFinished.AddDynamic(this, &UTakeHostageActivity::OnHostageTakeTurnComplete_Driver);
		HostageTakeEndDriver->Event_OnSlaveInteractionFinished.AddDynamic(this, &UTakeHostageActivity::OnHostageTakeTurnComplete_Slave);
	}
	else
	{
		bIsTurning = false;
	}
}

void UTakeHostageActivity::TickTurnState(float DeltaTime, float Uptime)
{
}

bool UTakeHostageActivity::ShouldTurn() const
{
	return bShouldTurnNow;
}

void UTakeHostageActivity::EnterEndHostageTakeState()
{
	Location = FVector::ZeroVector;
	
	GetCharacter()->ReasonsToStandStill.Remove("taking hostage");
	Hostage->ReasonsToStandStill.Remove("taken hostage");
	
	Hostage->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	
	GetCharacter()->MoveIgnoreActorAdd(Hostage);
	Hostage->MoveIgnoreActorAdd(GetCharacter());
	
	UReadyOrNotFunctionLibrary::StartTimerForCallback(this, &UTakeHostageActivity::ClearMoveStyleOverride, 0.5f);

	if (bKillHostageNow)
	{
		FName InteractionName = "HostageKill";
		if (GetCharacter()->GetEquippedItem<AMeleeWeapon>() != nullptr)
			InteractionName = "HostageKill_Knife";
		
		if (APairedInteractionDriver* HostageTakeEndDriver = GetCharacter()->PlayPairedInteraction(HostageInteractions[InteractionName], GetCharacter(), Hostage, nullptr))
		{
			GetCharacter()->PlayRawVO(VO_SUSPECTS_AND_CIVILIAN::BARK_KILL_CIVILIAN);
			
			HostageTakeEndDriver->Event_OnDriverInteractionFinished.AddDynamic(this, &UTakeHostageActivity::OnHostageTakeKillComplete_Driver);
			HostageTakeEndDriver->Event_OnSlaveInteractionFinished.AddDynamic(this, &UTakeHostageActivity::OnHostageTakeKillComplete_Slave);
		}
		
		return;
	}

	if (APairedInteractionDriver* HostageTakeEndDriver = GetCharacter()->PlayPairedInteraction(HostageInteractions["End"], GetCharacter(), Hostage, nullptr))
	{
		HostageTakeEndDriver->Event_OnDriverInteractionFinished.AddDynamic(this, &UTakeHostageActivity::OnHostageTakeEndComplete_Driver);
		HostageTakeEndDriver->Event_OnSlaveInteractionFinished.AddDynamic(this, &UTakeHostageActivity::OnHostageTakeEndComplete_Slave);
	}
}

void UTakeHostageActivity::TickEndHostageTakeState(float DeltaTime, float Uptime)
{
	if (Hostage)
	{
		Hostage->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
		Hostage->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
		Hostage->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	}
}

void UTakeHostageActivity::OnSensedCharacter(AReadyOrNotCharacter* SensedCharacter)
{
	if (UBpGameplayHelperLib::IsEnemy(GetCharacter()->GetTeam(), SensedCharacter->GetTeam()))
	{
		LastEnemySensed = SensedCharacter;

		const FVector DirectionToEnemy = (SensedCharacter->GetActorLocation() - GetCharacter()->GetActorLocation()).GetSafeNormal2D();
		
		const float ForwardDotProduct = FVector::DotProduct(DirectionToEnemy, GetCharacter()->GetActorForwardVector());
		//const float RightDotProduct = FVector::DotProduct(DirectionToEnemy, GetCharacter()->GetActorRightVector());

		// Surrender hostage if behind us
		if (ForwardDotProduct < 0.0f)
		{
			bSurrenderHostageNow = true;
		}
	}
}

void UTakeHostageActivity::OnHeardYell(AReadyOrNotCharacter* Shouter, bool bLOS)
{
	if (bLOS)
	{
		const float VisibleSWAT = GetCharacter()->GetVisibleSWATPercentage();
		GetCharacter()->ForceComplianceStrength += 0.05f * VisibleSWAT;
	}
}

void UTakeHostageActivity::OnKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter)
{
	Super::OnKilled(InstigatorCharacter, KilledCharacter);

	bIsLooping = false;
	
	if (APairedInteractionDriver* Interaction = UInteractionsData::IsPairedInteractionPlayingOn(GetCharacter()))
	{
		Interaction->EndInteraction();
	}

	if (!bKillHostageNow)
	{
		if (Hostage->GetCurrentMontage() != HostageInteractions["HostageKill"]->SlaveMontage)
		{
			Hostage->Play3PMontage(HostageInteractions["SuspectKill"]->SlaveMontage);
			
			ForceStop();
		}
	}
	
	OwningController->FinishActivity(this, true, true);
}

void UTakeHostageActivity::OnStunned(AReadyOrNotCharacter* StunnedCharacter, float Duration, EStunType StunType, AActor* DamageCauser)
{
	if (APairedInteractionDriver* Interaction = UInteractionsData::IsPairedInteractionPlayingOn(GetCharacter()))
	{
		Interaction->EndInteraction();
	}

	ForceStop();

	OwningController->FinishActivity(this, true, true);
}

void UTakeHostageActivity::ForceStop()
{
	bIsLooping = false;
	
	GetCharacter()->ReasonsToStandStill.Remove("taking hostage");
	Hostage->ReasonsToStandStill.Remove("taken hostage");

	GetCharacter()->MoveStyle->ClearOverrideMoveStyle();
	Hostage->MoveStyle->ClearOverrideMoveStyle();
	
	Hostage->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	
	GetCharacter()->Rep_TakeHostageAnimState = {};
	Hostage->Rep_TakeHostageAnimState = {};

	GetCharacter()->MoveIgnoreActorRemove(Hostage);
	Hostage->MoveIgnoreActorRemove(GetCharacter());
}

void UTakeHostageActivity::ForceFinish()
{
	OwningController->FinishActivity(this, true, true);
}
