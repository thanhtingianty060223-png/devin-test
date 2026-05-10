// Copyright Void Interactive, 2022

#pragma once

#include "CoreMinimal.h"
#include "Actors/DynamicWorldItem.h"
#include "DynamicInteractableWorldItem.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API ADynamicInteractableWorldItem : public AActor, public IUseabilityInterface
{
	GENERATED_BODY()

public:
	ADynamicInteractableWorldItem();
	
protected:
	virtual void BeginPlay() override;

	/**
	 * Whether the item's sound should restart everytime the item is toggled on.
	 */
	UPROPERTY(EditAnywhere, Category = "Tweaks")
	bool bRestartOnToggle = false;
	
	/**
	 * Whether the item can be toggled on/off when destroyed.
	 */
	UPROPERTY(EditAnywhere, Category = "Tweaks")
	bool bCanToggleIfDestroyed = false;

	/**
	 * Whether the item is on by default.
	 */
	UPROPERTY(EditAnywhere, Category = "Tweaks")
	bool bItemOn = false;

	/**
	 * Whether the item is destroyed by default.
	 */
	UPROPERTY(EditAnywhere, Category = "Tweaks")
	bool bItemDestroyed = false;
	
	UPROPERTY(EditAnywhere, Category = "Tweaks")
	UInteractableComponent* InteractableComponent;

	/**
	 * Audio to play when the item is interacted with.
	 */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	UFMODAudioComponent* InteractAudioFMOD;

	/**
	 * Audio used for when the item is running while intact.
	 */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	UFMODAudioComponent* IntactRunningAudioFMOD1;

	/**
	 * Audio used for when the item is running while destroyed.
	 */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	UFMODAudioComponent* DestroyedRunningAudioFMOD;

	/**
	 * Audio played when the item is impacted.
	 */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	UFMODAudioComponent* ImpactAudioFMOD;
	
	/**
	 * Materials used when the item is intact and on. Array index correlates to material slot.
	 */
	UPROPERTY(EditAnywhere, Category = "Material")
	TArray<UMaterialInterface*> IntactOnMaterials;

	/**
	 * Materials used when the item is intact and off. Array index correlates to material slot.
	 */
	UPROPERTY(EditAnywhere, Category = "Material")
	TArray<UMaterialInterface*> IntactOffMaterials;

	/**
	 * Materials used when the item is destroyed and on. Array index correlates to material slot.
	 */
	UPROPERTY(EditAnywhere, Category = "Material")
	TArray<UMaterialInterface*> DestroyedOnMaterials;

	/**
	 * Materials used when the item is destroyed and odd. Array index correlates to material slot.
	 */
	UPROPERTY(EditAnywhere, Category = "Material")
	TArray<UMaterialInterface*> DestroyedOffMaterials;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* ItemMesh;

	UPROPERTY(EditAnywhere, Category = "Tweaks")
	UStaticMesh* PostDestructionMesh;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	UParticleSystemComponent* ImpactParticle;

	UPROPERTY(EditAnywhere, Category = "Tweaks")
	UMaterialInterface* PhysicsImpactDecal;

	UPROPERTY(EditAnywhere, Category = "Tweaks")
	float PhysicsImpactDecalScale = 1.0f;

	
public:
	
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	virtual void Interact_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent) override;

	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit);
	
	bool bItemDestroyedLocally = false;

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_DestroyItem();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_ItemStateToggle();
	
	// For further Blueprint integration
	UFUNCTION(BlueprintImplementableEvent, Category = "Damage")
	void OnItemDestroyed();

	UFUNCTION(BlueprintImplementableEvent, Category = "Functionality")
	void OnItemStateToggled();


private:
	bool bUsingBeginningMaterials = true;
	void SwitchMaterials();
};
