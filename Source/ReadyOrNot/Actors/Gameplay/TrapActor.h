// Void Interactive, 2021

#pragma once

#include "Enums.h"
#include "GameFramework/Actor.h"
#include "Interfaces/CanUseMultitoolOn.h"
#include "Interfaces/ReceiveAISenseUpdates.h"
#include "Interfaces/ScoringInterface.h"
#include "Interfaces/UseabilityInterface.h"
#include "TrapActor.generated.h"

UENUM(BlueprintType)
enum class ETrapState : uint8
{
	TS_Live,		// trap is alive, not disabled or activated
	TS_Activated,	// trap has been tripped
	TS_Disabled		// trap has been disabled
};

UCLASS(Blueprintable, BlueprintType, HideCategories = ("Rendering", "Replication", "Collision", "Input", "Actor", "LOD", "Cooking"))
class READYORNOT_API ATrapActor : public AActor, public ICanUseMultitoolOn, public ICanIssueCommandOn,
								  public IUseabilityInterface, public IScoringInterface, public IReceiveAISenseUpdates
{
	GENERATED_BODY()

public:
	ATrapActor();
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Components)
	class UStaticMeshComponent* TrapMeshComponent = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Components)
	class UFMODAudioComponent* TrapActivateAudioComponent = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Components)
	class UInteractableComponent* InteractableComponent = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Components)
	class UScoringComponent* ScoringComponent = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Components)
	class USplineComponent* SplineComponent = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Components)
	class UCableComponent* CutCableComponent1 = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Components)
	class UCableComponent* CutCableComponent2 = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Components)
	class UBoxComponent* TripWireTriggerComponent = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	class UAIPerceptionStimuliSourceComponent* PerceptionStimuliComp;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Trap)
	USceneComponent* TrapRoot = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = Trap)
	class AActor* AttachedActor = nullptr;

	virtual bool CanDisarmTrap() const;
	
	virtual bool IsComponentRelevantForNavigation(UActorComponent* Component) const { return false; }

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Trap")
	void Server_OnTrapTriggered(AReadyOrNotCharacter* TriggeredBy);

	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = "Trap")
    void Multicast_OnTrapTriggered(AReadyOrNotCharacter* TriggeredBy);

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Trap")
	void Server_OnTrapDisarmed();

	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = "Trap")
    void Multicast_OnTrapDisarmed();
	
	UFUNCTION(BlueprintNativeEvent, Category = Trap)
			void TrapInit();
	virtual void TrapInit_Implementation();
	
	UFUNCTION(BlueprintNativeEvent, Category = Trap)
			void TrapDeInit();
	virtual void TrapDeInit_Implementation();
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Trap)
			void OnTrapTriggered(AReadyOrNotCharacter* TriggeredBy);
	virtual	void OnTrapTriggered_Implementation(AReadyOrNotCharacter* TriggeredBy);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Trap)
            void OnTrapDisarmed(AReadyOrNotCharacter* DisarmedBy = nullptr);
	virtual	void OnTrapDisarmed_Implementation(AReadyOrNotCharacter* DisarmedBy = nullptr);

	#if WITH_EDITORONLY_DATA
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Trap", DisplayName = "Simulate Cable (Editor Only)")
	uint8 bSimulateCable : 1;
	#endif
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap")
	FString TrapName = "Trap";
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap")
	ETrapType TrapType = ETrapType::Unknown;
	
	UPROPERTY(BlueprintReadWrite, Replicated, Category = "Trap")
	ETrapState TrapStatus = ETrapState::TS_Disabled;

	// If true, we can use the multitool on this thing while it is activated
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Trap")
	bool bCanUseMultitoolWhenActivated = false;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Trap")
	bool bInitializeTrapOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trap|Cable")
	UStaticMesh* CableMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trap|Cable")
	UMaterialInterface* CableMaterial = nullptr;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTrapTriggeredDelegate, ATrapActor*, Trap, AReadyOrNotCharacter*, TriggeredBy);
	UPROPERTY(BlueprintAssignable)
	FTrapTriggeredDelegate TrapTriggered;
	
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	virtual void PostLoad() override;

	#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	#endif

	virtual bool ShouldConsiderTrapCollisionFor(AReadyOrNotCharacter* InCharacter);

	// ICanIssueCommandOn
	///////////////////////////////////
	virtual bool CanIssueCommand_Implementation() const override;
	virtual AActor* GetCommandActor_Implementation() const override;
	///////////////////////////////////
	
	//	ICanUseMultitoolOn
	///////////////////////////////////
	virtual bool CanUseMultitoolNow_Implementation(class AReadyOrNotCharacter* ToolOwner, class AMultitool* Tool, FHitResult TraceHit) override;
	virtual bool CanCancelMultitoolAction_Implementation() override { return true; }
	virtual EMultitoolFunctions GetMultitoolUseType_Implementation() override;
	virtual float GetMultitoolUseTime_Implementation() override;
	virtual void Server_FinishedUsingMultitool_Implementation(class AReadyOrNotCharacter* ToolOwner) override;
	virtual void Client_FinishedUsingMultitool_Implementation(class AReadyOrNotCharacter* ToolOwner) override;
	///////////////////////////////////
	
	// IUseabilityInterface
	///////////////////////////////////
	virtual void Interact_Implementation(AReadyOrNotCharacter* InteractInstigator, class UInteractableComponent* InInteractableComponent) override;
	virtual void Fire_Implementation(AReadyOrNotCharacter* InteractInstigator, class UInteractableComponent* InInteractableComponent) override;
	virtual void EndFire_Implementation(AReadyOrNotCharacter* InteractInstigator, class UInteractableComponent* InInteractableComponent) override;
	virtual void OnFocusLost_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent) override;
	virtual bool CanInteractThroughHitActors_Implementation(const FHitResult& Hit) const override;
	virtual UInteractableComponent* GetInteractableComponent_Implementation() const override;
	///////////////////////////////////

	// IScoringInterface
	///////////////////////////////////
	virtual class UScoringComponent* GetScoringComponent_Implementation() const override;
	///////////////////////////////////

	// IReceiveAISenseUpdates
	///////////////////////////////////
	virtual void OnAIPerceptionSense_Implementation(class ACyberneticController* InSenseController, FAIStimulus Stimulus, AActor*& OutOverrideSensedActor) override;
	///////////////////////////////////

	virtual bool CanCutWire() const;
	
	bool CanEquipMultitool(APlayerCharacter* PlayerCharacter) const;
	bool CanDisarmTrap(APlayerCharacter* PlayerCharacter) const;
	
	virtual void Server_OnTrapTriggered_Implementation(AReadyOrNotCharacter* TriggeredBy);
	virtual bool Server_OnTrapTriggered_Validate(AReadyOrNotCharacter* TriggeredBy) { return true; }
	
	virtual void Multicast_OnTrapTriggered_Implementation(AReadyOrNotCharacter* TriggeredBy);
	
	virtual void Server_OnTrapDisarmed_Implementation();
	virtual bool Server_OnTrapDisarmed_Validate() { return true; }
	
	virtual void Multicast_OnTrapDisarmed_Implementation();

	void InitCable(class UCableComponent* InCableComponent);
	void EnableCable(class UCableComponent* InCableComponent);
	void DisableCable(class UCableComponent* InCableComponent);
	
	void CutCable(float Alpha);
	void CutCable(const FVector& InRelativeLocation);
	
	float TimeSinceTrapTriggered = 0.0f;
	
private:
	void UpdateTickRate();

	float OriginalEndPositionX = 0.0f;
};