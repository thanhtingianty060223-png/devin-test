// Copyright Void Interactive, 2017

#pragma once

#include "GameFramework/Actor.h"
#include "Interfaces/CanUseMultitoolOn.h"
#include "PlacedC2Explosive.generated.h"

UCLASS(Blueprintable, BlueprintType)
class READYORNOT_API APlacedC2Explosive : public AActor/*, public ICanUseMultitoolOn*/, public IUseabilityInterface
{
	GENERATED_BODY()

protected:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = C2, meta = (AllowPrivateAccess = "true"))
	class UParticleSystemComponent* ExplosionComp;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = C2, meta = (AllowPrivateAccess = "true"))
	class USkeletalMeshComponent* MeshComp;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	class UFMODAudioComponent* AudioComponent = nullptr;
	
public:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = C2, meta = (AllowPrivateAccess = "true"))
	class UInteractableComponent* C2InteractableComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = C2)
	UAIPerceptionStimuliSourceComponent* PerceptionStimuliComp;

	UPROPERTY()
	ARadialForceActor* RadialForce = nullptr;

	UPROPERTY()
	class AC2Explosive* ConnectedC2Explosive = nullptr;

	APlacedC2Explosive();

	UPROPERTY(Replicated)
	AController* PlacedByController;

	// C2 in-inventory class used to spawn this placed instance
	UPROPERTY()
	TSubclassOf<ABaseItem> ItemInventoryClass;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = C2)
	USkeletalMeshComponent* GetMeshComp() { return MeshComp; }

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;

	// How much damage to inflict
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "C2|Damage")
	float DamageToInflict = 120.0f;

	// How much damage to inflict (minimum)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "C2|Damage")
	float MinDamageToInflict = 5;

	// What kind of damage to inflict
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C2|Damage")
	TSubclassOf<UDamageType> DamageType;

	// How wide of a damage radius
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "C2|Damage")
	float DamageInnerRadius = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "C2|Damage")
	float DamageOuterRadius = 600.0f;
	// How much time it takes for the C2 actor to be removed after it has blown up
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = C2)
	float ExplosionPostKillTime = 15.0f;

	// Whether the C2 has been detonated or not
	UPROPERTY(BlueprintReadOnly, Category = C2)
	bool bDetonated = false;

	// How much damage the C2 inflicts towards door integrity
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = C2)
	float DoorIntegrityDamage = 2.0f;

	// The function that gets called when the removal action occurs
	UFUNCTION(BlueprintCallable, Category = C2)
	void PostExplosionKill();

	// Detonate the C2.
	UFUNCTION(BlueprintCallable, Reliable, Server, WithValidation, Category = C2)
			void Server_DetonateC2();
	virtual void Server_DetonateC2_Implementation();
	virtual bool Server_DetonateC2_Validate() { return true; }

	// Triggered when the C2 detonates.
	UFUNCTION(BlueprintCallable, NetMulticast, Reliable, Category = C2)
			void Multicast_OnC2Detonated();
	virtual void Multicast_OnC2Detonated_Implementation();

	// Stuff that we need to set on spawn!
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Replicated, Category = C2)
	AActor* TargetItem;

	// FMOD explosion audio to use when not set by the door
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = C2)
	UFMODEvent* FMODC2ExplosionAudio;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Replicated, Category = C2)
	FHitResult PlacementHit;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Grenade)
	bool bUseScreenShake = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Grenade)
	TSubclassOf<ULegacyCameraShake> ExplosionScreenShake;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Grenade)
	UCameraShakeBase* ExplosionScreenShakeInst;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Grenade)
	float CameraShakeRadius = 1000.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "C2")
	TSubclassOf<UStunDamage> StunDamageType;

	UPROPERTY(BlueprintReadOnly, Category = C2)
	bool bRemovedViaMultitool = false;

	FTimerHandle DestroyAfterRemoval_Handle;
	void DestroyAfterRemoval();

	UFUNCTION(BlueprintCallable)
	void RemoveFromTarget();

	//UFUNCTION(NetMulticast, Reliable, Category = C2)
	//		void Multicast_OnRemovedViaToolkit();
	//virtual void Multicast_OnRemovedViaToolkit_Implementation();

	bool CanRemoveC2(APlayerCharacter* PlayerCharacter) const;
	
	// ICanUseMultitoolOn
	////////////////////////////
	/*virtual void Server_FinishedUsingMultitool_Implementation(class AReadyOrNotCharacter* ToolOwner) override;
	virtual void Client_FinishedUsingMultitool_Implementation(class AReadyOrNotCharacter* ToolOwner) override;
	virtual bool CanUseMultitoolNow_Implementation(class AReadyOrNotCharacter* ToolOwner, class AMultitool* Tool, FHitResult TraceHit) override;
	virtual bool CanCancelMultitoolAction_Implementation() override { return true; }
	virtual EMultitoolFunctions GetMultitoolUseType_Implementation() override;
	virtual float GetMultitoolUseTime_Implementation() override;*/
	////////////////////////////

	UPROPERTY(Replicated)
	bool bIsBeingRemoved = false;
	UPROPERTY(Replicated)
	class AReadyOrNotCharacter* IsBeingRemovedBy;
	// IUseabilityInterface
	////////////////////////////
	virtual void Interact_Implementation(AReadyOrNotCharacter* InteractInstigator, class UInteractableComponent* InteractableComponent) override;
	virtual void EndInteract_Implementation(AReadyOrNotCharacter* InteractInstigator, class UInteractableComponent* InteractableComponent) override;
	virtual bool CanInteract_Implementation() const override;
	virtual bool CanInteractThroughHitActors_Implementation(const FHitResult& Hit) const override;
	////////////////////////////

	// Sound occlusion parameters
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound | Occlusion")
	float C2OcclusionMultiplier = 1.0f;

	// Depth to fully occlude gunshots (in cm)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound | Occlusion")
	float C2FullOcclusionDepth = 150.0f;

protected:
	void ReturnToInventory();

private:
	FTimerHandle TH_Removal;
};
