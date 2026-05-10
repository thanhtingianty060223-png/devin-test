// � Void Interactive, 2017

#pragma once
#include "GameFramework/SaveGame.h"
#include "Profile.generated.h"

//////////////////////////////////////////////////////
//
//	Profile base (URoNProfile)

USTRUCT(BlueprintType)
struct FBasicLevelStats
{
	GENERATED_BODY()

	// best rating in %
	UPROPERTY(BlueprintReadWrite)
		float BestRating;

	UPROPERTY(BlueprintReadWrite)
		float BestTime;

	UPROPERTY(BlueprintReadWrite)
		int32 TimesCompleted;
};

UCLASS(BlueprintType)
class READYORNOT_API UReadyOrNotProfile : public USaveGame
{
	GENERATED_BODY()

public:
	UReadyOrNotProfile();

	///////////////////////////////////////////////////////
	//
	//	Boilerplate code needed for the profile system overall

	UPROPERTY(VisibleAnywhere, Category = Basic)
	FString SaveSlotName;

	UPROPERTY(VisibleAnywhere, Category = Basic)
	uint32 UserIndex;

	// Reset the multiplayer profile
	UFUNCTION(BlueprintCallable, Category = "Profiles|Multiplayer")
	virtual void ResetProfile();

	UFUNCTION(BlueprintCallable, Category = "Profiles|Multiplayer")
	virtual void SaveProfile();

	// Default savegame
	UFUNCTION()
	static UReadyOrNotProfile* CreateDefaultSavegame(TSubclassOf<UReadyOrNotProfile> ProfileClass, FString LoadSlotName);

	//////////////////////////////////////////////////////
	//
	//	The actual saved data

	// Stats on level completion
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = LevelStat)
	TMap<FString, FBasicLevelStats> LevelStats;

	// Progress on challenges
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ChallengeStat)
	TMap<FName, int32> ChallengeProgress;
	
	UFUNCTION(BlueprintCallable, Category = "COOP")
	static void SaveLevelStats(FBasicLevelStats InStats, bool& NewBestRating, bool& NewBestTime);

	UFUNCTION(BlueprintCallable, Category = "COOP")
	static void LoadLevelStats(FBasicLevelStats& OutStats, ECOOPMode Mode, FString MapName = "");
};

//////////////////////////////////////////////////////
//
//	Multiplayer Profile (URoNMultiplayerProfile)

UCLASS(BlueprintType)
class READYORNOT_API UReadyOrNotMultiplayerProfile : public UReadyOrNotProfile
{
	GENERATED_BODY()

public:
	UReadyOrNotMultiplayerProfile();

	virtual void ResetProfile() override;
};
