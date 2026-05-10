// Copyright Void Interactive, 2017

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Sound/SoundCue.h"
#include "Sound/SoundBase.h"
#include "FMODEvent.h"
#include "MusicData.generated.h"

USTRUCT(BlueprintType)
struct FMusicKeyframe
{
	GENERATED_USTRUCT_BODY()

	// Where in the track this keyframe is located (in seconds)
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float Position;

	// The name of this label (so we can jump to it in scripting)
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName Label;

	// Whether we can transition away from the track at this keyframe.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bTransitionExit;

	// If this track has Transition Music on, this is the music piece to play at this transition.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	USoundCue* TransitionPiece;
};

USTRUCT(BlueprintType)
struct FMusicTrack
{
	GENERATED_USTRUCT_BODY()

	// The music piece to play on this track.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	USoundCue* MusicPiece;

	// If this is turned on, we will use specialized transition music pieces at Transition Exit keyframes.
	// Otherwise, we will fade out.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bTransitionMusic;

	// The amount of time it takes to fade out this track. (If Transition Music is turned off)
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float FadeTime;

	// The minimum amount of time to play this track.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float MinimumTime;

	// The keyframes for this track
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FMusicKeyframe> Keyframes;
};

USTRUCT(BlueprintType)
struct FMusicTrackFMOD
{
	GENERATED_USTRUCT_BODY()

	// The event for preplanning
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	UFMODEvent* PreplanningEvent;

	// The event for main level music
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	UFMODEvent* LevelEvent;

};

UCLASS(Blueprintable, BlueprintType)
class READYORNOT_API UMusicData : public UDataAsset
{
	GENERATED_BODY()

public:
	// The track(s) to play when in preplanning.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Preplanning)
	FMusicTrack PreplanningTrack;

	// The track(s) to play when not in action.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Ambient)
	FMusicTrack AmbientTrack;

	// The track(s) to play when in action.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Action)
	FMusicTrack ActionTrack;

	// The FMOD music track
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = FMOD)
	FMusicTrackFMOD FMODTracks;
};
