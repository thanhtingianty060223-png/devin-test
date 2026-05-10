// Copyright Void Interactive, 2023

#include "SubtitlesSubsystem.h"

#include "Serialization/Csv/CsvParser.h"

const FName FSpeakerTags::SwatTag = "swat";
const FName FSpeakerTags::SuspectTag = "suspect";
const FName FSpeakerTags::CivilianTag = "civilian";
const FName FSpeakerTags::UnknownTag = "unknown";
const FName FSpeakerTags::DefaultTag = "default";

void USubtitlesSubsystem::LoadVoiceLines(TMap<FString, FString>& OutVoiceLineMap, const FString& Locale)
{
	double StartTime = FPlatformTime::Seconds();
	
	OutVoiceLineMap.Empty();
	FString VoiceOverPath = GetVoiceOverPath();
	
	TArray<FString> SpeakerDirectories;
	IFileManager::Get().FindFiles(SpeakerDirectories, *(VoiceOverPath / TEXT("*")), false, true);
	
	for (const FString& Speaker : SpeakerDirectories)
	{
		double SpeakerStartTime = FPlatformTime::Seconds();

		FString LocalizationFileName = FString::Printf(TEXT("sub_%s.csv"), *Locale);
		FString LocalizationFilePath = FPaths::Combine(VoiceOverPath, Speaker, LocalizationFileName);
		
		FString LocalizationFileData;
		if (!FFileHelper::LoadFileToString(LocalizationFileData, *LocalizationFilePath))
		{
			UE_LOG(LogReadyOrNotSubtitles, Warning, TEXT("Failed to load missing or locked subtitles file '%s'"), *LocalizationFilePath);
			continue;
		}
		
		FCsvParser Parser(LocalizationFileData);
		const FCsvParser::FRows& Rows = Parser.GetRows();

		if (Rows.Num() <= 1)
		{
			UE_LOG(LogReadyOrNotSubtitles, Warning, TEXT("Skipping subtitles file %s due to broken or missing rows"), *LocalizationFilePath);
			continue;
		}

		for (int32 i = 1; i < Rows.Num(); i++)
		{
			if (!Rows[i].IsValidIndex(0) || !Rows[i].IsValidIndex(1))
				continue;
			
			FString Key = Rows[i][0];
			FString Dialogue = Rows[i][1];
			
			Key = Key.TrimStartAndEnd();
			if (Key.IsEmpty())
				continue;

			// Ignore commented keys
			if (Key.StartsWith(TEXT("#")))
				continue;

			Dialogue = Dialogue.TrimStartAndEnd();
			if (Dialogue.IsEmpty())
				continue;
			
			FString FinalKey = FString::Printf(TEXT("%s_%s"), *Speaker, *Key);
			OutVoiceLineMap.Add(FinalKey, Dialogue);
		}

		double SpeakerEndTime = FPlatformTime::Seconds();
		double SpeakerElapsedTime = (SpeakerEndTime - SpeakerStartTime) / 1000.0;
		UE_LOG(LogReadyOrNotSubtitles, Verbose, TEXT("Loaded '%s' subtitles for speaker %s in %f ms"), *Locale, *Speaker, SpeakerElapsedTime);
	}

	double EndTime = FPlatformTime::Seconds();
	double ElapsedTime = (EndTime - StartTime) / 1000.0;
	UE_LOG(LogReadyOrNotSubtitles, Log, TEXT("Loaded subtitles for '%s' locale in %f ms"), *Locale, ElapsedTime);
}

void USubtitlesSubsystem::LoadSpeakers()
{
	SpeakerMap.Empty();

	// FString SpeakersFileName = FString::Printf(TEXT("names_%s.csv"), *CurrentLocale);
	FString SpeakersFilePath = FPaths::Combine(GetVoiceOverPath(), TEXT("names.csv"));
	
	FString SpeakersFileData;
	if (!FFileHelper::LoadFileToString(SpeakersFileData, *SpeakersFilePath))
	{
		UE_LOG(LogReadyOrNotSubtitles, Warning, TEXT("Could not load subtitles character name file %s"), *SpeakersFilePath);
		return;
	}
	
	FCsvParser Parser(SpeakersFileData);
	const FCsvParser::FRows& Rows = Parser.GetRows();

	if (Rows.Num() <= 0)
	{
		UE_LOG(LogReadyOrNotSubtitles, Warning, TEXT("Skipping subtitles character name file %s due to broken or missing rows"), *SpeakersFilePath);
		return;
	}
	
	for (int32 i = 1; i < Rows.Num(); i++)
	{
		if (!Rows[i].IsValidIndex(0) || !Rows[i].IsValidIndex(1))
			continue;
		
		FString Key = Rows[i][0];
		FString Name = Rows[i][1];

		Key = Key.TrimStartAndEnd();
		if (Key.IsEmpty())
			continue;

		Name = Name.TrimStartAndEnd();
		if (Name.IsEmpty())
			continue;
		
		SpeakerMap.Add(Key, Name);
	}
}

bool USubtitlesSubsystem::IsLocaleValid(const FString& Locale)
{
	const USubtitlesSettings* Settings = GetDefault<USubtitlesSettings>();
	return Settings->AvailableLocales.Contains(Locale);
}

void USubtitlesSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	const USubtitlesSettings* Settings = GetDefault<USubtitlesSettings>();
	LoadVoiceLines(FallbackVoiceLineMap, Settings->FallbackLocale);

	// Game user settings should be loaded by the engine at this point
	const UReadyOrNotGameUserSettings* GameUserSettings = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (ensure(GameUserSettings))
	{
		SetLocale(GameUserSettings->SubtitlesLocale);
	}
	else
	{
		SetLocale(Settings->FallbackLocale);
	}
}

void USubtitlesSubsystem::PlaySubtitles(const FSubtitleParameters& Parameters)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_Subtitles_PlaySubtitles);
	
	if (Parameters.Length <= 0.0f)
		return;

	if (IsVoiceLineOnCooldown(Parameters.VoiceLine))
		return;
	
	FString VoiceLineWithoutExtension = FPaths::GetBaseFilename(Parameters.VoiceLine);
	FString LocalizationKey = FString::Printf(TEXT("%s_%s"), *Parameters.Speaker, *VoiceLineWithoutExtension);
	
	FString MappedSpeaker = Parameters.Speaker;

	// Try to resolve a speaker name, using the provided fallback name
	FString* SpeakerLookup = SpeakerMap.Find(Parameters.Speaker);
	if (SpeakerLookup)
	{
		MappedSpeaker = *SpeakerLookup;
	}
	else
	{
		FString* FallbackName = SpeakerMap.Find(Parameters.FallbackName);
		if (FallbackName)
			MappedSpeaker = *FallbackName;
	}

	// Override the on-screen name with a different one if provided
	if (!Parameters.ScreenNameOverride.IsEmpty())
	{
		MappedSpeaker = Parameters.ScreenNameOverride;
	}

	// Attempt to find a transcription, using fallback language if one is not found
	FString* DialogueLookup = VoiceLineMap.Find(LocalizationKey);
	if (!DialogueLookup)
		DialogueLookup = FallbackVoiceLineMap.Find(LocalizationKey);

	// Only play subtitles if a transcription was found
	if (DialogueLookup)
	{
		FString MappedDialogue = *DialogueLookup;
	
		FSubtitleData SubtitleData;
		SubtitleData.Speaker = MappedSpeaker;
		SubtitleData.Dialogue = MappedDialogue;
		SubtitleData.SpeakerTag = Parameters.SpeakerTag;
		SubtitleData.Length = Parameters.Length;
		PlaySubtitlesDelegate.Broadcast(SubtitleData);

		UE_LOG(LogReadyOrNotSubtitles, Verbose, TEXT("Playing subtitles for key %s"), *LocalizationKey);
	}
	else
	{
		UE_LOG(LogReadyOrNotSubtitles, Verbose, TEXT("Subtitle transcription lookup failed for key %s"), *LocalizationKey);
	}
}

void USubtitlesSubsystem::SetLocale(FString Locale)
{
	if (CurrentLocale == Locale)
		return;
	
	if (!IsLocaleValid(Locale))
	{
		UE_LOG(LogReadyOrNotSubtitles, Warning, TEXT("Tried to set invalid locale %s"), *Locale);
		return;
	}
	
	CurrentLocale = Locale;
	
	LoadSpeakers();
	LoadVoiceLines(VoiceLineMap, CurrentLocale);
}

bool USubtitlesSubsystem::IsVoiceLineOnCooldown(const FString& VoiceLine)
{
	const USubtitlesSettings* Settings = GetDefault<USubtitlesSettings>();

	FString OriginalVoiceLine = VoiceLine;
	FString VoiceLineWithoutNumber = VoiceLine;
	
	OriginalVoiceLine.Split("_", &VoiceLineWithoutNumber, nullptr,
		ESearchCase::IgnoreCase, ESearchDir::FromEnd);

	const float* Cooldown = Settings->VoiceLineCooldowns.Find(VoiceLineWithoutNumber);
	if (!Cooldown || *Cooldown <= 0.0f)
		return false;
	
	const float* CurrentCooldownTime = VoiceLineTimeMap.Find(VoiceLineWithoutNumber);
	if (CurrentCooldownTime && *CurrentCooldownTime > FPlatformTime::Seconds())
		return true;

	VoiceLineTimeMap.Add(VoiceLineWithoutNumber, FPlatformTime::Seconds() + *Cooldown);
	return false;
}

void USubtitlesStatics::PlaySubtitles(UObject* WorldContextObject, const FSubtitleParameters& Parameters)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (!World)
		return;

	UGameInstance* GameInstance = World->GetGameInstance();
	if (!GameInstance)
		return;
	
	USubtitlesSubsystem* SubtitlesSubsystem = GameInstance->GetSubsystem<USubtitlesSubsystem>();
	if (!SubtitlesSubsystem)
		return;

	SubtitlesSubsystem->PlaySubtitles(Parameters);
}

void USubtitlesStatics::PlaySubtitles(UObject* WorldContextObject, const FString& Speaker, const FString& VoiceLine,
	float Length, FName SpeakerTag)
{
	FSubtitleParameters Parameters;
	Parameters.Speaker = Speaker;
	Parameters.VoiceLine = VoiceLine;
	Parameters.Length = Length;
	Parameters.SpeakerTag = SpeakerTag;

	PlaySubtitles(WorldContextObject, Parameters);
}

void USubtitlesStatics::SetLocale(const UObject* WorldContextObject, const FString& Locale)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (!World)
		return;

	if (!World->GetGameInstance())
		return;
	
	USubtitlesSubsystem* SubtitlesSubsystem = World->GetGameInstance()->GetSubsystem<USubtitlesSubsystem>();
	if (!SubtitlesSubsystem)
		return;

	SubtitlesSubsystem->SetLocale(Locale);
}

const TMap<FString, FText>& USubtitlesStatics::GetAvailableLocales()
{
	const USubtitlesSettings* Settings = GetDefault<USubtitlesSettings>();
	return Settings->AvailableLocales;
}
