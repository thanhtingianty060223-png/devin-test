// Copyright Void Interactive, 2023

#include "MetaGameProfile.h"

const FString SlotName = "MetaGameProfile";
constexpr int32 InitialMetaGameProfileVersion = 1;

UMetaGameProfile::UMetaGameProfile()
{
	MetaGameVersion = InitialMetaGameProfileVersion;
}

void UMetaGameProfile::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if (Ar.IsLoading())
	{
		MetaGameVersion = InitialMetaGameProfileVersion;
	}
}

UMetaGameProfile* UMetaGameProfile::GetProfile(UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (!World)
		return nullptr;

	UReadyOrNotGameInstance* GameInstance = World->GetGameInstance<UReadyOrNotGameInstance>();
	if (!GameInstance)
		return nullptr;

	return GameInstance->MetaGameProfile;
}

void UMetaGameProfile::SaveProfile()
{
	UGameplayStatics::SaveGameToSlot(this, SlotName, 0);
}

UMetaGameProfile* UMetaGameProfile::LoadProfile()
{
	UWorld* World = UBpGameplayHelperLib::GetWorldStatic();
	if (World)
	{
		UReadyOrNotGameInstance* GameInstance = World->GetGameInstance<UReadyOrNotGameInstance>();
		if (GameInstance->MetaGameProfile)
			return GameInstance->MetaGameProfile;
	}
	
	UMetaGameProfile* SaveData = Cast<UMetaGameProfile>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
	if (SaveData)
		return SaveData;
	
	SaveData = Cast<UMetaGameProfile>(UGameplayStatics::CreateSaveGameObject(UMetaGameProfile::StaticClass()));
	UGameplayStatics::SaveGameToSlot(SaveData, SlotName, 0);

	// NOTE(killo): reset keybinds on creating a new meta game profile to get around our keybind setup
	// not giving us an effective way to ensure new keys are bound by the user
	UReadyOrNotGameUserSettings::ResetKeybinds();
	
	return SaveData;
}

void UMetaGameProfile::AddCompletedLevel(const FString& Level)
{
	if (!CompletedLevels.Contains(Level))
		TemporaryData.NewCompletedLevels.AddUnique(Level);
	
	CompletedLevels.AddUnique(Level);
}

void UMetaGameProfile::AddCompletedMultiplayerLevel(const FString& Level)
{
	CompletedMultiplayerLevels.AddUnique(Level);
}

void UMetaGameProfile::AddProgressionTag(FName Tag)
{
	if (!ProgressionTags.Contains(Tag))
		TemporaryData.NewProgressionTags.Add(Tag);
	
	ProgressionTags.Add(Tag);
}

void UMetaGameProfile::AddProgressionTags(const TSet<FName>& Tags)
{
	TSet<FName> Difference = Tags.Difference(ProgressionTags);
	
	TemporaryData.NewProgressionTags.Append(Difference);
	ProgressionTags.Append(Tags);
}

void UMetaGameProfile::AddExfilData(FExfiltrationData ExfilData)
{
	TemporaryData.ExfilData = ExfilData;
}
