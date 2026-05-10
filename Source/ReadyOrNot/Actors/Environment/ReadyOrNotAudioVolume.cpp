// Copyright Void Interactive, 2022

#include "ReadyOrNotAudioVolume.h"
#include "FMODAmbientSound.h"

AReadyOrNotAudioVolume::AReadyOrNotAudioVolume()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.5f;
	SetActorEnableCollision(false);

	if (GetBrushComponent())
	{
		GetBrushComponent()->SetCollisionObjectType(ECC_VOLUME);
	}

#if WITH_EDITORONLY_DATA
	static ConstructorHelpers::FObjectFinder<UTexture2D> AudioIcon(TEXT("/Engine/EditorResources/AudioIcons/S_AudioComponent.S_AudioComponent"));

	BillboardComponent = CreateDefaultSubobject<UBillboardComponent>("BillboardComponent");
	BillboardComponent->SetupAttachment(GetRootComponent());
	BillboardComponent->SetMobility(EComponentMobility::Movable);
	BillboardComponent->SetSprite(AudioIcon.Object);
	BillboardComponent->bIsScreenSizeScaled = true;
	BillboardComponent->bUseInEditorScaling = true;
	BillboardComponent->EditorScale = 0.75f;
#endif
}

void AReadyOrNotAudioVolume::BeginPlay()
{
	Super::BeginPlay();

	TArray<AActor*> OutActors;
	GetAttachedActors(OutActors, true);

	for (AActor* Actor : OutActors)
	{
		AFMODAmbientSound* FMODAmbientSound = Cast<AFMODAmbientSound>(Actor);
		if (FMODAmbientSound)
		{
			AttachedAudioComponents.Add(FMODAmbientSound->AudioComponent);
		}
	}
}

void AReadyOrNotAudioVolume::Tick(float DeltaSeconds)
{
    if (FMODUtils::IsWorldAudible(GetWorld(), false))
    {
    	LOCAL_PLAYER;
    	
		if (!LocalPlayer)
			return;
    	
		if (!bRanOnce)
		{
			for (UFMODAudioComponent* AudioComp : AttachedAudioComponents)
			{
				if (IsValid(AudioComp))
				{
					AudioComp->SetActive(false);
					
					if (AudioComp->Event)
					{
						const FMOD::Studio::EventDescription* EventDesc = IFMODStudioModule::Get().GetEventDescription(AudioComp->Event, EFMODSystemContext::Max);
						if (EventDesc != nullptr)
						{
							if (!AudioComp->StudioInstance || !AudioComp->StudioInstance->isValid())
							{
								EventDesc->createInstance(&AudioComp->StudioInstance);
							}

							#if WITH_EDITOR
							ensureAlwaysMsgf(AudioComp->StudioInstance != nullptr, TEXT("%s failed to initialize an event instance. Transitions volumes that reference %s, may not behave correctly"), *GetNameSafe(AudioComp), *GetNameSafe(AudioComp->Event));
							#endif
						}
					}
				}
			}
		}
		
		bRanOnce = true;
    	
		// We don't use collision/overlaps since we only care about the local player, so its probably cheaper to do it this way
		if (UKismetMathLibrary::IsPointInBox(LocalPlayer->GetActorLocation(), GetBounds().Origin, GetBounds().BoxExtent))
		{
			if (bLocalEffectsPlayed)
				return;

			bLocalEffectsPlayed = true;
			for (UFMODEvent* Event : ReverbEvents)
			{
				if (Event)
				{
					FFMODEventInstance EventInstance;
					if (!IsAnotherVolumeActivatedAndPlayingEvent(Event, EventInstance))
					{
						EventInstances.Add(UFMODBlueprintStatics::PlayEvent2D(GetWorld(), Event, true));
					}
					else
					{
						EventInstances.Add(EventInstance);
					}
				}
			}
			for (UFMODAudioComponent* AudioComp : AttachedAudioComponents)
			{
				if (AudioComp)
				{
					AudioComp->Play();
				}
			}
		}
		else
		{
			if (!bLocalEffectsPlayed)
				return;

			bLocalEffectsPlayed = false;
			for (FFMODEventInstance EventInst : EventInstances)
			{
				if (!IsAnotherVolumeActivatedAndPlayingEventInst(EventInst))
				{
					UFMODBlueprintStatics::EventInstanceStop(EventInst, true);
				}
			}

			EventInstances.Empty();
			for (UFMODAudioComponent* AudioComp : AttachedAudioComponents)
			{
				if (AudioComp)
				{
					AudioComp->Stop();
				}
			}
		}
    }
}

bool AReadyOrNotAudioVolume::IsAnotherVolumeActivatedAndPlayingEvent(UFMODEvent* Event, FFMODEventInstance& EventInstance) const
{
	for (TActorIterator<AReadyOrNotAudioVolume> It(GetWorld()); It; ++It)
	{
		AReadyOrNotAudioVolume* ReverbVolume = *It;
		if (ReverbVolume == this)
			continue;

		if (!ReverbVolume->bLocalEffectsPlayed)
			continue;

		TArray<UFMODEvent*> Events = ReverbVolume->ReverbEvents;
		TArray<FFMODEventInstance> EventInsts = ReverbVolume->EventInstances;
		if (Events.Num() == EventInsts.Num())
		{
			for (int32 i = 0; i < Events.Num(); i++)
			{
				if (Events[i] == Event)
				{
					EventInstance = EventInsts[i];
					return true;
				}
			}
		}
	}
	return false;
}

bool AReadyOrNotAudioVolume::IsAnotherVolumeActivatedAndPlayingEventInst(FFMODEventInstance EventInst) const
{
	for (TActorIterator<AReadyOrNotAudioVolume> It(GetWorld()); It; ++It)
	{
		AReadyOrNotAudioVolume* ReverbVolume = *It;
		if (ReverbVolume == this)
			continue;

		if (!ReverbVolume->bLocalEffectsPlayed)
			continue;

		for (FFMODEventInstance& OtherInstance : ReverbVolume->EventInstances)
		{
			if (OtherInstance.Instance == EventInst.Instance)
			{
				return true;
			}
		}
	}
	return false;
}
