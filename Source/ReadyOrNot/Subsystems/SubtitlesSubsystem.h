// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "SubtitlesSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FSubtitleData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString Speaker;

	UPROPERTY(BlueprintReadOnly)
	FName SpeakerTag;
	
	UPROPERTY(BlueprintReadOnly)
	FString Dialogue;

	UPROPERTY(BlueprintReadOnly)
	float Length = 1.0f;
};

USTRUCT()
struct FSubtitleParameters
{
	GENERATED_BODY()
	
	FString Speaker;
	FString VoiceLine;
	float Length = 0.0f;
	FName SpeakerTag = NAME_None;
	FString FallbackName;
	FString ScreenNameOverride;
};

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Subtitles"))
class READYORNOT_API USubtitlesSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditAnywhere, Category="Subtitles")
	FString FallbackLocale = "en";

	UPROPERTY(Config, EditAnywhere, Category="Subtitles")
	TMap<FString, FText> AvailableLocales;

	UPROPERTY(Config, EditAnywhere, Category="Subtitles")
	TMap<FString, float> VoiceLineCooldowns;
};

UCLASS(Config=Game)
class READYORNOT_API USubtitlesSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

	FString CurrentLocale;
	
	TMap<FString, FString> VoiceLineMap;
	TMap<FString, FString> FallbackVoiceLineMap;

	TMap<FString, FString> SpeakerMap;

	TMap<FString, float> VoiceLineTimeMap;
	
	void LoadVoiceLines(TMap<FString, FString>& OutVoiceLineMap, const FString& Locale);
	void LoadSpeakers();
	
	bool IsLocaleValid(const FString& Locale);

// Playstation uses at9 compression
#if defined(TARGET_PS5) || defined(TARGET_PS4)
	static FString GetVoiceOverPath() { return FPaths::Combine(FPaths::ProjectContentDir(), TEXT("VO_PS")); }
#elif defined(TARGET_XB1) || defined(TARGET_XSX)
	static FString GetVoiceOverPath() { return FPaths::Combine(FPaths::ProjectContentDir(), TEXT("VO_XBOX")); }
#else
	static FString GetVoiceOverPath() { return FPaths::Combine(FPaths::ProjectContentDir(), TEXT("VO")); }
#endif
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	void PlaySubtitles(const FSubtitleParameters& Parameters);
	void SetLocale(FString Locale);
	
	DECLARE_MULTICAST_DELEGATE_OneParam(FPlaySubtitles, const FSubtitleData&);
	FPlaySubtitles PlaySubtitlesDelegate;

protected:
	bool IsVoiceLineOnCooldown(const FString& VoiceLine);
};

class FDelayAction : public FPendingLatentAction
{
public:
	float TimeRemaining;
	FName ExecutionFunction;
	int32 OutputLink;
	FWeakObjectPtr CallbackTarget;
	
	FDelayAction(float Duration, const FLatentActionInfo& LatentInfo)
		: TimeRemaining(Duration)
		, ExecutionFunction(LatentInfo.ExecutionFunction)
		, OutputLink(LatentInfo.Linkage)
		, CallbackTarget(LatentInfo.CallbackTarget)
	{
	}
	
	virtual void UpdateOperation(FLatentResponse& Response) override
	{
		TimeRemaining -= Response.ElapsedTime();
		Response.FinishAndTriggerIf(TimeRemaining <= 0.0f, ExecutionFunction, OutputLink, CallbackTarget);
	}

#if WITH_EDITOR
	// Returns a human readable description of the latent operation's current state
	virtual FString GetDescription() const override
	{
		return "Waiting VO";
		//return FString::Printf( *NSLOCTEXT("DelayAction", "DelayActionTime", "Delay (%.3f seconds left)").ToString(), TimeRemaining);
	}
#endif
};

UCLASS()
class READYORNOT_API USubtitlesStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	static void PlaySubtitles(UObject* WorldContextObject, const FSubtitleParameters& Parameters);

	UFUNCTION(BlueprintCallable, meta=(WorldContext="WorldContextObject"))
	static void PlaySubtitles(UObject* WorldContextObject, const FString& Speaker, const FString& VoiceLine, float Length, FName SpeakerTag);

	static void SetLocale(const UObject* WorldContextObject, const FString& Locale);
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
	static const TMap<FString, FText>& GetAvailableLocales();
};

struct FSpeakerTags
{
	static const FName SwatTag;
	static const FName SuspectTag;
	static const FName CivilianTag;
	static const FName UnknownTag;
	static const FName DefaultTag;
};
