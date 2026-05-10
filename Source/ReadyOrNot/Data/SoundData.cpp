// Copyright Void Interactive, 2017

#include "SoundData.h"
#include "ReadyOrNot.h"

UAudioComponent* USoundData::SpawnSoundAtLocation(const UObject* WorldContextObject, USoundBase* Sound, FVector Location, FRotator Rotation, bool bAllowSpatialization, float VolumeMultiplier, float PitchMultiplier, float StartTime, bool bAutoDestroy)
{
	if (!Sound || !GEngine || !GEngine->UseSound())
	{
		return nullptr;
	}

	UWorld* ThisWorld = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!ThisWorld || !ThisWorld->bAllowAudioPlayback || ThisWorld->GetNetMode() == NM_DedicatedServer)
	{
		return nullptr;
	}

	const bool bIsInGameWorld = ThisWorld->IsGameWorld();

	FAudioDevice::FCreateComponentParams Params(ThisWorld);
	Params.SetLocation(Location);

	UAudioComponent* AudioComponent = FAudioDevice::CreateComponent(Sound, Params);

	if (AudioComponent)
	{
		AudioComponent->SetWorldLocationAndRotation(Location, Rotation);
		AudioComponent->SetVolumeMultiplier(VolumeMultiplier);
		AudioComponent->SetPitchMultiplier(PitchMultiplier);
		AudioComponent->bAllowSpatialization = bIsInGameWorld;
		AudioComponent->bIsUISound = !bIsInGameWorld;
		AudioComponent->bAutoDestroy = bAutoDestroy;
		AudioComponent->SubtitlePriority = Sound->GetSubtitlePriority();
		AudioComponent->Play(StartTime);
	}

	return AudioComponent;
}

UAudioComponent* USoundData::SpawnSoundAttached(USoundBase* Sound, USceneComponent* AttachToComponent, bool bAllowSpatialization, FName AttachPointName, FVector Location, FRotator Rotation, EAttachLocation::Type LocationType, bool bStopWhenAttachedToDestroyed, float VolumeMultiplier, float PitchMultiplier, float StartTime, bool bAutoDestroy)
{
	if (!Sound)
	{
		return nullptr;
	}

	if (!AttachToComponent)
	{
		UE_LOG(LogScript, Warning, TEXT("USoundD::SpawnSoundAttached: NULL AttachComponent specified! Trying to spawn sound [%s],"), *Sound->GetName());
		return nullptr;
	}

	// Location used to check whether to create a component if out of range
	FVector TestLocation = Location;
	if (LocationType != EAttachLocation::KeepWorldPosition)
	{
		if (AttachPointName != NAME_None)
		{
			TestLocation = AttachToComponent->GetSocketTransform(AttachPointName).TransformPosition(Location);
		}
		else
		{
			TestLocation = AttachToComponent->GetComponentTransform().TransformPosition(Location);
		}
	}

	FAudioDevice::FCreateComponentParams Params(AttachToComponent->GetWorld(), AttachToComponent->GetOwner());
	Params.SetLocation(TestLocation);
	Params.bStopWhenOwnerDestroyed = bStopWhenAttachedToDestroyed;

	
	UAudioComponent* AudioComponent = FAudioDevice::CreateComponent(Sound, Params);
	if (AudioComponent)
	{
		if (UWorld* ComponentWorld = AudioComponent->GetWorld())
		{
			const bool bIsInGameWorld = ComponentWorld->IsGameWorld();
			
			AudioComponent->AttachToComponent(AttachToComponent, FAttachmentTransformRules::KeepRelativeTransform, AttachPointName);
			if (LocationType == EAttachLocation::KeepWorldPosition)
			{
				AudioComponent->SetWorldLocationAndRotation(Location, Rotation);
			}
			else
			{
				AudioComponent->SetRelativeLocationAndRotation(Location, Rotation);
			}
			AudioComponent->SetVolumeMultiplier(VolumeMultiplier);
			AudioComponent->SetPitchMultiplier(PitchMultiplier);
			AudioComponent->bAllowSpatialization = bAllowSpatialization;
			AudioComponent->bIsUISound = !bIsInGameWorld;
			AudioComponent->bAutoDestroy = bAutoDestroy;
			AudioComponent->SubtitlePriority = Sound->GetSubtitlePriority();
			AudioComponent->Play(StartTime);
		}
	}

	return AudioComponent;
}

UAudioComponent* USoundData::SpawnSoundWithCustomAttenuation(USoundBase* Sound, USceneComponent* AttachToComponent, float AttenuationRange, float Volume)
{
	UAudioComponent* audioComp = SpawnSoundAttached(Sound, AttachToComponent, true);
	if (audioComp)
	{
		if (audioComp->AttenuationSettings)
			audioComp->AttenuationSettings->Attenuation.FalloffDistance = AttenuationRange;
		audioComp->SetVolumeMultiplier(Volume);
	}
	return audioComp;
}

USoundData* USoundData::GetSoundData(UWorld* WorldContext)
{
	if (!WorldContext)
		return nullptr;

	AReadyOrNotLevelScript* ls = Cast<AReadyOrNotLevelScript>(WorldContext->GetLevelScriptActor());
	if (ls)
	{
		return ls->SoundData;
	}

	return nullptr;
}
