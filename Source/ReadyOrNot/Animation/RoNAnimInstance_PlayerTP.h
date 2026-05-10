// Copyright Void Interactive, 2022
// Author: Alexander Mijalkovski

#pragma once

#include "Animation/ReadyOrNotAnimInstance.h"
#include "Characters/CyberneticCharacter.h"
#include "Animation/AnimDistanceMatchingLibrary.h"
#include "TurnInPlace/AnimTurnInPlaceTypes.h"
#include "RoNAnimInstance_PlayerTP.generated.h"

USTRUCT(BlueprintType, Blueprintable)
struct FReadyOrNotAnimInstanceProxyTP : public FAnimInstanceProxy
{
	GENERATED_BODY()

	FReadyOrNotAnimInstanceProxyTP() : FAnimInstanceProxy() {}
	FReadyOrNotAnimInstanceProxyTP(UAnimInstance* Instance);

	virtual void Update(float DeltaSeconds) override
	{
	}
};

UCLASS(transient, Blueprintable, hideCategories = AnimInstance, BlueprintType)
class URoNAnimInstance_PlayerTP : public UReadyOrNotAnimInstance
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Example", meta = (AllowPrivateAccess = "true"))
	FReadyOrNotAnimInstanceProxyTP Proxy;

	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override
	{
		// override this to just return the proxy on this instance
		return &Proxy;
	}

	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override
	{
	}

	friend struct FReadyOrNotAnimInstanceProxyTP;

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = Animation, meta = (BlueprintThreadSafe))
	UAnimSequenceBase * GetPlayerAnimation_TP(EBaseAnimType_TP::Entries AnimName) const;

	UPROPERTY()
	class UReadyOrNotWeaponAnimData* LastAnimWeaponData;

	// Preview entry for Anim Blueprint Editor
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UReadyOrNotWeaponAnimData* EditorWeaponAnimData = nullptr;

	UPROPERTY(BlueprintReadWrite)
	bool bAllowTurnInPlace = false;
	
	UPROPERTY(BlueprintReadWrite)
	bool bIsTurnInPlaceStateRelevant = false;

	UPROPERTY(BlueprintReadOnly)
	float YawOffsetLimit = 145.0f;
	
	UPROPERTY(BlueprintReadOnly)
	float TurnInPlaceSpeedMultiplier = 1.5f;

	UPROPERTY(BlueprintReadOnly)
	FAnimTurnInPlaceAnimSet TurnInPlaceAnimSet;
	
	UPROPERTY(BlueprintReadOnly)
	FAnimTurnInPlaceState TurnInPlaceState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turn In Place")
	FAnimTurnInPlaceAnimSet StandRifAnimSet;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turn In Place")
	FAnimTurnInPlaceAnimSet CrouchRifAnimSet;

	// Player & AI Motions
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
		bool bWeaponDown = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
		bool bIsAiming = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
		bool bIsShieldEquipped = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
		float AimingAlpha = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
		bool bLeanLeft = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
		bool bLeanRight = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
		float QuickLeanLeftAmount = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
		float QuickLeanRightAmount = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
		float QuickLeanLeftAlpha = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
		float QuickLeanRightAlpha = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
		float QuickLeanIntensity = 0.05f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
		float QuickLeanInterpSpeed = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
		float FootIKValue = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
		float FootIKAlpha;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
		bool bRagdoll = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
		bool bIsPlayingDeathAnim = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
		bool bArrested = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
		bool bIsDead = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
		bool bDeathAnimEnd = false;

	//bool bDoOnceOnDeath = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
		bool bStunned;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
		bool bTased;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
		bool bOnLadder;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
		float LadderUpDownMovement;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	float LeanAngleY;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	float LeanAngleZ;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cover")
		bool bCoverLeft;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cover")
		bool bCoverRight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cover")
		bool bCoverMiddle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cover")
		bool bCoverLeftLow;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cover")
		bool bCoverRightLow;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cover")
		bool bCoverPeek;

	/** Used code-wise to determine when to blend out the aimoffset for preventing broken poses on various actions. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	float AimOffsetAlpha;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TurnInPlace")
	FVector2D AimOffsets;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Direct Access Avoidance")
	bool bJumpStartTrigger;

	/* remapped values for leaning motions */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Direct Access Avoidance")
	float SmoothMappedLeanToAnimStandLeft;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Direct Access Avoidance")
	float SmoothMappedLeanToAnimStandRight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Direct Access Avoidance")
	float SmoothMappedLeanToAnimCrouchLeft;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Direct Access Avoidance")
	float SmoothMappedLeanToAnimCrouchRight;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Direct Access Avoidance")
		float Speed_tp_rifle_stand_sprint_f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Direct Access Avoidance")
		UAnimSequenceBase* Crouch_Idle_Pose_Low_TP;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Direct Access Avoidance")
		UAnimSequenceBase* Crouch_Idle_Pose_Up_TP;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Direct Access Avoidance")
		UAnimSequenceBase* Crouch_Idle_Pose_Shld_TP;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Direct Access Avoidance")
		UAnimSequenceBase* Crouch_Idle_Pose_Sights_TP;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Direct Access Avoidance")
		UAnimSequenceBase* Crouch_Idle_Pose_Ret_TP;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Direct Access Avoidance")
		UAnimSequenceBase* Crouch_Idle_Pose_Ovr_TP;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Direct Access Avoidance")
		UAnimSequenceBase* Idle_Pose_Low_TP;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Direct Access Avoidance")
		UAnimSequenceBase* Idle_Pose_Up_TP;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Direct Access Avoidance")
		UAnimSequenceBase* Idle_Pose_Shld_TP;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Direct Access Avoidance")
		UAnimSequenceBase* Idle_Pose_Sights_TP;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Direct Access Avoidance")
		UAnimSequenceBase* Idle_Pose_Ret_TP;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Direct Access Avoidance")
		UAnimSequenceBase* Idle_Pose_Ovr_TP;

	/* added for tp grips */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Direct Access Avoidance")
		UAnimSequenceBase* Idle_Pose_VFG_TP;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Direct Access Avoidance")
		UAnimSequenceBase* Idle_Pose_AFG_TP;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Direct Access Avoidance")
		UAnimSequenceBase* Idle_Pose_HSTOP_TP;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Direct Access Avoidance")
		bool bLeaningLeftNotCrouching;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Direct Access Avoidance")
		bool bNotLeaningLeftOrCrouching;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Direct Access Avoidance")
		bool bNotLeaningLeftOrNotCrouching;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Direct Access Avoidance")
		bool bLeaningRightNotCrouching;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Direct Access Avoidance")
		bool bNotLeaningRightOrCrouching;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Direct Access Avoidance")
		bool bNotLeaningRightOrNotCrouching;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Direct Access Avoidance")
		bool bCrouchingAndMoving;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Direct Access Avoidance")
		bool bNotCrouchingAndMoving;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Direct Access Avoidance")
		bool bAimingAndNotDeployable;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Direct Access Avoidance")
		float WalkSpeedForward;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Direct Access Avoidance")
		float WalkSpeedLeft;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Direct Access Avoidance")
		float WalkSpeedRight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Direct Access Avoidance")
		float WalkSpeedBackward;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Direct Access Avoidance")
		float CrouchWalkSpeedForward;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Direct Access Avoidance")
		float CrouchWalkSpeedLeft;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Direct Access Avoidance")
		float CrouchWalkSpeedRight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Direct Access Avoidance")
		float CrouchWalkSpeedBackward;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Direct Access Avoidance")
		float RunSpeedForward;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Direct Access Avoidance")
		float RunSpeedLeft;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Direct Access Avoidance")
		float RunSpeedRight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Direct Access Avoidance")
		float RunSpeedBackward;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Direct Access Avoidance")
		bool bLessThanPointOneSecondOnRelevantAnim;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Direct Access Avoidance")
		bool bIsInCombatOrAlerted;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Direct Access Avoidance")
		bool bMoving;

	// AI WEAPONRY GRAPH
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core | AI")
		bool bIsAlerted = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core | AI")
		float AlertAlpha;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core | AI")
		bool bIsInCombat = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core | AI")
		bool bIsSurrendering = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core | AI")
		bool bSprayed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core | AI")
		bool bStung;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core | AI")
		EPseudoSpeedType CurPseudoSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core | AI")
		bool bFemale;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core | AI")
		bool bChild;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core | AI")
		bool bHasInjury;

	// loco sync
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float AnimSpeedFwdPlayrateSync;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float AnimSpeedSidePlayrateSync;

	// new global variable, phase out old!
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float AnimSpeedPlayrateSync;


	// addition for retention/lowering/sights additive layers
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Additive")
	float RetentionAlpha;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Additive")
	float LoweredAlpha;

	int LoweredInternalVal;

	bool bIsLoweredActive;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Additive")
	float SightAlpha;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Additive")
	float LoweredCooldownTime = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Additive")
	bool IsLoweredUp;

	// temp vars lowered cooldown
	FTimerHandle LoweredCooldownHandle;
	void LoweredCooldownDone();
	bool bHasValidRetPoses;
	bool bHasValidLowPoses;
	bool bHasValidAimPoses;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Slot")
		float ArmsOnlySlotAlpha;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Slot")
		float LeftArmOnlySlotAlpha;

	/* related to stride warping */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StrideWarping")
	float VelocityInterpTime = 7.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "StrideWarping")
	FVector VelocitySmoothed;

	/* for stride warping */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Playback")
	float SpeedScaling;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Playback")
	float PlayrateClampMax;


	/* for distance matching */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "CharacterMovement")
	FAnimCharacterMovementSnapshot CharMovementSnapshot;

	/* The distance when reached to trigger the post pivot state */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pivoting")
	float PostPivotTriggerThreshold;

	UPROPERTY(BlueprintReadOnly, Category = "Pivoting")
	FCardinalDirectionSnapshot PivotingCardinalDirSnapShot;

	virtual void CalculateDistanceMatching(float DeltaTime, APlayerCharacter* CharRef, bool bShowDebug);
	virtual void CalculateDistanceMatchingAi(float DeltaTime, ACyberneticCharacter* CharRef, bool bShowDebug);

	/* Used for debugging line draw in distance matching function */
	FVector LastActorLocation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character")
	APlayerCharacter* CharacterRef;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character")
	ACyberneticCharacter* CharacterAiRef;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DistanceMatching")
	bool bUseDistanceMatching;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DistanceMatching")
	EDistanceMatchingType DistanceMatchingCurrentState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DistanceMatching")
	FPredictionResult StartMarker;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DistanceMatching")
	FPredictionResult StopMarker;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DistanceMatching")
	FPredictionResult PivotMarker;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DistanceMatching")
	FPredictionResult PostPivotMarker;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DistanceMatching")
	FPredictionResult TakeOffMarker;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DistanceMatching")
	FPredictionResult ApexMarker;

	UPROPERTY(VisibleAnywhere,BlueprintReadOnly, Category = "DistanceMatching")
	FPredictionResult LandingMarker;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DistanceMatching")
	TEnumAsByte<EMoveDirectionExt::Type> CurrentPivotDirectionExt;

	/* used to early out of pivots */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DistanceMatching")
	bool bPivotDirectionBroken;

	/* used to limit pivot directions to reduce issues with 45s/135s for now */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DistanceMatching")
	bool bCanPivotInCurDirection;

	/* used as rule for the pre-pivot transition in distance matching state machine */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DistanceMatching")
	bool bSMPrePivotRuleset;

	/* used to early out of starts */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DistanceMatching")
	bool bStartDirectionBroken;

	/* used to determine what transition set to use */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DistanceMatching")
	float ReplicatedMaxSpeed;

	/* used as rule for the start transition in distance matching state machine */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DistanceMatching")
	bool bSMStartRuleset;

	/* used as rule for the stop transition in distance matching state machine */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DistanceMatching")
	bool bSMStopRuleset;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PelvisBob")
	FVector PelvisDefaultWorldPos;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PelvisBob")
	FVector CrouchedPelvisDefaultWorldPos;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PelvisBob")
	FVector CrouchedPelvisMovingWorldPos;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PelvisBob")
	FVector CrouchedPelvisCurrentWorldPos;
};
