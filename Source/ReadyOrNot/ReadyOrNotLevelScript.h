// Copyright Void Interactive, 2022

#pragma once

#include "CoreMinimal.h"
#include "Engine/LevelScriptActor.h"
#include "ReadyOrNotLevelScript.generated.h"

UENUM(BlueprintType)
enum class ELightType : uint8
{
	LT_None,
    LT_Day,
    LT_Night
};

UENUM(BlueprintType)
enum class EGenerationType : uint8
{
	GT_None,
	GT_RandomScenarios
};

static FString WorldGenTypeToString(const EGenerationType& Type)
{
	switch (Type)
	{
		case EGenerationType::GT_None:				return "None";
		case EGenerationType::GT_RandomScenarios:	return "Random Scenarios";
		default: return "Invalid";
	}
}

UCLASS()
class READYORNOT_API AReadyOrNotLevelScript : public ALevelScriptActor
{
	GENERATED_BODY()

public:
	AReadyOrNotLevelScript(const FObjectInitializer& ObjectInitializer);

	#if WITH_EDITOR
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }
	#endif
	
	UPROPERTY(EditAnywhere, Category = "AI Generation")
	EGenerationType WorldGenerationType = EGenerationType::GT_RandomScenarios;

	UPROPERTY()
	UMaterial* TAASharpenFilter;

	UPROPERTY()
	TArray<class AActor*> SpawnedFromNotifyActors;

	// Unused, preserved for updating new properties
	UPROPERTY(VisibleAnywhere)
	float FlashlightIntensity = 6500.0f;

	// Boost value for flashlight intensities
	UPROPERTY(EditAnywhere)
	float FlashlightIntensityBoost = 0.0;

	// Boost value for flashlight bounced light
	UPROPERTY(EditAnywhere)
	float FlashlightBouncedIntensityBoost = 0.0;

	UPROPERTY(BlueprintReadOnly)
	TArray<class ABlockingVolume*> BlockingVolumesInLevel;
	
	UPROPERTY(BlueprintReadOnly)
	TArray<class AVisibilityBlockingVolume*> VisibilityBlockingVolumesInLevel;
	
	UPROPERTY(BlueprintReadOnly)
	TArray<class AReadyOrNotTriggerVolume*> TriggerVolumesInLevel;

	UPROPERTY()
	class AConversationManager* ConversationManager;

	UPROPERTY(EditAnywhere)
	TMap<ELightType, FName> LightingScenarios;

	UPROPERTY(Replicated)
	ELightType LightingType = ELightType::LT_None;

	void OnRep_LightingType();

	UPROPERTY(EditAnywhere, Category = Debug)
	bool bDrawCoverDebug = false;

	UFUNCTION(BlueprintPure, Category = Conversation)
	class AConversationManager* GetConversationManager();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Data)
	FLevelDataLookupTable LevelData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Data)
	class UMusicData* MusicData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Data)
	class UItemData* ItemData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Data)
	class USoundData* SoundData;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnOutOfBoundsTimeLimitEnded);
	UPROPERTY(BlueprintAssignable, Category = "Out of Bounds")
	FOnOutOfBoundsTimeLimitEnded Delegate_OnOutOfBoundsTimeLimitEnded;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Out of Bounds", meta = (Clamp = 0.0f))
	float OutOfBounds_MaxTimeLimit = 10.0f;
	
	UPROPERTY(BlueprintReadOnly, Category = "Out of Bounds")
	float OutOfBoundsTimeRemaining = 10.0f;

	UFUNCTION(BlueprintCallable, Category = "Out Of Bounds")
	void EnableOutOfBounds();
	
	UFUNCTION(BlueprintCallable, Category = "Out Of Bounds")
	void DisableOutOfBounds();
	
	UFUNCTION(BlueprintPure, Category = "Out Of Bounds")
	bool IsOutOfBoundsEnabled() const { return bIsOutOfBoundsEnabled; }
	
	UFUNCTION(BlueprintCallable, Category = "Out Of Bounds")
	void StartOutOfBoundsCountdown();

	UFUNCTION(BlueprintCallable, Category = "Out Of Bounds")
	void StopOutOfBoundsCountdown();

	UFUNCTION(BlueprintPure, Category = "Out Of Bounds")
	FORCEINLINE bool IsCountingDownForOutOfBounds() const { return bShouldCountdownForOutOfBounds; }
	
	UFUNCTION(BlueprintNativeEvent, Category = "Out Of Bounds")
	void OnOutOfBoundsTimeLimitEnded();
	void OnOutOfBoundsTimeLimitEnded_Implementation();

	uint8 bShouldCountdownForOutOfBounds : 1;
	
	uint8 bIsOutOfBoundsEnabled : 1;

	void VisualizeAllAudioSources();
	void RefreshAudioSourcesList();
	
	UPROPERTY(BlueprintReadOnly, Category = "Level")
	TArray<class AAmbientSound*> AudioSourcesInLevel;

	UPROPERTY(BlueprintReadOnly, Category = "Level")
	TArray<class AFMODAmbientSound*> FMODAudioSourcesInLevel;

	UFUNCTION(BlueprintCallable, Category = "Debug")
	void Start3DAudioVisualizer();

	UFUNCTION(BlueprintCallable, Category = "Debug")
    void Stop3DAudioVisualizer();

	UFUNCTION(BlueprintCallable, Category = "Debug")
	FORCEINLINE bool IsVisualizingAudioSources() const { return bVisualizeAudioSources; }
	
	uint8 bVisualizeAudioSources : 1;

	FTimerHandle TH_AudioVisualizer;

	UFUNCTION(BlueprintPure)
	bool AllAudioVolumesTicked() const;
	
	UPROPERTY(BlueprintReadOnly)
	TArray<class AReadyOrNotAudioVolume*> AudioVolumes;
	
protected:
	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;
	virtual void Tick(float DeltaTime) override;

	void OnWorldBeginPlay();

public:
	static float GlobalPlayDeadCooldown;

	UPROPERTY()
	TArray<class ACyberneticCharacter*> AIRequestingCover;
	
	static bool bAnyAIRequestingCover;
	
	// if using darkness, AI will be checking IsInLightSource before being able to see the AI..
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Environmental)
	bool bUseDarkness = false;

	// we probably don't want to consider lights that are super dim as they probs won't be affecting much.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Environmental)
	float MinimumLightIntensityForSource = 1000.0f;

	// the multiplier applied to the ais sight range when the player is considered to be 'in the dark'
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Environmental)
	float DarknessSightRangeMultiplier = 0.2f;

	// whether to include the skylight, directional light as a light source (or just point lights and spot lights in the world).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Environmental)
	bool bIncludeWorldLightsAsSources = false;

	// Whether this level has rain
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Environmental)
	bool bRaining;

	// Whether or not to spawn with AI officers on the level
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = AI)
	bool bSpawnOfficerAI = true;
	
	UPROPERTY(EditAnywhere, Category = "LevelSequenceMVP")
	class ULevelSequence* LevelSequenceMVP;

	UPROPERTY(EditAnywhere, Category = "LevelSequenceTeam")
	class ULevelSequence* LevelSequenceTeam;

	UPROPERTY()
	class ULevelSequencePlayer* LastPlayedSequence;

	FTimerHandle FadeToBlackAfterMVP;
	FTimerHandle FadeFromBlackAfterMVP;

	UFUNCTION(BlueprintCallable, Category = "LevelSequenceMVP")
	void PlayMVPSequence();

	void StopMVPSequence();

	UFUNCTION()
	void OnMVPSequenceFinished();

	UFUNCTION()
	void OnTeamSequenceFinished();

	UFUNCTION(BlueprintNativeEvent, Category = "Anti-Piracy")
			void OnPiracyCheckUpdate(bool bIsPirated, const FString& ProgramDetected);
	virtual void OnPiracyCheckUpdate_Implementation(bool bIsPirated, const FString& ProgramDetected);

private:
	UFUNCTION()
	void OnPiracyCheckUpdate_Private(bool bIsPirated, const FString& ProgramDetected);
};
