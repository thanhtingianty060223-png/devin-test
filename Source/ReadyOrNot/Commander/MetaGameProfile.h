// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "BaseProfile.h"
#include "CommanderGM.h"
#include "MetaGameProfile.generated.h"

/*
 * Holds temporary data for review that is cleared on return to the station
 */
USTRUCT(BlueprintType)
struct READYORNOT_API FMetaGameProfileTemporaryData
{
	GENERATED_BODY()

	// Completed levels since last lobby visit
	UPROPERTY(BlueprintReadOnly)
	TArray<FString> NewCompletedLevels;
	
	// Progression tags added since last lobby visit
	UPROPERTY(BlueprintReadOnly)
	TSet<FName> NewProgressionTags;

	/** Exfil data. Needed here for if player leaves mission by returning to main menu etc */
	UPROPERTY(BlueprintReadOnly)
	FExfiltrationData ExfilData;
};

/**
 * Meta game profile, represents practice mode progress and other globally relevant data
 */
UCLASS()
class READYORNOT_API UMetaGameProfile : public UBaseProfile
{
	GENERATED_BODY()

public:
	UMetaGameProfile();

	virtual void Serialize(FArchive& Ar) override;

	static UMetaGameProfile* GetProfile(UObject* WorldContextObject);
	
	void SaveProfile();
	static UMetaGameProfile* LoadProfile();

	const TArray<FString>& GetCompletedLevels() { return CompletedLevels; }
	const TArray<FString>& GetCompletedMultiplayerLevels() { return CompletedMultiplayerLevels; }
	const TSet<FName>& GetProgressionTags() { return ProgressionTags; }
	const FMetaGameProfileTemporaryData& GetTemporaryData() { return TemporaryData; }

	void AddCompletedLevel(const FString& Level);
	void AddCompletedMultiplayerLevel(const FString& Level);
	void AddProgressionTag(FName Tag);
	void AddProgressionTags(const TSet<FName>& Tags);
	/** Add mission exfil data (Did player exfiltrate mission/exit to main menu, were there active threats etc) to temporary data */
	void AddExfilData(FExfiltrationData ExfilData);
	
	void ClearTemporaryData() { TemporaryData = FMetaGameProfileTemporaryData(); }
	
private:
	// Version number for tracking if this save needs to be upgraded
	UPROPERTY()
	int32 MetaGameVersion;

public:
	// Last save loaded for commander mode
	UPROPERTY(BlueprintReadOnly)
	FString LastCampaignSave;
	
	// Total number of times the player has logged into the lobby
	UPROPERTY(BlueprintReadOnly)
	int32 TotalLobbyLogins = 0;

	// Whether or not the player has ever completed the tutorial
	UPROPERTY(BlueprintReadOnly)
	bool bHasCompletedTutorial = false;

private:
	// Levels completed across practice mode and commander mode
	UPROPERTY()
	TArray<FString> CompletedLevels;

	// Levels completed during multiplayer
	UPROPERTY()
	TArray<FString> CompletedMultiplayerLevels;
	
	// Tags used to record player progression
	UPROPERTY()
	TSet<FName> ProgressionTags;
	
	// Temporary data cleared on return to the game lobby
	UPROPERTY()
	FMetaGameProfileTemporaryData TemporaryData;
};
