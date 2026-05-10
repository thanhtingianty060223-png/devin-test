// Copyright Void Interactive, 2023

#pragma once

#include "Actors/Environment/ReadyOrNotTriggerVolume.h"
#include "ExfilPortal.generated.h"

UCLASS(Blueprintable)
class READYORNOT_API AExfilPortal : public AActor, public IUseabilityInterface, public IScoringInterface
{
	GENERATED_BODY()

	AExfilPortal();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UInteractableComponent* InteractableComponent;

	virtual void Interact_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent) override;
	virtual bool CanInteractThroughHitActors_Implementation(const FHitResult& Hit) const override;
	
	UFUNCTION(BlueprintCallable)
	void ExfiltrateMission();
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UScoringComponent* ScoringComponent;

	virtual UScoringComponent* GetScoringComponent_Implementation() const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* CollisionComponent;
	
	UFUNCTION()
	void ActorBeginOverlap(AActor* OverlappedActor, AActor* OtherActor);
	UFUNCTION()
	void ActorEndOverlap(AActor* OverlappedActor, AActor* OtherActor);
	
protected:
	UPROPERTY(BlueprintReadOnly)
	TArray<ASWATCharacter*> OverlappingSwatMembers;

private:
	UPROPERTY()
	TArray<UStaticMeshComponent*> CompsToOutline;

	//Likely should change this from stencil outline to just showing an animated icon like a loadout portal
	void DrawExfilOutline();
	void DisableExfilOutline();
	bool bIsDrawingOutline;

	UFUNCTION()
	void OnExfilSwatMemberKilled(AReadyOrNotCharacter* Killer, AReadyOrNotCharacter* KilledMember);

	UFUNCTION()
	void OnExfilEnabledChange(bool bEnabled);

	UFUNCTION(Server, Reliable)
	void ServerTriggerExfil();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastEnableExfil(bool bEnable);
	
	void UpdateOverlappingOfficer(ASWATCharacter* Officer, bool bAddOfficer);

public:
	UFUNCTION(BlueprintImplementableEvent)
	void OnPlayerInteracted();

	/** Whether exfil widget should be shown for player to confirm action */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exfil")
	bool bShowWarningDialogue = true;

	UFUNCTION()
	void OnMissionSoftComplete();
};
