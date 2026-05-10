// Copyright Void Interactive, 2017

#pragma once

#include "Sound/SoundNodeAssetReferencer.h"
#include "SoundNodeVolume.generated.h"

class USoundCue;

/**
 * Sound node that adjusts the volume of the audio stream and does nothing else.
 */
UCLASS(hidecategories = Object, editinlinenew, meta = (DisplayName = "Volume"))
class READYORNOT_API USoundNodeVolume : public USoundNodeAssetReferencer
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = CuePlayer, meta = (DisplayName = "Volume Multiplier"))
	float VolumeAdjust = 1.0f;

	virtual int32 GetMaxChildNodes() const override { return 1; };
	virtual int32 GetMinChildNodes() const override { return 1; };

	virtual void ParseNodes(FAudioDevice* AudioDevice, const UPTRINT NodeWaveInstanceHash, FActiveSound& ActiveSound, const FSoundParseParameters& ParseParams, TArray<FWaveInstance*>& WaveInstances) override;

	virtual void LoadAsset(bool bAddToRoot = false) override;
	virtual void ClearAssetReferences() override;
};
