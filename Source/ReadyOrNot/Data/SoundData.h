// Copyright Void Interactive, 2017

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AudioDevice.h"
#include "PhysicsEngine/BodyInstance.h"
#include "Sound/SoundEffectPreset.h"
#include "Sound/SoundEffectSubmix.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Misc/OutputDeviceArchiveWrapper.h"
#include "ProfilingDebugging/ProfilingHelpers.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/App.h"
#include "UObject/UObjectHash.h"
#include "UObject/UObjectIterator.h"
#include "UObject/Package.h"
#include "ActiveSound.h"
#include "ContentStreaming.h"
#include "UnrealEngine.h"
#include "Sound/SoundGroups.h"
#include "Sound/SoundEffectSource.h"
#include "Sound/SoundWave.h"
#include "AudioDecompress.h"
#include "AudioEffect.h"
#include "AudioPluginUtilities.h"
#include "Sound/AudioSettings.h"
#include "Sound/SoundCue.h"
#include "Sound/SoundNode.h"
#include "Sound/SoundNodeWavePlayer.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/WorldSettings.h"
#include "GeneralProjectSettings.h"
#include "HAL/LowLevelMemTracker.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "Developer/AssetTools/Public/IAssetTools.h"
#include "Developer/AssetTools/Public/AssetToolsModule.h"
#include "Editor/EditorEngine.h"
#include "AudioEditorModule.h"
#endif

#include "SoundData.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API USoundData : public UDataAsset
{
	GENERATED_BODY()
public:

	static UAudioComponent* SpawnSoundAtLocation(const UObject* WorldContextObject, USoundBase* Sound, FVector Location, FRotator Rotation, bool bAllowSpatialization, float VolumeMultiplier = 1.f, float PitchMultiplier = 1.f, float StartTime = 0.f, bool bAutoDestroy = true);
	static UAudioComponent* SpawnSoundAttached(USoundBase* Sound, USceneComponent* AttachToComponent, bool bAllowSpatialization, FName AttachPointName = NAME_None, FVector Location = FVector(ForceInit), FRotator Rotation = FRotator::ZeroRotator, EAttachLocation::Type LocationType = EAttachLocation::KeepRelativeOffset, bool bStopWhenAttachedToDestroyed = false, float VolumeMultiplier = 1.f, float PitchMultiplier = 1.f, float StartTime = 0.f, bool bAutoDestroy = true);

	static UAudioComponent* SpawnSoundWithCustomAttenuation(USoundBase* Sound, USceneComponent* AttachToComponent, float AttenuationRange, float Volume = 1.f);

	static USoundData* GetSoundData(UWorld* WorldContext);

};
