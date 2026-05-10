// Copyright Void Interactive, 2023

#include "CommanderProfile.h"

#include "CampaignData.h"

const FString DebugSlotName = "CommanderDebugSlot";
constexpr int32 InitialCommanderProfileVersion = 1;

UCommanderProfile::UCommanderProfile()
{
	CommanderVersion = InitialCommanderProfileVersion;
}

void UCommanderProfile::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if (Ar.IsLoading())
	{
		CommanderVersion = InitialCommanderProfileVersion;
	}
}

void UCommanderProfile::SaveProfile()
{
	if (!ensure(!Slot.IsEmpty()))
		return;
	
	SaveDate = FDateTime::Now();
	
	UReadyOrNotGameInstance* GameInstance = UReadyOrNotStatics::GetReadyOrNotGameInstance();
	if (GameInstance)
	{
		bIsModded = GameInstance->IsGameModded();
		GameChecksum = GameInstance->Checksum;
	}
	
	UGameplayStatics::SaveGameToSlot(this, Slot, 0);
}

UCommanderProfile* UCommanderProfile::LoadProfile(const FString& Slot)
{
	if (Slot.IsEmpty())
		return nullptr;
	
	UCommanderProfile* Profile = Cast<UCommanderProfile>(UGameplayStatics::LoadGameFromSlot(Slot, 0));
	if (!Profile)
		return nullptr;

	Profile->Slot = Slot;
	
	return Profile;
}

UCommanderProfile* UCommanderProfile::LoadProfile(int32 Slot)
{
	FString SlotName = GetSlotNameFromIndex(Slot);
	return LoadProfile(SlotName);
}

UCommanderProfile* UCommanderProfile::CreateProfile(const FString& Slot, bool bIronmanMode)
{
	UCommanderProfile* Profile = Cast<UCommanderProfile>(UGameplayStatics::CreateSaveGameObject(UCommanderProfile::StaticClass()));
	if (!Profile)
		return nullptr;

	// Set the slot name for tracking as well as our default campaign data
	Profile->Slot = Slot;
	Profile->Campaign = UBpGameplayHelperLib::GetCampaignData();
	Profile->bIronmanMode = bIronmanMode;
	Profile->SaveProfile();
	
	return Profile;
}

UCommanderProfile* UCommanderProfile::CreateProfile(int32 Slot, bool bIronmanMode)
{
	return CreateProfile(GetSlotNameFromIndex(Slot), bIronmanMode);
}

UCommanderProfile* UCommanderProfile::GetDebugProfile()
{
	UCommanderProfile* CommanderProfile = LoadProfile(DebugSlotName);
	if (!CommanderProfile)
		CommanderProfile = CreateProfile(DebugSlotName);

	return CommanderProfile;
}

void UCommanderProfile::DeleteProfile()
{
	if (!ensure(!Slot.IsEmpty()))
		return;
	
	UGameplayStatics::DeleteGameInSlot(Slot, 0);
}

FText UCommanderProfile::GetMostRecentLevelName(FText NothingCompletedText) const
{
	if (CompletedLevels.Num() > 0)
	{
		FString MostRecentLevel = CompletedLevels.Last();
		
		FLevelDataLookupTable LevelData = UBpGameplayHelperLib::GetMapDetailsFromName(MostRecentLevel);
		return !LevelData.FriendlyLevelName.IsEmpty() ? LevelData.FriendlyLevelName : FText::FromString(MostRecentLevel);
	}
	
	return NothingCompletedText;
}

TSoftObjectPtr<UTexture2D> UCommanderProfile::GetMostRecentLevelImage() const
{
	if (CompletedLevels.Num() > 0)
	{
		FString MostRecentLevel = CompletedLevels.Last();
		
		FLevelDataLookupTable LevelData = UBpGameplayHelperLib::GetMapDetailsFromName(MostRecentLevel);
		return LevelData.LevelPicture;
	}

	return nullptr;
}

FString UCommanderProfile::GetNextLevel() const
{
	if (!Campaign || Campaign->Levels.Num() <= 0)
		return FString();

	// Ensure no empty strings in levels array
	TArray<FString> CampaignLevels = Campaign->Levels;
	CampaignLevels.RemoveAll([](const FString& String) { return String.IsEmpty(); });

	// Get the most recently completed level name
	FString LastLevel = CompletedLevels.Num() > 0 ? CompletedLevels.Last() : FString();

	// Then find that level in the campaign and get the next level, if out of bounds return empty string
	int32 NextLevelIndex = CampaignLevels.IndexOfByKey(LastLevel) + 1;
	FString NextLevel = CampaignLevels.IsValidIndex(NextLevelIndex) ? Campaign->Levels[NextLevelIndex] : FString();

	return NextLevel;
}

FText UCommanderProfile::GetNextLevelName(FText CompletedText) const
{
	FString NextLevel = GetNextLevel();
	
	if (!NextLevel.IsEmpty())
	{
		FLevelDataLookupTable LevelData = UBpGameplayHelperLib::GetMapDetailsFromName(NextLevel);
		return !LevelData.FriendlyLevelName.IsEmpty() ? LevelData.FriendlyLevelName : FText::FromString(NextLevel);
	}
	return CompletedText;
}

TSoftObjectPtr<UTexture2D> UCommanderProfile::GetNextLevelImage() const
{
	FString NextLevel = GetNextLevel();
	
	if (!NextLevel.IsEmpty())
	{
		FLevelDataLookupTable LevelData = UBpGameplayHelperLib::GetMapDetailsFromName(NextLevel);
		return LevelData.LevelPicture;
	}

	return nullptr;
}

float UCommanderProfile::GetCompletionPercentage() const
{
	if (!ensure(Campaign))
		return 0.0f;

	int32 TotalLevels = Campaign->Levels.Num();
	int32 CompletedCount = 0;
	
	for (FString Level : Campaign->Levels)
	{
		if (CompletedLevels.Contains(Level))
			CompletedCount++;
	}

	return FMath::Clamp(static_cast<float>(CompletedCount) / TotalLevels, 0.0f, 1.0f);
}

bool UCommanderProfile::IsChecksumMismatched() const
{
	UReadyOrNotGameInstance* GameInstance = UReadyOrNotStatics::GetReadyOrNotGameInstance();
	if (!GameInstance)
		return false;

	// Return true if the checksum is mismatched ONLY if our game or the profile is modded
	return GameChecksum != GameInstance->Checksum &&
		(bIsModded || GameInstance->bIsModded);
}

FString UCommanderProfile::GetSlotNameFromIndex(int32 SlotIndex)
{
	if (SlotIndex < 0 || SlotIndex > 999)
		return FString();
	
	return FString::Printf(TEXT("CommanderSlot%03d"), SlotIndex); 
}
