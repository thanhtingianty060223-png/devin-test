// Copyright Void Interactive, 2021

#pragma once

#include "Actors/PickupActor.h"
#include "Interfaces/CanIssueCommandOn.h"
#include "Interfaces/ScoringInterface.h"
#include "EvidenceActor.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API AEvidenceActor : public APickupActor, public ICanIssueCommandOn, public IScoringInterface, public ISecurable
{
	GENERATED_BODY()
	
public:
	AEvidenceActor();

	virtual void ActorPickedUp(AActor* InPickupInstigator) override;
	virtual void ActorDropped(AActor* InDroppedInstigator) override;

	UFUNCTION(BlueprintPure, Category = "Evidence")
	FORCEINLINE FText GetEvidenceName() const { return EvidenceName; }
	
	UFUNCTION(BlueprintCallable, Category = "Evidence")
	virtual void StartExtractingEvidence();
	
	UFUNCTION(BlueprintCallable, Category = "Evidence")
	virtual void FinishExtractingEvidence();

	bool IsEvidenceCollected() const;

	void DestroyEvidence();

    void StartEvidenceCollection_COOP();

    void StopEvidenceCollection_COOP();
	
    void CompleteEvidenceCollection_COOP();

	UFUNCTION()
	void UpdateEvidenceCollection_COOP(float DeltaTime);
	
	void ShowEvidenceActor(bool bReportToInGameLog = true);
	void HideEvidenceActor(bool bReportToInGameLog = true);

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void Reset() override;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Scoring")
	class UScoringComponent* ScoringComponent = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	FText EvidenceName = FText::FromString("Evidence");
	
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Evidence")
	uint8 bEvidenceExtracted : 1;

	UPROPERTY(BlueprintReadOnly, Replicated)
	EEvidenceActorState PreviousEvidenceState = EEvidenceActorState::Unclaimed;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_EvidenceStateChanged)
	EEvidenceActorState EvidenceState = EEvidenceActorState::Unclaimed;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Collection")
	bool bIsBeingCollected;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Collection")
	float CurrentCollectionTime = 0.0f;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Collection")
	float MaxCollectionTime = 2.0f;
	
	UPROPERTY(BlueprintReadOnly, Category = "Collection")
	uint8 bInteractHeld : 1;

	void ReportPickupToInGameLog(AActor* InPickupInstigator);
	void ReportDropToInGameLog(AActor* InDropInstigator);
	void ReportExtractingToInGameLog(AActor* InPickupInstigator);
	void ReportExtractedToInGameLog();
		
	UFUNCTION()
	void OnRep_EvidenceStateChanged();

	virtual bool CanPickUpNow(class APlayerCharacter* PickerUpper) override;

	// Begin IUseabilityInterface
	virtual void Interact_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent) override;
	virtual void EndInteract_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent) override;
	virtual void OnFocusLost_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent) override;
	virtual FName DetermineAnimatedIcon_Implementation() const override;
	virtual bool CanInteractThroughHitActors_Implementation(const FHitResult& Hit) const override;
	// End IUseabilityInterface

	// ICanIssueCommandOn
	///////////////////////////////////
	virtual bool CanIssueCommand_Implementation() const override;
	virtual AActor* GetCommandActor_Implementation() const override;
	///////////////////////////////////

	// IScoringInterface
	///////////////////////////////////
	virtual class UScoringComponent* GetScoringComponent_Implementation() const override;
	///////////////////////////////////

	virtual void Secure_Implementation(AReadyOrNotCharacter* InInstigator) override;
	virtual bool IsSecured_Implementation() const override;
	virtual FVector GetLocation_Implementation() const override;
	virtual bool CanBeSecured_Implementation() const override;
};
