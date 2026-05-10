// Copyright Void Interactive, 2021
// Author: Alexander Mijalkovski

// Core of managing animationa assets for player motions

#pragma once

#include "Engine/DataAsset.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
// ##UE5UPGRADE##
#include "Animation/BlendSpace.h"
#include "ReadyOrNotWeaponAnimData.generated.h"

/* used to determine current active motion block */
UENUM(BlueprintType)
enum class EMotionBlockType : uint8
{
	MB_None,
	MB_Rifle,
	MB_Pistol,
	MB_Item,
	MB_HeavyItem,
	MB_Special,
	MB_Unarmed
};

UENUM(BlueprintType)
namespace EBaseAnimType_FP
{
	enum Entries
	{
		IdlePose_FP,
		Idle_FP,
		Run_FP,
		Walk_FP,
		Run_Limp_FP,
		Walk_Limp_FP,
		Lowered_Up_Pose_FP,
		Lowered_Down_Pose_FP,
		ADS_Run_FP,
		ADS_Walk_FP,
		ADS_Run_Limp_FP,
		ADS_Walk_Limp_FP,
		IdlePose_AFG_FP,
		IdlePose_VFG_FP,
		IdlePose_HSTOP_FP,
		ENone
	};
}

UENUM(BlueprintType)
namespace EBaseBlendspaces_FP
{
	enum Entries
	{
		Look_BS_FP,
		ENone
	};
}

UENUM(BlueprintType)
namespace EBaseAnimType_TP
{
	enum Entries
	{
		IdlePose_Low_TP,
		IdlePose_Up_TP,
		IdlePose_Shld_TP,
		IdlePose_Sights_TP,
		IdlePose_Ret_TP,
		IdlePose_Ovr_TP,
		Crouch_IdlePose_Low_TP,
		Crouch_IdlePose_Up_TP,
		Crouch_IdlePose_Shld_TP,
		Crouch_IdlePose_Sights_TP,
		Crouch_IdlePose_Ret_TP,
		Crouch_IdlePose_Ovr_TP,
		IdlePose_AFG_TP,
		IdlePose_VFG_TP,
		IdlePose_HSTOP_TP,
		ENone
	};
}

UENUM(BlueprintType)
enum EAnimationType
{
	AT_Gun_FP,
	AT_Gun_TP,
	AT_Body_FP,
	AT_Body_TP
};

/*
This struct combines everything you need to play a Weapon Animation Montage on all related Components
*/
USTRUCT(BlueprintType)
struct FWeaponAnim
{
	GENERATED_USTRUCT_BODY()

	/* Animation that plays on the Firstperson Body */
	UPROPERTY(EditAnywhere, Category = Animation)
	UAnimMontage* Body_FP;

	/* Animation that plays on the Thirdperson Body */
	UPROPERTY(EditAnywhere, Category = Animation)
	UAnimMontage* Body_TP;

	/* Animation that plays on the FirstPerson Gun */
	UPROPERTY(EditAnywhere, Category = Animation)
	UAnimMontage* Gun_FP;

	/* Animation that plays on the ThirdPerson Gun */
	UPROPERTY(EditAnywhere, Category = Animation)
	UAnimMontage* Gun_TP;
};

/*
The Data asset which gets referenced by each Weapon to set the correct Animations required

Can be extended and all existing derived blueprints will update
*/
UCLASS()
class UReadyOrNotWeaponAnimData : public UDataAsset
{
	GENERATED_BODY()

public:

	UReadyOrNotWeaponAnimData();

	// FIRSTPERSON BASE

	UPROPERTY(EditAnywhere, Category = Base_FP)
	UAnimSequenceBase* IdlePose_FP;

	UPROPERTY(EditAnywhere, Category = Base_FP)
	UAnimSequenceBase* Idle_FP;

	UPROPERTY(EditAnywhere, Category = Base_FP)
	UAnimSequenceBase* Run_FP;

	UPROPERTY(EditAnywhere, Category = Base_FP)
	UAnimSequenceBase* Walk_FP;

	UPROPERTY(EditAnywhere, Category = Base_FP)
	UAnimSequenceBase* Run_Limp_FP;

	UPROPERTY(EditAnywhere, Category = Base_FP)
	UAnimSequenceBase* Walk_Limp_FP;

	// ##UE5UPGRADE## Compatility
	UPROPERTY(EditAnywhere, Category = Base_FP)
	UBlendSpace* Look_BS_FP;

	UPROPERTY(EditAnywhere, Category = Base_FP)
	UAnimSequenceBase* Lowered_Up_Pose_FP;

	UPROPERTY(EditAnywhere, Category = Base_FP)
	UAnimSequenceBase* Lowered_Down_Pose_FP;

	UPROPERTY(EditAnywhere, Category = Base_FP)
	UAnimSequenceBase* ADS_Run_FP;

	UPROPERTY(EditAnywhere, Category = Base_FP)
	UAnimSequenceBase* ADS_Walk_FP;

	UPROPERTY(EditAnywhere, Category = Base_FP)
	UAnimSequenceBase* ADS_Run_Limp_FP;

	UPROPERTY(EditAnywhere, Category = Base_FP)
	UAnimSequenceBase* ADS_Walk_Limp_FP;

	// additions for underbarrel pose changes
	UPROPERTY(EditAnywhere, Category = Base_FP)
	UAnimSequenceBase* IdlePose_AFG_FP;

	UPROPERTY(EditAnywhere, Category = Base_FP)
	UAnimSequenceBase* IdlePose_VFG_FP;

	UPROPERTY(EditAnywhere, Category = Base_FP)
	UAnimSequenceBase* IdlePose_HSTOP_FP;


	// THIRDPERSON BASE

	UPROPERTY(EditAnywhere, Category = Base_TP)
	UAnimSequenceBase* IdlePose_Low_TP;

	// added
	UPROPERTY(EditAnywhere, Category = Base_TP)
	UAnimSequenceBase* IdlePose_Up_TP;

	UPROPERTY(EditAnywhere, Category = Base_TP)
	UAnimSequenceBase* IdlePose_Shld_TP;

	UPROPERTY(EditAnywhere, Category = Base_TP)
	UAnimSequenceBase* IdlePose_Sights_TP;

	// added
	UPROPERTY(EditAnywhere, Category = Base_TP)
	UAnimSequenceBase* IdlePose_Ret_TP;

	// added
	UPROPERTY(EditAnywhere, Category = Base_TP)
	UAnimSequenceBase* IdlePose_Ovr_TP;

	UPROPERTY(EditAnywhere, Category = Base_TP)
	UAnimSequenceBase* Crouch_IdlePose_Low_TP;

	// added
	UPROPERTY(EditAnywhere, Category = Base_TP)
	UAnimSequenceBase* Crouch_IdlePose_Up_TP;

	UPROPERTY(EditAnywhere, Category = Base_TP)
	UAnimSequenceBase* Crouch_IdlePose_Shld_TP;

	UPROPERTY(EditAnywhere, Category = Base_TP)
	UAnimSequenceBase* Crouch_IdlePose_Sights_TP;

	// added
	UPROPERTY(EditAnywhere, Category = Base_TP)
	UAnimSequenceBase* Crouch_IdlePose_Ret_TP;

	// added
	UPROPERTY(EditAnywhere, Category = Base_TP)
	UAnimSequenceBase* Crouch_IdlePose_Ovr_TP;

	// Montage related Animations for FP and TP start here

	UPROPERTY(EditAnywhere, Category = Montage_Reload)
		FWeaponAnim Reload;

	UPROPERTY(EditAnywhere, Category = Montage_Reload)
		FWeaponAnim ReloadEmpty;

	UPROPERTY(EditAnywhere, Category = Montage_Crouch_Reload)
		FWeaponAnim Crouch_Reload;

	UPROPERTY(EditAnywhere, Category = Montage_Crouch_Reload)
		FWeaponAnim Crouch_ReloadEmpty;

	UPROPERTY(EditAnywhere, Category = Montage_Reload)
	FWeaponAnim Tactical_Reload;

	UPROPERTY(EditAnywhere, Category = Montage_Reload)
	FWeaponAnim Tactical_ReloadEmpty;

	// Reloading the weapon from the shell rack
	UPROPERTY(EditAnywhere, Category = Montage_ShellRackReload)
	TArray<FWeaponAnim> ShellRack_Reload;

	// Reloading the weapon from the shell rack, when the weapon is empty
	UPROPERTY(EditAnywhere, Category = Montage_ShellRackReload)
	TArray<FWeaponAnim> ShellRack_ReloadEmpty;

	// Reloading the shell rack
	UPROPERTY(EditAnywhere, Category = Montage_ShellRackReload)
	TArray<FWeaponAnim> ShellRack_ReloadRack;

	UPROPERTY(EditAnywhere, Category = Montage_Crouch_Reload)
	FWeaponAnim Tactical_Crouch_Reload;

	UPROPERTY(EditAnywhere, Category = Montage_Crouch_Reload)
	FWeaponAnim Tactical_Crouch_ReloadEmpty;

	// Melee
	UPROPERTY(EditAnywhere, Category = Montage_Melee)
	FWeaponAnim MeleeHit;
	
	UPROPERTY(EditAnywhere, Category = Montage_Melee)
	FWeaponAnim MeleeMiss;

	UPROPERTY(EditAnywhere, Category = Montage_FireSelect)
		FWeaponAnim FireSelect_Auto;

	UPROPERTY(EditAnywhere, Category = Montage_FireSelect)
		FWeaponAnim FireSelect_Burst;

	UPROPERTY(EditAnywhere, Category = Montage_FireSelect)
		FWeaponAnim FireSelect_Semi;

	UPROPERTY(EditAnywhere, Category = Montage_FireSelect)
		FWeaponAnim FireSelect_Safe;

	UPROPERTY(EditAnywhere, Category = Montage_Crouch_FireSelect)
		FWeaponAnim Crouch_FireSelect_Auto;

	UPROPERTY(EditAnywhere, Category = Montage_Crouch_FireSelect)
		FWeaponAnim Crouch_FireSelect_Burst;

	UPROPERTY(EditAnywhere, Category = Montage_Crouch_FireSelect)
		FWeaponAnim Crouch_FireSelect_Semi;

	UPROPERTY(EditAnywhere, Category = Montage_Crouch_FireSelect)
		FWeaponAnim Crouch_FireSelect_Safe;

	UPROPERTY(EditAnywhere, Category = Montage_SingleReload)
	FWeaponAnim Reload_Start;

	UPROPERTY(EditAnywhere, Category = Montage_SingleReload)
	FWeaponAnim Reload_Loop;

	UPROPERTY(EditAnywhere, Category = Montage_SingleReload)
	FWeaponAnim Reload_End;

	UPROPERTY(EditAnywhere, Category = Montage_SingleReload)
	FWeaponAnim Reload_Start_Empty;

	UPROPERTY(EditAnywhere, Category = Montage_SingleReload)
	FWeaponAnim Reload_Loop_Empty;

	UPROPERTY(EditAnywhere, Category = Montage_SingleReload)
	FWeaponAnim Reload_End_Empty;

	UPROPERTY(EditAnywhere, Category = Montage_Crouch_SingleReload)
	FWeaponAnim Crouch_Reload_Start;

	UPROPERTY(EditAnywhere, Category = Montage_Crouch_SingleReload)
	FWeaponAnim Crouch_Reload_Loop;

	UPROPERTY(EditAnywhere, Category = Montage_Crouch_SingleReload)
	FWeaponAnim Crouch_Reload_End;

	UPROPERTY(EditAnywhere, Category = Montage_Crouch_SingleReload)
	FWeaponAnim Crouch_Reload_Start_Empty;

	UPROPERTY(EditAnywhere, Category = Montage_Crouch_SingleReload)
	FWeaponAnim Crouch_Reload_Loop_Empty;

	UPROPERTY(EditAnywhere, Category = Montage_Crouch_SingleReload)
	FWeaponAnim Crouch_Reload_End_Empty;

	UPROPERTY(EditAnywhere, Category = Montage_MagCheck)
		FWeaponAnim MagazineCheck;

	UPROPERTY(EditAnywhere, Category = Montage_MagCheck)
		FWeaponAnim Crouch_MagazineCheck;

	UPROPERTY(EditAnywhere, Category = Montage_MagCheck)
		FWeaponAnim MagazineCheckSights;

	UPROPERTY(EditAnywhere, Category = Montage_MagCheck)
		FWeaponAnim Crouch_MagazineCheckSights;

	// fire single stuff
	UPROPERTY(EditAnywhere, Category = Montage_FireSingle)
	TArray<FWeaponAnim> FireSingle;

	UPROPERTY(EditAnywhere, Category = Montage_FireSingle)
	TArray<FWeaponAnim> FireSingleSights;

	UPROPERTY(EditAnywhere, Category = Montage_FireSingle)
	FWeaponAnim FireSingleLast;

	UPROPERTY(EditAnywhere, Category = Montage_FireSingle)
	FWeaponAnim FireSingleSightsLast;

	UPROPERTY(EditAnywhere, Category = Montage_FireDry)
	FWeaponAnim DryFire;

	UPROPERTY(EditAnywhere, Category = Montage_Crouch_FireSingle)
	TArray<FWeaponAnim> Crouch_FireSingle;

	UPROPERTY(EditAnywhere, Category = Montage_Crouch_FireSingle)
	TArray<FWeaponAnim> Crouch_FireSingleSights;

	UPROPERTY(EditAnywhere, Category = Montage_Crouch_FireSingle)
	FWeaponAnim Crouch_FireSingleLast;

	UPROPERTY(EditAnywhere, Category = Montage_Crouch_FireSingle)
	FWeaponAnim Crouch_FireSingleSightsLast;

	UPROPERTY(EditAnywhere, Category = Montage_Crouch_FireDry)
	FWeaponAnim Crouch_Dryfire;

	// loop firing
	// Combine FireLoop into FireLoopStart + FireLoop (looping)
	//UPROPERTY(EditAnywhere, Category = Montage_FireLoop)
	//FWeaponAnim FireLoopStart;

	UPROPERTY(EditAnywhere, Category = Montage_FireLoop)
	FWeaponAnim FireLoop;

	UPROPERTY(EditAnywhere, Category = Montage_FireLoop)
	FWeaponAnim FireLoopEnd;

	// loop firing
	// Combine FireLoop into FireLoopStart + FireLoop (looping)
// 	UPROPERTY(EditAnywhere, Category = Montage_FireLoop)
// 	FWeaponAnim FireLoopSightsStart;

	UPROPERTY(EditAnywhere, Category = Montage_FireLoop)
	FWeaponAnim FireLoopSights;

	UPROPERTY(EditAnywhere, Category = Montage_FireLoop)
	FWeaponAnim FireLoopSightsEnd;

	// loop firing
	// Combine FireLoop into FireLoopStart + FireLoop (looping)
// 	UPROPERTY(EditAnywhere, Category = Montage_Crouch_FireLoop)
// 	FWeaponAnim Crouch_FireLoopStart;

	UPROPERTY(EditAnywhere, Category = Montage_Crouch_FireLoop)
	FWeaponAnim Crouch_FireLoop;

	UPROPERTY(EditAnywhere, Category = Montage_Crouch_FireLoop)
	FWeaponAnim Crouch_FireLoopEnd;

	// loop firing
	// Combine FireLoop into FireLoopStart + FireLoop (looping)
// 	UPROPERTY(EditAnywhere, Category = Montage_Crouch_FireLoop)
// 	FWeaponAnim Crouch_FireLoopSightsStart;

	UPROPERTY(EditAnywhere, Category = Montage_Crouch_FireLoop)
	FWeaponAnim Crouch_FireLoopSights;

	UPROPERTY(EditAnywhere, Category = Montage_Crouch_FireLoop)
	FWeaponAnim Crouch_FireLoopSightsEnd;


	UPROPERTY(EditAnywhere, Category = Montage_Draw)
	FWeaponAnim Draw;

	UPROPERTY(EditAnywhere, Category = Montage_Draw)
	FWeaponAnim DrawFirst;

	UPROPERTY(EditAnywhere, Category = Montage_Crouch_Draw)
	FWeaponAnim Crouch_Draw;

	UPROPERTY(EditAnywhere, Category = Montage_Crouch_Draw)
	FWeaponAnim Crouch_DrawFirst;


	UPROPERTY(EditAnywhere, Category = Montage_Holster)
	FWeaponAnim Holster;

	UPROPERTY(EditAnywhere, Category = Montage_Crouch_Holster)
	FWeaponAnim Crouch_Holster;


	// grenade refactor start
	UPROPERTY(EditAnywhere, Category = Montage_Grenade)
	FWeaponAnim PullPin;

	UPROPERTY(EditAnywhere, Category = Montage_Grenade)
	FWeaponAnim Throw;

	UPROPERTY(EditAnywhere, Category = Montage_Grenade)
	FWeaponAnim PullPinUnderarm;

	UPROPERTY(EditAnywhere, Category = Montage_Grenade)
	FWeaponAnim ThrowUnderarm;


	UPROPERTY(EditAnywhere, Category = Montage_Crouch_Grenade)
	FWeaponAnim Crouch_PullPin;

	UPROPERTY(EditAnywhere, Category = Montage_Crouch_Grenade)
	FWeaponAnim Crouch_Throw;

	UPROPERTY(EditAnywhere, Category = Montage_Crouch_Grenade)
	FWeaponAnim Crouch_PullPinUnderarm;

	UPROPERTY(EditAnywhere, Category = Montage_Crouch_Grenade)
	FWeaponAnim Crouch_ThrowUnderarm;

	UPROPERTY(EditAnywhere, Category = Montage_Grenade)
		FWeaponAnim QuickThrow_PinPull;

	UPROPERTY(EditAnywhere, Category = Montage_Grenade)
		FWeaponAnim QuickThrow_Throw;

	UPROPERTY(EditAnywhere, Category = Montage_Crouch_Grenade)
		FWeaponAnim Crouch_QuickThrow_PinPull;

	UPROPERTY(EditAnywhere, Category = Montage_Crouch_Grenade)
		FWeaponAnim Crouch_QuickThrow_Throw;

	UPROPERTY(EditAnywhere, Category = Montage_Grenade)
		FWeaponAnim QuickThrow_Fast;

	UPROPERTY(EditAnywhere, Category = Montage_Crouch_Grenade)
		FWeaponAnim Crouch_QuickThrow_Fast;

	// Only for the Multitool
	UPROPERTY(EditAnywhere, Category = Montage_Multitool)
	FWeaponAnim Multitool_Use;

	UPROPERTY(EditAnywhere, Category = Montage_Multitool)
	FWeaponAnim Multitool_Use_End;

	UPROPERTY(EditAnywhere, Category = Montage_Multitool)
	FWeaponAnim Multitool_Cutters_To_Lockpick;
	UPROPERTY(EditAnywhere, Category = Montage_Multitool)
	FWeaponAnim Multitool_Cutters_To_Knife;
	UPROPERTY(EditAnywhere, Category = Montage_Multitool)
	FWeaponAnim Multitool_Lockpick_To_Cutters;
	UPROPERTY(EditAnywhere, Category = Montage_Multitool)
	FWeaponAnim Multitool_Lockpick_To_Knife;
	UPROPERTY(EditAnywhere, Category = Montage_Multitool)
	FWeaponAnim Multitool_Knife_To_Lockpick;
	UPROPERTY(EditAnywhere, Category = Montage_Multitool)
	FWeaponAnim Multitool_Knife_To_Cutters;

	// Optiwand Montages
	UPROPERTY(EditAnywhere, Category = Montage_Optiwand)
	FWeaponAnim Optiwand_Start_Screening;

	UPROPERTY(EditAnywhere, Category = Montage_Optiwand)
	FWeaponAnim Optiwand_End_Screening;

	// C2 Charge Montages
	UPROPERTY(EditAnywhere, Category = Montage_C2)
	FWeaponAnim Charge_Valid_Plant_Start;

	UPROPERTY(EditAnywhere, Category = Montage_C2)
	FWeaponAnim Charge_Valid_Plant_End;

	UPROPERTY(EditAnywhere, Category = Montage_C2)
	FWeaponAnim PlantCharge;

	// C2 Clacker Montages
	UPROPERTY(EditAnywhere, Category = Montage_C2)
	FWeaponAnim DetonateCharge;


	// NVG STUFF
	UPROPERTY(EditAnywhere, Category = Montage_NVG)
	FWeaponAnim EnableNVG;

	UPROPERTY(EditAnywhere, Category = Montage_NVG)
	FWeaponAnim DisableNVG;

	// Shield stuff
	UPROPERTY(EditAnywhere, Category = Montage_Shield)
	FWeaponAnim ShieldDownToUp;

	UPROPERTY(EditAnywhere, Category = Montage_Shield)
	FWeaponAnim ShieldUpToDown;

	UPROPERTY(EditAnywhere, Category = Montage_Shield)
	FWeaponAnim Crouch_ShieldDownToUp;

	UPROPERTY(EditAnywhere, Category = Montage_Shield)
	FWeaponAnim Crouch_ShieldUpToDown;

	UPROPERTY(EditAnywhere, Category = Montage_Shield)
	FWeaponAnim ShieldHit;

	// Tablet stuff
	UPROPERTY(EditAnywhere, Category = Montage_Tablet)
		FWeaponAnim TabletDownToUp;

	UPROPERTY(EditAnywhere, Category = Montage_Tablet)
		FWeaponAnim TabletUpToDown;

	UPROPERTY(EditAnywhere, Category = Montage_Tablet)
		FWeaponAnim TabletSwitchCameraDown;

	UPROPERTY(EditAnywhere, Category = Montage_Tablet)
		FWeaponAnim TabletSwitchCameraUp;

	// Optional interactions
	UPROPERTY(EditAnywhere, Category = Montage_Optional_Interactions)
		FWeaponAnim EvidencePickup;

	UPROPERTY(EditAnywhere, Category = Montage_Optional_Interactions)
		FWeaponAnim Yell;

	UPROPERTY(EditAnywhere, Category = Montage_Optional_Interactions)
		FWeaponAnim RadioSelect;

	UPROPERTY(EditAnywhere, Category = Montage_Optional_Interactions)
		bool bRadioUsesNotifies = false;

	UPROPERTY(EditAnywhere, Category = Montage_Optional_Interactions)
		FWeaponAnim DoorPush;

	UPROPERTY(EditAnywhere, Category = Montage_Optional_Interactions)
		FWeaponAnim ButtonPush;

	UPROPERTY(EditAnywhere, Category = Montage_Optional_Interactions)
		FWeaponAnim WeaponClearing;

	// Reactions
	UPROPERTY(EditAnywhere, Category = Montage_Reactions)
		FWeaponAnim ReactToSting;

	UPROPERTY(EditAnywhere, Category = Montage_Reactions)
		FWeaponAnim ReactToFlash;

	UPROPERTY(EditAnywhere, Category = Montage_Reactions)
		FWeaponAnim ReactToTaser;

	UPROPERTY(EditAnywhere, Category = Montage_Reactions)
		FWeaponAnim ReactToGas;

	UPROPERTY(EditAnywhere, Category = Montage_Reactions)
		FWeaponAnim ReactToPepperSpray;

	// ending additions
	UPROPERTY(EditAnywhere, Category = Montage_Reactions)
		FWeaponAnim ReactToSting_End;

	UPROPERTY(EditAnywhere, Category = Montage_Reactions)
		FWeaponAnim ReactToFlash_End;

	UPROPERTY(EditAnywhere, Category = Montage_Reactions)
		FWeaponAnim ReactToTaser_End;

	UPROPERTY(EditAnywhere, Category = Montage_Reactions)
		FWeaponAnim ReactToGas_End;

	UPROPERTY(EditAnywhere, Category = Montage_Reactions)
		FWeaponAnim ReactToPepperSpray_End;


	UPROPERTY(EditAnywhere, Category = AI_Weapon_Actions)
	FWeaponAnim Reload_Level_01;

	UPROPERTY(EditAnywhere, Category = AI_Weapon_Actions)
	FWeaponAnim Reload_Level_02;

	UPROPERTY(EditAnywhere, Category = AI_Weapon_Actions)
	FWeaponAnim Reload_Level_03;

	UPROPERTY(EditAnywhere, Category = AI_Weapon_Actions)
	FWeaponAnim Recoil_Level_01;

	UPROPERTY(EditAnywhere, Category = AI_Weapon_Actions)
	FWeaponAnim Recoil_Level_02;

	UPROPERTY(EditAnywhere, Category = AI_Weapon_Actions)
	FWeaponAnim Recoil_Level_03;



	/* grip overrides for firing when needed */

	///////////////////////////////////////////////////////////////

	// what is actually exposed here
	UPROPERTY(EditAnywhere, Category = Grip_Override)
	UAnimMontage* Grip_VFG_Body_FP_Fire;

	UPROPERTY(EditAnywhere, Category = Grip_Override)
	UAnimMontage* Grip_VFG_Body_FP_Fire_Last;

	UPROPERTY(EditAnywhere, Category = Grip_Override)
	UAnimMontage* Grip_VFG_Body_FP_Fire_Aim;

	UPROPERTY(EditAnywhere, Category = Grip_Override)
	UAnimMontage* Grip_VFG_Body_FP_Fire_Aim_Last;

	UPROPERTY(EditAnywhere, Category = Grip_Override)
	UAnimMontage* Grip_AFG_Body_FP_Fire;

	UPROPERTY(EditAnywhere, Category = Grip_Override)
	UAnimMontage* Grip_AFG_Body_FP_Fire_Last;

	UPROPERTY(EditAnywhere, Category = Grip_Override)
	UAnimMontage* Grip_AFG_Body_FP_Fire_Aim;

	UPROPERTY(EditAnywhere, Category = Grip_Override)
	UAnimMontage* Grip_AFG_Body_FP_Fire_Aim_Last;

	///////////////////////////////////////////////////////////////

	/* grip overrides end*/






	/* ADS firstperson reload actions start */
	///////////////////////////////////////////////////////////////


	UPROPERTY(EditAnywhere, Category = ADS_Reload)
	UAnimMontage* Reload_FP_Ads;

	UPROPERTY(EditAnywhere, Category = ADS_Reload)
	UAnimMontage* ReloadEmpty_FP_Ads;

	UPROPERTY(EditAnywhere, Category = ADS_Reload)
	UAnimMontage* Tactical_Reload_FP_Ads;

	UPROPERTY(EditAnywhere, Category = ADS_Reload)
	UAnimMontage* Tactical_ReloadEmpty_FP_Ads;

		
	///////////////////////////////////////////////////////////////
	/* ADS firstperson reload actions end */


	/** Empty Reload supported? */
	UPROPERTY(EditAnywhere, Category = Settings)
	bool bEmptyReload;

	/* to support the new overrides without breaking existing weapons! */
	UPROPERTY(EditAnywhere, Category = Settings)
	bool bHasRetentionAdditives;

	UPROPERTY(EditAnywhere, Category = Settings)
	bool bHasLoweredAdditives;

	UPROPERTY(EditAnywhere, Category = Settings)
	bool bHasSightAdditives;

	/* additions to have ability to override fire and reload during grip attachments */
	UPROPERTY(EditAnywhere, Category = Settings)
	bool bOverrideFireAnimForGrip;

	// todo: do we need this?
	//UPROPERTY(EditAnywhere, Category = Settings)
	//bool bOverrideReloadAnimForGrip;

	/* TP Grip poses */

	// additions for underbarrel pose changes
	UPROPERTY(EditAnywhere, Category = Base_TP)
	UAnimSequenceBase* IdlePose_AFG_TP;

	UPROPERTY(EditAnywhere, Category = Base_TP)
	UAnimSequenceBase* IdlePose_VFG_TP;

	UPROPERTY(EditAnywhere, Category = Base_TP)
	UAnimSequenceBase* IdlePose_HSTOP_TP;

	/* AI Hand/Finger Overrides */
	UPROPERTY(EditAnywhere, Category = AI_Overrides)
	UAnimSequenceBase* IdlePose_AI_Calm;

	UPROPERTY(EditAnywhere, Category = AI_Overrides)
	UAnimSequenceBase* IdlePose_AI_Aiming;
};
