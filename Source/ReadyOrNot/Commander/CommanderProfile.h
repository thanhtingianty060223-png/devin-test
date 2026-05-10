// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "BaseProfile.h"
#include "CommanderProfile.generated.h"

USTRUCT()
struct FRosterCharacterSaveData
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<uint8> ObjectBytes;
};

USTRUCT()
struct READYORNOT_API FRosterSaveData
{
	GENERATED_BODY()

	UPROPERTY()
	bool bGenerateRoster = true;
	
	UPROPERTY()
	TArray<FRosterCharacterSaveData> Characters;

	UPROPERTY()
	TArray<FRosterCharacterSaveData> PreviousCharacters;
	
	UPROPERTY()
	int32 RosterSeed = -1;

	UPROPERTY()
	int32 FiredCount = 0;
	
	UPROPERTY()
	TSet<int32> UsedSerialNumbers;
};

USTRUCT()
struct READYORNOT_API FLobbySaveData
{
	GENERATED_BODY()

	// The name of the lobby level we saved on
	UPROPERTY()
	FString LevelName;
	
	// Whether or not the stored lobby player transform was set in code
	UPROPERTY()
	bool bPlayerTransformSet = false;
	
	// Position of the player when saving in the lobby
	UPROPERTY()
	FVector PlayerLocation;

	// Rotation of the player when saving in the lobby
	UPROPERTY()
	FRotator PlayerRotation;

	// Rotation of the player camera when saving in the lobby
	UPROPERTY()
	FRotator PlayerCameraRotation;
	
	bool IsValid() const { return bPlayerTransformSet && !PlayerLocation.ContainsNaN() && !PlayerRotation.ContainsNaN() && !PlayerCameraRotation.ContainsNaN(); }
};

/**
 * 
 */
UCLASS()
class READYORNOT_API UCommanderProfile : public UBaseProfile
{
	GENERATED_BODY()

public:
	UCommanderProfile();

	virtual void Serialize(FArchive& Ar) override;

	// Saves the profile to disk automatically populating additional information including save time, is modded, etc
	virtual void SaveProfile() override;
	
	// Loads a profile using the given slot filename
	static UCommanderProfile* LoadProfile(const FString& Slot);

	// Get the commander profile associated with a specific slot index. May return null
	UFUNCTION(BlueprintCallable, BlueprintPure)
	static UCommanderProfile* LoadProfile(int32 Slot);
	
	// Creates a profile, writes it to disk and returns it
	static UCommanderProfile* CreateProfile(const FString& Slot, bool bIronmanMode = false);
	
	// Creates a profile at the specified slot number, writes it to disk and returns it
	static UCommanderProfile* CreateProfile(int32 SlotIndex, bool bIronmanMode = false);

	// Get the commander debug profile
	UFUNCTION(BlueprintCallable, BlueprintPure)
	static UCommanderProfile* GetDebugProfile();
	
	// Deletes the profile from the disk
	UFUNCTION(BlueprintCallable)
	void DeleteProfile();

	// Get the name of the slot for this profile
	FString GetSlot() const { return Slot; }
	
	// Gets the friendly level name for the most recently completed level
	UFUNCTION(BlueprintCallable, BlueprintPure)
	FText GetMostRecentLevelName(FText NothingCompletedText) const;

	// Gets the most recently completed level's image
	UFUNCTION(BlueprintCallable, BlueprintPure)
	TSoftObjectPtr<UTexture2D> GetMostRecentLevelImage() const;

	// Gets the level asset name for the most recently incomplete level
	UFUNCTION(BlueprintCallable, BlueprintPure)
	FString GetNextLevel() const;
	
	// Gets the friendly level name for the most recently incomplete level
	UFUNCTION(BlueprintCallable, BlueprintPure)
	FText GetNextLevelName(FText CompletedText) const;

	// Gets the most recently incomplete level's image
	UFUNCTION(BlueprintCallable, BlueprintPure)
	TSoftObjectPtr<UTexture2D> GetNextLevelImage() const;
	
	// Gets the current completion percentage for this profile
	UFUNCTION(BlueprintCallable, BlueprintPure)
	float GetCompletionPercentage() const;

	// Checks for asset data mismatch between this profile and the current game instance (only when modded)
	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool IsChecksumMismatched() const;
	
private:
	// Version number for tracking if this save needs to be upgraded
	UPROPERTY()
	int32 CommanderVersion;

public:
	// The campaign that this profile is trying to complete
	UPROPERTY(BlueprintReadOnly)
	UCampaignData* Campaign;

	// List of levels that have been completed under this specific profile
	UPROPERTY(BlueprintReadOnly)
	TArray<FString> CompletedLevels;
	
	// Deprecated, not used in the AchievementsSubsystem anymore.
	// Completed ironman levels. Used for achievements. Updated from AchievementSubsystem.
	UPROPERTY(BlueprintReadOnly)
	TArray<FString> CompletedIronmanLevels;
	
	// Lost officers in ironman mode. Used for achievements. Updated from AchievementSubsystem.
	UPROPERTY(BlueprintReadOnly)
	int LostOfficers = 0;

	// Levels completed since the last visit to the lobby
	UPROPERTY(BlueprintReadOnly)
	TArray<FString> NewCompletedLevels;
	
	// Progression tags unlocked while playing on this specific profile
	UPROPERTY(BlueprintReadOnly)
	TSet<FName> ProgressionTags;
	
	// The date that this profile was last saved
	UPROPERTY(BlueprintReadOnly)
	FDateTime SaveDate;

	// The total in-game playtime for this profile
	UPROPERTY(BlueprintReadOnly)
	FTimespan TotalPlaytime;

	// Should this save be treated as ironman mode, set by the player when the save is created
	UPROPERTY(BlueprintReadOnly)
	bool bIronmanMode;
	
	// Whether or not this profile was using a modded game when last saved
	UPROPERTY(BlueprintReadOnly)
	bool bIsModded;

	// Checksum of the game when this profile was last saved
	UPROPERTY()
	int32 GameChecksum;

	// Flag set by commander game mode so the lobby knows we're returning
	UPROPERTY()
	bool bReturningFromMission = false;
	
	// Roster save data that holds all the non-transient data about this profile's roster
	UPROPERTY()
	FRosterSaveData RosterSaveData;
	
	// Lobby save data that represents the current lobby state
	UPROPERTY()
	FLobbySaveData LobbySaveData;

private:
	FString Slot;

	static FString GetSlotNameFromIndex(int32 SlotIndex);
};

