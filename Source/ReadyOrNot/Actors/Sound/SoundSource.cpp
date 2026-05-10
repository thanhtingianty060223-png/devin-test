// Copyright Void Interactive, 2023

#include "SoundSource.h"

#include "FMODSettings.h"
#include "Actors/Door.h"
#include "Components/FMODAudioPropagationComponent.h"
#include "FMODStudio/Private/FMODListener.h"
#include "Info/SoundManager.h"
#include "Material/Physical/CustomPhysicalMaterial.h"
#include <chrono>

#include "FMODAnimNotifyPlay.h"
#include "SoundSourceAnimNotify.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"

TAutoConsoleVariable<int32> CVarShowSoundPath(TEXT("ShowSoundPath"), 0, TEXT("Show path of Ready or Not sound sources"));
TAutoConsoleVariable<int32> CVarPrintSoundSourceDebug(TEXT("PrintSoundSourceDebug"), 0, TEXT("Show debug information of every SoundSource related sound."));

USoundSource::USoundSource()
{
}

void USoundSource::BeginDestroy()
{
	Super::BeginDestroy();

	// Ensure we release the event instance or else FMOD may call back on a destroyed object
	ReleaseEventInstance();
}

USoundSource* USoundSource::CreateFirstPersonSound(UWorld* InWorld, UFMODEvent* InEvent, FTransform InTransform, TArray<FMODParam> Params, bool InbDebugMode)
{
	USoundManager* SoundManager = InWorld ? InWorld->GetSubsystem<USoundManager>() : nullptr;
	if(SoundManager)
	{
		USoundSource* SoundSource = SoundManager->GetAvailableSoundSource();
		SoundSource->World = InWorld;
		SoundSource->SoundSourceType = ESoundSourceType::SST_FirstPerson;
		SoundSource->Initialize(InEvent, InTransform, Params, EOcclusionType::OT_None, EPropagationType::PT_None, InbDebugMode);
		return SoundSource;
	}
	return nullptr;
}

USoundSource* USoundSource::CreateThirdPersonSound(UWorld* InWorld, UFMODEvent* InEvent, FTransform InTransform, TArray<FMODParam> Params, EOcclusionType InOcclusionType, EPropagationType InPropagationType, bool InbDebugMode)
{
	USoundManager* SoundManager = InWorld ? InWorld->GetSubsystem<USoundManager>() : nullptr;
	if(SoundManager)
	{
		USoundSource* SoundSource = SoundManager->GetAvailableSoundSource();
		SoundSource->World = InWorld;
		SoundSource->SoundSourceType = ESoundSourceType::SST_ThirdPerson;
		SoundSource->Initialize(InEvent, InTransform, Params, InOcclusionType, InPropagationType, InbDebugMode);
		return SoundSource;
	}
	return nullptr;
}

void USoundSource::ResetSoundSource()
{
	if (StudioInstance)
	{
		ReleaseEventInstance();
		OnEventStopped.Broadcast();
	}
	
	Event = nullptr;
	OcclusionType = EOcclusionType::OT_None;
	PropagationType = EPropagationType::PT_None;
	SoundSourceType = ESoundSourceType::SST_ThirdPerson;
	HierarchyType = EHierarchyType::HT_Default;
	ParentSoundSource = nullptr;
	ChildrenSoundSources.Empty();
	bDebugMode = false;
	bIsRunning = false;
	bIsActive = false;
	ParameterCache.Empty();
	PrimaryUpdateInterval = 0.1f;
	AttachPointName = "";
	AttachToComponent = nullptr;
	bIsPaused = false;
	SetProgrammerSound(nullptr);
	NeedDestroyProgrammerSoundCallback = false;
	DoorOcclusionAdditive = 0.7f;
	OutsidePathOcclusionDistance = 750.0f;
	OutsideOcclusionMultiplier = 0.05f;
	AngleOcclusionMultiplier = 1.0f;
	ObstructionOcclusionAdditive = 0.2f;
	World = nullptr;
	bRegisterLater = false;
	bPlayLater = false;
	PrimaryUpdateInterval = 0.2;
	PropagationUpdateInterval = 0.2;
	TimeSincePrimaryUpdate = 0.0f;
	TimeSincePropagationUpdate = 0.0f;
	bObstructedLastTrace = false;
	
	OnSoundStopped.Clear();
	OnSoundFinished.Clear();
	OnEventStopped.Clear();
	
	bWantsProgrammerSoundLength = false;
	OnProgrammerSoundLengthReady.Clear();
	bTriggerProgrammerSoundPlayed = false;
}

void USoundSource::Initialize(UFMODEvent* InEvent, FTransform InTransform, TArray<FMODParam> Params, EOcclusionType InOcclusionType, EPropagationType InPropagationType, bool InbDebugMode)
{
	Event = InEvent;
	Transform = InTransform;
	OcclusionType = InOcclusionType;
	PropagationType = InPropagationType;
	bDebugMode = InbDebugMode;

	InitializeInternal(Params);
}

void USoundSource::CheckForLegacySound()
{
	USoundManager* SoundManager = World ? World->GetSubsystem<USoundManager>() : nullptr;
	if (SoundManager)
	{
		// Force legacy sound.
		if (SoundManager->bForceLegacySound && SoundSourceType != ESoundSourceType::SST_FirstPerson)
		{
			if (PropagationType == EPropagationType::PT_Portal)
			{
				OcclusionType = EOcclusionType::OT_None;
			}
			if (OcclusionType == EOcclusionType::OT_Angular)
			{
				OcclusionType = EOcclusionType::OT_Depth;
			}
		}
	}
}

void USoundSource::InitializeInternal(TArray<FMODParam> Params)
{
	TraceDelegate.BindUObject(this, &USoundSource::OnTraceCompleted);

	InstanceGuid = FGuid::NewGuid();
	bIsActive = true;
	
	for (int i = 0; i < EFMODEventProperty::Count; ++i)
	{
		StoredProperties[i] = -1.0f;
	}

	CacheDefaultParameterValues();

	// Add all custom params
	if (Params.Num() > 0)
	{
		for (int i = 0; i < Params.Num(); i++)
		{
			if (StudioInstance)
			{
				FMOD_RESULT Result = StudioInstance->setParameterByName(
					TCHAR_TO_UTF8(*Params[i].paramName.ToString()), Params[i].paramVal);
				if (Result != FMOD_OK)
				{
					UE_LOG(LogReadyOrNot, Warning, TEXT("Failed to set parameter %s"), *Params[i].paramName.ToString());
				}
			}
			ParameterCache.FindOrAdd(Params[i].paramName) = Params[i].paramVal;
		}
	}
}

void USoundSource::Tick(float DeltaTime)
{
	if (LastTickFrame == GFrameCounter)
		return;

	// If the programmer sound length is desired send it
	if (bTriggerProgrammerSoundPlayed && ProgrammerSoundInstance)
	{
		bTriggerProgrammerSoundPlayed = false;
		
		unsigned LengthMs = 0;
		ProgrammerSoundInstance->getLength(&LengthMs, FMOD_TIMEUNIT_MS);
				
		const float LengthSeconds = static_cast<float>(LengthMs) / 1000.0f;
		OnProgrammerSoundLengthReady.Broadcast(LengthSeconds);
	}
	
	// If the sound has not gotten a chance to play yet as a result of the SoundManager not having been completely initialized.
	USoundManager* SoundManager = World ? World->GetSubsystem<USoundManager>() : nullptr;
	if(bPlayLater && SoundManager && SoundManager->bHasFinishedSetup)
	{
		CheckForLegacySound();
		this->Play();
		bPlayLater = false;
		return;
	}

	// If the SoundSource is paused, no need to update.
	if(bIsPaused)
	{
		return;
	}
	
	if (bIsRunning)
	{
		FMOD_STUDIO_PLAYBACK_STATE state = FMOD_STUDIO_PLAYBACK_STOPPED;
		if (StudioInstance)
		{
			if (bEnableTimelineCallbacks)
			{
				TArray<FTimelineMarkerProperties> LocalMarkerQueue;
				TArray<FTimelineBeatProperties> LocalBeatQueue;
				{
					FScopeLock Lock(&CallbackLock);
					Swap(LocalMarkerQueue, CallbackMarkerQueue);
					Swap(LocalBeatQueue, CallbackBeatQueue);
				}

				for (const FTimelineMarkerProperties& EachProps : LocalMarkerQueue)
				{
					OnTimelineMarker.Broadcast(EachProps.Name, EachProps.Position);
				}
				for (const FTimelineBeatProperties& EachProps : LocalBeatQueue)
				{
					OnTimelineBeat.Broadcast(EachProps.Bar, EachProps.Beat, EachProps.Position, EachProps.Tempo, EachProps.TimeSignatureUpper, EachProps.TimeSignatureLower);
				}
			}

			if (TriggerSoundStoppedDelegate)
			{	
				FScopeLock Lock(&CallbackLock);
				TriggerSoundStoppedDelegate = false;
				OnSoundStopped.Broadcast();
			}
			
			StudioInstance->getPlaybackState(&state);
		}

		if (state == FMOD_STUDIO_PLAYBACK_STOPPED)
		{
			OnPlaybackCompleted();
		}
	}

	// Primary tick data updates
	if(TimeSincePrimaryUpdate >= PrimaryUpdateInterval)
	{
		UpdateTransform();
		TimeSincePrimaryUpdate = 0.0f;
	}
	else
	{
		TimeSincePrimaryUpdate += DeltaTime;
	}

	// Propagation update interval
	if(TimeSincePropagationUpdate >= PropagationUpdateInterval)
	{
		if (bIsRunning)
		{
			if (StudioInstance)
			{
				UpdateTickData();
			}
		}
		TimeSincePropagationUpdate = 0.0f;
	}
	else
	{
		TimeSincePropagationUpdate += DeltaTime;
	}
	
	LastTickFrame = GFrameCounter;
}

ETickableTickType USoundSource::GetTickableTickType() const
{
	return ETickableTickType::Conditional;
}

bool USoundSource::IsTickable() const
{
	return bIsRunning && !bIsPaused;
}

TStatId USoundSource::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(MyTickableClass, STATGROUP_Tickables);
}

void USoundSource::UpdateTickData()
{
	if(!FMODUtils::IsWorldAudible(World, EFMODSystemContext::Max == EFMODSystemContext::Editor))
		return;

	if(SoundSourceType == ESoundSourceType::SST_FirstPerson)
		return;

	// If this SoundSource is a child, we can update it using the parent's parameters
	if(HierarchyType == EHierarchyType::HT_Child)
	{
		if(ParentSoundSource && ParentSoundSource->bIsActive && ParentSoundSource->InstanceGuid == this->InstanceGuid)
		{
			ParameterCache.FindOrAdd("Occlusion") = *ParentSoundSource->ParameterCache.Find("Occlusion");
			ParameterCache.FindOrAdd("PropagateDistance") = *ParentSoundSource->ParameterCache.Find("PropagateDistance");
		}
	}
	else
	{
		std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

		TArray<TSharedPtr<FGraphNode>> ShortestPath1;
		TArray<TSharedPtr<FGraphNode>> ShortestPath2;
		TArray<TSharedPtr<FGraphNode>> UnobstructedPath;
		
		TSharedPtr<FGraphNode> A(new FGraphNode());
		TSharedPtr<FGraphNode> B(new FGraphNode());
		
		ARoomVolume* RoomVolumeA = nullptr;
		ARoomVolume* RoomVolumeB = nullptr;
		

		/*
		 * Path Calculation
		 */
		const FFMODListener& FMODListener = GetStudioModule().GetNearestListener(Transform.GetLocation());
		USoundManager* SoundManager = IsValid(World) ? World->GetSubsystem<USoundManager>() : nullptr;
		if (OcclusionType == EOcclusionType::OT_Angular || PropagationType == EPropagationType::PT_Portal)
		{
			if(SoundManager)
			{
				A->Initialize(EGraphNodeType::Listener, nullptr, Transform.GetLocation(), -1);
				RoomVolumeA = SoundManager->AddExternalNode(A);

				B->Initialize(EGraphNodeType::Listener, nullptr, FMODListener.Transform.GetLocation(), -2);
				RoomVolumeB = SoundManager->AddExternalNode(B);

				bool bSameRoom = (!RoomVolumeA && !RoomVolumeB) ? true : ( RoomVolumeA && RoomVolumeB && RoomVolumeA->RoomGroupID == RoomVolumeB->RoomGroupID && RoomVolumeA->RoomGroupID != -1);
				if(bSameRoom)
				{
					ShortestPath1 = {A, B};
				}
				else
				{
					ShortestPath1 = SoundManager->GetUnObstructedPath(A, B, RoomVolumeA, RoomVolumeB, false, {});
					if(ShortestPath1.Num() != 2 && ShortestPath1.Num() != 0)
					{
						ShortestPath2 = SoundManager->GetUnObstructedPath(A, B, RoomVolumeA, RoomVolumeB, false, {ShortestPath1[1]});
						UnobstructedPath = SoundManager->GetUnObstructedPath(A, B, RoomVolumeA, RoomVolumeB, true, {});
					}
				}
			}
		}
		
		/*
		 * Distance Calculation
		 */
		float FinalDistance = 0.0f;
		switch (PropagationType)
		{
		case EPropagationType::PT_None:
			FinalDistance = FVector::Distance(FMODListener.Transform.GetLocation(), Transform.GetLocation());
			break;
		case EPropagationType::PT_Portal:
			for (int i = 0; i < ShortestPath1.Num() - 1; i++)
			{
				FinalDistance += FVector::Distance(ShortestPath1[i]->Location, ShortestPath1[i + 1]->Location);
			}
			break;
		}

		/*
		 * Occlusion Calculation
		 */
		float FinalOcclusion = 0.0f;
		switch (OcclusionType)
		{
		case EOcclusionType::OT_None:
			break;
		case EOcclusionType::OT_Angular:
			{
				// Same Room
				if(ShortestPath1.Num() == 2)
				{
					float TotalDistance = 0.0f;
					float TotalOcclusion = 0.0f;
					GetPathOcclusionLength(ShortestPath1, &TotalDistance, &TotalOcclusion);
					
					if(RoomVolumeA == nullptr)
					{
						TotalOcclusion += (FVector::Distance(Transform.GetLocation(), FMODListener.Transform.GetLocation())/OutsidePathOcclusionDistance)*OutsideOcclusionMultiplier;
					}
					
					FinalOcclusion = TotalOcclusion;
				}
				// Different Room, Actual Path
				else
				{
					float ShortestPath1_Distance = 0.0f;
					float ShortestPath1_Occlusion = 0.0f;

					float ShortestPath2_Distance = 0.0f;
					float ShortestPath2_Occlusion = 0.0f;

					float UnobstructedPath_Distance = 0.0f;
					float UnobstructedPath_Occlusion = 0.0f;
					
					GetPathOcclusionLength(ShortestPath1, &ShortestPath1_Distance, &ShortestPath1_Occlusion);
					GetPathOcclusionLength(ShortestPath2, &ShortestPath2_Distance, &ShortestPath2_Occlusion);
					GetPathOcclusionLength(UnobstructedPath, &UnobstructedPath_Distance, &UnobstructedPath_Occlusion);

					float TotalWeight = 0.0f;

					float a = 1/(ShortestPath1_Distance*ShortestPath1_Distance);
					float b = 1/(ShortestPath2_Distance*ShortestPath2_Distance);
					float c = 1/(UnobstructedPath_Distance*UnobstructedPath_Distance);

					TotalWeight += a + b + c;

					FinalOcclusion += (a/TotalWeight)*ShortestPath1_Occlusion;
					FinalOcclusion += (b/TotalWeight)*ShortestPath2_Occlusion;
					FinalOcclusion += (c/TotalWeight)*UnobstructedPath_Occlusion;
				}

				SoundManager->RemoveExternalNode(A);
				SoundManager->RemoveExternalNode(B);

				// Small optimization, assume that portals won't have 3 lined up doors. L-P-P-P-S
				if(ShortestPath1.Num() >= 5)
				{
					FinalOcclusion += ObstructionOcclusionAdditive;
				}
				else if (IsValid(World))
				{
					// Add little bit of initial occlusion if the first trace has yet to be made, otherwise use value from last trace. Prevents occlusion cycling.
					InitiallyAddedOcclusion = bObstructedLastTrace ? ObstructionOcclusionAdditive : FMath::Clamp(FinalDistance/100, 0.0f , 50.0f)/50.0f * 0.2f;
					FinalOcclusion += InitiallyAddedOcclusion;

					// We want to ignore the root attached actor if we don't want inconsistent audio.
					FCollisionQueryParams QueryParams;
					if (AttachToComponent)
					{
						QueryParams.AddIgnoredActor(AttachToComponent->GetAttachmentRootActor());
					}
						
					World->AsyncLineTraceByChannel(EAsyncTraceType::Single, Transform.GetLocation(), FMODListener.Transform.GetLocation(), ECC_SOUND, QueryParams, FCollisionResponseParams::DefaultResponseParam, &TraceDelegate);
				}
			}
			break;
		case EOcclusionType::OT_Depth:
			FinalOcclusion = GetDepthMaterialOcclusionAmount(150, 1);
			break;
		}
		
		if (bDebugMode || CVarPrintSoundSourceDebug.GetValueOnAnyThread() != 0)
		{
			std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
			V_LOGM(LogReadyOrNot, "Sound Debug: %s", *this->GetName());
			if(Event)
				V_LOGM(LogReadyOrNot, "		Event: %s", *Event->GetName());
			V_LOGM(LogReadyOrNot, "		Calculation Time = %lld", std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count());
			V_LOGM(LogReadyOrNot, "		Distance: %f", FinalDistance/100);
			V_LOGM(LogReadyOrNot, "		Occlusion: %f", FinalOcclusion);
			V_LOGM(LogReadyOrNot, "		Location: %f %f %f", Transform.GetLocation().X, Transform.GetLocation().Y, Transform.GetLocation().Z);
			V_LOGM(LogReadyOrNot, "		Shortest Path Length:%d", ShortestPath1.Num());
			if(RoomVolumeA)
			{
				V_LOGM(LogReadyOrNot, "		Start Room Name: %s", *RoomVolumeA->GetName());
			}
			else
			{
				V_LOGM(LogReadyOrNot, "		Start Room Name: OUTSIDE");
			}
			if(RoomVolumeB)
			{
				V_LOGM(LogReadyOrNot, "		End Room Name: %s", *RoomVolumeB->GetName());
			}
			else
			{
				V_LOGM(LogReadyOrNot, "		End Room Name: OUTSIDE");
			}

#if WITH_EDITOR
			if (CVarShowSoundPath.GetValueOnAnyThread() != 0)
			{
				for(int i = 0; i < ShortestPath1.Num()-1; i++)
				{
					DrawDebugLine(World, ShortestPath1[i]->Location, ShortestPath1[i+1]->Location, FColor::Green, false, 1, 0, 2);
				}
				for(int i = 0; i < ShortestPath2.Num()-1; i++)
				{
					DrawDebugLine(World, ShortestPath2[i]->Location, ShortestPath2[i+1]->Location, FColor::Green, false, 1, 0, 2);
				}
				for(int i = 0; i < UnobstructedPath.Num()-1; i++)
				{
					DrawDebugLine(World, UnobstructedPath[i]->Location, UnobstructedPath[i+1]->Location, FColor::Orange, false, 1, 0, 2);
				}
			}
#endif
		} 

		FinalOcclusion = FinalOcclusion < 0 ? 0 : FinalOcclusion;
		
		ParameterCache.FindOrAdd("Occlusion") = FinalOcclusion;
		ParameterCache.FindOrAdd("PropagateDistance") = FinalDistance / 100;
		StudioInstance->setParameterByName("Occlusion", FinalOcclusion);
		StudioInstance->setParameterByName("PropagateDistance", FinalDistance / 100);
	}
}

void USoundSource::OnTraceCompleted(const FTraceHandle& Handle, FTraceDatum& Data)
{
	float* Occlusion = ParameterCache.Find("Occlusion");
	if (!Occlusion)
		return;
	
	float FinalOcclusion = *Occlusion;
	FinalOcclusion -= InitiallyAddedOcclusion;

	if(Data.OutHits.Num() > 0)
	{
		FinalOcclusion += ObstructionOcclusionAdditive;
	}
	bObstructedLastTrace = Data.OutHits.Num() > 0;
	
	ParameterCache.FindOrAdd("Occlusion") = FinalOcclusion;
	StudioInstance->setParameterByName("Occlusion", FinalOcclusion);
}

void USoundSource::GetPathOcclusionLength(TArray<TSharedPtr<FGraphNode>> Path, float* Distance, float* Occlusion)
{
	for (int i = 0; i < Path.Num() - 2; i++)
	{
		FVector L1 = Path[i]->Location;
		FVector L2 = Path[i + 1]->Location;
		FVector L3 = Path[i + 2]->Location;

		// Get the pivot portal
		APortalVolume* PortalVolume = Cast<APortalVolume>(Path[i+1]->Object);

		// Special case for sounds attached to doors. If the door happens to be 
		if(bIsDoorAttachedSound)
		{
			if(AttachToComponent && Cast<ADoor>(AttachToComponent->GetOwner()))
			{
				bool bShouldContinue = false;
				for(ADoor* Door : PortalVolume->Doors)
				{
					if(Door && Door == AttachToComponent->GetOwner())
					{
						bShouldContinue = true;
						*Occlusion += -100;
						break;
					}
				}
				if(bShouldContinue)
					continue;
			}
		}

		float DistanceToPortal = 0.0f;
		if (i == 0)
		{
			DistanceToPortal = ActorGetDistanceToCollision(PortalVolume, L1, L2);
		}
					
		// If we are touching the portal, make sure this portal doesn't occlude.
		float AdditionalBlockageOcclusion = 0.0f;
		if (PortalVolume && PortalVolume->AttachedObjects.Num() > 0)
		{
			bool bAllClosed = true;
			for (ADoor* Door : PortalVolume->Doors)
			{
				if (Door && Door->IsOpen())
				{
					bAllClosed = false;
					break;
				}
			}
			AdditionalBlockageOcclusion += bAllClosed? DoorOcclusionAdditive : 0;
		}
		
		if (i == 0 && (AdditionalBlockageOcclusion == 0.0f || bIsDoorAttachedSound))
		{
			if (DistanceToPortal <= 0.0f)
			{
				continue;
			}
		}

		FVector V1 = L2 - L1;
		FVector V2 = L3 - L2;
		V1.Normalize();
		V2.Normalize();
		
		*Occlusion += (1 - ((FMath::Clamp(FVector::DotProduct(V1, V2), -1.0f, 1.0f) + 1) / 2))*AngleOcclusionMultiplier + AdditionalBlockageOcclusion;
	}

	// Calculates distance and outside occlusion addition.
	for (int i = 0; i < Path.Num() - 1; i++)
	{
		float Dist = FVector::Distance(Path[i]->Location, Path[i + 1]->Location);
		*Distance += Dist;

		// Outside distance occlusion
		if(Path[i]->NodeType == EGraphNodeType::Portal && Path[i+1]->NodeType == EGraphNodeType::Portal)
		{
			if(Cast<APortalVolume>(Path[i]->Object)->bIsOutside && Cast<APortalVolume>(Path[i+1]->Object)->bIsOutside)
			{
				*Occlusion += (Dist/OutsidePathOcclusionDistance)*OutsideOcclusionMultiplier;
			}
		}
	}
}

float USoundSource::ActorGetDistanceToCollision(AActor* Actor, const FVector Point, FVector& ClosestPointOnCollision)
{
	ClosestPointOnCollision = Point;
	float ClosestPointDistanceSqr = -1.f;

	TInlineComponentArray<UPrimitiveComponent*> Components;
	Actor->GetComponents(Components);

	for (int32 ComponentIndex = 0; ComponentIndex < Components.Num(); ComponentIndex++)
	{
		UPrimitiveComponent* Primitive = Components[ComponentIndex];
		if (Primitive->IsRegistered())
		{
			FVector ClosestPoint;
			float DistanceSqr = -1.f;

			if (!Primitive->GetSquaredDistanceToCollision(Point, DistanceSqr, ClosestPoint))
			{
				// Invalid result, impossible to be better than ClosestPointDistance
				continue;
			}

			if ((ClosestPointDistanceSqr < 0.f) || (DistanceSqr < ClosestPointDistanceSqr))
			{
				ClosestPointDistanceSqr = DistanceSqr;
				ClosestPointOnCollision = ClosestPoint;

				// If we're inside collision, we're not going to find anything better, so abort search we've got our best find.
				if (DistanceSqr <= KINDA_SMALL_NUMBER)
				{
					break;
				}
			}
		}
	}
	return (ClosestPointDistanceSqr > 0.f ? FMath::Sqrt(ClosestPointDistanceSqr) : ClosestPointDistanceSqr);
}

FMOD_RESULT F_CALLBACK USoundSource_EventCallback(FMOD_STUDIO_EVENT_CALLBACK_TYPE type, FMOD_STUDIO_EVENTINSTANCE *event, void *parameters)
{
	USoundSource* Component = nullptr;
	FMOD::Studio::EventInstance *Instance = (FMOD::Studio::EventInstance *)event;
	if (Instance->getUserData((void **)&Component) == FMOD_OK && IsValid(Component))
	{
		if (type == FMOD_STUDIO_EVENT_CALLBACK_TIMELINE_MARKER && Component->bEnableTimelineCallbacks)
		{
			Component->EventCallbackAddMarker((FMOD_STUDIO_TIMELINE_MARKER_PROPERTIES *)parameters);
		}
		else if (type == FMOD_STUDIO_EVENT_CALLBACK_TIMELINE_BEAT && Component->bEnableTimelineCallbacks)
		{
			Component->EventCallbackAddBeat((FMOD_STUDIO_TIMELINE_BEAT_PROPERTIES *)parameters);
		}
		else if (type == FMOD_STUDIO_EVENT_CALLBACK_CREATE_PROGRAMMER_SOUND)
		{
			Component->EventCallbackCreateProgrammerSound((FMOD_STUDIO_PROGRAMMER_SOUND_PROPERTIES *)parameters);
		}
		else if (type == FMOD_STUDIO_EVENT_CALLBACK_DESTROY_PROGRAMMER_SOUND)
		{
			Component->EventCallbackDestroyProgrammerSound((FMOD_STUDIO_PROGRAMMER_SOUND_PROPERTIES *)parameters);
		}
		else if (type == FMOD_STUDIO_EVENT_CALLBACK_SOUND_PLAYED)
		{
			Component->EventCallbackSoundPlayed();
		}
		else if (type == FMOD_STUDIO_EVENT_CALLBACK_SOUND_STOPPED)
		{
			Component->EventCallbackSoundStopped();
		}
	}
	return FMOD_OK;
}

void USoundSource::Play()
{
	USoundManager* SoundManager = World ? World->GetSubsystem<USoundManager>() : nullptr;
	if(!SoundManager || (SoundManager && !SoundManager->bHasFinishedSetup))
	{
		bPlayLater = true;
		return;
	}
	
	bIsRunning = true;
	PlayInternal();
}

void USoundSource::PlayInternal()
{
	if (!FMODUtils::IsWorldAudible(World, EFMODSystemContext::Max == EFMODSystemContext::Editor))
	{
		return;
	}

	// Only play events in PIE/game, not when placing them in the editor
	FMOD::Studio::EventDescription* EventDesc = GetStudioModule().GetEventDescription(Event, EFMODSystemContext::Max);
	if (EventDesc != nullptr)
	{
		EventDesc->getLength(&EventLength);
		if (!StudioInstance || !StudioInstance->isValid())
		{
			FMOD_RESULT result = EventDesc->createInstance(&StudioInstance);
			if (result != FMOD_OK)
				return;
		}

		UpdateTransform();
		UpdateTickData();

		const UFMODSettings& Settings = *GetDefault<UFMODSettings>();
		FMOD_STUDIO_PARAMETER_DESCRIPTION paramDesc = {};
		FString param = Settings.OcclusionParameter;
		if (!param.IsEmpty())
		{
			if (EventDesc->getParameterDescriptionByName(TCHAR_TO_UTF8(*Settings.OcclusionParameter), &paramDesc) ==
				FMOD_OK)
			{
				// OcclusionID = paramDesc.id;
				// bApplyOcclusionParameter = true;
			}
		}

		paramDesc = {};
		param = Settings.AmbientVolumeParameter;
		if (!param.IsEmpty())
		{
			if (EventDesc->getParameterDescriptionByName(TCHAR_TO_UTF8(*param), &paramDesc) == FMOD_OK)
			{
				// AmbientVolumeID = paramDesc.id;
				// LastVolume = -1.0f;     // Invalidate LastVolume so the AmbientVolumeParameter of the Event will be set later on
				// bApplyAmbientVolumes = true;
			}
		}

		paramDesc = {};
		param = Settings.AmbientLPFParameter;
		if (!param.IsEmpty())
		{
			if (EventDesc->getParameterDescriptionByName(TCHAR_TO_UTF8(*Settings.AmbientLPFParameter), &paramDesc) ==
				FMOD_OK)
			{
				// AmbientLPFID = paramDesc.id;
				// LastLPF = -1.0f;     // Invalidate LastLPF so the AmbientLPFParameter of the Event will be set later on
				// bApplyAmbientVolumes = true;
			}
		}
		// Set initial parameters
		for (auto Kvp : ParameterCache)
		{
			FMOD_RESULT Result = StudioInstance->setParameterByName(TCHAR_TO_UTF8(*Kvp.Key.ToString()), Kvp.Value);
			if (Result != FMOD_OK)
			{
				UE_LOG(LogReadyOrNotAudio, Warning, TEXT("Failed to set initial parameter %s"), *Kvp.Key.ToString());
			}
		}
		for (int i = 0; i < EFMODEventProperty::Count; ++i)
		{
			if (StoredProperties[i] != -1.0f)
			{
				FMOD_RESULT Result = StudioInstance->setProperty((FMOD_STUDIO_EVENT_PROPERTY)i, StoredProperties[i]);
				if (Result != FMOD_OK)
				{
					UE_LOG(LogReadyOrNot, Warning, TEXT("Failed to set initial property %d"), i);
				}
			}
		}

		if (bEnableTimelineCallbacks || !ProgrammerSoundName.IsEmpty())
		{
			verifyfmod(StudioInstance->setCallback(USoundSource_EventCallback));
		}

		verifyfmod(StudioInstance->setUserData(this));
		verifyfmod(StudioInstance->start());
		
		UE_LOG(LogReadyOrNot, Verbose, TEXT("Playing component %p"), this);
	}
}

void USoundSource::SetParameter(FName Name, float Value)
{
	if (StudioInstance)
	{
		FMOD_RESULT Result = StudioInstance->setParameterByName(TCHAR_TO_UTF8(*Name.ToString()), Value);
		if (Result != FMOD_OK)
		{
			UE_LOG(LogReadyOrNot, Warning, TEXT("Failed to set parameter %s"), *Name.ToString());
		}
	}
	ParameterCache.FindOrAdd(Name) = Value;
}

void USoundSource::SetPaused(bool Paused)
{
	if (StudioInstance)
	{
		FMOD_RESULT Result = StudioInstance->setPaused(Paused);
		bIsPaused = Paused;
		if (Result != FMOD_OK)
		{
			UE_LOG(LogReadyOrNotAudio, Warning, TEXT("Failed to pause"));
		}

		// When we unpause, we want to update the tick data of the SoundSource.
		if(!bIsPaused)
		{
			UpdateTransform();
			UpdateTickData();
		}
	}
}

void USoundSource::SetPropagationTickInterval(float Interval)
{
	PropagationUpdateInterval = Interval;
}

void USoundSource::SetPrimaryTickInterval(float Interval)
{
	PrimaryUpdateInterval = Interval;
}

void USoundSource::Stop()
{
	if (StudioInstance)
	{
		if (StudioInstance->isValid())
		{
			bIsRunning = false;
			StudioInstance->stop(FMOD_STUDIO_STOP_ALLOWFADEOUT);
		}
	}
}

void USoundSource::OnPlaybackCompleted()
{
	UWorld* CurrentWorld = World;
	ResetSoundSource();
	
	USoundManager* SoundManager = CurrentWorld ? CurrentWorld->GetSubsystem<USoundManager>() : nullptr;
	if (SoundManager)
	{
		SoundManager->InactiveSoundSources.AddUnique(this);
		SoundManager->ActiveSoundSources.Remove(this);
	}
	
	// Ensure the SoundSource has detached itself from the child or parent.
	switch (HierarchyType)
	{
	case EHierarchyType::HT_Child:
		if(ParentSoundSource)
		{
			if(ParentSoundSource->bIsActive && ParentSoundSource->InstanceGuid == this->InstanceGuid)
			{
				ParentSoundSource->ChildrenSoundSources.Remove(this);
			}
		}
		break;
	case EHierarchyType::HT_Parent:
		for(USoundSource* Child : ChildrenSoundSources)
		{
			if(Child && Child->bIsActive && Child->InstanceGuid == this->InstanceGuid)
			{
				Child->HierarchyType = EHierarchyType::HT_Default;
				Child->ParentSoundSource = nullptr;
			}
		}
		break;
	case EHierarchyType::HT_Default:
		break;
	default: ;
	}
}

void USoundSource::AddChild(USoundSource* Child)
{
	ChildrenSoundSources.Add(Child);
	this->HierarchyType = EHierarchyType::HT_Parent;
	Child->HierarchyType = EHierarchyType::HT_Child;
	Child->InstanceGuid = this->InstanceGuid;
}

void USoundSource_ReleaseProgrammerSound(FMOD_STUDIO_PROGRAMMER_SOUND_PROPERTIES *props)
{
	if (props->sound)
	{
		UE_LOG(LogReadyOrNot, Verbose, TEXT("Destroying programmer sound"));
		FMOD_RESULT Result = ((FMOD::Sound *)props->sound)->release();
		verifyfmod(Result);
	}
}

FMOD_RESULT F_CALLBACK USoundSource_EventCallbackDestroyProgrammerSound(FMOD_STUDIO_EVENT_CALLBACK_TYPE type, FMOD_STUDIO_EVENTINSTANCE *event, void *parameters)
{
	USoundSource_ReleaseProgrammerSound((FMOD_STUDIO_PROGRAMMER_SOUND_PROPERTIES *)parameters);
	return FMOD_OK;
}

void USoundSource::ReleaseEventInstance()
{
	if (StudioInstance->isValid())
	{
		
		if (NeedDestroyProgrammerSoundCallback)
		{
			// We need a callback to destroy a programmer sound
			StudioInstance->setCallback(USoundSource_EventCallbackDestroyProgrammerSound, FMOD_STUDIO_EVENT_CALLBACK_DESTROY_PROGRAMMER_SOUND);
		}
		else
		{
			// We don't want any more callbacks
			StudioInstance->setCallback(nullptr);
		}
		
		StudioInstance->setCallback(nullptr);
		StudioInstance->release();
		StudioInstance = nullptr;
	}
}

void USoundSource::UpdateTransform()
{
	if (AttachToComponent)
	{
		if (AttachPointName != "")
		{
			Transform = AttachToComponent->GetSocketTransform(AttachPointName);
		}
		else
		{
			Transform = AttachToComponent->GetComponentTransform();
		}
	}

	if (StudioInstance)
	{
		FMOD_3D_ATTRIBUTES attr = {{0}};
		attr.position = FMODUtils::ConvertWorldVector(Transform.GetLocation());
		attr.up = FMODUtils::ConvertUnitVector(Transform.GetUnitAxis(EAxis::Z));
		attr.forward = FMODUtils::ConvertUnitVector(Transform.GetUnitAxis(EAxis::X));

		if (AttachToComponent)
		{
			attr.velocity = FMODUtils::ConvertWorldVector(AttachToComponent->GetComponentVelocity());
		}
		else
		{
			attr.velocity = FMODUtils::ConvertWorldVector(FVector(0, 0, 0));
		}

		StudioInstance->set3DAttributes(&attr);
	}
}

void USoundSource::CacheDefaultParameterValues()
{
	if (Event)
	{
		const UFMODSettings& Settings = *GetDefault<UFMODSettings>();
		TArray<FMOD_STUDIO_PARAMETER_DESCRIPTION> ParameterDescriptions;
		Event->GetParameterDescriptions(ParameterDescriptions);
		for (const FMOD_STUDIO_PARAMETER_DESCRIPTION& ParameterDescription : ParameterDescriptions)
		{
			if (!ParameterCache.Find(ParameterDescription.name) &&
				((ParameterDescription.flags & FMOD_STUDIO_PARAMETER_GLOBAL) == 0) &&
				(ParameterDescription.type == FMOD_STUDIO_PARAMETER_GAME_CONTROLLED) &&
				ParameterDescription.name != Settings.OcclusionParameter &&
				ParameterDescription.name != Settings.AmbientVolumeParameter &&
				ParameterDescription.name != Settings.AmbientLPFParameter)
			{
				ParameterCache.Add(ParameterDescription.name, ParameterDescription.defaultvalue);
			}
		}
	}
}

float USoundSource::GetDepthMaterialOcclusionAmount(float DefaultOcclusionDepth, float OcclusionMultiplier)
{
	const FFMODListener& FMODListener = GetStudioModule().GetNearestListener(Transform.GetLocation());

	TArray<FHitResult> ForwardHits;
	TArray<FHitResult> ReverseHits;

	FCollisionQueryParams CollisionQueryParams;
	CollisionQueryParams.bReturnPhysicalMaterial = true;
	CollisionQueryParams.bTraceComplex = true;

	APlayerCharacter* LocalPlayerCharacter = UBpGameplayHelperLib::GetLocalPlayerCharacter(World);
	if(LocalPlayerCharacter){
		CollisionQueryParams.AddIgnoredActor(LocalPlayerCharacter);
		CollisionQueryParams.AddIgnoredActors((TArray<AActor*>)LocalPlayerCharacter->GetInventoryComponent()->GetInventoryItems());
	}	
	
	World->LineTraceMultiByChannel(ForwardHits, Transform.GetLocation(), FMODListener.Transform.GetLocation(),
	                                    ECC_OCCLUSION, CollisionQueryParams);
	World->LineTraceMultiByChannel(ReverseHits, FMODListener.Transform.GetLocation(), Transform.GetLocation(),
	                                    ECC_OCCLUSION, CollisionQueryParams);

	// Add hitresults to component map
	TMap<UActorComponent*, TArray<FHitResult>> CompToIndex;
	for (int i = 0; i < ForwardHits.Num(); i++)
	{
		FHitResult ForwardHitResult = ForwardHits[i];
		if (CompToIndex.Contains(ForwardHitResult.GetComponent()))
		{
			CompToIndex.Find(ForwardHitResult.GetComponent())->Add(ForwardHitResult);
		}
		else
		{
			CompToIndex.Add(ForwardHitResult.GetComponent(), TArray<FHitResult>{ForwardHitResult});
		}
	}
	for (int i = 0; i < ReverseHits.Num(); i++)
	{
		FHitResult ReverseHitResult = ReverseHits[i];
		if (CompToIndex.Contains(ReverseHitResult.GetComponent()))
		{
			CompToIndex.Find(ReverseHitResult.GetComponent())->Add(ReverseHitResult);
		}
		else
		{
			CompToIndex.Add(ReverseHitResult.GetComponent(), TArray<FHitResult>{ReverseHitResult});
		}
	}

	float NewOcclusionAmount = 0.0f;
	// For every pair of hits
	for (auto It = CompToIndex.CreateConstIterator(); It; ++It)
	{
		// Make sure we have two hits.
		if (It.Value().Num() == 2)
		{
			FHitResult ForwardHitResult = It.Value()[0];
			FHitResult ReverseHitResult = It.Value()[1];

			// Check for specific actor tags
			if (ForwardHitResult.GetActor())
			{
				// If the actor shouldn't occlude.
				if (ForwardHitResult.GetActor()->ActorHasTag("NoOcclusion"))
				{
					continue;
				}

				// If the actor should not let any gunshot sound through.
				if (ForwardHitResult.GetActor()->ActorHasTag("NoSound"))
				{
					return -1;
				}
			}

			// If we hit a door, we should occlude more.
			float NewDoorMultiplier = 1.0f;
			if (ForwardHitResult.GetActor())
			{
				if (ADoor* DoorActor = Cast<ADoor>(ForwardHitResult.GetActor()))
				{
					NewDoorMultiplier = DoorActor->OcclusionMultiplier;
				}
			}

			// Calculate object depth based on reverse and forward hits
			float ObjectDepth = FVector::Distance(ForwardHitResult.Location, ReverseHitResult.Location);

			// Get custom physmat if it exists
			UPhysicalMaterial* PhysicalMaterial = ForwardHitResult.PhysMaterial.Get();
			UCustomPhysicalMaterial* CustomPhysicalMaterial = Cast<UCustomPhysicalMaterial>(PhysicalMaterial);

			// If there is a custom physmat, get the occlusion params.
			if (CustomPhysicalMaterial)
			{
				NewOcclusionAmount += (ObjectDepth / CustomPhysicalMaterial->FullOcclusionDepth) * OcclusionMultiplier *
					NewDoorMultiplier;
			}
			// Default occlusion params.
			else
			{
				NewOcclusionAmount += (ObjectDepth / DefaultOcclusionDepth) * OcclusionMultiplier * NewDoorMultiplier;
			}
		}
		// If we only have one hit, we hit something one sided.
		else
		{
			NewOcclusionAmount += 0.30f * OcclusionMultiplier;
		}
	}
	NewOcclusionAmount = FMath::Clamp(NewOcclusionAmount, -1.0f, 1.0f);
	return NewOcclusionAmount;
}

void USoundSource::Attach(USceneComponent* InAttachToComponent, FName InAttachPointName)
{
	AttachToComponent = InAttachToComponent;
	AttachPointName = InAttachPointName;
}

void USoundSource::Detach()
{
	AttachToComponent = nullptr;
	AttachPointName = "";
}

void USoundSource::SetOcclusionType(TEnumAsByte<EOcclusionType> InOcclusionType)
{
	OcclusionType = InOcclusionType;
}

void USoundSource::SetPropagationType(TEnumAsByte<EPropagationType> InPropagationType)
{
	PropagationType = InPropagationType;
}

void USoundSource::SetDebugMode(bool InbDebugMode)
{
	bDebugMode = InbDebugMode;
}

float USoundSource::GetDoorOcclusionAdditive() const
{
	return DoorOcclusionAdditive;
}

void USoundSource::SetDoorOcclusionAdditive(float InDoorOcclusionAdditive)
{
	this->DoorOcclusionAdditive = InDoorOcclusionAdditive;
}

float USoundSource::GetOutsidePathOcclusionDistance() const
{
	return OutsidePathOcclusionDistance;
}

void USoundSource::SetOutsidePathOcclusionDistance(float InOutsidePathOcclusionDistance)
{
	this->OutsidePathOcclusionDistance = InOutsidePathOcclusionDistance;
}

float USoundSource::GetOutsideOcclusionMultiplier() const
{
	return OutsideOcclusionMultiplier;
}

void USoundSource::SetOutsideOcclusionMultiplier(float InOutsideOcclusionMultiplier)
{
	this->OutsideOcclusionMultiplier = InOutsideOcclusionMultiplier;
}

float USoundSource::GetAngleOcclusionMultiplier() const
{
	return AngleOcclusionMultiplier;
}

void USoundSource::SetAngleOcclusionMultiplier(float InAngleOcclusionMultiplier)
{
	this->AngleOcclusionMultiplier = InAngleOcclusionMultiplier;
}

float USoundSource::GetObstructionOcclusionAdditive() const
{
	return ObstructionOcclusionAdditive;
}

void USoundSource::SetObstructionOcclusionAdditive(float InObstructionOcclusionAdditive)
{
	this->ObstructionOcclusionAdditive = InObstructionOcclusionAdditive;
}

void USoundSource::SetProgrammerSoundName(FString Value)
{
	FScopeLock Lock(&CallbackLock);
	ProgrammerSoundName = Value;
}

void USoundSource::SetProgrammerSound(FMOD::Sound *Sound)
{
	FScopeLock Lock(&CallbackLock);
	ProgrammerSound = Sound;
}

void USoundSource::EventCallbackDestroyProgrammerSound(FMOD_STUDIO_PROGRAMMER_SOUND_PROPERTIES *props)
{
	if (NeedDestroyProgrammerSoundCallback)
	{
		USoundSource_ReleaseProgrammerSound(props);
		NeedDestroyProgrammerSoundCallback = false;
	}
}

void USoundSource::EventCallbackCreateProgrammerSound(FMOD_STUDIO_PROGRAMMER_SOUND_PROPERTIES *props)
{
    // Make sure name isn't being changed as we are reading it
    FString ProgrammerSoundNameCopy;
    {
        FScopeLock Lock(&CallbackLock);
        ProgrammerSoundNameCopy = ProgrammerSoundName;
    }

    if (ProgrammerSound)
    {
        props->sound = (FMOD_SOUND *)ProgrammerSound;
        props->subsoundIndex = -1;
    }
    else if (ProgrammerSoundNameCopy.Len() || strlen(props->name) != 0)
    {
        FMOD::Studio::System *System = GetStudioModule().GetStudioSystem(EFMODSystemContext::Max);
        FMOD::System *LowLevelSystem = nullptr;
        System->getCoreSystem(&LowLevelSystem);
        FString SoundName = ProgrammerSoundNameCopy.Len() ? ProgrammerSoundNameCopy : UTF8_TO_TCHAR(props->name);
        FMOD_MODE SoundMode = FMOD_LOOP_NORMAL | FMOD_CREATECOMPRESSEDSAMPLE | FMOD_NONBLOCKING;

        if (SoundName.StartsWith(TEXT("http://")) || 
            SoundName.StartsWith(TEXT("http:\\\\")) || 
            SoundName.StartsWith(TEXT("https://")) || 
            SoundName.StartsWith(TEXT("https:\\\\")))
        {
            // Load via url
            FMOD::Sound* Sound = nullptr;
            FMOD_CREATESOUNDEXINFO exinfo;
            memset(&exinfo, 0, sizeof(FMOD_CREATESOUNDEXINFO));
            exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
            exinfo.filebuffersize = 1024 * 16;
            exinfo.ignoresetfilesystem = true;

            if (LowLevelSystem->createSound(TCHAR_TO_UTF8(*SoundName), FMOD_CREATESTREAM | FMOD_NONBLOCKING, &exinfo, &Sound) == FMOD_OK)
            {
                UE_LOG(LogReadyOrNot, Verbose, TEXT("Creating programmer sound from url '%s'"), *SoundName);
                props->sound = (FMOD_SOUND*)Sound;
                props->subsoundIndex = -1;
                NeedDestroyProgrammerSoundCallback = true;
            }
            else
            {
                UE_LOG(LogReadyOrNot, Warning, TEXT("Failed to load programmer sound url '%s'"), *SoundName);
            }
        }
        else if (SoundName.Contains(TEXT(".")))
        {
#if defined(TARGET_PS4) || defined(TARGET_PS5) || defined(TARGET_XBOXONE) || defined(TARGET_XSX)
            // Load via file on console
            FString SoundPath = SoundName;
            FMOD::Sound *Sound = nullptr;

        	// SoundPath = "../../../readyornot/content/vo_at9/agencycivilian_01/test.fsb";
			// D:\RON\Ready Or Not\Saved\StagedBuilds\PS5\readyornot\content\vo_at9\AgencyCivilian_01
			auto result = (LowLevelSystem->createSound(TCHAR_TO_UTF8(*SoundPath), SoundMode,0, &Sound));
            if (result == FMOD_OK)
            {
            	FMOD_SOUND_TYPE type;
            	FMOD_SOUND_FORMAT format;
            	int channels;
            	int bits;

            	int numsubsounds;
            	Sound->getNumSubSounds(&numsubsounds);

            	// get subsound to play from FSB file
            	FMOD::Sound *SoundToPlay = nullptr;
             	Sound->getSubSound(0, &SoundToPlay);
				SoundToPlay->getFormat(&type,&format, &channels, &bits);
	            UE_LOG(LogReadyOrNot, Warning, TEXT("Creating programmer sound from file '%s' type:%d, format:%d, channels:%d, bits:%d"), *SoundPath,type,format,channels,bits);
                props->sound = (FMOD_SOUND *)Sound;
                props->subsoundIndex = 0;
                NeedDestroyProgrammerSoundCallback = true;

            	ProgrammerSoundInstance = Sound;
            }
            else
            {
                UE_LOG(LogReadyOrNot, Warning, TEXT("Failed to load programmer sound file '%s', result:%d"), *SoundPath,(int) result);
            }
#else
        	// Load via file
        	FString SoundPath = SoundName;
        	if (FPaths::IsRelative(SoundPath))
        	{
        		SoundPath = FPaths::ProjectContentDir() / SoundPath;
        	}

        	FMOD::Sound *Sound = nullptr;
        	if (LowLevelSystem->createSound(TCHAR_TO_UTF8(*SoundPath), SoundMode, nullptr, &Sound) == FMOD_OK)
        	{
        		UE_LOG(LogReadyOrNot, Verbose, TEXT("Creating programmer sound from file '%s'"), *SoundPath);
        		props->sound = (FMOD_SOUND *)Sound;
        		props->subsoundIndex = -1;
        		NeedDestroyProgrammerSoundCallback = true;

        		ProgrammerSoundInstance = Sound;
        	}
        	else
        	{
        		UE_LOG(LogReadyOrNot, Warning, TEXT("Failed to load programmer sound file '%s'"), *SoundPath);
        	}
#endif        	
        }
        else
        {
            // Load via FMOD Studio asset table
            FMOD_STUDIO_SOUND_INFO SoundInfo = { 0 };
            FMOD_RESULT Result = System->getSoundInfo(TCHAR_TO_UTF8(*SoundName), &SoundInfo);
            if (Result == FMOD_OK)
            {
                FMOD::Sound *Sound = nullptr;
                Result = LowLevelSystem->createSound(SoundInfo.name_or_data, SoundMode | SoundInfo.mode, &SoundInfo.exinfo, &Sound);
                if (Result == FMOD_OK)
                {
                    UE_LOG(LogReadyOrNot, Verbose, TEXT("Creating programmer sound using audio entry '%s'"), *SoundName);
                	
                    props->sound = (FMOD_SOUND *)Sound;
                    props->subsoundIndex = SoundInfo.subsoundindex;
                    NeedDestroyProgrammerSoundCallback = true;
                }
                else
                {
                    UE_LOG(LogReadyOrNot, Warning, TEXT("Failed to load FMOD audio entry '%s'"), *SoundName);
                }
            }
            else
            {
                UE_LOG(LogReadyOrNot, Warning, TEXT("Failed to find FMOD audio entry '%s'"), *SoundName);
            }
        }
    }
}

void USoundSource::EventCallbackAddMarker(FMOD_STUDIO_TIMELINE_MARKER_PROPERTIES *props)
{
	FScopeLock Lock(&CallbackLock);
	FTimelineMarkerProperties info;
	info.Name = props->name;
	info.Position = props->position;
	CallbackMarkerQueue.Push(info);
}

void USoundSource::EventCallbackAddBeat(FMOD_STUDIO_TIMELINE_BEAT_PROPERTIES *props)
{
	FScopeLock Lock(&CallbackLock);
	FTimelineBeatProperties info;
	info.Bar = props->bar;
	info.Beat = props->beat;
	info.Position = props->position;
	info.Tempo = props->tempo;
	info.TimeSignatureUpper = props->timesignatureupper;
	info.TimeSignatureLower = props->timesignaturelower;
	CallbackBeatQueue.Push(info);
}

void USoundSource::EventCallbackSoundPlayed()
{
	FScopeLock Lock(&CallbackLock);
	if (ProgrammerSoundInstance && bWantsProgrammerSoundLength)
	{
		bTriggerProgrammerSoundPlayed = true;
	}
}

void USoundSource::EventCallbackSoundStopped()
{
	FScopeLock Lock(&CallbackLock);
	TriggerSoundStoppedDelegate = true;
}

float USoundSource::GetCurrentSoundLength() const
{
	FMOD_STUDIO_SOUND_INFO SoundInfo = {};
	const FMOD_RESULT Result = Module->GetStudioSystem(EFMODSystemContext::Max)->getSoundInfo(TCHAR_TO_UTF8(*ProgrammerSoundName), &SoundInfo);
	if (Result == FMOD_OK)
	{
		return (float)SoundInfo.exinfo.length / 1000.0f;
	}
	return 0.0f;
};

void USoundSource::ReplaceAnimNotifies(UAnimSequenceBase* AnimationSequence)
{
	TSubclassOf<USoundSourceAnimNotify> NewNotifyClass = USoundSourceAnimNotify::StaticClass();
	TSubclassOf<UFMODAnimNotifyPlay> OldNotifyClass = UFMODAnimNotifyPlay::StaticClass();
	
	if (AnimationSequence)
	{
		if (OldNotifyClass != nullptr && NewNotifyClass != nullptr)
		{
			bool bModified = false;
			for(int32 NotifyIndex = 0; NotifyIndex < AnimationSequence->Notifies.Num(); ++NotifyIndex)
			{
				FAnimNotifyEvent& NotifyEvent = AnimationSequence->Notifies[NotifyIndex];

				if (NotifyEvent.Notify && NotifyEvent.Notify->GetClass() == OldNotifyClass)
				{

					bModified = true;

					// Copy relevant data from the old notify
					float StartTime = NotifyEvent.GetTime();
					float Length = NotifyEvent.GetDuration();
					int32 TargetTrackIndex = NotifyEvent.TrackIndex;
					float TriggerTimeOffset = NotifyEvent.TriggerTimeOffset;
					float EndTriggerTimeOffset = NotifyEvent.EndTriggerTimeOffset;
					int32 SlotIndex = NotifyEvent.GetSlotIndex();
					int32 EndSlotIndex = NotifyEvent.EndLink.GetSlotIndex();
					int32 SegmentIndex = NotifyEvent.GetSegmentIndex();
					int32 EndSegmentIndex = NotifyEvent.GetSegmentIndex();
					EAnimLinkMethod::Type LinkMethod = NotifyEvent.GetLinkMethod();
					EAnimLinkMethod::Type EndLinkMethod = NotifyEvent.EndLink.GetLinkMethod();
					UAnimNotify* OldNotify = NotifyEvent.Notify;
					UAnimNotifyState* OldNotifyState = NotifyEvent.NotifyStateClass;

					// Remove old notify
					AnimationSequence->Notifies.RemoveAt(NotifyIndex, 1, false);

					// Add new notify in old notifies place
					AnimationSequence->Notifies.InsertDefaulted(NotifyIndex);
					FAnimNotifyEvent& NewEvent = AnimationSequence->Notifies[NotifyIndex];

					// Setup new notify
					NewEvent.NotifyName = NAME_None;
					NewEvent.Link(AnimationSequence, StartTime);
					NewEvent.TriggerTimeOffset = TriggerTimeOffset;
					NewEvent.TrackIndex = TargetTrackIndex;
					NewEvent.ChangeSlotIndex(SlotIndex);
					NewEvent.SetSegmentIndex(SegmentIndex);
					NewEvent.ChangeLinkMethod(LinkMethod);

					UObject* AnimNotifyClass = NewObject<UObject>(AnimationSequence, NewNotifyClass, NAME_None, RF_Transactional);
					NewEvent.NotifyStateClass = Cast<UAnimNotifyState>(AnimNotifyClass);
					NewEvent.Notify = Cast<UAnimNotify>(AnimNotifyClass);

					Cast<USoundSourceAnimNotify>(NewEvent.Notify)->Event = Cast<UFMODAnimNotifyPlay>(OldNotify)->Event;
					Cast<USoundSourceAnimNotify>(NewEvent.Notify)->AttachName = Cast<UFMODAnimNotifyPlay>(OldNotify)->AttachName;
					Cast<USoundSourceAnimNotify>(NewEvent.Notify)->bFollow = Cast<UFMODAnimNotifyPlay>(OldNotify)->bFollow;
					#if WITH_EDITOR
					NewEvent.Notify->NotifyColor = FColor(240, 138, 76);
					#endif
					
					// Setup name and duration for new event
					if (NewEvent.NotifyStateClass)
					{
						NewEvent.NotifyName = FName(*NewEvent.NotifyStateClass->GetNotifyName());
						NewEvent.EndTriggerTimeOffset = EndTriggerTimeOffset;
						NewEvent.EndLink.ChangeSlotIndex(EndSlotIndex);
						NewEvent.EndLink.SetSegmentIndex(EndSegmentIndex);
						NewEvent.EndLink.ChangeLinkMethod(EndLinkMethod);
					}
					else if(NewEvent.Notify)
					{
						NewEvent.NotifyName = FName(*NewEvent.Notify->GetNotifyName());
					}

					NewEvent.Update();
				}
			}

			if(bModified)
			{
				// Refresh all cached data
				AnimationSequence->MarkPackageDirty();
				AnimationSequence->RefreshCacheData();
			}
		}
		else
		{
			UE_LOG(LogReadyOrNot, Warning, TEXT("Invalid Notify Class for ReplaceAnimNotifies"));
		}
	}
	else
	{
		UE_LOG(LogReadyOrNot, Warning, TEXT("Invalid Animation Sequence for ReplaceAnimNotifies"));
	}
}