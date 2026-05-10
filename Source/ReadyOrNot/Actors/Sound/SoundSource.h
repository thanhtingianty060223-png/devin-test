// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "GraphNode.h"
#include "Components/FMODAudioPropagationComponent.h"
#include "UObject/Object.h"
#include "SoundSource.generated.h"

UENUM(BlueprintType)
enum class ESoundSourceType : uint8
{
	SST_FirstPerson,
	SST_ThirdPerson
};

UENUM(BlueprintType)
enum class EOcclusionType : uint8
{
	OT_None UMETA(DisplayName="No Occlusion"),
	OT_Depth UMETA(DisplayName="Depth-based Occlusion"),
	OT_Angular UMETA(DisplayName="Angular Occlusion")
};

UENUM(BlueprintType)
enum class EPropagationType : uint8
{
	PT_None UMETA(DisplayName="No Propagation"),
	PT_Portal UMETA(DisplayName="Portal Propagation")
};

UENUM(BlueprintType)
enum class EHierarchyType : uint8
{
	HT_Default,
	HT_Parent,
	HT_Child
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSoundFinished);

UCLASS()
class READYORNOT_API USoundSource : public UObject, public FTickableGameObject
{
	GENERATED_BODY()

	friend FMOD_RESULT F_CALLBACK USoundSource_EventCallback(FMOD_STUDIO_EVENT_CALLBACK_TYPE type, FMOD_STUDIO_EVENTINSTANCE *event, void *parameters);
	
	public:
		USoundSource();

		virtual void BeginDestroy() override;
	
		UFUNCTION()
		void Initialize(UFMODEvent* InEvent, FTransform InTransform, TArray<FMODParam> Params, EOcclusionType InOcclusionType, EPropagationType InPropagationType, bool InbDebugMode);

		UFUNCTION(BlueprintPure, BlueprintCallable)
		static USoundSource* CreateFirstPersonSound(UWorld* InWorld, UFMODEvent* InEvent, FTransform InTransform, TArray<FMODParam> Params, bool InbDebugMode = false);

		UFUNCTION(BlueprintPure, BlueprintCallable)
		static USoundSource* CreateThirdPersonSound(UWorld* InWorld, UFMODEvent* InEvent, FTransform InTransform, TArray<FMODParam> Params, EOcclusionType InOcclusionType, EPropagationType InPropagationType, bool InbDebugMode = false);

		// Resets the SoundSource back to its default parameters.
		UFUNCTION()
		void ResetSoundSource();

		// Stops the SoundSource from playing, and queues it for pool collection.
		UFUNCTION(BlueprintCallable)
		void Stop();

		// Starts playing the SoundSource.
		UFUNCTION(BlueprintCallable)
		void Play();

		// Pauses the event playing.
		UFUNCTION(BlueprintCallable)
		void SetPaused(bool Paused);

		// Sets a specific parameter to a value.
		UFUNCTION(BlueprintCallable)
		void SetParameter(FName Name, float Value);

		// Attaches the SoundSource to a specific actor and a socket.
		UFUNCTION(BlueprintCallable)
		void Attach(USceneComponent* InAttachToComponent, FName InAttachPointName);

		// Detaches the SoundSource from the attached actor.
		UFUNCTION(BlueprintCallable)
		void Detach();

		// Detaches the SoundSource from the attached actor.
		UFUNCTION(BlueprintCallable)
		void AddChild(USoundSource* Child);

		// Sets the occlusion type of the SoundSource.
		UFUNCTION(BlueprintCallable)
		void SetOcclusionType(TEnumAsByte<EOcclusionType> InOcclusionType);

		// Sets the propagation type of the SoundSource.
		UFUNCTION(BlueprintCallable)
		void SetPropagationType(TEnumAsByte<EPropagationType> InPropagationType);

		// Sets the debug mode of the SoundSource.
		UFUNCTION(BlueprintCallable)
		void SetDebugMode(bool InbDebugMode);
		
		// Time in seconds between propagation updates.
		UFUNCTION(BlueprintCallable)
		void SetPropagationTickInterval(float Interval);
		
		// Time in seconds between transform and primary data updates.
		UFUNCTION(BlueprintCallable)
		void SetPrimaryTickInterval(float Interval);

		// If the SoundSource is running or not.
		UPROPERTY(BlueprintReadOnly)
		bool bIsRunning = false;

		// If the SoundSource is active or not.
		UPROPERTY(BlueprintReadOnly)
		bool bIsActive = false;

		// If the SoundSource it paused or not.
		UPROPERTY()
		bool bIsPaused = false;

		// Time since the last primary sound update in seconds.
		UPROPERTY(BlueprintReadOnly)
		float TimeSincePrimaryUpdate = 0.0f;

		// Time since the last propagation sound update in seconds.
		UPROPERTY(BlueprintReadOnly)
		float TimeSincePropagationUpdate = 0.0f;

		// Time in seconds for the sound to update.
		UPROPERTY(BlueprintReadWrite)
		float PrimaryUpdateInterval = 0.2;

		// Time in seconds for the sound to update.
		UPROPERTY(BlueprintReadWrite)
		float PropagationUpdateInterval = 0.2;

		// The type of occlusion the SoundSource will use.
		UPROPERTY(BlueprintReadOnly)
		EOcclusionType OcclusionType;

		// The type of propagation the SoundSource will use.
		UPROPERTY(BlueprintReadOnly)
		EPropagationType PropagationType;

		// Whether the SoundSource is first or third person.
		UPROPERTY(BlueprintReadOnly)
		ESoundSourceType SoundSourceType;

		// What type of SoundSource this is.
		UPROPERTY(BlueprintReadOnly)
		EHierarchyType HierarchyType;

		// Children of the SoundSource
		UPROPERTY(BlueprintReadOnly)
		TArray<USoundSource*> ChildrenSoundSources;

		// Parent of this SoundSource if there is one.
		UPROPERTY(BlueprintReadOnly)
		USoundSource* ParentSoundSource;

		// Instance GUID
		UPROPERTY(BlueprintReadOnly)
		FGuid InstanceGuid;

		// The sound event to play.
		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FMODAudio)
		UFMODEvent* Event;

		// The transform of the SoundSource.
		UPROPERTY(BlueprintReadOnly)
		FTransform Transform;

		// The component the SoundSource is attached to.
		UPROPERTY(BlueprintReadOnly)
		USceneComponent* AttachToComponent;

		// The name of the socket to attach the SoundSource to.
		UPROPERTY(BlueprintReadOnly)
		FName AttachPointName;

		// If the SoundSource is in debug mode or not.
		UPROPERTY(BlueprintReadWrite)
		bool bDebugMode = false;
	
		// If the SoundSource should always avoid occlusion using the first portal if the portal is being touched.
		UPROPERTY(BlueprintReadWrite)
		bool bIsDoorAttachedSound = false;

		// The world this SoundSource is registered to.
		UPROPERTY()
		UWorld* World;

		// If this SoundSource needs to be registered later.
		UPROPERTY()
		bool bRegisterLater = false;

		// If this SoundSource has been requested to play, but cannot, and is queued to play as soon as possible.
		UPROPERTY()
		bool bPlayLater = false;
	
		virtual void Tick(float DeltaTime) override;
		virtual ETickableTickType GetTickableTickType() const override;
		virtual bool IsTickable() const override;
		virtual TStatId GetStatId() const override;
		uint32 LastTickFrame = INDEX_NONE;

		float GetDoorOcclusionAdditive() const;
		void SetDoorOcclusionAdditive(float DoorOcclusionAdditive);
		float GetOutsidePathOcclusionDistance() const;
		void SetOutsidePathOcclusionDistance(float OutsidePathOcclusionDistance);
		float GetOutsideOcclusionMultiplier() const;
		void SetOutsideOcclusionMultiplier(float OutsideOcclusionMultiplier);
		float GetAngleOcclusionMultiplier() const;
		void SetAngleOcclusionMultiplier(float AngleOcclusionMultiplier);
		float GetObstructionOcclusionAdditive() const;
		void SetObstructionOcclusionAdditive(float ObstructionOcclusionAdditive);

		/** Set the sound name to use for programmer sound.  Will look up the name in any loaded audio table. */
		UFUNCTION(BlueprintCallable, Category = "Audio|FMOD|Components")
		void SetProgrammerSoundName(FString Value);

		/** Set a programmer sound to use for this audio component.  Lifetime of sound must exceed that of the audio component. */
		void SetProgrammerSound(FMOD::Sound *Sound);
		
		/** Programmer Sound Create callback. */
		void EventCallbackCreateProgrammerSound(struct FMOD_STUDIO_PROGRAMMER_SOUND_PROPERTIES *props);

		/** Programmer Sound Destroy callback. */
		void EventCallbackDestroyProgrammerSound(struct FMOD_STUDIO_PROGRAMMER_SOUND_PROPERTIES *props);

		/** Timeline Marker callback. */
		void EventCallbackAddMarker(struct FMOD_STUDIO_TIMELINE_MARKER_PROPERTIES *props);

		/** Timeline Beat callback. */
		void EventCallbackAddBeat(struct FMOD_STUDIO_TIMELINE_BEAT_PROPERTIES *props);

		void EventCallbackSoundPlayed();
	
		void EventCallbackSoundStopped();
		bool TriggerSoundStoppedDelegate;

		float GetCurrentSoundLength() const;

		/** Called when a sound stops. */
		UPROPERTY(BlueprintAssignable)
		FOnSoundStopped OnSoundStopped;

		/** Direct assignment of programmer sound from other C++ code. */
		FMOD::Sound* ProgrammerSound;
		bool NeedDestroyProgrammerSoundCallback;

		FMOD::Sound* ProgrammerSoundInstance;
		bool bWantsProgrammerSoundLength = false;

		DECLARE_MULTICAST_DELEGATE_OneParam(FProgrammerSoundLengthReady, float)
		FProgrammerSoundLengthReady OnProgrammerSoundLengthReady;
	
		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FMODAudio)
		FString ProgrammerSoundName;
		
		// FMOD Audio Component Variables 
		bool bEnableTimelineCallbacks = true;
		FCriticalSection CallbackLock;
		TArray<FTimelineMarkerProperties> CallbackMarkerQueue;
		TArray<FTimelineBeatProperties> CallbackBeatQueue;
	
		UPROPERTY(BlueprintAssignable)
		FOnEventStopped OnEventStopped;

		UPROPERTY(BlueprintAssignable)
		FOnSoundFinished OnSoundFinished;
	
		UPROPERTY(BlueprintAssignable)
		FOnTimelineMarker OnTimelineMarker;
		
		UPROPERTY(BlueprintAssignable)
		FOnTimelineBeat OnTimelineBeat;

		UPROPERTY(EditAnywhere, BlueprintReadWrite, SimpleDisplay, Category = FMODAudio)
		TMap<FName, float> ParameterCache;

		int32 EventLength;
		float StoredProperties[EFMODEventProperty::Count];

private:
	IFMODStudioModule& GetStudioModule()
	{
		if (Module == nullptr)
		{
			Module = &IFMODStudioModule::Get();
		}
		return *Module;
	}
	IFMODStudioModule* Module;
	FMOD::Studio::EventInstance *StudioInstance;
	
	void OnPlaybackCompleted();
	void ReleaseEventInstance();
	void CacheDefaultParameterValues();
	void CheckForLegacySound();
	
	void UpdateTickData();
	void UpdateTransform();
	float GetDepthMaterialOcclusionAmount(float DefaultOcclusionDepth, float OcclusionMultiplier);
	float ActorGetDistanceToCollision(AActor* Actor, const FVector Point, FVector& ClosestPointOnCollision);
	void GetPathOcclusionLength(TArray<TSharedPtr<FGraphNode>> Path, float* Distance, float* Occlusion);
	void PlayInternal();
	void InitializeInternal(TArray<FMODParam> Params);

	float DoorOcclusionAdditive = 0.7f;
	float OutsidePathOcclusionDistance = 750.0f;
	float OutsideOcclusionMultiplier = 0.05f;
	float AngleOcclusionMultiplier = 1.0f;
	float ObstructionOcclusionAdditive = 0.2f;

	bool bObstructedLastTrace = false;

	FTraceDelegate TraceDelegate;
	void OnTraceCompleted(const FTraceHandle& Handle, FTraceDatum& Data);
	float InitiallyAddedOcclusion = 0.0f;

	bool bTriggerProgrammerSoundPlayed = false;

	UFUNCTION(BlueprintCallable)
	static void ReplaceAnimNotifies(UAnimSequenceBase* AnimationSequence);	
};
