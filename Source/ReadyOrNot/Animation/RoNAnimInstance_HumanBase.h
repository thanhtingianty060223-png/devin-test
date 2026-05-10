// Void Interactive, 2020

#pragma once

#include "Animation/AnimInstance.h"

#include "Characters/CyberneticController.h"

#include "Animation/MoveStyle/RoNMoveStyleComponent.h"

// libs we use
#include "Animation/TurnInPlace/AnimTurnInPlaceLibrary.h"

#include "RoNAnimInstance_HumanBase.generated.h"

// extended direction
UENUM(BlueprintType)
namespace EStrafeDirection
{
	enum Type
	{
		F,
		L,
		R,
		B,
		FL,
		FR,
		BR,
		BL,
	};
}

UCLASS(transient, Blueprintable, hideCategories = AnimInstance, BlueprintType)
class URoNAnimInstance_HumanBase : public UAnimInstance
{
	GENERATED_UCLASS_BODY()

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	
	/* reference to move style component set on initialization */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataReferences)
	URoNMoveStyleComponent* MoveStyleComponent;

	void DoMoveStyleCompUpdate();

	FName ActiveMoveStyleName;
	int ActiveGaitIndex;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	bool bIsStrafing;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DataReferences")
	FRoNStyleSlotData Slot0;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DataReferences")
	FRoNStyleSlotData Slot1;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DataReferences")
	FRoNStyleSlotData Slot2;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DataReferences")
	FRoNStyleSlotData Slot3;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DataReferences")
	FRoNStyleSlotData Slot4;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DataReferences")
	FRoNStyleSlotData Slot5;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DataReferences")
	FRoNStyleSlotData Slot6;

	// used in editor preview to prevent issues
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DataReferences")
	FRoNStyleIdleData IdleData_Default;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DataReferences")
	FRoNStyleTurnData TurnData_Default;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DataReferences")
	FRoNGaitTransitionData TransitionData_Default;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DataReferences")
	FRoNGaitLocomotionData LocomotionData_Default;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DataReferences")
	UBlendSpace* StrafeBS_Default;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DataReferences")
	UBlendSpace* NonStrafeBS_Default;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BlendValues")
	float SlotBlendTime = 0.3f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BlendValues")
	float DefaultSlotBlendTime = 0.3f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BlendValues")
	float AimOffsetAlpha = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BlendValues")
	float StrafeBlendTime = 0.2f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BlendValues")
	float SlopeWarpingAlpha = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SlotControl")
	bool bIsMoveStyleSlotBActive;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SlotControl")
	int32 SlotIdx = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	FRotator HeadLookRotation;
		
	void IncrementSlotIdx();

	int32 PreviousSlotIdx = -1;
	virtual void FillMoveStyleSlots(FRoNStyleIdleData& IdleData, FRoNStyleTurnData& TurnData, FRoNGaitTransitionData& TransitionData, FRoNGaitLocomotionData& LocomotionData, UBlendSpace* StrafeBSData, UBlendSpace* NonStrafeBSData, int32 Slot);

	// use for last tick values
	virtual void NativeLastTick(float DeltaSeconds);


	/* related to lean */
	virtual void CalculateLean(ACharacter* CharRef, float DeltaSeconds);
	FRotator ActorRotation;

	UPROPERTY(BlueprintReadOnly, Category = Animation)
	float Lean;

	UPROPERTY(BlueprintReadOnly, Category = Animation)
	float LeanClamped;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animation)
	float LeanFactor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animation)
	float LeanInterpSpeed;



	UFUNCTION(BlueprintCallable, Category = RoNMoveStyle)
	virtual void SetMoveStyleDataFromComp(URoNMoveStyleComponent* MoveStyleComp);

	UFUNCTION(BlueprintCallable, Category = RoNMoveStyle)
	virtual void UpdateMoveStyleDataFromComp(URoNMoveStyleComponent* MoveStyleComp);

	UFUNCTION(BlueprintCallable, Category = RoNMoveStyle)
	virtual void UpdateGaitDataFromComp(URoNMoveStyleComponent* MoveStyleComp);

	/* adjusted by speed curve in animation */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Playback")
	float AdjustedPlayrate;

	/* for stride warping */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Playback")
	float SpeedScaling;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Playback")
	float PlayrateClampMax;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DataReferences")
	int CurrentIdleIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TEnumAsByte<EStrafeDirection::Type> CurrentStrafeDirection;

	virtual void CalculateStrafeDirection(float DeltaTime, FVector CurVel, FRotator CurActorRotation);

	/** In Radians */
	float DirAngle;

	/** In Degrees */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
	float StrafeDirectionAngle;

	/** Aim Offset from Character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
	FVector2D AimOffsetInterpolated;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	bool bInRagdoll;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	bool bRecoveringFromRagdoll;

	/** Is Character dead */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	bool bIsDead;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	bool bIsPlayingDeathAnim;

	/** Pose that gets stored when character dies */
	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
	//FPoseSnapshot DeathPose;

	/* Data related to world building activity playback */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	FWorldBuildingAnimState WorldBuildingAnimState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	FCarryArrestedAnimState CarryArrestAnimState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	FTakeHostageAnimState TakeHostageAnimState;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status")
	EAnimWeaponType CurWeaponType;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	UAnimSequence* IncapacitationLoopAnim = nullptr;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	bool bIsArrested;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	bool bIsArrestedAsRagdoll;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	bool bIsBeingArrested;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	bool bIsBeingArrestedAsRagdoll;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	bool bSurrendered;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	bool bIsCarried;
	
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
    bool bIsGetUpPlaying;

	bool bCreatedDeathPose;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	bool bIncapacitated;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	bool bEnableIKProcess;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	bool bIsFemale;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	bool bIsUnarmed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	bool bIsSWAT;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	bool bIsCrouching;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	bool bIsArrestedAndDead;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cover")
	FCoverAnimStateMachineData CoverAnimStateMachineData;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cover")
	FHidingAnimStateMachineData HidingAnimStateMachineData;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cover")
	FHoleTraversalAnimStateMachineData HoleTraversalAnimStateMachineData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
	float LeftArmIKAlpha = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
	float RightArmIKAlpha = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
	float ArmsOnlySlotAlpha = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
	float LeftArmOnlySlotAlpha = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
	float HandAdditiveLockOverride = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	bool bWeaponDown;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
	bool bIsPistolAndWeaponDown;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
	bool bIsPistol;

	float PistolLeftHandIKAlphaChange;



	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DataReferences")
	UAnimSequenceBase* Calm_Override_Pose;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DataReferences")
	UAnimSequenceBase* Aiming_Override_Pose;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DataReferences")
	UAnimSequenceBase* Default_Calm_Override_Pose;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DataReferences")
	UAnimSequenceBase* Default_Aiming_Override_Pose;



	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	bool bIsReloading;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
	float FinalAimOffsetAlpha;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	bool bAnyMontageIsActive;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	bool bFullBodyMontagePlaying;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	bool bUpperBodyMontagePlaying;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	bool bInteractionMontagePlaying;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	bool bFullOrInteractionMontagePlaying;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	EItemOverrideRule CurOverrideRule;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	bool bIsLoweredSet;

	/**/
	// turn in place related
	/**/

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TurnInPlace")
	bool bAllowTurnInPlace;

	UPROPERTY(EditAnywhere, Category = "TurnInPlace")
	float TurnInPlaceSpeedMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TurnInPlace")
	float YawOffsetLimit = 140.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TurnInPlace")
	FAnimTurnInPlaceAnimSet TurnInPlaceAnimSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TurnInPlace")
	FAnimTurnInPlaceState TurnInPlaceState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TurnInPlace")
	bool bExitTurnRecoveryIfMoving;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TurnInPlace")
	FCachedAnimStateData TurnAnimStateData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TurnInPlace")
	bool bIsTurnInPlaceStateRelevant;

	virtual void FillTurnInPlaceAnimSetFromTurnData(FRoNStyleTurnData& TurnData);

	/* related to stride warping */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StrideWarping")
	float VelocityInterpTime = 7.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "StrideWarping")
	FVector VelocitySmoothed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Overrides")
	bool bDisableAdditiveOverrides;

	/* additive ignore rulesets */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Additive")
	TArray<FString> AdditiveDisableTagList;

	/* for strafe blendspace support */
	virtual void CalcStrafeBSValues(float DeltaTime, ACharacter* CharRef, URoNMoveStyleComponent* MStyleComponent);
	virtual void CalcNonStrafeBSValues(float DeltaTime, ACharacter* CharRef, URoNMoveStyleComponent* MStyleComponent);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
	float move_x;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
	float move_y;

	/* specific ik rules during hostage taking */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	bool bIsHostageTaker;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	bool bIsHostage;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	bool bIsHostageOrHostageTaker;

	/* added to be able to turn off IK and Aim Offset during move style blends */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	float MoveStyleBlendCoolDown = 0.25f;

	/** How long the system has been waiting to reset move style blend boolean. */
	float MoveStyleBlendResetCounter = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	bool bMoveStyleChanging;

	virtual void TriggerMoveStyleChangeStatus();
	virtual void ResetMoveStyleChangeStatus();
};
