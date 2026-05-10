// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Enums.h"
#include "ActivityData.generated.h"

struct FTutorialDescriptionInput;
class ATrainingTarget;

// Delegate signatures
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActivityComplete, UActivityData*, ActivityData);

// Binds a delegate to an object if it is not already bound.
#define BIND_DELEGATE(Object, Delegate, BindFunction) { \
	if (!Delegate.IsAlreadyBound(Object, BindFunction)) \
	{ \
		bDelegateBound = true; \
		Delegate.AddUniqueDynamic(Object, BindFunction); \
	} \
}

// Unbinds a delegate from an object if it is already bound.
#define UNBIND_DELEGATE(Object, Delegate, BindFunction) { \
	if (Delegate.IsAlreadyBound(Object, BindFunction)) \
	{ \
		bDelegateBound = false; \
		Delegate.RemoveDynamic(Object, BindFunction); \
	} \
}

USTRUCT(BlueprintType)
struct FSwatCommandData
{
	GENERATED_BODY()

	/** The SWAT command to issue. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ESwatCommand Command = ESwatCommand::SC_None;

	/** The SWAT team to issue the command to. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ETeamType Team = ETeamType::TT_NONE;

	/** The target to issue the command to. (optional - leave blank for none) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<AActor> Target;

	/** Whether to queue the SWAT command or execute immediately. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bQueue = false;
};

/**
 *	Activity Data contains the logic for performing an activity.
 *	An activity is an individual task that a player must complete.
 *	To be used in conjunction with the ActivityTriggerVolume actor.
 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew)
class READYORNOT_API UActivityData : public UDataAsset
{
	GENERATED_BODY()

public:
	/** The type of activity to perform. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Activity")
	EActivity Activity = EActivity::A_GoToLocation;

	/** The details of the SWAT command to issue. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Activity", meta = (EditCondition = "Activity == EActivity::A_IssueSwatCommand", EditConditionHides))
	FSwatCommandData SwatCommandData;

	/** The content to display in the tooltip widget. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Activity", meta = (EditCondition = "Activity != EActivity::A_Delay", EditConditionHides))
	FTutorialWidgetData WidgetData;

	/** The number of actions required to complete the activity. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Activity", meta = (ClampMin = 1))
	int32 ActionsRequired = 1;

	/** The amount of time (in seconds) required to complete the activity. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Activity", meta = (ClampMin = 0.25))
	float TimeRequired = 0.25f;

	/** Event triggered when the activity is complete. */
	UPROPERTY(BlueprintAssignable, Category = "Activities")
	FOnActivityComplete OnComplete;

private:
	/** The current player character bound to this activity. */
	UPROPERTY()
	APlayerCharacter* PlayerCharacter = nullptr;

	bool bIsComplete = false;
	int32 ActionsCompleted = 0;
	float TimeElapsed = 0.0f;

	bool bDelegateBound = false;

	/** The characters that have been killed during an "Arrest or Kill" activity. */
	UPROPERTY()
	TArray<AReadyOrNotCharacter*> CharactersKilled;

public:
	/** Bind a player character to this activity. */
	void BindPlayerToActivity(APlayerCharacter* Player);

	/** Unbind the player character from this activity. */
	void UnbindPlayerFromActivity();

	/** Reset any activity progress. */
	void ResetProgress();

	/** Check whether the activity is complete or not. */
	UFUNCTION(BlueprintPure)
	bool IsComplete() const;

private:
#if WITH_EDITOR
	/** Enable/disable the ActionsRequired/TimeRequired properties based on selected activity. */
	virtual bool CanEditChange(const FProperty* InProperty) const override;
	bool CanEditActionsRequired() const;
	bool CanEditTimeRequired() const;
#endif

	//~=============================================================================
	// Delegate Function Handlers

	/** Delegate Function Handler for Completing the Activity */
	UFUNCTION()
	void CompleteActivity();

	/** Delegate Function Handler for Moving */
	UFUNCTION()
	void OnCharacterMovementUpdated(float DeltaSeconds, FVector OldLocation, FVector OldVelocity);

	/** Delegate Function Handler for Interacting */
	UFUNCTION()
	void OnInteract(UInteractableComponent* InteractableComponent);

	/** Delegate Function Handler for Collecting Evidence */
	UFUNCTION()
	void OnEvidenceCollected(AActor* Evidence);

	UFUNCTION()
	void OnItemEquipped(ABaseItem* Item);

	/** Delegate Function Handler for Kicking Doors */
	UFUNCTION()
	void OnDoorKicked(ADoor* Door, AReadyOrNotCharacter* InstigatorCharacter, bool bSuccess);

	/** Delegate Function Handler for Shooting */
	UFUNCTION()
	void OnWeaponFire(AReadyOrNotCharacter* Character, ABaseMagazineWeapon* Weapon, FVector FireDirection);

	/** Delegate Function Handler for Reloading (fast) */
	UFUNCTION()
	void OnWeaponReload(APlayerCharacter* DelegatePlayerCharacter);

	/** Delegate Function Handler for Reloading (normal) */
	UFUNCTION()
	void OnWeaponTacticalReload(APlayerCharacter* DelegatePlayerCharacter);

	/** Delegate Function Handler for Switching Ammo Type */
	UFUNCTION()
	void OnWeaponSwitchAmmoType(APlayerCharacter* DelegatePlayerCharacter);

	/** Delegate Function Handler for Changing Fire Mode */
	UFUNCTION()
	void OnWeaponFireModeChanged(APlayerCharacter* DelegatePlayerCharacter, EFireMode NewFireMode, EFireMode LastFireMode);

	/** Delegate Function Handler for Toggling Weapon Light */
	UFUNCTION()
	void OnAttachmentLightToggled();

	/** Delegate Function Handler for Toggling Canted Sights */
	UFUNCTION()
	void OnCantedSightToggled(bool bUsingCantedSight);

	/** Delegate Function Handler for Using an Item */
	UFUNCTION()
	void OnItemUseStart(ABaseItem* Item);

	/** Delegate Function Handler for when an Item is Used */
	UFUNCTION()
	void OnItemUseCompleted(ABaseItem* Item);

	/** Delegate Function Handler for Using a Chemlight */
	UFUNCTION()
	void OnChemlightThrown(APlayerCharacter* DelegatePlayerCharacter);

	/** Delegate Function Handler for Using NVGs */
	UFUNCTION()
	void OnNightVisionGogglesToggled(AReadyOrNotCharacter* Character, bool bOn);

	/** Delegate Function Handler for Issuing a SWAT Command */
	UFUNCTION()
	void OnSwatCommandIssued(const ESwatCommand SwatCommand, const ETeamType TeamType, AActor* ContextActor);

	/** Delegate Function Handler for Queuing a SWAT Command */
	UFUNCTION()
	void OnSwatCommandQueued(const FQueuedSwatCommand QueuedSwatCommand, const ETeamType TeamType);

	/** Delegate Function Handler for Characters being arrested */
	UFUNCTION()
	void OnCharacterArrested(AReadyOrNotCharacter* Character, AReadyOrNotCharacter* ArrestedBy);

	/** Delegate Function Handler for Characters being killed */
	UFUNCTION()
	void OnCharacterKilled(AReadyOrNotCharacter* Character, AReadyOrNotCharacter* KilledBy);

	/** Delegate Function Handler for Hitting a Training Target */
	UFUNCTION()
	void OnTargetHit(ATrainingTarget* Target);

	/** Delegate Function Handler for Switching SWAT Team Camera */
	UFUNCTION()
	void OnTeamViewSet(AReadyOrNotCharacter* NewViewCharacter);

	/** Delegate Function Handler for Switching SWAT Team Element */
	UFUNCTION()
	void OnSwatElementChanged(ETeamType TeamType);

	/** Delegate Function Handler for Exfiltrating Training */
	UFUNCTION()
	void OnExfiltrateMission();
};
