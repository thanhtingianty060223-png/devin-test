// Copyright Void Interactive, 2023


#include "AchievementSubsystem.h"

#include "Commander/BaseProfile.h"
#include "Commander/CampaignData.h"
#include "Commander/CommanderProfile.h"
#include "Commander/MetaGameProfile.h"
#include "Commander/RosterManager.h"
#include "Engine/Engine.h"
#include "Info/LoadoutManager.h"
#include "Info/ScoringManager.h"
#include "Info/SWATManager.h"
#if defined(WITH_STEAM)
#include "SteamRequests.h"
#include "GetStoredStats.h"
#include "SetStoredStats.h"
#include "SetAchievementData.h"
#include "GetAchievementData.h"
#include "StoreStatsAndAchievements.h"
#include "IndicateAchievementProgress.h"
#endif
#if defined(TARGET_CONSOLE)
#include "Interfaces/OnlineAchievementsInterface.h"
#include "Interfaces/OnlineIdentityInterface.h"
#endif

DEFINE_LOG_CATEGORY(LogReadyOrNotAchievements);

// Naming of stats on Steam, needed to translate to coded names on steam.
const TMap<EAchievementStats, FString> UAchievementSubsystem::SteamStatNames = {
	{ EAchievementStats::SCORE_GAS, "SCORE_LEVEL_1" },
	{ EAchievementStats::SCORE_COYOTE, "SCORE_LEVEL_2" },
	{ EAchievementStats::SCORE_METH, "SCORE_LEVEL_3" },
	{ EAchievementStats::SCORE_CAMPUS, "SCORE_LEVEL_4" },
	{ EAchievementStats::SCORE_HOSPITAL, "SCORE_LEVEL_5" },
	{ EAchievementStats::SCORE_CLUB, "SCORE_LEVEL_6" },
	{ EAchievementStats::SCORE_FARM, "SCORE_LEVEL_7" },
	{ EAchievementStats::SCORE_SINS, "SCORE_LEVEL_8" },
	{ EAchievementStats::SCORE_PENTHOUSE, "SCORE_LEVEL_9" },
	{ EAchievementStats::SCORE_RIDGELINE, "SCORE_LEVEL_10" },
	{ EAchievementStats::SCORE_DEALER, "SCORE_LEVEL_11" },
	{ EAchievementStats::SCORE_PORT, "SCORE_LEVEL_12" },
	{ EAchievementStats::SCORE_BEACHFRONT, "SCORE_LEVEL_13" },
	{ EAchievementStats::SCORE_IMPORTER, "SCORE_LEVEL_14" },
	{ EAchievementStats::SCORE_AGENCY, "SCORE_LEVEL_15" },
	{ EAchievementStats::SCORE_VALLEY, "SCORE_LEVEL_16" },
	{ EAchievementStats::SCORE_DATACENTER, "SCORE_LEVEL_17" },
	{ EAchievementStats::SCORE_STREAMER, "SCORE_LEVEL_18" },
	{ EAchievementStats::PROGRESS_A1, "PROGRESS_A1" },
	{ EAchievementStats::PROGRESS_A2, "PROGRESS_A2" },
	{ EAchievementStats::PROGRESS_A3, "PROGRESS_A3" },
	{ EAchievementStats::PROGRESS_A4, "PROGRESS_A4" },
	{ EAchievementStats::PROGRESS_A5, "PROGRESS_A5" },
	{ EAchievementStats::PROGRESS_A6, "PROGRESS_A6" },
	{ EAchievementStats::PROGRESS_BREACH, "PROGRESS_BREACH" },
	{ EAchievementStats::PROGRESS_ARREST, "PROGRESS_ARREST" },
	{ EAchievementStats::PROGRESS_BATTERING_RAM, "PROGRESS_BATTERING_RAM" },
	{ EAchievementStats::PROGRESS_MIRRORGUN, "PROGRESS_MIRRORGUN" },
	{ EAchievementStats::PROGRESS_M320, "PROGRESS_M320" },
	{ EAchievementStats::PROGRESS_LOCKPICK, "PROGRESS_LOCKPICK" },

};

const TMap<EAchievementStats, EUniversalStatType> UAchievementSubsystem::StatTypes = {
	{ EAchievementStats::SCORE_GAS, EUniversalStatType::STAT_FLOAT },
	{ EAchievementStats::SCORE_COYOTE, EUniversalStatType::STAT_FLOAT },
	{ EAchievementStats::SCORE_METH, EUniversalStatType::STAT_FLOAT },
	{ EAchievementStats::SCORE_CAMPUS, EUniversalStatType::STAT_FLOAT },
	{ EAchievementStats::SCORE_HOSPITAL, EUniversalStatType::STAT_FLOAT },
	{ EAchievementStats::SCORE_CLUB, EUniversalStatType::STAT_FLOAT },
	{ EAchievementStats::SCORE_FARM, EUniversalStatType::STAT_FLOAT },
	{ EAchievementStats::SCORE_SINS, EUniversalStatType::STAT_FLOAT },
	{ EAchievementStats::SCORE_PENTHOUSE, EUniversalStatType::STAT_FLOAT },
	{ EAchievementStats::SCORE_RIDGELINE, EUniversalStatType::STAT_FLOAT },
	{ EAchievementStats::SCORE_DEALER, EUniversalStatType::STAT_FLOAT },
	{ EAchievementStats::SCORE_PORT, EUniversalStatType::STAT_FLOAT },
	{ EAchievementStats::SCORE_BEACHFRONT, EUniversalStatType::STAT_FLOAT },
	{ EAchievementStats::SCORE_IMPORTER, EUniversalStatType::STAT_FLOAT },
	{ EAchievementStats::SCORE_AGENCY, EUniversalStatType::STAT_FLOAT },
	{ EAchievementStats::SCORE_VALLEY, EUniversalStatType::STAT_FLOAT },
	{ EAchievementStats::SCORE_DATACENTER, EUniversalStatType::STAT_FLOAT },
	{ EAchievementStats::SCORE_STREAMER, EUniversalStatType::STAT_FLOAT },
	{ EAchievementStats::PROGRESS_A1, EUniversalStatType::STAT_INT },
	{ EAchievementStats::PROGRESS_A2, EUniversalStatType::STAT_INT },
	{ EAchievementStats::PROGRESS_A3, EUniversalStatType::STAT_INT },
	{ EAchievementStats::PROGRESS_A4, EUniversalStatType::STAT_INT },
	{ EAchievementStats::PROGRESS_A5,  EUniversalStatType::STAT_INT },
	{ EAchievementStats::PROGRESS_A6,  EUniversalStatType::STAT_INT },
	{ EAchievementStats::PROGRESS_BREACH, EUniversalStatType::STAT_INT },
	{ EAchievementStats::PROGRESS_ARREST, EUniversalStatType::STAT_INT },
	{ EAchievementStats::PROGRESS_BATTERING_RAM, EUniversalStatType::STAT_INT },
	{ EAchievementStats::PROGRESS_MIRRORGUN, EUniversalStatType::STAT_INT },
	{ EAchievementStats::PROGRESS_M320, EUniversalStatType::STAT_INT },
	{ EAchievementStats::PROGRESS_LOCKPICK, EUniversalStatType::STAT_INT },
};


// Naming of achievements on Steam, needed to translate to coded names on steam.
const TMap<EAchievement, FString> UAchievementSubsystem::SteamAchievementNames = {
	{ EAchievement::THE_WAR, "ACHIEVEMENT_A1" },
	{ EAchievement::THE_DECAYING_CITY, "ACHIEVEMENT_A2" },
	{ EAchievement::THE_LEFT_BEHIND, "ACHIEVEMENT_A3" },
	{ EAchievement::THE_ABDUCTED, "ACHIEVEMENT_A4" },
	{ EAchievement::THE_EXPLOITED, "ACHIEVEMENT_A5" },
	{ EAchievement::MEDAL_OF_VALOR, "ACHIEVEMENT_A6" },
	{ EAchievement::THE_WORLD, "ACHIEVEMENT_A7" },
	{ EAchievement::THE_HERMIT, "ACHIEVEMENT_A8" },
	{ EAchievement::THE_HANGED_MAN, "ACHIEVEMENT_A9" },
	{ EAchievement::WAY_OUT_WEST, "ACHIEVEMENT_A10" },
	{ EAchievement::THE_DEVIL, "ACHIEVEMENT_A11" },
	{ EAchievement::THE_MAGICIAN, "ACHIEVEMENT_A12" },
	{ EAchievement::THE_FOOL, "ACHIEVEMENT_A13" },
	{ EAchievement::FIRST_ARREST, "ACH_FIRST_ARREST" },
	{ EAchievement::DUE_PROCESS, "ACHIEVEMENT_A14" }
};

// TODO: add console names
// const TMap<EAchievementStats, FString> UAchievementSubsystem::XboxStatNames

// Translate levelnames to achievementstat, for keeping score
const TMap<FString, EAchievementStats> UAchievementSubsystem::LevelToEAchievementStat = {
	{ GAS_MISSION_NAME, EAchievementStats::SCORE_GAS},
	{ COYOTE_MISSION_NAME, EAchievementStats::SCORE_COYOTE},
	{ METH_MISSION_NAME, EAchievementStats::SCORE_METH},
	{ CAMPUS_MISSION_NAME, EAchievementStats::SCORE_CAMPUS},
	{ HOSPITAL_MISSION_NAME, EAchievementStats::SCORE_HOSPITAL},
	{ CLUB_MISSION_NAME, EAchievementStats::SCORE_CLUB},
	{ FARM_MISSION_NAME, EAchievementStats::SCORE_FARM},
	{ SINS_MISSION_NAME, EAchievementStats::SCORE_SINS},
	{ PENTHOUSE_MISSION_NAME, EAchievementStats::SCORE_PENTHOUSE},
	{ RIDGELINE_MISSION_NAME, EAchievementStats::SCORE_RIDGELINE},
	{ DEALER_MISSION_NAME, EAchievementStats::SCORE_DEALER},
	{ PORT_MISSION_NAME, EAchievementStats::SCORE_PORT},
	{ BEACHFRONT_MISSION_NAME, EAchievementStats::SCORE_BEACHFRONT},
	{ IMPORTER_MISSION_NAME, EAchievementStats::SCORE_IMPORTER},
	{ AGENCY_MISSION_NAME, EAchievementStats::SCORE_AGENCY},
	{ VALLEY_MISSION_NAME, EAchievementStats::SCORE_VALLEY},
	{ DATACENTER_MISSION_NAME, EAchievementStats::SCORE_DATACENTER},
	{ STREAMER_MISSION_NAME, EAchievementStats::SCORE_STREAMER}
};

void UAchievementSubsystem::CheckCustomizationAchievement(const TArray<FName>& TagRequirements, EAchievement Achievement)
{
	if(TagRequirements.Num() == 0 || UnlockedAchievements.Contains(Achievement))
		return;

	UMetaGameProfile* MetaGameProfile = UMetaGameProfile::GetProfile(GetWorld());
	if(!MetaGameProfile) 
		return;

	for (const FName Requirement : TagRequirements)
	{
		if (!MetaGameProfile->GetProgressionTags().Contains(Requirement))
		{
			return;
		}
	}

	UnlockAchievement(Achievement);
}

void UAchievementSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	for (EAchievementStats StatId : TEnumRange<EAchievementStats>())
	{
		FUniversalStat Stat;
		
		Stat.StatType = StatTypes[StatId]; 
		AchievementStats.Add(StatId, Stat);
	}
	
	LocalAchievements = ULocalAchievements::Load();
	
	// StoreStatsAndAchievements = UStoreStatsAndAchievements::StoreUserStatsAndAchievements();
	
	// StoreStatsAndAchievements->onSuccess.AddUniqueDynamic(this, &UAchievementSubsystem::StoreStatsAndAchievementsSuccess);
	// StoreStatsAndAchievements->onFailure.AddUniqueDynamic(this, &UAchievementSubsystem::StoreStatsAndAchievementsFailure);
}

void UAchievementSubsystem::LateInitialize()
{
	if (bInitialized) return;

	// SteamUserStats()->RequestUserStats, is already run in OnlineLeaderboardInterfaceSteam.cpp
	// QueryAchievementsAndStats();
	// Jump to reading the stats.
	QueryAchievementsAndStatsSuccess(0);
}

void UAchievementSubsystem::Tick(float DeltaTime)
{
	if (!bInitialized)
		return;
	if ( LastFrameNumberWeTicked+300 > GFrameCounter ) // cooldown so we don't get rate limited by steam
		return;
	LastFrameNumberWeTicked = GFrameCounter;

	if (bAchievementsOrStatsDirty)
	{

		UE_LOG(LogReadyOrNotAchievements, Log, TEXT("Tick AchievementSubsystem - StoreUserStatsAndAchievements"));
		// store local cache to steam
#if defined(WITH_STEAM)
		// steam generates 1 callback for each stat/achievement, we don't know how many so we assume that it will finish within timeout period
		UStoreStatsAndAchievements* StoreStatsAndAchievements = UStoreStatsAndAchievements::StoreUserStatsAndAchievements();

		StoreStatsAndAchievements->Activate();
		bAchievementsOrStatsDirty = false;
#endif
	}
}

void UAchievementSubsystem::UpdateLevelScore(FString Level, float PercentageScore)
{
	if (LevelToEAchievementStat.Contains(Level))
	{
		const EAchievementStats Stat = LevelToEAchievementStat[Level];

		if (AchievementStats.Contains(Stat)) 
		{
			if (PercentageScore > AchievementStats[Stat].FloatValue)
			{
				SetAchievementStat(Stat, PercentageScore);
				bAchievementsOrStatsDirty = true;
			}
		} 
	}
}

void UAchievementSubsystem::UnlockAchievement(EAchievement Achievement)
{
	if (!bInitialized) return;
	
	if (UnlockedAchievements.Contains(Achievement))
		return;
#if defined(WITH_STEAM)
	UE_LOG(LogReadyOrNotAchievements, Log, TEXT("Unlocking achievement %s"), *UEnum::GetValueAsString(Achievement));
	if (SteamAchievementNames.Contains(Achievement))
	{
 		USetAchievementData::SetAchievement(SteamAchievementNames[Achievement]);
	} 
	else
	{
		UE_LOG(LogReadyOrNotAchievements, Error, TEXT("Missing SteamAchievementName %s"), *UEnum::GetValueAsString(Achievement));
	}
	UnlockedAchievements.Add(Achievement);
	LocalAchievements->UnlockAchievement(Achievement);

	// push to server
	bAchievementsOrStatsDirty = true;

#elif defined(TARGET_CONSOLE)

	FOnlineSubsystemBPCallHelperAdvanced Helper(TEXT("AchievementSubsystem"), GetWorld());

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	Helper.QueryIDFromPlayerController(PlayerController);

	if (!Helper.IsValid())
	{
		UE_LOG(LogReadyOrNotAchievements, Error, TEXT("Could not get Online subsystem Helper."));
		return;
	}

	IOnlineSubsystem* OnlineSub = Helper.OnlineSub;
	if (!OnlineSub)
	{
		UE_LOG(LogReadyOrNotAchievements, Error, TEXT("Could not get Online subsystem Helper."));
		return;
	}

	if (!Helper.UserID.IsValid())
	{
		UE_LOG(LogReadyOrNotAchievements, Error, TEXT("Could not get Online Identity UserID."));
		return;
	}

	IOnlineIdentityPtr Identity = OnlineSub->GetIdentityInterface();
	if (!Identity.IsValid())
	{
		UE_LOG(LogReadyOrNotAchievements, Error, TEXT("Could not get Identity Interface."));
		return;
	}

	IOnlineAchievementsPtr Achievements = OnlineSub->GetAchievementsInterface();
	if (Achievements.IsValid())
	{
		FOnlineAchievementsWritePtr WriteObject = MakeShareable(new FOnlineAchievementsWrite());
#if defined(TARGET_XBOX)
		WriteObject->SetFloatStat(*UBpGameplayHelperLib::EnumToString("EAchievement", Achievement), 100);
#else
		WriteObject->SetFloatStat(*UBpGameplayHelperLib::EnumToString("EAchievement", Achievement), 100);
#endif

		FOnlineAchievementsWriteRef WriteObjectRef = WriteObject.ToSharedRef();
		Achievements->WriteAchievements(*Helper.UserID, WriteObjectRef);
		UnlockedAchievements.Add(Achievement);
	}
	else
	{
		UE_LOG(LogOnline, Warning, TEXT("No valid achievement interface or another write is in progress."));
	}

#else
	UE_LOG(LogReadyOrNotAchievements, Error, TEXT("Platform does not support UnlockAchievement"));
#endif
}

float UAchievementSubsystem::GetLevelScoreStat(EAchievementStats Level)
{
	if (AchievementStats.Contains(Level))
	{
		return AchievementStats[Level].FloatValue;
	}
	return 0;
}

int UAchievementSubsystem::GetProgressStat(EAchievementStats Level)
{
	if (AchievementStats.Contains(Level))
	{
		return AchievementStats[Level].IntegerValue;
	}
	return 0;
}


void UAchievementSubsystem::UpdateStat(EAchievementStats Stat, float Value)
{
	FUniversalStat UniversalStat = AchievementStats[Stat];
#if defined(WITH_STEAM)	
	ESteamStatType SteamStatType;

	if (UniversalStat.StatType == EUniversalStatType::STAT_INT)
	{
		SteamStatType = ESteamStatType::STAT_INT;
	}
	else
	{
		SteamStatType = ESteamStatType::STAT_FLOAT;
	}

	USetStoredStats::SetStoredStat(SteamStatNames[Stat], SteamStatType, UniversalStat.FloatValue, UniversalStat.IntegerValue, 0);
#endif
	bAchievementsOrStatsDirty = true;
	CheckForAnyNewUnlockedAchievements(Stat, Value);
}

void UAchievementSubsystem::UpdateIronmanAchievements(FString SafeMissionName)
{
	if(!UnlockedAchievements.Contains(EAchievement::THE_HERMIT))
	{
		UBaseProfile* BaseProfile = UBaseProfile::GetCurrentProfile();
		if (!BaseProfile)
			return;
		
		UCommanderProfile* CommanderProfile = Cast<UCommanderProfile>(BaseProfile);
		if (!CommanderProfile)
			return;
		
		// TODO: This assumes that a team always start with 4 officers which might cause future issues
		if(const USWATManager* Manager = USWATManager::Get(this))
		{
			const TArray<ASWATCharacter*> team = Manager->SwatAI;
			CommanderProfile->LostOfficers += 4 - team.Num();
		}
		
		CommanderProfile->SaveProfile();

		if(SafeMissionName == PORT_MISSION_NAME)
		{
			UAchievementStatics::UnlockAchievement(GetWorld(),EAchievement::THE_WORLD, false);
	
			if(CommanderProfile->LostOfficers == 0)
			{
				UAchievementStatics::UnlockAchievement(GetWorld(),EAchievement::THE_HERMIT, false);
			}
		}
	}
}

void UAchievementSubsystem::SetAchievementStat(EAchievementStats Stat, float Value)
{
	if (!bInitialized) return;

	UE_LOG(LogReadyOrNotAchievements, Log, TEXT("SetAchievementStat %s %f %d"), *UEnum::GetValueAsString(Stat), Value);
	if (AchievementStats.Contains(Stat))
	{
		if (AchievementStats[Stat].StatType == EUniversalStatType::STAT_FLOAT) {
			AchievementStats[Stat].FloatValue = Value;
		}
		else if (AchievementStats[Stat].StatType == EUniversalStatType::STAT_INT) {
			AchievementStats[Stat].IntegerValue = Value;
		}
		LocalAchievements->SetAchievementStat(Stat, AchievementStats[Stat].StatType, Value);

		UpdateStat(Stat, Value);
	}
	else
	{
		UE_LOG(LogReadyOrNotAchievements, Log, TEXT("SetAchievementStat %s not found"), *UEnum::GetValueAsString(Stat));
	}
}

void UAchievementSubsystem::IncreaseAchievementStat(EAchievementStats Stat, float Amount)
{
	if (!bInitialized) return;
	
	float total = 0;
	if (AchievementStats.Contains(Stat))
	{
		if (AchievementStats[Stat].StatType == EUniversalStatType::STAT_FLOAT) {
			AchievementStats[Stat].FloatValue += Amount;
			total = AchievementStats[Stat].FloatValue;
		}
		else if (AchievementStats[Stat].StatType == EUniversalStatType::STAT_INT) {
			AchievementStats[Stat].IntegerValue += Amount;
			total = AchievementStats[Stat].IntegerValue;
		}

		UpdateStat(Stat, total);
	}
}

void UAchievementSubsystem::StoreStatsAndAchievementsSuccess(int32 SteamErrorOutput)
{
	// called once for stats and once for each achievement
	UE_LOG(LogReadyOrNotAchievements, Log, TEXT("Successfully stored stats"));
}

void UAchievementSubsystem::StoreStatsAndAchievementsFailure(int32 SteamErrorOutput)
{
	UE_LOG(LogReadyOrNotAchievements, Error, TEXT("Storing stats failed %d"), SteamErrorOutput);
}

void UAchievementSubsystem::IndicateUserAchievementProgress(EAchievement Achievement, int Progress, int MaxProgress)
{
	if (!bInitialized) return;

#if defined(WITH_STEAM)	
	if (SteamAchievementNames.Contains(Achievement) && Progress != MaxProgress) 
	{
		UIndicateAchievementProgress* UserAchievementProgress = UIndicateAchievementProgress::IndicateUserAchievementProgress(SteamAchievementNames[Achievement], Progress, MaxProgress);

		// UserAchievementProgress->onSuccess.AddUniqueDynamic(this, &UAchievementSubsystem::UserAchievementProgressSuccess);
		// UserAchievementProgress->onFailure.AddUniqueDynamic(this, &UAchievementSubsystem::UserAchievementProgressFailure);

		UserAchievementProgress->Activate();
	}
	else 
	{
		UnlockAchievement(Achievement);
	}
#elif defined(TARGET_CONSOLE)
	FOnlineSubsystemBPCallHelperAdvanced Helper(TEXT("AchievementSubsystem"), GetWorld());

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	Helper.QueryIDFromPlayerController(PlayerController);

	if (!Helper.IsValid())
	{
		UE_LOG(LogReadyOrNotAchievements, Error, TEXT("Could not get Online subsystem Helper."));
		return;
	}

	IOnlineSubsystem* OnlineSub = Helper.OnlineSub;
	if (!OnlineSub)
	{
		UE_LOG(LogReadyOrNotAchievements, Error, TEXT("Could not get Online subsystem Helper."));
		return;
	}

	if (!Helper.UserID.IsValid())
	{
		UE_LOG(LogReadyOrNotAchievements, Error, TEXT("Could not get Online Identity UserID."));
		return;
	}

	IOnlineIdentityPtr Identity = OnlineSub->GetIdentityInterface();
	if (!Identity.IsValid())
	{
		UE_LOG(LogReadyOrNotAchievements, Error, TEXT("Could not get Identity Interface."));
		return;
	}

	IOnlineAchievementsPtr Achievements = OnlineSub->GetAchievementsInterface();
	if (Achievements.IsValid())
	{
		FOnlineAchievementsWritePtr WriteObject = MakeShareable(new FOnlineAchievementsWrite());
#if defined(TARGET_XBOX)
		WriteObject->SetFloatStat(*UBpGameplayHelperLib::EnumToString("EAchievement", Achievement), 100 * Progress / MaxProgress);
#else
		WriteObject->SetFloatStat(*UBpGameplayHelperLib::EnumToString("EAchievement", Achievement), Progress);
#endif

		FOnlineAchievementsWriteRef WriteObjectRef = WriteObject.ToSharedRef();
		Achievements->WriteAchievements(*Helper.UserID, WriteObjectRef);
		UnlockedAchievements.Add(Achievement);
	}
	else
	{
		UE_LOG(LogOnline, Warning, TEXT("No valid achievement interface or another write is in progress."));
	}

#else
	UE_LOG(LogReadyOrNotAchievements, Error, TEXT("Platform does not support UnlockAchievement"));
#endif
}

void UAchievementSubsystem::UserAchievementProgressSuccess(int32 SteamErrorOutput)
{
	UE_LOG(LogReadyOrNotAchievements, Log, TEXT("IndicateUserAchievementProgressSuccess"));
}

void UAchievementSubsystem::UserAchievementProgressFailure(int32 SteamErrorOutput)
{
	UE_LOG(LogReadyOrNotAchievements, Error, TEXT("IndicateUserAchievementProgressFailure %d"), SteamErrorOutput);
}

// check if any new progression achievements have been unlocked
void UAchievementSubsystem::CheckForAnyNewLevelScoreAchievements()
{
	if (!bInitialized) return;

	if (!UnlockedAchievements.Contains(EAchievement::THE_WAR))
	{
		int numCompleted = int(GetLevelScoreStat(EAchievementStats::SCORE_GAS) > SCORE_REQUIRED_C) + int(GetLevelScoreStat(EAchievementStats::SCORE_COYOTE) > SCORE_REQUIRED_C) + int(GetLevelScoreStat(EAchievementStats::SCORE_METH) > SCORE_REQUIRED_C);

		if (numCompleted > GetProgressStat(EAchievementStats::PROGRESS_A1)) {
			bAchievementsOrStatsDirty = true;
			SetAchievementStat(EAchievementStats::PROGRESS_A1, numCompleted);
			IndicateUserAchievementProgress(EAchievement::THE_WAR, numCompleted, 3);
		}
	}

	if (!UnlockedAchievements.Contains(EAchievement::THE_DECAYING_CITY))
	{
		int numCompleted = int(GetLevelScoreStat(EAchievementStats::SCORE_CAMPUS) > SCORE_REQUIRED_C) + int(GetLevelScoreStat(EAchievementStats::SCORE_HOSPITAL) > SCORE_REQUIRED_C) + int(GetLevelScoreStat(EAchievementStats::SCORE_CLUB) > SCORE_REQUIRED_C);
		if (numCompleted > GetProgressStat(EAchievementStats::PROGRESS_A2)) {
			bAchievementsOrStatsDirty = true;
			SetAchievementStat(EAchievementStats::PROGRESS_A2, numCompleted);
			IndicateUserAchievementProgress(EAchievement::THE_DECAYING_CITY, numCompleted, 3);
		}
	}

	if (!UnlockedAchievements.Contains(EAchievement::THE_LEFT_BEHIND))
	{
		int numCompleted = int(GetLevelScoreStat(EAchievementStats::SCORE_FARM) > SCORE_REQUIRED_C) + int(GetLevelScoreStat(EAchievementStats::SCORE_SINS) > SCORE_REQUIRED_C) + int(GetLevelScoreStat(EAchievementStats::SCORE_PENTHOUSE) > SCORE_REQUIRED_C) + int(GetLevelScoreStat(EAchievementStats::SCORE_RIDGELINE) > SCORE_REQUIRED_C);
		if (numCompleted > GetProgressStat(EAchievementStats::PROGRESS_A3)) {
			bAchievementsOrStatsDirty = true;
			SetAchievementStat(EAchievementStats::PROGRESS_A3, numCompleted);
			IndicateUserAchievementProgress(EAchievement::THE_LEFT_BEHIND, numCompleted, 4);
		}
	}

	if (!UnlockedAchievements.Contains(EAchievement::THE_ABDUCTED))
	{
		int numCompleted = int(GetLevelScoreStat(EAchievementStats::SCORE_DEALER) > SCORE_REQUIRED_C) + int(GetLevelScoreStat(EAchievementStats::SCORE_PORT) > SCORE_REQUIRED_C) + int(GetLevelScoreStat(EAchievementStats::SCORE_BEACHFRONT) > SCORE_REQUIRED_C) + int(GetLevelScoreStat(EAchievementStats::SCORE_IMPORTER) > SCORE_REQUIRED_C);
		if (numCompleted > GetProgressStat(EAchievementStats::PROGRESS_A4)) {
			bAchievementsOrStatsDirty = true;
			SetAchievementStat(EAchievementStats::PROGRESS_A4, numCompleted);
			IndicateUserAchievementProgress(EAchievement::THE_ABDUCTED, numCompleted, 4);
		}
	}

	if (!UnlockedAchievements.Contains(EAchievement::THE_EXPLOITED))
	{
		int numCompleted = int(GetLevelScoreStat(EAchievementStats::SCORE_AGENCY) > SCORE_REQUIRED_C) + int(GetLevelScoreStat(EAchievementStats::SCORE_VALLEY) > SCORE_REQUIRED_C) + int(GetLevelScoreStat(EAchievementStats::SCORE_DATACENTER) > SCORE_REQUIRED_C) + int(GetLevelScoreStat(EAchievementStats::SCORE_STREAMER) > SCORE_REQUIRED_C);
		if (numCompleted > GetProgressStat(EAchievementStats::PROGRESS_A5)) {
			bAchievementsOrStatsDirty = true;
			SetAchievementStat(EAchievementStats::PROGRESS_A5, numCompleted);
			IndicateUserAchievementProgress(EAchievement::THE_EXPLOITED, numCompleted, 4);
		}
	}

	if (!UnlockedAchievements.Contains(EAchievement::MEDAL_OF_VALOR))
	{
		int numCompleted = 0;
		for (EAchievementStats StatId : TEnumRange<EAchievementStats>())
		{
			if (StatId > EAchievementStats::SCORE_STREAMER) // Last level stat
				break;
			if (GetLevelScoreStat(StatId) >= SCORE_REQUIRED_S)
			{
				numCompleted++;
			}
		}
		if (numCompleted > GetProgressStat(EAchievementStats::PROGRESS_A6)) {
			bAchievementsOrStatsDirty = true;
			SetAchievementStat(EAchievementStats::PROGRESS_A6, numCompleted);
			IndicateUserAchievementProgress(EAchievement::MEDAL_OF_VALOR, numCompleted, 18);
		}
	}
}

void UAchievementSubsystem::CheckForAnyNewUnlockedAchievements(EAchievementStats Stat, float Value)
{
	switch (Stat)
	{
	case EAchievementStats::PROGRESS_BREACH:
		IndicateUserAchievementProgress(EAchievement::PINE_PAINBRINGER, Value, 20);
		IndicateUserAchievementProgress(EAchievement::WALNUT_WARRIOR, Value, 50);
		IndicateUserAchievementProgress(EAchievement::MAHOGANY_MASOCHIST, Value, 100);
		break;
	case EAchievementStats::PROGRESS_ARREST:
		IndicateUserAchievementProgress(EAchievement::ARREST_WARRANT, Value, 50);
		IndicateUserAchievementProgress(EAchievement::POWERS_OF_ARREST, Value, 100);
		IndicateUserAchievementProgress(EAchievement::REST_FOR_THE_WICKED, Value, 200);
		break;
	case EAchievementStats::PROGRESS_BATTERING_RAM:
		IndicateUserAchievementProgress(EAchievement::HERES_JOHNNY, Value, 20);
		break;
	case EAchievementStats::PROGRESS_MIRRORGUN:
		IndicateUserAchievementProgress(EAchievement::PEEPING_TOM, Value, 20);
		break;
	case EAchievementStats::PROGRESS_M320:
		IndicateUserAchievementProgress(EAchievement::SAY_HELLO, Value, 20);
		break;
	case EAchievementStats::PROGRESS_LOCKPICK:
		IndicateUserAchievementProgress(EAchievement::CLICK_FROM, Value, 20);
		break;
	}
}

/* Low level - Stats handling code */

// querystats for respective platform, execute waiting code when done
// steam initializes stats and achievements separately
void UAchievementSubsystem::QueryStatsConsole() 
{
	FOnlineSubsystemBPCallHelperAdvanced Helper(TEXT("OnlineActivityManager"), GetWorld());
 	
 	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
 	Helper.QueryIDFromPlayerController(PlayerController);
 	
 	if (!Helper.IsValid())
 	{
 		UE_LOG(LogReadyOrNotAchievements, Error, TEXT("Could not get Online subsystem Helper."));
 		//RunWhenInitialized.Execute(false);
 		return;
 	}
		
 	IOnlineSubsystem* OnlineSub = Helper.OnlineSub;
 	if (!OnlineSub)
 	{
 		UE_LOG(LogReadyOrNotAchievements, Error, TEXT("Could not get Online subsystem Helper."));
 		//RunWhenInitialized.Execute(false);
 		return;
 	}

 	if (!Helper.UserID.IsValid())
 	{
 		UE_LOG(LogReadyOrNotAchievements, Error, TEXT("Could not get Online Identity UserID."));
 		//RunWhenInitialized.Execute(false);
 		return;
 	}

 	IOnlineStatsPtr Stats = OnlineSub->GetStatsInterface();
 	if (!Stats.IsValid())
 	{
 		UE_LOG(LogReadyOrNotAchievements, Error, TEXT("Could not get Stats Interface."));
 		//RunWhenInitialized.Execute(false);
 		return;
 	}
 	else
 	{
 		//FUniqueNetIdRef UserID; // = *Helper.UserID;
 		Stats->QueryStats(Helper.UserID.ToSharedRef(),Helper.UserID.ToSharedRef(), FOnlineStatsQueryUserStatsComplete::CreateLambda([this /*,*RunWhenInitialized*/](const FOnlineError& Error,  const TSharedPtr<const FOnlineStatsUserStats>& QueriedStats)
 		{
 			UE_LOG(LogReadyOrNotAchievements, Warning, TEXT("Read stats %d queried stats"), QueriedStats->Stats.Num());
 			if (!Error.bSucceeded)
 			{
 				UE_LOG(LogReadyOrNotAchievements, Error, TEXT("ReadStats test failed: %s"), *Error.GetErrorMessage().ToString());
 				//RunWhenInitialized.ExecuteIfBound(false);
 			}
 			else
 			{
 				for (const TPair<FString, FOnlineStatValue>& StatReads : QueriedStats->Stats)
 				{
 					UE_LOG(LogReadyOrNotAchievements, Warning, TEXT("Stat name %s has value %s"), *StatReads.Key, *StatReads.Value.ToString());
 				}
 		
 				// TSharedPtr<const FOnlineStatsUserStats> ReadUserStats = Stats->GetStats(*Helper.UserId.ToSharedRef());
 				// for (const TPair<FString, FOnlineStatValue>& StatReads : ReadUserStats->Stats)
 				// {
 				// 	UE_LOG(LogReadyOrNotAchievements, Warning, TEXT("Read Stat name %s has value %s"), *StatReads.Key, *StatReads.Value.ToString());
 				// }
 				//bAchievementsAndStatsInitalized = true;
 				//RunWhenInitialized.ExecuteIfBound(true);
 			}
 		}));
 	}
}

bool UAchievementSubsystem::GetAchievementData(EAchievement Achievement, bool& AchievementUnlocked)
{
#if defined(WITH_STEAM)
	if (SteamAchievementNames.Contains(Achievement))
	{
		FDateTime UnlockTime;
		UGetAchievementData::GetAchievementUnlockStatusAndUnlockTime(SteamAchievementNames[Achievement], AchievementUnlocked, UnlockTime);
		if (UnlockTime == FDateTime::MinValue())
		{
			return false;
		}
		return true;
	}
#endif
	return false;
}

bool UAchievementSubsystem::GetUserStoredStats(TArray<FUniversalStat>& StatsOut)
{
#if defined(WITH_STEAM)
	TArray<FSteamStat> StatsToGet;
	TArray<FSteamStat> SteamStatsOut;

	for (EAchievementStats StatId : TEnumRange<EAchievementStats>())
	{
		FSteamStat Stat;
		if (StatTypes[StatId] == EUniversalStatType::STAT_FLOAT) {
			Stat.StatType = ESteamStatType::STAT_FLOAT;
		}
		else if (StatTypes[StatId] == EUniversalStatType::STAT_INT) {
			Stat.StatType = ESteamStatType::STAT_INT;
		}
		Stat.APIStatName = SteamStatNames[StatId];
		StatsToGet.Add(Stat);
	}

//#if !WITH_EDITOR
	UGetStoredStats::GetUserStoredStats(StatsToGet, SteamStatsOut);
	if (StatsToGet.Num() == SteamStatsOut.Num()) {
		for (EAchievementStats StatId : TEnumRange<EAchievementStats>()) {
			FUniversalStat stat = FUniversalStat(SteamStatsOut[(int8)StatId]);
			StatsOut.Add(stat);
		}
		return true;
	}
//#endif

#endif
	return false;
}

FUniversalStat UAchievementSubsystem::GetBestStat(FUniversalStat FirstStat, FUniversalStat SecondStat, bool& dirty)
{
	dirty = false;
	if (FirstStat.StatType == EUniversalStatType::STAT_FLOAT && SecondStat.StatType == EUniversalStatType::STAT_FLOAT)
	{
		if (FirstStat.FloatValue >= SecondStat.FloatValue) 
		{
			return FirstStat;
		}
		else
		{
			dirty = true;
			return SecondStat;
		}
	}
	else if (FirstStat.StatType == EUniversalStatType::STAT_INT)
	{
		if (FirstStat.IntegerValue >= SecondStat.IntegerValue)
		{
			return FirstStat;
		}
		else
		{
			dirty = true;
			return SecondStat;
		}
	}
	return FirstStat;
}

void UAchievementSubsystem::UpdateAchievedTraits()
{
	if (UBpGameplayHelperLib::IsMultiplayer(GetWorld()))
		return;

	TArray<FName> UnlockedTraits = LocalAchievements->GetUnlockedTraits();
	int numUnlockedTraitsBefore = UnlockedTraits.Num();
	
	for(TActorIterator<ASWATCharacter> It(GetWorld()); It; ++It)
	{
		ASWATCharacter* Actor = *It;
		if(URosterCharacter* RosterCharacter = Actor->GetRosterCharacter())
		{
			if(RosterCharacter->bTraitUnlocked && IsValid(RosterCharacter->Trait))
			{
				UnlockedTraits.AddUnique(RosterCharacter->Trait->Reference);
			}
		}
	}

	if(numUnlockedTraitsBefore == UnlockedTraits.Num())
	{
		return;
	}

	LocalAchievements->SetUnlockedTraits(UnlockedTraits);
	
	if(const URosterManagerSettings* RosterManagerSettings = GetDefault<URosterManagerSettings>())
	{
		if(RosterManagerSettings->AvailableTraits.Num() <= UnlockedTraits.Num())
		{
			UnlockAchievement(EAchievement::THE_EMPEROR);
		}
	}
}

/* Low level - Achievement handling code */

void UAchievementSubsystem::QueryAchievementsAndStatsSuccess(int32 SteamErrorOutput)
{
	bInitialized = true;
	TArray<FUniversalStat> StatsOut;

	// Get Online Stats
	bool bGotOnlineStats = GetUserStoredStats(StatsOut);
	// Get Saved Stats
	TMap<EAchievementStats, FUniversalStat> SavedStats = LocalAchievements->GetStats();

	// Get Online Achievements
	for (EAchievement Achievement : TEnumRange<EAchievement>())
	{
		bool AchievementValue = false;
		if (GetAchievementData(Achievement, AchievementValue))
		{
			if (AchievementValue) 
			{
				UnlockedAchievements.Add(Achievement);
				LocalAchievements->UnlockAchievement(Achievement);
			}
		}
	}
	// Get Saved Achievements
	TArray<EAchievement> SavedAchievements = LocalAchievements->GetAchievements();
	for (EAchievement Achievement : SavedAchievements)
	{
		UnlockAchievement(Achievement);
	}

	if (bGotOnlineStats)
	{
		for (EAchievementStats StatId : TEnumRange<EAchievementStats>())
		{
			FUniversalStat BestStats = StatsOut[(int)StatId];
			if (SavedStats.Contains(StatId))
			{
				bool dirty;
				BestStats = GetBestStat(StatsOut[(int)StatId], SavedStats[StatId], dirty);
				bAchievementsOrStatsDirty |= dirty;
			}

			if (BestStats.StatType == EUniversalStatType::STAT_FLOAT)
			{
				SetAchievementStat(StatId, BestStats.FloatValue);
			}
			else
			{
				SetAchievementStat(StatId, BestStats.IntegerValue);
			}
		}
	}
	else
	{
		UE_LOG(LogReadyOrNotAchievements, Error, TEXT("Reading of  stats failed from steam failed"));
		// update 
		for (EAchievementStats StatId : TEnumRange<EAchievementStats>())
		{
			if (SavedStats.Contains(StatId))
			{
				if (SavedStats[StatId].StatType == EUniversalStatType::STAT_FLOAT)
				{
					SetAchievementStat(StatId, SavedStats[StatId].FloatValue);
				}
				else
				{
					SetAchievementStat(StatId, SavedStats[StatId].IntegerValue);
				}
			}
		}
		bAchievementsOrStatsDirty = true;
	}
}


void UAchievementSubsystem::QueryAchievementsAndStatsFailure(int32 SteamErrorOutput)
{
	UE_LOG(LogReadyOrNotAchievements, Error, TEXT("UAchievementSubsystem initialization failure error: %d"), SteamErrorOutput);
}

// precache achievements
void UAchievementSubsystem::QueryAchievementsAndStats() // QueryAchievementsAndStatsCompleteDelegate RunWhenInitialized)
{
#if defined(WITH_STEAM)
	// SteamUserStats()->RequestUserStats, is already run in OnlineLeaderboardInterfaceSteam.cpp

	// RequestStatAndAchievements = URequestStatsAndAchievements::RequestUserStatsAndAchievements();
	//
	// RequestStatAndAchievements->onSuccess.AddUniqueDynamic(this, &UAchievementSubsystem::QueryAchievementsAndStatsSuccess);
	// RequestStatAndAchievements->onFailure.AddUniqueDynamic(this, &UAchievementSubsystem::QueryAchievementsAndStatsFailure);
	//
	// RequestStatAndAchievements->Activate();

    return;
#elif defined(TARGET_CONSOLE)
	FOnlineSubsystemBPCallHelperAdvanced Helper(TEXT("OnlineActivityManager"), GetWorld());
	
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	Helper.QueryIDFromPlayerController(PlayerController);
	
	if (!Helper.IsValid())
	{
		UE_LOG(LogReadyOrNotAchievements, Error, TEXT("Could not get Online subsystem Helper."));
		//RunWhenInitialized.Execute(false);
		return;
	}
		
	IOnlineSubsystem* OnlineSub = Helper.OnlineSub;
	if (!OnlineSub)
	{
		UE_LOG(LogReadyOrNotAchievements, Error, TEXT("Could not get Online subsystem Helper."));
		//RunWhenInitialized.Execute(false);
		return;
	}

	// IOnlineIdentityPtr Identity = OnlineSub->GetIdentityInterface();
	// if (!Identity.IsValid())
	// {
	// 	UE_LOG(LogReadyOrNotAchievements, Error, TEXT("Could not get Online Identity interface."));
	// 	RunWhenInitialized.Execute(false);
	// 	return;
	// }

	if (!Helper.UserID.IsValid())
	{
		UE_LOG(LogReadyOrNotAchievements, Error, TEXT("Could not get Online Identity UserID."));
		//RunWhenInitialized.Execute(false);
		return;
	}

	IOnlineAchievementsPtr Achievements = OnlineSub->GetAchievementsInterface();
	if (!Achievements.IsValid())
	{
		UE_LOG(LogReadyOrNotAchievements, Error, TEXT("Could not get Achievements Interface."));
		//RunWhenInitialized.Execute(false);
		return;
	}
	else
	{
		Achievements->QueryAchievements(*Helper.UserID, FOnQueryAchievementsCompleteDelegate::CreateLambda([this/*, RunWhenInitialized*/](const FUniqueNetId& PlayerId, const bool bWasSuccessful)
		{
			UE_LOG(LogReadyOrNotAchievements, Warning, TEXT("UAchievementSubsystem::OnQueryAchievementsComplete(bWasSuccessful = %s)"), bWasSuccessful ? TEXT("TRUE") : TEXT("FALSE"));
			
			if (bWasSuccessful)
			{
				// continue initalizing stats
				QueryStatsConsole(/*RunWhenInitialized*/);
			}
			else
			{
				//RunWhenInitialized.ExecuteIfBound(false);
			}
		}));
	}
#else
	UE_LOG(LogReadyOrNotAchievements, Error, TEXT("Platform does not support Achievements"));
#endif
}


void UAchievementSubsystem::RetrySteamStats()
{
	bAchievementsOrStatsDirty = true;
}

bool UAchievementSubsystem::HasOnlyUsedItems(TArray<EItemCategory> Categories)
{
	for (auto& Item : UsedItemsInCurrentMission)
	{
		bool success = false;
		for(auto Category : Categories)
		{
			if(Item->ContainsItemCategory(Category))
			{
				success = true;
				break;
			}
		}
		if(!success)
		{
			return false;
		}
	}
	return true;
}

void UAchievementSubsystem::UsedItem(ABaseItem* Item)
{
	if (!bInitialized) return;
	
	UsedItemsInCurrentMission.Add(Item);
}

void UAchievementSubsystem::BeginMission()
{
	if (!bInitialized) return;

	UsedItemsInCurrentMission.Empty();
}

void UAchievementSubsystem::EndMission(UObject* WorldContextObject)
{
	if (!bInitialized) return;

	UReadyOrNotGameInstance* gi = UBpGameplayHelperLib::GetRONGameInstance();
	const FString MissionName = UReadyOrNotFunctionLibrary::GetCurrentLevelNameForLookupTable(gi).ToString();
	const FString SafeMissionName = MissionName.ToLower().Replace(TEXT("barricadedsuspects_"),TEXT(""),ESearchCase::IgnoreCase);
	bool IsCampaign = false;
	
	UE_LOG(LogReadyOrNotAchievements, Log, TEXT("EndMission %s"), *MissionName);

	if(const UCampaignData* CampaignData = UBpGameplayHelperLib::GetCampaignData())
	{
		IsCampaign = CampaignData->Levels.Num()>0;
	}

	float PercentageScore = 0.0f;
	if(AScoringManager* const ScoringManager = AScoringManager::Get())
	{
		PercentageScore = ScoringManager->GetCurrentScoreAsPercentage();
	}
	bool IsGradeCOrAbove = PercentageScore >= SCORE_REQUIRED_C;

	// check score base on mission name
	UpdateLevelScore(SafeMissionName, PercentageScore);

	if(IsCampaign && IsGradeCOrAbove && UBpGameplayHelperLib::IsIronmanMode(WorldContextObject))
	{
		UpdateIronmanAchievements(SafeMissionName);
	}
		
	CheckForAnyNewLevelScoreAchievements();
	UpdateAchievedTraits();

	CacheCustomizationRequirements();
	CheckCustomizationAchievement(TagRequirementsForAllCustomItems, EAchievement::DRESS_TO_IMPRESS);
	CheckCustomizationAchievement(TagRequirementsForAllTattoos, EAchievement::INKED_UP);
	
	if(SafeMissionName == PORT_MISSION_NAME && IsGradeCOrAbove)
	{
		if(HasOnlyUsedItems(TArray<EItemCategory> {EItemCategory::IC_BreachingShotgun, EItemCategory::IC_Taser, EItemCategory::IC_Flashbang }))
		{
			UnlockAchievement(EAchievement::THE_HANGED_MAN);
		}
	}
	else if(SafeMissionName == COYOTE_MISSION_NAME && IsGradeCOrAbove) {

		TArray<ABaseItem*> UsedWeapons = UsedItemsInCurrentMission.Array().FilterByPredicate(
				[](ABaseItem* Item) {
					if(Item)
					{
						return Item->WeaponType != EWeaponType::WT_None;
					}
					return false;
				}
			);
		
		if(UsedWeapons.Num() == 1 && UsedWeapons[0]->GetClass()->GetName() == PYTHON_REVOLVER_NAME)
		{
			UnlockAchievement(EAchievement::WAY_OUT_WEST);
		}
	}
}

void UAchievementSubsystem::CacheCustomizationRequirements()
{
	if(TagRequirementsForAllCustomItems.Num() > 0 && TagRequirementsForAllTattoos.Num() > 0)
	{
		return;
	}
	
	TagRequirementsForAllCustomItems = TArray<FName>();
	TagRequirementsForAllTattoos = TArray<FName>();
	
	UWorld* World = GetWorld();
	if (!World)
		return;

	UReadyOrNotGameInstance* GameInstance = World->GetGameInstance<UReadyOrNotGameInstance>();

	if (!GameInstance || !GameInstance->LoadoutManager)
		return;
	
	for (UCustomizationDataBase* DataAsset : GameInstance->LoadoutManager->CustomizationAssets)
	{
		if (!DataAsset->bShowInLoadout)
			continue;

		if(DataAsset->Type != ECustomizationType::Tattoo
			&& !ItemCustomizationTypes.Contains(DataAsset->Type))
			continue;

		if(DataAsset->LockedToDLC.Num() > 0)
			continue;
		
		for (FName RequiredTag : DataAsset->RequiredTags)
		{
			if(DataAsset->Type == ECustomizationType::Tattoo)
			{
				TagRequirementsForAllTattoos.AddUnique(RequiredTag);
			}
			else
			{
				TagRequirementsForAllCustomItems.AddUnique(RequiredTag);
			}
		}
	}
}

void UAchievementSubsystem::DrankBeverage(FString Beverage)
{
	FString SafeBeverageName = Beverage.Replace(TEXT(VENDING_MACHINE_BEVERAGE_PREFIX),TEXT(""),ESearchCase::IgnoreCase);

	if(CaffeineFreeBeverages.Contains(SafeBeverageName))
	{
		return;
	}

	if(SafeBeverageName == VENDING_MACHINE_COFFEE_STRONG)
	{
		BeverageCount+=3;
	}
	else if (LessCaffeineBeverages.Contains(SafeBeverageName))
	{
		BeverageCount+=1;
	}
	else {
		// If we can't find the beverage, we assume it's a coffee so that the achievement won't break
		BeverageCount+=2;
	}

	// Get the achievement for getting summarized caffeine points at 8 or more
	if(BeverageCount > 8)
	{
		UnlockAchievement(EAchievement::TEMPERANCE);
	}
}

/* UAchievementStatics */

UAchievementSubsystem* UAchievementStatics::GetAchievementSubsystem(UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (!World)
		return nullptr;
	
	UAchievementSubsystem* AchievementsSubsystem = World->GetGameInstance()->GetSubsystem<UAchievementSubsystem>();
	if (!AchievementsSubsystem)
		return nullptr;

	return AchievementsSubsystem;
}

void UAchievementStatics::SetAchievementStat(UObject* WorldContextObject, EAchievementStats Stat, float Value)
{
	if (UAchievementSubsystem* AchievementsSubsystem = UAchievementStatics::GetAchievementSubsystem(WorldContextObject))
	{
		AchievementsSubsystem->SetAchievementStat(Stat, Value);
	}
}

void UAchievementStatics::IncreaseAchievementStat(UObject* WorldContextObject, EAchievementStats Stat, float Amount)
{
	if (UAchievementSubsystem* AchievementsSubsystem = UAchievementStatics::GetAchievementSubsystem(WorldContextObject))
	{
		AchievementsSubsystem->IncreaseAchievementStat(Stat, Amount);
	}	
}

void UAchievementStatics::UnlockAchievement(UObject* WorldContextObject, EAchievement Achievement, bool SupportMultiplayer)
{
	if (UAchievementSubsystem* AchievementsSubsystem = UAchievementStatics::GetAchievementSubsystem(WorldContextObject))
	{
		UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
		if (UBpGameplayHelperLib::IsMultiplayer(World) && !SupportMultiplayer)
			return;

		AchievementsSubsystem->UnlockAchievement(Achievement);
	}
}

void UAchievementStatics::BeginMission(UObject* WorldContextObject)
{
	if (UAchievementSubsystem* AchievementsSubsystem = GetAchievementSubsystem(WorldContextObject))
	{
		AchievementsSubsystem->BeginMission();
	}	
}

void UAchievementStatics::EndMission(UObject* WorldContextObject)
{
	if (UAchievementSubsystem* AchievementsSubsystem = GetAchievementSubsystem(WorldContextObject))
	{
		AchievementsSubsystem->EndMission(WorldContextObject);
	}
}

void UAchievementStatics::UsedItem(UObject* WorldContextObject, ABaseItem* Item)
{
	if (UAchievementSubsystem* AchievementsSubsystem = GetAchievementSubsystem(WorldContextObject))
	{
		AchievementsSubsystem->UsedItem(Item);
	}
}

void UAchievementStatics::RetrySteamStats(UObject* WorldContextObject)
{
	if (UAchievementSubsystem* AchievementsSubsystem = GetAchievementSubsystem(WorldContextObject))
	{
		AchievementsSubsystem->RetrySteamStats();
	}
}

/* ULocalAchievements */

void ULocalAchievements::UnlockAchievement(EAchievement Achievement)
{
	if (!Achievements.Contains(Achievement))
	{
		Achievements.Add(Achievement);
		Save();
	}
}

void ULocalAchievements::SetAchievementStat(const EAchievementStats Stat, const EUniversalStatType StatType, const float Value)
{
	if(Stats.Contains(Stat)
		&& (StatType == EUniversalStatType::STAT_FLOAT && Stats[Stat].FloatValue < Value)
		&& (StatType == EUniversalStatType::STAT_INT && Stats[Stat].IntegerValue < Value)
		)
	{
		return;
	}
		
	FUniversalStat StatData;
	StatData.StatType = StatType;

	switch(StatType)
	{
	case EUniversalStatType::STAT_FLOAT:
		StatData.FloatValue = Value;
		break;
	case EUniversalStatType::STAT_INT:
		StatData.IntegerValue = Value;
		break;
	default:
		UE_LOG(LogReadyOrNotAchievements, Error, TEXT("Unknown StatType"));
		return;
	}
	
	Stats.Add(Stat, StatData);
	Save();
}

TArray<FName> ULocalAchievements::GetUnlockedTraits()
{
	return UnlockedTraits;
}

void ULocalAchievements::SetUnlockedTraits(TArray<FName> Traits)
{
	UnlockedTraits = Traits;
	Save();
}

ULocalAchievements* ULocalAchievements::Load()
{
	ULocalAchievements* LocalAchievements = Cast<ULocalAchievements>(UGameplayStatics::LoadGameFromSlot(ACHIEVEMENTS_SLOT_NAME, 0));
	if (!IsValid(LocalAchievements)) {
		LocalAchievements = Cast<ULocalAchievements>(UGameplayStatics::CreateSaveGameObject(ULocalAchievements::StaticClass()));
	}

	return LocalAchievements;
}

void ULocalAchievements::Save()
{
	UGameplayStatics::SaveGameToSlot(this, ACHIEVEMENTS_SLOT_NAME , 0);
}

TMap<EAchievementStats, FUniversalStat> ULocalAchievements::GetStats()
{
	return Stats;
}

TArray<EAchievement> ULocalAchievements::GetAchievements()
{
	return Achievements;
}
