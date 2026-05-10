#include "Profile.h"
#include "lib/BpGameplayHelperLib.h"

///////////////////////////////////////////////////////////
//
//	Profile

UReadyOrNotProfile::UReadyOrNotProfile()
{
	SaveSlotName = "UnspecifiedProfile";
	UserIndex = 0;
}

void UReadyOrNotProfile::SaveProfile()
{
	UGameplayStatics::SaveGameToSlot(this, SaveSlotName, UserIndex);
}

UReadyOrNotProfile* UReadyOrNotProfile::CreateDefaultSavegame(TSubclassOf<UReadyOrNotProfile> ProfileClass, FString LoadSlotName)
{
	UReadyOrNotProfile* Profile = Cast<UReadyOrNotProfile>(UGameplayStatics::CreateSaveGameObject(ProfileClass));
	if (Profile == nullptr)
	{
		return nullptr;
	}

	// add defaults here

	UGameplayStatics::SaveGameToSlot(Profile, LoadSlotName, Profile->UserIndex);

	return Profile;
}

void UReadyOrNotProfile::ResetProfile()
{
	// add things to reset here
	LevelStats.Empty();
	ChallengeProgress.Empty();
}

void UReadyOrNotProfile::SaveLevelStats(FBasicLevelStats InStats, bool& NewBestRating, bool& NewBestTime)
{
	UWorld* World = UBpGameplayHelperLib::GetWorldStatic();
	if (!World)
		return;

	AReadyOrNotGameState* GS = World->GetGameState<AReadyOrNotGameState>();
	if (!GS)
		return;
	
	FString Key = World->GetMapName();
	Key.RemoveFromStart(World->StreamingLevelsPrefix);
	Key = Key.ToLower();
	
	if (UReadyOrNotProfile* Profile = GS->GetCurrentProfile())
	{
		FBasicLevelStats LevelStats;

		if (const FBasicLevelStats* LevelStatPtr = Profile->LevelStats.Find(Key))
		{
			LevelStats = *LevelStatPtr;
			LevelStats.TimesCompleted++;
			
			if (InStats.BestRating > LevelStats.BestRating)
			{
				LevelStats.BestRating = InStats.BestRating;
				NewBestRating = true;
			}

			if (LevelStats.BestTime == 0.0f || InStats.BestTime < LevelStats.BestTime)
			{
				LevelStats.BestTime = InStats.BestTime;
				NewBestTime = true;
			}
		}
		else
		{
			InStats.TimesCompleted = 1;
			LevelStats = InStats;
			NewBestRating = true;
			NewBestTime = true;
		}

		Profile->LevelStats.Add(Key, LevelStats);
		Profile->SaveProfile();
	}
}

void UReadyOrNotProfile::LoadLevelStats(FBasicLevelStats& OutStats, ECOOPMode Mode, FString MapName)
{
	UWorld* World = UBpGameplayHelperLib::GetWorldStatic();
	if (!World)
		return;
	
	AReadyOrNotGameState* GS = World->GetGameState<AReadyOrNotGameState>();
	if (!GS)
		return;

	FString Key = World->GetMapName();
	if (!MapName.IsEmpty())
	{
		Key = MapName;
	}
	
	Key.RemoveFromStart(World->StreamingLevelsPrefix);
	Key = Key.ToLower();

	if (UReadyOrNotProfile* Profile = GS->GetCurrentProfile())
	{
		if (const FBasicLevelStats* LevelStatPtr = Profile->LevelStats.Find(Key))
		{
			OutStats = *LevelStatPtr;
		}
	}
}

UReadyOrNotMultiplayerProfile::UReadyOrNotMultiplayerProfile()
{
	SaveSlotName = "LevelStats";
	UserIndex = 0;
}

void UReadyOrNotMultiplayerProfile::ResetProfile()
{
	Super::ResetProfile();

	// add things to reset here
}
