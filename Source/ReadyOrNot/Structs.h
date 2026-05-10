#pragma once

#include "CoverData.h"
#include "Enums.h"
#include "Structs.generated.h"

USTRUCT(BlueprintType)
struct FSwatCommand
{
	GENERATED_USTRUCT_BODY()
	
	FSwatCommand() {}

	FSwatCommand(FText InCommandText, ESwatCommand InCommandType, TArray<FSwatCommand> InSubCommands, bool bInEnabled = true, bool bHoldPage = false, bool bIssueTextSameAsCommandText = true)
	{
		CommandText = InCommandText;
		CommandType = InCommandType,
		SubCommands = InSubCommands,
		bEnabled = bInEnabled;
		bHoldPageUntilExecute = bHoldPage;
		bCommandTextAsIssuedText = bIssueTextSameAsCommandText;
	}

	FSwatCommand(FText InCommandText, ESwatCommand InCommandType, bool bInGrabContextualDataOnExecute = false, bool bInEnabled = true, bool bHoldPage = false, bool bIssueTextSameAsCommandText = true)
	{
		CommandText = InCommandText,
		CommandType = InCommandType,
		bGrabContextualDataOnExecute = bInGrabContextualDataOnExecute,
		bEnabled = bInEnabled;
		bHoldPageUntilExecute = bHoldPage;
		bCommandTextAsIssuedText = bIssueTextSameAsCommandText;
	}

	FSwatCommand(FText InCommandText, ESwatCommand InCommandType, TArray<FSwatCommand> InSubCommands, AActor* InCommandTarget, bool bInEnabled = true, bool bHoldPage = false, bool bIssueTextSameAsCommandText = true)
	{
		CommandText = InCommandText;
		CommandType = InCommandType;
		SubCommands = InSubCommands;
		CommandTarget = InCommandTarget;
		bEnabled = bInEnabled;
		bHoldPageUntilExecute = bHoldPage;
		bCommandTextAsIssuedText = bIssueTextSameAsCommandText;
	}

	FSwatCommand(FText InCommandText, ESwatCommand InCommandType, AActor* InCommandTarget, bool bInGrabContextualDataOnExecute = false, bool bInEnabled = true, bool bHoldPage = false, bool bIssueTextSameAsCommandText = true)
	{
		CommandText = InCommandText;
		CommandType = InCommandType;
		bGrabContextualDataOnExecute = bInGrabContextualDataOnExecute;
		CommandTarget = InCommandTarget;
		bEnabled = bInEnabled;
		bHoldPageUntilExecute = bHoldPage;
		bCommandTextAsIssuedText = bIssueTextSameAsCommandText;
	}

	friend bool operator ==(const FSwatCommand& LHS, const FSwatCommand& RHS)
	{
		return  LHS.bEnabled == RHS.bEnabled &&
				LHS.bHoldPageUntilExecute == RHS.bHoldPageUntilExecute &&
				LHS.InputKey == RHS.InputKey &&
				LHS.CommandText.EqualTo(RHS.CommandText) &&
				LHS.CommandType == RHS.CommandType &&
				LHS.SubCommands == RHS.SubCommands &&
				LHS.bGrabContextualDataOnExecute == RHS.bGrabContextualDataOnExecute;
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FKey InputKey = EKeys::Invalid;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText CommandText = FText();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ESwatCommand CommandType = ESwatCommand::SC_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bGrabContextualDataOnExecute = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bEnabled = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bUseSecondaryContextData = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bHoldPageUntilExecute = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCommandTextAsIssuedText = true;

	UPROPERTY(BlueprintReadWrite)
	AActor* CommandTarget = nullptr;
	
	UPROPERTY(BlueprintReadWrite)
	AActor* CommandTarget2 = nullptr;

	UPROPERTY(BlueprintReadOnly)
	int32 Index = 0;

	TArray<FSwatCommand> SubCommands = {};
};

USTRUCT(BlueprintType)
struct FLevelStreamOptions
{
	GENERATED_USTRUCT_BODY()

	bool bShouldCreateLoadingScreen;
	bool bShouldRemoveLoadingScreen;
	bool bExecuteOpenLevelWhenLoaded;
	bool bStreamInLevelBeforeLoad;
	bool bIsSeamlessTravel;
	FString ModeName;
	FString SessionName;

	FLevelStreamOptions()
	{
		bShouldCreateLoadingScreen = true;
		bShouldRemoveLoadingScreen = true;
		bExecuteOpenLevelWhenLoaded = true;
		bStreamInLevelBeforeLoad = false;
		ModeName = "";
		SessionName = "";
	}
};

USTRUCT(BlueprintType)
struct FLoadoutEquipOptions
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	EItemCategory EquipItemCategory;
	UPROPERTY()
	bool bReplicates;
	UPROPERTY()
	class AReadyOrNotPlayerState* OverridePlayerState;
	UPROPERTY()
	bool bSanitizeLoadout;

	FLoadoutEquipOptions()
	{
		EquipItemCategory = EItemCategory::IC_Primary;
		bReplicates = true;
		OverridePlayerState = nullptr;
		bSanitizeLoadout = true;
	}
};


USTRUCT(BlueprintType)
struct FCharacterLookOverride : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere)
	EGameVersionRestriction LockedToDLC;
	
	UPROPERTY(EditAnywhere)
	FText FriendlyBodyName;

	UPROPERTY(EditAnywhere)
	FString SpeechCharacterName;

	UPROPERTY(EditAnywhere)
	UTexture2D* BodyIcon;

	UPROPERTY(EditAnywhere)
	FText FriendlyHeadName;

	UPROPERTY(EditAnywhere)
	UTexture2D* HeadIcon;

	UPROPERTY(EditAnywhere)
	TMap<TSubclassOf<class ABaseArmour>, USkeletalMesh*> ArmorOverrideMap;

	UPROPERTY(EditAnywhere)
	USkeletalMesh* FPMeshOverride;

	UPROPERTY(EditAnywhere)
	USkeletalMesh* FPBodyMeshOverride;

	UPROPERTY(EditAnywhere)
	USkeletalMesh* FaceMeshOverride;

	UPROPERTY(EditAnywhere)
	USkeletalMesh* BodyMeshOverride;

	FCharacterLookOverride()
	{
		ArmorOverrideMap = {};
		FriendlyBodyName = FText::FromName(NAME_None);
		FriendlyHeadName = FText::FromName(NAME_None);
		FPMeshOverride = nullptr;
		FPBodyMeshOverride = nullptr;
		FaceMeshOverride = nullptr;
		BodyMeshOverride = nullptr;
		FPBodyMeshOverride = nullptr;
		BodyIcon = nullptr;
		HeadIcon = nullptr;
		SpeechCharacterName = "";
	}
};

USTRUCT(BlueprintType)
struct FCharacterPersonalizationData
{
	
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadOnly)
	EGameVersionRestriction LockedToDLC;
	
	UPROPERTY(BlueprintReadOnly)
	FName RowName;
	
	UPROPERTY(BlueprintReadOnly)
	FText FriendlyName;

	UPROPERTY(BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> Icon;

	friend bool operator ==(const FCharacterPersonalizationData& LHS, const FCharacterPersonalizationData& RHS)
	{
		return	LHS.RowName == RHS.RowName;
	}
	
};

USTRUCT(BlueprintType)
struct FCarryArrestedAnimState
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite, Category = PickupArrest)
	UAnimSequence* LoopAnim = nullptr;
};

USTRUCT(BlueprintType)
struct FTakeHostageAnimState
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Hostage Take")
	uint8 bIsTakingHostage : 1;
	
	UPROPERTY(BlueprintReadWrite, Category = "Hostage Take")
	uint8 bIsLooping : 1;
	
	UPROPERTY(BlueprintReadWrite, Category = "Hostage Take")
	UAnimSequence* LoopAnim = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "Hostage Take")
	class UAimOffsetBlendSpace* AimOffset = nullptr;
};

USTRUCT(BlueprintType)
struct FWorldBuildingAnimState
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "World Building")
	bool bIsPlaying = false;

	UPROPERTY(BlueprintReadWrite, Category = "World Building")
	bool bIsLooping = false;

	UPROPERTY(BlueprintReadWrite, Category = "World Building")
	UAnimSequence* LoopAnim = nullptr;
};

USTRUCT(BlueprintType)
struct FScriptedLookAt
{
	GENERATED_USTRUCT_BODY()

	FScriptedLookAt()
	{
		LookAtActor = nullptr;
		LookAtLocation = FVector::ZeroVector;
		TimeRemaining = 0.0f;
	}

	void Reset()
	{
		*this = FScriptedLookAt();
	}
	
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite)
	AActor* LookAtActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector LookAtLocation = FVector::ZeroVector;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TimeRemaining = 0.0f;
};

USTRUCT(BlueprintType)
struct FScriptedFireAt
{
	GENERATED_USTRUCT_BODY()

	FScriptedFireAt()
	{
		FireAtActor = nullptr;
		FireAtLocation = FVector::ZeroVector;
		TimeRemaining = 0.0f;
		bOverrideTargetedEnemy = false;
		AccuracyPenaltyMultiplier = 1.0f;
	}

	void Reset()
	{
		*this = FScriptedFireAt();
	}
	
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite)
	AActor* FireAtActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector FireAtLocation = FVector::ZeroVector;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TimeRemaining = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bOverrideTargetedEnemy = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AccuracyPenaltyMultiplier = 0.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FireAngleThreshold = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bInfiniteAmmo = false;
};

USTRUCT(BlueprintType)
struct FCoverAnimStateMachineData
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cover")
	bool bIsInCover = false;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cover")
	bool bIsFiring = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cover")
	bool bIsReturningToIdle = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cover")
	ECoverDirection ActiveCoverDirection = ECoverDirection::Left;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cover")
	UAnimSequence* ActiveCoverFirePose = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cover")
	UAnimSequence* ActiveCoverIdlePose = nullptr;
};

USTRUCT(BlueprintType)
struct FHidingAnimStateMachineData
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hiding")
	uint8 bIsHiding : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hiding")
	uint8 bLooping : 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hiding")
	UAnimSequence* LoopAnim = nullptr;
};

USTRUCT(BlueprintType)
struct FHoleTraversalAnimStateMachineData
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hiding")
	uint8 bIsTraversing : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hiding")
	uint8 bLooping : 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hiding")
	UAnimSequence* LoopAnim = nullptr;
};

USTRUCT(BlueprintType)
struct FExposedToNoise
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class AReadyOrNotCharacter* Instigator = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName Tag = NAME_None;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float HeardAtDistance;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector StimulusLocation;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bAggressive;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bFriendly;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float TimeSinceExposed;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float ExpiryTime = 10.0f;

	FExposedToNoise()
	{
		HeardAtDistance = FLT_MAX;
		TimeSinceExposed = 0.0f;
		StimulusLocation = FVector::ZeroVector;
		bAggressive = false;
		bFriendly = false;
	}
};

USTRUCT(BlueprintType)
struct FQueuedSwatCommand
{
	GENERATED_USTRUCT_BODY()
	
	UPROPERTY()
	FSwatCommand Command;

	UPROPERTY()
	FHitResult ContextualData;

	FQueuedSwatCommand()
	{
		Command = FSwatCommand();
		ContextualData = FHitResult();
	};
};

USTRUCT(BlueprintType)
struct FSpeedRange
{
	GENERATED_USTRUCT_BODY()

	FSpeedRange()
	{
		bRandomSpeed = false;
	}

	explicit FSpeedRange(const float InSpeed)
	{
		bRandomSpeed = false;

		Speed = InSpeed;
	}

	explicit FSpeedRange(const float InMinSpeed, const float InMaxSpeed)
	{
		bRandomSpeed = true;
		
		MinSpeed = InMinSpeed;
		MaxSpeed = InMaxSpeed;
	}

	void RecalculateSpeed()
	{
		if (bRandomSpeed)
		{
			Speed = FMath::RandRange(MinSpeed, MaxSpeed);
		}
	}
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "!bRandomSpeed", ClampMin = 0.0f))
	float Speed = 1.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	uint8 bRandomSpeed : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "bRandomSpeed", ClampMin = 0.0f))
	float MinSpeed = 1.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "bRandomSpeed", ClampMin = 0.0f))
	float MaxSpeed = 10.0f;
};

USTRUCT(BlueprintType)
struct FRonKey
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RON Input Key")
	FString InputName = "None";
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RON Input Key")
	FString AlternativeInputName = "None";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RON Input Key")
	FSlateBrush IconBrush;
};

USTRUCT(BlueprintType)
struct FRonInputKeyTable : public FTableRowBase
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RON Input Key")
	FRonKey Key;
};

USTRUCT(BlueprintType)
struct FCommandWheelIconMapping : public FTableRowBase
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ESwatCommand SwatCommand;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FSlateBrush Icon;
};

USTRUCT(BlueprintType)
struct FRonInputKeyGamePadIconTable : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RON Input Key Game Pad Icon")
	FSlateBrush PS5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RON Input Key Game Pad Icon")
	FSlateBrush XSX;
	
};

USTRUCT()
struct FCharacterCollisionTemplate
{
	GENERATED_BODY()
	
	FCollisionResponseTemplate CapsuleCollision;

	FCollisionResponseTemplate MeshCollision;
};

USTRUCT(BlueprintType)
struct FTutorialDescriptionInput
{
	GENERATED_BODY()

	/** Refer to Project Settings -> Input to get the exact input action/axis name. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName InputName;

	/** The index of the key to display. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 KeyIndex;
};

USTRUCT(BlueprintType)
struct FTutorialWidgetData
{
	GENERATED_USTRUCT_BODY()

	/** The title for the main widget. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Title;

	/** The description for the main widget. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (MultiLine = "true"))
	FText Description;

	/** The input(s) to display in the description. (optional) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (TitleProperty = "InputName"))
	TArray<FTutorialDescriptionInput> DescriptionInputs;

	/** Whether or not to include a gamepad-specific description. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bGamepadDescription;

	/** The gamepad-specific description for the main widget. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "bGamepadDescription", EditConditionHides, MultiLine = "true"))
	FText GamepadDescription;

	/** The input(s) to display in the gamepad-specific description. (optional) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "bGamepadDescription", EditConditionHides, TitleProperty = "InputName"))
	TArray<FTutorialDescriptionInput> GamepadDescriptionInputs;

	/** The media to display in the main widget. (optional) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UTexture2D* Media;

	FTutorialWidgetData(FText InTitle, FText InDescription, TArray<FTutorialDescriptionInput> InDescriptionInputs, bool bInGamepadDescription, FText InGamepadDescription, TArray<FTutorialDescriptionInput> InGamepadDescriptionInputs, UTexture2D* InMedia)
	{
		Title = InTitle;
		Description = InDescription;
		DescriptionInputs = InDescriptionInputs;
		bGamepadDescription = bInGamepadDescription;
		GamepadDescription = InGamepadDescription;
		GamepadDescriptionInputs = InGamepadDescriptionInputs;
		Media = InMedia;
	}

	FTutorialWidgetData()
	{
		Title = FText();
		Description = FText();
		DescriptionInputs = {};
		bGamepadDescription = false;
		GamepadDescription = FText();
		GamepadDescriptionInputs = {};
		Media = nullptr;
	}
};

USTRUCT(BlueprintType)
struct FRoom
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName Name = NAME_None;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	FIntVector Location = FIntVector::ZeroValue;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	class ADoor* RootDoor = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	TArray<class AThreatAwarenessActor*> Threats;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	TArray<class ADoor*> AdditionalRootDoors;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	TArray<FName> ConnectingRooms;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	bool bClearedBySwat = false;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	bool bClearedByTrailers = false;

	float TimeNotLookingAtRoom = 0.0f;

	friend bool operator==(const FRoom& Lhs, const FRoom& Rhs)
	{
		return Lhs.Name == Rhs.Name;
	}
};

USTRUCT(BlueprintType)
struct FRoomGenData
{
	GENERATED_BODY()

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, meta = (TitleProperty = "Name"))
	TArray<FRoom> Rooms;
	
	TArray<FRoom*> ClearedRooms;
};

USTRUCT(BlueprintType)
struct FClearPoint
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category=Location, meta=(MakeEditWidget=true))
	FVector Location_Relative = FVector::ZeroVector;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category=Location)
	FVector Location = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	uint8 Stage = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EClearDirection Direction = EClearDirection::None;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<class ACoverLandmark*> CoverLandmarks;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bHasLineOfSightToDoor = false;
	
	bool bCleared = false;

	friend bool operator==(const FClearPoint& Lhs, const FClearPoint& Rhs)
	{
		return Lhs.Location == Rhs.Location;
	}
};

USTRUCT(BlueprintType)
struct FSharedTeamData
{
	GENERATED_BODY()
	
	void Reset() { *this = FSharedTeamData(); }

	// Used to sync up activities of the same type when checking team related stuff
	FGuid ActivityId = FGuid();
	
	UPROPERTY(BlueprintReadOnly)
	FVector CommandLocation = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly)
	FVector PreviousCommandLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite)
	ETeamType CommandTeam = ETeamType::TT_NONE;

	UPROPERTY(BlueprintReadOnly)
	uint8 NumInTeam = 0;
};

USTRUCT(BlueprintType)
struct FSharedFallInData : public FSharedTeamData
{
	GENERATED_BODY()
	
	void Reset() { *this = FSharedFallInData(); }

	EFallInPattern Pattern = EFallInPattern::Snake;
};

USTRUCT(BlueprintType)
struct FSharedStackUpData : public FSharedTeamData
{
	GENERATED_BODY()
	
	void Reset() { *this = FSharedStackUpData(); }

	UPROPERTY()
	class ADoor* StackUpDoor = nullptr;
	
	UPROPERTY()
	class ACyberneticCharacter* DoorChecker = nullptr;
	
	UPROPERTY()
	TArray<class ASWATCharacter*> StackUpSortedSwat;
	
	UPROPERTY()
	ADoor* DoorToClose = nullptr;
	
	FVector CheckLocation = FVector::ZeroVector;

	EStackUpStyle StackUpStyle = EStackUpStyle::Auto;
	EStackUpStyle PreviousStackUpStyle = EStackUpStyle::Auto;

	EDoorRoomPosition StackingRoomPosition = EDoorRoomPosition::Center;
	
	EDoorCheckResult DoorCheckResult = EDoorCheckResult::None;
	
	FString DoorCheckAnimMontage;
	
	FRoom* Room = nullptr;
	FRoom* CurrentRoom = nullptr;
	
	uint8 bNewStackUpDoor : 1;
	uint8 bStackOppositeSide : 1;
	uint8 bHasCheckedDoor : 1;
	uint8 bHasStartedCheckingLock : 1;
	uint8 bPreviousCommandFrontOfDoor : 1;
	uint8 bWasSplitUp : 1;
};

USTRUCT(BlueprintType)
struct FSharedBreachData : public FSharedStackUpData
{
	GENERATED_BODY()
	
	void Reset() { *this = FSharedBreachData(); }

	// Door users are required (unless StackUp door is a doorway), they wait for breachers to get ready and 'use' the door (kick, c2, ram, shotgun or open, or custom action)
	UPROPERTY()
	class ACyberneticCharacter* DoorUser = nullptr;
	
	// Door breachers are optional, if in the case of no breach item was specified
	UPROPERTY()
	class ACyberneticCharacter* DoorBreacher = nullptr;

	// The door scanner scans the room for threats before making entry (eg. PIE or Center Check)
	UPROPERTY()
	class ACyberneticCharacter* DoorScanner = nullptr;
	
	UPROPERTY()
	class ACyberneticCharacter* ShieldUser = nullptr;

	UPROPERTY()
	class ACyberneticCharacter* BreachCaller = nullptr;

	// The item to breach the door with, used by the DoorBreacher (eg. flashbang, stinger, bazooka, etc..)
	UPROPERTY()
	TSubclassOf<class ABaseItem> DoorBreachItemClass = nullptr;
	
	// The item to use the door with, used by the DoorUser (eg. c2, ram, shotgun, kick, etc..)
	UPROPERTY()
	TSubclassOf<class ABaseItem> DoorUseItemClass = nullptr;
	
	UPROPERTY()
	class UDoorBreachActivity* DoorUseActivity = nullptr;
	
	UPROPERTY()
	class UDoorBreachActivity* DoorBreachActivity = nullptr;
	
	UPROPERTY()
	class UScanDoorActivity* DoorScanActivity = nullptr;
	
	UPROPERTY()
	TArray<ASWATCharacter*> ClearingSortedSwat = {};

	EDoorBreachType DoorBreachType = EDoorBreachType::None;
	EThresholdAssessment Assessment = EThresholdAssessment::None;
	EClearingStyle ClearingStyle = EClearingStyle::PointsOfDomination;
	EEntryMethod FirstEntryMethod = EEntryMethod::Flow;
	
	EDoorRoomPosition BreachingRoomPosition = EDoorRoomPosition::Center;
	
	float ClearingTime = 0.0f;
	float BreachingTime = 0.0f;
	
	bool bCalledOutLeftOpening = false;
	bool bCalledOutRightOpening = false;
	bool bCalledOutFrontOpening = false;
	
	bool bCalledOutBorderPatrol = false;
	
	uint8 bIsLeaderBreach : 1;
	uint8 bIsLeaderThrow : 1;
	uint8 bIsBreaching : 1;
	uint8 bBreacherReady : 1;
	uint8 bHasBreacherBreached : 1;
	uint8 bHasLeaderBreached : 1;
	uint8 bLeaderUsedItem : 1;
	uint8 bHasUserBreached : 1;
	uint8 bHasChosenScanner : 1;
	uint8 bIsAuto : 1;
	uint8 bLastForAutoClear : 1;
};

USTRUCT(BlueprintType)
struct FPathAwarenessInfo
{
	GENERATED_BODY()

	FPathAwarenessInfo() {}
	FPathAwarenessInfo(AActor* InActor, FVector InLocation, FVector InSensedFrom)
	{
		Actor = InActor;
		Location = InLocation;
		SensedFrom = InSensedFrom;
		Age = 0.0f;
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AActor* Actor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Location = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector SensedFrom = FVector::ZeroVector;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Age = 0.0f;
};
