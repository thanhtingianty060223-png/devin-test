// Copyright Void Interactive, 2023

#pragma once

#include "Engine.h"

UENUM(BlueprintType)
enum class ETeamType : uint8
{
	TT_NONE			UMETA(DisplayName="No Team"),
	TT_SERT_RED		UMETA(DisplayName="Red Team"),
	TT_SERT_BLUE	UMETA(DisplayName="Blue Team"),
	TT_SUSPECT		UMETA(DisplayName="Suspect"),
	TT_CIVILIAN		UMETA(DisplayName="Civilian"),
	TT_SQUAD		UMETA(DisplayName="Gold Team") // TT_SQUAD is treated as Element/Gold Team. But SWAT have an actual team type of TT_SERT_RED or TT_SERT_BLUE.
};

UENUM(BlueprintType)
enum class ESurrenderExitType : uint8
{
	None,
	Default,
	Gun,
	Knife
};

UENUM(BlueprintType)
enum class ESessionType : uint8
{
	ST_None,
	ST_SinglePlayer,
	ST_Public,
	ST_Friends
};

UENUM(BlueprintType)
enum class ECOOPMode : uint8
{
	CM_None,
	CM_BombThreat, // barricaded suspects but with a bomb threat
	CM_ActiveShooter, // single shooter, killing civilians
	CM_HostageRescue, // group of shooters, group of civilians, find them quietly and then engage
	CM_BarricadedSuspects, // default mode
	CM_Raid // baricaded suspects, no ROE	
};

UENUM(BlueprintType)
enum class EGameVersionRestriction : uint8
{
	GVR_NoRestriction				UMETA(DisplayName = "No Resiction (Any Version)"), 
	GVR_Base						UMETA(DisplayName = "Base Fullgame"), // standard full name
	GVR_Supporter				    UMETA(DisplayName = "Supporter Edition (DLC)"),
	GVR_PreorderBonus			    UMETA(DisplayName = "Preorder (DLC)"),
	GVR_Demo = MAX_uint8			UMETA(DisplayName = "Demo Version"), // used to denote demo mode, cant have other DLC / Addons if this is here
};

UENUM(BlueprintType)
enum class EGameFeature : uint8
{
	GF_None,
	GF_Practice,
	GF_Training,
	GF_Commander,
	GF_Mulitplayer,
	GF_ModSupport,

	MAX UMETA(Hidden)
};
ENUM_RANGE_BY_COUNT(EGameFeature, EGameFeature::MAX);

UENUM(BlueprintType)
enum class ECombatState : uint8
{
	CS_Unaware, // Unaware
	CS_Suspicious, // Suspicious (has fleed and is hiding)
	CS_Fleeing, // Actively fleeing
	CS_Hesitation, // Stuck in one way room, hesitate or move to armed and dangerous or surrender/fake surrender
	CS_Cover, // covering from enemy (actively engaging in combat)
	CS_ArmedAndDangerous // armed and dangerous (actively engaging in combat)
};

UENUM(BlueprintType)
enum class EOptiwandViewMode : uint8
{
	PiP,
	Fullscreen
};

UENUM(BlueprintType)
enum class EToggleInventoryVis : uint8
{
	TIV_None,
	TIV_HideAll,
	TIV_ShowAll,
	TIV_HideEquipped,
	TIV_ShowEquipped
};

UENUM(BlueprintType)
enum class EItemVisualizationStatus : uint8
{
	IVS_None		UMETA(DisplayName = "Non Item Status"),
	IVS_FPEquipped	UMETA(DisplayName = "First Person Equipped (Hands FP)"),
};

UENUM(BlueprintType)
enum class EItemVisualizationType : uint8
{
	IVT_None,
	IVT_Primary,
	IVT_Secondary,
	IVT_LongTactical,
	IVT_Helmet,
	IVT_Armor,
	IVT_Equipped
};

UENUM(BlueprintType)
enum class EAnimWeaponType : uint8
{
	CWT_Unarmed,
	CWT_Pistol,
	CWT_Rifle,
	CWT_Arrested, // aka cuffed
	CWT_Surrendered, // aka comply
	CWT_Any // play if any match
};

static FString AnimWeaponTypeToString(const EAnimWeaponType& InAnimWeaponType)
{
	switch (InAnimWeaponType)
	{
		case EAnimWeaponType::CWT_Unarmed: return "Unarmed";
		case EAnimWeaponType::CWT_Pistol: return "Pistol";
		case EAnimWeaponType::CWT_Rifle: return "Rifle";
		case EAnimWeaponType::CWT_Arrested: return "Arrested";
		case EAnimWeaponType::CWT_Surrendered: return "Surrendered";
		case EAnimWeaponType::CWT_Any: return "Any";
		default: return "None";
	}
}

UENUM(BlueprintType)
enum class EStackupGenArea : uint8
{
	SGA_None,
	SGA_FrontLeft,
	SGA_FrontRight,
	SGA_BackLeft,
	SGA_BackRight,
	SGA_All
};

UENUM()
enum class EDoorRoomPosition : uint8
{
	Center,
	CornerLeft,
	CornerRight,
	Hallway,
	HallwayLeft,
	HallwayRight,
};

UENUM(BlueprintType)
enum class EStackUpStyle : uint8
{
	Auto,
	Split,
	Left,
	Right
};

UENUM(BlueprintType)
enum class EThresholdAssessment : uint8
{
	None,
	Pie,
	CenterCheck
};

UENUM(BlueprintType)
enum class EEntryMethod : uint8
{
	Flow,
	ButtonHook,
};

UENUM(BlueprintType)
enum class EClearingStyle : uint8
{
	None,
	StrongWall,
	PointsOfDomination,
	RunningTheRabbit
};

UENUM()
enum class EDoorCheckResult : uint8
{
	None,
	Unlocked,
	Locked,
	Jammed,
	Blocked
};

static FText DoorCheckResultToText(const EDoorCheckResult& InDoorCheckResult)
{
	switch (InDoorCheckResult)
	{
		case EDoorCheckResult::None:		return FText::FromStringTable("SwatCommandTable", "DoorNone");
		case EDoorCheckResult::Unlocked:	return FText::FromStringTable("SwatCommandTable", "DoorUnlocked");
		case EDoorCheckResult::Locked:		return FText::FromStringTable("SwatCommandTable", "DoorLocked");
		case EDoorCheckResult::Jammed:		return FText::FromStringTable("SwatCommandTable", "DoorJammed");
		case EDoorCheckResult::Blocked:		return FText::FromStringTable("SwatCommandTable", "DoorBlocked");
		default:							return FText::FromStringTable("SwatCommandTable", "DoorNone");
	}
}

UENUM(BlueprintType)
enum class ETrapType : uint8
{
	Alarm,
	Flashbang,
	Explosive,
	Unknown
};

static FString TrapTypeToString(const ETrapType& InTrapType)
{
	switch (InTrapType)
	{
		case ETrapType::Alarm:			return "Alarm";
		case ETrapType::Flashbang:		return "Flashbang";
		case ETrapType::Explosive:		return "Explosive";
		case ETrapType::Unknown:		return "Unknown";
		default:						return "None";
	}
}

UENUM(BlueprintType)
enum class ETargetingCompTracking : uint8
{
	TCT_None,
	TCT_TrackingActivity,
	TCT_TrackingCombatMoveActivity,
	TCT_TrackingVisibleNeutrals,
	TCT_TrackingEnemyLastKnownPosition,
	TCT_TrackingNoiseStimulus,
	TCT_TrackingOverrideInterests,
	TCT_TrackingStairThreatAwarenessActor,
	TCT_TrackingThreatAwarenessActor,
	TCT_TrackingLatestStimulus,
	TCT_TrackingVisibleTarget,
	TCT_TrackingMoveVector,
	TCT_TrackScriptedFireAtActor,
	TCT_TrackNearestDoor,
	TCT_TrackUncheckedThreatAwareness,
	TCT_TrackMontagePosition,
	TCT_TrackCustomLocation
};

UENUM(BlueprintType)
enum class ERonNavigationQueryResult : uint8
{
	Invalid,
	Error,
	Fail,
	Success
};

UENUM(BlueprintType)
enum class EPathedAwareness : uint8
{
	PA_None,
	PA_Noise,
	PA_LastKnownEnemyPosition,
	PA_ActivityLocation
};

UENUM(BlueprintType)
enum class EScenarioImportance : uint8
{
	SI_None,
	SI_AlwaysSpawn,
	SI_Pooled
};

UENUM(BlueprintType)
enum class EToggleBoneVis : uint8
{
	TBV_None,
    TBV_HideBone,
	TBV_ShowBone
};

UENUM(BlueprintType)
enum class EChangeBehaviour : uint8
{
	CB_Add,
	CB_Remove
};

UENUM(BlueprintType)
enum EFilterMovePointGeneration
{
	FMNP_None,
	FNMP_LeftOnly,
	FNMP_RightOnly,
	FNMP_HardLeft,
	FNMP_HardRight
};

UENUM(BlueprintType)
enum class EDoorBreachType : uint8
{
	None,
	Open,
	Move,
	Kick,
	Shotgun,
	Ram,
	C2,
	Leader,
	Custom
};

static FText DoorBreachTypeToText(const EDoorBreachType& InDoorBreachType)
{
	switch (InDoorBreachType)
	{
		case EDoorBreachType::None:		return FText::FromStringTable("SwatCommandTable", "None");
		case EDoorBreachType::Open:		return FText::FromStringTable("SwatCommandTable", "Open");
		case EDoorBreachType::Move:		return FText::FromStringTable("SwatCommandTable", "Move");
		case EDoorBreachType::Kick:		return FText::FromStringTable("SwatCommandTable", "Kick");
		case EDoorBreachType::Shotgun:	return FText::FromStringTable("SwatCommandTable", "Shotgun");
		case EDoorBreachType::Ram:		return FText::FromStringTable("SwatCommandTable", "Ram");
		case EDoorBreachType::C2:		return FText::FromStringTable("SwatCommandTable", "C2");
		case EDoorBreachType::Leader:	return FText::FromStringTable("SwatCommandTable", "Leader");
		default:						return FText::FromString("ruh-roh");
	}
}

UENUM()
enum class EFallInPattern : uint8
{
	Snake,
	HalfSnake,
	Diamond,
	Flock
};

UENUM(BlueprintType)
enum class ESwatCommand : uint8
{
	SC_None,
	SC_MoveTo,
	SC_FallIn,
	SC_FallIn_Snake,
	SC_FallIn_HalfSnake,
	SC_FallIn_Diamond,
	SC_FallIn_Flock,
	SC_Cover,
	SC_Hold,
	SC_Resume,
	SC_DeployFlashbang,
	SC_DeployStinger,
	SC_DeployCSGas,
	SC_DeployChemlight,
	SC_DoArrestTarget,
	SC_DoCollectEvidence,
	SC_DoReportTarget,
	SC_DisarmStandaloneTrap,
	SC_KillMe,
	SC_StackUp,
	SC_StackUpSplit,
	SC_StackUpLeft,
	SC_StackUpRight,
	SC_PickLock,
	SC_RemoveDoorJam,
	SC_DeployMirrorgun,
	SC_DeployDoorJam,
	SC_CheckForTrap,
	SC_DisarmTrap,
	SC_OpenDoor,
	SC_CloseDoor,
	SC_Slide,
	SC_Slice,
	SC_Snap,
	SC_SearchAndSecure,
	SC_SearchAndSecureRoom,
	SC_SearchAndSecureRoom_Individual,
	SC_MoveAndClear,
	SC_MoveAndClearFlashbang,
	SC_MoveAndClearStinger,
	SC_MoveAndClearCSGas,
	SC_MoveAndClearLauncher,
	SC_MoveAndClearLeader,
	SC_OpenAndClear,
	SC_OpenAndClearFlashbang,
	SC_OpenAndClearStinger,
	SC_OpenAndClearCSGas,
	SC_OpenAndClearLauncher,
	SC_OpenAndClearLeader,
	SC_KickAndClear,
    SC_KickAndClearFlashbang,
    SC_KickAndClearStinger,
    SC_KickAndClearCSGas,
    SC_KickAndClearLauncher,
    SC_KickAndClearLeader,
	SC_ShotgunClear,
	SC_ShotgunClearFlashbang,
	SC_ShotgunClearStinger,
	SC_ShotgunClearCSGas,
	SC_ShotgunClearLeader,
	SC_ShotgunClearLauncher,
	SC_RamAndClear,
	SC_RamAndClearFlashbang,
	SC_RamAndClearStinger,
	SC_RamAndClearCSGas,
	SC_RamAndClearLauncher,
	SC_RamAndClearLeader,
	SC_C2Clear,
	SC_C2ClearFlashbang,
	SC_C2ClearStinger,
	SC_C2ClearCSGas,
	SC_C2ClearLauncher,
	SC_C2ClearLeader,
	SC_LeaderAndClear,
	SC_LeaderAndClearFlashbang,
	SC_LeaderAndClearStinger,
	SC_LeaderAndClearCSGas,
	SC_LeaderAndClearLauncher,
	SC_LeaderAndClearLeader,
	SC_SwapWithAlpha,
	SC_SwapWithBeta,
	SC_SwapWithCharlie,
	SC_SwapWithDelta,
	SC_SwapWithAlphaOpposite,
	SC_SwapWithBetaOpposite,
	SC_SwapWithCharlieOpposite,
	SC_SwapWithDeltaOpposite,
	SC_MoveToAlpha,
	SC_MoveToBeta,
	SC_MoveToCharlie,
	SC_MoveToDelta,
	SC_MoveTo_Individual,
	SC_MoveTo_MyPosition_Individual,
	SC_MoveTo_Stop_Individual,
	SC_MoveTo_Exit_Individual,
	SC_MoveToAndBack_Individual,
	SC_Focus_Individual,
	SC_Focus_MyPosition_Individual,
	SC_UnFocus_Individual,
	SC_FocusDoor_Individual,
	SC_FocusTarget_Individual,
	SC_TurnAround_Individual,
	SC_Execute, // initiate queued command
	SC_Cancel, // cancel queued command
	SC_DeployShield,
	SC_HolsterShield,
	SC_DeployTaser,
	SC_DeployPepperspray,
	SC_DeployPepperball,
	SC_DeployBeanbag,
	SC_MeleeTarget,
	SC_Roger,
	SC_Negative,
	PC_Deploy, // Parent command: Has subcommands (SC). Used to avoid SC_None
	PC_ConfirmOrderRequest,
	PC_StackUp,
	PC_Open,
	PC_Door,
	PC_OtherDoor,
	PC_DoorWay,
	PC_OtherDoorWay,
	PC_Scan,
	PC_Move,
	PC_Kick,
	PC_Ram,
	PC_Shotgun,
	PC_Leader,
	PC_C2,
	PC_Breach,
	PC_FallIn,
	PC_Focus,
	PC_SwapWith,
	PC_OpenedDoor,
	PC_ClosedDoor,
};

static FString TeamTypeEnumToString(const ETeamType& EnumValue)
{
	switch (EnumValue)
	{
		case ETeamType::TT_NONE:		return "No Team";
		case ETeamType::TT_SERT_RED:	return "Red Team";
		case ETeamType::TT_SERT_BLUE:	return "Blue Team";
		case ETeamType::TT_SUSPECT:		return "Suspect";
		case ETeamType::TT_CIVILIAN:	return "Civilian";
		case ETeamType::TT_SQUAD:		return "Gold Team";
		default:						return "TeamTypeEnumToString | Enum does not exist!";
	}
}

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	WT_None,
	WT_Rifles,
	WT_SubmachineGun,
	WT_Shotgun,
	WT_PistolsLethal,
	WT_PistolsNonLethal,
	WT_PrimaryNonLethal,
	WT_Launcher,
	WT_Special,
	WT_Unarmed, // civilians use this slot, suspects can too if they want :P
};

UENUM(BlueprintType)
enum class ESquadPosition : uint8
{
	SP_Alpha,
	SP_Beta,
	SP_Charlie,
	SP_Delta,
	SP_Foxtrot,
	SP_Golf,
	SP_Hotel,
	SP_India,
	SP_NONE
};

UENUM(BlueprintType)
enum class EFireMode : uint8
{
    FM_Single,
    FM_Auto,
    FM_Burst,
    FM_Continuous,
    FM_Safe
};

UENUM(BlueprintType)
enum class EItemType : uint8
{
	IT_None,
	IT_Rifles,
	IT_SubmachineGun,
	IT_LightMachineGun,
	IT_Shotgun,
	IT_Sniper,
	IT_PistolsLethal,
	IT_PistolsNonLethal,
	IT_PrimaryNonLethal,
	IT_Launcher,
	IT_Melee,
	IT_LessLethal,
	IT_Headwear,
	IT_BodyArmor,
	IT_Grenade,
	IT_GrenadeNonLethal,
	IT_TacticalDevice,
	IT_TacticalDeviceNonLethal,
	IT_TacticalOne,
	IT_TacticalTwo,
	IT_TacticalThree,
	IT_TacticalFour,
	IT_TacticalFive,
	IT_TacticalSix,
	IT_TacticalSeven,
	IT_TacticalEight,
	IT_LongTactical,
	IT_Skins,
	IT_Loadouts
};

UENUM(BlueprintType)
enum class EItemClass : uint8
{
	IC_NoClass,
	IC_AssaultRifle,
	IC_SMG,
	IC_LMG,
	IC_Pistol,
	IC_Sniper,
	IC_Melee,
	IC_LessLethal,
	IC_Shotgun,
	IC_Launcher,
	IC_Grenade,
	IC_Shield,
	IC_Armor,
	IC_Headgear,
	IC_TacticalDevice,
	IC_LongTactical,
	IC_Officer,
	IC_Uniform,
	IC_Plates,
	IC_Patches
};

static FString ItemClassEnumToString(const EItemClass& ItemClassEnum)
{
	switch (ItemClassEnum)
	{
		case EItemClass::IC_NoClass:		return "No Class";
		case EItemClass::IC_AssaultRifle:	return "Assault Rifle";
		case EItemClass::IC_SMG:			return "SMG";
		case EItemClass::IC_LMG:			return "LMG";
		case EItemClass::IC_Pistol:			return "Pistol";
		case EItemClass::IC_Sniper:			return "Sniper";
		case EItemClass::IC_Melee:			return "Melee";
		case EItemClass::IC_Shotgun:		return "Shotgun";
		case EItemClass::IC_Launcher:		return "Launcher";
		case EItemClass::IC_Grenade:		return "Grenade";
		case EItemClass::IC_Shield:			return "Shield";
		case EItemClass::IC_LessLethal:		return "Less Lethal";
		case EItemClass::IC_Armor:			return "Armor";
		case EItemClass::IC_Headgear:		return "Headgear";
		case EItemClass::IC_TacticalDevice:	return "Tactical Device";
		case EItemClass::IC_LongTactical:	return "Long Tactical";
		case EItemClass::IC_Officer:		return "Officer";
		case EItemClass::IC_Uniform:		return "Uniform";
		case EItemClass::IC_Plates:			return "Plates";
		case EItemClass::IC_Patches:		return "Patches";
		default:							return "EItemClass | Enum does not exist";
	}
}

UENUM(BlueprintType)
enum class EItemAttachment : uint8
{
	IA_None,
	IA_Flashlight,
	IA_NVG
};

UENUM(BlueprintType)
enum class EKillfeedType : uint8
{
	KT_None,
	// killed a player
	KT_Kill,
	// arrested a player
	KT_Arrest,
	// freed a player
	KT_Free,
	// killed a player a second time (recovery shows first)
	KT_Recovered,
	// our player died
	KT_Death
};

UENUM(BlueprintType)
enum class EMedicalHealScreen : uint8
{
	MHS_Healer				UMETA(DisplayName = "Healer"),
	MHS_Healee				UMETA(DisplayName = "Healee"),
    MHS_MortallyWounded		UMETA(DisplayName = "Mortally Wounded"),
    MHS_NoBrokenLimbs		UMETA(DisplayName = "No Broken Limbs")
};

// Giving names to custom depth stencil values
UENUM(BlueprintType)
enum class EActorOutlineType : uint8
{
	Outline_0, // Empty
	Outline_1, // Empty
    Outline_2 = 2, // Command targeting
    Outline_3, // Empty
    Outline_4, // Empty
	Outline_5, // Empty
	Outline_6  // Empty
};

UENUM(BlueprintType)
enum class ECommWheelLockOnBehaviour : uint8
{
	// When line of sight is obstructed by an actor, lock on to the actor if it's appropriate to do so
	LB_LockOnToObstruction			UMETA(DisplayName = "LockOn to Obstruction, if valid"),

    // When line of sight is obstructed by an actor, stay locked on to the targeted character (Will cancel lockon if obstruted by anything other than a RON Character)
    LB_KeepLockOn					UMETA(DisplayName = "Keep LockOn"),

    // When line of sight is obstructed by an actor, cancel the lockon and close the comm wheel
    LB_CancelLockOnWhenObstructed	UMETA(DisplayName = "Cancel LockOn"),
};

UENUM(BlueprintType)
enum class ERONBuildConfiguration : uint8
{
	Unknown,
	Editor,
    Debug,
    Development,
    Shipping,
	FinalRelease,
    Test
};

UENUM(BlueprintType)
enum class EItemSelectionInterfaceType : uint8
{
	Wheel,
    Panel
};

UENUM(BlueprintType)
enum class EPVPEvent : uint8
{
	None,
	PlayerKilled,
    PlayerArrested,
    PlayerFreed,
    KillConfirmed,
    ReportedEvidence,
    VIPSecured,
    VIPArrested,
    VIPFreed,
	VIPKilled,
	FlagCaptured,
	FlagDropped,
	IntelCollected,
	IntelDropped,
	IntelExtracting,
	IntelExtracted,
	IncrimClueFound,
};

UENUM(BlueprintType)
enum class ECharacterDeathReason : uint8
{
	None,
	PrimaryWeapon,
	SecondaryWeapon,
	TasedToDeath,
	FellFromHighHeight,
    Suicide,
    Headshot,
	Bleedout,
	Grenade,
	Explosion,
	MultipleUnhealedWounds
};

UENUM(BlueprintType)
enum class EEvidenceActorState : uint8
{
	Unclaimed,
	Collected,
	Extraction,
	Dropped
};

UENUM(BlueprintType)
enum class EVoteState : uint8
{
    Undecided,
    Yes,
    No,
};

UENUM(BlueprintType)
enum class ETutorialMessageContext : uint8
{
	Movement
};

UENUM(BlueprintType)
enum class EItemCategory : uint8
{
	IC_None,
	IC_Primary,
	IC_Secondary,
	IC_Grenade,
	IC_Gadget,
	IC_Magazine,
	IC_Helmet,
	IC_Helmet_Light,
	IC_Headset,
	IC_Goggles,
	IC_Chest,
	IC_Watch,
	IC_Radio,
	IC_NVG,
	IC_UseableWithShield,
	IC_Grenade_Keybind1,
	IC_Grenade_Keybind2,
	IC_Grenade_Keybind3,
	IC_Grenade_Keybind4,
	IC_Device_Keybind1,
	IC_Device_Keybind2,
	IC_Device_Keybind3,
	IC_Device_Keybind4,
	IC_Badge_Armour,
	IC_OCSpray,
	IC_Multitool,
	IC_Zipcuffs,
	IC_Armor,
	IC_Chemlight,
	IC_Shield,
	IC_Flashbang,
	IC_Ninebang,
	IC_CSGas,
	IC_Stingball,
	IC_Optiwand,
	IC_Beanbag,
	IC_Taser,
	IC_Pepperball,
	IC_C2Explosive,
	IC_Detonator,
	IC_Doorjam,
	IC_BatteringRam,
	IC_BreachingShotgun,
	IC_Tablet,
	IC_TacticalDevice,
	IC_LongTactical,
	IC_GasMask,
	IC_Launcher,
	IC_MedicalKit,
	IC_LockpickGun,
	IC_Shotgun
};

// NOTE: Must match the event_type.py in Ron Events Server
UENUM(BlueprintType)
enum class EGameEventMetric : uint8
{
	GEM_NONE = 0,
	GEM_GAME_STARTED = 1,
	GEM_GAME_JOINED = 2,
	GEM_GAME_FINISHED = 3, 
	GEM_GAME_CRASHED = 4,
	GEM_PLAYER_GAME_FINISHED = 5, // Used to show an individual players game results (i.e FPS) as opposed to the result of the game (e.g F)
};

/**
 * Enum for the different states during training.
 *
 * When adding a new state, make sure to add it to the switch in GetNextState() in TrainingGM.cpp.
 */
UENUM(BlueprintType)
enum class ETrainingState : uint8
{
	TS_Invalid					UMETA(Hidden),
	TS_Spawned					UMETA(Hidden),
	TS_PickupWeapon				UMETA(DisplayName="Pickup Primary Weapon"),
	TS_MoveToShooting			UMETA(DisplayName="Move to Shooting Range"),
	TS_ReloadWeapon				UMETA(DisplayName="Reload your Weapon"),
	TS_TargetShooting1 			UMETA(DisplayName="Shoot the Target"),
	TS_TargetShooting2 			UMETA(DisplayName="Shoot the Target - ADS"),
	TS_EquipSecondary 			UMETA(DisplayName="Equip Secondary Weapon"),
	TS_ShootingChallenge1 		UMETA(DisplayName="Shooting Challenge #1"),
	TS_ShootingChallenge2 		UMETA(DisplayName="Shooting Challenge #2"),
	TS_UseLoadout 				UMETA(DisplayName="Use Loadout"),
	TS_TargetGrenades 			UMETA(DisplayName="Throw Grenade at Target"),
	TS_MoveToTraining 			UMETA(DisplayName="Move to Training Grounds"),
	TS_OpenDoor 				UMETA(DisplayName="Open Door"),
	TS_KickDoor 				UMETA(DisplayName="Kick Door"),
	TS_MeetTheTeam 				UMETA(DisplayName="Meet the Team"),
	TS_EnterTraining 			UMETA(DisplayName="Enter the Training Grounds"),
	TS_ClearRoom1 				UMETA(DisplayName="Clear Room #1"),
	TS_ClearRoom2 				UMETA(DisplayName="Clear Room #2"),
	TS_ClearRoom3 				UMETA(DisplayName="Clear Room #3"),
	TS_ClearRoom4 				UMETA(DisplayName="Clear Room #4"),
	TS_Exfiltrate 				UMETA(DisplayName="Exfiltrate"),
	TS_Completed				UMETA(Hidden),
	TS_Start = TS_Spawned		UMETA(Hidden),
	TS_Finish = TS_Completed	UMETA(Hidden)
};

UENUM(BlueprintType)
enum ETutorialWidgetLocation
{
	TWL_Bottom,
	TWL_Right,
	TWL_Left
};

/**
 * Enum for the different activities to be performed by the player.
 * To be used in as the "activity" in UActivityData.
 *
 * When adding a new activity, make sure to add it to the switch in BindPlayerToActivity(),
 * CanEditActionsRequired(), and CanEditTimeRequired() as well as any required sub-functions.
 */
UENUM(BlueprintType)
enum class EActivity : uint8
{
	A_GoToLocation				UMETA(DisplayName="Go To Location"),
	A_Delay						UMETA(DisplayName="Delay"),
	A_UIOnly					UMETA(DisplayName="UI Only (Never Complete)"),
	A_MoveForward				UMETA(DisplayName="Move Forward"),
	A_MoveBackward				UMETA(DisplayName="Move Backward"),
	A_MoveRight					UMETA(DisplayName="Move Right"),
	A_MoveLeft					UMETA(DisplayName="Move Left"),
	A_MoveForwardLowReady		UMETA(DisplayName="Move Forward Low Ready"),
	A_Interact					UMETA(DisplayName="Interact"),
	A_InteractTriggerable		UMETA(DisplayName="Interact with Triggerable"),
	A_OpenDoor					UMETA(DisplayName="Open Door"),
	A_SecureEvidence			UMETA(DisplayName="Secure Evidence"),
	A_EquipPrimary				UMETA(DisplayName="Equip Primary"),
	A_EquipSecondary			UMETA(DisplayName="Equip Secondary"),
	A_ShootPrimaryHip			UMETA(DisplayName="Shoot Primary from Hip"),
	A_ShootPrimaryADS			UMETA(DisplayName="Shoot Primary from ADS"),
	A_ShootSecondaryHip			UMETA(DisplayName="Shoot Secondary from Hip"),
	A_ShootSecondaryADS			UMETA(DisplayName="Shoot Secondary from ADS"),
	A_Reload					UMETA(DisplayName="Reload"),
	A_ReloadEmpty				UMETA(DisplayName="Reload From Empty"),
	A_ReloadAiming				UMETA(DisplayName="Reload While Aiming"),
	A_ReloadQuick				UMETA(DisplayName="Reload Quick"),
	A_SwitchAmmoType			UMETA(DisplayName="Switch Ammo Type"),
	A_SwitchFireMode			UMETA(DisplayName="Switch Fire Mode"),
	A_ToggleTacticalLight		UMETA(DisplayName="Toggle Tactical Light"),
	A_ToggleCantedSight			UMETA(DisplayName="Toggle Canted Sight"),
	A_ThrowFlashbangGrenade		UMETA(DisplayName="Throw Flashbang Grenade"),
	A_ThrowCSGasGrenade			UMETA(DisplayName="Throw CS Gas Grenade"),
	A_ThrowStingerGrenade		UMETA(DisplayName="Throw Stinger Grenade"),
	A_ThrowNineBangerGrenade	UMETA(DisplayName="Throw Nine Banger Grenade"),
	A_UseChemlight				UMETA(DisplayName="Use Chemlight"),
	A_UseOptiwand				UMETA(DisplayName="Use Optiwand"),
	A_UseDoorjam				UMETA(DisplayName="Use Doorjam"),
	A_UseBatteringRam			UMETA(DisplayName="Use Battering Ram"),
	A_UseC2Explosive			UMETA(DisplayName="Use C2 Explosive"),
	A_UseLockpick				UMETA(DisplayName="Use Lockpick"),
	A_UseNVGs					UMETA(DisplayName="Use NVGs"),
	A_IssueSwatCommand			UMETA(DisplayName="Issue SWAT Command"),
	A_ArrestOrKillAi			UMETA(DisplayName="Arrest or Kill an AI Character"),
	A_ShootTarget				UMETA(DisplayName="Shoot Target"),
	A_ShootTargetADS			UMETA(DisplayName="Shoot Target while ADS"),
	A_ShootTargetCanted			UMETA(DisplayName="Shoot Target while Sights Canted"),
	A_ShootTargetLaser			UMETA(DisplayName="Shoot Target with a Laser On"),
	A_GrenadeTarget				UMETA(DisplayName="Grenade Training Target"),
	A_SwitchTeamCamera			UMETA(DisplayName="Switch SWAT Team Camera"),
	A_SwitchSwatElement			UMETA(DisplayName="Switch SWAT Team Element"),
	A_Exfiltrate				UMETA(DisplayName="Exfiltrate Mission"),
};

UENUM(BlueprintType)
enum class EEventType : uint8
{
	E_Standalone				UMETA(DisplayName="Standalone"),
	E_Target					UMETA(DisplayName="Target"),
	E_FmodAudio					UMETA(DisplayName="FMOD Audio"),
	E_UnrealAudio				UMETA(DisplayName="Unreal Audio"),
};

UENUM(BlueprintType)
enum class EStandaloneEvent : uint8
{
	E_None,
	E_LockPlayerMovement		UMETA(DisplayName="Lock Player Movement"),
	E_UnlockPlayerMovement		UMETA(DisplayName="Unlock Player Movement"),
	E_LockPlayerItemSelection	UMETA(DisplayName="Lock Player Item Selection"),
	E_UnlockPlayerItemSelection	UMETA(DisplayName="Unlock Player Item Selection"),
	E_LockAllPlayerActions		UMETA(DisplayName="Lock All Player Actions"),
	E_UnlockAllPlayerActions	UMETA(DisplayName="Unlock All Player Actions"),
	E_LockPlayerCommandMenu		UMETA(DisplayName="Lock Player Command Menu"),
	E_UnlockPlayerCommandMenu	UMETA(DisplayName="Unlock Player Command Menu"),
	E_LockWeaponAttachments		UMETA(DisplayName="Lock Weapon Attachments"),
	E_UnlockWeaponAttachments	UMETA(DisplayName="Unlock Weapon Attachments"),
	E_LockCantedSights			UMETA(DisplayName="Lock Canted Sights"),
	E_UnlockCantedSights		UMETA(DisplayName="Unlock Canted Sights"),
	E_SetPlayerLowReady			UMETA(DisplayName="Set Player Low Ready"),
	E_SetPlayerNotLowReady		UMETA(DisplayName="Set Player Not Low Ready"),
	E_SpawnPolice				UMETA(DisplayName="Spawn SWAT Team"),
	E_SpawnPoliceAtPlayer		UMETA(DisplayName="Spawn SWAT Team at Player"),
	E_HidePlayerWeapon			UMETA(DisplayName="Hide Player Weapon"),
	E_EquipTrainingLoadout		UMETA(DisplayName="Equip Training Loadout"),
	E_RemoveAmmoFromLoadout		UMETA(DisplayName="Remove Ammo from Loadout"),
	E_AddAmmoToLoadout			UMETA(DisplayName="Add Ammo to Loadout"),
	E_AddGrenadesToLoadout		UMETA(DisplayName="Add Grenades to Loadout"),
	E_AddChemlightsToLoadout	UMETA(DisplayName="Add Chemlights to Loadout"),
	E_AddEmptyMagPrimary		UMETA(DisplayName="Add Empty Magazine to Primary"),
};

UENUM(BlueprintType)
enum class ETargetEvent : uint8
{
	E_None,
	E_LockDoor					UMETA(DisplayName="Lock Door"),
	E_UnlockDoor				UMETA(DisplayName="Unlock Door"),
	E_DisableDoor				UMETA(DisplayName="Disable Door for Player"),
	E_EnableDoor				UMETA(DisplayName="Enable Door for Player"),
	E_DisableDoorInteraction	UMETA(DisplayName="Disable All Door Interactables"),
	E_EnableDoorInteraction		UMETA(DisplayName="Enable All Door Interactables"),
	E_EnableDoorOpen			UMETA(DisplayName="Enable Door Open Interactable"),
	E_EnableDoorPeek			UMETA(DisplayName="Enable Door Peek Interactable"),
	E_EnableDoorKick			UMETA(DisplayName="Enable Door Kick Interactable"),
	E_EnableDoorOptiwand		UMETA(DisplayName="Enable Door Optiwand Interactable"),
	E_ActivateTriggerable		UMETA(DisplayName="Activate Triggerable"),
	E_DeactivateTriggerable		UMETA(DisplayName="Deactivate Triggerable"),
	E_SpawnAi					UMETA(DisplayName="Spawn AI"),
	E_SetPlayerSpawn			UMETA(DisplayName="Set Player Spawn"),
};

UENUM(BlueprintType)
enum class EClearDirection : uint8
{
	None,
	Right,
	Forward
};

UENUM(BlueprintType)
enum class EEquippingSwat : uint8
{
	ES_None,
	ES_BlueOne,
	ES_BlueTwo,
	ES_RedOne,
	ES_RedTwo
};