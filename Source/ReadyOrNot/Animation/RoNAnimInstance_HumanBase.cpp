// Void Interactive, 2020

#include "Animation/RoNAnimInstance_HumanBase.h"

// ##UE5UPGRADE## Duplicated in Engine
// #include "Animation/CachedAnimDataLibrary.h"
#include "Engine/Public/Animation/CachedAnimDataLibrary.h"

#include "Characters/CyberneticCharacter.h"

#include "Animation/StrideWarping/StrideWarpingLibrary.h"

#include "Info/Activities/BaseCombatActivity.h"
#include "Info/Activities/WorldBuildingActivity.h"

DECLARE_CYCLE_STAT_EXTERN(TEXT("RoNAnimInstance_HumanBase_NativeUpdate"), STAT_NativeRoNHumanBaseAnimUpdate, STATGROUP_Anim,);
static TAutoConsoleVariable<float> CVarRonDrawMovestyle(TEXT("a.RonDrawMovestyle"), 0, TEXT("1 = Draw Move style debug o nscreen"));
DEFINE_STAT(STAT_NativeRoNHumanBaseAnimUpdate);

URoNAnimInstance_HumanBase::URoNAnimInstance_HumanBase(const FObjectInitializer& ObjectInitializer)
{
	SpeedScaling = 1.0f;
	Lean = 0.f;
	LeanClamped = 0.0f;
	LeanFactor = 0.3f;
	LeanInterpSpeed = 10.f;
	ActiveMoveStyleName = "None";
	ActiveGaitIndex = 0;
	PlayrateClampMax = 0.15f;
	bCreatedDeathPose = false;
	bIsStrafing = false;
	bIsMoveStyleSlotBActive = false;
	AdjustedPlayrate = 1.0f;
	CurrentStrafeDirection = EStrafeDirection::F;
	MoveStyleComponent = nullptr;
	DirAngle = 0.0f;
	StrafeDirectionAngle = 0.0f;
	bIsDead = false;
	bIsArrested = false;
	bSurrendered = false;
	CurWeaponType = EAnimWeaponType::CWT_Any;
	bIsGetUpPlaying = false;
	bIsFemale = false;
	bIsUnarmed = false;
	bIsSWAT = false;
	bEnableIKProcess = false;
	bWeaponDown = false;
	bIsPistolAndWeaponDown = false;
	PistolLeftHandIKAlphaChange = 1.0f;
	Calm_Override_Pose = nullptr;
	Aiming_Override_Pose = nullptr;
	bIsReloading = false;
	FinalAimOffsetAlpha = 1.0f;
	bAnyMontageIsActive = false;
	bFullBodyMontagePlaying = false;
	bUpperBodyMontagePlaying = false;
	bInteractionMontagePlaying = false;
	CurOverrideRule = EItemOverrideRule::NONE;
	bFullOrInteractionMontagePlaying = false;
	bIsLoweredSet = false;
	bIsPistol = false;
	bInRagdoll = false;
	bIsCarried = false;
	bIsCrouching = false;
	HidingAnimStateMachineData = {};
	bAllowTurnInPlace = false;
	bExitTurnRecoveryIfMoving = false;
	bIsTurnInPlaceStateRelevant = false;
	bIsArrestedAndDead = false;
	bDisableAdditiveOverrides = false;

	TurnAnimStateData.StateMachineName = "Turn_SM";
	TurnAnimStateData.StateName = "TurnInPlace";

	FAnimTurnTransition CurEmptyTransition;
	CurEmptyTransition.DelayBeforeTrigger = 0.1f;
	TurnInPlaceAnimSet.TurnDeadZoneAngle = 75.0f;
	TurnInPlaceAnimSet.TurnTransitions.Init(CurEmptyTransition, 4);

	TurnInPlaceSpeedMultiplier = 1.6f; // twice as fast
	bMoveStyleChanging = false;
}

void URoNAnimInstance_HumanBase::NativeInitializeAnimation()
{
	
	for (int32 i = 0; i < 6; i++)
	{
		FillMoveStyleSlots(IdleData_Default, TurnData_Default, TransitionData_Default, LocomotionData_Default, StrafeBS_Default, NonStrafeBS_Default, i);
	}
	
	AReadyOrNotCharacter* OwnerCharacter = Cast<AReadyOrNotCharacter>(TryGetPawnOwner());

	if (!IsValid(OwnerCharacter))
		return;
	
	// try to find the move style component
	TInlineComponentArray<UActorComponent*> Components;
	OwnerCharacter->GetComponents(Components);


	for (int32 CompIdx = 0; CompIdx < Components.Num(); CompIdx++)
	{
		UActorComponent* Comp = Components[CompIdx];
		//UE_LOG(LogTemp, Warning, TEXT("URoNAnimInstance_HumanBase: %s"), *Comp->GetName());

		URoNMoveStyleComponent* CurMoveStyleComponent = Cast<URoNMoveStyleComponent>(Comp);

		if (CurMoveStyleComponent)
		{
			MoveStyleComponent = CurMoveStyleComponent;
			break;
		}
	}

	// fill slots already
	if (MoveStyleComponent)
	{
		MoveStyleComponent->SetMovementStyleByName("male01_shared_unarmed_strafe");
		//GEngine->AddOnScreenDebugMessage(-1, -1.0f, FColor::White,"Component is valid");
		SetMoveStyleDataFromComp(MoveStyleComponent);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("URoNAnimInstance_HumanBase: Move Style Component is invalid!"));
	}
}

// main func for tick calculations
void URoNAnimInstance_HumanBase::NativeUpdateAnimation(const float DeltaSeconds)
{
	AReadyOrNotCharacter* OwnerCharacter = Cast<AReadyOrNotCharacter>(TryGetPawnOwner());

	if (!IsValid(OwnerCharacter))
		return;

	const bool bIsActiveForMovement = !OwnerCharacter->IsDeadOrUnconscious() && !OwnerCharacter->IsIncapacitated() && !OwnerCharacter->IsInRagdoll();
	
	if (bIsActiveForMovement)
	{
		CalculateLean(OwnerCharacter, DeltaSeconds);
	}
	else
	{
		Lean = 0.0f;
		LeanClamped = 0.0f;
		ActorRotation = FRotator::ZeroRotator;
	}

	if (MoveStyleComponent && bIsActiveForMovement)
	{
		DoMoveStyleCompUpdate();
		AimOffsetAlpha = FMath::FInterpTo(AimOffsetAlpha, 1.0f, DeltaSeconds, 10.0f);
	}
	else
	{
		AimOffsetAlpha = FMath::FInterpTo(AimOffsetAlpha, 0.0f, DeltaSeconds, 10.0f);
	}

	// added to have a way to know if we are blending the move styles
	// notify system that we are in blending phase now
	if (bMoveStyleChanging)
	{
		MoveStyleBlendResetCounter = MoveStyleBlendResetCounter + DeltaSeconds;

		if (MoveStyleBlendResetCounter >= MoveStyleBlendCoolDown)
		{
			ResetMoveStyleChangeStatus();
		}
	}

	/* to adjust animations to current capsule speed, may need to divide/decide how much to spread across stride and playback weighting */
	/*
	const float CurrentSpeedCurveValue = GetCurveValue("speed");
	const float PlayrateScaling = OwnerCharacter->GetVelocity().Size2D() / CurrentSpeedCurveValue;
	if (CurrentSpeedCurveValue != 0.0f)
		AdjustedPlayrate = FMath::Clamp(PlayrateScaling, 0.0f, PlayrateClampMax);
	else
		AdjustedPlayrate = 1.0f;
	*/

	// calculate variables needed for blendspace support!
	if (bIsActiveForMovement)
	{
		if (bIsStrafing)
			CalcStrafeBSValues(DeltaSeconds, OwnerCharacter, MoveStyleComponent);
		else
			CalcNonStrafeBSValues(DeltaSeconds, OwnerCharacter, MoveStyleComponent);
		/* 04.05.22 Alex: Implemented support for stride/speed warping */
		
		const float CurrentSpeedCurveValue = GetCurveValue("speed");
		UStrideWarpingLibrary::UpdateStrideWarping(DeltaSeconds, OwnerCharacter->GetVelocity(), VelocityInterpTime, CurrentSpeedCurveValue, PlayrateClampMax, SpeedScaling, AdjustedPlayrate, VelocitySmoothed);
		
		// strafe direction
		CalculateStrafeDirection(DeltaSeconds, OwnerCharacter->GetVelocity(), OwnerCharacter->GetActorRotation());
		StrafeDirectionAngle = FMath::RadiansToDegrees(DirAngle);
	}
	else
	{
		move_x = 0.0f;
		move_y = 0.0f;
		StrafeDirectionAngle = 0.0f;
		DirAngle = 0.0f;
	}

	bAnyMontageIsActive = IsAnyMontagePlaying();
	bFullBodyMontagePlaying = GetSlotMontageGlobalWeight("DefaultSlot") >= 1.0f;
	bUpperBodyMontagePlaying = GetSlotMontageGlobalWeight("Upperbody") >= 1.0f;
	bInteractionMontagePlaying = GetSlotMontageGlobalWeight("Interaction") >= 1.0f;
	bFullOrInteractionMontagePlaying = bFullBodyMontagePlaying || bInteractionMontagePlaying;
	
	// ron specifics for AI
	if (const ACyberneticCharacter* CurCyberneticChar = Cast<ACyberneticCharacter>(OwnerCharacter))
	{
		bRecoveringFromRagdoll = CurCyberneticChar->bRecoveringFromRagdoll;
		bIsDead = CurCyberneticChar->IsDeadOrUnconscious();
		bIncapacitated = CurCyberneticChar->IsIncapacitated() && CurCyberneticChar->bBlendInIncapacitation;
		IncapacitationLoopAnim = CurCyberneticChar->IncapacitationLoopAnim;
		bIsPlayingDeathAnim = CurCyberneticChar->bStartedPlayingDeath || CurCyberneticChar->bPlayingDeathMontage;
		bInRagdoll = CurCyberneticChar->IsInRagdoll() && !bIsPlayingDeathAnim; // Alex 10/10/23 Because Physical Animation has the Simulation running it will trigger this so only make it true if we are not playing a death animation!
		WorldBuildingAnimState = CurCyberneticChar->Rep_WorldBuildingAnimState;
		CarryArrestAnimState = CurCyberneticChar->Rep_CarryArrestedAnimState;
		TakeHostageAnimState = CurCyberneticChar->Rep_TakeHostageAnimState;
		CoverAnimStateMachineData = CurCyberneticChar->Rep_CoverAnimState;
		HidingAnimStateMachineData = CurCyberneticChar->Rep_HidingAnimState;
		HoleTraversalAnimStateMachineData = CurCyberneticChar->Rep_HoleTraversalAnimState;
		
		bIsArrestedAndDead = CurCyberneticChar->IsArrestedAndDead() || CurCyberneticChar->IsArrestedAndIncapacitated();
		bIsArrested = CurCyberneticChar->bArrestComplete || bIsArrestedAndDead;
		bSurrendered = CurCyberneticChar->IsSurrenderedFor(1.5f) || CurCyberneticChar->IsSurrenderComplete();
		bIsFemale = CurCyberneticChar->bFemale;
		bIsCarried = CurCyberneticChar->IsCarried();
		bIsGetUpPlaying = CurCyberneticChar->IsGettingUp();
		bIsCrouching = CurCyberneticChar->IsCrouching();
		bIsArrestedAsRagdoll = CurCyberneticChar->bArrestedAsRagdoll;
		bIsBeingArrested = CurCyberneticChar->bIsBeingArrested;
		bIsBeingArrestedAsRagdoll = bIsBeingArrested && bIsArrestedAsRagdoll;

		//const bool bHostageEventsPlaying = CurCyberneticChar->IsBeginningHostageTake() || CurCyberneticChar->IsEndingHostageTake();
		
		// additions for hostage taking
		bIsHostageTaker = CurCyberneticChar->IsTakingHostage();// && !bHostageEventsPlaying;
		bIsHostage = CurCyberneticChar->IsBeingTakenHostage(); //&& !bHostageEventsPlaying;
		bIsHostageOrHostageTaker = (bIsHostageTaker || bIsHostage);// && !bHostageEventsPlaying;

		// ALEX HACK: if we are in hostage mode exclude any active override!
		CurWeaponType = bIsHostageOrHostageTaker ? EAnimWeaponType::CWT_Unarmed : CurCyberneticChar->GetCurrentWeaponAnimType();
		
		bWeaponDown = CurCyberneticChar->IsLowReady();

		//const float HeadInterpSpeed = CurCyberneticChar->GetVelocity().Size() > 0.0f ? FMath::Clamp(CurCyberneticChar->GetVelocity().Size(), 0.0f, 25.0f) : 1.5f;
		HeadLookRotation = UKismetMathLibrary::RInterpTo(HeadLookRotation, CurCyberneticChar->GetLookAtRotation(35.0f, 30.0f), DeltaSeconds, 1.0f);
		
		// IK Processing should be turned off under these conditions
		const bool bDisableIK = bInRagdoll || bIsDead || bIsGetUpPlaying || WorldBuildingAnimState.bIsPlaying || bSurrendered || bIsArrested || bIsCarried;
		bEnableIKProcess = !bDisableIK;

		// are we currently armed?
		bIsUnarmed = CurWeaponType == EAnimWeaponType::CWT_Unarmed || CurWeaponType == EAnimWeaponType::CWT_Arrested || CurWeaponType == EAnimWeaponType::CWT_Surrendered;

		/* ruleset for Arm IK */
		/*
		 * Disable for:
		 * Unarmed, Arrested, Complying, Pistol not Aiming ( Not Strafing simply ? )
		 *
		 **/

		// needed for IK!
		ArmsOnlySlotAlpha = (GetSlotMontageGlobalWeight("ArmsOnly") == 1.0f) ? FMath::FInterpTo(ArmsOnlySlotAlpha, 0.0f, 12.0f, DeltaSeconds) : FMath::FInterpTo(ArmsOnlySlotAlpha, 1.0f, 12.0f, DeltaSeconds);
		LeftArmOnlySlotAlpha = (GetSlotMontageGlobalWeight("LeftArmOnly") == 1.0f) ? FMath::FInterpTo(LeftArmOnlySlotAlpha, 0.0f, 12.0f, DeltaSeconds) : FMath::FInterpTo(LeftArmOnlySlotAlpha, 1.0f, 12.0f, DeltaSeconds);

		if (CurCyberneticChar->IsTableMontagePlaying("tp_swat_gestures"))
			HandAdditiveLockOverride = 1.0f + GetCurveValue("Hand_IK_Left");
		else
			HandAdditiveLockOverride = 1.0f;

		if (!CurCyberneticChar->IsActive())
		{
			SlotBlendTime = 0.0f;
		}
		else
		{
			SlotBlendTime = DefaultSlotBlendTime;
		}

		if (bSurrendered)
		{
			StrafeBlendTime = 1.0f;
		}
		else
		{
			StrafeBlendTime = 0.3f;
		}

		bIsPistolAndWeaponDown = CurWeaponType == EAnimWeaponType::CWT_Pistol && bWeaponDown;
		bIsPistol = CurWeaponType == EAnimWeaponType::CWT_Pistol;

		/* if we have a pistol and its lowered then we assume its held only in the right hand so disable left hand ik fully even if armed */
		PistolLeftHandIKAlphaChange = FMath::FInterpTo(PistolLeftHandIKAlphaChange, bIsPistolAndWeaponDown ? 0.0f : 1.0f, DeltaSeconds, 10.0f);
			
		if (bInRagdoll || bIsArrested || bIsDead || bSurrendered || bIsUnarmed || bIsGetUpPlaying || bMoveStyleChanging)
		{
			LeftArmIKAlpha = FMath::FInterpTo(LeftArmIKAlpha, 0.0f, DeltaSeconds, 10.f) * LeftArmOnlySlotAlpha * ArmsOnlySlotAlpha * HandAdditiveLockOverride;
			RightArmIKAlpha = FMath::FInterpTo(RightArmIKAlpha, 0.0f, DeltaSeconds, 10.f) * ArmsOnlySlotAlpha;
		}
		else
		{
			LeftArmIKAlpha = FMath::FInterpTo(LeftArmIKAlpha, 1.0f, DeltaSeconds, 10.f) * LeftArmOnlySlotAlpha * ArmsOnlySlotAlpha * HandAdditiveLockOverride * PistolLeftHandIKAlphaChange;
			RightArmIKAlpha = FMath::FInterpTo(RightArmIKAlpha, 1.0f, DeltaSeconds, 10.f) * ArmsOnlySlotAlpha;
		}

		/* 22.04.22 Alex: new boolean for reading reloading state */
		const ABaseItem* CurItem = CurCyberneticChar->GetEquippedItem();
		if (CurItem)
		{
			if (const ACyberneticController* CurController = CurCyberneticChar->GetCyberneticsController())
			{
				if (const UBaseCombatActivity* CombatActivity = CurController->GetCombatActivity())
					bIsReloading = CombatActivity->IsReloading();
				else
					bIsReloading = false;
			}
		}
		

		if (CurItem && CurItem->AnimationData)
		{
			Calm_Override_Pose = CurItem->AnimationData->IdlePose_AI_Calm;
			Aiming_Override_Pose = CurItem->AnimationData->IdlePose_AI_Aiming;
		}
		else
		{
			if (Default_Calm_Override_Pose)
				Calm_Override_Pose = Default_Calm_Override_Pose;

			if (Default_Aiming_Override_Pose)
				Aiming_Override_Pose = Default_Aiming_Override_Pose;
		}


		bool bPerformingWorldBuilding = false;
		if (const ACyberneticController* CurController = CurCyberneticChar->GetCyberneticsController())
		{
			bPerformingWorldBuilding = CurController->GetCurrentActivity<UWorldBuildingActivity>() != nullptr;
		}

		bool bIsRaisingWeapon = CurCyberneticChar->IsRaisingWeapon();

		// alex 06.06.22 fix issues with additives and aimspace messing up melee/reload, todo add other actions that need to blend it out here
		UAnimMontage* CurPlayingMontage = GetCurrentActiveMontage();
		if (CurPlayingMontage)
		{
			/*
			if (CurPlayingMontage->GetName().Contains("reload") || CurPlayingMontage->GetName().Contains("melee") || CurPlayingMontage->GetName().Contains("hit"))
				bDisableAdditiveOverrides = true;
			else
				bDisableAdditiveOverrides = false;
			*/

			for (int32 tagidx = 0; tagidx < AdditiveDisableTagList.Num(); tagidx++)
			{
				if (CurPlayingMontage->GetName().Contains(AdditiveDisableTagList[tagidx]))
				{
					bDisableAdditiveOverrides = true;
					//ULog::Info("Disabling additive override because " + CurPlayingMontage->GetName() + " is playing");
					break;
				}
			}
		}
		else
		{
			bDisableAdditiveOverrides = false;
		}

		float TempAimOffsetValue;

		if (bDisableAdditiveOverrides)
			TempAimOffsetValue = 0.0f;
		else
			TempAimOffsetValue = 1.0f;

		if (bMoveStyleChanging)
			TempAimOffsetValue = 0.0f;

		if (bIsRaisingWeapon)
		{
			TempAimOffsetValue = 1.0f;
		}

		FinalAimOffsetAlpha = FMath::FInterpTo(FinalAimOffsetAlpha, TempAimOffsetValue, DeltaSeconds, 3.5f);

		if (bIsActiveForMovement)
		{
			// turn in place related
			if (FMath::Abs(OwnerCharacter->GetVelocity().Size2D()) > 75.0f ||
				CurCyberneticChar->bIsMoving || 
				(CurCyberneticChar->GetCurrentMontage() && !CurCyberneticChar->IsRaisingWeapon() && !CurCyberneticChar->IsRecoiling() && !CurCyberneticChar->IsUpperBodyMontagePlaying()) ||
				CurCyberneticChar->Rep_CoverAnimState.bIsInCover ||
				bPerformingWorldBuilding ||
				CurCyberneticChar->bIsPairedInteractionPlaying ||
				CurCyberneticChar->bIsBeingArrested ||
				bInRagdoll ||
				CurCyberneticChar->IsDeadOrUnconscious() || CurCyberneticChar->IsIncapacitated() ||
				bIsPlayingDeathAnim ||
				bIsArrested || bIsArrestedAndDead ||
				CurCyberneticChar->IsTakingHostage() ||
				CurCyberneticChar->IsBeingTakenHostage())
				bAllowTurnInPlace = false;
			else
				bAllowTurnInPlace = true;

			// limit to prevent issues!
			UAnimTurnInPlaceLibrary::UpdateTurnInPlace(DeltaSeconds, bAllowTurnInPlace, false, bIsTurnInPlaceStateRelevant, true, YawOffsetLimit, CurCyberneticChar->GetMesh()->GetComponentRotation(), TurnInPlaceAnimSet, TurnInPlaceState, TurnInPlaceSpeedMultiplier);

			if (OwnerCharacter->GetVelocity().Size2D() != 0.0f && !TurnInPlaceState.bTurnRecoveryRequested)
				bExitTurnRecoveryIfMoving = true;
			else
				bExitTurnRecoveryIfMoving = false;

			// aim offset should only be interpolated here
			AimOffsetInterpolated = FMath::Vector2DInterpTo(AimOffsetInterpolated, FVector2D(bAllowTurnInPlace ? TurnInPlaceState.RootYawOffset : CurCyberneticChar->AimOffset.X, CurCyberneticChar->AimOffset.Y), DeltaSeconds, CurCyberneticChar->AimOffsetInterpSpeed);
			
			// update turn in place
			bIsTurnInPlaceStateRelevant = UCachedAnimDataLibrary::StateMachine_IsStateRelevant(this, TurnAnimStateData);
		}
		else
		{
			YawOffsetLimit = 0.0f;
			bAllowTurnInPlace = false;
			bIsTurnInPlaceStateRelevant = false;
			bExitTurnRecoveryIfMoving = false;
			AimOffsetInterpolated = FVector2D::ZeroVector;
		}
	}

	// tick this last
	NativeLastTick(DeltaSeconds);
}

void URoNAnimInstance_HumanBase::DoMoveStyleCompUpdate()
{
	if (MoveStyleComponent)
	{
		//UE_LOG(LogTemp, Warning, TEXT("URoNAnimInstance_HumanBase: Move Style Component is valid!"));
		UpdateMoveStyleDataFromComp(MoveStyleComponent);
		UpdateGaitDataFromComp(MoveStyleComponent);
	}
}

void URoNAnimInstance_HumanBase::NativeLastTick(float DeltaSeconds)
{
}

void URoNAnimInstance_HumanBase::IncrementSlotIdx()
{
	SlotIdx++;
	if (SlotIdx > 6)
	{
		SlotIdx = 0;
	}
}

void URoNAnimInstance_HumanBase::FillMoveStyleSlots(FRoNStyleIdleData& IdleData, FRoNStyleTurnData& TurnData, FRoNGaitTransitionData& TransitionData, FRoNGaitLocomotionData& LocomotionData, UBlendSpace* StrafeBSData, UBlendSpace* NonStrafeBSData, int32 Slot)
{
	FRandomStream RandomStream;
	// If we can get to the gamestate then use the synced seed for random streams
	if (UReadyOrNotStatics::GetReadyOrNotGameState())
	{
		RandomStream = UReadyOrNotStatics::GetReadyOrNotGameState()->RandomStream;
	}

	// Slot has not been incremented since previously calling this function... idle adnimations will bug!
	#if WITH_EDITOR
	ensure (Slot != PreviousSlotIdx);
	#endif
	
	if (Slot == PreviousSlotIdx)
		return;
	
	PreviousSlotIdx = Slot;

	switch(Slot)
	{
		case 0:
			Slot0.IdleData = IdleData;
			Slot0.TurnData = TurnData;
			Slot0.TransitionData = TransitionData;
			Slot0.LocomotionData = LocomotionData;

			if(NonStrafeBSData != nullptr)
				Slot0.NonStrafeBSData = NonStrafeBSData;
			if(StrafeBSData != nullptr)
				Slot0.StrafeBSData = StrafeBSData;

			if(Slot0.IdleData.BaseIdleData.Num() != 0)
			{
				CurrentIdleIndex = UKismetMathLibrary::RandomIntegerInRangeFromStream(0, Slot0.IdleData.BaseIdleData.Num() - 1, RandomStream);
				Slot0.IdleReference = Slot0.IdleData.BaseIdleData[CurrentIdleIndex];
			}
			break;
		case 1:
			Slot1.IdleData = IdleData;
			Slot1.TurnData = TurnData;
			Slot1.TransitionData = TransitionData;
			Slot1.LocomotionData = LocomotionData;

			if (NonStrafeBSData != nullptr)
				Slot1.NonStrafeBSData = NonStrafeBSData;
			if (StrafeBSData != nullptr)
				Slot1.StrafeBSData = StrafeBSData;

			if(Slot1.IdleData.BaseIdleData.Num() != 0)
			{
				CurrentIdleIndex = UKismetMathLibrary::RandomIntegerInRangeFromStream(0, Slot1.IdleData.BaseIdleData.Num() - 1, RandomStream);
				Slot1.IdleReference = Slot1.IdleData.BaseIdleData[CurrentIdleIndex];
			}
			break;

		case 2:
			Slot2.IdleData = IdleData;
			Slot2.TurnData = TurnData;
			Slot2.TransitionData = TransitionData;
			Slot2.LocomotionData = LocomotionData;

			if (NonStrafeBSData != nullptr)
				Slot2.NonStrafeBSData = NonStrafeBSData;
			if (StrafeBSData != nullptr)
				Slot2.StrafeBSData = StrafeBSData;

			if(Slot2.IdleData.BaseIdleData.Num() != 0)
			{
				CurrentIdleIndex = UKismetMathLibrary::RandomIntegerInRangeFromStream(0, Slot2.IdleData.BaseIdleData.Num() - 1, RandomStream);
				Slot2.IdleReference = Slot2.IdleData.BaseIdleData[CurrentIdleIndex];
			}
			break;

		case 3:
			Slot3.IdleData = IdleData;
			Slot3.TurnData = TurnData;
			Slot3.TransitionData = TransitionData;
			Slot3.LocomotionData = LocomotionData;

			if (NonStrafeBSData != nullptr)
				Slot3.NonStrafeBSData = NonStrafeBSData;
			if (StrafeBSData != nullptr)
				Slot3.StrafeBSData = StrafeBSData;

			if(Slot3.IdleData.BaseIdleData.Num() != 0)
			{
				CurrentIdleIndex = UKismetMathLibrary::RandomIntegerInRangeFromStream(0, Slot3.IdleData.BaseIdleData.Num() - 1, RandomStream);
				Slot3.IdleReference = Slot3.IdleData.BaseIdleData[CurrentIdleIndex];
			}
			break;

		case 4:
			Slot4.IdleData = IdleData;
			Slot4.TurnData = TurnData;
			Slot4.TransitionData = TransitionData;
			Slot4.LocomotionData = LocomotionData;

			if (NonStrafeBSData != nullptr)
				Slot4.NonStrafeBSData = NonStrafeBSData;
			if (StrafeBSData != nullptr)
				Slot4.StrafeBSData = StrafeBSData;

			if(Slot4.IdleData.BaseIdleData.Num() != 0)
			{
				CurrentIdleIndex = UKismetMathLibrary::RandomIntegerInRangeFromStream(0, Slot4.IdleData.BaseIdleData.Num() - 1, RandomStream);
				Slot4.IdleReference = Slot4.IdleData.BaseIdleData[CurrentIdleIndex];
			}
			break;

		case 5:
			Slot5.IdleData = IdleData;
			Slot5.TurnData = TurnData;
			Slot5.TransitionData = TransitionData;
			Slot5.LocomotionData = LocomotionData;

			if (NonStrafeBSData != nullptr)
				Slot5.NonStrafeBSData = NonStrafeBSData;
			if (StrafeBSData != nullptr)
				Slot5.StrafeBSData = StrafeBSData;

			if(Slot5.IdleData.BaseIdleData.Num() != 0)
			{
				CurrentIdleIndex = UKismetMathLibrary::RandomIntegerInRangeFromStream(0, Slot5.IdleData.BaseIdleData.Num() - 1, RandomStream);
				Slot5.IdleReference = Slot5.IdleData.BaseIdleData[CurrentIdleIndex];
			}
			break;

		case 6:
			Slot6.IdleData = IdleData;
			Slot6.TurnData = TurnData;
			Slot6.TransitionData = TransitionData;
			Slot6.LocomotionData = LocomotionData;

			if (NonStrafeBSData != nullptr)
				Slot6.NonStrafeBSData = NonStrafeBSData;
			if (StrafeBSData != nullptr)
				Slot6.StrafeBSData = StrafeBSData;

			if(Slot6.IdleData.BaseIdleData.Num() != 0)
			{
				CurrentIdleIndex = UKismetMathLibrary::RandomIntegerInRangeFromStream(0, Slot6.IdleData.BaseIdleData.Num() - 1, RandomStream);
				Slot6.IdleReference = Slot6.IdleData.BaseIdleData[CurrentIdleIndex];
			}
			break;

		default:
			Slot0.IdleData = IdleData;
			Slot0.TurnData = TurnData;
			Slot0.TransitionData = TransitionData;
			Slot0.LocomotionData = LocomotionData;

			if (NonStrafeBSData != nullptr)
				Slot0.NonStrafeBSData = NonStrafeBSData;
			if (StrafeBSData != nullptr)
				Slot0.StrafeBSData = StrafeBSData;

			if(Slot0.IdleData.BaseIdleData.Num() != 0)
			{
				CurrentIdleIndex = FMath::RandRange(0, Slot0.IdleData.BaseIdleData.Num() - 1);
				Slot0.IdleReference = Slot0.IdleData.BaseIdleData[CurrentIdleIndex];
			}
			break;
	}

#if !UE_BUILD_SHIPPING
	if (CVarRonDrawMovestyle.GetValueOnAnyThread() > 0)
	{
		AReadyOrNotCharacter* OwnerCharacter = Cast<AReadyOrNotCharacter>(TryGetPawnOwner());
		if (OwnerCharacter)
		{
			FString DebugStr = FString::Format(TEXT("MoveStyle: {0} GaitIdx: {1} Slot: {2}"), {ActiveMoveStyleName.ToString(), FString::FromInt(ActiveGaitIndex), FString::FromInt(Slot)});
			DrawDebugString(GetWorld(), OwnerCharacter->GetActorLocation() + FVector(0.0f, 0.0f, Slot * 10.0f), DebugStr, nullptr, FColor::White, 2.0f, false, 1);	
		}
		
	}
#endif
}


void URoNAnimInstance_HumanBase::CalculateLean(ACharacter* CharRef, float DeltaSeconds)
{
	const FRotator NewActorRotation = CharRef->GetActorRotation();
	const float YawDelta = FMath::FindDeltaAngleDegrees(ActorRotation.Yaw, NewActorRotation.Yaw);
	Lean = FMath::FInterpTo(Lean, YawDelta / DeltaSeconds * LeanFactor, DeltaSeconds, LeanInterpSpeed);
	LeanClamped = FMath::ClampAngle(Lean, -45.0f, 45.0f);
	ActorRotation = NewActorRotation;
}

void URoNAnimInstance_HumanBase::SetMoveStyleDataFromComp(URoNMoveStyleComponent* MoveStyleComp)
{
	if (MoveStyleComp)
	{
		bIsStrafing = MoveStyleComp->bIsStrafing;
		ActiveMoveStyleName = MoveStyleComp->ActiveMoveStyle.Name;

		FRoNStyleIdleData IdleData = MoveStyleComp->ActiveMoveStyle.IdleData;
		//FRoNStyleTurnData TurnData = MoveStyleComp->ActiveMoveStyle.TurnData;

		if (MoveStyleComp->ActiveMoveStyle.GaitEntries.IsValidIndex(MoveStyleComp->ActiveGaitIndex))
		{
			FRoNGaitTransitionData TransitionData = MoveStyleComp->ActiveMoveStyle.GaitEntries[MoveStyleComp->ActiveGaitIndex].TransitionData;
			FRoNGaitLocomotionData LocomotionData = MoveStyleComp->ActiveMoveStyle.GaitEntries[MoveStyleComp->ActiveGaitIndex].LocomotionData;

			UBlendSpace* StrafeBSData = MoveStyleComp->ActiveMoveStyle.StrafeBS;
			UBlendSpace* NonStrafeBSData = MoveStyleComp->ActiveMoveStyle.NonStrafeBS;

			for (int32 i = 0; i < 6; i++)
			{
				FillMoveStyleSlots(IdleData, TurnData_Default, TransitionData, LocomotionData, StrafeBSData, NonStrafeBSData, i);
			}
		}

		// also fill turn in place data
		FillTurnInPlaceAnimSetFromTurnData(MoveStyleComp->ActiveMoveStyle.TurnData);

		// set the current override rule
		CurOverrideRule = MoveStyleComp->ActiveMoveStyle.ItemOverrideRule;
		bIsLoweredSet = MoveStyleComp->ActiveMoveStyle.bIsLoweredSet;

		ActiveGaitIndex = MoveStyleComp->ActiveGaitIndex;

		// notify system that we are in blending phase now
		TriggerMoveStyleChangeStatus();
	}
	
}

void URoNAnimInstance_HumanBase::UpdateMoveStyleDataFromComp(URoNMoveStyleComponent* MoveStyleComp)
{
	if (MoveStyleComp)
	{
		bIsStrafing = MoveStyleComp->bIsStrafing;

		if (MoveStyleComp->ActiveMoveStyle.Name != ActiveMoveStyleName)
		{
			ActiveMoveStyleName = MoveStyleComp->ActiveMoveStyle.Name;

			IncrementSlotIdx();
			FillMoveStyleSlots(MoveStyleComp->ActiveMoveStyle.IdleData, 
				MoveStyleComp->ActiveMoveStyle.TurnData,
				MoveStyleComp->ActiveMoveStyle.GaitEntries[MoveStyleComp->ActiveGaitIndex].TransitionData,
				MoveStyleComp->ActiveMoveStyle.GaitEntries[MoveStyleComp->ActiveGaitIndex].LocomotionData, 
				MoveStyleComp->ActiveMoveStyle.StrafeBS, 
				MoveStyleComp->ActiveMoveStyle.NonStrafeBS, 
				SlotIdx);

			// also fill turn in place data
			FillTurnInPlaceAnimSetFromTurnData(MoveStyleComp->ActiveMoveStyle.TurnData);

			// set the current override rule
			CurOverrideRule = MoveStyleComp->ActiveMoveStyle.ItemOverrideRule;
			bIsLoweredSet = MoveStyleComp->ActiveMoveStyle.bIsLoweredSet;

			// notify system that we are in blending phase now
			TriggerMoveStyleChangeStatus();
		}
	}
}

void URoNAnimInstance_HumanBase::UpdateGaitDataFromComp(URoNMoveStyleComponent* MoveStyleComp)
{
	if (MoveStyleComp)
	{
		if (ActiveGaitIndex != MoveStyleComp->ActiveGaitIndex)
		{
			// set new data
			IncrementSlotIdx();
			FillMoveStyleSlots(MoveStyleComp->ActiveMoveStyle.IdleData, 
				MoveStyleComp->ActiveMoveStyle.TurnData,
				MoveStyleComp->ActiveMoveStyle.GaitEntries[MoveStyleComp->ActiveGaitIndex].TransitionData,
				MoveStyleComp->ActiveMoveStyle.GaitEntries[MoveStyleComp->ActiveGaitIndex].LocomotionData, 
				MoveStyleComp->ActiveMoveStyle.StrafeBS, 
				MoveStyleComp->ActiveMoveStyle.NonStrafeBS, 
				SlotIdx);

			ActiveGaitIndex = MoveStyleComp->ActiveGaitIndex;
		}
	}
}

/* rewritten to work better */
void URoNAnimInstance_HumanBase::CalculateStrafeDirection(float DeltaTime, FVector CurVel, FRotator CurActorRotation)
{
	float TargetDirAngle;
	FVector	VelDir = CurVel;
	VelDir.Z = 0.0f;

	if (VelDir.IsNearlyZero())
	{
		TargetDirAngle = 0.f;
	}
	else
	{
		// TODO Optimize: Calculating this again for each AnimNode is inefficient
		VelDir = VelDir.GetSafeNormal();

		FVector LookDir = CurActorRotation.Vector();
		LookDir.Z = 0.f;
		LookDir = LookDir.GetSafeNormal();

		FVector LeftDir = LookDir ^ FVector(0.f, 0.f, 1.f);
		LeftDir = LeftDir.GetSafeNormal();

		float ForwardPct = (LookDir | VelDir);
		float LeftPct = (LeftDir | VelDir);

		TargetDirAngle = FMath::Acos(ForwardPct);
		if (LeftPct > 0.f)
		{
			TargetDirAngle *= -1.f;
		}
	}

	// Move DirAngle towards TargetDirAngle as fast as DirRadsPerSecond allows
	float DeltaDir = FMath::FindDeltaAngleRadians(DirAngle, TargetDirAngle);
	if (DeltaDir != 0.f)
	{
		//FLOAT MaxDelta = DeltaSeconds * DirDegreesPerSecond * (PI / 180.f);
		//DeltaDir = FMath::Clamp<float>(DeltaDir, -MaxDelta, MaxDelta);
		DirAngle = FMath::UnwindRadians(DirAngle + DeltaDir);
	}


	if (DirAngle < -0.875f*PI) // Back
	{
		CurrentStrafeDirection = EStrafeDirection::Type::B;
	}
	else if (DirAngle < -0.625f*PI) // Back-Left
	{
		CurrentStrafeDirection = EStrafeDirection::Type::BL;
	}
	else if (DirAngle < -0.375f*PI) // Left
	{
		CurrentStrafeDirection = EStrafeDirection::Type::L;
	}
	else if (DirAngle < -0.125f*PI) // Forward-Left
	{
		CurrentStrafeDirection = EStrafeDirection::Type::FL;
	}
	else if (DirAngle < 0.125f*PI) // Forward
	{
		CurrentStrafeDirection = EStrafeDirection::Type::F;
	}
	else if (DirAngle < 0.375f*PI) // Forward-Right
	{
		CurrentStrafeDirection = EStrafeDirection::Type::FR;
	}
	else if (DirAngle < 0.625f*PI) // Right
	{
		CurrentStrafeDirection = EStrafeDirection::Type::R;
	}
	else if (DirAngle < 0.875f*PI) // Back-Right
	{
		CurrentStrafeDirection = EStrafeDirection::Type::BR;
	}
	else // Back
	{
		CurrentStrafeDirection = EStrafeDirection::Type::B;
	}
}

void URoNAnimInstance_HumanBase::FillTurnInPlaceAnimSetFromTurnData(FRoNStyleTurnData& TurnData)
{
	TurnInPlaceAnimSet.TurnTransitions[0].Anim = TurnData.Turn90_Left;
	TurnInPlaceAnimSet.TurnTransitions[1].Anim = TurnData.Turn90_Right;
	TurnInPlaceAnimSet.TurnTransitions[2].Anim = TurnData.Turn180_Left;
	TurnInPlaceAnimSet.TurnTransitions[3].Anim = TurnData.Turn180_Right;
}

void URoNAnimInstance_HumanBase::CalcStrafeBSValues(float DeltaTime, ACharacter* CharRef, URoNMoveStyleComponent* MStyleComponent)
{
	if (!CharRef)
		return;

	if (!MStyleComponent)
		return;

	// make sure we have at least a walk and jog gait, otherwise this will not work properly!!!!
	if (MStyleComponent->ActiveMoveStyle.GaitEntries.Num() == 0)
		return;

	if (!MStyleComponent->ActiveMoveStyle.GaitEntries.IsValidIndex(MStyleComponent->ActiveGaitIndex))
		return;

	//int32 JogIndex = 1;
	float WalkingMod;

	/* apply the mod if we are currently idle or walking */
	if (ActiveGaitIndex == 0)
		WalkingMod = 0.5f;
	else if (ActiveGaitIndex == 1)
		WalkingMod = 1.0f;
	else
		WalkingMod = 2.0f;
	
	// valve styled approach in terms of how to deal with strafing.
	float forwardVel = FVector::DotProduct(CharRef->GetCharacterMovement()->Velocity, CharRef->GetActorForwardVector());
	move_x = forwardVel / MStyleComponent->ActiveMoveStyle.GaitEntries[MStyleComponent->ActiveGaitIndex].Speed * WalkingMod;

	float sideVel = FVector::DotProduct(CharRef->GetCharacterMovement()->Velocity, CharRef->GetActorRightVector());
	move_y = sideVel / MStyleComponent->ActiveMoveStyle.GaitEntries[MStyleComponent->ActiveGaitIndex].Speed * WalkingMod;

	//UE_LOG(LogTemp, Warning, TEXT("URoNAnimInstance_HumanBase::CalcNonStrafeBSValues MOVE_X: %f ---- MOVE_Y: %f ----- WALKING MOD: %f"), move_x, move_y, WalkingMod);
}

void URoNAnimInstance_HumanBase::CalcNonStrafeBSValues(float DeltaTime, ACharacter* CharRef, URoNMoveStyleComponent* MStyleComponent)
{
	if (!CharRef)
		return;

	if (!MStyleComponent)
		return;

	// make sure we have at least a walk and jog gait, otherwise this will not work properly!!!!
	if (MStyleComponent->ActiveMoveStyle.GaitEntries.Num() == 0)
		return;

	if (!MStyleComponent->ActiveMoveStyle.GaitEntries.IsValidIndex(MStyleComponent->ActiveGaitIndex))
		return;

	float NormalizedSpeed = CharRef->GetCharacterMovement()->Velocity.Size2D() / MStyleComponent->ActiveMoveStyle.GaitEntries[MStyleComponent->ActiveGaitIndex].Speed;

	if (CharRef->GetCharacterMovement()->Velocity.Size2D() != 0.0f)
	{
		if (MStyleComponent->ActiveMoveStyle.GaitEntries[MStyleComponent->ActiveGaitIndex].Name == "walk")
			move_x = NormalizedSpeed * 0.5f;
		else if (MStyleComponent->ActiveMoveStyle.GaitEntries[MStyleComponent->ActiveGaitIndex].Name == "jog")
			move_x = NormalizedSpeed;
		else if (MStyleComponent->ActiveMoveStyle.GaitEntries[MStyleComponent->ActiveGaitIndex].Name == "sprint")
			move_x = NormalizedSpeed * 2.0f;
	}
	else
	{
		move_x = 0.0f;
	}

	//UE_LOG(LogTemp, Warning, TEXT("URoNAnimInstance_HumanBase::CalcNonStrafeBSValues MOVE_X: %f"), move_x);
}

void URoNAnimInstance_HumanBase::TriggerMoveStyleChangeStatus()
{
	//UE_LOG(LogReadyOrNot, Display, TEXT("URoNAnimInstance_HumanBase: Move Style Blend started!"));
	MoveStyleBlendResetCounter = 0.0f;
	bMoveStyleChanging = true;
}

void URoNAnimInstance_HumanBase::ResetMoveStyleChangeStatus()
{
	bMoveStyleChanging = false;
	//UE_LOG(LogReadyOrNot, Display, TEXT("URoNAnimInstance_HumanBase: Move Style Blend completed!"));
}
