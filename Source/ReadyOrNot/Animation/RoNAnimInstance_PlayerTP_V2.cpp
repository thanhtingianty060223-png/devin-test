// Copyright Void Interactive, 2023
// Author: Alexander Mijalkovski

// Improved foundation
//
// Changes from Original:
// 100% Nativized Logic
// Reworked Clean Distance Matching
// Remove unnessary duplicate locomotion statemachine
// Native state transition handling and updating
// big TODO: multi-threading

#include "Animation/RoNAnimInstance_PlayerTP_V2.h"

// ##UE5UPGRADE## Duplicated in Engine
//#include "CachedAnimDataLibrary.h"
#include "Engine/Public/Animation/CachedAnimDataLibrary.h"
#include "Characters/CyberneticCharacter.h"
#include "Characters/AI/SWATCharacter.h"
#include "Actors/Items/BallisticsShield.h"
#include "Components/ReadyOrNotCharMovementComp.h"
#include "Actors/Items/Optiwand.h"
#include "Animation/StrideWarping/StrideWarpingLibrary.h"
#include "Characters/CyberneticController.h"
#include "Info/Activities/Team/DoorInteractionActivity.h"
#include "Info/Activities/Team/TeamBreachAndClearActivity.h"
#include "Info/Activities/Team/TeamStackUpActivity.h"
#include "TurnInPlace/AnimTurnInPlaceLibrary.h"

#include "NavigationSystem.h"
#include "ReadyOrNotAISystem.h"

TAutoConsoleVariable<int32> CVar_RonAnimInstance_PlayerTP_V2_EnableDebug(TEXT("a.RonAnimInstance_PlayerTP_V2.EnableDebug"), 0, TEXT("Toggle Debug Information for Ron Anim Instance Player TP."));
TAutoConsoleVariable<int32> CVar_RonAnimInstance_PlayerTP_V2_EnableDistanceMatching(TEXT("a.RonAnimInstance_PlayerTP_V2.EnableDistanceMatching"), 1, TEXT("Enable Distance Matching for Player Graphs."));

URoNAnimInstance_PlayerTP_V2::URoNAnimInstance_PlayerTP_V2(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	LoweredCooldownTime = 0.8f;
	PlayrateClampMax = 0.15f;

	DistanceMatchingCurrentState = EDistanceMatchingType::None;
	PostPivotTriggerThreshold = 1.0f;

	PelvisDefaultWorldPos = FVector(0.294243f, -1.095897f, 91.050880f);
	CrouchedPelvisDefaultWorldPos = FVector(2.459475f, -14.275543f, 43.456177f);
	CrouchedPelvisMovingWorldPos = FVector(-0.602592f, -1.029617f, 75.528893f);
}

void URoNAnimInstance_PlayerTP_V2::NativeInitializeAnimation()
{
	AReadyOrNotCharacter* BaseCharacter = Cast<AReadyOrNotCharacter>(TryGetPawnOwner());
	if (BaseCharacter)
	{
		BaseCharacterRef = BaseCharacter;
	}

	APlayerCharacter* OwningCharacter = Cast<APlayerCharacter>(TryGetPawnOwner());
	if (OwningCharacter)
	{
		CharacterRef = OwningCharacter;
		LastActorLocation = OwningCharacter->GetActorLocation();
	}

	ACyberneticCharacter* CurAiChar = Cast<ACyberneticCharacter>(TryGetPawnOwner());
	if (CurAiChar)
	{
		CharacterAiRef = CurAiChar;
		LastActorLocation = CurAiChar->GetActorLocation();
		bIsAIControlled = true;
	}
	else
	{
		bIsAIControlled = false;
	}

	DefineStateMachineData();
	AddNativeStateMachineBindings();
}

// main func for tick calculations
void URoNAnimInstance_PlayerTP_V2::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// debug if requested
	bShowDebug = (CVar_RonAnimInstance_PlayerTP_V2_EnableDebug.GetValueOnAnyThread() == 1);

	AReadyOrNotCharacter* CurChar = Cast<AReadyOrNotCharacter>(TryGetAnyOwner());

	if (CurChar == nullptr)
	{
		return;
	}

	if (!GetOwningComponent()->SkeletalMesh)
	{
		return;
	}

	APlayerCharacter* CurPlayerChar = Cast<APlayerCharacter>(CurChar);

	// we use the suspect base class, this should be renamed to reflect a unified AI class
	ACyberneticCharacter* AICharacter = Cast<ACyberneticCharacter>(TryGetPawnOwner());

	/*
	LOCAL_PLAYER;
	if (LocalPlayer)
	{
		LeanAngleY = LocalPlayer->FreeLeanX + LocalPlayer->QuickLeanAmount;
		LeanAngleZ = LocalPlayer->FreeLeanZ;
		bIsAiming = LocalPlayer->bAiming && !LocalPlayer->IsReloading();
		bLeanLeft = LocalPlayer->bLeaningLeft;
		bLeanRight = LocalPlayer->bLeaningRight;
		QuickLeanLeftAmount = FMath::Abs(FMath::Clamp(LocalPlayer->QuickLeanAmount, -1.0f, 0.0f));
		QuickLeanRightAmount = FMath::Clamp(LocalPlayer->QuickLeanAmount, 0.0f, 1.0f);
	}
	*/

	if (CurChar)
	{
		LeanAngleY = CurChar->FreeLeanX + CurChar->QuickLeanAmount;
		LeanAngleZ = CurChar->FreeLeanZ;
		bIsAiming = CurChar->bAiming && !CurChar->IsReloading();
		bLeanLeft = CurChar->bLeaningLeft;
		bLeanRight = CurChar->bLeaningRight;
		QuickLeanLeftAmount = FMath::Abs(FMath::Clamp(CurChar->QuickLeanAmount, -1.0f, 0.0f));
		QuickLeanRightAmount = FMath::Clamp(CurChar->QuickLeanAmount, 0.0f, 1.0f);
		// weapon down
		bWeaponDown = CurChar->IsLowReady();
		QuickLeanIntensity = CurChar->QuickLeanIntensity;
		QuickLeanInterpSpeed = CurChar->QuickLeanInterpSpeed;

		// extra check to switch to up pose when needed
		IsLoweredUp = CurChar->IsLowReadyPointUp();
		

		if (bIsDeployableEquipped)
			bWeaponDown = false;
		if (bIsC2Charge)
			bWeaponDown = true;
	}

	// TODO: temp. will remove later
	StandRifAnimSet.TurnDeadZoneAngle = 0.0f;
	CrouchRifAnimSet.TurnDeadZoneAngle = 0.0f;

	AimingAlpha = bIsAiming ? UKismetMathLibrary::FInterpTo_Constant(AimingAlpha, 1.0f, DeltaSeconds, 9.0f) : UKismetMathLibrary::FInterpTo_Constant(AimingAlpha, 0.0f, DeltaSeconds, 9.0f);
	QuickLeanLeftAlpha = UKismetMathLibrary::FInterpTo(QuickLeanLeftAlpha, QuickLeanLeftAmount * QuickLeanIntensity, DeltaSeconds, QuickLeanInterpSpeed);
	QuickLeanRightAlpha = UKismetMathLibrary::FInterpTo(QuickLeanRightAlpha, QuickLeanRightAmount * QuickLeanIntensity, DeltaSeconds, QuickLeanInterpSpeed);
	//FootIKValue = CurChar->bIsCrouched || CurChar->IsPlayingRootMotionFromMontage() || CurChar->IsLocalPlayer() ? 0.0f : 1.0f;
	//FootIKAlpha = UKismetMathLibrary::FInterpTo(FootIKAlpha, FootIKValue, DeltaSeconds, 8.0f);
	bArrested = /*CurPlayerChar->bIsBeingArrested ||*/ CurChar->IsArrested();
	bIsDead = CurChar->IsDeadOrUnconscious();
	bIsCarrying = CurChar->IsCarrying();
	bIsCarried = CurChar->IsCarried();
	
	bStunned = false;
// !CurChar->IsBeingArrested() &&
// 				(CurChar->IsStunnedWith(EStunType::ST_Gassed) ||
// 				CurChar->IsStunnedWith(EStunType::ST_Flash) ||
// 				CurChar->IsStunnedWith(EStunType::ST_Stung) ||
// 				CurChar->IsStunnedWith(EStunType::ST_Peppersprayed));
// 	
	bTased = !CurChar->IsBeingArrested() && CurChar->IsStunnedWith(EStunType::ST_Tased);
	bIsShieldEquipped = Cast<class ABallisticsShield>(CurChar->GetEquippedItem()) ? true : false;
	bIsInCombatOrAlerted = bIsAlerted || bIsInCombat;
	bMoving = Speed != 0.0f;

	if (bOnLadder)
	{
		CurChar->GetCharacterMovement()->MaxWalkSpeed = GetCurveValue("SpeedUp") * 0.3f;
	}

	const bool bDisableTIP = CurChar->bDisableTurnInPlace;

	/*
	if (bIsDead)
	{
		if (!IsPlayingSlotAnimation(nullptr, "DeathSlot") && !bDoOnceOnDeath)
		{
			bDoOnceOnDeath = true;
			SavePoseSnapshot("DeathPose");
		}
	}
	*/

	if (!bUseDistanceMatching)
	{
		// turn in place related
		if (FMath::Abs(CurChar->GetVelocity().Size2D()) > 75.0f ||
			CurChar->IsDeadOrUnconscious() || CurChar->IsIncapacitated() ||
			CurChar->IsArrested() ||
			bIsPlayingDeathAnim ||
			bDisableTIP ||
			CurChar->IsAny3PMontageActive())
		{
			bAllowTurnInPlace = false;
		}
		else
		{
			bAllowTurnInPlace = true;
		}
	}

	if (CurChar->IsCrouching())
	{
		TurnInPlaceAnimSet = CrouchRifAnimSet;
	}
	else
	{
		TurnInPlaceAnimSet = StandRifAnimSet;
	}
	
	// update state machine data
	UpdateStateMachineData(DeltaSeconds);

	// update turn in place
	bAllowTurnInPlace = (bIsIdleStateRelevant || bIsStopStateRelevant) ? true : false;
	if (bIsLocallyFirstPerson)
	{
		bAllowTurnInPlace = FMath::Abs(CurChar->GetVelocity().Size2D()) > 0.0f ? false : true;
	}
	const bool bHoldRootYawOffset = (bIsIdleStateRelevant || bIsStartStateRelevant) ? true : false;

	UAnimTurnInPlaceLibrary::UpdateTurnInPlace(DeltaSeconds, bAllowTurnInPlace, bHoldRootYawOffset, bIsTurnInPlaceStateRelevant, true, 145.0f, CurChar->GetMesh()->GetComponentRotation(), TurnInPlaceAnimSet, TurnInPlaceState, TurnInPlaceSpeedMultiplier);
	
	ACyberneticCharacter* CurAiChar = Cast<ACyberneticCharacter>(TryGetPawnOwner());
	if (CurAiChar)
	{
		bIsPlayingDeathAnim = CurAiChar->bStartedPlayingDeath || CurAiChar->bPlayingDeathMontage;
		bRagdoll = CurAiChar->IsInRagdoll() && !bIsPlayingDeathAnim;
		bIsAiming = CurAiChar->IsStrafing();
		bWeaponDown = CurAiChar->IsLowReady();
		AlertAlpha = UKismetMathLibrary::FInterpTo(AlertAlpha, bIsAlerted ? 1.0f : 0.0f, DeltaSeconds, 7.0f);
		bIsSurrendering = CurAiChar->IsSurrendered();
		if (CurAiChar->IsStunned())
		{
			bTased = CurAiChar->IsCurrentlyTased();

			bStung = CurAiChar->IsCurrentlyStung();
		}
		else
		{
			bTased = false;
			bSprayed = false;
			bStung = false;
		}
		bFemale = CurAiChar->bFemale;
		bChild = CurAiChar->bChild;
		CurPseudoSpeed = EPseudoSpeedType::Walk; // Note: Pseudo speed deprecated. Default to walk

		const FRotator TargetRotation = CurAiChar->GetLookAtRotation(35.0f, 30.0f);
		HeadLookRotation = UKismetMathLibrary::RInterpTo(HeadLookRotation, TargetRotation, DeltaSeconds, 1.5f);

		if (const ACyberneticController* CurController = CurAiChar->GetCyberneticsController())
		{
			if (UBaseActivity* Activity = CurController->GetCurrentActivity())
			{
				bool bIsBreaching = false;
				if (const UTeamBreachAndClearActivity* BreachAndClearActivity = Cast<UTeamBreachAndClearActivity>(Activity))
				{
					bIsBreaching = BreachAndClearActivity->GetActiveStateID() >= 4;
				}
				
				if (Cast<UDoorInteractionActivity>(Activity) || bIsBreaching)
					bAllowTurnInPlace = false;
			}
		}

		if (bAllowTurnInPlace)
		{
			if (CurAiChar->bDisableTurnInPlace || CurAiChar->IsAny3PMontageActive() /* || FMath::Abs(CurChar->GetVelocity().Size2D()) > 75.0f*/)
				bAllowTurnInPlace = false;
		}
	}
	else
	{
		HeadLookRotation = UKismetMathLibrary::RInterpTo(HeadLookRotation, GetLookAtRotation(), DeltaSeconds, 1.5f);
	}

	/* ======================*/
	/* ======================*/
	/* player snyc speed     */
	/* ======================*/
	/* ======================*/

	/* 04.05.22 Alex: Implemented support for stride/speed warping */
	const float CurrentSpeedCurveValue = GetCurveValue("speed");
	UStrideWarpingLibrary::UpdateStrideWarping(DeltaSeconds, CurChar->GetVelocity(), VelocityInterpTime, CurrentSpeedCurveValue, PlayrateClampMax, SpeedScaling, AnimSpeedPlayrateSync, VelocitySmoothed);
	AnimSpeedFwdPlayrateSync = AnimSpeedPlayrateSync;
	AnimSpeedSidePlayrateSync = AnimSpeedPlayrateSync;

	// Fast Walk Switches
	SpeedSelection = bCrouching ? DefaultCrouchSpeed : DefaultStandSpeed;
	// simplify speed selection
	bIsFastWalking = CharMovementSnapshot.Speed2D >= SpeedSelection;

	// see if we need to use distance matching
	
	//bUseDistanceMatching = CharacterRef ? CVar_RonAnimInstance_PlayerTP_EnableDistanceMatching.GetValueOnAnyThread() == 1 && !(CharacterRef->IsLocallyControlled() && CharacterRef->GetFirstPersonCameraComponent()) : CVar_RonAnimInstance_PlayerTP_EnableDistanceMatching.GetValueOnAnyThread() == 1;
	bUseDistanceMatching = CVar_RonAnimInstance_PlayerTP_V2_EnableDistanceMatching.GetValueOnAnyThread() == 1;

	if(bUseDistanceMatching)
	{
		if (CharacterRef)
		{	
			// needed for replicated acceleration!
			UReadyOrNotCharMovementComp* RonMovementComponent = Cast<UReadyOrNotCharMovementComp>(CharacterRef->GetCharacterMovement());
			ReplicatedMaxSpeed = CharacterRef->ReplicatedMaxSpeed;

			/* distance matching is not applied to locally controlled character due to messing up firstperson view too much */
			bIsLocallyFirstPerson = (CharacterRef->IsLocallyControlled() && CharacterRef->GetFirstPersonCameraComponent()) ? true : false;

			if (RonMovementComponent)
			{
				/* distance matching! */
				UAnimCharacterMovementLibrary::UpdateCharacterMovementSnapshot(CharacterRef->GetActorTransform(), CharacterRef->GetVelocity(), RonMovementComponent->GetCurrentAcceleration(), CharacterRef->GetCharacterMovement()->IsMovingOnGround(), 0.0f, CharMovementSnapshot);
				CalculateDistanceMatching(DeltaSeconds, CharacterRef);
			}

			/* */

		}

		if (CharacterAiRef)
		{
			// needed for replicated acceleration!
			UReadyOrNotCharMovementComp* RonMovementComponent = Cast<UReadyOrNotCharMovementComp>(CharacterAiRef->GetCharacterMovement());
			ReplicatedMaxSpeed = CharacterAiRef->ReplicatedMaxSpeed; // temp workaround for AI

			if (RonMovementComponent)
			{
				/* distance matching! */
				UAnimCharacterMovementLibrary::UpdateCharacterMovementSnapshot(CharacterAiRef->GetActorTransform(), CharacterAiRef->GetVelocity(), RonMovementComponent->GetCurrentAcceleration(), CharacterAiRef->GetCharacterMovement()->IsMovingOnGround(), 0.0f, CharMovementSnapshot);
				CalculateDistanceMatching(DeltaSeconds, CharacterAiRef);
			}

			/* */
		}
	}


	/* ======================*/
	/* ======================*/
	/* ======================*/
	/* generic */
	/* ======================*/
	/* ======================*/

	// post processed aim offset after turn in place calculation

	// post process aimoffsets
	/* 06.01.2020 Alex FIX: don't post process while moving, save computation */
	//if (Speed == 0.0f && !bRotationRateReached)
		//PostAimProcessing(AimOffsets, DeltaSeconds);

	Speed_tp_rifle_stand_sprint_f = Speed / 441.0f;
	Crouch_Idle_Pose_Low_TP = GetPlayerAnimation_TP(EBaseAnimType_TP::Crouch_IdlePose_Low_TP);
	Crouch_Idle_Pose_Up_TP = GetPlayerAnimation_TP(EBaseAnimType_TP::Crouch_IdlePose_Up_TP);
	Crouch_Idle_Pose_Shld_TP = GetPlayerAnimation_TP(EBaseAnimType_TP::Crouch_IdlePose_Shld_TP);
	Crouch_Idle_Pose_Sights_TP = GetPlayerAnimation_TP(EBaseAnimType_TP::Crouch_IdlePose_Sights_TP);
	Crouch_Idle_Pose_Ret_TP = GetPlayerAnimation_TP(EBaseAnimType_TP::Crouch_IdlePose_Ret_TP);
	Crouch_Idle_Pose_Ovr_TP = GetPlayerAnimation_TP(EBaseAnimType_TP::Crouch_IdlePose_Ovr_TP);
	Idle_Pose_Low_TP = GetPlayerAnimation_TP(EBaseAnimType_TP::IdlePose_Low_TP);
	Idle_Pose_Up_TP = GetPlayerAnimation_TP(EBaseAnimType_TP::IdlePose_Up_TP);
	Idle_Pose_Shld_TP = GetPlayerAnimation_TP(EBaseAnimType_TP::IdlePose_Shld_TP);
	Idle_Pose_Sights_TP = GetPlayerAnimation_TP(EBaseAnimType_TP::IdlePose_Sights_TP);
	Idle_Pose_Ret_TP = GetPlayerAnimation_TP(EBaseAnimType_TP::IdlePose_Ret_TP);
	Idle_Pose_Ovr_TP = GetPlayerAnimation_TP(EBaseAnimType_TP::IdlePose_Ovr_TP);

	Idle_Pose_VFG_TP = GetPlayerAnimation_TP(EBaseAnimType_TP::IdlePose_VFG_TP);
	Idle_Pose_AFG_TP = GetPlayerAnimation_TP(EBaseAnimType_TP::IdlePose_AFG_TP);
	Idle_Pose_HSTOP_TP = GetPlayerAnimation_TP(EBaseAnimType_TP::IdlePose_HSTOP_TP);

	bLeaningLeftNotCrouching = bLeanLeft && !bCrouching;
	bLeaningRightNotCrouching = bLeanRight && !bCrouching;
	bNotLeaningLeftOrCrouching = !bLeanLeft || bCrouching;
	bNotLeaningRightOrCrouching = !bLeanRight || bCrouching;
	bNotLeaningLeftOrNotCrouching = !bLeanLeft || !bCrouching;
	bNotLeaningRightOrNotCrouching = !bLeanRight || !bCrouching;
	bCrouchingAndMoving = bCrouching && Speed != 0.0f;
	bNotCrouchingAndMoving = !bCrouching && Speed != 0.0f;
	bAimingAndNotDeployable = bIsAiming && !bIsDeployableEquipped;

	// todo expose to design?

	bJumpStartTrigger = (bIsFalling && !bHasPrelanded) ? true : false;

	/* lean mapping */
	SmoothMappedLeanToAnimStandLeft = FMath::FInterpTo(SmoothMappedLeanToAnimStandLeft, FMath::GetMappedRangeValueClamped(FVector2D(0.0f, 40.0f), FVector2D(0.0f, 1.22f), FMath::Abs(LeanAngleY)), DeltaSeconds, 8.0f);
	SmoothMappedLeanToAnimStandRight = FMath::FInterpTo(SmoothMappedLeanToAnimStandRight, FMath::GetMappedRangeValueClamped(FVector2D(0.0f, 40.0f), FVector2D(0.0f, 1.1f), FMath::Abs(LeanAngleY)), DeltaSeconds, 8.0f);
	SmoothMappedLeanToAnimCrouchLeft = FMath::FInterpTo(SmoothMappedLeanToAnimCrouchLeft, FMath::GetMappedRangeValueClamped(FVector2D(0.0f, 40.0f), FVector2D(0.0f, 1.5f), FMath::Abs(LeanAngleY)), DeltaSeconds, 8.0f);
	SmoothMappedLeanToAnimCrouchRight = FMath::FInterpTo(SmoothMappedLeanToAnimCrouchRight, FMath::GetMappedRangeValueClamped(FVector2D(0.0f, 40.0f), FVector2D(0.0f, 1.4f), FMath::Abs(LeanAngleY)), DeltaSeconds, 8.0f);



	/* ======================*/
	/* ======================*/
	/* ======================*/
	/* aimoffset alpha */
	/* ======================*/
	/* ======================*/
	/* ======================*/

	ABaseItem* CurItem = CurChar ? Cast<ABaseItem>(CurChar->GetEquippedItem()) : nullptr;

	if (CurItem && CurItem->AnimationData)
	{
		LastAnimWeaponData = CurItem->AnimationData;
	}

	//if (!AICharacter)
	{
		/* shield for now always keeps aim offset alpha */
		if (bIsShieldEquipped)
		{
			AimOffsetAlpha = FMath::FInterpTo(AimOffsetAlpha, 1.0f, DeltaSeconds, 8.0f);
		}
		else
		{
			/* alex 12.08.22 fix this not working in recent builds! */
			if (GetCurrentActiveMontage())
			{
				/* only exclude fire motions from blending out aim offset at the moment */
				if (!GetCurrentActiveMontage()->GetName().Contains("fire"))
				{
					AimOffsetAlpha = FMath::FInterpTo(AimOffsetAlpha, 0.3f, DeltaSeconds, 8.0f); // alex 12.08.22 use really low value to indicate aiming to some degree
				}
				else
				{
					AimOffsetAlpha = FMath::FInterpTo(AimOffsetAlpha, 1.0f, DeltaSeconds, 8.0f);
				}
			}
			else
			{
				AimOffsetAlpha = FMath::FInterpTo(AimOffsetAlpha, 1.0f, DeltaSeconds, 8.0f);
			}
		}
	}


	/* ======================*/
	/* ======================*/
	/* ======================*/
	/* additive layer logic */
	/* ======================*/
	/* ======================*/
	/* ======================*/

	/* don't run this on AI */
	//if (!AICharacter)
	{
		// check if we have valid assets before we bother blending
		if (LastAnimWeaponData)
		{
			if (LastAnimWeaponData->bHasRetentionAdditives)
			{
				/* Retention Alpha, If we are not playing a Montage and Close to wall trigger retention alpha! */
				if (!CurChar->Is3PMontagePlaying(nullptr) && bWeaponDown /*&& !bIsAiming*/)
				{
					RetentionAlpha = FMath::FInterpTo(RetentionAlpha, 1.0f, DeltaSeconds, 8.0f);
				}
				else
				{
					RetentionAlpha = FMath::FInterpTo(RetentionAlpha, 0.0f, DeltaSeconds, 8.0f);
				}
			}
			else
			{
				RetentionAlpha = 0.0f;
			}

			if (LastAnimWeaponData->bHasSightAdditives)
			{
				bool MontageRuleSights = false;

				if (GetCurrentActiveMontage())
					MontageRuleSights = !GetCurrentActiveMontage()->GetName().Contains("fire");

				/* Sight Alpha, If we are aiming down the sights and we are not playing a montage, trigger aiming alpha!*/
				if (!MontageRuleSights && bIsAiming && !bWeaponDown)
				{
					SightAlpha = FMath::FInterpTo(SightAlpha, 1.0f, DeltaSeconds, 6.0f);
				}
				else
				{
					SightAlpha = FMath::FInterpTo(SightAlpha, 0.0f, DeltaSeconds, 6.0f);
				}
			}
			else
			{
				SightAlpha = FMath::FInterpTo(SightAlpha, 0.0f, DeltaSeconds, 6.0f);
			}

			const bool bIsSprinting = CurPlayerChar && CurPlayerChar->IsSprinting();

			if (LastAnimWeaponData->bHasLoweredAdditives && !IsLoweredUp)
			{
				const bool bViewBlocking = Cast<ASWATCharacter>(CurChar) ? Cast<ASWATCharacter>(CurChar)->bViewBlockedByOtherSwat : false;

				// Lowered Alpha, if we have a active montage or in sights or weapon close to walls, trigger lowered alpha!
				if ((CurChar->IsAny3PMontageActive() ||
					bIsAiming ||
					bWeaponDown ||
					bIsSprinting) && !CurChar->IsLowReady())
				{
					if (bIsLoweredActive)
					{
						if (Cast<ASWATCharacter>(CurChar))
						{
							if (!UReadyOrNotFunctionLibrary::IsCallbackTimerActive(this, LoweredCooldownHandle))
							{
								FTimerDelegate D;
								D.BindWeakLambda(this, [&]()
								{
									LoweredInternalVal = 0;
								});

								UReadyOrNotFunctionLibrary::StartTimerForCallback(LoweredCooldownHandle, this, D, 0.5f, false);
							
								bIsLoweredActive = false;
							}
						}
						else
						{
							LoweredInternalVal = 0;
							bIsLoweredActive = false;
						}
					}
				}
				else
				{
					// run timer here
					if (!bIsLoweredActive)
					{
						if (Cast<ASWATCharacter>(CurChar))
						{
							if (!UReadyOrNotFunctionLibrary::IsCallbackTimerActive(this, LoweredCooldownHandle))
							{
								FTimerDelegate D;
								D.BindWeakLambda(this, [&]()
								{
									LoweredInternalVal = 1;
								});
								
								UReadyOrNotFunctionLibrary::StartTimerForCallback(LoweredCooldownHandle, this, D, 0.1f, false);
								
								bIsLoweredActive = true;
							}
						}
						else
						{
							LoweredInternalVal = 0;
							bIsLoweredActive = false;
						}
					}
				}

				LoweredAlpha = FMath::FInterpTo(LoweredAlpha, LoweredInternalVal, DeltaSeconds, 4.0f);
			}
			else
			{
				
				LoweredAlpha = FMath::FInterpTo(LoweredAlpha, 0.0f, DeltaSeconds, 4.0f);
			}
		}
	}
	/*
	else
	{
		if (LastAnimWeaponData && LastAnimWeaponData->bHasLoweredAdditives)
		{
			// Lowered Alpha, if we have a active montage or in sights or weapon close to walls, trigger lowered alpha!
			if (((CurChar->Is3PMontagePlaying(nullptr) && !CurChar->IsTableMontagePlaying("tp_swat_gestures")) || bIsAiming || (CurPlayerChar && CurPlayerChar->IsSprinting())) && !CurChar->IsLowReady())
			{
				LoweredInternalVal = 0;
				bIsLoweredActive = false;
			}
			else
			{
				// run timer here
				if (!bIsLoweredActive)
				{
					CurChar->GetWorldTimerManager().SetTimer(LoweredCooldownHandle, this, &URoNAnimInstance_PlayerTP::LoweredCooldownDone, LoweredCooldownTime, false);

					bIsLoweredActive = true;
				}
			}

			LoweredAlpha = FMath::FInterpTo(LoweredAlpha, LoweredInternalVal, DeltaSeconds, 8.0f);
		}
		else
		{
			LoweredAlpha = 0.0f;
		}
	}
	*/
	
	if (const ACyberneticCharacter* CyberneticCharacter = Cast<ACyberneticCharacter>(CurChar))
	{
		AimOffsets = FMath::Vector2DInterpTo(AimOffsets, FVector2D(bAllowTurnInPlace ? TurnInPlaceState.RootYawOffset : CyberneticCharacter->AimOffset.X, CyberneticCharacter->AimOffset.Y), DeltaSeconds, CyberneticCharacter->AimOffsetInterpSpeed);
		//AimOffsetAlpha = 1.0f;
	}
	else
	{
		// aimoffsets
		bool bShouldUpdateAimOffset = true;
		AOptiwand* Optiwand = Cast<AOptiwand>(CurChar->GetEquippedItem());
		if (Optiwand && Optiwand->IsMirroring())
		{
			bShouldUpdateAimOffset = false;
		}

		if (bShouldUpdateAimOffset)
		{
			AimOffsets = FMath::Vector2DInterpTo(AimOffsets, FVector2D(TurnInPlaceState.RootYawOffset, UKismetMathLibrary::NormalizedDeltaRotator(CurChar->GetBaseAimRotation(), CurChar->GetActorRotation()).Pitch), DeltaSeconds, 8.0f);
			//AimOffsets = FVector2D(TurnInPlaceState.RootYawOffset, UKismetMathLibrary::NormalizedDeltaRotator(CurChar->GetBaseAimRotation(), CurChar->GetActorRotation()).Pitch);
		}
	}

	// needed for IK!
	ArmsOnlySlotAlpha = (GetSlotMontageGlobalWeight("ArmsOnly") == 1.0f) ? FMath::FInterpTo(ArmsOnlySlotAlpha, 0.0f, 12.0f, DeltaSeconds) : FMath::FInterpTo(ArmsOnlySlotAlpha, 1.0f, 12.0f, DeltaSeconds);
	LeftArmOnlySlotAlpha = (GetSlotMontageGlobalWeight("LeftArmOnly") == 1.0f) ? FMath::FInterpTo(LeftArmOnlySlotAlpha, 0.0f, 12.0f, DeltaSeconds) : FMath::FInterpTo(LeftArmOnlySlotAlpha, 1.0f, 12.0f, DeltaSeconds);

	/* needed for fp bob damping */
	CrouchedPelvisCurrentWorldPos = FMath::VInterpTo(CrouchedPelvisCurrentWorldPos, bIsMoving ? CrouchedPelvisMovingWorldPos : CrouchedPelvisDefaultWorldPos, DeltaSeconds, 8.0f);

	if (CurPlayerChar)
	{
		FootIKAlpha = (CurPlayerChar->IsLocallyControlled() && CurPlayerChar->GetFirstPersonCameraComponent()) ? FMath::FInterpTo(FootIKAlpha, 0.0f, DeltaSeconds, 8.0f) : FMath::FInterpTo(FootIKAlpha, 1.0f, DeltaSeconds, 8.0f);
	}

	// Lean Values
	CrouchHighPoseAdditiveAlpha = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, 40.0f), FVector2D(0.0f, 1.0f), LeanAngleZ) - FMath::GetMappedRangeValueClamped(FVector2D(0.0f, 40.0f), FVector2D(0.0f, 1.0f), FMath::Abs(LeanAngleY));
	StandHighPoseAdditiveAlpha = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, 40.0f), FVector2D(0.0f, 1.0f), LeanAngleZ) - FMath::GetMappedRangeValueClamped(FVector2D(0.0f, 40.0f), FVector2D(0.0f, 1.0f), FMath::Abs(LeanAngleY));
	ZHeightLeanAdjustment = FVector(0.0f, 0.0f, FMath::Clamp(LeanAngleZ * 0.6f, -30.0f, 2.0f));

	// Motion blocks
	bIsItemOrPistolMotionBlock = (bIsItem || bIsPistol) ? true : false;
	bIsRifleMotionBlock = (CurMotionBlock == EMotionBlockType::MB_Rifle) ? true : false;
	bIsCrouchingWithShield = (bIsShieldEquipped && bCrouching) ? true : false;
	IsCrouchingWithShield_AsFloat = bIsCrouchingWithShield ? 1.0f : 0.0f;

	// Left Hand IK Logic
	LeftHandIKAlpha_ItemBased = LeftHandIKAlpha * (bIsShieldEquipped ? 0.0f : 1.0f);
	LeftHandTempFinIK = LeftHandIKAlpha_ItemBased * HandAdditiveLockOverride;

	// Underbarrel Type
	UpperbodySlotNoWeight = (GetWeightFromSlot("Upperbody") != 0.0f) ? 1.0f : 0.0f;

	ABaseWeapon* BaseWeapon = Cast<ABaseWeapon>(BaseCharacterRef->GetEquippedItem());
	if (!BaseWeapon)
	{
		bHasUnderbarrelAttachment = false;
	}

	if (BaseWeapon)
	{
		bHasUnderbarrelAttachment = (BaseWeapon->GetUnderbarrelAnimationType() != EWeaponUnderbarrelAnimationType::WU_None) ? true : false;
		UnderbarrelType = BaseWeapon->GetUnderbarrelAnimationType();

		if (bHasUnderbarrelAttachment)
		{
			UpperbodySlotNoWeight = GetWeightFromSlot("Upperbody") != 0.0f ? 1.0f : 0.0f;

			UAnimMontage* ActiveMontage = GetCurrentActiveMontage();

			if (ActiveMontage)
			{
				/* Hack to fix Curve Issues */
				// ##UE5UPGRADE## Compatibility
				if ( ( Montage_GetPosition(ActiveMontage) <= (ActiveMontage->GetPlayLength() - 0.1f) ) && IsAnyMontagePlaying() )
				{
					LeftHandGripAlpha = GetCurveValue("Hand_IK_Left");
				}
				else
				{
					LeftHandGripAlpha = FMath::FInterpTo(LeftHandGripAlpha, 1.0f, DeltaSeconds, 10.0f);
				}
			}
			else
			{
				LeftHandGripAlpha = FMath::FInterpTo(LeftHandGripAlpha, 1.0f, DeltaSeconds, 10.0f);
			}
		}
	}

	if (!bIsDead && !bDeathAnimEnd)
	{
		bHasCapturedDeathPose = false;
	}

	// Store Death Pose!
	if (bDeathAnimEnd && !bHasCapturedDeathPose)
	{
		SnapshotPose(DeathPose);
		bHasCapturedDeathPose = true;
	}

	// Hand IK and Additive Override through Curve
	if (BaseCharacterRef->IsTableMontagePlaying("tp_swat_gestures"))
	{
		HandAdditiveLockOverride = GetCurveValue("Hand_IK_Left") + 1.0f;
		LeftHandAdditiveOvrBlend = FMath::GetMappedRangeValueClamped(FVector2D(1.0f, 0.0f), FVector2D(0.0f, 1.0f), HandAdditiveLockOverride);
	}
	else
	{
		HandAdditiveLockOverride = 1.0f;
		LeftHandAdditiveOvrBlend = 0.0f;
	}
}

UAnimSequenceBase * URoNAnimInstance_PlayerTP_V2::GetPlayerAnimation_TP(EBaseAnimType_TP::Entries AnimName) const
{
	UAnimSequenceBase * AnimSequence = nullptr;

	// use editor preview data if no data found in item
	UReadyOrNotWeaponAnimData* WeaponAnimData = LastAnimWeaponData ? LastAnimWeaponData : EditorWeaponAnimData;

	// if we have valid anim data proceed to find the requested motion
	if (WeaponAnimData)
	{
		switch (AnimName)
		{
		case EBaseAnimType_TP::IdlePose_Low_TP: AnimSequence = WeaponAnimData->IdlePose_Low_TP;
			break;

		// new
		case EBaseAnimType_TP::IdlePose_Up_TP: AnimSequence = WeaponAnimData->IdlePose_Up_TP;
			break;

		case EBaseAnimType_TP::IdlePose_Shld_TP: AnimSequence = WeaponAnimData->IdlePose_Shld_TP;
			break;

		case EBaseAnimType_TP::IdlePose_Sights_TP: AnimSequence = WeaponAnimData->IdlePose_Sights_TP;
			break;

		// added
		case EBaseAnimType_TP::IdlePose_Ret_TP: AnimSequence = WeaponAnimData->IdlePose_Ret_TP;
			break;

		case EBaseAnimType_TP::IdlePose_Ovr_TP: AnimSequence = WeaponAnimData->IdlePose_Ovr_TP;
			break;

		case EBaseAnimType_TP::Crouch_IdlePose_Low_TP: AnimSequence = WeaponAnimData->Crouch_IdlePose_Low_TP;
			break;

		// new
		case EBaseAnimType_TP::Crouch_IdlePose_Up_TP: AnimSequence = WeaponAnimData->Crouch_IdlePose_Up_TP;
			break;

		case EBaseAnimType_TP::Crouch_IdlePose_Shld_TP: AnimSequence = WeaponAnimData->Crouch_IdlePose_Shld_TP;
			break;

		case EBaseAnimType_TP::Crouch_IdlePose_Sights_TP: AnimSequence = WeaponAnimData->Crouch_IdlePose_Sights_TP;
			break;

		// added
		case EBaseAnimType_TP::Crouch_IdlePose_Ret_TP: AnimSequence = WeaponAnimData->Crouch_IdlePose_Ret_TP;
			break;

		case EBaseAnimType_TP::Crouch_IdlePose_Ovr_TP: AnimSequence = WeaponAnimData->Crouch_IdlePose_Ovr_TP;
			break;


		case EBaseAnimType_TP::IdlePose_AFG_TP: AnimSequence = WeaponAnimData->IdlePose_AFG_TP;
			break;

		case EBaseAnimType_TP::IdlePose_VFG_TP: AnimSequence = WeaponAnimData->IdlePose_VFG_TP;
			break;

		case EBaseAnimType_TP::IdlePose_HSTOP_TP: AnimSequence = WeaponAnimData->IdlePose_HSTOP_TP;
			break;

		default:
			break;
		}
	}

	return AnimSequence;
}

/* we want to trigger the lowered override with a bit of delay to appear more natural */
void URoNAnimInstance_PlayerTP_V2::LoweredCooldownDone()
{
	// just to be safe, plus get pawn
	/*
	AReadyOrNotCharacter* OwningCharacter = Cast<AReadyOrNotCharacter>(TryGetPawnOwner());
	if (OwningCharacter == nullptr)
	{
		return;
	}
	*/

	// set lowered active again
	LoweredInternalVal = 0;
	bIsLoweredActive = false;

	// clear timer data
	//OwningCharacter->GetWorldTimerManager().ClearTimer(LoweredCooldownHandle);
}

void URoNAnimInstance_PlayerTP_V2::CalculateDistanceMatching(float DeltaTime, ACharacter* CharRef)
{
	if (!CharRef)
	{
		return;
	}

	bReadDisableSpeedCurve = (bIsStartStateFullWeight || bIsPostPivotStateFullWeight) ? true : false;
	DisableSpeedWarping = bReadDisableSpeedCurve ? FMath::GetMappedRangeValueClamped(FVector2D(1.0f, 0.0f), FVector2D(0.0f, 1.0f), GetCurveValue("DisableSpeedWarping")) : 0.0f;

	/* add higher speed threshold to prevent too much spamming */
	bCanEnterPrePivotRuleSet = (CharMovementSnapshot.bAccelerationOpposesVelocity && CharMovementSnapshot.Speed2D > 80.0f) ? true : false;

	/* if we are ai controlled update ai only related properties in regards to distance matching */
	if (bIsAIControlled)
	{
		UpdateAIDistanceMatchingProperties(DeltaTime);

		// this is a better place to visualize this longer
		/*
		{
			DrawDebugSphere(GetWorld(), GetActorLocationOnNav(), 8.0f, 8.0f, FColor::Blue, false, -1.0f, 0, 1.0f);
			DrawDebugLine(GetWorld(), GetActorLocationOnNav(), PFCurrentStopLocation, FColor::Turquoise, false, -1.0f, 0, 1.0f);
			DrawDebugSphere(GetWorld(), PFCurrentStopLocation, 8.0f, 8.0f, FColor::Turquoise, false, -1.0f, 0, 1.0f);
			FString RadiusDebugText = FString::Printf(TEXT("Acceptance Radius: %f"), GetCurrentMovementAcceptanceRadius());
			FString DistanceDebugText = FString::Printf(TEXT("Stop Distance: %f"), PFStopDistance);
			FVector DistanceHeadLocation = CharacterAiRef->GetActorLocation() + FVector(0, 0, 130); // 100 is an example offset
			FVector RadiusHeadLocation = CharacterAiRef->GetActorLocation() + FVector(0, 0, 150); // 100 is an example offset
			FColor TextColor = FColor::White;
			DrawDebugString(GetWorld(), DistanceHeadLocation, DistanceDebugText, nullptr, TextColor, DeltaTime + 0.02f, false);
			DrawDebugString(GetWorld(), RadiusHeadLocation, RadiusDebugText, nullptr, TextColor, DeltaTime + 0.02f, false);
		}
		*/
	}

	UpdateIdleState(DeltaTime);
	UpdateMoveState(DeltaTime);
	UpdateStopState(DeltaTime);
	UpdateStartState(DeltaTime);
	UpdatePrePivotState(DeltaTime);
	UpdatePostPivotState(DeltaTime);

#if ENABLE_DRAW_DEBUG
	/* draw important debugging functionality if requested */
	if (bShowDebug)
	{
		FVector CharacterPositionFeet = FVector(CharRef->GetActorLocation().X, CharRef->GetActorLocation().Y, CharRef->GetActorLocation().Z - CharRef->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 0.9f);

		float VelocityRemapClamped = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, CharRef->GetCharacterMovement()->MaxWalkSpeed), FVector2D(0.0f, 100.0f), CharRef->GetVelocity().Size());
		float AccRemapClamped = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, CharRef->GetCharacterMovement()->MaxWalkSpeed), FVector2D(0.0f, 100.0f), CharRef->GetCharacterMovement()->GetCurrentAcceleration().Size());

		// velocity and acceleration
		DrawDebugDirectionalArrow(CharRef->GetWorld(), CharacterPositionFeet, CharacterPositionFeet + (CharRef->GetVelocity().GetSafeNormal() * VelocityRemapClamped), 300.0f, FColor::Yellow, false, -1.0f, 0, 1.5f);
		DrawDebugDirectionalArrow(CharRef->GetWorld(), CharacterPositionFeet, CharacterPositionFeet + (CharRef->GetCharacterMovement()->GetCurrentAcceleration().GetSafeNormal() * AccRemapClamped), 300.0f, FColor::Magenta, false, -1.0f, 0, 1.5f);

		// for debugging mesh locking, turn in place and rotational transitions
		FRotator MeshRotationWithOffset = UKismetMathLibrary::ComposeRotators(CharRef->GetActorRotation(), FRotator(0.0f, TurnInPlaceState.RootYawOffset, 0.0f));
		DrawDebugDirectionalArrow(CharRef->GetWorld(), CharacterPositionFeet, CharacterPositionFeet + (MeshRotationWithOffset.Vector().GetSafeNormal() * 100.0f), 300.0f, FColor::Emerald, false, -1.0f, 0, 1.5f);
		const FVector TransformYAxisVector = CharRef->GetActorTransform().TransformVector(FVector(1.0f, 0.0f, 0.0f));
		const FVector TransformZAxisVector = CharRef->GetActorTransform().TransformVector(FVector(0.0f, 1.0f, 0.0f));
		DrawDebugCircle(CharRef->GetWorld(), CharacterPositionFeet, 100.0f, 32, FColor::Black, false, -1.0f, SDPG_World, 1.0f, TransformYAxisVector, TransformZAxisVector, true);
		DrawDebugDirectionalArrow(CharRef->GetWorld(), CharacterPositionFeet, CharacterPositionFeet + (CharRef->GetActorRotation().Vector().GetSafeNormal() * 100.0f), 300.0f, FColor::Black, false, -1.0f, 0, 1.5f);
		DrawDebugCapsule(CharRef->GetWorld(), CharRef->GetActorLocation(), CharRef->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(), CharRef->GetCapsuleComponent()->GetScaledCapsuleRadius(), CharRef->GetActorRotation().Quaternion(), FColor::Silver, false, -1.0f, 0, 0.1f);
		DrawDebugLine(CharRef->GetWorld(), CharRef->GetActorLocation(), LastActorLocation, FColor::White, false, 1.0f, 0, 0.5f);
		LastActorLocation = CharRef->GetActorLocation();
	}
#endif
}

/* used for looking up relevant states later in update */
void URoNAnimInstance_PlayerTP_V2::DefineStateMachineData()
{
	IdleStateData.StateMachineName = "DistanceMatch_SM";
	IdleStateData.StateName = "Idle";

	StartStateData.StateMachineName = "DistanceMatch_SM";
	StartStateData.StateName = "Start";

	StopStateData.StateMachineName = "DistanceMatch_SM";
	StopStateData.StateName = "Stop";

	MoveStateData.StateMachineName = "DistanceMatch_SM";
	MoveStateData.StateName = "Move";

	PrePivotStateData.StateMachineName = "DistanceMatch_SM";
	PrePivotStateData.StateName = "PrePivot";

	PostPivotStateData.StateMachineName = "DistanceMatch_SM";
	PostPivotStateData.StateName = "PostPivot";

	TurnInPlaceStateData.StateMachineName = "Turn_SM";
	TurnInPlaceStateData.StateName = "TurnInPlace";

	/* AI Controlled Characters use a different state-machine path */
	if (bIsAIControlled)
	{
		IdleStateData.StateMachineName = "DistanceMatch_AI_SM";
		StartStateData.StateMachineName = "DistanceMatch_AI_SM";
		StopStateData.StateMachineName = "DistanceMatch_AI_SM";
		MoveStateData.StateMachineName = "DistanceMatch_AI_SM";
		PrePivotStateData.StateMachineName = "DistanceMatch_AI_SM";
		PostPivotStateData.StateMachineName = "DistanceMatch_AI_SM";
		TurnInPlaceStateData.StateMachineName = "Turn_AI_SM";
	}

	LocalTurnInPlaceStateData.StateMachineName = "Turn_LocallyFP_SM";
	LocalTurnInPlaceStateData.StateName = "TurnInPlace";
}

void URoNAnimInstance_PlayerTP_V2::UpdateStateMachineData(float DeltaSeconds)
{
	bIsIdleStateRelevant = UCachedAnimDataLibrary::StateMachine_IsStateRelevant(this, IdleStateData);

	bIsStartStateRelevant = UCachedAnimDataLibrary::StateMachine_IsStateRelevant(this, StartStateData);
	bIsStartStateFullWeight = StartStateData.IsFullWeight(*this);

	bIsStopStateRelevant = UCachedAnimDataLibrary::StateMachine_IsStateRelevant(this, StopStateData);

	bIsPrePivotStateRelevant = UCachedAnimDataLibrary::StateMachine_IsStateRelevant(this, PrePivotStateData);
	bIsPrePivotStateFullWeight = PrePivotStateData.IsFullWeight(*this);

	bIsPostPivotStateRelevant = UCachedAnimDataLibrary::StateMachine_IsStateRelevant(this, PostPivotStateData);
	bIsPostPivotStateFullWeight = PostPivotStateData.IsFullWeight(*this);

	bIsMoveStateRelevant = UCachedAnimDataLibrary::StateMachine_IsStateRelevant(this, MoveStateData);

	bIsTurnInPlaceStateRelevant = UCachedAnimDataLibrary::StateMachine_IsStateRelevant(this, bIsLocallyFirstPerson ? LocalTurnInPlaceStateData : TurnInPlaceStateData);
}

void URoNAnimInstance_PlayerTP_V2::AddNativeStateMachineBindings()
{
	AddNativeStateEntryBinding(StartStateData.StateMachineName, StartStateData.StateName, FOnGraphStateChanged::CreateUObject(this, &URoNAnimInstance_PlayerTP_V2::EnterStartState));
	AddNativeStateEntryBinding(StopStateData.StateMachineName, StopStateData.StateName, FOnGraphStateChanged::CreateUObject(this, &URoNAnimInstance_PlayerTP_V2::EnterStopState));
	AddNativeStateEntryBinding(PrePivotStateData.StateMachineName, PrePivotStateData.StateName, FOnGraphStateChanged::CreateUObject(this, &URoNAnimInstance_PlayerTP_V2::EnterPrePivotState));
	AddNativeStateEntryBinding(PostPivotStateData.StateMachineName, PostPivotStateData.StateName, FOnGraphStateChanged::CreateUObject(this, &URoNAnimInstance_PlayerTP_V2::EnterPostPivotState));
}

void URoNAnimInstance_PlayerTP_V2::EnterStartState(const FAnimNode_StateMachine& Machine, int32 PrevStateIndex, int32 NextStateIndex)
{
	TArray<UAnimSequence*> CurrentStartAnimations = GetStartAnimationSet();
	if (CurrentStartAnimations.Num() == 0)
	{
		return;
	}

	bStartToCycleRuleSet = false;
	TimeInStartState = 0.0f;
	LastStartSpeed = ReplicatedMaxSpeed;
	StartVelocityDirection = CharMovementSnapshot.LocalAccelerationDirection;
	ActiveStartAnimTime = 0.0f; // Reset to zero
	ActiveStartAnim = UAnimDistanceMatchingLibrary::FindBestDirectionAnimation(CurrentStartAnimations, StartVelocityDirection, false);
	StrideWarpingStartAlpha = 0.0f;

	bStartWasCrouchedInitially = bCrouching;

	if (!ActiveStartAnim)
	{
		return;
	}

	// we hold the root yaw offset in start!
	// TODO add

#if ENABLE_DRAW_DEBUG
	if (bShowDebug)
	{
		//UE_LOG(LogTemp, Warning, TEXT("Picked Start Sequence %s"), *ActiveStartAnim->GetName());

		// draw debug for distance matching!
		DrawDebugSphere(BaseCharacterRef->GetWorld(), BaseCharacterRef->GetActorLocation(), 16.0f, 32, FColor::Blue, false, 2.5f, 0.0f);
	}
#endif
}

void URoNAnimInstance_PlayerTP_V2::EnterStopState(const FAnimNode_StateMachine& Machine, int32 PrevStateIndex, int32 NextStateIndex)
{
	TArray<UAnimSequence*> CurrentStopAnimations = GetStopAnimationSet();
	if (CurrentStopAnimations.Num() == 0)
	{
		return;
	}

	/* find fitting stop animation */
	ActiveStopAnim = UAnimDistanceMatchingLibrary::FindBestDirectionAnimation(CurrentStopAnimations, CharMovementSnapshot.LocalVelocityDirection, true);

	bIsFastStop = CharMovementSnapshot.Speed2D > SpeedSelection;
	if (!ActiveStopAnim)
	{
		return;
	}

	bStopWasCrouchedInitially = bCrouching;

	if (bIsAIControlled)
	{
		UE_LOG(LogTemp, Warning, TEXT("Stop State entered!"));
		bPFRequestedNewPath = false;
		PFLastStopLocation = GetLastPathPointWithAcceptanceRadius();
		PFStopDistance = FMath::Clamp(FVector::Distance(GetActorLocationOnNav(), PFLastStopLocation), 0.0f, 1000.0f);
		ActiveStopAnimTime = UAnimDistanceMatchingLibrary::GetTimeFromDistance(ActiveStopAnim, PFStopDistance, DistanceCurveName);
	}
	else
	{
		// reset explicit time and assign animation for usage
		FVector StopDistanceVector = UAnimDistanceMatchingLibrary::PredictGroundMovementStopLocation(CharMovementSnapshot.WorldVelocity, BaseCharacterRef->GetCharacterMovement()->bUseSeparateBrakingFriction, BaseCharacterRef->GetCharacterMovement()->BrakingFriction, BaseCharacterRef->GetCharacterMovement()->GroundFriction, BaseCharacterRef->GetCharacterMovement()->BrakingFrictionFactor, BaseCharacterRef->GetCharacterMovement()->BrakingDecelerationWalking);
		float DistanceToMatch = StopDistanceVector.Size2D();
		ActiveStopAnimTime = UAnimDistanceMatchingLibrary::GetTimeFromDistance(ActiveStopAnim, DistanceToMatch, DistanceCurveName);
	}

#if ENABLE_DRAW_DEBUG
	if (bShowDebug)
	{
		// UE_LOG(LogTemp, Warning, TEXT("Picked Stop Sequence %s"), *ActiveStopAnim->GetName());
	}
#endif
}

void URoNAnimInstance_PlayerTP_V2::EnterPrePivotState(const FAnimNode_StateMachine& Machine, int32 PrevStateIndex, int32 NextStateIndex)
{
	TArray<UAnimSequence*> CurrentPivotAnims = GetPivotAnimationSet(ReplicatedMaxSpeed);
	if (CurrentPivotAnims.Num() == 0)
	{
		return;
	}

	PivotStartingAcceleration = CharMovementSnapshot.WorldAcceleration;
	PivotVelocityDirection = CharMovementSnapshot.LocalVelocityDirection;

	ActivePrePivotAnim = UAnimDistanceMatchingLibrary::FindBestDirectionAnimation(CurrentPivotAnims, PivotVelocityDirection, true);

	if (!ActivePrePivotAnim)
	{
		return;
	}

	// reset explicit time and assign animation for usage
	FVector PivotDistanceVector = UAnimDistanceMatchingLibrary::PredictGroundMovementPivotLocation(CharMovementSnapshot.WorldAcceleration, BaseCharacterRef->GetCharacterMovement()->GetLastUpdateVelocity(), BaseCharacterRef->GetCharacterMovement()->GroundFriction);
	float DistanceToMatch = PivotDistanceVector.Size2D();
	ActivePrePivotAnimTime = UAnimDistanceMatchingLibrary::GetTimeFromDistance(ActivePrePivotAnim, DistanceToMatch, DistanceCurveName);
	PrePivotStationaryTimePoint = UAnimDistanceMatchingLibrary::GetTimeFromDistance(ActivePrePivotAnim, 0.0f, DistanceCurveName);

#if ENABLE_DRAW_DEBUG
	if (bShowDebug)
	{
		//UE_LOG(LogTemp, Warning, TEXT("Picked Pre Pivot Sequence %s"), *ActivePrePivotAnim->GetName());
	}
#endif
}

void URoNAnimInstance_PlayerTP_V2::EnterPostPivotState(const FAnimNode_StateMachine& Machine, int32 PrevStateIndex, int32 NextStateIndex)
{
	if (!ActivePrePivotAnim)
	{
		return;
	}

	ActivePostPivotAnim = ActivePrePivotAnim;
	if (!ActivePostPivotAnim)
	{
		return;
	}

	bPivotWasCrouchedInitially = bCrouching;

	bPivotToCycleRuleSet = false;
	LastPivotSpeed = ReplicatedMaxSpeed;
	PostPivotVelocityDirection = CharMovementSnapshot.LocalAccelerationDirection;
	TimeInPostPivotState = 0.0f;
	ActivePostPivotAnimTime = ActivePrePivotAnimTime;
	StrideWarpingPivotAlpha = 0.0f;

#if ENABLE_DRAW_DEBUG
	if (bShowDebug)
	{
		//UE_LOG(LogTemp, Warning, TEXT("Picked Post Pivot Sequence %s"), *ActivePostPivotAnim->GetName());
	}
#endif
}

void URoNAnimInstance_PlayerTP_V2::UpdateStartState(float DeltaSeconds)
{
	if (!BaseCharacterRef)
	{
		return;
	}

	if (bIsStartStateRelevant)
	{
		TimeInStartState += DeltaSeconds;

		/* main update logic */

		/* wait until state machine has full weight to apply rulesets! */
		if (bIsStartStateFullWeight)
		{
			// updated the state transition ruleset combination
			bool bDisplacementTooLowRule = (ActiveStartAnimTime > 0.2f && BaseCharacterRef->GetVelocity().Size2D() < 10.0f) ? true : false;
			float CurDirAngle = CharMovementSnapshot.LocalVelocityDirection.Rotation().Yaw;
			float CurStartAngle = StartVelocityDirection.Rotation().Yaw;
			float CurrentDeltaAngle = FMath::FindDeltaAngleDegrees(CurDirAngle, CurStartAngle);
			bool bStartDirectionBrokenRule = (ActiveStartAnimTime > 0.2f && (FMath::Abs(CurrentDeltaAngle) > 15.0f)) ? true : false;
			bool bHasSpeedChanged = (LastStartSpeed != ReplicatedMaxSpeed) ? true : false;
			bool bHasStanceChanged = (bStartWasCrouchedInitially != bCrouching) ? true : false;
			bStartToCycleRuleSet = (bHasStanceChanged || bHasSpeedChanged || bDisplacementTooLowRule || bStartDirectionBrokenRule) ? true : false;
		}

		// use lyra approach for distance....
		DistanceTraveledSinceLastUpdate = FVector(BaseCharacterRef->GetActorLocation() - WorldLocation).Size2D();
		WorldLocation = BaseCharacterRef->GetActorLocation();

		float ExplicitTime = UAnimDistanceMatchingLibrary::GetRelevantAnimTime(this, StartStateData.StateMachineName, StartStateData.StateName);

		/* Alpha = (ExplicitTime - Offset)/Duration */
		StrideWarpingStartAlpha = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, StrideWarpingBlendInDurationScaled), FVector2D(0.0f, 1.0f), ExplicitTime - StrideWarpingBlendInStartOffset);

		/* Smoothly increase the minimum playrate speed, as we blend in stride warping */
		FVector2D CurClampedPlayrate = FVector2D(FMath::Lerp(StrideWarpingBlendInDurationScaled, PlayRateClampStartsPivots.X, StrideWarpingStartAlpha), PlayRateClampStartsPivots.Y);

		/* advance time by distance matching! */
		UAnimDistanceMatchingLibrary::AdvanceTimeByDistanceMatching(DeltaSeconds, ActiveStartAnim, ActiveStartAnimTime, DistanceTraveledSinceLastUpdate, DistanceCurveName, CurClampedPlayrate);
	}
}

void URoNAnimInstance_PlayerTP_V2::UpdateStopState(float DeltaSeconds)
{
	if (!BaseCharacterRef)
	{
		return;
	}

	if (bIsStopStateRelevant)
	{
		const bool bHasStanceChanged = bStartWasCrouchedInitially != bCrouching;
		bStopToIdleRuleSet = bHasStanceChanged || TurnInPlaceState.bTurnTransitionRequested;

		if (bIsAIControlled)
		{
			float PathDistanceOldNew = FVector::Dist(PFCurrentStopLocation, PFLastStopLocation);
			const float PathChangeThreshold = 10.0f; // Adjust this value as needed
			if (PathDistanceOldNew > PathChangeThreshold)
			{
				bPFRequestedNewPath = true;
				//UE_LOG(LogTemp, Warning, TEXT("REQUESTED NEW PATH!"));
			}

			bStopToIdleRuleSet = bHasStanceChanged || TurnInPlaceState.bTurnTransitionRequested || bPFRequestedNewPath;

			if (!bStopToIdleRuleSet)
			{
				const float CurrentDistance = FMath::Clamp(FVector::Distance(GetActorLocationOnNav(), PFLastStopLocation), 0.0f, 1000.0f);

				// use small threshold
				if (bPFPathIsValid && CurrentDistance > 0.0f)
				{
					UAnimDistanceMatchingLibrary::DistanceMatchToTarget(ActiveStopAnim, ActiveStopAnimTime, CurrentDistance, DistanceCurveName);
				}
				else
				{
					// advance time naturally once we reach 0 distance
					UAnimDistanceMatchingLibrary::AdvanceTimeNaturally(DeltaSeconds, ActiveStopAnim, ActiveStopAnimTime);
				}
			}
		}
		else
		{
			/* main update logic */
			if (UAnimDistanceMatchingLibrary::ShouldDistanceMatchStop(BaseCharacterRef->GetActorTransform(), CharMovementSnapshot.WorldVelocity, CharMovementSnapshot.WorldAcceleration))
			{
				FVector StopDistanceVector = UAnimDistanceMatchingLibrary::PredictGroundMovementStopLocation(CharMovementSnapshot.WorldVelocity, BaseCharacterRef->GetCharacterMovement()->bUseSeparateBrakingFriction, BaseCharacterRef->GetCharacterMovement()->BrakingFriction, BaseCharacterRef->GetCharacterMovement()->GroundFriction, BaseCharacterRef->GetCharacterMovement()->BrakingFrictionFactor, BaseCharacterRef->GetCharacterMovement()->BrakingDecelerationWalking);
				float DistanceToMatch = StopDistanceVector.Size2D();

				if (bShowDebug)
				{
					// draw debug for distance matching!
					DrawDebugSphere(BaseCharacterRef->GetWorld(), BaseCharacterRef->GetActorLocation() + StopDistanceVector, 16.0f, 32, FColor::Red, false, -1.0f, 0.0f);
				}

				if (DistanceToMatch > 0.0f)
				{
					// Distance Match to Stop Point
					UAnimDistanceMatchingLibrary::DistanceMatchToTarget(ActiveStopAnim, ActiveStopAnimTime, DistanceToMatch, DistanceCurveName);
				}
				else
				{
					// advance time naturally once we reach 0 distance
					UAnimDistanceMatchingLibrary::AdvanceTimeNaturally(DeltaSeconds, ActiveStopAnim, ActiveStopAnimTime);
				}
			}
			else
			{
				//UE_LOG(LogTemp, Warning, TEXT("Playing Sequence"));
				// advance time naturally if we cannot distance match stop
				UAnimDistanceMatchingLibrary::AdvanceTimeNaturally(DeltaSeconds, ActiveStopAnim, ActiveStopAnimTime);
			}
		}
	}
}

void URoNAnimInstance_PlayerTP_V2::UpdatePrePivotState(float DeltaSeconds)
{
	if (!BaseCharacterRef)
	{
		return;
	}

	if (bIsPrePivotStateRelevant)
	{

		// While acceleration opposes velocity, the character is still approaching the pivot point, so we distance match to that point.
		FVector PivotDistanceVector = UAnimDistanceMatchingLibrary::PredictGroundMovementPivotLocation(CharMovementSnapshot.WorldAcceleration, BaseCharacterRef->GetCharacterMovement()->GetLastUpdateVelocity(), BaseCharacterRef->GetCharacterMovement()->GroundFriction);
		float DistanceToMatch = PivotDistanceVector.Size2D();
	
#if ENABLE_DRAW_DEBUG
		if (bShowDebug)
		{
			// draw debug for distance matching!
			DrawDebugSphere(BaseCharacterRef->GetWorld(), BaseCharacterRef->GetActorLocation() + PivotDistanceVector, 16.0f, 32, FColor::Purple, false, -1.0f, 0.0f);
		}
#endif

		/* if current animation time exceeds stationary point time */
		float AnimTime = UAnimDistanceMatchingLibrary::GetRelevantAnimTime(this, PrePivotStateData.StateMachineName, PrePivotStateData.StateName);
		bPrePivotToPostPivotRuleSet = (bIsPrePivotStateFullWeight && (AnimTime >= PrePivotStationaryTimePoint) && CharMovementSnapshot.bAccelerationEqualsVelocity) ? true : false;

		if (DistanceToMatch > 0.1f)
		{
			// Distance Match to Pivot Point
			UAnimDistanceMatchingLibrary::DistanceMatchToTarget(ActivePrePivotAnim, ActivePrePivotAnimTime, DistanceToMatch, DistanceCurveName);
		}
		else
		{
			// advance time naturally if we cannot distance match pivot
			UAnimDistanceMatchingLibrary::AdvanceTimeNaturally(DeltaSeconds, ActivePrePivotAnim, ActivePrePivotAnimTime);
		}
	}
}

void URoNAnimInstance_PlayerTP_V2::UpdatePostPivotState(float DeltaSeconds)
{
	if (!BaseCharacterRef)
	{
		return;
	}

	if (bIsPostPivotStateRelevant)
	{
		TimeInPostPivotState += DeltaSeconds;

		bPostPivotToPrePivotRuleSet = (bIsPostPivotStateFullWeight && CharMovementSnapshot.bAccelerationOpposesVelocity && CharMovementSnapshot.bIsMoving) ? true : false;

		if (TimeInPostPivotState > 0.25f)
		{
			// Once acceleration and velocity are aligned, the character is accelerating away from the pivot point, so we just advance time by distance traveled for the rest of the animation.
			float CurDirAngle = CharMovementSnapshot.VelocityYawAngle;
			float CurPivotAngle = PostPivotVelocityDirection.Rotation().Yaw;
			float CurrentDeltaAngle = FMath::FindDeltaAngleDegrees(CurDirAngle, CurPivotAngle);
			bool DeltaBiggerThenThreshold = (FMath::Abs(CurrentDeltaAngle) > 20.0f) ? true : false;
			bool bSpeedHasChanged = (LastPivotSpeed != ReplicatedMaxSpeed) ? true : false;
			bool bStanceHasChanged = (bPivotWasCrouchedInitially != bCrouching) ? true : false;
			bPivotToCycleRuleSet = (bStanceHasChanged || DeltaBiggerThenThreshold || bSpeedHasChanged) ? true : false;
		}

		// use lyra approach for distance....
		DistanceTraveledSinceLastUpdate = FVector(BaseCharacterRef->GetActorLocation() - WorldLocation).Size2D();
		WorldLocation = BaseCharacterRef->GetActorLocation();

		float ExplicitTime = UAnimDistanceMatchingLibrary::GetRelevantAnimTime(this, StartStateData.StateMachineName, StartStateData.StateName);

		StrideWarpingPivotAlpha = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, StrideWarpingBlendInDurationScaled), FVector2D(0.0f, 1.0f), (ExplicitTime - PrePivotStationaryTimePoint) - StrideWarpingBlendInStartOffset);

		/* Smoothly increase the minimum playrate speed, as we blend in stride warping */
		FVector2D CurClampedPlayrate = FVector2D(FMath::Lerp(0.2f, PlayRateClampStartsPivots.X, StrideWarpingPivotAlpha), PlayRateClampStartsPivots.Y);

		/* advance time by distance matching! */
		UAnimDistanceMatchingLibrary::AdvanceTimeByDistanceMatching(DeltaSeconds, ActivePostPivotAnim, ActivePostPivotAnimTime, DistanceTraveledSinceLastUpdate, DistanceCurveName, CurClampedPlayrate);
	}
}

void URoNAnimInstance_PlayerTP_V2::UpdateMoveState(float DeltaSeconds)
{
}

void URoNAnimInstance_PlayerTP_V2::UpdateIdleState(float DeltaSeconds)
{
}

TArray<UAnimSequence*> URoNAnimInstance_PlayerTP_V2::GetStartAnimationSet()
{
	TArray<UAnimSequence*> CurrentStartAnimations;
	if (ReplicatedMaxSpeed > SpeedSelection)
	{
		CurrentStartAnimations = bCrouching ? CrouchFastStartAnimations : StandFastStartAnimations;
	}
	else
	{
		CurrentStartAnimations = bCrouching ? CrouchSlowStartAnimations : StandSlowStartAnimations;
	}
	return CurrentStartAnimations;
}

TArray<UAnimSequence*> URoNAnimInstance_PlayerTP_V2::GetStopAnimationSet()
{
	TArray<UAnimSequence*> CurrentStopAnimations;
	if (CharMovementSnapshot.Speed2D > 220.0f)
	{
		CurrentStopAnimations = bCrouching ? CrouchFastStopAnimations : StandFastStopAnimations;
	}
	else
	{
		CurrentStopAnimations = bCrouching ? CrouchSlowStopAnimations : StandSlowStopAnimations;
	}
	return CurrentStopAnimations;
}

TArray<UAnimSequence*> URoNAnimInstance_PlayerTP_V2::GetPivotAnimationSet(float InputSpeed)
{
	if (InputSpeed > 220.0f) // hardcode for now...
	{
		return bCrouching ? CrouchFastPivotAnimations : StandFastPivotAnimations;
	}
	
	return bCrouching ? CrouchSlowPivotAnimations : StandSlowPivotAnimations;
}

FVector URoNAnimInstance_PlayerTP_V2::GetActorLocationOnNav()
{
	if (!CharacterAiRef)
	{
		return FVector::ZeroVector;
	}

	/* we need the actor position projected on the nav mesh */
	FVector CurrentActorLocationNav = CharacterAiRef->GetNavAgentLocation();
	UReadyOrNotAISystem::ProjectPointToNav(CharacterAiRef->GetNavAgentLocation(), CurrentActorLocationNav);
	return CurrentActorLocationNav;
}

FVector URoNAnimInstance_PlayerTP_V2::GetLastPathPointWithAcceptanceRadius()
{
	if (!CharacterAiRef)
	{
		return FVector::ZeroVector;
	}

	/* get the ai controller class of the current character */
	AAIController* AIController = Cast<AAIController>(CharacterAiRef->GetController());
	if (!AIController)
	{
		return FVector::ZeroVector;
	}

	/* get the path following component */
	const UPathFollowingComponent* PFComp = AIController->GetPathFollowingComponent();
	if (!PFComp)
	{
		return FVector::ZeroVector;
	}

	// if path is invalid don't bother
	if (!PFComp->GetPath().IsValid())
	{
		return FVector::ZeroVector;
	}

	float CurrentAcceptanceRadius = PFComp->GetAcceptanceRadius(); // BaseActivity contains MoveAcceptanceRadius = 10.0f;
	const FVector PreviousPathPointLocation = PFComp->GetPath()->GetPathPoints()[PFComp->GetPath()->GetPathPoints().Num() - 2].Location;
	const FVector CurrentTarget = PFComp->GetPath()->GetEndLocation();
	FVector direction = (CurrentTarget - PreviousPathPointLocation).GetSafeNormal2D();  // 2D normalization.
	return CurrentTarget - (direction * CurrentAcceptanceRadius);
}

float URoNAnimInstance_PlayerTP_V2::GetCurrentMovementAcceptanceRadius()
{
	if (!CharacterAiRef)
	{
		return 0.0f;
	}

	/* get the ai controller class of the current character */
	AAIController* AIController = Cast<AAIController>(CharacterAiRef->GetController());
	if (!AIController)
	{
		return 0.0f;
	}

	/* get the path following component */
	const UPathFollowingComponent* PFComp = AIController->GetPathFollowingComponent();
	if (!PFComp)
	{
		return 0.0f;
	}

	// if path is invalid don't bother
	if (!PFComp->GetPath().IsValid())
	{
		return 0.0f;
	}

	return PFComp->GetAcceptanceRadius(); // BaseActivity contains MoveAcceptanceRadius = 10.0f;
}

bool URoNAnimInstance_PlayerTP_V2::GetIsPathTooShort(float Tolerance)
{
	if (!CharacterAiRef)
	{
		return false;
	}

	/* get the ai controller class of the current character */
	AAIController* AIController = Cast<AAIController>(CharacterAiRef->GetController());
	if (!AIController)
	{
		return false;
	}

	/* get the path following component */
	const UPathFollowingComponent* PFComp = AIController->GetPathFollowingComponent();
	if (!PFComp)
	{
		return false;
	}

	// if path is invalid don't bother
	if (!PFComp->GetPath().IsValid())
	{
		return false;
	}

	float CurrentPathSize = FVector::Distance(PFComp->GetPath()->GetPathPoints()[0].Location, PFComp->GetPath()->GetPathPoints().Last().Location);
	if (CurrentPathSize <= Tolerance)
	{
		return true;
	}

	return false;
}

void URoNAnimInstance_PlayerTP_V2::UpdateAIDistanceMatchingProperties(float DeltaSeconds)
{
	if (!CharacterAiRef)
	{
		return;
	}

	/* get the ai controller class of the current character */
	AAIController* AIController = Cast<AAIController>(CharacterAiRef->GetController());
	if (!AIController)
	{
		return;
	}

	/* get the path following component */
	const UPathFollowingComponent* PFComp = AIController->GetPathFollowingComponent();
	if (!PFComp)
	{
		return;
	}

	bPFPathIsValid = PFComp->GetPath().IsValid();

	// if path is invalid don't bother
	if (!PFComp->GetPath().IsValid())
	{
		PFStopDistance = 0.0f;
		bPFStartPhase = false;
		bPFStoppingPhase = false;
		bPFPathTooShort = false;
		return;
	}

	PFCurrentStopLocation = GetLastPathPointWithAcceptanceRadius();
	PFStopDistance = FMath::Clamp(FVector::Distance(GetActorLocationOnNav(), PFCurrentStopLocation), 0.0f, 1000.0f);
	bPFPathTooShort = GetIsPathTooShort(PFStartDistanceThreshold + PFStopDistanceThreshold);
	bPFStartPhase = ((PFComp->GetStatus() == EPathFollowingStatus::Moving) && CharMovementSnapshot.bIsMoving && !bPFPathTooShort) ? true : false;
	bPFStoppingPhase = ((PFComp->GetStatus() == EPathFollowingStatus::Moving) && !bPFPathTooShort && (PFStopDistance < PFStopDistanceThreshold)) ? true : false;
}