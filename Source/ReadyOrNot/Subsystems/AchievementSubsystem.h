// Copyright Void Interactive, 2023

#pragma once

#include <map>

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Subsystems/GameInstanceSubsystem.h"
#if defined(WITH_STEAM)
#include "SteamRequests.h"
#endif
#include "AchievementSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogReadyOrNotAchievements, Log, All);   // Logging for Achievements and online Stats

#define GAS_MISSION_NAME "ron_gas_core"
#define STREAMER_MISSION_NAME "ron_streamer_core" 
#define METH_MISSION_NAME "ron_meth_core"
#define AGENCY_MISSION_NAME "ron_agency_core"
#define RIDGELINE_MISSION_NAME "ron_ridgeline_core"
#define PENTHOUSE_MISSION_NAME "ron_penthouse_core"
#define DATACENTER_MISSION_NAME "ron_datacenter_core"
#define BEACHFRONT_MISSION_NAME "ron_beachfront_core"
#define IMPORTER_MISSION_NAME "ron_importer_core"
#define VALLEY_MISSION_NAME "ron_valley_core"
#define CAMPUS_MISSION_NAME "ron_campus_core"
#define COYOTE_MISSION_NAME "ron_coyote_core"
#define SINS_MISSION_NAME "ron_sins_core"
#define CLUB_MISSION_NAME "ron_club_core"
#define DEALER_MISSION_NAME "ron_dealer_core"
#define FARM_MISSION_NAME "ron_farm_core"
#define HOSPITAL_MISSION_NAME "ron_hospital_core"
#define PORT_MISSION_NAME "ron_port_core"

#define PYTHON_REVOLVER_NAME "Secondary_Python_V2_C"

#define SCORE_REQUIRED_C 0.7f
#define SCORE_REQUIRED_S 1.0f

#define VENDING_MACHINE_BEVERAGE_PREFIX "BP_CoffeeVendingMachine3.Select_"
#define VENDING_MACHINE_WATER "Water"
#define VENDING_MACHINE_HOT_WATER "HotWater"
#define VENDING_MACHINE_CHOCOLATE "Choccolate" // yes, it's spelled like this in the game
#define VENDING_MACHINE_CHOCOLATE_FORTE "ChoccolateForte"
#define VENDING_MACHINE_COFFEE_STRONG "CoffeeStrong"
#define VENDING_MACHINE_LEMON_TEA "LemonTea"
#define VENDING_MACHINE_GREEN_TEA "GreenTea"
#define VENDING_MACHINE_COLD_GREEN_TEA "ColdGreenTea"

// From Steam docs:
// There are three types of statistics your game can store:
// INT - A 32-bit (signed) integer (e.g. number of games played)
// FLOAT - A 32-bit floating point value (e.g. number of miles driven)
// AVGRATE - A moving average. See: The AVGRATE stat type

// We use stats, stored online to survive save game wipes.

UENUM(BlueprintType, Category = "Simple Steam Stats & Achievements")
enum class EUniversalStatType : uint8
{
	STAT_INT      UMETA(DisplayName = "Integer"),
	STAT_FLOAT    UMETA(DisplayName = "Float"),
	STAT_AVGRATE  UMETA(DisplayName = "Average"),
};

USTRUCT(BlueprintType, Category = "Stats for Achievments, Crossplatform")
struct FUniversalStat
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Simple Steam Stats & Achievements")
	EUniversalStatType StatType;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Simple Steam Stats & Achievements")
	int32 IntegerValue;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Simple Steam Stats & Achievements")
	float FloatValue;
	
	FUniversalStat()
	{
		StatType = EUniversalStatType::STAT_INT;
		IntegerValue = 0;
		FloatValue = 0.f;
	}

#if defined(WITH_STEAM)
	FUniversalStat(FSteamStat SteamStat)
	{
		StatType = EUniversalStatType::STAT_INT;
		IntegerValue = 0;
		FloatValue = 0.f;
		if (SteamStat.StatType == ESteamStatType::STAT_FLOAT)
		{
			StatType = EUniversalStatType::STAT_FLOAT;
			FloatValue = SteamStat.FloatValue;
		}
		else if (SteamStat.StatType == ESteamStatType::STAT_INT) 
		{
			StatType = EUniversalStatType::STAT_INT;
			IntegerValue = SteamStat.IntegerValue;
		}
	}
#endif
};

// actual achievements, need setup on each respective platform
UENUM(BlueprintType)
enum class EAchievement : uint8
{
	THE_WAR,            // A1, Progressive 0-3
	THE_DECAYING_CITY,  // A2, Progressive 0-3
	THE_LEFT_BEHIND,    // A3, Progressive 0-4
	THE_ABDUCTED,       // A4, Progressive 0-4
	THE_EXPLOITED,      // A5, Progressive 0-4
	MEDAL_OF_VALOR,     // A6, Progressive 0-18
	THE_WORLD,          // A7, Boolean
	THE_HERMIT,         // A8, Boolean
	THE_EMPEROR,
	THE_AUDIOPHILE,
	//READY_FOR_DUTY,   // Not used
	DRESS_TO_IMPRESS,
	TEMPERANCE,
	THE_FOOL,           // A13, Boolean
	THE_HANGED_MAN,     // A9, Boolean
	WAY_OUT_WEST,       // A10, Boolean
	THE_DEVIL,          // A11, Boolean
	THE_MAGICIAN,       // A12, Boolean
	FIRST_ARREST,       // ACH_FIRST_ARREST, Boolean
	DUE_PROCESS,        // A14, Boolean
	INKED_UP,
	PINE_PAINBRINGER,   // BREACH Progressive 0-20
	WALNUT_WARRIOR,     // BREACH Progressive 0-50
	MAHOGANY_MASOCHIST, // BREACH Progressive 0-100
	ARREST_WARRANT,     // ARREST Progressive 0-50
	POWERS_OF_ARREST,   // ARREST Progressive 0-100
	REST_FOR_THE_WICKED,// ARREST Progressive 0-200
	WELL_SEASONED,
	HERES_JOHNNY,       // BATTERING_RAM Progressive 0-20
	PEEPING_TOM,        // MIRRORGUN Progressive 0-30
	SAY_HELLO,          // M320 Progressive 0-30
	CLICK_FROM,         // LOCKPICK Progressive 0-20
	TOXIC_FUMES,
	DOOR_KICKERS,
	THE_TACTICIAN,
	BY_THE_BOOK,
	YOU_WERE_READY
};
ENUM_RANGE_BY_FIRST_AND_LAST(EAchievement, EAchievement::THE_WAR, EAchievement::YOU_WERE_READY);

UENUM()
enum class EAchievementStats : uint8
{
	SCORE_GAS,			
	SCORE_COYOTE,
	SCORE_METH,
	SCORE_CAMPUS,
	SCORE_HOSPITAL,
	SCORE_CLUB,
	SCORE_FARM,
	SCORE_SINS,
	SCORE_PENTHOUSE,
	SCORE_RIDGELINE,
	SCORE_DEALER,
	SCORE_PORT,
	SCORE_BEACHFRONT,
	SCORE_IMPORTER,
	SCORE_AGENCY,
	SCORE_VALLEY,
	SCORE_DATACENTER,
	SCORE_STREAMER,
	PROGRESS_A1,
	PROGRESS_A2,
	PROGRESS_A3,
	PROGRESS_A4,
	PROGRESS_A5,
	PROGRESS_A6,
	PROGRESS_BREACH,
	PROGRESS_ARREST,
	PROGRESS_BATTERING_RAM,
	PROGRESS_MIRRORGUN,
	PROGRESS_M320,
	PROGRESS_LOCKPICK

	// could also be a simple counter like STAT_ARRESTS or STAT_KILLS
};
ENUM_RANGE_BY_FIRST_AND_LAST(EAchievementStats, EAchievementStats::SCORE_GAS, EAchievementStats::PROGRESS_LOCKPICK);

class URequestStatsAndAchievements;
class UStoreStatsAndAchievements;
class UIndicateAchievementProgress;
class ULocalAchievements;

DECLARE_DELEGATE_OneParam(FOnQueryAchievementsAndStatsCompleteDelegate, const bool);

/**
 * 
 */
UCLASS()
class READYORNOT_API UAchievementSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	void RetrySteamStats();

protected:
	// maps to translate to proper backend id's
	static const TMap<EAchievementStats, FString> SteamStatNames; 
	static const TMap<EAchievementStats, EUniversalStatType> StatTypes;
	static const TMap<FString, EAchievementStats> LevelToEAchievementStat;
	static const TMap<EAchievement, FString> SteamAchievementNames;
	
	// current inmemory stats 
	UPROPERTY()
	TMap<EAchievementStats, FUniversalStat> AchievementStats;

	// Will be used later on console
	// static const TMap<EAchievementStats, FString> XboxStatNames;
	// static const TMap<EAchievement, FString> XboxAchievementNames;
	// static const TMap<EAchievementStats, FString> PSNStatNames;
	// static const TMap<EAchievement, FString> PSNAchievementNames;
	
private:
	bool bInitialized = false;
	
	int BeverageCount = 0;
	
	uint32 LastFrameNumberWeTicked = INDEX_NONE;

	bool bAchievementsOrStatsDirty = false;
	
	UPROPERTY()
	TSet<ABaseItem*> UsedItemsInCurrentMission;
	
	UFUNCTION()
	bool HasOnlyUsedItems(TArray<EItemCategory> Categories);
	
	TSet<EAchievement> UnlockedAchievements;
	
	void QueryStatsConsole();
	void QueryAchievementsAndStats();
	
	float GetLevelScoreStat(EAchievementStats Level);
	int GetProgressStat(EAchievementStats Level);
	
	UPROPERTY()
	ULocalAchievements* LocalAchievements;

	// used only on steam 
	// UPROPERTY()
	// URequestStatsAndAchievements *RequestStatAndAchievements;

	UFUNCTION()
	void QueryAchievementsAndStatsSuccess(int32 SteamErrorOutput);

	UFUNCTION()
	void QueryAchievementsAndStatsFailure(int32 SteamErrorOutput);

	// UPROPERTY()
	// UStoreStatsAndAchievements *StoreStatsAndAchievements;

	UFUNCTION()
	void StoreStatsAndAchievementsSuccess(int32 SteamErrorOutput);

	UFUNCTION()
	void StoreStatsAndAchievementsFailure(int32 SteamErrorOutput);

	// UPROPERTY()
	// UIndicateAchievementProgress *UserAchievementProgress;

	UFUNCTION()
	void UserAchievementProgressSuccess(int32 SteamErrorOutput);

	UFUNCTION()
	void UserAchievementProgressFailure(int32 SteamErrorOutput);

	UFUNCTION()
	void IndicateUserAchievementProgress(EAchievement Achievement, int Progress, int MaxProgress);

	void UpdateStat(EAchievementStats Stat, float value);
	void UpdateIronmanAchievements(FString SafeMissionName);

	bool GetUserStoredStats(TArray<FUniversalStat>& StatsOut);
	bool GetAchievementData(EAchievement Achievement, bool& AchievementUnlocked);
	FUniversalStat GetBestStat(FUniversalStat FirstStat, FUniversalStat SecondStat, bool& dirty);

	void UpdateAchievedTraits();
	void CacheCustomizationRequirements();
	void CheckCustomizationAchievement(const TArray<FName> &TagRequirements, EAchievement Achievement);

	TArray<FName> TagRequirementsForAllCustomItems;
	TArray<FName> TagRequirementsForAllTattoos;

	const TArray<ECustomizationType> ItemCustomizationTypes = TArray<ECustomizationType> {
		ECustomizationType::Helmet,
		ECustomizationType::Shirt,
		ECustomizationType::Pants,
		ECustomizationType::Shoes,
		ECustomizationType::Eyewear,
		ECustomizationType::Belt,
		ECustomizationType::Gloves,
		ECustomizationType::Watch,
	};

	const TArray<FString> CaffeineFreeBeverages = TArray<FString> {
		VENDING_MACHINE_WATER,
		VENDING_MACHINE_HOT_WATER,
		VENDING_MACHINE_CHOCOLATE,
		VENDING_MACHINE_CHOCOLATE_FORTE,
	};

	const TArray<FString> LessCaffeineBeverages = TArray<FString> {
		VENDING_MACHINE_LEMON_TEA,
		VENDING_MACHINE_GREEN_TEA,
		VENDING_MACHINE_COLD_GREEN_TEA
	};

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
	void LateInitialize();

	virtual ETickableTickType GetTickableTickType() const override
	{
		return ETickableTickType::Always;
	}

	virtual TStatId GetStatId() const override
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT( UAchievementSubsystem, STATGROUP_Tickables );
	}

	virtual bool IsTickableWhenPaused() const
	{
		return true;
	}
	virtual bool IsTickableInEditor() const
	{
		return false;
	}
	
	virtual void Tick( float DeltaTime ) override;
	
	// call this when a level is completed, writes the result to a stat
	void UpdateLevelScore(FString Level, float PercentageScore);

	// set arbitrary int - stat
	void SetAchievementStat(EAchievementStats Stat, float Value);
	
	// set arbitrary int - stat
	void IncreaseAchievementStat(EAchievementStats Stat, float Amount);

	// call this to unlock a simple achievement, like ACH_FIRST_ARREST
	void UnlockAchievement(EAchievement Achievement);

	// called to check if any new achievements should be unlocked, called automatically when a stat is updated
	void CheckForAnyNewUnlockedAchievements(EAchievementStats Stat, float Value);
	// called to check if any new level score achievements should be unlocked, called in EndMission
	void CheckForAnyNewLevelScoreAchievements();

	UFUNCTION()
	void UsedItem(ABaseItem* Item);

	UFUNCTION()
	void BeginMission();

	UFUNCTION()
	void EndMission(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable)
	void DrankBeverage(FString Beverage);
};

UCLASS()
class READYORNOT_API UAchievementStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

private:
	static UAchievementSubsystem* GetAchievementSubsystem(UObject* WorldContextObject);
	
public:
	static void SetAchievementStat(UObject* WorldContextObject, EAchievementStats Stat, float Value);
	static void IncreaseAchievementStat(UObject* WorldContextObject, EAchievementStats Stat, float Amount);
	static void UnlockAchievement(UObject* WorldContextObject, EAchievement Achievement, bool SupportMultiplayer = true);
	static void BeginMission(UObject* WorldContextObject);
	static void EndMission(UObject* WorldContextObject);
	static void UsedItem(UObject* WorldContextObject, ABaseItem* Item);
	static void RetrySteamStats(UObject* WorldContextObject);
};

#define ACHIEVEMENTS_SLOT_NAME "Achievements"

UCLASS()
class READYORNOT_API ULocalAchievements : public USaveGame
{
	GENERATED_BODY()
	
public:
	static ULocalAchievements* Load();
	TMap<EAchievementStats, FUniversalStat> GetStats();
	TArray<EAchievement> GetAchievements();
	void UnlockAchievement(EAchievement Achievement);
	void SetAchievementStat(const EAchievementStats Stat, const EUniversalStatType StatType, const float FloatValue);
	TArray<FName> GetUnlockedTraits();
	void SetUnlockedTraits(TArray<FName> Names);

private:
	UPROPERTY()
	TMap<EAchievementStats, FUniversalStat> Stats;
	UPROPERTY()
	TArray<EAchievement> Achievements;
	UPROPERTY()
	TArray<FName> UnlockedTraits;

	void Save();
};