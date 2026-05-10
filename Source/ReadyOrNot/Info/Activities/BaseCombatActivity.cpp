// Copyright Void Interactive, 2021

#include "BaseCombatActivity.h"

#include "CommitSuicideActivity.h"
#include "ReloadSafelyActivity.h"

#include "Team/TeamBreachAndClearActivity.h"

#include "AI/AIAction.h"

#include "Actors/ExplosiveVest.h"
#include "Actors/Items/MeleeWeapon.h"

#include "Characters/CyberneticController.h"
#include "Characters/CyberneticCharacter.h"

#include "CombatMove/BaseCombatMoveActivity.h"
#include "CombatMove/DuelingCombatMove.h"
#include "CombatMove/HardCoverCombatMove.h"
#include "CombatMove/SuppressionCombatMove.h"
#include "CombatMove/RepositionCombatMove.h"
#include "TraverseHoleActivity.h"

#include "MoveToExitActivity.h"

#include "PickupItemActivity.h"
#include "PlayDeadActivity.h"

#include "ReadyOrNotAIConfig.h"
#include "TakeCoverActivity.h"
#include "Actors/Attachments/LaserAttachment.h"
#include "Actors/Attachments/LightAttachment.h"

#include "CombatMove/ChargeCombatMove.h"
#include "CombatMove/FlankingCombatMove.h"
#include "CombatMove/FleeingCombatMove.h"
#include "CombatMove/PushCombatMove.h"

#include "InvestigateStimulusActivity.h"
#include "Actors/WallHoleTraversal.h"
#include "Animation/MoveStyle/RoNMoveStyleComponent.h"

#include "ActivityManagerTemplates.h"
#include "Commander/RosterManager.h"

DECLARE_CYCLE_STAT(TEXT("RoN ~ Perform Activity"), STAT_BaseCombatActivityPerformActivity, STATGROUP_CombatActivity);

TAutoConsoleVariable<int32> CVarCombatActivityDrawFiringLogic(TEXT("CombatActivity.DrawFiringLogic"), 0, TEXT("0 = Dont draw firing logic. 1 = Draw debug firing"));
TAutoConsoleVariable<int32> CVarCombatActivityAlwaysPlayDead(TEXT("CombatActivity.AlwaysPlayDead"), 0, TEXT("0 = Dont force play dead. 1 = Always play dead (when triggered)"));

TAutoConsoleVariable<int32> CVarCombatActivityNoShoot(TEXT("CombatActivity.NoShoot"), 0, TEXT("0 = Dont force play dead. 1 = Always play dead (when triggered)"));

UBaseCombatActivity::UBaseCombatActivity()
{
	bIsProgressActivity = false;
	bNoMoveActivity = true;

	ActivityStateMachine->AddState("No Strafe")
						.BindEventEnter(MAKE_DELEGATE_BINDING(this, &UBaseCombatActivity::EnterNoStrafeState))
						.BindEventTick(MAKE_DELEGATE_BINDING(this, &UBaseCombatActivity::PerformNoStrafeLogic))
						.CreateTransition("Strafe", MAKE_DELEGATE_BINDING(this, &UBaseCombatActivity::ShouldForceStrafe));
	
	ActivityStateMachine->AddState("Strafe")
						.BindEventEnter(MAKE_DELEGATE_BINDING(this, &UBaseCombatActivity::EnterStrafeState))
						.BindEventTick(MAKE_DELEGATE_BINDING(this, &UBaseCombatActivity::PerformStrafeLogic))
						.CreateTransition("No Strafe", MAKE_DELEGATE_BINDING(this, &UBaseCombatActivity::ShouldForceNoStrafe));
}

void UBaseCombatActivity::BindEvents()
{
	GetCharacter()->OnCharacterTakeDamage.AddDynamic(this, &UBaseCombatActivity::OnTakeDamage);
}

void UBaseCombatActivity::UnbindEvents()
{
	GetCharacter()->OnCharacterTakeDamage.RemoveAll(this);
}

void UBaseCombatActivity::StartActivity(AAIController* Owner)
{
	Super::StartActivity(Owner);

	if (!IsValid(HardCoverCombatMove))
	{
		HardCoverCombatMove = UActivityManager::CreateActivity<UHardCoverCombatMove>(this);
		if (HardCoverCombatMove && HardCoverCombatMove->InitActivity(Owner))
		{
			HardCoverCombatMove->OwningCombatActivity = this;
			HardCoverCombatMove->NewCoverFound.AddDynamic(this, &UBaseCombatActivity::OnCoverFound);
			HardCoverCombatMove->NoCoverFound.AddDynamic(this, &UBaseCombatActivity::OnNoCoverFound);
			HardCoverCombatMove->OnCoverExit.AddDynamic(this, &UBaseCombatActivity::OnCoverExit);
			HardCoverCombatMove->OnRequestCover.AddDynamic(this, &UBaseCombatActivity::OnRequestCover);
			HardCoverCombatMove->OnRequestCoverLandmark.AddDynamic(this, &UBaseCombatActivity::OnRequestCoverLandmark);
			HardCoverCombatMove->OnCoverLandmarkExit.AddDynamic(this, &UBaseCombatActivity::OnCoverLandmarkExit);
		}
	}

	if (!IsValid(DuelingCombatMove))
	{
		DuelingCombatMove = UActivityManager::CreateActivity<UDuelingCombatMove>(this);

		if (DuelingCombatMove)
		{
			DuelingCombatMove->InitActivity(Owner);
			DuelingCombatMove->OwningCombatActivity = this;
		}
	}

	if (!IsValid(FleeingCombatMove))
	{
		FleeingCombatMove = UActivityManager::CreateActivity<UFleeingCombatMove>(this);

		if (FleeingCombatMove)
		{
			FleeingCombatMove->InitActivity(Owner);
			FleeingCombatMove->OwningCombatActivity = this;
		}
	}
	
	if (!IsValid(FlankingCombatMove))
	{
		FlankingCombatMove = UActivityManager::CreateActivity<UFlankingCombatMove>(this);

		if (FlankingCombatMove)
		{
			FlankingCombatMove->InitActivity(Owner);
			FlankingCombatMove->OwningCombatActivity = this;
		}
	}
	
	if (!IsValid(ChargeCombatMove))
	{
		ChargeCombatMove = UActivityManager::CreateActivity<UChargeCombatMove>(this);

		if (ChargeCombatMove)
		{
			ChargeCombatMove->InitActivity(Owner);
			ChargeCombatMove->OwningCombatActivity = this;
		}
	}
	
	if (!IsValid(SuppressionCombatMove))
	{
		SuppressionCombatMove = UActivityManager::CreateActivity<USuppressionCombatMove>(this);

		if (SuppressionCombatMove)
		{
			SuppressionCombatMove->InitActivity(Owner);
			SuppressionCombatMove->OwningCombatActivity = this;
		}
	}
	
	if (!IsValid(PushCombatMove))
	{
		PushCombatMove = UActivityManager::CreateActivity<UPushCombatMove>(this);

		if (PushCombatMove)
		{
			PushCombatMove->InitActivity(Owner);
			PushCombatMove->OwningCombatActivity = this;
		}
	}
	
	if (!IsValid(RepositionCombatMove))
	{
		RepositionCombatMove = UActivityManager::CreateActivity<URepositionCombatMove>(this);

		if (RepositionCombatMove)
		{
			RepositionCombatMove->InitActivity(Owner);
			RepositionCombatMove->OwningCombatActivity = this;
		}
	}
	
	if (!IsValid(PickupItemActivity))
	{
		PickupItemActivity = UActivityManager::CreateActivity<UPickupItemActivity>(this);
		
		if (PickupItemActivity)
			PickupItemActivity->InitActivity(Owner);
	}

	if (!IsValid(ReloadSafelyActivity))
	{
		ReloadSafelyActivity = UActivityManager::CreateActivity<UReloadSafelyActivity>(this);
		
		if (ReloadSafelyActivity)
			ReloadSafelyActivity->InitActivity(Owner);
	}

	if (!IsValid(PlayDeadActivity))
	{
		PlayDeadActivity = UActivityManager::CreateActivity<UPlayDeadActivity>(this);
		
		if (PlayDeadActivity)
		{
			PlayDeadActivity->InitActivity(Owner);
			PlayDeadActivity->OnStartActivity.RemoveAll(this);
			PlayDeadActivity->OnFinishActivity.RemoveAll(this);
			PlayDeadActivity->OnStartActivity.AddDynamic(this, &UBaseCombatActivity::PlayDeadStarted);
			PlayDeadActivity->OnFinishActivity.AddDynamic(this, &UBaseCombatActivity::PlayDeadFinished);
		}
	}

	if (!IsValid(CommitSuicideActivity))
	{
		CommitSuicideActivity = UActivityManager::CreateActivity<UCommitSuicideActivity>(this);

		if (CommitSuicideActivity)
		{
			CommitSuicideActivity->InitActivity(Owner);
			CommitSuicideActivity->OnFakeOutSuccess.RemoveAll(this);
			CommitSuicideActivity->OnFakeOutSuccess.AddDynamic(this, &UBaseCombatActivity::OnSuicideFakeOutSuccess);		
		}
	}

	if (!IsValid(TraverseHoleActivity))
	{
		TraverseHoleActivity = UActivityManager::CreateActivity<UTraverseHoleActivity>(this);

		if (TraverseHoleActivity)
		{
			TraverseHoleActivity->InitActivity(Owner);
		}
	}

	CurrentCoverLandmarkEvaluationCooldown = 0.0f;

	BindEvents();
}

void UBaseCombatActivity::FinishedActivity(const bool bSuccess)
{
	Super::FinishedActivity(bSuccess);

	UnbindEvents();
}

void UBaseCombatActivity::PerformActivity(const float DeltaTime)
{
	SCOPE_CYCLE_COUNTER(STAT_BaseCombatActivityPerformActivity);

	// Combat activities should never set a location, combat/non-movement related logic only
	Location = FVector::ZeroVector;
	
	if (ShouldFinishCombatMoveNow())
	{
		FinishCombatMove();
	}

	CurrentCoverLandmarkEvaluationCooldown = FMath::Max(CurrentCoverLandmarkEvaluationCooldown - DeltaTime, 0.0f);
	CurrentScriptedFireAt.TimeRemaining = FMath::Max(CurrentScriptedFireAt.TimeRemaining - DeltaTime, 0.0f);
	CurrentScriptedLookAt.TimeRemaining = FMath::Max(CurrentScriptedLookAt.TimeRemaining - DeltaTime, 0.0f);

	// determine engagement type
	{
		if (GetCharacter()->GetEquippedWeapon())
			ActiveEngagementType = ECombatEngagementType::FireWeapon;
		else if (GetCharacter()->IsWearingExplosiveVest())
			ActiveEngagementType = ECombatEngagementType::ExplosiveVest;
		else
			ActiveEngagementType = ECombatEngagementType::Melee;
	}
	
	if (FleeingCombatMove)
	{
		FleeDesire = FleeingCombatMove->CalculateFleeDesire();
	}
	else
	{
		FleeDesire = 0.0f;
	}

	//GetCharacter()->bAimingAtTarget = TimeSpentEngagingOnTarget > 0.0f;
	
	if (ABaseMagazineWeapon* bmw = GetCharacter()->GetEquippedWeapon())
	{
		const FVector& FireDirection = bmw->GetBulletSpawn()->GetForwardVector();
		const FVector& DirectionToFocalPoint = (GetCharacter()->Rep_FocalPoint - bmw->GetBulletSpawn()->GetComponentLocation()).GetSafeNormal();
		const float TargetRotationDelta = FVector::DotProduct(FireDirection, DirectionToFocalPoint);

		GetCharacter()->bAimingAtTarget = TargetRotationDelta >= GetCharacter()->FireAngleThreshold;
	}
	else
	{
		GetCharacter()->bAimingAtTarget = TimeSpentEngagingOnTarget > 0.2f; // takes a bit of time before they actually aim at them
	}

	if (const AReadyOrNotCharacter* C = Cast<AReadyOrNotCharacter>(CurrentScriptedFireAt.FireAtActor))
	{
		const bool bShouldStop = (GetCharacter()->IsOnSWATTeam() && (C->IsDeadOrUnconscious() || C->IsIncapacitated() || C->IsInRagdoll())) ||
								(GetCharacter()->IsSuspect() && C->IsDeadNotUnconscious());
		
		if (bShouldStop)
		{
			StopScriptedFire();
		}
	}
	
	if (IsExplodingVest())
		return;
	
	if (ShouldReloadNow())
	{
		ReloadEquippedWeapon();
	}
	
	// non-combatmove activities have higher priority for movement
	UBaseActivity* CurrentActivity = OwningController->GetCurrentActivity();

	// No active activity running (that supports movement)?
	if (!CurrentActivity ||
		(CurrentActivity && CurrentActivity->IsNoMoveActivity()))
	{
		if (CombatMoveActivity)
		{
			if (CombatMoveActivity->GetInterruptActivity())
			{
				CombatMoveActivity->OnMoveResumed();
			}
			
			CombatMoveActivity->PerformActivity(DeltaTime);
			return;
		}
		
		Super::PerformActivity(DeltaTime);
	}
	else
	{
		if (CombatMoveActivity && CombatMoveActivity->GetInterruptActivity())
		{
			CombatMoveActivity->OnMoveInterrupted(CurrentActivity);
		}
		
		TimeSinceLastAsyncMove += DeltaTime;
	}

	if (CombatMoveActivity)
	{
		TimeSincePerformingAnyCombatMove = 0.0f;
		TimePerformingAnyCombatMove += DeltaTime;
	}
	else
	{
		TimeSincePerformingAnyCombatMove += DeltaTime;
		TimePerformingAnyCombatMove = 0.0f;
	}
}

#if !UE_BUILD_SHIPPING
void UBaseCombatActivity::PerformActivity_Debug(const float DeltaTime)
{
	Super::PerformActivity_Debug(DeltaTime);

	if (CombatMoveActivity)
		CombatMoveActivity->PerformActivity_Debug(DeltaTime);
}
#endif

bool UBaseCombatActivity::CanShoot() const
{
	if (const ABaseMagazineWeapon* EquippedWeapon = GetCharacter()->GetEquippedWeapon())
	{
		if (!EquippedWeapon->HasAmmo())
			return false;

		if (IsReloading())
			return false;

		if (GetCharacter()->IsAnimationBlocking())
			return false;

		// Can't shoot when playing a full body animation
		if (GetCharacter()->IsFullBodyMontagePlaying())
		{
			// Allow weapon recoils
			if (!GetCharacter()->IsRecoiling() && !GetCharacter()->IsPlayingHitReaction())
				return false;
		}

		// Can't shoot when movestyle is not a strafe
		const bool bIsMoveStyleStrafe = GetCharacter()->MoveStyle && GetCharacter()->MoveStyle->bIsStrafing;
		if (!bIsMoveStyleStrafe && OwningController->IsMoving())
			return false;
		
		if (GetCharacter()->IsStunned() || GetCharacter()->HasRecentlyTakenStunDamage())
			return false;

		if (GetCharacter()->IsRaisingWeapon() || GetCharacter()->IsLoweringWeapon() ||
			GetCharacter()->IsDrawingWeapon())
			return false;
		
		if (const UBaseActivity* CurrentActivity = OwningController->GetCurrentActivity())
		{
			return CurrentActivity->CanShoot();
		}

		return true;
	}
	
	return false;
}

bool UBaseCombatActivity::IsReloading() const
{
	return OwningController->GetCurrentActivity<UReloadSafelyActivity>() == ReloadSafelyActivity;
}

bool UBaseCombatActivity::CanReload() const
{
	if (IsReloading())
		return false;
	
	const ABaseMagazineWeapon* EquippedWeapon = GetCharacter()->GetEquippedWeapon();

	// Don't bother if no equipped weapon
	if (!EquippedWeapon)
		return false;

	// Can't reload, no more mags available
	if (EquippedWeapon->Magazines.Num() == 0)
		return false;
	
	// Can't reload, no ammo to fill the mags
	if (EquippedWeapon->AmmoMax == 0.0f)
		return false;
	
	if (EquippedWeapon->GetAmmo() >= EquippedWeapon->AmmoMax)
		return false;

	if (const UBaseActivity* CurrentActivity = OwningController->GetCurrentActivity())
	{
		if (CurrentActivity->CanReload())
			return GetCharacter()->bWantsReload;

		return false;
	}

	return GetCharacter()->bWantsReload;
}

float UBaseCombatActivity::GetDestinationTolerance() const
{
	if (ActiveEngagementType == ECombatEngagementType::Melee)
		return 50.0f;

	return Super::GetDestinationTolerance();
}

bool UBaseCombatActivity::ShouldReloadNow() const
{
	return CanReload() && !GetCharacter()->IsAny3PMontageActive() && GetCharacter()->IsActiveForCombat();
}

bool UBaseCombatActivity::ShouldTrackTarget() const
{
	return true;
}

bool UBaseCombatActivity::IsStrafing() const
{
	return GetActiveStateID() > 0; // Not strafing 
}

void UBaseCombatActivity::GatherDebugString(FString& OutString)
{
	#if !UE_BUILD_SHIPPING
	OutString.Empty();

	FString StrafeReason;
	const bool bStrafing = GetStrafeDebugString(StrafeReason);
	if (!StrafeReason.IsEmpty())
	{
		OutString += AddDebugString(bStrafing ? "Strafe Reason" : "No Strafe Reason", StrafeReason);
	}
	#endif
}

bool UBaseCombatActivity::GetStrafeDebugString(FString& OutString) const
{
	OutString = "";
	
	if ((CurrentScriptedFireAt.FireAtActor || CurrentScriptedFireAt.FireAtLocation != FVector::ZeroVector) && CurrentScriptedFireAt.TimeRemaining > 0.0f)
	{
		OutString = "Scripted fire";
		return true;
	}
	
	if (GetCharacter()->IsAnimationBlocking())
	{
		OutString = "Animation Blocking";
		return false;
	}
	
	const bool bIsMoveStyleStrafe = GetCharacter()->MoveStyle && GetCharacter()->MoveStyle->bIsStrafing;
	if (!bIsMoveStyleStrafe && GetOwningController()->IsMoving())
	{
		return false;
	}
	
	if (GetCharacter()->IsPlayingDead())
	{
		OutString = "Playing dead";
		return false;
	}
	
	if (const UBaseActivity* CurrentActivity = OwningController->GetCurrentActivity())
	{
		if (CurrentActivity->ShouldForceStrafe())
		{
			OutString = CurrentActivity->GetName() + " wants strafe";
			return true;
		}

		if (CurrentActivity->ShouldForceNoStrafe())
		{
			OutString = CurrentActivity->GetName() + " wants no strafe";
			return false;
		}
	}

	if (CombatMoveActivity)
	{
		if (CombatMoveActivity->ShouldForceStrafe())
		{
			OutString = CombatMoveActivity->GetName() + " wants strafe";
			return true;
		}

		if (CombatMoveActivity->ShouldForceNoStrafe())
		{
			OutString = CombatMoveActivity->GetName() + " wants no strafe";
			return false;
		}
	}

	if (OwningController->IsCivilian() && OwningController->GetPathFollowingComponent()->GetStatus() != EPathFollowingStatus::Moving)
	{
		OutString = "Moving on path";
		return false;
	}
	
	// Are tracking an enemy or have seen one in the past 30 seconds
	if (OwningController->GetTrackedTarget())
	{
		OutString = "Tracking an enemy (" + GetNameSafe(OwningController->GetTrackedTarget()) + ")";
		return true;
	}
	
	if (GetCharacter()->GetTimeSinceLastStun() < 30.0f)
	{
		OutString = "Time since last stun was " + FString::Printf(TEXT("%.2f"), GetCharacter()->GetTimeSinceLastStun()) + " secs";
		return true;
	}

	if (OwningController->GetTargetingComp()->GetTimeSinceLastSeenEnemy() < 30.0f)
	{
		OutString = "Tracking an enemy (since " + FString::SanitizeFloat(OwningController->GetTargetingComp()->GetTimeSinceLastSeenEnemy()) + " secs)";
		return true;
	}
	
	if (OwningController->HasBeenExposedToAggressiveNoise(30.0f, 2000.0f))
	{
		OutString = "Tracking an enemy (since " + FString::SanitizeFloat(OwningController->GetTargetingComp()->GetTimeSinceLastSeenEnemy()) + " secs)";
		return true;
	}
	
	if (OwningController->GetAwarenessState() > EAIAwarenessState::Unalerted)
	{
		OutString = "Suspicious or Alerted";
		return true;
	}
	
	if (!OwningController->GetCurrentActivity<UTeamStackUpActivity>() && OwningController->IsSWAT())
	{
		if (OwningController->GetTargetingComp()->GetTrackingType() != ETargetingCompTracking::TCT_None &&
			OwningController->GetTargetingComp()->GetTrackingType() != ETargetingCompTracking::TCT_TrackingMoveVector)
		{
			OutString = "Tracking: " + RON_ENUM_TO_STRING(ETargetingCompTracking, OwningController->GetTargetingComp()->GetTrackingType());
			return true;
		}
	}

	return false;
}

bool UBaseCombatActivity::CanEngageNow() const
{
	if (OwningController->HasActivityType(UPickupItemActivity::StaticClass()))
		return false;

	if (GetCharacter()->Rep_HidingAnimState.bIsHiding)
		return false;

	if (GetCharacter()->IsTakingCover() && !GetCharacter()->IsFiringFromCover())
		return false;
	
	if (GetCharacter()->bCommitingSuicide)
		return false;

	if (GetCharacter()->IsPlayingDead())
		return false;

	if (GetCharacter()->IsStunned())
		return false;

	if (GetCharacter()->IsTraversingHole())
		return false;

	if (GetCharacter()->IsRaisingWeapon())
		return false;

	if (GetCharacter()->IsLoweringWeapon())
		return false;
	
	return true;
}

void UBaseCombatActivity::ReloadEquippedWeapon()
{
	const ABaseMagazineWeapon* Weapon = GetCharacter()->GetEquippedWeapon();
	if (IsReloading() || !Weapon->HasAnyAmmo())
		return;
	
	UActivityManager::GiveActivityTo(ReloadSafelyActivity, GetCharacter());
}

void UBaseCombatActivity::EnterStrafeState()
{
	if (!GetCharacter())
		return;

	GetCharacter()->SetIsStrafing(true, OwningController->GetAwarenessState() == EAIAwarenessState::Alerted);

	//#if !UE_BUILD_SHIPPING
	//ULog::Info(GetCharacter()->GetName() + " is strafing");
	//#endif
}

void UBaseCombatActivity::EnterNoStrafeState()
{
	if (!GetCharacter())
		return;
	
	GetCharacter()->SetIsStrafing(false, OwningController->GetAwarenessState() == EAIAwarenessState::Alerted);
	GetCharacter()->bWasWeaponRaised = false;
	
	OneFrameAccuracyMultiplier = 5.0f;
	
	//#if !UE_BUILD_SHIPPING
	//ULog::Info(GetCharacter()->GetName() + " is not strafing");
	//#endif
}

void UBaseCombatActivity::PerformNoStrafeLogic(const float DeltaTime, const float Uptime)
{
	//ULog::Info(OwningController->GetName() + " | PerformNoStrafeLogic");

	TimeSpentWithWeaponUp = 0.0f;
	TimeStrafing = 0.0f;
	TimeSpentEngagingOnTarget = 0.0f;
	TimeNotStrafing = Uptime;
	GetCharacter()->TimeNotStrafing = Uptime;

	GetCharacter()->SetIsStrafing(false, false);
	GetCharacter()->bAimingAtTarget = false;
	
	RunNonEngagementLogic(DeltaTime);
}

void UBaseCombatActivity::PerformStrafeLogic(const float DeltaTime, const float Uptime)
{
	//ULog::Info(OwningController->GetName() + " | PerformStrafeLogic");
	
	GetCharacter()->SetIsStrafing(true, false);
	
	TimeNotStrafing = 0.0f;
	GetCharacter()->TimeNotStrafing = 0.0f;
	TimeStrafing = Uptime;

	if (GetCharacter()->GetEquippedWeapon())
	{
		if (GetCharacter()->IsStartling())
		{
			TimeSpentWithWeaponUp += DeltaTime * 0.2f;
		}
		else
		{
			TimeSpentWithWeaponUp += DeltaTime;
		}
	}
	else
	{
		TimeSpentWithWeaponUp = 0.0f;
	}

	if (GetCharacter()->IsAnimationBlocking() || IsReloading())
	{
		TimeSpentWithWeaponUp = 0.0f;
		TimeSpentEngagingOnTarget = 0.0f;
	}

	// Engagement time tracking
	{
		if (OwningController->GetTrackedTarget() && OwningController->GetTrackedTarget()->IsActive())
		{
			if (!OwningController->GetTargetingComp()->HasLineOfSightToTrackedTarget() ||
				!OwningController->GetTargetingComp()->CanSeeTrackedTarget())
			{
				TimeSpentEngagingOnTarget = 0.0f; // reset so they dont track the higher bone zones (head bone for example)
			}
			else
			{
				TimeSpentEngagingOnTarget += DeltaTime;
			}
		}
		else
		{
			TimeSpentEngagingOnTarget = 0.0f;
		}
	}

	// Scipted fire logic
	{
		if ((CurrentScriptedFireAt.FireAtActor || CurrentScriptedFireAt.FireAtLocation != FVector::ZeroVector) &&
			CurrentScriptedFireAt.TimeRemaining > 0.0f)
		{
			if (!OwningController->GetTrackedTarget() || CurrentScriptedFireAt.bOverrideTargetedEnemy)
			{
				FireWeapon(nullptr, CurrentScriptedFireAt.bInfiniteAmmo);
			}
		}
	}
	
	if (AReadyOrNotCharacter* TrackedTarget = OwningController->GetTrackedTarget())
	{
		if (OwningController->IsCharacterEnemy(TrackedTarget))
		{
			if (TryTrackEnemy(TrackedTarget))
			{
				TimeSpentEngagingOnTarget = 0.0f;
				OnTrackNewEnemy.Broadcast(TrackedTarget);
			}

			const bool bResetEngagementTime = GetCharacter()->IsAnimationBlocking();
			if (bResetEngagementTime)
			{
				TimeSpentEngagingOnTarget = 0.0f;
			}
			
			EngageEnemy(TrackedTarget, DeltaTime);
		}
		else
		{
			TimeSpentEngagingOnTarget = 0.0f;
		}
	}
	else
	{
		TimeSpentEngagingOnTarget = 0.0f;
	}

	if (!OwningController->IsSWAT())
	{
		if (ABaseMagazineWeapon* Weapon = GetCharacter()->GetEquippedWeapon())
		{
			if (Weapon->GetLightAttachment())
			{
				Weapon->GetLightAttachment()->ToggleLight(true);
			}

			if (Weapon->GetLaserAttachment())
			{
				Weapon->GetLaserAttachment()->ToggleLaser(true);
			}
		}
	}
}

void UBaseCombatActivity::BeginAction(FAIActionData* InAction)
{
	if (!CanPerformAction())
		return;
	
	AReadyOrNotCharacter* Target = OwningController->GetTrackedTarget();
	
	if (!Target)
	{
		Target = OwningController->GetLastTrackedEnemy();
		if (Target)
		{
			if (Target->IsDeadOrUnconscious())
				Target = nullptr;
		}
	}

	const FVector& AggressiveNoiseLocation = OwningController->GetTargetingComp()->GetLastHeardAggressiveNoiseLocation();
	
	const FAIStimulus& LatestSightStimulus = OwningController->LatestSightStimulus;
	const FAIStimulus& LatestHearingStimulus = OwningController->LatestHearingStimulus;
	
	switch (InAction->ActionType)
	{
		case EAIAction::Custom:
		{
			if (UAIAction* CustomAction = InAction->GetCustomAction(OwningController))
			{
				//#if !UE_BUILD_SHIPPING
				//ULog::Info("Begin Custom Action | " + InAction->Name.ToString());
				//#endif
				
				CustomAction->InitAction(OwningController, InAction);
				CustomAction->BeginAction();
			}
		}
		break;
		
		case EAIAction::FireWeapon:
		{
			// Scripted fire logic has higher priority
			if (CurrentScriptedFireAt.FireAtActor && CurrentScriptedFireAt.TimeRemaining > 0.0f)
			{
				if (!Target || CurrentScriptedFireAt.bOverrideTargetedEnemy)
				{
					FireWeapon(nullptr, CurrentScriptedFireAt.bInfiniteAmmo);
				}
			}
			else
			{
				if (HasRecentlySeenTarget(Target))
				{
					FireWeapon(Target);
				}
			}
		}
		break;

		case EAIAction::Melee:
			GetCharacter()->MeleeVictim(Target);
		break;
		
		case EAIAction::HardCover:
			if (Target && OwningController->IsCharacterEnemy(Target))
			{
				TryMoveIntoCover(Target, 0.0f, 0.0f, false);
			}
			else
			{
				FVector StimulusLocation = AggressiveNoiseLocation;
				
				if (OwningController->TimeSinceLastExposedToSightStimulus < 0.5f)
				{
					if (OwningController->LastSensedCharacter && (OwningController->IsCharacterFriendly(OwningController->LastSensedCharacter) || OwningController->IsCharacterNeutral(OwningController->LastSensedCharacter)))
					{
						StimulusLocation = OwningController->LastSensedCharacter->GetActorLocation();
					}
					else
					{
						StimulusLocation = LatestSightStimulus.StimulusLocation;
					}
				}
				else
				{
					if (OwningController->TimeSinceLastExposedToSoundStimulus < 0.5f)
					{
						if (OwningController->HeardActorInstigator && (OwningController->IsCharacterFriendly(OwningController->HeardActorInstigator) || OwningController->IsCharacterNeutral(OwningController->HeardActorInstigator)))
						{
							StimulusLocation = OwningController->HeardActorInstigator->GetActorLocation();
						}
						else
						{
							StimulusLocation = LatestHearingStimulus.StimulusLocation;
						}
					}
				}
				
				FCoverInstigatorStimulus InstigatorStimulus;
				InstigatorStimulus.InstigatorCharacter = nullptr;
				InstigatorStimulus.ThreatTransform = FTransform(StimulusLocation);
				InstigatorStimulus.SearchRadius = 0.0f;
				InstigatorStimulus.ExclusionRadiusFromInstigator = 0.0f;
				InstigatorStimulus.bUseThreatTransformAsInstigatorTransform = true;
				
				TryMoveIntoCover(InstigatorStimulus, false);
			}
		break;

		case EAIAction::Hide:
			TryMoveIntoCoverLandmark(AggressiveNoiseLocation);
		break;
		
		case EAIAction::HideExit:
			if (CombatMoveActivity == HardCoverCombatMove)
				FinishCombatMove();
		break;

		case EAIAction::TraverseHole:
			TryTraverseNearestHole();
		break;

		case EAIAction::Surrender:
			GetCharacter()->Surrender();
		break;

		case EAIAction::FakeSurrender:
			GetCharacter()->FakeSurrender();
		break;

		case EAIAction::PlayDead:
			TryPlayDead();
		break;

		case EAIAction::Duel:
			StartRunningCombatMove(DuelingCombatMove);
		break;

		case EAIAction::Flee:
			TryFlee();
		break;

		case EAIAction::Rush:
			StartRunningCombatMove(ChargeCombatMove);
		break;

		case EAIAction::Flank:
			StartRunningCombatMove(FlankingCombatMove);
		break;

		case EAIAction::Suppress:
			StartRunningCombatMove(SuppressionCombatMove);
		break;
		
		case EAIAction::Push:
			StartRunningCombatMove(PushCombatMove);
		break;

		case EAIAction::Reposition:
			StartRunningCombatMove(RepositionCombatMove);
		break;
		
		case EAIAction::Investigate:
		{
			bool bAnyAIInvestigatingSameStimulus = false;

			UActivityManager::IterateAllActivitiesOfType<UInvestigateStimulusActivity>([&](UInvestigateStimulusActivity* Activity)
			{
				if (Activity->GetLocation() != FVector::ZeroVector)
				{
					if (Activity->GetLocation() == OwningController->LatestHearingStimulus.StimulusLocation)
					{
						bAnyAIInvestigatingSameStimulus = true;
						return false;
					}
					
					if (FVector::Distance(Activity->GetLocation(), OwningController->LatestHearingStimulus.StimulusLocation) < 100.0f)
					{
						bAnyAIInvestigatingSameStimulus = true;
						return false;
					}
				}

				return true;
			});
			
			if (!bAnyAIInvestigatingSameStimulus)
			{
				OwningController->InvestigateStimulus(OwningController->LatestHearingStimulus);
			}
		}
		break;

		case EAIAction::PickUpItem:
			TryFindPickupItem();
		break;

		case EAIAction::Suicide:
			TryCommitSuicide(FMath::RandBool());
		break;

		case EAIAction::NeverFakeSuicide:
			TryCommitSuicide(false);
		break;

		default:
		break;
	}

	if (IsActionCombatMove(InAction))
	{
		LastBegunCombatMoveAction = InAction;
		ActiveCombatMoveAction = InAction;

		if (!CombatMoveActionsStartTime.Contains(InAction->Name))
		{
			CombatMoveActionsStartTime.Emplace(InAction->Name);
		}
		CombatMoveActionsStartTime[InAction->Name] = GetWorld()->GetTimeSeconds();
	}
}

void UBaseCombatActivity::EndAction(FAIActionData* InAction)
{
	if (InAction->ActionType == EAIAction::Custom)
	{
		if (UAIAction* CustomAction = InAction->GetCustomAction(OwningController))
		{
			CustomAction->EndAction();
		}

		return;
	}

	switch (InAction->ActionType)
	{
		case EAIAction::HardCover:
			if (CombatMoveActivity == HardCoverCombatMove)
				FinishCombatMove();
		break;
			
		case EAIAction::Hide:
			if (CombatMoveActivity == HardCoverCombatMove)
				FinishCombatMove();
		break;
		
		case EAIAction::Flee:
			if (CombatMoveActivity == FleeingCombatMove)
				FinishCombatMove();
		break;
		
		case EAIAction::Rush:
			if (CombatMoveActivity == ChargeCombatMove)
				FinishCombatMove();
		break;
			
		case EAIAction::Flank:
			if (CombatMoveActivity == FlankingCombatMove)
				FinishCombatMove();
		break;
		
		case EAIAction::Duel:
			if (CombatMoveActivity == DuelingCombatMove)
				FinishCombatMove();
		break;
		
		case EAIAction::Suppress:
			if (CombatMoveActivity == SuppressionCombatMove)
				FinishCombatMove();
		break;
			
		case EAIAction::Push:
			if (CombatMoveActivity == PushCombatMove)
				FinishCombatMove();
		break;
		
		case EAIAction::Reposition:
			if (CombatMoveActivity == RepositionCombatMove)
				FinishCombatMove();
		break;
		
		case EAIAction::Investigate:
			if (OwningController->GetCurrentActivity<UInvestigateStimulusActivity>() == OwningController->InvestigateStimulusActivity)
			{
				OwningController->FinishActivity(OwningController->InvestigateStimulusActivity, false, true);
			}
		break;
		
		default:
		break;
	}

	if (IsActionCombatMove(InAction))
	{
		ActiveCombatMoveAction = nullptr;

		if (!CombatMoveActionsEndTime.Contains(InAction->Name))
		{
			CombatMoveActionsEndTime.Emplace(InAction->Name);
		}
		CombatMoveActionsEndTime[InAction->Name] = GetWorld()->GetTimeSeconds();
	}
}

void UBaseCombatActivity::OnSuccessfullyConsideredAction(FAIActionData* InAction)
{
	switch (InAction->ActionType)
	{
		case EAIAction::Custom:
			if (UAIAction* AIAction = InAction->GetCustomAction(OwningController))
			{
				AIAction->OnSucceededToConsider();
			}
		break;
			
		default:
		break;
	}

	if (!ActionSuccessfulConsiderations.Contains(InAction->Name))
	{
		ActionSuccessfulConsiderations.Emplace(InAction->Name);
	}
	
	ActionSuccessfulConsiderations[InAction->Name]++;
}

void UBaseCombatActivity::OnFailedToConsiderAction(FAIActionData* InAction)
{
	switch (InAction->ActionType)
	{
		case EAIAction::Custom:
			if (UAIAction* AIAction = InAction->GetCustomAction(OwningController))
			{
				AIAction->OnFailedToConsider();
			}
		break;
		
		default:
		break;
	}
	
	if (!ActionFailedConsiderations.Contains(InAction->Name))
	{
		ActionFailedConsiderations.Emplace(InAction->Name);
	}
	
	ActionFailedConsiderations[InAction->Name]++;
}

bool UBaseCombatActivity::PerformAction(FAIActionData* InAction, float DeltaTime)
{
	if (!CanPerformAction())
		return false;
	
	AReadyOrNotCharacter* Target = OwningController->GetTrackedTarget();

	switch (InAction->ActionType)
	{
		case EAIAction::Custom:
			if (UAIAction* CustomAction = InAction->GetCustomAction(OwningController))
			{
				if (CustomAction->WantsAbort())
				{
					//#if !UE_BUILD_SHIPPING
					//ULog::Info(CustomAction->GetName() + " wants abort");
					//#endif
					
					return false;
				}
				
				CustomAction->Tick(DeltaTime);
				
				return CustomAction->ShouldPerformAction();
			}
		return false;
		
		case EAIAction::FireWeapon:
		{
			if (!CanEngageNow())
			{
				return false;
			}
				
			// Scripted fire logic has higher priority
			if (CurrentScriptedFireAt.FireAtActor && CurrentScriptedFireAt.TimeRemaining > 0.0f)
			{
				if (!Target || CurrentScriptedFireAt.bOverrideTargetedEnemy)
				{
					FireWeapon(nullptr, CurrentScriptedFireAt.bInfiniteAmmo);
				}
			}
			else
			{
				if (HasRecentlySeenTarget(Target))
				{
					FireWeapon(Target);
				}
			}
		}
		return true;

		case EAIAction::Surrender:
			GetCharacter()->Surrender();
		return true;
		
		case EAIAction::FakeSurrender:
			GetCharacter()->FakeSurrender();
		return true;
		
		case EAIAction::Melee:
		return GetCharacter()->IsTableMontagePlaying("tp_melee");
		
		case EAIAction::HardCover:
		return CombatMoveActivity == HardCoverCombatMove;
		
		case EAIAction::Hide:
		return CombatMoveActivity == HardCoverCombatMove;

		case EAIAction::TraverseHole:
		return OwningController->GetCurrentActivity<UTraverseHoleActivity>() == TraverseHoleActivity;

		case EAIAction::PlayDead:
		return GetCharacter()->IsPlayingDead();
		
		case EAIAction::Duel:
		return CombatMoveActivity == DuelingCombatMove;

		case EAIAction::Flee:
		return CombatMoveActivity == FleeingCombatMove;

		case EAIAction::Rush:
		return CombatMoveActivity == ChargeCombatMove;

		case EAIAction::Flank:
		return CombatMoveActivity == FlankingCombatMove;
		
		case EAIAction::Suppress:
		return CombatMoveActivity == SuppressionCombatMove;
		
		case EAIAction::Push:
		return CombatMoveActivity == PushCombatMove;
		
		case EAIAction::Reposition:
		return CombatMoveActivity == RepositionCombatMove;

		case EAIAction::Investigate:
		return OwningController->GetCurrentActivity<UInvestigateStimulusActivity>() == OwningController->InvestigateStimulusActivity;
		
		case EAIAction::PickUpItem:
		return OwningController->GetCurrentActivity<UPickupItemActivity>() == PickupItemActivity;

		case EAIAction::Suicide:
		case EAIAction::NeverFakeSuicide:
		return OwningController->GetCurrentActivity<UCommitSuicideActivity>() == CommitSuicideActivity;

		default:
		break;
	}

	return false;
}

bool UBaseCombatActivity::CanPerformAction() const
{
	return GetCharacter()->IsActiveForCombat();
}

#if !UE_BUILD_SHIPPING
FString UBaseCombatActivity::DebugActionData(FAIActionData* InAction)
{
	if (InAction->ActionType == EAIAction::Custom)
	{
		if (const UAIAction* Action = InAction->GetCustomAction(OwningController))
		{
			return Action->GatherDebugInfo();
		}

		return "";
	}
	
	if (InAction->ActionType == EAIAction::FireWeapon)
	{
		if (!UnableToFireReason.IsEmpty())
			return "Unable To Fire: " + UnableToFireReason;
	}

	switch (InAction->ActionType)
	{
		case EAIAction::Melee:
			if (!UnableToMeleeReason.IsEmpty())
				return "Unable To Melee: " + UnableToMeleeReason;
		break;
		
		case EAIAction::PlayDead:
			if (OwningController->GetCurrentActivity<UPlayDeadActivity>() == PlayDeadActivity)
			{
				FString DebugData;
				PlayDeadActivity->GatherDebugString(DebugData);

				return DebugData;
			}
		return "";
		
		case EAIAction::Duel:
			if (!DuelingCombatMove->UnableToCombatReason.IsEmpty())	
				return "Unable To Duel: " + DuelingCombatMove->UnableToCombatReason;
		break;

		case EAIAction::Flee:
			if (!FleeingCombatMove->UnableToCombatReason.IsEmpty())	
				return "Unable To Flee: " + FleeingCombatMove->UnableToCombatReason;
		break;

		case EAIAction::Rush:
			if (!ChargeCombatMove->UnableToCombatReason.IsEmpty())	
				return "Unable To Charge: " + ChargeCombatMove->UnableToCombatReason;
		break;

		case EAIAction::Flank:
			if (!FlankingCombatMove->UnableToCombatReason.IsEmpty())	
				return "Unable To Flank: " + FlankingCombatMove->UnableToCombatReason;
		break;
		
		case EAIAction::Suppress:
			if (!SuppressionCombatMove->UnableToCombatReason.IsEmpty())	
				return "Unable To Suppress: " + SuppressionCombatMove->UnableToCombatReason;
		break;
		
		case EAIAction::Push:
			if (!PushCombatMove->UnableToCombatReason.IsEmpty())	
				return "Unable To Push: " + PushCombatMove->UnableToCombatReason;
		break;
		
		case EAIAction::Reposition:
			if (!RepositionCombatMove->UnableToCombatReason.IsEmpty())	
				return "Unable To Reposition: " + RepositionCombatMove->UnableToCombatReason;
		break;

		case EAIAction::Investigate:
			if (OwningController->GetCurrentActivity<UInvestigateStimulusActivity>() == OwningController->InvestigateStimulusActivity)
			{
				FString DebugData;
				OwningController->InvestigateStimulusActivity->GatherDebugString(DebugData);

				return DebugData;
			}
		break;
		
		case EAIAction::PickUpItem:
			if (OwningController->GetCurrentActivity<UPickupItemActivity>() == PickupItemActivity)
			{
				FString DebugData;
				PickupItemActivity->GatherDebugString(DebugData);

				return DebugData;
			}
		break;

		case EAIAction::Suicide:
		case EAIAction::NeverFakeSuicide:
			if (OwningController->GetCurrentActivity<UCommitSuicideActivity>() == CommitSuicideActivity)
			{
				FString DebugData;
				CommitSuicideActivity->GatherDebugString(DebugData);

				return DebugData;
			}
		break;

		default:
		break;
	}
	
	return "";
}
#endif

float UBaseCombatActivity::GetConsiderationsCount(FAIActionData* InAction, bool bSuccessfulConsiderations)
{
	if (bSuccessfulConsiderations)
	{
		if (!ActionSuccessfulConsiderations.Contains(InAction->Name))
			return 0;
		return ActionSuccessfulConsiderations[InAction->Name];
	}
	else
	{
		if (!ActionFailedConsiderations.Contains(InAction->Name))
			return 0;
		return ActionFailedConsiderations[InAction->Name];
	}
}

bool UBaseCombatActivity::ShouldForceStrafe() const
{
	return ShouldStrafe();
}

bool UBaseCombatActivity::ShouldForceNoStrafe() const
{
	return !ShouldStrafe();
}

bool UBaseCombatActivity::EngageEnemy(AReadyOrNotCharacter* EnemyCharacter, const float DeltaTime)
{
	return false;
}

bool UBaseCombatActivity::TryMoveIntoCover(AReadyOrNotCharacter* InstigatorCharacter, const float MinDistanceFromInstigator, const float ExclusionRadiusAroundInstigator, bool bRequireLOS)
{
	if (!OwningController || !GetCharacter() || !InstigatorCharacter)
		return false;
	
	FCoverInstigatorStimulus InstigatorStimulus;
	InstigatorStimulus.InstigatorCharacter = InstigatorCharacter;
	InstigatorStimulus.ThreatTransform = InstigatorCharacter->GetMesh()->GetSocketTransform("head_end");
	InstigatorStimulus.SearchRadius = MinDistanceFromInstigator;
	InstigatorStimulus.ExclusionRadiusFromInstigator = ExclusionRadiusAroundInstigator;
	
	return TryMoveIntoCover(InstigatorStimulus, bRequireLOS);
}

bool UBaseCombatActivity::TryMoveIntoCoverLandmark(const FVector& ThreatLocation, const float MinDistanceFromInstigator, AReadyOrNotCharacter* InstigatorCharacter)
{
	return TryMoveIntoCoverLandmark(ThreatLocation, (GetCharacter()->GetActorLocation() - ThreatLocation).GetSafeNormal(), MinDistanceFromInstigator, InstigatorCharacter);
}

AWallHoleTraversal* UBaseCombatActivity::FindClosestWallHoleTraversal() const
{
	bool bBypassChecks = false;
	#if !UE_BUILD_SHIPPING
	if (const UAIArchetypeData* Archetype = GetCharacter()->GetAIArchetype())
	{
		bBypassChecks = Archetype->Name == "DEBUG Traverse Hole";
	}
	#endif
				
	return UReadyOrNotFunctionLibrary::FindClosestActor<AWallHoleTraversal>(GetWorld(), GetCharacter()->GetActorLocation(), [&](const AWallHoleTraversal* WallHole, const float Distance)
	{
		if (!WallHole->bEnabled)
			return false;

		if (!bBypassChecks)
		{
			if (WallHole->IsCooldownActiveFor(OwningController))
				return false;
		}
		
		const float MaxZ = FMath::Max(GetCharacter()->GetActorLocation().Z, WallHole->GetActorLocation().Z);
		const float MinZ = FMath::Min(GetCharacter()->GetActorLocation().Z, WallHole->GetActorLocation().Z);
			
		const float ZHeightDifference = MaxZ - MinZ;

		// Don't consider landmarks that are above or below us.
		// Most likely not near us, due to being on another floor of a building for example
		if (ZHeightDifference > 150.0f)
			return false;

		// Too far to consider a viable wall hole
		if (Distance > 3000.0f)
			return false;
		
		return true;
	});
}

bool UBaseCombatActivity::TryTraverseNearestHole()
{
	if (OwningController->GetCurrentActivity<UTraverseHoleActivity>())
		return true;
	
	if (AWallHoleTraversal* WallHole = FindClosestWallHoleTraversal())
	{
		TraverseHoleActivity->WallHoleTraversalActor = WallHole;
		TraverseHoleActivity->bIgnoreCooldown = true;
		TraverseHoleActivity->bFromNavLink = false;
		
		return UActivityManager::GiveActivityTo(TraverseHoleActivity, GetCharacter(), true);
	}

	return false;
}

bool UBaseCombatActivity::TryFlee()
{
	StartRunningCombatMove(FleeingCombatMove);
	
	return CombatMoveActivity == FleeingCombatMove;
}

int32 UBaseCombatActivity::GetFailureCountForCombatMove(TSubclassOf<UBaseCombatMoveActivity> CombatMoveClass) const
{
	if (CombatMoveClass == UHardCoverCombatMove::StaticClass())
	{
		return HardCoverCombatMove->FailureCount;
	}
	
	if (CombatMoveClass == UDuelingCombatMove::StaticClass())
	{
		return DuelingCombatMove->FailureCount;
	}
	
	if (CombatMoveClass == UFlankingCombatMove::StaticClass())
	{
		return FlankingCombatMove->FailureCount;
	}
	
	if (CombatMoveClass == USuppressionCombatMove::StaticClass())
	{
		return SuppressionCombatMove->FailureCount;
	}
	
	if (CombatMoveClass == UPushCombatMove::StaticClass())
	{
		return PushCombatMove->FailureCount;
	}
	
	if (CombatMoveClass == UChargeCombatMove::StaticClass())
	{
		return ChargeCombatMove->FailureCount;
	}
	
	if (CombatMoveClass == UFleeingCombatMove::StaticClass())
	{
		return FleeingCombatMove->FailureCount;
	}
	
	if (CombatMoveClass == URepositionCombatMove::StaticClass())
	{
		return RepositionCombatMove->FailureCount;
	}

	if (CombatMoveActivity)
	{
		if (CombatMoveClass == CombatMoveActivity->GetClass())
		{
			return CombatMoveActivity->FailureCount;
		}
	}

	return -1;
}

bool UBaseCombatActivity::IsRunningCombatMoveActivity(UClass* Class) const
{
	if (!Class)
		return false;

	if (!CombatMoveActivity)
		return false;

	return CombatMoveActivity->IsA(Class);
}

void UBaseCombatActivity::StartRunningCombatMove(UClass* Class)
{
	if (!GetCharacter()->IsActiveForCombat())
	{
		FinishCombatMove();
		return;
	}		

	if (!IsRunningCombatMoveActivity(Class))
	{
		// Swat don't take hard cover
		if (GetCharacter()->IsOnSWATTeam())
		{
			if (Class == UHardCoverCombatMove::StaticClass())
			{
				return;
			}
		}
		
		FinishCombatMove();
		
		CombatMoveActivity = NewObject<UBaseCombatMoveActivity>(this, Class);
		CombatMoveActivity->OwningCombatActivity = this;
		
		if (CombatMoveActivity->InitActivity(OwningController))
			CombatMoveActivity->StartActivity(OwningController);
	}
}

void UBaseCombatActivity::StartRunningCombatMove(UBaseCombatMoveActivity* CombatMove)
{	
	if (!GetCharacter()->IsActiveForCombat())
	{
		FinishCombatMove();
		return;
	}

	if (GetCharacter()->IsSurrendered())
	{
		FinishCombatMove();
		return;
	}
	
	if (!CombatMove)
		return;

	// Swat don't take hard cover
	if (GetCharacter()->IsOnSWATTeam())
	{
		if (CombatMove == HardCoverCombatMove)
		{
			return;
		}
	}
	
	if (CombatMoveActivity != CombatMove)
	{
		FinishCombatMove();

		CombatMoveActivity = CombatMove;
		CombatMoveActivity->OwningCombatActivity = this;
		CombatMoveActivity->ResetData();
		
		if (CombatMoveActivity->InitActivity(OwningController))
		{
			if (!CombatMoveActivity->HasStartedActivity())
			{
				CombatMoveActivity->StartActivity(OwningController);
			}
		}
	}
}

void UBaseCombatActivity::OnTrackEnemy(AReadyOrNotCharacter* Enemy)
{
	if (!Enemy)
		return;

	if (!GetCharacter()->GetEquippedItem())
		return;

	//LastTrackedEnemy = Enemy;
	LastTrackedEnemyFireDirection = (GetCharacter()->GetActorLocation() - Enemy->GetActorLocation()).GetSafeNormal();
}

bool UBaseCombatActivity::RunEngagementLogic(const float DeltaTime)
{
	return RunEngagementLogic(OwningController->GetTrackedTarget(), DeltaTime);
}

void UBaseCombatActivity::ScriptedFireAtActor(AActor* InActor, float InTime, bool bOverrideTarget, float AccuracyPenaltyMultiplier, bool bInfiniteAmmo)
{
	if (!InActor)
		return;
	
	CurrentScriptedFireAt.FireAtActor = InActor;
	CurrentScriptedFireAt.FireAtLocation = FVector::ZeroVector;
	CurrentScriptedFireAt.TimeRemaining = InTime;
	CurrentScriptedFireAt.bOverrideTargetedEnemy = bOverrideTarget;
	CurrentScriptedFireAt.AccuracyPenaltyMultiplier = AccuracyPenaltyMultiplier;
	CurrentScriptedFireAt.bInfiniteAmmo = bInfiniteAmmo;
}

void UBaseCombatActivity::ScriptedFireAtLocation(FVector InLocation, float InTime, bool bOverrideTarget, float AccuracyPenaltyMultiplier, bool bInfiniteAmmo)
{
	if (InLocation == FVector::ZeroVector)
		return;
	
	CurrentScriptedFireAt.FireAtActor = nullptr;
	CurrentScriptedFireAt.FireAtLocation = InLocation;
	CurrentScriptedFireAt.TimeRemaining = InTime;
	CurrentScriptedFireAt.bOverrideTargetedEnemy = bOverrideTarget;
	CurrentScriptedFireAt.AccuracyPenaltyMultiplier = AccuracyPenaltyMultiplier;
	CurrentScriptedFireAt.bInfiniteAmmo = bInfiniteAmmo;
}

void UBaseCombatActivity::StopScriptedFire()
{
	if (CurrentScriptedFireAt.bInfiniteAmmo)
	{
		if (ABaseMagazineWeapon* Weapon = GetCharacter()->GetEquippedWeapon())
		{
			Weapon->bInfiniteAmmo = false;
		}
	}

	CurrentScriptedFireAt.Reset();
}

void UBaseCombatActivity::ScriptedLookAtActor(AActor* InActor, float InTime)
{
	if (!InActor)
		return;
	
	CurrentScriptedLookAt.LookAtActor = InActor;
	CurrentScriptedLookAt.TimeRemaining = InTime;
}

void UBaseCombatActivity::ScriptedLookAtLocation(FVector InLocation, float InTime)
{
	if (InLocation == FVector::ZeroVector)
		return;
	
	CurrentScriptedLookAt.LookAtActor = nullptr;
	CurrentScriptedLookAt.LookAtLocation = InLocation;
	CurrentScriptedLookAt.TimeRemaining = InTime;
}

void UBaseCombatActivity::StopScriptedLook()
{
	CurrentScriptedLookAt.Reset();
}

bool UBaseCombatActivity::IsTryingToFireAtScriptedActor() const
{
	FScriptedFireAt ScriptedFireAt;
	return IsTryingToFireAtScriptedActor(ScriptedFireAt);
}

bool UBaseCombatActivity::IsTryingToFireAtScriptedActor(FScriptedFireAt& OutScriptedFireAt) const
{
	if (ActiveEngagementType != ECombatEngagementType::FireWeapon)
		return false;

	if (HardCoverCombatMove->IsMovingToCover())
		return false;
	
	OutScriptedFireAt = CurrentScriptedFireAt;

	if (CurrentScriptedFireAt.FireAtActor || CurrentScriptedFireAt.FireAtLocation != FVector::ZeroVector)
		return CurrentScriptedFireAt.TimeRemaining > 0.0f;
	
	return false;
}

FVector UBaseCombatActivity::GetFiringPointOnActor(AActor* Actor)
{
	if (const AReadyOrNotCharacter* Character = Cast<AReadyOrNotCharacter>(Actor))
	{
		const FName TargetedBone = OwningController->GetTargetingComp()->GetTargetedBone();
		if (TargetedBone != NAME_None)
		{
			return Character->GetMesh()->GetBoneLocation(TargetedBone);
		}

		// Default to spine_3
		return Character->GetMesh()->GetBoneLocation("spine_3");
	}

	return OwningController->GetFocalPointOnActor(Actor);
}

bool UBaseCombatActivity::RunEngagementLogic(AReadyOrNotCharacter* Enemy, const float DeltaTime)
{
	return false;
}

void UBaseCombatActivity::RunNonEngagementLogic(float DeltaTime)
{
	if (!GetCharacter())
		return;

	if (!GetCharacter()->IsActive())
		return;

	if (GetCharacter()->IsPlayingDead())
		return;

	switch (ActiveEngagementType)
	{
		case ECombatEngagementType::Melee:
		{
			if (GetCharacter()->IsSuspect())
			{
				if (!GetCharacter()->GetEquippedItem())
				{
					const bool bFoundItem = TryFindPickupItem();

					if (bFoundItem)
					{
						const bool bShouldTryPickupWeaponNow = !GetCharacter()->IsAny3PMontageActive();

						if (bShouldTryPickupWeaponNow)
						{
							if (!OwningController->HasActivityType(UPickupItemActivity::StaticClass()))
							{
								UActivityManager::GiveActivityTo(PickupItemActivity, GetCharacter(), true, true);
							}
						}
					}
				}
			}
		}
		break;
		
		default:
		break;
	}
}

bool UBaseCombatActivity::CanEngageEnemy(AReadyOrNotCharacter* Enemy) const
{
	if (!Enemy)
		return false;

	if (!Enemy->IsActive())
		return false;

	return true;
}

bool UBaseCombatActivity::FireWeapon(AReadyOrNotCharacter* EnemyCharacter, bool bEnableInfiniteAmmo)
{
	#if !UE_BUILD_SHIPPING
	if (CVarCombatActivityNoShoot.GetValueOnAnyThread() > 0)
	{
		UnableToFireReason = "CombatActivity.NoShoot 1";
		
		return true;
	}
	#endif
	
	if (!OwningController || !GetCharacter())
		return false;
	
	ABaseMagazineWeapon* EquippedWeapon = GetCharacter()->GetEquippedWeapon();
	
	// Don't bother if no equipped weapon
	if (!IsValid(EquippedWeapon))
		return false;

	if (!CanShoot())
	{
		#if !UE_BUILD_SHIPPING
		if (!EquippedWeapon->HasAmmo())
		{
			UnableToFireReason = "No ammo";
		}

		if (IsReloading())
		{
			UnableToFireReason = "Reloading";
		}

		if (GetCharacter()->IsAnimationBlocking())
		{
			UnableToFireReason = "Animation is blocking";
		}

		// Can't shoot when playing a full body animation
		if (GetCharacter()->IsFullBodyMontagePlaying())
		{
			// Allow weapon recoils
			if (!GetCharacter()->IsRecoiling() && !GetCharacter()->IsPlayingHitReaction())
			{
				UnableToFireReason = "Full body montage is playing: " + GetNameSafe(GetCharacter()->GetCurrentMontage());
			}
		}

		// Can't shoot when movestyle is not a strafe
		const bool bIsMoveStyleStrafe = GetCharacter()->MoveStyle && GetCharacter()->MoveStyle->bIsStrafing;
		if (!bIsMoveStyleStrafe && OwningController->IsMoving())
		{
			UnableToFireReason = "Current move style is non-strafe: " + GetCharacter()->MoveStyle->Rep_MoveStyleName.ToString();
		}
		
		if (GetCharacter()->IsStunned() || GetCharacter()->HasRecentlyTakenStunDamage())
		{
			UnableToFireReason = "Stunned";
		}

		if (GetCharacter()->IsRaisingWeapon() || GetCharacter()->IsLoweringWeapon() ||
			GetCharacter()->IsDrawingWeapon())
		{
			UnableToFireReason = "Raising/Drawing/Lowering weapon";
		}
		
		if (const UBaseActivity* CurrentActivity = OwningController->GetCurrentActivity())
		{
			if (!CurrentActivity->CanShoot())
			{
				UnableToFireReason = CurrentActivity->GetName() + " is disabling shooting";
			}
		}
		#endif
		
		if (CanReload())
		{
			ReloadEquippedWeapon();
			return false;
		}

		//ULog::Info(GetCharacter()->GetName() + " | Can't shoot");
		
		return false;
	}

	if (EquippedWeapon->RefireDelayTimer > 0.0f)
	{
		return false;
	}
	
	if (TimeSpentWithWeaponUp < RequiredTimeSpentWithWeaponUp)
	{
		#if !UE_BUILD_SHIPPING
		UnableToFireReason = "Weapon not up yet " + FString::SanitizeFloat(TimeSpentWithWeaponUp);
		#endif
		
		//ULog::Info(GetCharacter()->GetName() + " | Can't fire. Weapon not up yet " + FString::SanitizeFloat(TimeSpentWithWeaponUp));
		return false;
	}

	if (EnemyCharacter)
	{
		const float CurrentTrackingTime = OwningController->GetTargetingComp()->GetTimeTrackingTarget();
		float RequiredTrackingTime = OwningController->GetTargetingComp()->GetRequiredTrackingTime();
		
		// Half reaction time if holding less lethal weapon
		if (EquippedWeapon->IsLessLethalWeapon())
			RequiredTrackingTime /= 2;
		
		if (CurrentTrackingTime < RequiredTrackingTime)
		{
			#if !UE_BUILD_SHIPPING
			UnableToFireReason = "Still tracking " + FString::SanitizeFloat(CurrentTrackingTime);
			#endif
		
			//ULog::Info(GetCharacter()->GetName() + " | Can't fire. Still tracking " + FString::SanitizeFloat(CurrentTrackingTime));
			return false;
		}

		/*
		#ifdef ENHANCED_SIGHT_DETECTION
		// Must track the head bone to shoot
		if (GetCharacter()->SeenBone != "head")
		{
			#if !UE_BUILD_SHIPPING
			UnableToFireReason = "Not tracking head. Tracking: " + GetCharacter()->SeenBone.ToString();
			#endif
		
			//ULog::Info(GetCharacter()->GetName() + " | Can't fire. Not tracking head. Tracking: " + GetCharacter()->SeenBone.ToString());
			return false;
		}

		if (OwningController->GetTargetingComp()->GetTimeTrackingHead() < OwningController->GetReactionTime(EActorSenseType::Sight))
		{
			#if !UE_BUILD_SHIPPING
			UnableToFireReason = "Still tracking head: " + FString::SanitizeFloat(OwningController->GetTargetingComp()->GetTimeTrackingHead());
			#endif
		
			return false;
		}
		#endif
		*/
	}

	const bool bIsLowReady = GetCharacter()->IsLowReady();
	const bool bIsStrafing = GetCharacter()->IsStrafing();

	const bool bIsWeaponUp = bIsStrafing && !bIsLowReady;
	
	const bool bInCorrectCombatState = bIsWeaponUp;

	if (bInCorrectCombatState)
	{
		const bool bHitScanSuccess = DoEquippedWeaponHitScan(EnemyCharacter);
		
		if (!bHitScanSuccess)
		{
			//ULog::Info(GetCharacter()->GetName() + " | Hit scan failed");
			return false;
		}

		// Exit early (if the hit scan hit a friendly) so as to not fire on them and kill them
		if (IsTryingToFireOnFriendly())
		{
			//ULog::Info("Firing on friendly");
			
			#if !UE_BUILD_SHIPPING
			UnableToFireReason = "Firing on friendly";
			#endif
		
			return true;
		}
		
		// add a special exception for the head if the hitscan hit the head bone of a target
		// but we're not targeting the head, ignore this hitscan
		if (GetCharacter()->IsHeadBone(GetCharacter()->CachedHitScanResult.BoneName) &&
			!GetCharacter()->IsHeadBone(OwningController->GetTargetingComp()->GetTargetedBone()))
		{
			#if !UE_BUILD_SHIPPING
			UnableToFireReason = "Hit-scan hit head bone but not targeting it";
			#endif
		
			//ULog::Info("not targeting head bone, ignoring hit scan");
			return true;
		}

		if (ABaseMagazineWeapon* Weapon = GetCharacter()->GetEquippedWeapon())
		{
			Weapon->bInfiniteAmmo = bEnableInfiniteAmmo;
		}

		EquippedWeapon->OnFireAtBulletSpawn();
		
		if (bEnableInfiniteAmmo)
		{
			if (ABaseMagazineWeapon* Weapon = GetCharacter()->GetEquippedWeapon())
			{
				Weapon->bInfiniteAmmo = false;
			}
		}

		float FireRate = GetFireRate(EquippedWeapon);
		float Deviation = GetFireRateDeviationPercentage(EquippedWeapon);

		// only want single fire to apply to SWAT
		if (EnemyCharacter && GetCharacter() && GetCharacter()->IsOnSWATTeam() && EquippedWeapon->ItemClass != EItemClass::IC_Pistol)
		{
			// Should probably be single
			const float Distance = GetCharacter()->GetDistanceTo(EnemyCharacter);
			if (Distance > AI_CONFIG_GET_FLOAT("SwatSemiAutoDistanceStart"))
			{
				if (EquippedWeapon->AvailableFireModes.Contains(EFireMode::FM_Auto) && EquippedWeapon->CurrentFireMode == EFireMode::FM_Auto)
				{
					EquippedWeapon->CurrentFireMode = EFireMode::FM_Single;
					GetCharacter()->PlayNonLocal3PMontage(EquippedWeapon->AnimationData->FireSelect_Semi.Body_TP);
				}
			}
			// Should be auto
			else
			{
				if (EquippedWeapon->AvailableFireModes.Contains(EFireMode::FM_Auto) && EquippedWeapon->CurrentFireMode == EFireMode::FM_Single)
				{
					EquippedWeapon->CurrentFireMode = EFireMode::FM_Auto;
					GetCharacter()->PlayNonLocal3PMontage(EquippedWeapon->AnimationData->FireSelect_Auto.Body_TP);
				}
			}

			if (EquippedWeapon->CurrentFireMode == EFireMode::FM_Single)
			{
				float FireRateMultiplierStart;
				float FireRateMultiplierEnd;
				
				switch (EquippedWeapon->ItemClass)
				{
				case EItemClass::IC_AssaultRifle:	
				FireRateMultiplierStart = AI_CONFIG_GET_FLOAT("SwatRifleSemiFireRateMultiplierStart", 5);
				FireRateMultiplierEnd = AI_CONFIG_GET_FLOAT("SwatRifleSemiFireRateMultiplierEnd", 5);
					break;
				case EItemClass::IC_SMG:
					FireRateMultiplierStart = AI_CONFIG_GET_FLOAT("SwatSMGSemiFireRateMultiplierStart", 5);
					FireRateMultiplierEnd = AI_CONFIG_GET_FLOAT("SwatSMGSemiFireRateMultiplierEnd", 5);
					break;
				case EItemClass::IC_LMG:
					FireRateMultiplierStart = AI_CONFIG_GET_FLOAT("SwatLMGSemiFireRateMultiplierStart", 5);
					FireRateMultiplierEnd = AI_CONFIG_GET_FLOAT("SwatLMGSemiFireRateMultiplierEnd", 5);
					break;
				case EItemClass::IC_LessLethal:
					FireRateMultiplierStart = AI_CONFIG_GET_FLOAT("SwatLessLethalSemiFireRateMultiplierStart", 5);
					FireRateMultiplierEnd = AI_CONFIG_GET_FLOAT("SwatLessLethalSemiFireRateMultiplierEnd", 5);
					break;
				default:
					FireRateMultiplierStart = AI_CONFIG_GET_FLOAT("SwatDefaultSemiFireRateMultiplierStart", 5);
					FireRateMultiplierEnd = AI_CONFIG_GET_FLOAT("SwatDefaultSemiFireRateMultiplierEnd", 5);
					break;
				}

				const float StartDistance = AI_CONFIG_GET_FLOAT("SwatSemiAutoDistanceStart");
				const float EndDistance = AI_CONFIG_GET_FLOAT("SwatSemiAutoDistanceEnd");
				const float DeviationMultiplier = AI_CONFIG_GET_FLOAT("SwatSemiAutoDeviationMultiplier");
				
				const float NewFireRateMultiplier = FireRateMultiplierStart + (FireRateMultiplierEnd - FireRateMultiplierStart)*(Distance - StartDistance)/(EndDistance - StartDistance);
					
				FireRate = FireRate*(NewFireRateMultiplier + NewFireRateMultiplier*Deviation*DeviationMultiplier);
				EquippedWeapon->RefireDelayTimer = FireRate;
			}
		}
		else
		{
			EquippedWeapon->RefireDelayTimer = FireRate + FireRate * Deviation;
		}
		
		GetCharacter()->Multicast_ChangeFaceEmotion(ECharacterEmotion::Angry, 5.0f, 1.0f, 0.25f, 1);

		return true;
	}

	return false;
}

bool UBaseCombatActivity::ShouldStrafe() const
{
	if ((CurrentScriptedFireAt.FireAtActor || CurrentScriptedFireAt.FireAtLocation != FVector::ZeroVector) && CurrentScriptedFireAt.TimeRemaining > 0.0f)
	{
		//ULog::Info("ShouldStrafe | Scripted fire");
		return true;
	}
	
	if ((CurrentScriptedLookAt.LookAtActor || CurrentScriptedLookAt.LookAtLocation != FVector::ZeroVector) && CurrentScriptedLookAt.TimeRemaining > 0.0f)
	{
		//ULog::Info("ShouldStrafe | Scripted fire");
		return true;
	}

	if (GetCharacter()->IsHesitating() || GetCharacter()->IsStartling())
	{
		return false;
	}

	if (GetCharacter()->IsExitingSurrender())
	{
		return true;
	}
	
	if (GetCharacter()->IsAnimationBlocking())
	{
		if (const UAnimMontage* Montage = GetCharacter()->GetCurrentMontage())
		{
			const float BlendOutTime = Montage->GetDefaultBlendOutTime() + 0.1f;
			float TimeRemaining = 0.0f;
			if (GetCharacter()->IsMontagePlayingWithTimeRemaining(Montage, TimeRemaining))
			{
				if (TimeRemaining <= BlendOutTime)
				{
					//ULog::Info("ShouldStrafe | Animation is blocking but allow it");
					return true;
				}
			}
		}
		
		//ULog::Info("ShouldStrafe | Animation is blocking");
		return false;
	}
	
	if (GetCharacter()->IsPlayingDead())
		return false;
	
	if (const UBaseActivity* CurrentActivity = OwningController->GetCurrentActivity())
	{
		if (CurrentActivity->ShouldForceStrafe())
		{
			//ULog::Info("ShouldStrafe | " + CurrentActivity->GetName() + " wants strafe");
			return true;
		}

		if (CurrentActivity->ShouldForceNoStrafe())
		{
			//ULog::Info("ShouldStrafe | " + CurrentActivity->GetName() + " wants no strafe");
			return false;
		}
	}

	if (CombatMoveActivity)
	{
		if (CombatMoveActivity->ShouldForceStrafe())
		{
			//ULog::Info("ShouldStrafe | " + CombatMoveActivity->GetName() + " wants strafe");
			return true;
		}

		if (CombatMoveActivity->ShouldForceNoStrafe())
		{
			//ULog::Info("ShouldStrafe | " + CombatMoveActivity->GetName() + " wants no strafe");
			return false;
		}
	}
	
	const bool bIsMoveStyleStrafe = GetCharacter()->MoveStyle && GetCharacter()->MoveStyle->bIsStrafing;
	if (!bIsMoveStyleStrafe && GetOwningController()->IsMoving())
		return false;

	if (OwningController->IsCivilian() && OwningController->GetPathFollowingComponent()->GetStatus() != EPathFollowingStatus::Moving)
		return false;
	
	// Are tracking an enemy or have seen one in the past 30 seconds
	if (OwningController->GetTrackedTarget())
	{
		//ULog::Info("ShouldStrafe | Tracking an enemy (" + GetNameSafe(OwningController->GetTrackedTarget()) + ")");
		return true;
	}
	
	if (GetCharacter()->GetTimeSinceLastStun() < 30.0f)
		return true;

	if (OwningController->GetTargetingComp()->GetTimeSinceLastSeenEnemy() < OwningController->GetTargetingComp()->GetLastKnownTrackingTimeConfig())
	{
		//ULog::Info("ShouldStrafe | Tracking an enemy (since " + FString::SanitizeFloat(OwningController->GetTargetingComp()->GetTimeSinceLastSeenEnemy()) + " secs)");
		return true;
	}
	
	if (OwningController->HasBeenExposedToAggressiveNoise(30.0f, 2000.0f, (int32)EAITargetType::Enemy))
	{
		//ULog::Info("ShouldStrafe | Tracking an enemy (since " + FString::SanitizeFloat(OwningController->GetTargetingComp()->GetTimeSinceLastSeenEnemy()) + " secs)");
		return true;
	}
	
	if (OwningController->GetAwarenessState() > EAIAwarenessState::Unalerted)
	{
		//ULog::Info("ShouldStrafe | Not unalerted");
		return true;
	}
	
	if (!OwningController->GetCurrentActivity<UTeamStackUpActivity>() && OwningController->IsSWAT())
	{
		if (OwningController->GetTargetingComp()->GetTrackingType() != ETargetingCompTracking::TCT_None &&
			OwningController->GetTargetingComp()->GetTrackingType() != ETargetingCompTracking::TCT_TrackingMoveVector)
		{
			return true;
		}
	}
	
	return false;
}

bool UBaseCombatActivity::TryFindPickupItem()
{
	TArray<ABaseItem*> PossiblePickupWeapons = GetCharacter()->GetInventoryComponent()->GetRemovedInventoryItems();

	PossiblePickupWeapons.RemoveAll([](ABaseItem* Item)
	{
		if (const ABaseMagazineWeapon* Weapon = Cast<ABaseMagazineWeapon>(Item))
		{
			return !Weapon->HasAnyAmmo();
		}

		return false;
	});

	return UReadyOrNotFunctionLibrary::FindClosestActorFromLocation(GetCharacter()->GetActorLocation(), PossiblePickupWeapons) != nullptr;
}

bool UBaseCombatActivity::IsTryingToFireOnFriendly() const
{
	if (GetCharacter())
	{
		return GetCharacter()->bHitScannedFriendly;
	}
	
	return false;
}

int32 UBaseCombatActivity::GetWeaponAmmoRemaining() const
{
	if (GetCharacter())
	{
		if (const ABaseMagazineWeapon* EquippedWeapon = GetCharacter()->GetEquippedItem<ABaseMagazineWeapon>())
		{
			return FMath::FloorToInt(EquippedWeapon->GetAmmo());
		}
	}

	return 0;
}

bool UBaseCombatActivity::ShouldTriggerReloadNow() const
{
	return GetCharacter()->bWantsReload;
}

bool UBaseCombatActivity::ShouldFinishCombatMoveNow() const
{
	if (!GetCharacter()->IsActive())
		return true;

	// Special case for when stunned by gas: If we're trying to flee, and we're only stunned with gas, we can continue
	if (GetCharacter()->IsStunned() && !(GetCharacter()->IsOnlyStunnedWithGas() && CombatMoveActivity == FleeingCombatMove))
		return true;
		
	if (GetCharacter()->IsPlayingDead())
		return true;
		
	if (IsExplodingVest())
		return true;

	if (OwningController->GetCurrentActivity<UMoveToExitActivity>())
		return true;
	
	return false;
}

bool UBaseCombatActivity::DoEquippedWeaponHitScan(AReadyOrNotCharacter* EnemyCharacter)
{
	ABaseMagazineWeapon* EquippedWeapon = GetCharacter()->GetEquippedWeapon();

	if (!EquippedWeapon)
		return false;

	FVector BulletSpawnLocation = EquippedWeapon->GetBulletSpawn()->GetComponentLocation();
	FVector BulletSpawnForward = EquippedWeapon->GetBulletSpawn()->GetForwardVector();

	AActor* TargetedActor = EnemyCharacter;
	FVector TargetedLocation = GetFiringPointOnActor(TargetedActor);
	FVector FocalPoint = TargetedLocation;

	/*
	if (TargetedActor)
	{
		FocalPoint = OwningController->GetFocalPointOnActor(TargetedActor);
	}
	*/
	
	float Threshold = 0.95f;
	
	if (UBaseActivity* Activity = OwningController->GetCurrentActivity())
	{
		float Override = Threshold;
		if (Activity->OverrideFireAngleThreshold(Override))
		{
			Threshold = Override;
		}
	}

	if (GetCharacter()->IsPlayingHitReaction())
	{
		Threshold -= 0.25f;
	}

	float ScriptedFireAccuracyLostMultiplier = 1.0f;
	
	if (CurrentScriptedFireAt.FireAtLocation != FVector::ZeroVector && (CurrentScriptedFireAt.bOverrideTargetedEnemy || !TargetedActor) && CurrentScriptedFireAt.TimeRemaining > 0.0f)
	{
		TargetedLocation = CurrentScriptedFireAt.FireAtLocation;
		FocalPoint = TargetedLocation;
		ScriptedFireAccuracyLostMultiplier = CurrentScriptedFireAt.AccuracyPenaltyMultiplier;
		Threshold = CurrentScriptedFireAt.FireAngleThreshold;
	}
	else if (CurrentScriptedFireAt.FireAtActor && (CurrentScriptedFireAt.bOverrideTargetedEnemy || !TargetedActor) && CurrentScriptedFireAt.TimeRemaining > 0.0f)
	{
		TargetedActor = CurrentScriptedFireAt.FireAtActor;
		TargetedLocation = GetFiringPointOnActor(TargetedActor);
		FocalPoint = OwningController->GetFocalPointOnActor(CurrentScriptedFireAt.FireAtActor);
		ScriptedFireAccuracyLostMultiplier = CurrentScriptedFireAt.AccuracyPenaltyMultiplier;
		Threshold = CurrentScriptedFireAt.FireAngleThreshold;
	}
	
	GetCharacter()->FireAngleThreshold = Threshold;

	if (TargetedLocation == FVector::ZeroVector || !FAISystem::IsValidLocation(TargetedLocation))
	{
		#if !UE_BUILD_SHIPPING
		UnableToFireReason = "Invalid target location";
		#endif
		
		//ULog::Info("Can't fire at target. Invalid target location");
		return false;
	}

	const FVector DirectionToFocalPoint = (FocalPoint - BulletSpawnLocation).GetSafeNormal();
	const FVector DirectionToTarget = (TargetedLocation - BulletSpawnLocation).GetSafeNormal();

	// Dont try to hit scan when gun is not yet directly/almost directly looking at the tracked enemy
	float DotProduct = FVector::DotProduct(BulletSpawnForward, DirectionToFocalPoint);

	float DistanceToTarget = FVector::Distance(FocalPoint, BulletSpawnLocation);

	/*
	DrawDebugLine(GetWorld(), FocalPoint, FocalPoint + FVector(0.0f, 0.0, 1000000.0f), FColor::White, false, 1.0f, 0, 2.0f);
	DrawDebugBox(GetWorld(), FocalPoint, FVector(20.0f), FColor::White, false, 1.0f, 0, 5.0f);
	DrawDebugBox(GetWorld(), BulletSpawnLocation, FVector(10.0f), FColor::Magenta, false, 1.0f);
	LOG_NUMBER(DistanceToTarget);
	*/
	
	if (DistanceToTarget < 200.0f)
	{
		DotProduct = 1.0f;
	}
	
	if (TargetedActor)
	{
		if (GetCharacter()->TimeInsideFireAngleThreshold < 0.1f)
		{
			#if !UE_BUILD_SHIPPING
			UnableToFireReason = "Not enough time inside the fire angle threshold: " + FString::SanitizeFloat(GetCharacter()->TimeInsideFireAngleThreshold);
			#endif
		
			return false;
		}
	}
	
	if (DotProduct < Threshold)
	{
		#if !UE_BUILD_SHIPPING
		UnableToFireReason = "Dot product test failed (< " + FString::SanitizeFloat(GetCharacter()->FireAngleThreshold) + "). Actual: " + FString::SanitizeFloat(DotProduct);
		#endif
		
		//ULog::Info("Can't fire at target. Dot product test failed (< " + FString::SanitizeFloat(GetCharacter()->FireAngleThreshold) + "). Actual: " + FString::SanitizeFloat(DotProduct));
		return false;
	}

	FRotator SpawnRot = DirectionToTarget.Rotation();

	float AccuracyOffsetInDegrees = GetCharacter()->IsSuspect() ? AI_CONFIG_GET_FLOAT("SuspectAccuracy") : AI_CONFIG_GET_FLOAT("SwatAccuracy");
	float AccuracyLostPerMeter = GetCharacter()->IsSuspect() ? AI_CONFIG_GET_FLOAT("SuspectAccuracyLostPerTenMetersToTarget") : AI_CONFIG_GET_FLOAT("SwatAccuracyLostPerTenMetersToTarget");
	
	if (UAIArchetypeData* ArchetypeData = GetCharacter()->GetAIArchetype())
	{
		if (ArchetypeData->bAccuracyOverride)
			AccuracyOffsetInDegrees = ArchetypeData->Accuracy;
	}

	const float DistToEnemy = FVector::Distance(TargetedLocation, GetCharacter()->GetActorLocation());

	float MovementAccuracyPenalty = 0.0f;
	float Velocity = GetCharacter()->GetVelocity().Size();
	if (Velocity > 100.0f)
		MovementAccuracyPenalty = (Velocity / 100.0f) * (GetCharacter()->IsOnSWATTeam() ? AI_CONFIG_GET_FLOAT("SwatAccuracyLostPerMeterSecond") : AI_CONFIG_GET_FLOAT("SuspectAccuracyLostPerMeterSecond"));

	if (GetCharacter()->IsOnSWATTeam())
	{
		float VeteranModifier = 1.0f - FMath::Clamp(URosterManager::GetSquadTraitValue("Veteran", GetWorld()), 0.0f, 1.0f);

		AccuracyOffsetInDegrees *= VeteranModifier;
		MovementAccuracyPenalty *= VeteranModifier;
	}
	
	AccuracyOffsetInDegrees += MovementAccuracyPenalty;
	AccuracyOffsetInDegrees += AccuracyLostPerMeter * (DistToEnemy / 1000.0f);

	float ShrinkConeAtRangeScale = GetCharacter()->IsSuspect() ? AI_CONFIG_GET_FLOAT("ShrinkAccuracyConeAtRangeScale") : AI_CONFIG_GET_FLOAT("ShrinkAccuracyConeAtRangeScale");
	float ShrinkConeAtRange = (DistToEnemy / 1000.0f) * ShrinkConeAtRangeScale;
	if (ShrinkConeAtRange > 1.0f)
	{
		AccuracyOffsetInDegrees /= ShrinkConeAtRange;
	}
	
	
	AccuracyOffsetInDegrees *= ScriptedFireAccuracyLostMultiplier;
	
	if (!GetCharacter()->IsOnSWATTeam())
	{
		AccuracyOffsetInDegrees *= GetCharacter()->AccuracyNerfPercentage;
		AccuracyOffsetInDegrees *= ExplosiveVestAccuracyMultiplier;
		AccuracyOffsetInDegrees *= 1.0f + GetCharacter()->StunAccuracyPenalty;
		AccuracyOffsetInDegrees *= 1.0f + GetCharacter()->PepperSprayAccuracyPenalty;
	}

	//if (TargetedActor && !OwningController->LineOfSightTo(TargetedActor))
	/*
	if (TargetedActor && !OwningController->GetTargetingComp()->CanActorBeSeen(TargetedActor))
	{
		AccuracyOffsetInDegrees += 5.0f;
	}
	*/

	// GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::White, "Accuracy Penalty: " + FString::SanitizeFloat(AccuracyOffsetInDegrees));
	
	if (!GetCharacter()->IsOnSWATTeam())
	{
	 	if (EquippedWeapon->ItemClass == EItemClass::IC_AssaultRifle ||
	 		EquippedWeapon->ItemClass == EItemClass::IC_SMG)
	 	{
	 		if (EquippedWeapon->BulletsFired < 2 && GetCharacter()->TimeSinceLastShot > 1.5f)
	 		{
	 			AccuracyOffsetInDegrees += 3.0f;
	 		}
	 	}
		
		if (OneFrameAccuracyMultiplier != 1.0f)
		{
			AccuracyOffsetInDegrees *= OneFrameAccuracyMultiplier;
			OneFrameAccuracyMultiplier = 1.0f;
		}
	}

	SpawnRot.Yaw += FMath::RandRange(-AccuracyOffsetInDegrees, AccuracyOffsetInDegrees);
	SpawnRot.Pitch += FMath::RandRange(-AccuracyOffsetInDegrees, AccuracyOffsetInDegrees);

	FVector TraceStart = BulletSpawnLocation;
	FVector TraceEnd = TraceStart + SpawnRot.Vector() * 100000.0f;

	FHitResult Hit;
	FCollisionQueryParams CollisionQueryParams = GetCharacter()->GetCollisionQueryParameters();
	CollisionQueryParams.bTraceComplex = true;
	CollisionQueryParams.bReturnPhysicalMaterial = true;
	
	if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_PROJECTILE, CollisionQueryParams))
	{
		GetCharacter()->bHitScannedFriendly = false;
		
		if (AReadyOrNotCharacter* HitCharacter = Cast<AReadyOrNotCharacter>(Hit.GetActor()))
		{
			bool bSameTeam = AReadyOrNotCharacter::IsOnSameTeam(HitCharacter, GetCharacter()) && !OwningController->IsCharacterKnownEnemy(HitCharacter);
			
			GetCharacter()->bHitScannedFriendly = bSameTeam;
			
			// only allow AI to shoot AI and hit them that are looking towards them
			// quick hack to make AI <-> AI fair fights always no one gets shot in the back around here
			if (bSameTeam && FVector::DotProduct(GetCharacter()->GetActorForwardVector(), HitCharacter->GetActorForwardVector()) > 0.0f)
			{
				GetCharacter()->CachedHitScanResult = {};
				return false;
			}
		}
	}
	
	GetCharacter()->CachedHitScanResult = Hit;
	
	#if !UE_BUILD_SHIPPING
	if (CVarCombatActivityDrawFiringLogic.GetValueOnAnyThread() > 0)
		DrawDebugLine(GetWorld(), Hit.TraceStart, Hit.TraceEnd, FColor::Cyan, false, 1.0f, 0, 1);
	#endif
	
	return true;
}

bool UBaseCombatActivity::TryTrackEnemy(AReadyOrNotCharacter* NewTrackedEnemy)
{
	if (!NewTrackedEnemy)
		return false;

	if (!OwningController->IsCharacterEnemy(NewTrackedEnemy))
		return false;
	
	if (LastTrackedEnemy != NewTrackedEnemy)
	{
		// Remove previous binding if LastTrackedEnemy was set
		if (LastTrackedEnemy)
		{
			LastTrackedEnemy->OnWeaponFire.RemoveAll(this);
			LastTrackedEnemy->OnCharacterKilled.RemoveAll(this);
		}

		// Listen to whenever this new tracked enemy AI is firing
		LastTrackedEnemy = NewTrackedEnemy;
		LastTrackedEnemy->OnWeaponFire.RemoveAll(this);
		LastTrackedEnemy->OnWeaponFire.AddDynamic(this, &UBaseCombatActivity::TrackEnemyFire);
		LastTrackedEnemy->OnCharacterKilled.RemoveAll(this);
		LastTrackedEnemy->OnCharacterKilled.AddDynamic(this, &UBaseCombatActivity::TrackEnemyKilled);

		// Reset, we're tracking someone now
		CurrentScriptedFireAt.Reset();
		
		OnTrackEnemy(NewTrackedEnemy);
		
		return true;
	}

	return false;
}

bool UBaseCombatActivity::IsFocusingOnActor(const AActor* InActor) const
{
	if (!InActor || !OwningController)
		return false;

	return OwningController->GetFocusActor() == InActor;
}

bool UBaseCombatActivity::TryMoveIntoCoverLandmark(const FVector& ThreatLocation, const FVector& ThreatDirection, const float MinDistanceFromInstigator, AReadyOrNotCharacter* InstigatorCharacter)
{
	if (!OwningController || !GetCharacter())
		return false;

	if (bAmbushAttacking)
		return false;

	if (IsExplodingVest())
		return false;
	
	if (GetCharacter()->IsTakingHostage())
		return false;
	
	if (CurrentCoverLandmarkEvaluationCooldown > 0.0f)
		return false;

	LastTrackedEnemy = InstigatorCharacter;

	StartRunningCombatMove(HardCoverCombatMove);

	if (CombatMoveActivity == HardCoverCombatMove)
	{
		if (HardCoverCombatMove->TryMoveIntoCoverLandmark(ThreatLocation, ThreatDirection, MinDistanceFromInstigator, InstigatorCharacter))
		{
			// Reset cooldown so the logic doesn't get spammed. Potentially bugging out AI
			CurrentCoverLandmarkEvaluationCooldown = CoverLandmarkEvaluationCooldown;
			return true;
		}
	}

	return false;
}

bool UBaseCombatActivity::TryMoveIntoCoverLandmark()
{
	if (!OwningController || !GetCharacter())
		return false;

	if (bAmbushAttacking)
		return false;

	if (IsExplodingVest())
		return false;
	
	if (GetCharacter()->IsTakingHostage())
		return false;
	
	if (CurrentCoverLandmarkEvaluationCooldown > 0.0f)
		return false;

	StartRunningCombatMove(HardCoverCombatMove);

	if (CombatMoveActivity == HardCoverCombatMove)
	{
		if (HardCoverCombatMove->TryMoveIntoCoverLandmark())
		{
			// Reset cooldown so the logic doesn't get spammed. Potentially bugging out AI
			CurrentCoverLandmarkEvaluationCooldown = CoverLandmarkEvaluationCooldown;
			return true;
		}
	}

	return false;
}

bool UBaseCombatActivity::TryMoveIntoCover(const FCoverInstigatorStimulus& InstigatorStimulus, const bool bRequireLOS)
{
	if (!OwningController || !GetCharacter() || !InstigatorStimulus.IsValid())
		return false;

	if (!GetCharacter()->IsActive())
		return false;

	if (GetCharacter()->IsSuspect() && bAmbushAttacking)
		return false;

	if (IsExplodingVest())
		return false;

	if (GetCharacter()->IsTakingHostage())
		return false;

	if (HardCoverCombatMove->IsRequestingCover())
		return true;
	
	//if (CurrentCoverEvaluationCooldown > 0.0f)
	//	return false;

	LastTrackedEnemy = InstigatorStimulus.InstigatorCharacter;

	StartRunningCombatMove(HardCoverCombatMove);

	if (CombatMoveActivity == HardCoverCombatMove)
	{
		if (HardCoverCombatMove->TryMoveIntoCover(InstigatorStimulus, bRequireLOS))
		{
			// Reset cooldown so the logic doesn't get spammed. Potentially bugging out AI
			//CurrentCoverEvaluationCooldown = CoverEvaluationCooldown;
			return true;
		}
	}

	return false;
}

void UBaseCombatActivity::TrackEnemyFire(AReadyOrNotCharacter* FromCharacter, ABaseMagazineWeapon* Weapon, FVector FireDirection)
{
	if (!FromCharacter || !Weapon)
		return;

	if (!OwningController || !GetCharacter())
		return;
	
	LastTrackedEnemyFireDirection = FireDirection;
}

void UBaseCombatActivity::TrackEnemyKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter)
{
}

void UBaseCombatActivity::OnTakeDamage(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* DamagedCharacter, AActor* DamageCauser, float Damage, float HealthRemaining)
{
	if (!OwningController || !GetCharacter())
	{
		return;
	}

	if (OwningController->BestAction)
	{
		if (UAIAction* CustomAction = OwningController->BestAction->GetCustomAction(OwningController))
		{
			CustomAction->OnTakeDamage(Damage, InstigatorCharacter);
		}
	}
	
	if (OwningController->BestCombatMoveAction)
	{
		if (UAIAction* CustomAction = OwningController->BestCombatMoveAction->GetCustomAction(OwningController))
		{
			CustomAction->OnTakeDamage(Damage, InstigatorCharacter);
		}
	}
	
	if (Cast<APlayerCharacter>(InstigatorCharacter))
	{
		if (!GetCharacter()->bIsPlayingDead)
		{
			if (GetCharacter()->IsSuspect())
			{
				#if !UE_BUILD_SHIPPING
				if (CVarCombatActivityAlwaysPlayDead.GetValueOnAnyThread() > 0)
				{
					TryPlayDead(false, true);
					return;
				}
				#endif

				if (FVector::Distance(InstigatorCharacter->GetActorLocation(), GetCharacter()->GetActorLocation()) > 700.0f)
				{
					// Try to play dead only when we're about to die
					// TODO: some AI could increase or decrease this chance
					if (GetCharacter()->IsLowHealth() && FMath::FRand() < 0.4f)
					{
						TryPlayDead();
						return;
					}
				}
			}
		}
	}
}

void UBaseCombatActivity::OnStunned(AReadyOrNotCharacter* StunnedCharacter, float Duration, EStunType StunType, AActor* DamageCauser)
{
	if (!GetCharacter()->bIsPlayingDead)
	{
		if (GetCharacter()->IsSuspect())
		{
			const bool bSemiLethal = StunType == EStunType::ST_Stung || StunType == EStunType::ST_Tased || StunType == EStunType::ST_Rubberball;
			if (bSemiLethal)
			{
				#if !UE_BUILD_SHIPPING
				if (CVarCombatActivityAlwaysPlayDead.GetValueOnAnyThread() > 0)
				{
					TryPlayDead(false, true);
					return;
				}
				#endif

				// Small chance of playing dead
				if (GetCharacter()->IsLowHealth() && FMath::FRand() < 0.4f)
				{
					TryPlayDead();
				}
			}
		}
	}
}

void UBaseCombatActivity::FinishCombatMove(const bool bSuccess)
{
	if (CombatMoveActivity)
	{
		#if !UE_BUILD_SHIPPING
		ULog::Info("Finished " + CombatMoveActivity->GetName());
		#endif

		if (!bSuccess &&
			(!PreviousCombatMoveActivity || (PreviousCombatMoveActivity == CombatMoveActivity)))
		{
			CombatMoveActivity->FailureCount++;
			CombatMoveActivity->LastFailTime = GetWorld()->GetTimeSeconds();
		}
		else
		{
			if (!bSuccess)
			{
				CombatMoveActivity->FailureCount++;
				CombatMoveActivity->LastFailTime = GetWorld()->GetTimeSeconds();
			}
			else
			{
				CombatMoveActivity->FailureCount = 0;
				CombatMoveActivity->LastSuccessTime = GetWorld()->GetTimeSeconds();
			}
		}

		LOG_NUMBER(CombatMoveActivity->FailureCount);
		
		if (GetCharacter())
			CombatMoveActivity->FinishedActivity(bSuccess);
		else
			CombatMoveActivity->FinishedActivity_NoOwner(bSuccess);

		PreviousCombatMoveActivity = CombatMoveActivity;
		CombatMoveActivity->ResetData();
		CombatMoveActivity = nullptr;
	}
}

bool UBaseCombatActivity::HasRecentlySeenTarget(AReadyOrNotCharacter* TargetCharacter) const
{
	if (OwningController->GetTargetingComp()->GetTimeSinceLastSeenEnemy() < 0.5f)
	{
		return true;
	}
	
	if (OwningController->GetTargetingComp()->CanCharacterBeSeen(TargetCharacter))
	{
		return true;
	}

	if (const UTakeCoverActivity* TakeCoverActivity = OwningController->GetCurrentActivity<UTakeCoverActivity>())
	{
		/*if (TakeCoverActivity->IsMovingToCover())
			return true;*/
			
		if (TakeCoverActivity->IsCoverFiring() && GetCharacter()->ActiveCoverFireType == ECoverFireType::Blind)
			return true;
	}
	
	return false;
}

float UBaseCombatActivity::GetFireRate(const ABaseMagazineWeapon* MagazineWeapon) const
{
	if (!GetCharacter() ||!MagazineWeapon)
		return -1.0f;
	
	const FString TeamPrefixString = GetCharacter()->IsOnSWATTeam() ? "Swat" : "Suspect";
	
	float FireRate;
	switch (MagazineWeapon->ItemClass)
	{
		case EItemClass::IC_AssaultRifle:	FireRate = AI_CONFIG_GET_FLOAT(TeamPrefixString + "RifleFireRate"); break;
		case EItemClass::IC_SMG:			FireRate = AI_CONFIG_GET_FLOAT(TeamPrefixString + "SMGFireRate"); break;
		case EItemClass::IC_LMG:			FireRate = AI_CONFIG_GET_FLOAT(TeamPrefixString + "LMGFireRate"); break;
		case EItemClass::IC_Pistol:			FireRate = AI_CONFIG_GET_FLOAT(TeamPrefixString + "PistolFireRate"); break;
		case EItemClass::IC_Shotgun:		FireRate = AI_CONFIG_GET_FLOAT(TeamPrefixString + "ShotgunFireRate"); break;
		case EItemClass::IC_LessLethal:		FireRate = AI_CONFIG_GET_FLOAT(TeamPrefixString + "LessLethalFireRate"); break;
		
		default:							FireRate = AI_CONFIG_GET_FLOAT(TeamPrefixString + "DefaultFireRate", 0.1f); break;
	}

	return FMath::Clamp(FireRate, 0.008f, 5.0f);
}

float UBaseCombatActivity::GetFireRateDeviationPercentage(const ABaseMagazineWeapon* MagazineWeapon) const
{
	if (!GetCharacter() || !MagazineWeapon)
		return -1.0f;

	const FString TeamPrefixString = GetCharacter()->IsOnSWATTeam() ? "Swat" : "Suspect";
	
	FVector2D DeviationRange;
	switch (MagazineWeapon->ItemClass)
	{
		case EItemClass::IC_AssaultRifle:	DeviationRange = AI_CONFIG_GET_VECTOR2D(TeamPrefixString + "RifleFireRateDeviation"); break;
		case EItemClass::IC_SMG:			DeviationRange = AI_CONFIG_GET_VECTOR2D(TeamPrefixString + "SMGFireRateDeviation"); break;
		case EItemClass::IC_LMG:			DeviationRange = AI_CONFIG_GET_VECTOR2D(TeamPrefixString + "LMGFireRateDeviation"); break;
		case EItemClass::IC_Pistol:			DeviationRange = AI_CONFIG_GET_VECTOR2D(TeamPrefixString + "PistolFireRateDeviation"); break;
		case EItemClass::IC_Shotgun:		DeviationRange = AI_CONFIG_GET_VECTOR2D(TeamPrefixString + "ShotgunFireRateDeviation"); break;
		case EItemClass::IC_LessLethal:		DeviationRange = AI_CONFIG_GET_VECTOR2D(TeamPrefixString + "LessLethalFireRateDeviation"); break;
		
		default:							DeviationRange = FVector2D::ZeroVector; break;
	}

	return FMath::FRandRange(DeviationRange.X, DeviationRange.Y);
}

bool UBaseCombatActivity::TryCommitSuicide(const bool bFakeOut)
{
	if (!GetCharacter())
		return false;
	
	if (IsReloading())
		return false;
	
	bConsideredSuicide = true;

	if (!OwningController->HasActivityType(UCommitSuicideActivity::StaticClass()))
	{
		float Chance = AI_CONFIG_GET_FLOAT("SuicideChance");
		if (GetCharacter()->AssignedAIData->bOverrideSuicideChance)
			Chance = GetCharacter()->AssignedAIData->SuicideChance;
		
		const bool bCanCommitSuicide = GetCharacter()->AssignedAIData->bCanEverSuicide && FMath::FRand() <= Chance;
		const bool bCanCommitSuicideNow = bCanCommitSuicide && GetCharacter()->GetEquippedWeapon();
					
		if (bCanCommitSuicideNow)
		{
			CommitSuicideActivity->bFakeOut = bFakeOut;
			return UActivityManager::GiveActivityTo(CommitSuicideActivity, GetCharacter(), true, true);
		}
	}

	return false;
}

void UBaseCombatActivity::ResetSuicideConsideration()
{
	bConsideredSuicide = false;
}

void UBaseCombatActivity::OnCoverFound()
{
}

void UBaseCombatActivity::OnNoCoverFound()
{
	if (GetCombatMoveActivity() == HardCoverCombatMove)
	{
		if (!OwningController->GetActivity<UTakeCoverActivity>())
			FinishCombatMove();
	}
}

void UBaseCombatActivity::OnCoverExit()
{
}

void UBaseCombatActivity::OnRequestCover()
{
}

void UBaseCombatActivity::OnRequestCoverLandmark()
{
}

void UBaseCombatActivity::OnCoverLandmarkExit()
{
}

bool UBaseCombatActivity::IsExplodingVest() const
{
	if (const AExplosiveVest* ExplosiveVest = Cast<AExplosiveVest>(GetCharacter()->GetArmour()))
	{
		return GetCharacter()->IsTableMontagePlaying(ExplosiveVest->DetonationMontage);
	}

	return false;
}

bool UBaseCombatActivity::CanPlayDead() const
{
	return false;
}

bool UBaseCombatActivity::TryPlayDead(const bool bSilentDeath, const bool bForce)
{
	if (!GetCharacter()->IsActive())
		return false;
	
	if (AReadyOrNotLevelScript::GlobalPlayDeadCooldown > 0.0f && !bForce)
		return false;

	if (GetCharacter()->bCommitingSuicide)
		return false;

	if (GetCharacter()->IsTakingCoverAtLandmark())
		return false;

	if (!OwningController->HasActivityType(UPlayDeadActivity::StaticClass()))
	{
		PlayDeadActivity->bSilentDeath = bSilentDeath;
		return UActivityManager::GiveActivityTo(PlayDeadActivity, GetCharacter(), true, true);
	}

	return false;
}

void UBaseCombatActivity::PlayDeadStarted(UBaseActivity* Activity, ACyberneticController* Controller)
{
	AReadyOrNotLevelScript::GlobalPlayDeadCooldown = 60.0f;

	FinishCombatMove();
}

void UBaseCombatActivity::PlayDeadFinished(UBaseActivity* Activity, ACyberneticController* Controller)
{
	bTryPlayDead = false;
	AReadyOrNotLevelScript::GlobalPlayDeadCooldown = 60.0f;
}

void UBaseCombatActivity::OnSuicideFakeOutSuccess()
{
	if (OwningController->GetTargetingComp()->HasLineOfSightToLastTrackedTarget())
	{
		ScriptedFireAtActor(OwningController->GetLastTrackedEnemy(), 1.0f, true, 1.0f);
	}
}

bool UBaseCombatActivity::IsActionCombatMove(FAIActionData* InAction)
{
	if (InAction->ActionType == EAIAction::HardCover || InAction->ActionType == EAIAction::Hide || InAction->ActionType == EAIAction::HideExit || InAction->ActionType == EAIAction::Duel || InAction->ActionType == EAIAction::Flee || InAction->ActionType == EAIAction::Rush || InAction->ActionType == EAIAction::Flank || InAction->ActionType == EAIAction::Suppress || InAction->ActionType == EAIAction::Push || InAction->ActionType == EAIAction::Reposition)
	{
		return true;
	}

	return false;
}