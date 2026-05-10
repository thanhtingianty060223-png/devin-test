// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "Enums.h"
#include "ActivityTriggerVolume.generated.h"

class UScreenspaceMarkerWidget;
class UTutorialWidget;
class UActivityData;

USTRUCT(BlueprintType)
struct FActivityEvent
{
	GENERATED_BODY()

	#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere)
	FString Name = "";
	#endif

	/** The type of event to perform. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event")
	EEventType EventType;

	/** The standalone event to perform. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event", meta = (EditCondition = "EventType == EEventType::E_Standalone", EditConditionHides))
	EStandaloneEvent StandaloneEvent = EStandaloneEvent::E_None;

	/** The target event to perform. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event", meta = (EditCondition = "EventType == EEventType::E_Target", EditConditionHides))
	ETargetEvent TargetEvent = ETargetEvent::E_None;

	/** The target actor to perform the event on. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event", meta = (EditCondition = "EventType == EEventType::E_Target", EditConditionHides))
	TSoftObjectPtr<AActor> TargetActor;

	/** The FMOD event to play. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event", meta = (EditCondition = "EventType == EEventType::E_FmodAudio", EditConditionHides))
	UFMODEvent* FmodAudioEvent;

	/** The Unreal Audio event to play. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event", meta = (EditCondition = "EventType == EEventType::E_UnrealAudio", EditConditionHides))
	USoundCue* UnrealAudioEvent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event")
	bool bTriggerOnlyOnce = false;

	bool bHasTriggered = false;
};

// Delegate signatures
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAllActivitiesComplete, AActivityTriggerVolume*, TriggerVolume);

UCLASS()
class READYORNOT_API AActivityTriggerVolume : public ATriggerBox
{
	GENERATED_BODY()

public:
	AActivityTriggerVolume();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

private:
	/** Events to perform when the volume is activated. */
	UPROPERTY(Category=Activities, EditAnywhere, meta = (TitleProperty = "Name"))
	TArray<FActivityEvent> ActivationEvents;

	/** Events to perform when player enters the volume. */
	UPROPERTY(Category=Activities, EditAnywhere, meta = (TitleProperty = "Name"))
	TArray<FActivityEvent> OnEnterEvents;

	/** Activities player is required to perform. */
	UPROPERTY(Category=Activities, EditAnywhere, Instanced)
	TArray<UActivityData*> Activities;

	/** Events to perform when player leaves the volume. */
	UPROPERTY(Category=Activities, EditAnywhere, meta = (TitleProperty = "Name"))
	TArray<FActivityEvent> OnLeaveEvents;

	/** Events to perform after all activities are complete. */
	UPROPERTY(Category=Activities, EditAnywhere, meta = (TitleProperty = "Name"))
	TArray<FActivityEvent> CompletionEvents;

	/** Transition Volume to activate once all activities are complete. */
	UPROPERTY(Category=OnCompletion, EditAnywhere)
	TSoftObjectPtr<AActivityTriggerVolume> NextTransitionVolume;

	/** Delay before transitioning to the next volume. */
	UPROPERTY(Category=OnCompletion, EditAnywhere)
	float TransitionDelay = 0.0f;

	/** Whether or not the trigger is activate on start. */
	UPROPERTY(Category=Settings, EditAnywhere)
	bool bStartActive = false;

	/** The delay before activity checks begin. */
	UPROPERTY(Category=Settings, EditAnywhere, meta = (ClampMin = 0.0f))
	float ActivityDelay = 0.0f;

	/** Whether or not to trigger events while the volume is inactive. */
	UPROPERTY(Category=Settings, EditAnywhere)
	bool bTriggerEventsWhileInactive = false;

	/** Whether or not to draw a debug box while PIE. */
	UPROPERTY(Category=Debug, EditAnywhere)
	bool bDrawDebug = true;

public:
	/** Activate the trigger to begin activity checks. */
	UFUNCTION(Category=Triggerable, BlueprintCallable)
	virtual void Activate();

	/** Deactivate the trigger to stop activity checks. */
	UFUNCTION(Category=Triggerable, BlueprintCallable)
	virtual void Deactivate();

	/** Reset all progress and activate the trigger. */
	UFUNCTION(Category=Triggerable, BlueprintCallable)
	virtual void Reactivate(float ReactivateDelay = 0.0f);

	/** Reset all progress made to activities and events. */
	UFUNCTION(Category=Triggerable, BlueprintCallable)
	virtual void ResetAllProgress();

	/** Check if the trigger is active. */
	UFUNCTION(Category=Activities, BlueprintPure)
	bool IsActive() const { return bIsActive; }

	/** Check if all activities are complete. */
	UFUNCTION(Category=Activities, BlueprintPure)
	bool AllActivitiesComplete() const;

	/** Get the activities required to complete the trigger. */
	UFUNCTION(Category=Activities, BlueprintPure)
	TArray<UActivityData*> GetActivities() const { return Activities; }

private:
	UPROPERTY()
	APlayerCharacter* PlayerCharacter = nullptr;

	UPROPERTY()
	FTimerHandle ActivateDelayTimerHandle;

	bool bIsActive = false;
	bool bIsComplete = false;

public:
	/** Event triggered when all actions are complete. */
	UPROPERTY(BlueprintAssignable, Category = "Activities")
	FOnAllActivitiesComplete OnAllActivitiesComplete;

	/** Reset trigger to it's initial state. */
	virtual void Reset() override;

	/** Check if volume contains any standalone events. */
	bool ContainsAnyEvent(EStandaloneEvent StandaloneEvent) const;

	/** Check if volume contains any target events. */
	bool ContainsAnyEvent(ETargetEvent TargetEvent) const;

	/** Handle player spawning inside an active volume. */
	void OnPlayerRespawned(APlayerCharacter* PlayerRespawned);

private:
	/** Delegate for when a player begins overlapping this trigger. */
	UFUNCTION()
	virtual void OnPlayerBeginOverlap(AActor* OverlappedActor, AActor* OtherActor);

	/** Handle any on enter events and bind player to activities. */
	void ActivateActivities();

	/** Delegate for when a player ends overlapping this trigger. */
	UFUNCTION()
	virtual void OnPlayerEndOverlap(AActor* OverlappedActor, AActor* OtherActor);

	/** Handle any on leave events and unbind player from activities. */
	void DeactivateActivities();

	/** Delegate for when a player completes an activity. */
	UFUNCTION()
	virtual void OnActivityComplete(UActivityData* Activity);

	/** Deactivate this volume and activate the next. */
	void TransitionToNextVolume();

	/** Check whether or not the given actor is the local player character. */
	bool IsLocalPlayer(const AActor* Actor) const;

	/** Handle binding/unbinding of player to this trigger's activities. */
	void HandleActivities(const bool bUnbind = false);
	void ResetActivities();

	/** Perform all events for the provided array. */
	void HandleEvents(TArray<FActivityEvent>& Events);
	void ResetEvents(TArray<FActivityEvent>& Events);
	void HandleStandAloneEvent(const FActivityEvent& Event);
	void HandleTargetEvent(const FActivityEvent& Event) const;
	void HandleFmodAudioEvent(const FActivityEvent& Event) const;
	void HandleUnrealAudioEvent(const FActivityEvent& Event) const;

	/** Get the tutorial tooltip widget from the HUD. */
	UTutorialWidget* GetTutorialWidget() const;

	/** Show the tutorial tooltip widget. */
	void ShowTutorialWidget() const;

	/** Hide the tutorial tooltip widget. */
	void HideTutorialWidget() const;

	/**
	 * Whether or not we need to show the tutorial tooltip widget.
	 * And if so, what tooltip content to the display.
	 * @return True if the tooltip was set. False if all activities are complete or activity is delay.
	 */
	bool SetTooltipToNextActivity(FTutorialWidgetData& OutWidgetData) const;

	/** Get the screenspace marker widget. */
	UScreenspaceMarkerWidget* GetScreenspaceMarkerWidget() const;

	/** Show the screenspace marker widget. */
	void ShowScreenspaceMarker() const;

	/** Hide the screenspace marker widget. */
	void HideScreenspaceMarker() const;

	/** Delegate for when a player kills a friendly AI. */
	UFUNCTION()
	void OnSpawnedAiKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter);
};
