// Copyright Void Interactive, 2022
#pragma once

#include "CoreMinimal.h"
#include "ReadyOrNotAudioVolume.generated.h"

/*
 *	Audio volume, plays child components and enables reverb effects when entered
 */
UCLASS()
class READYORNOT_API AReadyOrNotAudioVolume : public AVolume
{
	GENERATED_BODY()
	
	AReadyOrNotAudioVolume();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	UBillboardComponent* BillboardComponent;
#endif

public:	
	// Reverb events to play when walking through this volume
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio Volume")
    TArray<UFMODEvent*> ReverbEvents;

	UFUNCTION(BlueprintPure)
	FORCEINLINE bool HasRanOnce() const { return bRanOnce; }

protected:
	UPROPERTY(BlueprintReadOnly)
	bool bLocalEffectsPlayed = false;
	
	UPROPERTY(BlueprintReadOnly)
	TArray<FFMODEventInstance> EventInstances;

	UPROPERTY(BlueprintReadOnly)
	TArray<UFMODAudioComponent*> AttachedAudioComponents;

	bool bRanOnce = false;

	UFUNCTION(BlueprintPure)
	bool IsAnotherVolumeActivatedAndPlayingEvent(UFMODEvent* Event, FFMODEventInstance& EventInstance) const;
	UFUNCTION(BlueprintPure)
	bool IsAnotherVolumeActivatedAndPlayingEventInst(FFMODEventInstance EventInst) const;
};
