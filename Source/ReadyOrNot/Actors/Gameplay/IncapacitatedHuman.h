// Void Interactive, 2020

#pragma once

#include "GameFramework/Actor.h"
#include "IncapacitatedHuman.generated.h"

UCLASS(HideCategories = ("Rendering", "Replication", "Collision", "Input", "Actor", "LOD", "Cooking"))
class READYORNOT_API AIncapacitatedHuman : public AActor,
	public IUseabilityInterface, public IReportable, public IScoringInterface,
	public IReceiveAISenseUpdates, public IAISightTargetInterface
{
	GENERATED_BODY()
	
public:	
	AIncapacitatedHuman();

	UFUNCTION(BlueprintPure, Category = "Incap Human")
	FORCEINLINE bool HasBeenReported() const { return bHasBeenReported; }
	
	UFUNCTION(BlueprintPure, Category = "Incap Human")
	FORCEINLINE bool IsChild() const { return bIsChild; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }
	virtual void EditorTick(float DeltaTime);
#endif

	void BecomeDead();

	virtual bool CanBeSeenFrom(const FVector& ObserverLocation, FVector& OutSeenLocation, int32& NumberOfLoSChecksPerformed, float& OutSightStrength, const AActor* IgnoreActor, const bool* bWasVisible, int32* UserData) const override;
	virtual void OnAIPerceptionSense_Implementation(ACyberneticController* InSenseController, FAIStimulus Stimulus, AActor*& OutOverrideSensedActor) override;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	USceneComponent* DefaultScene = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* HumanMesh = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* HumanMeshFace = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UCapsuleComponent* CapsuleComponent = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UInteractableComponent* ReportInteractableComponent = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UScoringComponent* ScoringComponent = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Components")
	class UFMODAudioComponent* IncapacitatedAudioComponent = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Component")
	UAIPerceptionStimuliSourceComponent* PerceptionStimuliComp = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup")
	UPhysicsAsset* RagdollPhysicsAsset = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup")
	UAnimMontage* DyingMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup")
	UFMODEvent* FMODEventLoop = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup")
	UParticleSystem* ShotParticleEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup", meta = (EditCondition = "!bIsInGroup || bIsInGroup && bIsMasterOfGroup"))
	bool bIsChild = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup", meta = (EditCondition = "!bIsInGroup || bIsInGroup && bIsMasterOfGroup"))
	bool bStartDead = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup", meta = (EditCondition = "!bStartDead && !bIsInGroup || bIsInGroup && bIsMasterOfGroup"))
	bool bCanEverDieByTime = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup", meta = (EditCondition = "!bStartDead && !bIsInGroup || bIsInGroup && bIsMasterOfGroup && bCanEverDieByTime"))
	float TimeRemainingUntilDead = 60.0f;
	
	UFUNCTION(CallInEditor, Category = "Group")
	void SelectAllInGroup();
	
	UFUNCTION(CallInEditor, Category = "Group")
	void MakeMasterInGroup();
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Group")
	bool bIsInGroup = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Group", meta = (EditCondition = "bIsInGroup && MasterHumanInGroup == nullptr", EditConditionHides))
	bool bIsMasterOfGroup = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Group", DisplayName = "Humans In Group", meta = (EditCondition = "bIsInGroup && bIsMasterOfGroup", EditConditionHides))
	TArray<AIncapacitatedHuman*> IncapacitatedHumansInGroup;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Group", meta = (EditCondition = "bIsInGroup && !bIsMasterOfGroup", EditConditionHides))
	AIncapacitatedHuman* MasterHumanInGroup = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reporting", meta = (EditCondition = "!bIsInGroup || bIsInGroup && bIsMasterOfGroup"))
	bool bAttachReportInteractableToMesh = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reporting", meta = (EditCondition = "!bIsInGroup || bIsInGroup && bIsMasterOfGroup && bAttachReportInteractableToMesh"))
	FName SocketToAttach = "spine_1";
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reporting", meta = (EditCondition = "!bIsInGroup || bIsInGroup && bIsMasterOfGroup"))
	ETeamType Team = ETeamType::TT_CIVILIAN;
	
	UPROPERTY(BlueprintReadOnly, Replicated)
	bool bIsDead = false;

	UPROPERTY(BlueprintReadOnly, Replicated)
	bool bHasBeenReported = false;

	// IUseability interface
	////////////////////////////
	virtual void Interact_Implementation(AReadyOrNotCharacter* InInteractInstigator, class UInteractableComponent* InInteractableComponent) override;
	virtual FName DetermineAnimatedIcon_Implementation() const override;
	virtual FText DetermineActionText_Implementation() const override;
	virtual bool CanInteractThroughHitActors_Implementation(const FHitResult& Hit) const override;
	////////////////////////////

	// IReportable interface
	////////////////////////////
	virtual void ReportToTOC_Implementation(class AReadyOrNotCharacter* Reporter, bool bPlayAnimation = true) override;
	virtual bool CanReportNow_Implementation() override;
	virtual FString GetSpeechTypeForReport_Implementation() override;
	////////////////////////////
	
	// IScoring interface
	////////////////////////////
	virtual class UScoringComponent* GetScoringComponent_Implementation() const override;
	////////////////////////////

private:
	void SetupGroup(bool bRemoveNullElements = false);

	void StopAllAudio();
};
