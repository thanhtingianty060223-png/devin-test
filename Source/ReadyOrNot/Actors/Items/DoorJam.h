// Copyright Void Interactive, 2021

#pragma once

#include "Actors/BaseItem.h"
#include "DoorJam.generated.h"

UCLASS(Abstract)
class READYORNOT_API ADoorJam : public ABaseItem, public ICanUseMultitoolOn
{
	GENERATED_BODY()
	
public:
	ADoorJam();
	
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	
	virtual bool CanEquip(AReadyOrNotCharacter* ToCharacter) const override;
	
	virtual void Reset() override;
	
	virtual bool CanShowActionSlot1_Implementation(class AReadyOrNotCharacter* PC) override;
	virtual void StunnedWhileEquipped_Implementation() override;

	virtual bool IsDepleted() const override { return bSet; }
	virtual bool ShouldHideInPictureInPictureScopes() override { return !bSet; }

	UFUNCTION(BlueprintCallable, Category = "Door")
	void JamDoor(ADoor* Door);

	UFUNCTION()
	void OnRep_DoorjamSet();
	
	UFUNCTION(NetMulticast, Reliable, Category = Doorjam)
            void Multicast_StartPlacement();
	virtual void Multicast_StartPlacement_Implementation();

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = DoorJam)
            void Server_StartDoorjamPlacement(ADoor* PendingDoor);
	virtual void Server_StartDoorjamPlacement_Implementation(ADoor* PendingDoor);
	virtual bool Server_StartDoorjamPlacement_Validate(ADoor* PendingDoor) { return true; }

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = DoorJam)
            void Server_FinishDoorjamPlacement(ADoor* PendingDoor);
	virtual void Server_FinishDoorjamPlacement_Implementation(ADoor* PendingDoor);
	virtual bool Server_FinishDoorjamPlacement_Validate(ADoor* PendingDoor) { return PendingDoor != nullptr; }

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_DoorjamSet, Category = DoorJam)
	uint8 bSet : 1;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = DoorJam)
	AReadyOrNotCharacter* PlacedBy;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = DoorJam)
	ADoor* PendingPlacement = nullptr;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = DoorJam)
	ADoor* JammedDoor = nullptr;

	UPROPERTY(EditAnywhere, Category = DoorJam)
	FName DoorJamSocket = "DoorJam";

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = DoorJam)
	USkeletalMesh* PlacedMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = DoorJam)
	float PlacementTimer = 0.3f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Multitool)
	float WedgeRemovalTime = 1.5f;

	// ICanUseMultitoolOn implementation
	//////////////////////////////////////
	virtual bool CanCancelMultitoolAction_Implementation() override { return true; }
	virtual EMultitoolFunctions GetMultitoolUseType_Implementation() override;
	virtual float GetMultitoolUseTime_Implementation() override;
	virtual void Server_FinishedUsingMultitool_Implementation(class AReadyOrNotCharacter* ToolOwner) override;
	//////////////////////////////////////

	// IUseability implementation
	//////////////////////////////////////
	virtual void Interact_Implementation(AReadyOrNotCharacter* InteractInstigator, class UInteractableComponent* InInteractableComponent) override;
	virtual void Fire_Implementation(AReadyOrNotCharacter* InteractInstigator, class UInteractableComponent* InInteractableComponent) override;
	virtual void EndFire_Implementation(AReadyOrNotCharacter* InteractInstigator, class UInteractableComponent* InInteractableComponent) override;
	virtual void OnFocusLost_Implementation(AReadyOrNotCharacter* InteractInstigator, class UInteractableComponent* InInteractableComponent) override;
	virtual FName DetermineAnimatedIcon_Implementation() const override;
	virtual bool CanInteractThroughHitActors_Implementation(const FHitResult& Hit) const override;
	//////////////////////////////////////

private:
	bool CanEquipMultitool(APlayerCharacter* PlayerCharacter) const;
	bool CanRemoveWedge(APlayerCharacter* PlayerCharacter) const;

	void ResetWedgeState();

	uint8 bDeploying : 1;
	uint8 bPlacementCanceled : 1;
};
