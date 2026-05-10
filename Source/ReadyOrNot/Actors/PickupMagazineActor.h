// Copyright Void Interactive, 2017

#pragma once

#include "Actors/PickupActor.h"
#include "PickupMagazineActor.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API APickupMagazineActor : public APickupActor
{
	GENERATED_BODY()

public:
	APickupMagazineActor();
	
	UFUNCTION(BlueprintCallable, Category = "Ammo")
	class ABaseMagazineWeapon* GetValidWeaponForPickerUpper(class APlayerCharacter* PlayerCharacter);

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Ammo")
			void Server_Pickup(AActor* InPickupInstigator);
	virtual void Server_Pickup_Implementation(AActor* InPickupInstigator);
	virtual bool Server_Pickup_Validate(AActor* InPickupInstigator) { return true; }

	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = "Ammo")
			void Multicast_SetWeapon(class ABaseMagazineWeapon* Weapon);
	virtual void Multicast_SetWeapon_Implementation(class ABaseMagazineWeapon* Weapon);

	void SetWeapon(class ABaseMagazineWeapon* Weapon);

	// Weapons with matching MagazineLabels will be able to pick this ammo up
	UPROPERTY(EditAnywhere, Replicated, BlueprintReadOnly, Category = "Ammo")
    FName MagazineLabel;

	// Minimum speed needed to trigger hit event
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound|Hit")
	float MinimumHitThreshold = 50.0f;

	// The event that plays when the magazine hits the ground
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound|Hit")
	UFMODEvent* DroppedMagazineHitEvent;

	virtual bool CanPickUpNow(class APlayerCharacter* PickerUpper) override;
	
	UFUNCTION()
	virtual void OnComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

protected:
	virtual void Tick(float DeltaSeconds) override;
	virtual void ActorPickedUp(AActor* InPickupInstigator) override;

	// Begin IUseabilityInterface
	virtual void Interact_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent) override;	
	virtual FName DetermineAnimatedIcon_Implementation() const override;
	// End IUseabilityInterface

private:
	UPROPERTY(Replicated)
	FMagazine MagazineData = FMagazine();

	int32 ImpactSoundsPlayed = 0;

	// make it only work with a specific weapon.. can only be picked up by the player that dropped it?
	UPROPERTY(Replicated)
    class ABaseMagazineWeapon* CameFromWeapon;
};
