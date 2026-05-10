// Copyright Void Interactive, 2021

#include "TakeCoverActivity.h"

#include "Characters/CyberneticCharacter.h"
#include "Characters/CyberneticController.h"

#include "Actors/Items/MeleeWeapon.h"

#include "BaseCombatActivity.h"

#include "Animation/Notifies/AnimNotify_SetCoverFirePose.h"
#include "Animation/Notifies/AnimNotify_SetCoverIdlePose.h"

#include "CoverSystem.h"
#include "NavigationSystem.h"
#include "ReadyOrNotAIConfig.h"
#include "Actors/Door.h"

#include "Info/ReadyOrNotSignificanceManager.h"

TAutoConsoleVariable<int32> CVarCoverActivityDebug(TEXT("CoverActivity.DrawDebug"), 0, TEXT("0 = Dont draw debug info. 1 = Draw debug info"));
TAutoConsoleVariable<int32> CVarCoverActivityForceCoverFireType(TEXT("CoverActivity.ForceFireType"), 0, TEXT("0 = Dont force fire type. 1 = Force blind fire. 1 = Force exposed fire"));
TAutoConsoleVariable<int32> CVarCoverActivityForceExitCoverFire(TEXT("CoverActivity.ForceExitCoverFire"), 0, TEXT("0 = Dont force cover fire exit. 1 = Force cover fire exit"));
TAutoConsoleVariable<int32> CVarCoverActivityNoWait(TEXT("CoverActivity.NoWait"), 0, TEXT("0 = Allow waiting in cover. 1 = Don't allow waiting in cover"));

static float GApproachExitRange = 400.0f;

FVector FCoverInstigatorStimulus::GetLocation() const
{
	if (::IsValid(InstigatorCharacter))
		return InstigatorCharacter->GetActorLocation();

	if (bUseThreatTransformAsInstigatorTransform)
		return ThreatTransform.GetLocation();

	return FVector::ZeroVector;
}

bool FCoverInstigatorStimulus::IsValid() const
{
	if (bUseThreatTransformAsInstigatorTransform)
		return ThreatTransform.GetLocation() != FVector::ZeroVector;
	
	return ::IsValid(InstigatorCharacter);
}

UTakeCoverActivity::UTakeCoverActivity()
{
	ActivityName = FText::FromStringTable("SwatCommandTable", "TakeCover");
	bIsProgressActivity = false;
	bResetStateMachineWhenActivityResumed = false;
	bAbortIfTrackingEnemy = false;
	bAbortMoveWhenActivityOverriden = true;

	ActivityStateMachine->AddState("Move To Cover")
						.BindEventEnter(MAKE_DELEGATE_BINDING(this, &UTakeCoverActivity::EnterMoveToCoverState))
						.BindEventTick(MAKE_DELEGATE_BINDING(this, &UTakeCoverActivity::TickMoveToCoverState))
						.CreateTransition("Cover", MAKE_DELEGATE_BINDING(this, &UTakeCoverActivity::CanCover));

	ActivityStateMachine->AddState("Cover")
						.BindEventEnter(MAKE_DELEGATE_BINDING(this, &UTakeCoverActivity::EnterCoverState))
						.BindEventTick(MAKE_DELEGATE_BINDING(this, &UTakeCoverActivity::TickCoverState))
						.BindEventExit(MAKE_DELEGATE_BINDING(this, &UTakeCoverActivity::ExitCoverState))
						.CreateTransition("Cover Fire", MAKE_DELEGATE_BINDING(this, &UTakeCoverActivity::CanFireFromCover), 0)
						.CreateTransition("Complete", MAKE_DELEGATE_BINDING(this, &UTakeCoverActivity::CanCompleteCover), 1);
	
	ActivityStateMachine->AddState("Cover Fire")
						.BindEventEnter(MAKE_DELEGATE_BINDING(this, &UTakeCoverActivity::EnterCoverFireState))
						.BindEventExit(MAKE_DELEGATE_BINDING(this, &UTakeCoverActivity::ExitCoverFireState))
						.BindEventTick(MAKE_DELEGATE_BINDING(this, &UTakeCoverActivity::TickCoverFireState))
						.CreateTransition("Cover", MAKE_DELEGATE_BINDING(this, &UTakeCoverActivity::CanStopCoverFire))
						.CreateTransition("CompleteAbrupt", MAKE_DELEGATE_BINDING(this, &UTakeCoverActivity::CanAbruptCompleteCover));

	ActivityStateMachine->AddState("Complete")
						.BindEventEnter(MAKE_DELEGATE_BINDING(this, &UTakeCoverActivity::EnterCompleteState));
	
	ActivityStateMachine->AddState("CompleteAbrupt")
						.BindEventEnter(MAKE_DELEGATE_BINDING(this, &UTakeCoverActivity::EnterCompleteAbruptState));
}

void UTakeCoverActivity::StartActivity(AAIController* Owner)
{
	Super::StartActivity(Owner);

	UReadyOrNotSignificanceManager::UnregisterActorWithSignificanceManager(GetCharacter());

	LOG_CLASS_FUNC

	GetCharacter()->OnCharacterTakeDamage.RemoveAll(this);
	GetCharacter()->OnCharacterTakeDamage.AddDynamic(this, &UTakeCoverActivity::OnTakeDamage);
	GetCharacter()->OnStunnedEvent.RemoveAll(this);
	GetCharacter()->OnStunnedEvent.AddDynamic(this, &UTakeCoverActivity::OnStunned);

	GetCharacter()->ReasonsToSprint.AddUnique("taking cover");

	COVER_SYSTEM->OccupyCover(CoverData.CoverLocation);

	for (TActorIterator<APlayerCharacter> It(GetWorld()); It; ++It)
	{
		APlayerCharacter* PlayerCharacter = *It;
		PlayerCharacter->OnWeaponFire.RemoveAll(this);
		PlayerCharacter->OnWeaponFire.AddDynamic(this, &UTakeCoverActivity::OnEnemyWeaponFire);
	}

	UReadyOrNotFunctionLibrary::StartTimerForCallback(ForgetLocationTimer, this, &UTakeCoverActivity::RemoveLocationFromHistory, 7.0f, true);
	
	if (!IsCoverValid(CoverData))
	{
		ACTIVITY_FAILED("CoverData is not valid");
		return;
	}
}

void UTakeCoverActivity::PerformActivity(const float DeltaTime)
{
	Super::PerformActivity(DeltaTime);

	TimeSinceLastShotAt += DeltaTime;

	if (!InstigatorStimulus.IsValid())
	{
		ACTIVITY_FAILED("No valid instigator");
		return;
	}

	if (OwningController->GetTrackedTarget())
	{
		InstigatorStimulus.InstigatorCharacter = OwningController->GetTrackedTarget();
		InstigatorStimulus.bUseThreatTransformAsInstigatorTransform = false;
	}

	if (!IsCoverValid(CoverData))
	{
		#if !UE_BUILD_SHIPPING
		ULog::Warning("CoverData is not valid. Aborting cover...");
		#endif
		
		AbortCoverNow(EAbortCoverReason::Forced);
		return;
	}

	if (GetCharacter()->IsPlayingDead())
	{
		ACTIVITY_FAILED("Playing dead", true);
		return;
	}

	if (!IsMovingToCover())
	{
		if ((bShouldCrouch && !HasUpCover(CurrentCrouchCoverType)) || !bShouldCrouch)
		{
			// If the last sensed enemy position is not on the same side as our current cover fire direction,
			// abort cover now, not a good cover spot anymore
			if (LastSensedEnemyPosition != FVector::ZeroVector)
			{
				const FVector DirectionToInstigator = (LastSensedEnemyPosition - CoverData.CoverLocation).GetSafeNormal2D();

				const FCoverDirection& CoverDirection = (bShouldCrouch ? CoverData.CrouchCoverDirection : CoverData.StandCoverDirection);
				const float CoverFireDotProduct = FVector::DotProduct(DirectionToInstigator, GetCharacter()->ActiveCoverDirection == ECoverDirection::Left ? CoverDirection.Left : CoverDirection.Right);

				if (CoverFireDotProduct < -0.25f)
				{
					AbortCoverNow(EAbortCoverReason::EnemySensed);
					return;
				}
			}
		}

		if (IsInstigatorMovingTowardsCover())
		{
			AbortCoverNow(EAbortCoverReason::EnemyMovingTowardsUs);
			return;
		}

		// is instigator behind me?
		if (InstigatorStimulus.InstigatorCharacter && GetCharacter()->IsSuspect())
		{
			const FVector DirectionToLastSense = (OwningController->GetTargetingComp()->GetLastKnownEnemyPosition() - CoverData.CoverLocation).GetSafeNormal2D();
			const FVector DirectionToInstigator = (InstigatorStimulus.InstigatorCharacter->GetActorLocation() - CoverData.CoverLocation).GetSafeNormal();
			const float SenseDot = FVector::DotProduct(DirectionToInstigator, DirectionToLastSense);
			const float CoverDot = FVector::DotProduct(DirectionToInstigator, -CoverData.CoverNormal);
			//LOG_NUMBER(SenseDot);
			//LOG_NUMBER(CoverDot);

			if (SenseDot < 0.6f || CoverDot < 0.5f)
			{
				AbortCoverNow(EAbortCoverReason::EnemyBehindUs);
				return;
			}
		}
	}
}

#if !UE_BUILD_SHIPPING
void UTakeCoverActivity::PerformActivity_Debug(const float DeltaTime)
{
	Super::PerformActivity_Debug(DeltaTime);

	if (CVarCoverActivityDebug.GetValueOnAnyThread() > 0)
	{
		/*if (const APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0))
		{
			const FVector DirectionToCoverPoint = (CoverData.CoverLocation - CameraManager->GetCameraLocation()).GetSafeNormal();
			const float DotProduct = FVector::DotProduct(CameraManager->GetCameraRotation().Vector(), DirectionToCoverPoint);
			// Not being looked at, don't draw debug elements
			if (DotProduct < 0.985f)
				return;
		}*/

		DrawDebugSphere(GetWorld(), CoverData.CoverLocation, 20.0f, 4, FColor::Yellow);

		// Draw a beam of light
		DrawDebugLine(GetWorld(), CoverData.CoverLocation + FVector(0.0f, 0.0f, 10.0f), CoverData.CoverLocation + FVector::UpVector * 10000.0f, FColor::Cyan, false, -1.0f, 0, 10.0f);

		//FString DebugStringAI;
		//GatherDebugString(DebugStringAI);
		
		//DrawDebugString(GetWorld(), GetCharacter()->GetActorLocation(), DebugStringAI, nullptr, FColor::White, DeltaTime + 0.005f, true);

		DrawDebugLine(GetWorld(), CoverData.CoverLocation, CoverData.CoverLocation + CoverData.CoverNormal * 100.0f, FColor::Purple, false, -1.0f, 0, 1.5f);
		
		DrawDebugLine(GetWorld(), CoverData.CoverLocation, CoverData.CoverLocation + FVector::CrossProduct(CoverData.CoverNormal, FVector::UpVector) * 100.0f, FColor::Orange, false, -1.0f, 0, 1.5f);

		/*for (int32 i = 0; i < InstigatorLocationHistory.Num(); i++)
		{
			DrawDebugString(GetWorld(), InstigatorLocationHistory[i], "InstigatorLocationHistory[" + FString::FromInt(i) + "]: " + InstigatorLocationHistory[i].ToString(), nullptr, FColor::White, DeltaTime + 0.05f, true);
			
			DrawDebugBox(GetWorld(), InstigatorLocationHistory[i], FVector(15.0f), FColor::Cyan, false, DeltaTime + 0.05f, 0, 2.0f);
			
			if (InstigatorLocationHistory.IsValidIndex(i+1))
				DrawDebugLine(GetWorld(), InstigatorLocationHistory[i], InstigatorLocationHistory[i+1], FColor::Yellow, false, DeltaTime + 0.05f, 0, 2.0f);
		}
		*/

		if (IsMovingToCover())
		{
			DrawDebugCircle(GetWorld(), CoverData.CoverLocation, 200.0f, 32, FColor::White, false, DeltaTime + 0.05f, 0, 1.0f, FVector::RightVector, FVector::ForwardVector);

			if (EntryLocation != FVector::ZeroVector)
			{
				DrawDebugCircle(GetWorld(), EntryLocation, 10.0f, 32, FColor::Red, false, DeltaTime + 0.05f, 0, 1.0f, FVector::RightVector, FVector::ForwardVector);

				// Navmesh projected entry location
				{
					FVector ProjectedLocation = EntryLocation;
					if (const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
					{
						FNavLocation NewLocationProjected(ProjectedLocation);
						if (NavSys->ProjectPointToNavigation(ProjectedLocation, NewLocationProjected))
						{
							ProjectedLocation = NewLocationProjected.Location;
					
							// Failsafe
							const float ZHeightDifference = FMath::Abs(ProjectedLocation.Z - NewLocationProjected.Location.Z);
							if (ZHeightDifference > 50.0f)
								ProjectedLocation = CoverData.CoverLocation;
						}
					}

					DrawDebugCircle(GetWorld(), ProjectedLocation, 5.0f, 32, FColor::Yellow, false, DeltaTime + 0.05f, 0, 1.0f, FVector::RightVector, FVector::ForwardVector);
				}

				const FVector& EntryLocationOffset = EntryLocation + CoverData.CoverNormal * 50.0f;

				DrawDebugCircle(GetWorld(), EntryLocationOffset, 25.0f, 32, FColor::Yellow, false, DeltaTime + 0.05f, 0, 1.0f, FVector::RightVector, FVector::ForwardVector);
			}
		}
	}
}

void UTakeCoverActivity::GatherDebugString(FString& OutString)
{
	//Super::GatherDebugString(OutString);
	
	OutString = "\n\t" + GetName();
	OutString += AddDebugString("Location", Location.ToCompactString());
	
	if (Location != FVector::ZeroVector)
	{
		OutString += AddDebugString("Distance To Destination", FString::SanitizeFloat(FVector::Distance(GetCharacter()->GetNavAgentLocation(), Location)));
		OutString += AddDebugString("Has Reached Destination", HasReachedLocation(GetDestinationTolerance()) ? "true" : "false");
	}
	
	if (!InstigatorStimulus.IsValid())
		return;
	
	FVector CoverLocation = CoverData.CoverLocation;
	CoverLocation.Z = InstigatorStimulus.GetLocation().Z;
	const FVector DirectionToInstigator = (InstigatorStimulus.GetLocation() - CoverLocation).GetSafeNormal2D();

	const FCoverDirection& CoverDirection = (bShouldCrouch ? CoverData.CrouchCoverDirection : CoverData.StandCoverDirection);
	const float CoverFireDotProduct = FVector::DotProduct(DirectionToInstigator, GetCharacter()->ActiveCoverDirection == ECoverDirection::Left ? CoverDirection.Left : CoverDirection.Right);

	const bool bCanCalculateCoverFireMismatch = (bShouldCrouch && !HasUpCover(CurrentCrouchCoverType)) || !bShouldCrouch;
	
	OutString += LINE_TERMINATOR;
	OutString += AddDebugString("Last Abort Cover Reason", RON_ENUM_TO_STRING(EAbortCoverReason, LastAbortCoverReason));
	
	OutString += LINE_TERMINATOR;
	OutString += AddDebugString("State", GetActiveStateName() + " (ID: " + FString::FromInt(GetActiveStateID()) + ")");
	OutString += AddDebugString("Stance", (bShouldCrouch ? "Crouch" : "Stand"));
	OutString += AddDebugString("Is Waiting", (IsWaiting() ? "True" : "False"));
	OutString += AddDebugString("Chosen Aim Type", (bChosenCoverAimType ? "True" : "False"));

	OutString += LINE_TERMINATOR;
	OutString += AddDebugString("Active Cover Fire Type", RON_ENUM_TO_STRING(ECoverFireType, GetCharacter()->ActiveCoverFireType));
	OutString += AddDebugString("Current Cover Fire Type", RON_ENUM_TO_STRING(ECoverFireType, CurrentCoverFireType));
	OutString += AddDebugString("Active Cover Direction", RON_ENUM_TO_STRING(ECoverDirection, GetCharacter()->ActiveCoverDirection));
	OutString += AddDebugString("Desired Cover Direction", RON_ENUM_TO_STRING(ECoverDirection, DetermineDesiredCoverDirection()));
	OutString += AddDebugString("Current Crouch Cover Type", CrouchCoverTypeToString(CurrentCrouchCoverType));
	OutString += AddDebugString("Current Stand Cover Type", StandCoverTypeToString(CurrentStandCoverType));
	
	OutString += LINE_TERMINATOR;
	OutString += AddDebugString("Cover Fire Time", FString::SanitizeFloat(CoverFireTime));
	//OutString += AddDebugString("Last Heard Noise", FText::AsNumber(TimeSinceHeardNoiseInCover).ToString());

	OutString += LINE_TERMINATOR;
	OutString += AddDebugString("Exit Cover Fire Now", (bShouldExitCoverFireNow ? "True" : "False"));
	OutString += AddDebugString("Complete Cover Now", (bShouldCompleteCoverNow ? "True" : "False"));
	OutString += AddDebugString("CanStopCoverFire", (CanStopCoverFire() ? "True" : "False"));
	
	if (bCanCalculateCoverFireMismatch)
	{
		OutString += LINE_TERMINATOR;
		OutString += AddDebugString("Cover Fire Mismatch", FString::Printf(TEXT(": %.2f"), CoverFireDotProduct));
	}
}
#endif

void UTakeCoverActivity::ResumeActivity()
{
	Super::ResumeActivity();

	bChosenCoverAimType = false;

	GetCharacter()->bIsCrouching = bShouldCrouch;
}

void UTakeCoverActivity::ActivityOverriden(UBaseActivity* OverridingActivity)
{
	Super::ActivityOverriden(OverridingActivity);

	bChosenCoverAimType = false;
}

void UTakeCoverActivity::FinishedActivity_NoOwner(const bool bSuccess)
{
	Super::FinishedActivity_NoOwner(bSuccess);

	OwningController->bStopDecisionMaking = false;
	OwningController->GetCombatActivity()->TimeSpentWithWeaponUp = 0.0f;
	
	OwningController->bCanTrackMoveVectorWhileStrafe = false;

	COVER_SYSTEM->ReleaseCover(CoverData.CoverLocation);

	ResetCoverData();
}

void UTakeCoverActivity::FinishedActivity(const bool bSuccess)
{
	Super::FinishedActivity(bSuccess);

	LOG_CLASS_FUNC

	OwningController->bStopDecisionMaking = false;
	OwningController->bCanTrackMoveVectorWhileStrafe = false;

	COVER_SYSTEM->ReleaseCover(CoverData.CoverLocation);

	for (TActorIterator<APlayerCharacter> It(GetWorld()); It; ++It)
	{
		APlayerCharacter* PlayerCharacter = *It;
		PlayerCharacter->OnWeaponFire.RemoveAll(this);
	}
	
	if (GetCharacter()->GetEquippedWeapon())
	{
		GetCharacter()->GetEquippedWeapon()->bInfiniteAmmo = false;
	}

	OwningController->GetCombatActivity()->StopScriptedFire();
	
	GetCharacter()->Rep_CoverAnimState.bIsInCover = false;
	GetCharacter()->Rep_CoverAnimState.bIsFiring = false;
	GetCharacter()->Rep_CoverAnimState.bIsReturningToIdle = false;
	
	GetCharacter()->AccuracyNerfPercentage = 1.0f;
	GetCharacter()->bIsCrouching = false;

	GetCharacter()->OnCharacterTakeDamage.RemoveAll(this);
	GetCharacter()->OnStunnedEvent.RemoveAll(this);
	
	GetCharacter()->ReasonsToSprint.Remove("taking cover");

	// Ensure that no entry/exit anims play when stopping this activity
	StopEntryAnims();
	StopExitAnims();
	
	ResetCoverData();

	UReadyOrNotSignificanceManager::RegisterActorWithSignificanceManager(GetCharacter());
}

bool UTakeCoverActivity::CanFinishActivity() const
{
	return false; // Force finished only
}

bool UTakeCoverActivity::OverrideFocalPoint(FVector& FocalPoint)
{
	const float ZOffset = bShouldCrouch ? 0.0f : 50.0f;
	const FVector RaisedCoverPoint = FVector(CoverData.CoverLocation.X, CoverData.CoverLocation.Y, GetCharacter()->GetActorLocation().Z + ZOffset);
	//const float DistanceToCoverPoint = FVector::Distance(GetCharacter()->GetActorLocation(), RaisedCoverPoint);

	const ABaseMagazineWeapon* EquippedWeapon = GetCharacter()->GetEquippedItem<ABaseMagazineWeapon>();

	const bool bHoldingFirearm = EquippedWeapon != nullptr;

	const bool bCantFire = (EquippedWeapon && !EquippedWeapon->HasAmmo());

	if (IsMovingToCover())
	{
		if (HasReachedEntryLocation(50.0f))
		{
			FocalPoint = RaisedCoverPoint;
			return true;
		}
		
		if (HasReachedEntryLocation(300.0f))
		{
			FVector TraceStart = GetCharacter()->GetMesh()->GetSocketLocation("head_end");
			FVector TraceEnd = RaisedCoverPoint;
			
			// Does we have LOS to the cover point?
			bool bHasLOS = !GetWorld()->LineTraceTestByChannel(TraceStart, TraceEnd, ECC_Visibility, GetCharacter()->GetCollisionQueryParameters());

			FocalPoint = bHasLOS ? RaisedCoverPoint : FVector::ZeroVector;

			if (!bHoldingFirearm || !bHasLOS)
			{
				return false;
			}
			
			return true;
		}

		if (!EquippedWeapon)
			return false;
		
		if (bCantFire)
		{
			FocalPoint = RaisedCoverPoint;
			return true;
		}

		if (OwningController->GetTrackedTarget())
		{
			FocalPoint = OwningController->GetTrackedTarget()->GetActorLocation();
		}
		
		return true;
	}

	if (IsCoverFiring())
	{
		FocalPoint = CoverFireFocalPoint;
		return true;
	}

	// Wait for first sideswitch after entry
	if (!bHasEverSideSwitched)
	{
		FocalPoint = FVector::ZeroVector;
		return true;
	}

	FocalPoint = FVector::ZeroVector;
	return true;
}

bool UTakeCoverActivity::OverrideFireAngleThreshold(float& Threshold) const
{
	if (IsCoverFiring() && CurrentCoverFireType == ECoverFireType::Blind)
	{
		Threshold = 0.75f;
		return true;
	}

	return false;
}

bool UTakeCoverActivity::ShouldForceStrafe() const
{
	return true;
}

bool UTakeCoverActivity::ShouldForceNoStrafe() const
{
	return false;
}

bool UTakeCoverActivity::CanShoot() const
{
	return IsMovingToCover() || IsCoverFiring() || IsExitingCover();
}

bool UTakeCoverActivity::CanReload() const
{
	if (IsMovingToCover())
	{
		//if (PathDistanceToCover > 1500.0f)
		//	return true;
		
		return false;
	}
	
	if (IsInCover() && !IsCoverFiring() && GetElapsedTimeInCover() > 0.5f)
		return true;

	if (GetCharacter()->ActiveCoverDirection != CurrentCoverDirection)
		return false;

	if (!bHasEverSideSwitched)
		return false;

	return false;
}

float UTakeCoverActivity::GetDestinationTolerance() const
{
	if (IsMovingToCover())
		return 10.0f;

	return 50.0f;
}

bool UTakeCoverActivity::GetLowReadyOverride(bool& bLowReady) const
{
	bLowReady = false;
	return true;
}

bool UTakeCoverActivity::ShouldDisableMoveRequest() const
{
	return IsInCover();
}

bool UTakeCoverActivity::IsMovingToCover() const
{
	return GetActiveStateID() == 0;
}

bool UTakeCoverActivity::IsInCover() const
{
	return GetActiveStateID() > 0 && GetActiveStateID() < 3;
}

bool UTakeCoverActivity::IsCoverFiring() const
{
	return GetActiveStateID() == 2;
}

bool UTakeCoverActivity::IsExitingCover() const
{
	return GetActiveStateID() == 3;
}

bool UTakeCoverActivity::IsPlayingCoverEnterAnims() const
{
	return GetCharacter()->IsTableMontagePlaying("tp_cover_enter_left_0") ||
			GetCharacter()->IsTableMontagePlaying("tp_cover_enter_left_90") ||
			GetCharacter()->IsTableMontagePlaying("tp_cover_enter_right_0") ||
			GetCharacter()->IsTableMontagePlaying("tp_cover_enter_right_90") ||
			GetCharacter()->IsTableMontagePlaying("tp_cover_aim_enter_left") ||
			GetCharacter()->IsTableMontagePlaying("tp_cover_aim_enter_right") ||
			GetCharacter()->IsTableMontagePlaying("tp_cover_aim_enter_left_up") ||
			GetCharacter()->IsTableMontagePlaying("tp_cover_aim_enter_right_up") ||
			GetCharacter()->IsTableMontagePlaying("tp_cover_blind_enter_left") ||
			GetCharacter()->IsTableMontagePlaying("tp_cover_blind_enter_right") ||
			GetCharacter()->IsTableMontagePlaying("tp_cover_blind_enter_left_up") ||
			GetCharacter()->IsTableMontagePlaying("tp_cover_blind_enter_right_up");
}

bool UTakeCoverActivity::IsPlayingCoverExitAnims() const
{
	return GetCharacter()->IsTableMontagePlaying("tp_cover_exit_left_0") ||
			GetCharacter()->IsTableMontagePlaying("tp_cover_exit_left_90") ||
			GetCharacter()->IsTableMontagePlaying("tp_cover_exit_right_0") ||
			GetCharacter()->IsTableMontagePlaying("tp_cover_exit_right_90") ||
			GetCharacter()->IsTableMontagePlaying("tp_cover_aim_exit_left") ||
			GetCharacter()->IsTableMontagePlaying("tp_cover_aim_exit_right") ||
			GetCharacter()->IsTableMontagePlaying("tp_cover_aim_exit_left_up") ||
			GetCharacter()->IsTableMontagePlaying("tp_cover_aim_exit_right_up") ||
			GetCharacter()->IsTableMontagePlaying("tp_cover_blind_exit_left") ||
			GetCharacter()->IsTableMontagePlaying("tp_cover_blind_exit_right") ||
			GetCharacter()->IsTableMontagePlaying("tp_cover_blind_exit_left_up") ||
			GetCharacter()->IsTableMontagePlaying("tp_cover_blind_exit_right_up");
}

void UTakeCoverActivity::AbortCoverNow(const EAbortCoverReason& InReason)
{
	#if !UE_BUILD_SHIPPING
	ULog::Info(GetNameSafe(GetCharacter()) + " | AbortCoverNow | Reason: " + RON_ENUM_TO_STRING(EAbortCoverReason, InReason));
	#endif
	
	LastAbortCoverReason = InReason;
	
	UReadyOrNotFunctionLibrary::StopCallbackTimer(this, ForgetLocationTimer);

	bShouldTryCoverFireNow = false;
	bShouldExitCoverFireNow = true;
	bShouldCompleteCoverNow = true;
}

FName UTakeCoverActivity::GetMoveStyleOverride_Implementation() const
{
	if (GetActiveStateID() == 0) // move to state
	{
		if (HasReachedEntryLocation(300.0f))
			return NAME_None;
		
		const bool bCantFire = GetCharacter()->GetEquippedWeapon() && !GetCharacter()->GetEquippedWeapon()->HasAmmo();
		const bool bShouldLower = !OwningController->GetTrackedTarget() || bCantFire;
		if (bShouldLower)
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
		}
	}

	return NAME_None;
}

void UTakeCoverActivity::OnPathFound(uint32 PathId, ENavigationQueryResult::Type ResultType, FNavPathSharedPtr NavPath)
{
	Super::OnPathFound(PathId, ResultType, NavPath);

	if (NavPath.IsValid())
	{
		PathDistanceToCover = NavPath->GetLength();
	}
}

void UTakeCoverActivity::OnEnemyWeaponFire(AReadyOrNotCharacter* Character, ABaseMagazineWeapon* Weapon, FVector FireDirection)
{
	if (!OwningController || !GetCharacter() || !Character)
		return;

	if (!IsCoverFiring())
		return;

	if (bShouldExitCoverFireNow)
		return;

	if (Character->GetTeam() == GetCharacter()->GetTeam())
		return;
	
	const FVector DirectionToEnemy = (GetCharacter()->GetActorLocation() - Character->GetActorLocation()).GetSafeNormal2D();
	const float ForwardDotProduct = FVector::DotProduct(DirectionToEnemy, FireDirection);

	// Is the enemy firing in the general direction to us?
	if (ForwardDotProduct > 0.85f)
	{
		FVector Direction = CoverData.CoverNormal;
		float TraceDistance = 0.0f;

		if (GetCharacter()->ActiveCoverAimType != ECoverAimType::Up)
		{
			TraceDistance = COVER_SYSTEM->CoverGenSettings.LeftRightEdgeExtent + 10.0f; // extra padding
			
			if (GetCharacter()->IsCrouching())
				Direction = GetCharacter()->ActiveCoverDirection == ECoverDirection::Left ? CoverData.CrouchCoverDirection.Left : CoverData.CrouchCoverDirection.Right;
			else
				Direction = GetCharacter()->ActiveCoverDirection == ECoverDirection::Left ? CoverData.StandCoverDirection.Left : CoverData.StandCoverDirection.Right;
		}
		
		float HeightOffset = (bShouldCrouch ? COVER_SYSTEM->CoverGenSettings.MaxCrouchCoverHeight : COVER_SYSTEM->CoverGenSettings.MaxStandCoverHeight/2);
		FVector Offset = FVector::UpVector * (HeightOffset + COVER_SYSTEM->CoverGenSettings.VertexZOffset);

		FVector StartTrace = CoverData.CoverLocation + Offset + (Direction * TraceDistance);
		
		FHitResult Hit;
		const bool bHasLOS = !GetWorld()->LineTraceSingleByChannel(Hit, StartTrace, Character->GetMesh()->GetSocketLocation("head"), ECC_Visibility, UReadyOrNotFunctionLibrary::CreateCollisionQueryParams(GetCharacter(), Character));

		if (!bHasLOS)
			return;

		TimeSinceHeardNoiseInCover = 0.0f;

		TimeSinceLastShotAt = 0.0f;

		//FiredAtCount++;

		if (GetCharacter()->ActiveCoverFireType == ECoverFireType::Exposed)
		{
			const float DistanceToInstigator = FVector::Distance(GetCharacter()->GetActorLocation(), Character->GetActorLocation());

			if (DistanceToInstigator < GApproachExitRange)
			{
				AbortCoverNow(EAbortCoverReason::EnemyFiredNearUs);
				return;
			}
		}
		
		bShouldExitCoverFireNow = FMath::FRand() < 0.25f; // 25% chance of exiting

		#if !UE_BUILD_SHIPPING
		if (CVarCoverActivityForceExitCoverFire.GetValueOnAnyThread() > 0)
			bShouldExitCoverFireNow = true;
		#endif
	}
}

void UTakeCoverActivity::OnStunned(AReadyOrNotCharacter* StunnedCharacter, float Duration, EStunType StunType, AActor* DamageCauser)
{
	OwningController->FinishActivity(this, false, true);
}

void UTakeCoverActivity::OnTakeDamage(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* DamagedCharacter, AActor* DamageCauser, float Damage, float HealthRemaining)
{
	// Ignore friendly fire 
	if (InstigatorCharacter && !InstigatorCharacter->IsOnSWATTeam())
		return;
	
	if (IsInCover())
	{
		if (CurrentCoverFireType == ECoverFireType::Exposed) // exit if exposed only
		{
			AbortCoverNow();
		}
	}

	if (GetCharacter())
	{
		if (!IsPlayingCoverEnterAnims() && !IsPlayingCoverExitAnims())
		{
			bool bAllowHitReactions = true;
			if (GetCharacter()->Archetype)
				bAllowHitReactions = !GetCharacter()->Archetype->bIgnoreDamageHitReactions;

			if (bAllowHitReactions)
			{
				bShouldExitCoverFireNow = true;
				bShouldTryCoverFireNow = false;
				CurrentCoverFireType = ECoverFireType::None;
				UpdateCoverFireType();
				
				GetCharacter()->PlayMontageFromTable("tp_hits");
			}
		}
	}
}

void UTakeCoverActivity::EnterMoveToCoverState()
{
	SetLocation(CoverData.CoverLocation, true);

	GetCharacter()->PlayBarkOrStartConversation(VO_SUSPECTS_AND_CIVILIAN::FLEEING, true);
}

void UTakeCoverActivity::TickMoveToCoverState(const float DeltaTime, float Uptime)
{
	CoverFireTime = 0.0f;
	bNoMoveActivity = false;

	if (AnySWATHasLOSToCoverPoint())
	{
		ACTIVITY_FAILED("SWAT had LOS to cover point", true);
		return;
	}

	if (HasReachedCoverLocation(300.0f))
	{
		GetCharacter()->ReasonsToSprint.Remove("taking cover");
		
		if (!bChosenEntryPoint)
		{
			const FVector& BestEntryLocation = InitializeCoverAndCalculateEntry();

			#if WITH_EDITOR
			// Something went wrong calculating the entry location
			ensureAlways(BestEntryLocation != FVector::ZeroVector);
			#endif

			EntryLocation = BestEntryLocation;

			if (const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
			{
				FNavLocation NewLocationProjected(BestEntryLocation);
				if (NavSys->ProjectPointToNavigation(BestEntryLocation, NewLocationProjected))
				{
					EntryLocation = NewLocationProjected.Location;
					
					// Failsafe
					const float ZHeightDifference = FMath::Abs(BestEntryLocation.Z - NewLocationProjected.Location.Z);
					if (ZHeightDifference > 50.0f)
						EntryLocation = CoverData.CoverLocation;
				}
			}

			Location = EntryLocation;
			RequestMoveAsync();

			bChosenEntryPoint = true;
		}
		else
		{
			FHitResult HitResult;
			FCollisionQueryParams CollisionQueryParams = GetCharacter()->GetCollisionQueryParameters();
			FCollisionObjectQueryParams CollisionObjectQueryParams;
			CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
			CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_DOOR);
			CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_DOORWAY);
			
			bool bEntryPointHit = GetWorld()->LineTraceSingleByObjectType(HitResult, CoverData.CoverLocation, FVector(EntryLocation.X, EntryLocation.Y, CoverData.CoverLocation.Z), CollisionObjectQueryParams, CollisionQueryParams);

			// Something is in the way..
			if (bEntryPointHit)
			{
				// Recalculate the entry point
				bChosenEntryPoint = false;
				return;
			}
			
			EntryDotProductDecreaseRate += 0.001f;

			if (HasReachedEntryLocation(50.0f))
			{
				// Magnetize towards the entry location, slowly
				{
					const FVector Current = GetCharacter()->GetActorLocation();
					const FVector Target = FVector(EntryLocation.X, EntryLocation.Y, Current.Z);

					GetCharacter()->SetActorLocation(FMath::VInterpConstantTo(Current, Target, DeltaTime, 30.0f), false, nullptr, ETeleportType::TeleportPhysics);
				}
			}
			else
			{
				if (const FAIRequestID* MoveRequestPtr = MoveRequestID.Find(FIntVector(EntryLocation)))
				{
					if (!OwningController->IsMovingForRequest(MoveRequestPtr->GetID()))
					{
						SetLocation(EntryLocation, true);
					}
				}
				else
				//if (!OwningController->GetCurrentMoveRequestID().IsValid())
				{
					SetLocation(EntryLocation, true);
				}
			}
		}
	}
	else
	{
		if (!OwningController->GetCurrentMoveRequestID().IsValid())
		{
			SetLocation(CoverData.CoverLocation, true);
		}
	}
}

void UTakeCoverActivity::EnterCoverState()
{
	LOG_CLASS_FUNC
	
	CoverFireTime = 0.0f;
	
	GetCharacter()->bIsCrouching = bShouldCrouch;

	if (!InstigatorStimulus.IsValid())
		return;

	if (OwningController->IsCivilian())
		OwningController->bStopDecisionMaking = true;
	
	if (!bHasEverEnteredCover)
	{
		AbortMove();
		
		#if WITH_EDITOR
		// If not chosen, something went wrong
		ensureAlways(bChosenEntryPoint);
		//ensureAlways(HasReachedLocation());
		ensureAlways(!EntryAnimation.IsEmpty());
		#endif

		if (AnySWATHasLOSToCoverPoint())
		{
			OwningController->FinishActivity(this, true, true);
			return;
		}
		
		// If enemy is too close, abort activity before we try to enter cover
		if (FVector::Distance(InstigatorStimulus.GetLocation(), GetCharacter()->GetActorLocation()) < 700.0f)
		{
			FHitResult HitResult;
			
			const FCollisionQueryParams CollisionQueryParams = UReadyOrNotFunctionLibrary::CreateCollisionQueryParams(GetCharacter(), InstigatorStimulus.InstigatorCharacter);
			
			FVector StartTrace = InstigatorStimulus.GetLocation();
			if (IsValid(InstigatorStimulus.InstigatorCharacter))
				StartTrace = InstigatorStimulus.InstigatorCharacter->GetMesh()->GetSocketLocation("head_end");
				
			if (!GetWorld()->LineTraceSingleByChannel(HitResult, StartTrace, GetCharacter()->GetMesh()->GetSocketLocation("head_end"), ECC_Visibility, CollisionQueryParams))
			{
				OwningController->FinishActivity(this, true, true);
				return;
			}
		}
	
		OnEnteredCover.Broadcast();
		
		bHasEverEnteredCover = true;
		
		const FVector& DirectionToUs = (GetCharacter()->GetActorLocation() - CoverData.CoverLocation).GetSafeNormal2D();
		const float RightDotProduct = FVector::DotProduct(DirectionToUs, FVector::CrossProduct(CoverData.CoverNormal, FVector::UpVector));
		const bool bIsRightSideOfCoverPoint = RightDotProduct > 0.0f;

		const FVector EntryPoint1 = CoverData.CoverLocation + (-CoverData.CoverNormal).RotateAngleAxis(-15, FVector::UpVector) * 300.0f;
		const FVector EntryPoint2 = CoverData.CoverLocation + (-CoverData.CoverNormal).RotateAngleAxis(15, FVector::UpVector) * 300.0f;

		FVector FocalPoint = bIsRightSideOfCoverPoint ? EntryPoint1 : EntryPoint2;

		//FRotator Rotation = UKismetMathLibrary::FindLookAtRotation(GetCharacter()->GetActorLocation(), CoverData.CoverLocation);
		//GetCharacter()->SetActorRotation(FRotator(0.0f, Rotation.Yaw, 0.0f));
		//if (UAnimMontage* Montage = GetCharacter()->PlayMontageFromTableWithIndex(EntryAnimation, EnterAnimIndex))
		
		GetCharacter()->StopTPMontageFromTable(GetCharacter()->GetLastTableMontagePlayed());
		GetCharacter()->PlayedTableMontageMap3P.Empty();
		
		UAnimMontage* Montage = GetCharacter()->PlayMontageFromTableWithIndexWithFocalPoint(EntryAnimation, EnterAnimIndex, FocalPoint);
		
		#if WITH_EDITOR
		// something went wrong, this should never happen
		ensureAlways(Montage != nullptr);
		//todo: tp_flinch is breaking this
		#endif
		
		if (Montage)
		{
			EntryMontage = Montage;
			
			// Don't immediately set ActiveCoverFireType, wait until montage is almost finished
			// Fixes little snap in animation when ActiveCoverFireType is immediately set
			const float Delay = Montage->GetPlayLength() - (Montage->GetPlayLength()/2.0f);

			#if WITH_EDITOR
			// If Delay is negative, animation is not setup correctly. Must fix immediately
			ensure(Delay >= 0.0f);

			// Ensure UAnimNotify_SetCoverIdlePose are present in the enter anims
			for (const FAnimNotifyEvent& NotifyEvent : Montage->Notifies)
			{
				ensureAlways(Cast<UAnimNotify_SetCoverIdlePose>(NotifyEvent.Notify) != nullptr);
			}
			#endif

			if (Delay > 0.0f)
			{
				UReadyOrNotFunctionLibrary::StartTimerForCallback(this, &UTakeCoverActivity::OnInitialEnterAnimFinished, Delay);
			}
			else
			{
				OnInitialEnterAnimFinished();
			}
		}

		// Try to find a door, if there is one nearby
		if (!Door)
		{
			FHitResult HitResult;
			FCollisionQueryParams CollisionQueryParams = GetCharacter()->GetCollisionQueryParameters();
			FCollisionObjectQueryParams CollisionObjectQueryParams;
			CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
			CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_DOOR);
			CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_DOORWAY);
			
			constexpr float TraceDistance = 150.0f;

			FVector Direction = CoverData.CoverRail.Direction;
			FVector CoverLocation_Offset = FVector(CoverData.CoverLocation.X, CoverData.CoverLocation.Y, GetCharacter()->GetActorLocation().Z);
			FVector StartTrace = CoverLocation_Offset + (Direction * TraceDistance);

			GetWorld()->LineTraceSingleByObjectType(HitResult, StartTrace, StartTrace + -CoverData.CoverNormal * TraceDistance, CollisionObjectQueryParams, CollisionQueryParams);

			//DrawDebugLine(GetWorld(), StartTrace, StartTrace + -CoverData.CoverNormal * TraceDistance, FColor::Cyan, false, 5.0f, 0, 1.5f);

			Door = Cast<ADoor>(HitResult.GetActor());
			if (!Door)
			{
				Direction = -CoverData.CoverRail.Direction;
				StartTrace = CoverLocation_Offset + (Direction * TraceDistance);

				//DrawDebugLine(GetWorld(), StartTrace, StartTrace + -CoverData.CoverNormal * TraceDistance, FColor::Cyan, false, 5.0f, 0, 1.5f);

				GetWorld()->LineTraceSingleByObjectType(HitResult, StartTrace, StartTrace + -CoverData.CoverNormal * TraceDistance, CollisionObjectQueryParams, CollisionQueryParams);

				Door = Cast<ADoor>(HitResult.GetActor());
			}
		}

		// Always open the door away from us if we found a door near this cover point
		if (Door)
		{
			if (GetCharacter()->IsCivilian())
			{
				Door->CloseDoor(nullptr);
			}
			else
			{
				if (Door->GetSubDoor())
				{
					float Angle = Door->IsPointInFrontOfDoorway(CoverData.CoverLocation) ? Door->GetMaxOpenAmount() : -Door->GetMaxOpenAmount();
					Door->OpenDoor_SpecificAngle(nullptr, Angle, false, false);
					Door->GetSubDoor()->OpenDoor_SpecificAngle(nullptr, -Angle);
				}
				else
				{
					float Angle = Door->IsPointInFrontOfDoorway(CoverData.CoverLocation) ? Door->GetMaxOpenAmount() : -Door->GetMaxOpenAmount();
					Door->OpenDoor_SpecificAngle(nullptr, Angle, false, false);
				}
			}
		}
	}
}

void UTakeCoverActivity::TickCoverState(const float DeltaTime, const float Uptime)
{
	// Stop trying to move back to cover
	Location = FVector::ZeroVector;

	AbortMove();

	ElapsedTimeInCover = Uptime;

	TimeSinceHeardNoiseInCover += DeltaTime;

	bShouldExitCoverFireNow = false;
	
	#if !UE_BUILD_SHIPPING
	if (CVarCoverActivityNoWait.GetValueOnAnyThread() > 0)
	{
		ResetCurrentCoverTypeToOriginal();
	}
	#endif

	if (GetCharacter()->IsSuspect())
	{
		if (ABaseMagazineWeapon* Weapon = GetCharacter()->GetEquippedWeapon())
		{
			if (!Weapon->HasAnyAmmo())
			{
				bShouldCompleteCoverNow = true;
			}
		}
	}

	if (!InstigatorStimulus.IsValid())
		return;

	if (bShouldCompleteCoverNow)
	{
		bChosenCoverAimType = false;
		CurrentCoverFireType = ECoverFireType::None;
		return;
	}

	if (IsSideSwitching())
		return;

	const bool bIsStunned = GetCharacter()->IsStunned() || GetCharacter()->IsPlayingHitReaction() || GetCharacter()->IsPlayingStunAnimation();
	
	if ((!IsPlayingCoverExitAnims() || bIsStunned) && CurrentCoverFireType == ECoverFireType::None)
	{
		// Force reset to idle (fixes being stuck in a fire pose when not cover firing)
		GetCharacter()->ActiveCoverFireType = ECoverFireType::None;
	}

	const bool bWaitingForAnimReset = IsPlayingCoverEnterAnims() || IsPlayingCoverExitAnims() || bIsStunned ||
									UReadyOrNotFunctionLibrary::IsCallbackTimerActive(this, ChangeCoverDirectionTypeTimer) ||
									UReadyOrNotFunctionLibrary::IsCallbackTimerActive(this, ChangeCoverFireTypeTimer);

	if (!bWaitingForAnimReset)
	{
		if (EnterCoverCount > 0 || ElapsedTimeInCover > 5.0f)
		{
			if (AnySWATHasLOSToUs())
			{
				AbortCoverNow(EAbortCoverReason::EnemySensed);
				return;
			}
		}

		if (AnySWATHasLOSToCoverPoint())
		{
			AbortCoverNow(EAbortCoverReason::EnemySensed);
			return;
		}

		// Magnetize towards the cover location, slowly
		{
			const FVector Current = GetCharacter()->GetActorLocation();
			const FVector Target = FVector(CoverData.CoverLocation.X, CoverData.CoverLocation.Y, Current.Z);

			GetCharacter()->SetActorLocation(FMath::VInterpConstantTo(Current, Target, DeltaTime, 20.0f), false, nullptr, ETeleportType::TeleportPhysics);

			const FRotator CurrentRot = GetCharacter()->GetActorRotation();

			FRotator TargetRot = CoverData.CoverNormal.Rotation() + (GetCharacter()->ActiveCoverDirection == ECoverDirection::Left ? FRotator(0.0f, -180.0f, 0.0f) : FRotator(0.0f, 180.0f, 0.0f));

			GetCharacter()->SetActorRotation(FMath::RInterpConstantTo(CurrentRot, TargetRot, DeltaTime, 50.0f), ETeleportType::TeleportPhysics);
		}
		
		if (ElapsedTimeInCover > CoverFireCooldown)
		{
			// Set the reload trigger when ammo hits these values, to pre-emptively reload before starting a cover fire
			// Just so that we're not on low ammo before we start to cover fire
			/*
			if (const ABaseMagazineWeapon* EquippedWeapon = GetCharacter()->GetEquippedItem<ABaseMagazineWeapon>())
			{
				switch (EquippedWeapon->ItemClass)
				{
					case EItemClass::IC_AssaultRifle:	ReloadAtAmmoCount = 5; break;
					case EItemClass::IC_SMG:			ReloadAtAmmoCount = 5; break;
					case EItemClass::IC_LMG:			ReloadAtAmmoCount = 5; break;
					case EItemClass::IC_Pistol:			ReloadAtAmmoCount = 3; break;
					case EItemClass::IC_Shotgun:		ReloadAtAmmoCount = 0; break;
					case EItemClass::IC_LessLethal:		ReloadAtAmmoCount = 2; break;
		
					default:							ReloadAtAmmoCount = 0; break;
				}

				if (EquippedWeapon->GetAmmo() < ReloadAtAmmoCount)
					return;
			}
			*/
			
			if (!bChosenCoverAimType)
			{
				//UReadyOrNotFunctionLibrary::StopCallbackTimer(this, ChangeCoverFireTypeTimer);

				const ABaseMagazineWeapon* EquippedWeapon = GetCharacter()->GetEquippedItem<ABaseMagazineWeapon>();
				const bool bHoldingFirearm = EquippedWeapon != nullptr;
				
				if (bShouldCrouch && bHoldingFirearm)
				{
					// If we have a coruch cover that's accompanied by a left or right cover,
					// should we choose the 'up' cover over the 'left/right'?
					// Always choose 'up' if left/right have no LOS to the enemy, otherwise can pick either type

					bool bUpCoverAccompaniedByLeftOrRight = CoverData.CrouchCoverType == ECrouchCoverType::LeftAndUp ||
															CoverData.CrouchCoverType == ECrouchCoverType::RightAndUp ||
															CoverData.CrouchCoverType == ECrouchCoverType::LeftRightAndUp;
					
					if (bUpCoverAccompaniedByLeftOrRight)
					{
						FHitResult HitResult;
						
						FCollisionQueryParams CollisionQueryParams = UReadyOrNotFunctionLibrary::CreateCollisionQueryParams(GetCharacter(), InstigatorStimulus.InstigatorCharacter);
						
						FVector Direction = GetCharacter()->ActiveCoverDirection == ECoverDirection::Left ? CoverData.CrouchCoverDirection.Left : CoverData.CrouchCoverDirection.Right;
						float DirectionOffset = COVER_SYSTEM->CoverGenSettings.LeftRightEdgeExtent;

						FVector StartTrace = CoverData.CoverLocation + (Direction * DirectionOffset) + FVector::UpVector * 50.0f;
			
						FVector EndTrace = InstigatorStimulus.GetLocation();
						if (IsValid(InstigatorStimulus.InstigatorCharacter))
							EndTrace = InstigatorStimulus.InstigatorCharacter->GetMesh()->GetSocketLocation("head_end");
						
						const bool bLOSToInstigator = !GetWorld()->LineTraceSingleByChannel(HitResult, StartTrace, EndTrace, ECC_Visibility, CollisionQueryParams);
						
						#if !UE_BUILD_SHIPPING
						if (CVarCoverActivityDebug.GetValueOnAnyThread() > 0)
							DrawDebugLine(GetWorld(), StartTrace, EndTrace, bLOSToInstigator ? FColor::Green : FColor::Red, false, 1.0f, 0, 1.5f);
						#endif


						if (bLOSToInstigator)
						{
							GetCharacter()->ActiveCoverAimType = FMath::RandBool() ? ECoverAimType::LeftOrRight : ECoverAimType::Up;
						}
						// Only allow up cover to be chosen, if no LOS to instigator 
						else
						{
							GetCharacter()->ActiveCoverAimType = ECoverAimType::Up;
						}
					}
					else
					{
						GetCharacter()->ActiveCoverAimType = CoverData.CrouchCoverType == ECrouchCoverType::UpOnly ? ECoverAimType::Up : ECoverAimType::LeftOrRight;
					}
				}
				else
				{
					GetCharacter()->ActiveCoverAimType = ECoverAimType::LeftOrRight;
				}

				if (ShouldBlindFire())
				{
					CurrentCoverFireType = ECoverFireType::Blind;
				}
				else
				{
					CurrentCoverFireType = ECoverFireType::Exposed;
				}

				#if !UE_BUILD_SHIPPING
				if (CVarCoverActivityForceCoverFireType.GetValueOnAnyThread() == 1)
				{
					CurrentCoverFireType = ECoverFireType::Blind;
				}
				else if (CVarCoverActivityForceCoverFireType.GetValueOnAnyThread() > 1)
				{
					CurrentCoverFireType = ECoverFireType::Exposed;
				}
				#endif

				PreviousCoverFireType = CurrentCoverFireType;
				bChosenCoverAimType = true;

				SwitchCoverDirection(DetermineDesiredCoverDirection(), true);
			}
		}

		ECoverDirection DesiredCoverDirection = GetCharacter()->ActiveCoverDirection;
		switch (GetCharacter()->ActiveCoverDirection)
		{
			case ECoverDirection::Left:
				if ((bShouldCrouch && CoverData.CrouchCoverType == ECrouchCoverType::RightOnly) ||
					(bShouldCrouch && CoverData.CrouchCoverType == ECrouchCoverType::RightAndUp) ||
					(!bShouldCrouch && CoverData.StandCoverType == EStandCoverType::RightOnly))
				{
					DesiredCoverDirection = ECoverDirection::Right;
				}
			break;
			case ECoverDirection::Right:
				if ((bShouldCrouch && CoverData.CrouchCoverType == ECrouchCoverType::LeftOnly) ||
					(bShouldCrouch && CoverData.CrouchCoverType == ECrouchCoverType::LeftAndUp) ||
					(!bShouldCrouch && CoverData.StandCoverType == EStandCoverType::LeftOnly))
				{
					DesiredCoverDirection = ECoverDirection::Left;
				}
			break;
			default: ;
		}
		
		if (GetCharacter()->ActiveCoverDirection != DesiredCoverDirection)
		{
			SwitchCoverDirection(DesiredCoverDirection, true);
		}
	}
}

void UTakeCoverActivity::ExitCoverState()
{
	LOG_CLASS_FUNC

	ElapsedTimeInCover = 0.0f;
	//FiredAtCount = 0;
}

void UTakeCoverActivity::EnterCoverFireState()
{
	LOG_CLASS_FUNC

	ElapsedTimeInCover = 0.0f;
	CoverFireTime = 0.0f;
	bEverHadLOSToEnemyInPreviousCoverFire = false;
	bChosenCoverAimType = false;
	bHasCoverFireLOS = false;
	bShouldExitCoverFireNow = false;
	bShouldTryCoverFireNow = false;
	
	// Always change it back to the original
	CurrentCrouchCoverType = OriginalCrouchCoverType;
	CurrentStandCoverType = OriginalStandCoverType;

	LastSensedEnemyPosition = OwningController->GetTargetingComp()->GetLastKnownEnemyPosition();
	
	OwningController->GetCombatActivity()->TimeSpentWithWeaponUp = 0.0f;
	
	// Stop trying to move back to cover
	Location = FVector::ZeroVector;

	AbortMove();

	if (!InstigatorStimulus.IsValid())
		return;

	if (bShouldCompleteCoverNow)
		return;

	#if WITH_EDITOR
	// Always make sure that when we enter this state, CurrentCoverFireType is something other than None
	// Must be set before we enter this state
	ensureAlways(CurrentCoverFireType != ECoverFireType::None);
	#endif
	
	FString Animation;

	if (CurrentCoverFireType == ECoverFireType::Blind)
	{
		if (GetCharacter()->ActiveCoverAimType == ECoverAimType::Up)
			Animation = GetCharacter()->ActiveCoverDirection == ECoverDirection::Left ? "tp_cover_blind_enter_left_up" : "tp_cover_blind_enter_right_up";
		else
			Animation = GetCharacter()->ActiveCoverDirection == ECoverDirection::Left ? "tp_cover_blind_enter_left" : "tp_cover_blind_enter_right";
	}
	else
	{
		if (GetCharacter()->ActiveCoverAimType == ECoverAimType::Up)
		{
			Animation = GetCharacter()->ActiveCoverDirection == ECoverDirection::Left ? "tp_cover_aim_enter_left_up" : "tp_cover_aim_enter_right_up";

			// TODO: dont stand if stand cover is wall and not crouch only cover
		}
		else
		{
			Animation = GetCharacter()->ActiveCoverDirection == ECoverDirection::Left ? "tp_cover_aim_enter_left" : "tp_cover_aim_enter_right";
		}
	}

	const int32 AnimCount = GetCharacter()->GetMontageAnimCountFromTable(Animation);
	EnterAimAnimIndex = FMath::RandRange(0, AnimCount-1);

	// Determine the focal point before we play the anim montage to ensure smooth rotations
	//FVector FocalPoint = OwningController->GetTargetingComp()->GetLastKnownEnemyPosition();

	FCollisionQueryParams CollisionQueryParams = UReadyOrNotFunctionLibrary::CreateCollisionQueryParams(GetCharacter(), InstigatorStimulus.InstigatorCharacter);
	
	FVector Direction = CoverData.CoverNormal;
	float TraceDistance = 0.0f;

	if (GetCharacter()->ActiveCoverAimType != ECoverAimType::Up)
	{
		TraceDistance = COVER_SYSTEM->CoverGenSettings.LeftRightEdgeExtent + 10.0f; // extra padding
		
		if (GetCharacter()->IsCrouching())
			Direction = GetCharacter()->ActiveCoverDirection == ECoverDirection::Left ? CoverData.CrouchCoverDirection.Left : CoverData.CrouchCoverDirection.Right;
		else
			Direction = GetCharacter()->ActiveCoverDirection == ECoverDirection::Left ? CoverData.StandCoverDirection.Left : CoverData.StandCoverDirection.Right;
	}

	float HeightOffset = (bShouldCrouch ? COVER_SYSTEM->CoverGenSettings.MaxCrouchCoverHeight : COVER_SYSTEM->CoverGenSettings.MaxStandCoverHeight/2);
	float AngleOffset = GetCharacter()->ActiveCoverDirection == ECoverDirection::Left ? -COVER_SYSTEM->CoverGenSettings.LeftRightEdgeAngle : COVER_SYSTEM->CoverGenSettings.LeftRightEdgeAngle;
	FVector Offset = FVector::UpVector * (HeightOffset + COVER_SYSTEM->CoverGenSettings.VertexZOffset);

	FVector StartTrace = CoverData.CoverLocation + Offset + (Direction * TraceDistance);
	
	FHitResult HitResult_LastKnown;
	
	FVector EndTrace = InstigatorStimulus.GetLocation();
	if (IsValid(InstigatorStimulus.InstigatorCharacter))
		EndTrace = InstigatorStimulus.InstigatorCharacter->GetMesh()->GetSocketLocation("head_end");
	
	const bool bLastKnownPositionHit = GetWorld()->LineTraceSingleByChannel(HitResult_LastKnown, StartTrace, EndTrace, ECC_WorldStatic, CollisionQueryParams);

	if (bLastKnownPositionHit)
	{
		CoverFireFocalPoint_Entry = StartTrace + (-CoverData.CoverNormal.RotateAngleAxis(AngleOffset, FVector::UpVector) * 500.0f);
	}
	else if (IsValid(Door))
	{
		CoverFireFocalPoint_Entry = Door->CalculateClosestPoint(GetCharacter()->GetActorLocation());
		
		if (Door->IsPointInFrontOfDoorway(CoverFireFocalPoint_Entry))
		{
			CoverFireFocalPoint_Entry += Door->GetActorForwardVector() * -150.0f;
		}
		else
		{
			CoverFireFocalPoint_Entry += Door->GetActorForwardVector() * 150.0f;
		}

		//CoverFireFocalPoint_Entry = OwningController->GetTargetingComp()->GetLastKnownEnemyPosition();
	}

	if (CurrentCoverFireType == ECoverFireType::Exposed)
	{
		if (!bShouldCrouch) // stand only
			StartTrace.Z = GetCharacter()->GetMesh()->GetSocketLocation("head_end").Z;

		FVector EndTrace2 = InstigatorStimulus.GetLocation();
		if (IsValid(InstigatorStimulus.InstigatorCharacter))
			EndTrace2 = InstigatorStimulus.InstigatorCharacter->GetMesh()->GetSocketLocation("head_end");
		
		const bool bHit = GetWorld()->LineTraceTestByChannel(StartTrace, EndTrace2, ECC_WorldStatic, CollisionQueryParams);
		
		#if !UE_BUILD_SHIPPING
		if (CVarCoverActivityDebug.GetValueOnAnyThread() > 0)
			DrawDebugLine(GetWorld(), StartTrace, EndTrace2, bHit ? FColor::Red : FColor::Green, false, 1.0f, 0, 1.5f);
		#endif

		if (!bHit)
		{
			CoverFireFocalPoint_Entry = EndTrace2;
		}
		else if (IsValid(Door))
		{
			if (Door && Door->IsPointInFrontOfDoorway(CoverFireFocalPoint_Entry))
			{
				CoverFireFocalPoint_Entry += Door->GetActorForwardVector() * -150.0f;
			}
			else
			{
				CoverFireFocalPoint_Entry += Door->GetActorForwardVector() * 150.0f;
			}
			
			//CoverFireFocalPoint_Entry = OwningController->GetTargetingComp()->GetLastKnownEnemyPosition();
		}
	}

	CoverFireFocalPoint = CoverFireFocalPoint_Entry;

	if (IsInstigatorMovingTowardsCover())
	{
		AbortCoverNow(EAbortCoverReason::EnemyMovingTowardsUs);
		return;
	}

	#if !UE_BUILD_SHIPPING
	if (CVarCoverActivityDebug.GetValueOnAnyThread() > 0)
		DrawDebugSphere(GetWorld(), CoverFireFocalPoint, 20.0f, 4, FColor::Yellow, false, 1.0f, 0, 1.5f);
	#endif

	if (CurrentCoverFireType == ECoverFireType::Blind)
	{
		if (GetCharacter()->GetEquippedWeapon())
		{
			GetCharacter()->GetEquippedWeapon()->bInfiniteAmmo = true;
		}
	}

	if (UAnimMontage* Montage = GetCharacter()->PlayMontageFromTableWithIndexWithFocalPoint(Animation, EnterAimAnimIndex, CoverFireFocalPoint))
	{
		#if WITH_EDITOR
		// Must have at least one notify (specifically AnimNotify_SetCoverFirePose) to ensure proper animations are set for the anim graph
		ensureAlways(Montage->Notifies.Num() > 0);

		// Ensure UAnimNotify_SetCoverFirePose are present in the enter anims
		for (const FAnimNotifyEvent& NotifyEvent : Montage->Notifies)
		{
			ensureAlways(Cast<UAnimNotify_SetCoverFirePose>(NotifyEvent.Notify) != nullptr);
		}
		#endif
		
		// Don't immediately set ActiveCoverFireType, wait until montage is almost finished
		// Fixes little snap in animation when ActiveCoverFireType is immediately set
		const float Delay = Montage->GetPlayLength() - (Montage->GetPlayLength()/2.0f);

		#if WITH_EDITOR
		// If Delay is negative, animation is not setup correctly. Must fix immediately
		ensure(Delay >= 0.0f);
		#endif

		if (Delay > 0.0f)
		{
			UReadyOrNotFunctionLibrary::StartTimerForCallback(ChangeCoverFireTypeTimer, this, &UTakeCoverActivity::UpdateCoverFireType, Delay);
		}
		else
		{
			UpdateCoverFireType();
		}

		if (FMath::RandBool())
		{
			GetCharacter()->PlayBarkOrStartConversation(VO_SUSPECTS_AND_CIVILIAN::REPLY_SHOOTING, true, 10.0f);
		}
	}
}

void UTakeCoverActivity::ExitCoverFireState()
{
	LOG_CLASS_FUNC
	
	if (GetCharacter()->GetEquippedWeapon())
	{
		GetCharacter()->GetEquippedWeapon()->bInfiniteAmmo = false;
	}

	ElapsedTimeInCover = 0.0f;
	bChosenCoverAimType = false;
	bHasCoverFireLOS = false;
	bShouldExitCoverFireNow = false;
	bShouldTryCoverFireNow = false;
	
	bPerpetualCoverFire = false;

	EnterCoverCount++;

	CoverFireCooldown = FMath::FRandRange(0.5f, 2.0f);

	OwningController->GetCombatActivity()->StopScriptedFire();
	
	OwningController->GetCombatActivity()->TimeSpentWithWeaponUp = 0.0f;

	if (CanAbruptCompleteCover())
	{
		return;
	}

	UpdateCoverFireType();
	
	FString Animation;
	switch (GetCharacter()->ActiveCoverFireType)
	{
		case ECoverFireType::None:
			Animation = GetCharacter()->ActiveCoverDirection == ECoverDirection::Left ? "tp_cover_exit_left" : "tp_cover_exit_right";
		break;

		case ECoverFireType::Blind:
		if (GetCharacter()->ActiveCoverAimType == ECoverAimType::Up)
			Animation = GetCharacter()->ActiveCoverDirection == ECoverDirection::Left ? "tp_cover_blind_exit_left_up" : "tp_cover_blind_exit_right_up";
		else
			Animation = GetCharacter()->ActiveCoverDirection == ECoverDirection::Left ? "tp_cover_blind_exit_left" : "tp_cover_blind_exit_right";
		break;

		case ECoverFireType::Exposed:
		if (GetCharacter()->ActiveCoverAimType == ECoverAimType::Up)
		{
			Animation = GetCharacter()->ActiveCoverDirection == ECoverDirection::Left ? "tp_cover_aim_exit_left_up" : "tp_cover_aim_exit_right_up";
		}
		else
		{
			Animation = GetCharacter()->ActiveCoverDirection == ECoverDirection::Left ? "tp_cover_aim_exit_left" : "tp_cover_aim_exit_right";
		}
		break;
	}

	if (UAnimMontage* Montage = GetCharacter()->PlayMontageFromTableWithIndexWithFocalPoint(Animation, EnterAimAnimIndex, CoverFireFocalPoint))
	{
		#if WITH_EDITOR
		// Ensure no UAnimNotify_SetCoverFirePose are present in the exit anims
		for (const FAnimNotifyEvent& NotifyEvent : Montage->Notifies)
		{
			ensureAlways(Cast<UAnimNotify_SetCoverFirePose>(NotifyEvent.Notify) == nullptr);
		}
		#endif
		
		LastExitMontagePlayed = Montage;
		
		CurrentCoverFireType = ECoverFireType::None;

		// Fixes little snap in animation when ActiveCoverFireType is immediately set
		{
			const float Delay = Montage->GetPlayLength() - (Montage->GetPlayLength()/2.0f);

			#if WITH_EDITOR
			// If Delay is negative, animation is not setup correctly. Must fix immediately
			ensure(Delay >= 0.0f);
			#endif

			//LOG_NUMBER(Delay);

			if (Delay > 0.0f)
			{
				UReadyOrNotFunctionLibrary::StartTimerForCallback(ChangeCoverFireTypeTimer, this, &UTakeCoverActivity::UpdateCoverFireType, Delay);
			}
			else
			{
				UpdateCoverFireType();
			}
		}
	}

	// We didn't see anyone during this cover fire attempt, so sit still and listen for the enemy
	if (!bEverHadLOSToEnemyInPreviousCoverFire)
	{
		CurrentCrouchCoverType = ECrouchCoverType::Wall;
		CurrentStandCoverType = EStandCoverType::Wall;

		bPerpetualCoverFire = true;

		if (EnterCoverCount > 2)
		{
			AbortCoverNow();
			return;
		}

		UReadyOrNotFunctionLibrary::StartTimerForCallback(this, &UTakeCoverActivity::ForceExposeFire, 5.0f);
	}
}

void UTakeCoverActivity::EnterCompleteState()
{
	FString Animation = GetCharacter()->ActiveCoverDirection == ECoverDirection::Left ? "tp_cover_exit_left" : "tp_cover_exit_right";

	const int32 Angle = EntryAngle == 90 ? 0 : 90;

	Animation += "_" + FString::FromInt(Angle);

	UAnimMontage* Montage = GetCharacter()->GetMontageFromTableWithIndex(Animation, EnterAnimIndex);
	
	#if WITH_EDITOR
	ensureAlways(Montage != nullptr);
	#endif
	
	if (!IsValid(Montage))
	{
		CompleteCover();
		return;
	}
	
	GetCharacter()->Play3PMontage(Montage);
	
	#if WITH_EDITOR
	// Ensure no UAnimNotify_SetCoverFirePose are present in the exit anims
	for (const FAnimNotifyEvent& NotifyEvent : Montage->Notifies)
	{
		ensureAlways(Cast<UAnimNotify_SetCoverFirePose>(NotifyEvent.Notify) != nullptr);
	}
	#endif
	
	LastExitMontagePlayed = Montage;

	const float Delay = Montage->GetPlayLength() - (Montage->GetPlayLength()/2.0f);

	#if WITH_EDITOR
	// If Delay is negative, animation is not setup correctly. Must fix immediately
	ensure(Delay >= 0.0f);
	#endif

	if (Delay > 0.0f)
	{
		UReadyOrNotFunctionLibrary::StartTimerForCallback(this, &UTakeCoverActivity::CompleteCover, Delay);
	}
	else
	{
		CompleteCover();
	}
}

void UTakeCoverActivity::EnterCompleteAbruptState()
{
	CompleteCover();
}

void UTakeCoverActivity::TickCoverFireState(const float DeltaTime, const float Uptime)
{
	// Stop trying to move back to cover
	Location = FVector::ZeroVector;

	AbortMove();

	if (!InstigatorStimulus.IsValid())
		return;
	
	if (IsSideSwitching())
	{
		CoverFireTime = 0.0f;
		return;
	}

	if (bShouldCompleteCoverNow)
		return;
		
	if (bShouldExitCoverFireNow)
		return;

	if (GetCharacter()->IsStunned() || GetCharacter()->IsPlayingStunAnimation() || GetCharacter()->IsPlayingHitReaction())
	{
		bShouldExitCoverFireNow = true;
		bShouldTryCoverFireNow = false;
		return;
	}

	if (GetCharacter()->bWantsReload && !GetCharacter()->IsAny3PMontageActive())
	{
		bShouldExitCoverFireNow = true;
		return;
	}
	
	if (AnySWATHasLOSToCoverPoint())
	{
		AbortCoverNow(EAbortCoverReason::EnemySensed);
		return;
	}
	
	CoverFireTime += DeltaTime;
	ElapsedTimeInCover = 0.0f;
	TimeSinceHeardNoiseInCover = 0.0f;

	ReloadAtAmmoCount = 0;

	bool bHasLOS;
	if (CurrentCoverFireType == ECoverFireType::Blind)
	{
		bHasLOS = true;
	}
	else
	{
		const FCollisionQueryParams CollisionQueryParams = UReadyOrNotFunctionLibrary::CreateCollisionQueryParams(GetCharacter(), InstigatorStimulus.InstigatorCharacter);
	
		FVector EndTrace = InstigatorStimulus.GetLocation();
		if (IsValid(InstigatorStimulus.InstigatorCharacter))
			EndTrace = InstigatorStimulus.InstigatorCharacter->GetMesh()->GetSocketLocation("head_end");
		
		bHasLOS = !GetWorld()->LineTraceTestByChannel(GetCharacter()->GetMesh()->GetSocketLocation("head_end") - FVector::UpVector * 10.0f, EndTrace - FVector::UpVector * 10.0f, ECC_Visibility, CollisionQueryParams);
		
		bHasCoverFireLOS = bHasLOS;

		if (bHasLOS)
		{
			CoverFireFocalPoint = InstigatorStimulus.GetLocation();
		}
		else if (IsValid(Door))
		{
			CoverFireFocalPoint = Door->CalculateClosestPoint(GetCharacter()->GetActorLocation());
			
			if (Door->IsPointInFrontOfDoorway(CoverFireFocalPoint))
			{
				CoverFireFocalPoint += Door->GetActorForwardVector() * -150.0f;
			}
			else
			{
				CoverFireFocalPoint += Door->GetActorForwardVector() * 150.0f;
			}
		}

		#if !UE_BUILD_SHIPPING
		//DrawDebugBox(GetWorld(), EnemyLocation, FVector(15.0f), FColor::Orange, false, 2.0f, 0, 2.0f);
		//DrawDebugLine(GetWorld(), GetCharacter()->GetMesh()->GetSocketLocation("head_end") - FVector::UpVector * 10.0f, InstigatorEnemy->GetMesh()->GetSocketLocation("head_end") - FVector::UpVector * 10.0f, bHasLOS ? FColor::Green : FColor::Red, false, DeltaTime + 0.1f, 0, 2.0f);
		#endif
	}

	if (bHasLOS)
	{
		bPerpetualCoverFire = false;
		bEverHadLOSToEnemyInPreviousCoverFire = true;
		//LastTrackedEnemyLocationInCover = InstigatorEnemy->GetActorLocation();
		
		//OwningController->GetCombatActivity()->TimeSpentOnTarget = 999.0f;
		
		if (CurrentCoverFireType == ECoverFireType::Blind)
		{
			GetCharacter()->AccuracyNerfPercentage = 5.0f;
			
			OwningController->GetCombatActivity()->CurrentScriptedFireAt.FireAtActor = nullptr;
			OwningController->GetCombatActivity()->CurrentScriptedFireAt.FireAtLocation = CoverFireFocalPoint;
			OwningController->GetCombatActivity()->CurrentScriptedFireAt.bOverrideTargetedEnemy = true;
			OwningController->GetCombatActivity()->CurrentScriptedFireAt.TimeRemaining = 2.0f;
			OwningController->GetCombatActivity()->CurrentScriptedFireAt.FireAngleThreshold = 0.75f;
			OwningController->GetCombatActivity()->TimeSpentWithWeaponUp = 999.0f;
			
			//OwningController->GetCombatActivity()->FireWeapon(nullptr);
		}
		else
		{
			GetCharacter()->AccuracyNerfPercentage = 1.0f;
			
			OwningController->GetCombatActivity()->StopScriptedFire();
			
			float DistanceToInstigator = FVector::Distance(GetCharacter()->GetActorLocation(), LastSensedEnemyPosition);
		
			// Too close, exit cover when done cover firing
			if (DistanceToInstigator < GApproachExitRange)
			{
				AbortCoverNow(EAbortCoverReason::SeenEnemyApproaching);
				return;
			}
			
			//OwningController->GetCombatActivity()->FireWeapon(InstigatorEnemy);
		}

		//OwningController->GetCombatActivity()->FireWeapon(InstigatorEnemy);
		//OwningController->GetCombatActivity()->RunEngagementLogic(InstigatorEnemy, DeltaTime);
	}
	else
	{
		if (!bEverHadLOSToEnemyInPreviousCoverFire)
		{
			CoverFireTime = 0.0f;
			bPerpetualCoverFire = true;
		}
	}
}

bool UTakeCoverActivity::CanCover() const
{
	if (!GetCharacter())
		return false;

	if (HasReachedEntryLocation(20.0f) && bChosenEntryPoint)
	{
		// Special case for firearms, must require looking at cover before entry
		// Required for root motion entry to work properly and not miss the target point
		//const ABaseMagazineWeapon* EquippedWeapon = GetCharacter()->GetEquippedItem<ABaseMagazineWeapon>();
		//const bool bHoldingFirearm = EquippedWeapon != nullptr;
		//if (bHoldingFirearm)
		{
			const float DotProduct = FVector::DotProduct((CoverData.CoverLocation - GetCharacter()->GetActorLocation()).GetSafeNormal2D(), GetCharacter()->GetActorForwardVector());
			const bool bIsFacingCover = DotProduct > (0.985f - EntryDotProductDecreaseRate);
			//LOG_NUMBER(DotProduct);
			
			return bIsFacingCover;
		}

		//return true;
	}
	
	return false;
}

bool UTakeCoverActivity::CanStopCoverFire() const
{
	if (GetCharacter()->IsStunned() || GetCharacter()->IsPlayingHitReaction() || GetCharacter()->IsPlayingStunAnimation())
		return true;
	
	if (bShouldCompleteCoverNow || bShouldExitCoverFireNow)
		return true;
	
	if (bPerpetualCoverFire)
	{
		if (CoverFireTime > AI_CONFIG_GET_FLOAT("PerpetualCoverFireTime", 60.0f))
			return true;
	}
	else
	{
		if (CoverFireTime > 2.0f)
			return true;
	}

	return false;
}

bool UTakeCoverActivity::CanFireFromCover() const
{
	if (!GetCharacter())
		return false;

	if (IsSideSwitching())
		return false;

	if (bShouldCompleteCoverNow)
		return false;

	// Don't try to fire if active cover direction is not the desired direction.
	// Most likely waiting for a sideswitch to happen
	if (GetCharacter()->ActiveCoverDirection != CurrentCoverDirection)
		return false;

	if (IsPlayingCoverEnterAnims())
		return false;

	if (OwningController->GetCombatActivity()->GetCombatEngagementType() != ECombatEngagementType::FireWeapon)
	{
		return false;
	}
	
	if (const ABaseMagazineWeapon* EquippedWeapon = GetCharacter()->GetEquippedItem<ABaseMagazineWeapon>())
	{
		if (!EquippedWeapon->HasAmmo())
		{
			return false;
		}
	}

	if (!bChosenCoverAimType)
		return false;
	
	if (!HasReachedCoverLocation(25.0f))
		return false;

	// If we're sitting and waiting, should we open fire now?
	if (IsWaiting())
	{
		return bShouldTryCoverFireNow;
	}
	
	return true;
}

bool UTakeCoverActivity::CanCompleteCover() const
{
	if (IsSideSwitching())
		return false;

	if (IsPlayingCoverExitAnims())
		return false;
	
	if (bShouldCompleteCoverNow)
		return true;
	
	// Exit cover when it is safe
	/*
	if (IsWaiting() && !GetCharacter()->IsCivilian())
	{
		if (TimeSinceHeardNoiseInCover > 60.0f)
		{
			return true;
		}
	}
	*/

	return false;
}

bool UTakeCoverActivity::CanAbruptCompleteCover() const
{
	if (IsSideSwitching())
		return false;

	if (bShouldCompleteCoverNow && GetActiveStateID() == 2 && GetCharacter()->ActiveCoverFireType == ECoverFireType::Exposed)
		return true;

	return false;
}

void UTakeCoverActivity::CompleteCover()
{
	#if !UE_BUILD_SHIPPING
	ULog::Success(GetName() + " | Cover complete " + GetNameSafe(GetCharacter()));
	#endif
	
	OwningController->FinishActivity(this, true, true);
}

bool UTakeCoverActivity::IsSideSwitching() const
{
	return GetCharacter()->IsTableMontagePlaying("tp_cover_sideswitch_to_right") ||
			GetCharacter()->IsTableMontagePlaying("tp_cover_sideswitch_to_left") ||
			UReadyOrNotFunctionLibrary::IsCallbackTimerActive(this, ChangeCoverDirectionTypeTimer);
}

bool UTakeCoverActivity::IsWaiting() const
{
	return IsInCover() && ((bShouldCrouch && CurrentCrouchCoverType == ECrouchCoverType::Wall) ||
						(!bShouldCrouch && CurrentStandCoverType == EStandCoverType::Wall));
}

void UTakeCoverActivity::OnInitialEnterAnimFinished()
{
	bInitialEntryComplete = true;
}

void UTakeCoverActivity::UpdateCoverFireType()
{
	if (GetCharacter())
		GetCharacter()->ActiveCoverFireType = CurrentCoverFireType;

	#if !UE_BUILD_SHIPPING
	ULog::Info("Updated cover fire type: " + RON_ENUM_TO_STRING(ECoverFireType, CurrentCoverFireType));
	#endif
}

void UTakeCoverActivity::UpdateCoverDirection()
{
	if (GetCharacter())
		GetCharacter()->ActiveCoverDirection = CurrentCoverDirection;

	bHasEverSideSwitched = true;

	ULog::Info("Updated cover direction: " + RON_ENUM_TO_STRING(ECoverDirection, CurrentCoverDirection));
}

void UTakeCoverActivity::OnAIPerceptionSense_Implementation(ACyberneticController* InSenseController, FAIStimulus Stimulus, AActor*& OutOverrideSensedActor)
{
	if (OutOverrideSensedActor && OutOverrideSensedActor == GetCharacter())
		return;

	const AReadyOrNotCharacter* SensedCharacter = Cast<AReadyOrNotCharacter>(OutOverrideSensedActor);
	if (SensedCharacter)
	{
		if (SensedCharacter->GetTeam() == GetCharacter()->GetTeam())
			return;

		if (GetCharacter()->IsSuspect() && SensedCharacter->IsCivilian())
			return;
		
		LastSensedEnemyPosition = SensedCharacter->GetActorLocation();

		if (IsMovingToCover())
			return;

		InstigatorLocationHistory.AddUnique(LastSensedEnemyPosition);

		if (GetCharacter()->IsCivilian())
		{
			AbortCoverNow();
		}
	}
}

void UTakeCoverActivity::OnAIHearingSense_Implementation(ACyberneticController* InSenseController, FAIStimulus Stimulus, AActor*& OutOverrideSensedActor)
{
	if (OutOverrideSensedActor && OutOverrideSensedActor == GetCharacter())
		return;

	const AReadyOrNotCharacter* SensedCharacter = Cast<AReadyOrNotCharacter>(OutOverrideSensedActor);
	if (SensedCharacter)
	{
		if (SensedCharacter->GetTeam() == GetCharacter()->GetTeam())
			return;
		
		if (GetCharacter()->IsSuspect() && SensedCharacter->IsCivilian())
			return;
			
		LastSensedEnemyPosition = SensedCharacter->GetActorLocation();

		TimeSinceHeardNoiseInCover = 0.0f;

		const bool bWaitingForAnimReset = IsPlayingCoverEnterAnims() || IsPlayingCoverExitAnims() ||
										UReadyOrNotFunctionLibrary::IsCallbackTimerActive(this, ChangeCoverDirectionTypeTimer) ||
										UReadyOrNotFunctionLibrary::IsCallbackTimerActive(this, ChangeCoverFireTypeTimer);

		// Only care about hearing if we're sitting and waiting
		if (!IsSideSwitching() && IsInCover() && !IsCoverFiring() && ElapsedTimeInCover > 0.5f && !bWaitingForAnimReset)
		{
			const float DistanceToHeardActor = FVector::Distance(OutOverrideSensedActor->GetActorLocation(), GetCharacter()->GetActorLocation());

			#if !UE_BUILD_SHIPPING
			ULog::Info(GetCharacter()->GetName() + " heard " + OutOverrideSensedActor->GetName() + " in cover | Dist: " + FString::SanitizeFloat(DistanceToHeardActor/100.0f) + "m");
			#endif

			SideSwitch(OutOverrideSensedActor->GetActorLocation(), true);

			// After sideswitching, try exposed firing
			if (GetCharacter()->GetEquippedItem<ABaseMagazineWeapon>())
			{
				bShouldTryCoverFireNow = true;
				CurrentCoverFireType = ECoverFireType::Exposed;
			}
		}
	}
}

bool UTakeCoverActivity::SideSwitch(const FVector LastSensedLocation, const bool bPlayAnim)
{
	#if !UE_BUILD_SHIPPING
	ULog::Info(GetCharacter()->GetName() + " attempting side switch...");
	#endif

	if (IsCoverFiring())
		return false;
	
	if (!IsInCover())
		return false;
	
	FVector CoverLocation = CoverData.CoverLocation;
	CoverLocation.Z = LastSensedLocation.Z;
	const FVector DirectionToInstigator = (LastSensedLocation - CoverLocation).GetSafeNormal2D();

	const float RightDotProduct = FVector::DotProduct(DirectionToInstigator, FVector::CrossProduct(CoverData.CoverNormal, FVector::UpVector));
	
	const bool bIsEnemyRight = RightDotProduct > 0.0f;
	const bool bIsEnemyLeft = RightDotProduct < 0.0f;

	if (bIsEnemyLeft && GetCharacter()->ActiveCoverDirection == ECoverDirection::Right)
	{
		SwitchCoverDirection(ECoverDirection::Left, bPlayAnim);
		return true;
	}
	
	if (bIsEnemyRight && GetCharacter()->ActiveCoverDirection == ECoverDirection::Left)
	{
		SwitchCoverDirection(ECoverDirection::Right, bPlayAnim);
		return true;
	}

	return false;
}

void UTakeCoverActivity::ToggleCoverDirection(const bool bPlayAnim)
{
	if (GetCharacter()->ActiveCoverDirection == ECoverDirection::Left)
	{
		SwitchCoverDirection(ECoverDirection::Right, bPlayAnim);
	}
	else
	{
		SwitchCoverDirection(ECoverDirection::Left, bPlayAnim);
	}
}

void UTakeCoverActivity::SwitchCoverDirection(const ECoverDirection& NewCoverDirection, const bool bPlayAnim)
{
	// Already on this cover direction, dont bother
	if (GetCharacter()->ActiveCoverDirection == NewCoverDirection)
		return;
	
	if ((bShouldCrouch && CoverData.CrouchCoverType == ECrouchCoverType::LeftOnly) || CoverData.CrouchCoverType == ECrouchCoverType::RightOnly ||
		(!bShouldCrouch && CoverData.StandCoverType == EStandCoverType::LeftOnly) || CoverData.StandCoverType == EStandCoverType::RightOnly)
		return;
	
	if (IsSideSwitching())
		return;

	if (bHasEverSideSwitched)
		return;

	//if (IsCoverDirectionBlocked(NewCoverDirection))
	//	return;
		
	CurrentCoverDirection = NewCoverDirection;

	#if !UE_BUILD_SHIPPING
	ULog::Info("Side switching to " + RON_ENUM_TO_STRING(ECoverDirection, NewCoverDirection));
	#endif
	
	if (bPlayAnim)
	{
		FString Animation = GetCharacter()->ActiveCoverDirection == ECoverDirection::Left ? "tp_cover_sideswitch_to_right" : "tp_cover_sideswitch_to_left";

		if (UAnimMontage* Montage = GetCharacter()->PlayMontageFromTable(Animation))
		{
			// Do not set ActiveCoverDirection immediately
			// Fixes little snap in animation when ActiveCoverDirection is immediately set
			{
				const float Delay = Montage->GetPlayLength() - (Montage->GetPlayLength()/2.0f);

				#if WITH_EDITOR
				// If Delay is negative, animation is not setup correctly. Must fix immediately
				ensure(Delay >= 0.0f);
				#endif

				if (Delay > 0.0f)
				{
					UReadyOrNotFunctionLibrary::StartTimerForCallback(ChangeCoverDirectionTypeTimer, this, &UTakeCoverActivity::UpdateCoverDirection, Delay);
				}
				else
				{
					UpdateCoverDirection();
				}
			}
		}
	}
	else
	{
		UpdateCoverDirection();
	}
}

bool UTakeCoverActivity::AnySWATHasLOSToCoverPoint() const
{
	for (TActorIterator<AReadyOrNotCharacter> It(GetWorld()); It; ++It)
	{
		AReadyOrNotCharacter* Character = *It;

		if (Character->IsOnSWATTeam())
		{
			if (FVector::Distance(Character->GetActorLocation(), GetCharacter()->GetActorLocation()) < 2000.0f)
			{
				const FVector DirectionToCover = (CoverData.CoverLocation - Character->GetActorLocation()).GetSafeNormal2D();
				const float DotProduct = FVector::DotProduct(Character->GetActorForwardVector(), DirectionToCover);

				// Looking in the general direction of this cover point?
				if (DotProduct > 0.7f)
				{
					FHitResult HitResult;
					const FCollisionQueryParams CollisionQueryParams = UReadyOrNotFunctionLibrary::CreateCollisionQueryParams(GetCharacter(), Character);
			
					FVector TraceStart = Character->GetMesh()->GetSocketLocation("head_end");
					FVector TraceEnd = CoverData.CoverLocation;
					
					// Does the instigator have LOS to us?
					bool bHasLOS = !GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, CollisionQueryParams);

					if (bHasLOS)
					{
						return true;
					}
				}
			}
		}
	}

	return false;
}

bool UTakeCoverActivity::AnySWATHasLOSToUs() const
{
	for (TActorIterator<AReadyOrNotCharacter> It(GetWorld()); It; ++It)
	{
		AReadyOrNotCharacter* Character = *It;

		if (Character->IsOnSWATTeam())
		{
			if (FVector::Distance(Character->GetActorLocation(), GetCharacter()->GetActorLocation()) < 1000.0f)
			{
				const FVector DirectionToCover = (CoverData.CoverLocation - Character->GetActorLocation()).GetSafeNormal2D();
				const float DotProduct = FVector::DotProduct(Character->GetActorForwardVector(), DirectionToCover);

				// Looking in the general direction of this cover point?
				if (DotProduct > 0.95f)
				{
					FHitResult HitResult;
					const FCollisionQueryParams CollisionQueryParams = UReadyOrNotFunctionLibrary::CreateCollisionQueryParams(GetCharacter(), Character);
			
					FVector TraceStart = Character->GetMesh()->GetSocketLocation("head_end");
					//FVector TraceEnd = bShouldCrouch ? GetCharacter()->GetActorLocation() - FVector::UpVector * 25.0f : GetCharacter()->GetMesh()->GetSocketLocation("head");
					FVector TraceEnd = GetCharacter()->GetActorLocation();
					
					// Does the instigator have LOS to us?
					bool bHasLOS = !GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, CollisionQueryParams);

					if (bHasLOS)
					{
						return true;
					}
				}
			}
		}
	}

	return false;
}

bool UTakeCoverActivity::HasReachedCoverLocation(const float Tolerance) const
{
	if (!OwningController || !GetCharacter() || CoverData.CoverLocation == FVector::ZeroVector)
		return false;

	const float ZHeightDifference = FMath::Abs(CoverData.CoverLocation.Z - GetCharacter()->GetNavAgentLocation().Z);
	if (ZHeightDifference > 100.0f)
		return false;

	const float Dist = FVector::Distance(CoverData.CoverLocation, FVector(GetCharacter()->GetNavAgentLocation().X, GetCharacter()->GetNavAgentLocation().Y, CoverData.CoverLocation.Z));
	//LOG_NUMBER(Dist);
	return Dist < Tolerance;
}

bool UTakeCoverActivity::HasReachedEntryLocation(const float Tolerance) const
{
	if (!OwningController || !GetCharacter() || EntryLocation == FVector::ZeroVector)
		return false;

	const float ZHeightDifference = FMath::Abs(EntryLocation.Z - GetCharacter()->GetNavAgentLocation().Z);
	//LOG_NUMBER(ZHeightDifference);
	if (ZHeightDifference > 100.0f)
		return false;

	const float Dist = FVector::Distance(EntryLocation, FVector(GetCharacter()->GetNavAgentLocation().X, GetCharacter()->GetNavAgentLocation().Y, EntryLocation.Z));
	//LOG_NUMBER(Dist);
	return Dist < Tolerance;
}

bool UTakeCoverActivity::IsActiveCoverDirectionBlocked() const
{
	return IsCoverDirectionBlocked(GetCharacter()->ActiveCoverDirection);
}

bool UTakeCoverActivity::IsCoverDirectionBlocked(const ECoverDirection& InCoverDirection) const
{
	const ABaseMagazineWeapon* EquippedWeapon = GetCharacter()->GetEquippedItem<ABaseMagazineWeapon>();
	const bool bHoldingFirearm = EquippedWeapon != nullptr;

	if (!bHoldingFirearm)
		return false;
	
	FHitResult HitResult;
	
	FCollisionQueryParams CollisionQueryParams = UReadyOrNotFunctionLibrary::CreateCollisionQueryParams(GetCharacter(), InstigatorStimulus.InstigatorCharacter);
	
	FVector Direction = CoverData.CoverNormal;
	float TraceDistance = 0.0f;

	if (GetCharacter()->ActiveCoverAimType != ECoverAimType::Up)
	{
		TraceDistance = COVER_SYSTEM->CoverGenSettings.LeftRightEdgeExtent + 10.0f; // extra padding
		
		if (GetCharacter()->IsCrouching())
			Direction = InCoverDirection == ECoverDirection::Left ? CoverData.CrouchCoverDirection.Left : CoverData.CrouchCoverDirection.Right;
		else
			Direction = InCoverDirection == ECoverDirection::Left ? CoverData.StandCoverDirection.Left : CoverData.StandCoverDirection.Right;
	}

	float HeightOffset = (bShouldCrouch ? COVER_SYSTEM->CoverGenSettings.MaxCrouchCoverHeight : COVER_SYSTEM->CoverGenSettings.MaxStandCoverHeight/2);
	FVector Offset = FVector::UpVector * (HeightOffset + COVER_SYSTEM->CoverGenSettings.VertexZOffset);

	FVector StartTrace = CoverData.CoverLocation + Offset + (Direction * TraceDistance);

	//DrawDebugLine(GetWorld(), StartTrace, StartTrace + -CoverData.CoverNormal * 100.0f, FColor::Orange, false, 0.033f, 0, 1.5f);
	
	return GetWorld()->LineTraceSingleByChannel(HitResult, StartTrace, StartTrace + -CoverData.CoverNormal * 100.0f, ECC_WorldStatic, CollisionQueryParams);
}

void UTakeCoverActivity::StopEntryAnims()
{
	const FString EntryAnims[14] = {"tp_cover_enter_left_0",
								"tp_cover_enter_left_90",
								"tp_cover_enter_right_0",
								"tp_cover_enter_right_90",
								"tp_cover_aim_enter_left",
								"tp_cover_aim_enter_right",
								"tp_cover_aim_enter_left_up",
								"tp_cover_aim_enter_right_up",
								"tp_cover_blind_enter_left",
								"tp_cover_blind_enter_right",
								"tp_cover_blind_enter_left_up",
								"tp_cover_blind_enter_right_up"};

	for (uint8 i = 0; i < 14; i++)
		GetCharacter()->StopTPMontageFromTable(EntryAnims[i], 0.25f);

	if (LastExitMontagePlayed)
	{
		GetCharacter()->Multicast_Stop3PMontage(LastExitMontagePlayed, 0.25f);
		GetCharacter()->PlayedTableMontageMap3P.Empty();
	}
}

void UTakeCoverActivity::StopExitAnims()
{
	const FString ExitAnims[14] = {"tp_cover_exit_left_0",
								"tp_cover_exit_left_90",
								"tp_cover_exit_right_0",
								"tp_cover_exit_right_90",
								"tp_cover_aim_exit_left",
								"tp_cover_aim_exit_right",
								"tp_cover_aim_exit_left_up",
								"tp_cover_aim_exit_right_up",
								"tp_cover_blind_exit_left",
								"tp_cover_blind_exit_right",
								"tp_cover_blind_exit_left_up",
								"tp_cover_blind_exit_right_up"};

	for (uint8 i = 0; i < 14; i++)
		GetCharacter()->StopTPMontageFromTable(ExitAnims[i], 0.25f);

	if (LastExitMontagePlayed)
	{
		GetCharacter()->Multicast_Stop3PMontage(LastExitMontagePlayed, 0.25f);
		GetCharacter()->PlayedTableMontageMap3P.Empty();
	}
}

void UTakeCoverActivity::RemoveLocationFromHistory()
{
	if (InstigatorLocationHistory.Num() > 0)
		InstigatorLocationHistory.RemoveAt(0);
}

void UTakeCoverActivity::ResetCurrentCoverTypeToOriginal()
{
	CurrentCrouchCoverType = OriginalCrouchCoverType;
	CurrentStandCoverType = OriginalStandCoverType;
}

void UTakeCoverActivity::ForceExposeFire()
{
	bShouldTryCoverFireNow = true;
	CurrentCoverFireType = ECoverFireType::Exposed;
}

bool UTakeCoverActivity::IsInstigatorMovingTowardsCover() const
{
	if (InstigatorLocationHistory.Num() > 1)
	{
		const FVector& First = InstigatorLocationHistory[0];
		const FVector& Last = InstigatorLocationHistory.Last();
		
		const FVector& MovementDirection = (Last - First).GetSafeNormal2D();
		const FVector& DirectionToInstigator = (Last - GetCharacter()->GetActorLocation()).GetSafeNormal2D();

		const float Distance = FVector::Distance(GetCharacter()->GetActorLocation(), Last);
		if (Distance < GApproachExitRange)
		{
			const float DotProduct = FVector::DotProduct(MovementDirection, DirectionToInstigator);
			//LOG_NUMBER(DotProduct);
			
			// Moving towards us, abort cover
			if (DotProduct < 0.1f)
			{
				return true;
			}
		}
	}

	return false;
}

bool UTakeCoverActivity::ShouldBlindFire() const
{
	if (EnterCoverCount < 1)
		return false;
	
	if (PreviousCoverFireType == ECoverFireType::Exposed && bEverHadLOSToEnemyInPreviousCoverFire)
		return true;
	
	if (TimeSinceLastShotAt < 2.0f)
		return true;

	return false;
}

FVector UTakeCoverActivity::InitializeCoverAndCalculateEntry()
{
	// Initialize activity data
	// calculate 0 or 90 deg
	// get the 0 or 90 deg entry anim
	// get the travel distance from the anim
	// calculate entry point
	// determine cover direction
	// return

	FVector FinalEntryPoint = FVector::ZeroVector;

	OriginalCrouchCoverType = CoverData.CrouchCoverType;
	OriginalStandCoverType = CoverData.StandCoverType;

	CurrentCrouchCoverType = OriginalCrouchCoverType;
	CurrentStandCoverType = OriginalStandCoverType;

	EntryDotProductDecreaseRate = 0.0f;
	
	const bool bHoldingFirearm = GetCharacter()->GetEquippedItem<ABaseMagazineWeapon>() != nullptr;

	if (!bHoldingFirearm)
	{
		CurrentCrouchCoverType = ECrouchCoverType::Wall;
		CurrentStandCoverType = EStandCoverType::Wall;
	}

	if (CoverData.bIsCrouchOnlyCover || !bHoldingFirearm)
	{
		bShouldCrouch = true;
		
		GetCharacter()->ActiveCoverAimType = ECoverAimType::LeftOrRight;
	}
	else
	{
		// If both cover stances are not a wall cover type, choose a random stance
		if (CoverData.CrouchCoverType > ECrouchCoverType::Wall && CoverData.StandCoverType > EStandCoverType::Wall)
		{
			FHitResult HitResult;
			
			const FCollisionQueryParams CollisionQueryParams = UReadyOrNotFunctionLibrary::CreateCollisionQueryParams(GetCharacter(), InstigatorStimulus.InstigatorCharacter);

			FVector StartTrace = InstigatorStimulus.GetLocation();
			if (IsValid(InstigatorStimulus.InstigatorCharacter))
				StartTrace = InstigatorStimulus.InstigatorCharacter->GetMesh()->GetSocketLocation("head_end");

			// If we dont see the cover point from a stand offset, choose either crouch or stand
			if (GetWorld()->LineTraceSingleByChannel(HitResult, StartTrace, CoverData.CoverLocation + FVector::UpVector * 175.0f, ECC_Visibility, CollisionQueryParams))
			{
				bShouldCrouch = false; // Always choose stand
				//bShouldCrouch = FMath::RandBool();
			}
			// if we do see the cover point from stand offset, always crouch
			else
			{
				bShouldCrouch = true; // Always choose crouch
			}
		}
		else
		{
			if (CoverData.CrouchCoverType == ECrouchCoverType::Wall)
			{
				bShouldCrouch = false;
			}
			else if (CoverData.StandCoverType == EStandCoverType::Wall)
			{
				bShouldCrouch = true;
			}
		}
	}

	if (bShouldCrouch && CurrentCrouchCoverType == ECrouchCoverType::UpOnly)
	{
		if (CurrentCrouchCoverType == ECrouchCoverType::UpOnly)
			GetCharacter()->ActiveCoverAimType = ECoverAimType::Up;
	}
	else
	{
		GetCharacter()->ActiveCoverAimType = ECoverAimType::LeftOrRight;
	}

	const FVector& DirectionToUs = (GetCharacter()->GetActorLocation() - CoverData.CoverLocation).GetSafeNormal2D();
	//const float ForwardDotProduct = FVector::DotProduct(DirectionToUs, CoverData.CoverNormal);
	const float RightDotProduct = FVector::DotProduct(DirectionToUs, FVector::CrossProduct(CoverData.CoverNormal, FVector::UpVector));
	const bool bIsRightSideOfCoverPoint = RightDotProduct > 0.0f;
	//const bool bIsInFrontOfCoverPoint = ForwardDotProduct > 0.0f;

	//LOG_NUMBER(RightDotProduct);

	//const bool bOnSide = FMath::Abs(RightDotProduct) > 0.5f || !bIsInFrontOfCoverPoint;
	
	float Angle = 60;
	EntryAngle = 90;
	//float Angle = EntryAngle == 90 ? 0 : 90;

	CurrentCoverDirection = bIsRightSideOfCoverPoint ? ECoverDirection::Right : ECoverDirection::Left;

	if ((bShouldCrouch && CoverData.CrouchCoverType == ECrouchCoverType::LeftOnly) ||
		(!bShouldCrouch && CoverData.StandCoverType == EStandCoverType::LeftOnly))
	{
		Angle = 0;
		CurrentCoverDirection = ECoverDirection::Left;
	}
	
	if ((bShouldCrouch && CoverData.CrouchCoverType == ECrouchCoverType::RightOnly) ||
		(!bShouldCrouch && CoverData.StandCoverType == EStandCoverType::RightOnly))
	{
		Angle = 0;
		CurrentCoverDirection = ECoverDirection::Right;
	}

	GetCharacter()->bIsCrouching = bShouldCrouch;
	GetCharacter()->ActiveCoverDirection = CurrentCoverDirection;
	
	EntryAnimation = GetCharacter()->ActiveCoverDirection == ECoverDirection::Left ? "tp_cover_enter_left" : "tp_cover_enter_right";
		
	EntryAnimation += "_" + FString::FromInt(EntryAngle);

	const int32 AnimCount = GetCharacter()->GetMontageAnimCountFromTable(EntryAnimation);

	EnterAnimIndex = FMath::RandRange(0, FMath::Clamp(AnimCount-1, 0, AnimCount));
	
	#if WITH_EDITOR
	// Animation string should never be empty
	ensureAlways(!EntryAnimation.IsEmpty());
	#endif
	
	if (UAnimMontage* Montage = GetCharacter()->GetMontageFromTableWithIndex(EntryAnimation, EnterAnimIndex))
	{
		EntryMontage = Montage;
		
		if (const UAnimSequence* AnimSequence = Cast<const UAnimSequence>(Montage->SlotAnimTracks[0].AnimTrack.AnimSegments[0].AnimReference))
		{
			const FTransform& PoseTransform = AnimSequence->ExtractRootTrackTransform(Montage->GetPlayLength(), nullptr);

			const float TravelDistance = FMath::Clamp(FVector::Distance(FVector::ZeroVector, PoseTransform.GetLocation()) - 30.0f, 10.0f, 500.0f);
			EntryDistance = TravelDistance;
			//LOG_NUMBER(TravelDistance);
		}
	}

	GetCharacter()->bIsCrouching = false;
	
	const FVector EntryPoint1 = CoverData.CoverLocation + CoverData.CoverNormal.RotateAngleAxis(-Angle, FVector::UpVector) * EntryDistance;
	const FVector EntryPoint2 = CoverData.CoverLocation + CoverData.CoverNormal.RotateAngleAxis(Angle, FVector::UpVector) * EntryDistance;

	FHitResult HitResult_1, HitResult_2;
	FCollisionQueryParams CollisionQueryParams = GetCharacter()->GetCollisionQueryParameters();
	bool bEntryPoint1Hit = GetWorld()->LineTraceSingleByChannel(HitResult_1, CoverData.CoverLocation, EntryPoint1, ECC_Visibility, CollisionQueryParams);
	bool bEntryPoint2Hit = GetWorld()->LineTraceSingleByChannel(HitResult_2, CoverData.CoverLocation, EntryPoint2, ECC_Visibility, CollisionQueryParams);

	Door = Cast<ADoor>(HitResult_1.GetActor());
	
	if (!Door)
		Door = Cast<ADoor>(HitResult_2.GetActor());

	if (bIsRightSideOfCoverPoint)
	{
		if (bEntryPoint1Hit)
		{
			Door = Cast<ADoor>(HitResult_1.GetActor());
			
			FinalEntryPoint = EntryPoint2;
			FinalEntryPoint.Z = CoverData.CoverLocation.Z;
			return FinalEntryPoint;
		}

		FinalEntryPoint = EntryPoint1;
		FinalEntryPoint.Z = CoverData.CoverLocation.Z;
		return FinalEntryPoint;
	}

	if (bEntryPoint2Hit)
	{
		Door = Cast<ADoor>(HitResult_2.GetActor());

		FinalEntryPoint = EntryPoint1;
		FinalEntryPoint.Z = CoverData.CoverLocation.Z;
		return FinalEntryPoint;
	}

	FinalEntryPoint = EntryPoint2;
	FinalEntryPoint.Z = CoverData.CoverLocation.Z;
	return FinalEntryPoint;
}

ECoverDirection UTakeCoverActivity::DetermineDesiredCoverDirection()
{
	ECoverDirection CoverDirection = GetCharacter()->ActiveCoverDirection;
	
	const bool bIsExclusiveLeftOrRight = ((bShouldCrouch && CoverData.CrouchCoverType == ECrouchCoverType::LeftOnly) ||
										(bShouldCrouch && CoverData.CrouchCoverType == ECrouchCoverType::LeftAndUp) ||
										(!bShouldCrouch && CoverData.StandCoverType == EStandCoverType::LeftOnly)) ||
										((bShouldCrouch && CoverData.CrouchCoverType == ECrouchCoverType::RightOnly) ||
										(bShouldCrouch && CoverData.CrouchCoverType == ECrouchCoverType::RightAndUp) ||
										(!bShouldCrouch && CoverData.StandCoverType == EStandCoverType::RightOnly));

	if (bIsExclusiveLeftOrRight)
	{
		switch (GetCharacter()->ActiveCoverDirection)
		{
			case ECoverDirection::Left:
				if ((bShouldCrouch && CoverData.CrouchCoverType == ECrouchCoverType::RightOnly) ||
					(bShouldCrouch && CoverData.CrouchCoverType == ECrouchCoverType::RightAndUp) ||
					(!bShouldCrouch && CoverData.StandCoverType == EStandCoverType::RightOnly))
				{
					CoverDirection = ECoverDirection::Right;
				}
			break;
			
			case ECoverDirection::Right:
				if ((bShouldCrouch && CoverData.CrouchCoverType == ECrouchCoverType::LeftOnly) ||
					(bShouldCrouch && CoverData.CrouchCoverType == ECrouchCoverType::LeftAndUp) ||
					(!bShouldCrouch && CoverData.StandCoverType == EStandCoverType::LeftOnly))
				{
					CoverDirection = ECoverDirection::Left;
				}
			break;
			
			default:
			break;
		}
	}
	else
	{
		const FVector CoverLocation = CoverData.CoverLocation;
		const FVector SenseLocation = EnterCoverCount == 0 ? InstigatorStimulus.GetLocation() : LastSensedEnemyPosition;
		const FVector DirectionToInstigator = (SenseLocation - CoverLocation).GetSafeNormal2D();
		const float RightDotProduct = FVector::DotProduct(DirectionToInstigator, FVector::CrossProduct(CoverData.CoverNormal, FVector::UpVector));

		CoverDirection = RightDotProduct > 0.0f ? ECoverDirection::Right : ECoverDirection::Left;
	}

	return CoverDirection;
}

void UTakeCoverActivity::ResetCoverData()
{
	LOG_CLASS_FUNC
	
	UReadyOrNotFunctionLibrary::StopCallbackTimer(this, ChangeCoverFireTypeTimer);
	UReadyOrNotFunctionLibrary::StopCallbackTimer(this, ChangeCoverDirectionTypeTimer);
	UReadyOrNotFunctionLibrary::StopCallbackTimer(this, ForgetLocationTimer);

	Door = nullptr;
	LastExitMontagePlayed = nullptr;
	
	OriginalCrouchCoverType = ECrouchCoverType::Wall;
	CurrentCrouchCoverType = ECrouchCoverType::Wall;
	OriginalStandCoverType = EStandCoverType::Wall;
	CurrentStandCoverType = EStandCoverType::Wall;
	
	CurrentCoverFireType = ECoverFireType::None;
	CurrentCoverDirection = ECoverDirection::Left;
	PreviousCoverFireType = ECoverFireType::None;

	InstigatorLocationHistory.Reset();
	
	EntryLocation = FVector::ZeroVector;
	CoverFireFocalPoint = FVector::ZeroVector;
	CoverFireFocalPoint_Entry = FVector::ZeroVector;
	LastSensedEnemyPosition = FVector::ZeroVector;

	EnterAnimIndex = 0;
	EnterAimAnimIndex = 0;
	EnterCoverCount = 0;
	//FiredAtCount = 0;
	EntryAngle = 0;
	ReloadAtAmmoCount = 0;
	
	ElapsedTimeInCover = 0.0f;
	TimeSinceHeardNoiseInCover = 0.0f;
	TimeSinceLastShotAt = 999.0f;
	CoverFireTime = 0.0f;
	EntryDistance = 0.0f;
	PathDistanceToCover = 0.0f;
	EntryDotProductDecreaseRate = 0.0f;
	
	EntryAnimation = "";
	
	bShouldCrouch = false;
	bHasCoverFireLOS = false;
	
	bShouldExitCoverFireNow = false;
	bShouldTryCoverFireNow = false;
	bShouldCompleteCoverNow = false;

	bHasEverEnteredCover = false;
	
	bEverHadLOSToEnemyInPreviousCoverFire = false;
	bChosenCoverAimType = false;
	bChosenEntryPoint = false;

	bHasEverSideSwitched = false;
	bInitialEntryComplete = false;

	bPerpetualCoverFire = false;
}
