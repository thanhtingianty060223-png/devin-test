// Copyright Void Interactive, 2017
// Author: Alexander Mijalkovski

#pragma once

#include "Animation/AnimInstance.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/ReadyOrNotWeaponAnimData.h"
#include "Animation/AnimInstanceProxy.h"

#include "ReadyOrNotAnimInstance.generated.h"

struct FAnimCurveBufferAccess;

// used for directional animation blending

// simplified direction
UENUM(BlueprintType)
namespace EMoveDirection
{
	enum Type
	{
		F,
		L,
		R,
		B,
	};
}

// extended direction
UENUM(BlueprintType)
namespace EMoveDirectionExt
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
class UReadyOrNotAnimInstance : public UAnimInstance
{
	GENERATED_UCLASS_BODY()

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	float HeadLeadAmount = 1.0f;

	AReadyOrNotCharacter* TryGetAnyOwner();

	// use for last tick values
	virtual void NativeLastTick(float DeltaSeconds);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	FVector Velocity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	float Speed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	float SpeedHorizontal;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	float SpeedVertical;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	float MaxSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	float Direction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	float ViewPitch;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	float ViewYaw;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	bool bIsMoving;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	bool bIsInAir;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	FRotator HeadLookRotation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	FTransform ActorTransform;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	FVector VelocityLocalNormalized;

	FRotator GetBaseAimRotation();

	float CalculateDirAngle(float DeltaTime, FVector CurVel, FRotator CurActorRotation, FRotator DirRotation, float CurDirAngle);
	virtual void CalculateDirectionCase(float DeltaTime, float DirAngle);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	TEnumAsByte<EMoveDirection::Type> CurrentDirection;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	TArray<float> WalkSpeedDatabase;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	TArray<float> JogSpeedDatabase;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	TArray<float> CrouchWalkSpeedDatabase;


	/** Allows control over how quickly the directional blend should be allowed to change. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	float DirDegreesPerSecond;

	/** In radians. Between -PI and PI. 0.0 is running the way we are looking. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	float ForwardDirAngle;

	/** In radians. Between -PI and PI. 0.0 is running the way we are looking. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	float BackwardDirAngle;

	/** In radians. Between -PI and PI. 0.0 is running the way we are looking. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	float LeftDirAngle;

	/** In radians. Between -PI and PI. 0.0 is running the way we are looking. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	float RightDirAngle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	float ForwardDirDeg;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	float BackwardDirDeg;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	float LeftDirDeg;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	float RightDirDeg;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	float StrafeForwardDir;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	float StrafeBackwardDir;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	float StrafeLeftDir;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	float StrafeRightDir;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	float MovementAlpha;

	//float DirectionUpdateRate;
	//float DirectionNextUpdateTime;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	float MovementJogAlpha;

	/* Start to transition into Jog when reaching this Threshold */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	float MovementJogThreshold;

public:

	/** IK Location for left foot for match floor */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|IK")
		FVector LeftFootIKLocation;

	/** IK Location for right foot for match floor */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|IK")
		FVector RightFootIKLocation;

	/** IK Rotation for left foot for match floor surface */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|IK")
		FRotator LeftFootIKRotation;

	/** IK Rotation for right foot for match floor surface */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|IK")
		FRotator RightFootIKRotation;

	/** IK Hip offset vector only work in Z direction */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|IK")
		FVector HipOffsetVector;


	// moved up from tp instance class
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Posture")
		bool bCrouching;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
		bool bIsStopping;

	/** Head / Look At Rotation */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Head")
	FRotator HeadRotation;

	UFUNCTION(BlueprintCallable, Category = "Head Look")
	FRotator GetLookAtRotation();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	FCarryArrestedAnimState CarryArrestedAnimState;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bIsCarried;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bIsCarrying;

	

	FAnimCurveBufferAccess GetAnimationDataCurveBuffer(UAnimSequence* Animation, FName CurveName);
	float FindPositionFromDistanceCurve(FAnimCurveBufferAccess DistanceCurve, const float& Distance);
	float EvalAnimCurveBuffer(FAnimCurveBufferAccess Curve, float InTime);
	float EvalForTwoKeys(FAnimCurveBufferAccess Curve, int32 Key1, int32 Key2, const float InTime);

	
	// additon for motion block
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
	EMotionBlockType CurMotionBlock;

	// jump additions
	UPROPERTY(BlueprintReadOnly, Category = "Jump")
	bool bIsFalling;

	UPROPERTY(BlueprintReadOnly, Category = "Jump")
	bool bHasPrelanded;

	UPROPERTY(BlueprintReadOnly, Category = "Jump")
	bool bJumpRecoveryActive;

	UPROPERTY(BlueprintReadOnly, Category = "Jump")
	float JumpRecoveryAnimTime;

	UPROPERTY(BlueprintReadOnly, Category = "Jump")
	float JumpRecoveryStrength;

	UPROPERTY(BlueprintReadOnly, Category = "Jump")
	float JumpRecoveryTime;

	virtual void CalculateJump(float DeltaTime);
	void JumpRecoveryDone();

	// temp vars jump
	FTimerHandle JumpRecoveryHandle;

	virtual void CalculateDirectionExtended(float DeltaTime, FVector CurVel, FRotator CurActorRotation);

	/** In radians. Between -PI and PI. 0.0 is running the way we are looking. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Core")
	float DirAngle;

	/** In Degrees. Converted from DirAngle. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Core")
	float DirAngleDegrees;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	TEnumAsByte<EMoveDirectionExt::Type> CurrentDirectionExt;

	UFUNCTION(BlueprintCallable, Category = "Core")
	TEnumAsByte<EMoveDirectionExt::Type> GetCurrentDirectionExtFromYawAngle(float YawAngle);

	UFUNCTION(BlueprintCallable, Category = "Core")
	TEnumAsByte<EMoveDirectionExt::Type> GetOppositeDirectionExt(TEnumAsByte<EMoveDirectionExt::Type> CurrentDir);

	/* moved up to share between fp and tp */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	bool bIsTeamMLO;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	float LeftHandIKAlpha;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	float SprintAlpha;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ItemSpecfic")
	bool bIsDeployableEquipped;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ItemSpecfic")
	bool bIsPistol;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ItemSpecfic")
	bool bIsRifle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ItemSpecfic")
	bool bIsItem;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ItemSpecfic")
	bool bItemOneHanded;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ItemSpecfic")
	bool bIsC2Charge;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Direct Access Avoidance")
	bool bLevel1MovementTrigger;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Direct Access Avoidance")
	bool bLevel2MovementTrigger;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Direct Access Avoidance")
	bool bLevel3MovementTrigger;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Direct Access Avoidance")
	bool bCrouchLevel1MovementTrigger;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TurnInPlace")
	bool bRotationRateReached;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TurnInPlace")
	float DeltaRotation;

	FRotator CachedRotation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	float SprintFPAlpha;

	UFUNCTION(BlueprintCallable, Category = "SlotWeights")
	float GetWeightFromSlot(FName SlotName);

	UFUNCTION(BlueprintCallable, Category = "SlotWeights")
	float GetWeightFromSlotInversed(FName SlotName);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pelvis")
	float PelvisMovementBobAlpha;
};
