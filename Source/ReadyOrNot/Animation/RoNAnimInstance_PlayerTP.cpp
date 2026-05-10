// Copyright Void Interactive, 2022
// Author: Alexander Mijalkovski

#include "Animation/RoNAnimInstance_PlayerTP.h"

#include "CachedAnimDataLibrary.h"
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

TAutoConsoleVariable<int32> CVar_RonAnimInstance_PlayerTP_EnableDebug(TEXT("a.RonAnimInstance_PlayerTP.EnableDebug"), 0, TEXT("Toggle Debug Information for Ron Anim Instance Player TP."));
TAutoConsoleVariable<int32> CVar_RonAnimInstance_PlayerTP_EnableDistanceMatching(TEXT("a.RonAnimInstance_PlayerTP.EnableDistanceMatching"), 1, TEXT("Enable Distance Matching for Player Graphs."));

URoNAnimInstance_PlayerTP::URoNAnimInstance_PlayerTP(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	LoweredCooldownTime = 0.8f;
	PlayrateClampMax = 0.15f;

	DistanceMatchingCurrentState = EDistanceMatchingType::None;
	PostPivotTriggerThreshold = 1.0f;

	PelvisDefaultWorldPos = FVector(0.294243f, -1.095897f, 91.050880f);
	CrouchedPelvisDefaultWorldPos = FVector(2.459475f, -14.275543f, 43.456177f);
	CrouchedPelvisMovingWorldPos = FVector(-0.602592f, -1.029617f, 75.528893f);
}

void URoNAnimInstance_PlayerTP::NativeInitializeAnimation()
{
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
	}
}

// main func for tick calculations
void URoNAnimInstance_PlayerTP::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

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
	
	// see if we need to use distance matching
	/* distance matching is not applied to locally controlled character due to messing up firstperson view too much */
	if (CharacterRef)
	{
		bUseDistanceMatching = !(CharacterRef->IsLocallyControlled() && CharacterRef->GetFirstPersonCameraComponent());
	}
	else
	{
		bUseDistanceMatching = true;
	}
	
	#if !UE_BUILD_SHIPPING
	if (CVar_RonAnimInstance_PlayerTP_EnableDistanceMatching.GetValueOnAnyThread() == 0)
	{
		bUseDistanceMatching = false;
	}
	#endif

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

	// update turn in place
	UAnimTurnInPlaceLibrary::UpdateTurnInPlace(DeltaSeconds, bAllowTurnInPlace, false, bIsTurnInPlaceStateRelevant, true, 145.0f, CurChar->GetMesh()->GetComponentRotation(), TurnInPlaceAnimSet, TurnInPlaceState, TurnInPlaceSpeedMultiplier);
	
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
			if (CurAiChar->bDisableTurnInPlace || CurAiChar->IsAny3PMontageActive() || FMath::Abs(CurChar->GetVelocity().Size2D()) > 75.0f)
				bAllowTurnInPlace = false;
		}
	}
	else
	{
		HeadLookRotation = UKismetMathLibrary::RInterpTo(HeadLookRotation, GetLookAtRotation(), DeltaSeconds, 1.5f);
	}

	// debug if requested
	const bool bShowDebug = (CVar_RonAnimInstance_PlayerTP_EnableDebug.GetValueOnAnyThread() == 1);


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
	
	if (bUseDistanceMatching)
	{
		if (CharacterRef)
		{	
			// needed for replicated acceleration!
			UReadyOrNotCharMovementComp* RonMovementComponent = Cast<UReadyOrNotCharMovementComp>(CharacterRef->GetCharacterMovement());
			ReplicatedMaxSpeed = CharacterRef->ReplicatedMaxSpeed;

			if (RonMovementComponent)
			{
				/* distance matching! */
				UAnimCharacterMovementLibrary::UpdateCharacterMovementSnapshot(CharacterRef->GetActorTransform(), CharacterRef->GetVelocity(), RonMovementComponent->GetCurrentAcceleration(), CharacterRef->GetCharacterMovement()->IsMovingOnGround(), 0.0f, CharMovementSnapshot);
				CalculateDistanceMatching(DeltaSeconds, CharacterRef, bShowDebug);
			}
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
				CalculateDistanceMatchingAi(DeltaSeconds, CharacterAiRef, bShowDebug);
			}
		}

		// see if start direction is broken
		if (GetCurrentDirectionExtFromYawAngle(StartMarker.YawAngle) != CurrentDirectionExt)
		{
			bStartDirectionBroken = true;
		}
		else
		{
			bStartDirectionBroken = false;
		}

		// calculate pivot direction
		if (CharMovementSnapshot.bAccelerationOpposesVelocity)
			CurrentPivotDirectionExt = CurrentDirectionExt;
		
		// limit possible direction to pivot in
		if (CurrentPivotDirectionExt == EMoveDirectionExt::BL)
			bCanPivotInCurDirection = false;
		else if (CurrentPivotDirectionExt == EMoveDirectionExt::BR)
			bCanPivotInCurDirection = false;
		else if (CurrentPivotDirectionExt == EMoveDirectionExt::FL)
			bCanPivotInCurDirection = false;
		else if (CurrentPivotDirectionExt == EMoveDirectionExt::FR)
			bCanPivotInCurDirection = false;
		else
			bCanPivotInCurDirection = true;

		// see if we broke the pivot direction
		if (GetOppositeDirectionExt(CurrentPivotDirectionExt) != CurrentDirectionExt)
		{
			bPivotDirectionBroken = true;
		}
		else
		{
			bPivotDirectionBroken = false;
		}

		// create the pre-pivot transition rule here
		bSMPrePivotRuleset = bCanPivotInCurDirection && CharMovementSnapshot.bAccelerationOpposesVelocity;
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

	bJumpStartTrigger = (bIsFalling & !bHasPrelanded) ? true : false;

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

	if (!AICharacter)
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
					SightAlpha = FMath::FInterpTo(SightAlpha, 1.0f, DeltaSeconds, 8.0f);
				}
				else
				{
					SightAlpha = FMath::FInterpTo(SightAlpha, 0.0f, DeltaSeconds, 8.0f);
				}
			}
			else
			{
				SightAlpha = 0.0f;
			}

			const bool bIsSprinting = CurPlayerChar && CurPlayerChar->IsSprinting();

			if (LastAnimWeaponData->bHasLoweredAdditives && !IsLoweredUp)
			{
				// Lowered Alpha, if we have a active montage or in sights or weapon close to walls, trigger lowered alpha!
				if ((CurChar->IsAny3PMontageActive() ||
					bIsAiming ||
					bWeaponDown ||
					bIsSprinting) && !CurChar->IsLowReady())
				{
					LoweredInternalVal = 0;
					bIsLoweredActive = false;
				}
				else
				{
					// run timer here
					if (!bIsLoweredActive)
					{
						if (Cast<ASWATCharacter>(CurChar))
						{
							CurChar->GetWorldTimerManager().SetTimer(LoweredCooldownHandle, this, &URoNAnimInstance_PlayerTP::LoweredCooldownDone, 0.1f, false);
							//LoweredInternalVal = 1;
							bIsLoweredActive = true;
						}
						else
						{
							LoweredInternalVal = 0;
							bIsLoweredActive = false;
						}
						
					}
				}

				LoweredAlpha = FMath::FInterpTo(LoweredAlpha, LoweredInternalVal, DeltaSeconds, 8.0f);
			}
			else
			{
				LoweredAlpha = 0.0f;
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
	
	ACyberneticCharacter* CyberneticCharacter = Cast<ACyberneticCharacter>(CurChar);
	if (CyberneticCharacter)
	{
		//AimOffsets = FVector2D(CyberneticCharacter->AimOffset.Y, CyberneticCharacter->AimOffset.X);
		AimOffsets = FMath::Vector2DInterpTo(AimOffsets, FVector2D(bAllowTurnInPlace ? TurnInPlaceState.RootYawOffset : CyberneticCharacter->AimOffset.X, CyberneticCharacter->AimOffset.Y), DeltaSeconds, CyberneticCharacter->AimOffsetInterpSpeed);
		AimOffsetAlpha = 1.0f;
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
			AimOffsets = FVector2D(TurnInPlaceState.RootYawOffset, UKismetMathLibrary::NormalizedDeltaRotator(CurChar->GetBaseAimRotation(), CurChar->GetActorRotation()).Pitch);
		}
	}

	// needed for IK!
	ArmsOnlySlotAlpha = (GetSlotMontageGlobalWeight("ArmsOnly") == 1.0f) ? FMath::FInterpTo(ArmsOnlySlotAlpha, 0.0f, 12.0f, DeltaSeconds) : FMath::FInterpTo(ArmsOnlySlotAlpha, 1.0f, 12.0f, DeltaSeconds);
	LeftArmOnlySlotAlpha = (GetSlotMontageGlobalWeight("LeftArmOnly") == 1.0f) ? FMath::FInterpTo(LeftArmOnlySlotAlpha, 0.0f, 12.0f, DeltaSeconds) : FMath::FInterpTo(LeftArmOnlySlotAlpha, 1.0f, 12.0f, DeltaSeconds);

	/* needed for fp bob damping */
	CrouchedPelvisCurrentWorldPos = FMath::VInterpTo(CrouchedPelvisCurrentWorldPos, bIsMoving ? CrouchedPelvisMovingWorldPos : CrouchedPelvisDefaultWorldPos, DeltaSeconds, 8.0f);

	/* */
	if (CurPlayerChar)
	{
		FootIKAlpha = (CurPlayerChar->IsLocallyControlled() && CurPlayerChar->GetFirstPersonCameraComponent()) ? FMath::FInterpTo(FootIKAlpha, 0.0f, DeltaSeconds, 8.0f) : FMath::FInterpTo(FootIKAlpha, 1.0f, DeltaSeconds, 8.0f);
	}
}

UAnimSequenceBase * URoNAnimInstance_PlayerTP::GetPlayerAnimation_TP(EBaseAnimType_TP::Entries AnimName) const
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
void URoNAnimInstance_PlayerTP::LoweredCooldownDone()
{
	// just to be safe, plus get pawn
	AReadyOrNotCharacter* OwningCharacter = Cast<AReadyOrNotCharacter>(TryGetPawnOwner());
	if (OwningCharacter == nullptr)
	{
		return;
	}

	// set lowered active again
	LoweredInternalVal = 1;

	// clear timer data
	OwningCharacter->GetWorldTimerManager().ClearTimer(LoweredCooldownHandle);
}

void URoNAnimInstance_PlayerTP::CalculateDistanceMatching(float DeltaTime, APlayerCharacter* CharRef, bool bShowDebug)
{
	FAnimCharacterMovementPredictionSnapshot CurPrediction;
	CurPrediction.BrakingDecelerationWalking = CharRef->GetCharacterMovement()->BrakingDecelerationWalking;
	CurPrediction.BrakingFriction = CharRef->GetCharacterMovement()->BrakingFriction;
	CurPrediction.BrakingFrictionFactor = CharRef->GetCharacterMovement()->BrakingFrictionFactor;
	CurPrediction.bUseSeparateBrakingFriction = CharRef->GetCharacterMovement()->bUseSeparateBrakingFriction;
	CurPrediction.CapsuleHalfHeight = CharRef->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	CurPrediction.CapsuleRadius = CharRef->GetCapsuleComponent()->GetScaledCapsuleRadius();
	CurPrediction.GravityZ = CharRef->GetCharacterMovement()->GetGravityZ();
	CurPrediction.GroundFriction = CharRef->GetCharacterMovement()->GroundFriction;

	const float MinPivotAngle = 135.f;
	TArray<AActor*> ActorsToIgnore;

	UAnimDistanceMatchingLibrary::CalculateDistanceMatchingStates(DeltaTime,
		CharRef,
		CharRef->GetCharacterMovement(),
		DistanceMatchingCurrentState,
		CharMovementSnapshot,
		CurPrediction, GetWorld(),
		MinPivotAngle,
		ActorsToIgnore,
		LastActorLocation,
		PivotingCardinalDirSnapShot,
		StartMarker,
		StopMarker,
		PivotMarker,
		TakeOffMarker,
		ApexMarker,
		LandingMarker,
		bSMStartRuleset,
		bSMStopRuleset,
		false,
		bShowDebug);
}

void URoNAnimInstance_PlayerTP::CalculateDistanceMatchingAi(float DeltaTime, ACyberneticCharacter* CharRef, bool bShowDebug)
{
	FAnimCharacterMovementPredictionSnapshot CurPrediction;
	CurPrediction.BrakingDecelerationWalking = CharRef->GetCharacterMovement()->BrakingDecelerationWalking;
	CurPrediction.BrakingFriction = CharRef->GetCharacterMovement()->BrakingFriction;
	CurPrediction.BrakingFrictionFactor = CharRef->GetCharacterMovement()->BrakingFrictionFactor;
	CurPrediction.bUseSeparateBrakingFriction = CharRef->GetCharacterMovement()->bUseSeparateBrakingFriction;
	CurPrediction.CapsuleHalfHeight = CharRef->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	CurPrediction.CapsuleRadius = CharRef->GetCapsuleComponent()->GetScaledCapsuleRadius();
	CurPrediction.GravityZ = CharRef->GetCharacterMovement()->GetGravityZ();
	CurPrediction.GroundFriction = CharRef->GetCharacterMovement()->GroundFriction;

	const float MinPivotAngle = 135.f;
	TArray<AActor*> ActorsToIgnore;

	UAnimDistanceMatchingLibrary::CalculateDistanceMatchingStates(DeltaTime,
		CharRef,
		CharRef->GetCharacterMovement(),
		DistanceMatchingCurrentState,
		CharMovementSnapshot,
		CurPrediction, GetWorld(),
		MinPivotAngle,
		ActorsToIgnore,
		LastActorLocation,
		PivotingCardinalDirSnapShot,
		StartMarker,
		StopMarker,
		PivotMarker,
		TakeOffMarker,
		ApexMarker,
		LandingMarker,
		bSMStartRuleset,
		bSMStopRuleset,
		true,
		bShowDebug);
}