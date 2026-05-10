#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/AudioComponent.h"
#include "Components/SceneComponent.h"
#include "Sound/SoundWave.h"
#include "Data/MusicData.h"
#include "MusicSequencerBase.h"
#include "MusicSequencerUnreal.generated.h"

// There is a single Music Sequencer actor in every level. It's responsible for keeping all of the music mixed together like it should be.
UCLASS(BlueprintType)
class READYORNOT_API AMusicSequencerUnreal : public AMusicSequencerBase
{
	GENERATED_BODY()

	// The last seeked position in the track
	float LastPlayTime = 0.0f;

	// The duration of the currently playing track.
	float CurrentTrackDuration = 0.0f;

	// Whether we are fading out or a transition is coming up
	bool bFadingOut = false;

	// How much time left to fade or transition
	float TimeRemaining = 0.0f;

	// Whether this state requires a transition
	bool bTransition = false;

	// Whether or not we have played a transition
	bool bPlayedTransition = false;

	// The index of the transition sound to play
	int32 TransitionIndex = -1;
	USoundCue* ScriptedAudio;

	void StartFadeoutInternal(float Duration);
	void PrimeTransitionInternal(float MinimumTime, const TArray<FMusicKeyframe>& Keyframes);
	void TransitionForCurrentState();

public:

	UPROPERTY(BlueprintReadOnly)
	UAudioComponent* AudioPlayer;

	// The current music state.
	UPROPERTY(BlueprintReadOnly)
	EMusicState CurrentState;

	// The next music state.
	UPROPERTY(BlueprintReadOnly)
	EMusicState NextState;

	// The next state that will occur after the current scripted music is complete.
	UPROPERTY(BlueprintReadOnly)
	EMusicState NextScriptedState;

	AMusicSequencerUnreal();
	virtual void Tick(float DeltaSeconds) override;

	// Should not be called with MS_Scripted.
	virtual void Multicast_StartTransitioningToState_Implementation(EMusicState NewState) override;
	virtual void Multicast_StopAudio_Implementation() override;
	virtual void Multicast_ResetAudio_Implementation() override;

	UFUNCTION(BlueprintCallable)
	void PlayScriptedMusic(USoundCue* Music, EMusicState NewScriptedState, bool bImmediately);

	UFUNCTION()
	void OnAudioPlaybackPercent(const USoundWave* PlayingSoundWave, const float PlaybackPercent);

	UFUNCTION()
	void OnAudioFinished();
};
