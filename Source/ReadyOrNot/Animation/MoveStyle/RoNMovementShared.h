#pragma once

#include "Animation/AimOffsetBlendSpace.h"

#include "RonMovementShared.generated.h"


/* generic to be used across all */
UENUM(BlueprintType)
enum class ERoNGaitState : uint8
{
	RON_TURN UMETA(DisplayName = "Turn"),
	RON_WALK UMETA(DisplayName = "Walk"),
	RON_RUN UMETA(DisplayName = "Run"),
	RON_SPRINT UMETA(DisplayName = "Sprint")
};

UENUM(BlueprintType)
enum class EItemOverrideRule : uint8
{
	NONE UMETA(DisplayName = "None"),
	ADDITIVE_ONLY UMETA(DisplayName = "Additive_Only"),
	LAYERED_ONLY UMETA(DisplayName = "Layered_Only"),
	ADDITIVE_LAYERED UMETA(DisplayName = "Additive_Layered")
};

USTRUCT(BlueprintType)
struct FRoNStyleIdleData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<UAnimSequence*> BaseIdleData;

	FRoNStyleIdleData()
	{
	}
};



USTRUCT(BlueprintType)
struct FRoNStyleTurnData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAimOffsetBlendSpace* AimOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* Turn45_Left;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* Turn45_Right;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* Turn90_Left;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* Turn90_Right;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* Turn180_Left;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* Turn180_Right;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* Turn135_Left;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* Turn135_Right;
	
	FRoNStyleTurnData()
	{
	}
};

USTRUCT(BlueprintType)
struct FRoNGaitTransitionData
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString Start_Section = "----START MOTIONS----";

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* Start;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* Start_45_Left;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* Start_45_Right;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* Start_90_Left;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* Start_90_Right;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* Start_180;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* Start_180_Left;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* Start_180_Right;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* Start_135_Left;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* Start_135_Right;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString Stop_Section = "----STOP MOTIONS----";
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* Stop;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* Stop_45_Left;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* Stop_45_Right;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* Stop_90_Left;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* Stop_90_Right;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* Stop_180;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* Stop_180_Left;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* Stop_180_Right;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* Stop_135_Left;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* Stop_135_Right;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString Pivot_Section = "----PIVOT MOTIONS----";
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* Pivot;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* Pivot_45_Left;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* Pivot_45_Right;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* Pivot_90_Left;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* Pivot_90_Right;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* Pivot_180;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* Pivot_180_Left;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* Pivot_180_Right;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* Pivot_135_Left;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* Pivot_135_Right;
	
	FRoNGaitTransitionData()
	{
	}
};


USTRUCT(BlueprintType)
struct FRoNLeanMotion
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* Base;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* BaseLeanLeft;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* BaseLeanRight;
	
	FRoNLeanMotion()
	{
	}
};

USTRUCT(BlueprintType)
struct FRoNGaitLocomotionData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRoNLeanMotion Fwd;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRoNLeanMotion FwdLeft;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRoNLeanMotion FwdRight;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRoNLeanMotion Left;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRoNLeanMotion Right;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRoNLeanMotion Bwd;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRoNLeanMotion BwdLeft;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRoNLeanMotion BwdRight;

	FRoNGaitLocomotionData()
	{
	}
};

/* this struct is mainly ment for the MoveData so put it here */
USTRUCT(BlueprintType)
struct FRoNGaitType
{
	GENERATED_BODY()

	/* The name of this gait entry */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName Name = "walk";
	
	// The base speed to set when using this entry
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Speed = 200.0f;

	// The base acceleration to set when using this entry
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Acceleration = 250.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRoNGaitTransitionData TransitionData;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRoNGaitLocomotionData LocomotionData;
};

USTRUCT(BlueprintType)
struct FRoNStyleSlotData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRoNStyleIdleData IdleData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRoNStyleTurnData TurnData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRoNGaitTransitionData TransitionData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRoNGaitLocomotionData LocomotionData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* IdleReference;

	/* The strafing blendspace containing walk and jog gait samples */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UBlendSpace* StrafeBSData;

	/* The non-strafing blendspace containing walk, jog and sprint samples */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UBlendSpace* NonStrafeBSData;
};

USTRUCT(BlueprintType)
struct FRoNMovementStyle
{
	GENERATED_BODY()

	// The name of this movement style
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsStrafeMovement;
	
	/* if this set is treated as lowered ( unarmed, weapon not aimed ready to shoot etc ) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsLoweredSet;

	/* how should item specific overrives be applied to this move style? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EItemOverrideRule ItemOverrideRule;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRoNStyleIdleData IdleData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRoNStyleTurnData TurnData;

	/* The possible gait entries for this style, needs to be at least 1 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (TitleProperty = "Name"))
	TArray<FRoNGaitType> GaitEntries;

	//store global blendspace refs here, contain all data for the gaitentries!

	/* The strafing blendspace containing walk and jog gait samples */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UBlendSpace* StrafeBS;

	/* The non-strafing blendspace containing walk, jog and sprint samples */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UBlendSpace* NonStrafeBS;

	FRoNMovementStyle()
	{
		Name = "default";
		GaitEntries.Empty();
		bIsStrafeMovement = false;
		ItemOverrideRule= EItemOverrideRule::NONE;
		bIsLoweredSet = false;
	}
};