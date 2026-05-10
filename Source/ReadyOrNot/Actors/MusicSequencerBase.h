#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/AudioComponent.h"
#include "Components/SceneComponent.h"
#include "Sound/SoundWave.h"
#include "Data/MusicData.h"
#include "MusicSequencerBase.generated.h"

UENUM(BlueprintType)
enum class EMusicState : uint8
{
	MS_Preplanning,
	MS_Ambient,
	MS_Action,
	MS_Scripted,
};

// There is a single Music Sequencer actor in every level. It's responsible for keeping all of the music mixed together like it should be.
UCLASS(BlueprintType)
class READYORNOT_API AMusicSequencerBase : public AActor
{
	GENERATED_BODY()

public:

	UMusicData * GetMusicData();

	UPROPERTY(BlueprintReadOnly)
		USceneComponent* Scene;

	AMusicSequencerBase();
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintImplementableEvent)
		void OnStartedTransitioningToState(EMusicState NewState);

	UFUNCTION(BlueprintImplementableEvent)
		void OnStoppedAudio();

	UFUNCTION(BlueprintImplementableEvent)
		void OnAudioReset();

	// Should not be called with MS_Scripted.
	UFUNCTION(NetMulticast, Reliable, BlueprintCallable)
		void Multicast_StartTransitioningToState(EMusicState NewState);
	virtual void Multicast_StartTransitioningToState_Implementation(EMusicState NewState) { OnStartedTransitioningToState(NewState); };

	UFUNCTION(NetMulticast, Reliable, BlueprintCallable)
		void Multicast_StopAudio();
	virtual void Multicast_StopAudio_Implementation() { OnStoppedAudio(); };

	UFUNCTION(NetMulticast, Reliable)
		void Multicast_ResetAudio();
	virtual void Multicast_ResetAudio_Implementation() { OnAudioReset(); };
};
