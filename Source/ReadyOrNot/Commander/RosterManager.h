// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "RosterManager.generated.h"

class UCommanderProfile;

UENUM(BlueprintType)
enum class ERosterCharacterState : uint8
{
	Available,
	Incapacitated,
	InTherapy,
	Deceased
};

UENUM(BlueprintType)
enum class ERosterSquadPosition : uint8
{
	Unassigned,
	RedOne,
	RedTwo,
	BlueOne,
	BlueTwo
};

UENUM(BlueprintType)
enum class ERosterRemovalReason : uint8
{
	None,
	Deceased,
	Overstressed,
	Fired
};

USTRUCT(BlueprintType)
struct FRosterLoadout
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<ABaseItem> Primary;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<ABaseItem> Secondary;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<ABaseItem> LongTactical;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TSubclassOf<ABaseItem>> GrenadeSlots;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TSubclassOf<ABaseItem>> TacticalSlots;
};

UCLASS(BlueprintType)
class URosterTrait : public UDataAsset
{
	GENERATED_BODY()

public:
  	// Name used to reference this trait in code
	UPROPERTY(BlueprintReadOnly)
	FName Reference;
	
	// Name to represent this trait in the user interface
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Name;

	// Description provided in some contexts by this trait
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Description;

	// How to format the trait's values using regular printf-style formatting
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Format = FText::FromString(".0f%");
	
	// The curve to follow for this trait, where X is the number of officers and Y is the trait output
	UPROPERTY(EditAnywhere)
	FRuntimeFloatCurve TraitCurve;
};

USTRUCT()
struct FRosterTraitEntry
{
	GENERATED_BODY()

	// Name used to reference this trait in code
	UPROPERTY(EditAnywhere)
	FName Name;

	// Optionally disable this trait without removing it
	UPROPERTY(EditAnywhere)
	bool bEnabled = true;

	// The trait asset which contains the trait's curve and other data
	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<URosterTrait> Trait;
};

UCLASS()
class URosterStoryline : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, meta=(MultiLine=true))
	TArray<FText> DeathEvents;

	UPROPERTY(EditAnywhere, meta=(MultiLine=true))
	TArray<FText> UnfitForDutyEvents;

	UPROPERTY(EditAnywhere, meta=(MultiLine=true))
	TArray<FText> ReturnFromIncapacitationEvents;
	
	UPROPERTY(EditAnywhere, meta=(MultiLine=true))
	TArray<FText> ReturnFromTherapyEvents;

	UPROPERTY(EditAnywhere, meta=(MultiLine=true))
	TArray<FText> StressQuitEvents;

	UPROPERTY(EditAnywhere, meta=(MultiLine=true))
	TArray<FText> TherapistEvents;

	UPROPERTY(EditAnywhere, meta=(MultiLine=true))
	TArray<FText> TherapistAssessmentEvents;
};

USTRUCT()
struct FTherapistReminderEvent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	float SquadAverageStressRequired = 0.0f;

	UPROPERTY(EditAnywhere, meta=(MultiLine=true))
	FText EventText;
};

UCLASS()
class URosterEventData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	TArray<URosterStoryline*> Storylines;
	
	UPROPERTY(EditAnywhere)
	TArray<FTherapistReminderEvent> TherapistReminderEvents;
};

/**
 *
 */
UCLASS(BlueprintType)
class URosterCharacterArchetype : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<class UCustomizationCharacter*> CharacterPool;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<class UCustomizationVoice*> VoicePool;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FText> BackgroundTextPool;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FText> FirstNamePool;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FText> LastNamePool;
};

/**
 * 
 */
UCLASS(BlueprintType)
class READYORNOT_API URosterCharacter : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
	ERosterCharacterState State;

	UPROPERTY(BlueprintReadOnly)
	ERosterSquadPosition Position;

	UPROPERTY(BlueprintReadOnly)
	ERosterRemovalReason RemovalReason;
	
	UPROPERTY(BlueprintReadOnly)
	int32 MissionsPlayed;
	
	UPROPERTY(BlueprintReadOnly)
	float StressLevel;
	
	UPROPERTY(BlueprintReadOnly)
	int32 MissionsUntilReturn;

	UPROPERTY(Transient, BlueprintReadOnly)
	bool bIsNew;
	
	UPROPERTY(BlueprintReadOnly)
	ERosterCharacterState PreviousState;
	
	UPROPERTY(BlueprintReadOnly)
	float PreviousStressLevel;

	UPROPERTY(BlueprintReadOnly)
	bool bTherapistIntervened;
	
	UPROPERTY(BlueprintReadOnly)
	URosterTrait* Trait;

	UPROPERTY(BlueprintReadOnly)
	bool bTraitUnlocked;

	UPROPERTY(BlueprintReadOnly)
	bool bJustUnlockedTrait;
	
	UPROPERTY(BlueprintReadOnly)
	FText MostRecentEventText;
	
	UPROPERTY(BlueprintReadOnly)
	URosterCharacterArchetype* Archetype;

	UPROPERTY(BlueprintReadOnly)
	class UCustomizationCharacter* Character;

	UPROPERTY(BlueprintReadOnly)
	class UCustomizationVoice* Voice;
	
	UPROPERTY(BlueprintReadOnly)
	FText FirstName;

	UPROPERTY(BlueprintReadOnly)
	FText LastName;

	UPROPERTY(BlueprintReadOnly)
	int32 SerialNumber;

	UPROPERTY(BlueprintReadOnly)
	int32 YearsInSWAT;

	UPROPERTY(BlueprintReadOnly)
	FText Description;

	UPROPERTY(BlueprintReadOnly)
	URosterStoryline* Storyline;
};

UCLASS(Config=Commander, DefaultConfig, meta=(DisplayName="Roster Manager"))
class READYORNOT_API URosterManagerSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return NAME_Game; }

	// Starting number of character slots in the roster
	UPROPERTY(Config, EditAnywhere)
	int32 RosterSize = 10;

	// Amount of characters available for recruitment at a time
	UPROPERTY(Config, EditAnywhere)
	int32 NumRecruitableCharacters = 3;
	
	// Additional roster slots, where each entry represents a slot that is unlocked after that many missions
	UPROPERTY(Config, EditAnywhere)
	TArray<int32> AdditionalRosterSlots = { 3, 6, 9 };
	
	// Maximum starting stress value for new characters
	UPROPERTY(Config, EditAnywhere)
	float MaximumStartingStress = 0.35f;

	// Traits available in-game, traits not in the list will be ignored
	UPROPERTY(Config, EditAnywhere)
	TArray<FRosterTraitEntry> AvailableTraits;

	// How many missions a squad member must participate in before they can unlock their trait
	UPROPERTY(Config, EditAnywhere)
	int32 MissionsUntilTraitUnlockable = 2;

	// Asset defining event texts for the roster
	UPROPERTY(Config, EditAnywhere)
	TSoftObjectPtr<URosterEventData> EventData;
	
	// Maximum chance for a roster character to die instead of being incapacitated
	UPROPERTY(Config, EditAnywhere)
	float DeathChance = 0.5f;

	// Maximum time for a character to be incapacitated for
	UPROPERTY(Config, EditAnywhere)
	int32 MaxIncapacitationTime = 3;

	// When a character is incapacitated cap their stress to this value for balance
	UPROPERTY(Config, EditAnywhere)
	float MaxStressWhenIncapacitated = 0.9f;
	
	// Max number of officers that can be fired at once while not on a mission
	UPROPERTY(Config, EditAnywhere)
	int32 MaxOfficersFiredAtOnce = 1;
	
	// Stress gain when firing an officer
	UPROPERTY(Config, EditAnywhere)
	float StressGainForOfficerFired = 0.15f;

	// Maximum stress an officer can reach after another officer is fired
	UPROPERTY(Config, EditAnywhere)
	float MaxStressForOfficerFired = 0.9f;
	
	// Stress gain for squadmates whenever a fellow officer is killed in a mission
	UPROPERTY(Config, EditAnywhere)
	float StressGainForOfficerKilled = 0.2f;

	// Stress gain for the team when the player character is killed in a mission
	UPROPERTY(Config, EditAnywhere)
	float StressGainForPlayerKilled = 0.25f;
	
	// Stress gain for officer when exfiltrated from a mission
	UPROPERTY(Config, EditAnywhere)
	float StressGainForExfil = 0.05f;

	// Stress gain for officer when exfiltrated from an active shooter mission
	UPROPERTY(Config, EditAnywhere)
	float StressGainForActiveShooterExfil = 0.25f;

	// Stress gain for officer when left behind when exfiltrating a mission
	UPROPERTY(Config, EditAnywhere)
	float StressGainForOfficerNotExfil = 1.0f;

	// Initial stress value whenever a player kills a friendly or otherwise surrendered character
	UPROPERTY(Config, EditAnywhere)
	float StressGainForFriendlyKilledByPlayer = 0.05f;

	// Base for exponential growth of the stress gained for friendly kills
	UPROPERTY(Config, EditAnywhere)
	float StressGainForFriendlyKilledByPlayerBase = 3.0f;
	
	// Minimum random base stress gain for all squadmates when a suspect is killed during a mission
	UPROPERTY(Config, EditAnywhere)
	float MinBaseStressGainForKill = 0.05f;

	// Maximum random base stress gain for all squadmates when a suspect is killed during a mission
	UPROPERTY(Config, EditAnywhere)
	float MaxBaseStressGainForKill = 0.1f;

	// Multiplier applied to the squadmate responsible for a kill
	UPROPERTY(Config, EditAnywhere)
	float StressMultiplierForKillInstigator = 1.3f;

	// Multiplier applied whenever a civilian is killed
	UPROPERTY(Config, EditAnywhere)
	float StressMultiplierForCivilianKill = 2.0f;

	// Stress loss for all squadmates when a civilian is arrested
	UPROPERTY(Config, EditAnywhere)
	float StressLossForCivilianArrest = 0.05f;

	// Stress loss for all squadmates when a suspect is arrested
	UPROPERTY(Config, EditAnywhere)
	float StressLossForSuspectArrest = 0.1f;

	// Stress lost passively per successful mission by inactive SWAT members
	UPROPERTY(Config, EditAnywhere)
	float PassiveStressLoss = 0.05f;

	// The max number of characters that can be in therapy at a time
	UPROPERTY(Config, EditAnywhere)
	int32 MaxCharactersInTherapy = 3;
	
	// Minimum stress level before you are able to send a character to the therapist
	UPROPERTY(Config, EditAnywhere)
	float MinimumStressForTherapy = 0.5f;
	
	// Scalar for number of missions away when using therapist (i.e. nearly 100% stress will have 4 missions away)
	UPROPERTY(Config, EditAnywhere)
	float TherapistTimeScale = 4.0f;
};

/**
 * 
 */
UCLASS()
class READYORNOT_API URosterManager : public UObject
{
	GENERATED_BODY()

public:
	URosterManager();
	
	void SaveToProfile(UCommanderProfile* Profile);
	void LoadFromProfile(UCommanderProfile* Profile);

	void StartMission();
	void CompleteMission(const FString& MapName);
	void PreReview();
	
	/* General */
	const TArray<URosterCharacter*>& GetAllCharacters() { return Characters; }
	const TArray<URosterCharacter*>& GetPreviousCharacters() { return PreviousCharacters; }
	FRosterLoadout GetCharacterLoadout(URosterCharacter* Character);

	const URosterEventData* GetEventData() const { return EventData; }
	
	/* Roster Size */
	int32 GetCurrentRosterSize() const;
	int32 GetMaximumRosterSize() const;
	TArray<int32> GetUnlockableSlotsMissionsRemaining() const;
	bool IsRosterSlotUnlocked() const;
	
	/* Squad */
	const TMap<ERosterSquadPosition, URosterCharacter*>& GetSquadCharacters() { return SquadCharacters; }
	URosterCharacter* GetSquadCharacter(ERosterSquadPosition SquadPosition, bool bGenerateStandIn);
	void SetCharacterSquadPosition(URosterCharacter* TargetCharacter, ERosterSquadPosition Position);

	/* Recruitment */
	const TArray<URosterCharacter*>& GetRecruitableCharacters() { return RecruitableCharacters; }
	void RecruitCharacter(URosterCharacter* Character);
	void FireCharacter(URosterCharacter* Character);
	bool CanFireCharacter();
	
	/* Therapist */
	void SetCharacterInTherapy(URosterCharacter* TargetCharacter);
	bool CanUseTherapist(URosterCharacter* Character);
	bool IsTherapyFull() const;
	int32 GetNumCharactersInTherapy() const;
	int32 GetMaxCharactersInTherapy() const;
	
	/* Traits */
	const TMap<FName, URosterTrait*>& GetPossibleTraits() const { return PossibleTraits; }
	float GetSquadTraitValue(FName Trait) const;
	float GetSquadTraitValue(FName Trait, int32& OutCount) const;
	static float GetSquadTraitValue(FName Trait, UWorld* World);

	void OnPlayerKilled(AReadyOrNotCharacter* Instigator);
	void OnCharacterKilled(URosterCharacter* Character, bool bForceIncapacitation = false);
	void OnSuspectKilled(URosterCharacter* Instigator, bool bCivilian);
	void OnSuspectArrested(URosterCharacter* Instigator, bool bCivilian);

	void OnFriendlyOrSurrenderedKilled();
	
	void OnExfiltratedMission(UWorld* World, TArray<ASWATCharacter*> ExfiltratedOfficers, bool bActiveBombThreat, bool bIsActiveShooter);
	
private:
	UPROPERTY()
	const URosterManagerSettings* Settings;
	
	UPROPERTY()
	UCommanderProfile* CommanderProfile;
	
	UPROPERTY()
	TArray<URosterCharacter*> Characters;

	UPROPERTY()
	TArray<URosterCharacter*> PreviousCharacters;

	UPROPERTY()
	TArray<URosterCharacter*> RecruitableCharacters;
	
	UPROPERTY()
	TMap<ERosterSquadPosition, URosterCharacter*> SquadCharacters;

	UPROPERTY()
	TArray<URosterCharacterArchetype*> PossibleCharacters;
	
	UPROPERTY()
	TMap<FName, URosterTrait*> PossibleTraits;

	UPROPERTY()
	URosterEventData* EventData;
	
	UPROPERTY()
	URosterCharacterArchetype* LastArchetype;
	
	FRandomStream RandomStream;
	int32 FiredCount;
	TSet<int32> UsedSerialNumbers;

	int32 CharactersKilled;
	int32 PlayerFriendlyKills;
	bool bHasTickedTimers = false;
	
	void ProcessRoster();
	void ProcessTraitUnlocks();
	void ValidateRoster();
	void ValidateSquad();

	void TickTimers();
	
	void GenerateCharacters();
	void GenerateRecruitableCharacters();
	
	void GenerateCharacters(TArray<URosterCharacter*>& OutCharacters, int32 Count);
	URosterCharacter* GenerateCharacter();

	TArray<URosterCharacterArchetype*> GetAllCharacterArchetypes();
};