// Copyright Void Interactive, 2023

#pragma once

#include "Actors/Gameplay/ReportableActor.h"
#include "GameFramework/Info.h"
#include "Objective.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FObjectiveDelegate, class AObjective*, Objective);

UENUM(BlueprintType)
enum class EObjectiveStatus : uint8
{
	Objective_InProgress,
	Objective_Complete,
	Objective_Failed
};

UENUM(BlueprintType)
enum class EHiddenObjectiveUnlockMethod : uint8
{
	/** Objective cannot unlock, stays hidden throughout the mission */
	Unlock_None,
	/** Objective can be unlocked after reporting a specific reportable */
	Unlock_Reportable,
	/** Objective can be unlocked after completing a specific objective */
	Unlock_Objective,
	/** Objective unlocks itself upon completion*/
	Unlock_Self
};

UCLASS(Abstract, Blueprintable, BlueprintType, HideCategories = ("Actor", "LOD", "Cooking", "Replication"))
class READYORNOT_API AObjective : public AInfo
{
	GENERATED_BODY()

public:
	AObjective();
	
	virtual void TickObjective();

	UFUNCTION(BlueprintPure)
	bool IsObjectiveInProgress() const;
	
	UFUNCTION(BlueprintPure)
	bool IsObjectiveCompleted() const;
	
	UFUNCTION(BlueprintPure)
	bool IsObjectiveFailed() const;
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE EObjectiveStatus GetObjectiveStatus() const { return ObjectiveStatus; }

	void SetObjectiveStatus(const EObjectiveStatus& NewObjectiveStatus) { ObjectiveStatus = NewObjectiveStatus; }
		
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Objective")
	class UScoringComponent* ScoringComponent = nullptr;

	UFUNCTION(BlueprintCallable, Category = "Objective")
	void ObjectiveCompleted();

	UFUNCTION(BlueprintCallable, Category = "Objective")
    void ObjectiveFailed();

	UPROPERTY(BlueprintAssignable)
	FObjectiveDelegate OnObjectiveUpdated;
	
	// When this objective has been completed, this blueprint event will fire
	UFUNCTION(BlueprintNativeEvent)
    void OnObjectiveCompleted();

	// When this objective has been failed, this blueprint event will fire
	UFUNCTION(BlueprintNativeEvent)
    void OnObjectiveFailed();

	// This function is called when the objective is initialized.
	UFUNCTION(BlueprintNativeEvent)
    void OnObjectiveCreated();

	UPROPERTY()
	UFMODEvent* ObjectiveCompleteAudio;

	UPROPERTY()
	UFMODEvent* ObjectiveFailedAudio;

	//UFUNCTION()
	//void OnRep_ObjectiveStatus();

	// The name of this objective
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText ObjectiveName;

	// The description of this objective.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText ObjectiveDescription;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	virtual FText GetFormattedName() { return ObjectiveName; }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	virtual FText GetFormattedDescription() { return ObjectiveDescription; }
	
	// If locked to a mode it wont be displayed in any other mode
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ECOOPMode LockedToMode = ECOOPMode::CM_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bFailureEndsMission = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bHiddenObjective = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "bHiddenObjective"))
	bool bAllowCompletionWhileHidden = true;
	
	UPROPERTY(EditDefaultsOnly, Category = "Objective", meta = (EditCondition = "bHiddenObjective"))
	EHiddenObjectiveUnlockMethod UnlockMethod = EHiddenObjectiveUnlockMethod::Unlock_None;

	UPROPERTY(EditDefaultsOnly, Category = "Objective", meta = (EditCondition = "bHiddenObjective && UnlockMethod == EHiddenObjectiveUnlockMethod::Unlock_Reportable", EditConditionHides))
	TSubclassOf<AReportableActor> UnlockingReportableClass = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Objective", meta = (EditCondition = "bHiddenObjective && UnlockMethod == EHiddenObjectiveUnlockMethod::Unlock_Objective", EditConditionHides))
	TSubclassOf<AObjective> UnlockingObjectiveClass = nullptr;

	UPROPERTY()
	AReportableActor* UnlockingReportable = nullptr;
	
	UPROPERTY()
	AObjective* UnlockingObjective = nullptr;
	

protected:
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "Tick Objective")
    void TickObjective_BP();

	virtual void OnObjectiveCompleted_Implementation();
	virtual void OnObjectiveFailed_Implementation();
	virtual void OnObjectiveCreated_Implementation();
	
	//UPROPERTY(BlueprintReadOnly)
	//class AReadyOrNotGameState* GameState;

private:
	// The current status of this objective.
	UPROPERTY(Replicated, ReplicatedUsing=OnRep_ObjectiveStatus)
	EObjectiveStatus ObjectiveStatus = EObjectiveStatus::Objective_InProgress;

	UFUNCTION()
	void OnRep_ObjectiveStatus();

public:
	void OnMissionObjectivesCreated();

	UFUNCTION()
	void OnUnlockingReportableReported(AReportableActor* ReportableActor);

	UFUNCTION()
	void OnUnlockingObjectiveUpdated(AObjective* Objective);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_UnlockObjective();
};
