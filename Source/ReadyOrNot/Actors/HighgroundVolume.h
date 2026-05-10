// Copyright Void Interactive, 2017

#pragma once

#include "CoreMinimal.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "HighgroundVolume.generated.h"

/*
 *	High Ground Volumes are used to denote locations in the world where a Spotter, Marksman, or Sniper team has "vision."
 *	When a specific actor trespasses on a High Ground volume, all players in the world are informed of this news.
 *	Likewise, when an actor leaves, they are also notified.
 *	For players, we only play the entry and leave notifications exactly *once*, otherwise this would get annoying.
 */

UCLASS(Blueprintable)
class READYORNOT_API AHighgroundVolume : public AActor
{
	GENERATED_BODY()

private:
	bool bPlayedEntryTeamEnteringSound = false;
	bool bPlayedEntryTeamExitingSound = false;
	bool bPlayingSoundCurrent = false;
	float LastSoundFinished = 0.0f;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);
	UFUNCTION()
	void OnAudioFinished();

	void StartAudio(USoundBase* Audio);

public:
	AHighgroundVolume();

	void EnableHighgroundVolume(int32 NewSierraDesignation);

	// The amount of time to wait between playing sounds (in seconds)
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Highground)
	float AudioDebounce = 10.0f;

	// The thing that determines the bounds of this volume.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Highground)
	UBoxComponent* Bounds;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Highground)
	UAudioComponent* AudioComp;

	// Whether this highground volume is actively watching for trespassers
	UPROPERTY(BlueprintReadOnly, Category = Highground)
	bool bWatching;

	// The Sierra designation of this volume. 0 = Sierra One, 1 = Sierra Two, 2 = Sierra Three, ... 
	UPROPERTY(BlueprintReadOnly, Category = Highground)
	int32 SierraDesignation;

	// The Personnel Map Point Volume Label associated with this Highground Volume.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Highground)
	FName VolumeLabel;

	// TEMPORARY - these are the sounds that are played (randomly) when a suspect or civilian crosses into the threshold
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Audio)
	TArray<USoundBase*> ContactEnteredVolumeAudio;

	// TEMPORARY - these are the sounds that are played (randomly) when a suspect or civilian crosses into the threshold
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Audio)
	TArray<USoundBase*> ContactExitedVolumeAudio;

	// TEMPORARY - these are the sounds that are played (randomly) when a SWAT officer crosses into the threshold
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Audio)
	TArray<USoundBase*> SwatEnteredVolumeAudio;

	// TEMPORARY - these are the sounds that are played (randomly) whe na SWAT officer crosses into the threshold
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Audio)
	TArray<USoundBase*> SwatExitedVolumeAudio;
};
