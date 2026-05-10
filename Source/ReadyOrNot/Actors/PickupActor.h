// Copyright Void Interactive, 2021

#pragma once

#include "GameFramework/Actor.h"
#include "Interfaces/UseabilityInterface.h"
#include "PickupActor.generated.h"

UCLASS(HideCategories=("Collision", "Input", "Actor", "LOD", "Cooking"))
class READYORNOT_API APickupActor : public AActor, public IUseabilityInterface
{
	GENERATED_BODY()
	
public:	
	APickupActor();

	UFUNCTION(BlueprintCallable, Category = "Evidence Actor")
	virtual void ActorPickedUp(AActor* InPickupInstigator);

	UFUNCTION(BlueprintCallable, Category = "Evidence Actor")
	virtual void ActorDropped(AActor* InDroppedInstigator);

	UFUNCTION(BlueprintCallable, Category = "Evidence Actor")
	virtual void ToggleObjectiveMarker();
	
	UFUNCTION(BlueprintCallable, Category = "Evidence Actor")
    virtual void ShowObjectiveMarker();
	
	UFUNCTION(BlueprintCallable, Category = "Evidence Actor")
    virtual void HideObjectiveMarker();

	UFUNCTION(BlueprintPure, Category = PickupActor)
    FORCEINLINE AActor* GetPickupInstigator() const { return PickupInstigator; }
    
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActorPickedUp, AActor*, PickedActor);
	UPROPERTY(BlueprintAssignable, Category = Events)
	FOnActorPickedUp OnActorPickedUp;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnActorPickedUp_NoParam);
	UPROPERTY(BlueprintAssignable, Category = Events)
	FOnActorPickedUp_NoParam OnActorPickedUp_NoParam;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActorDropped, AActor*, DroppedActor);
	UPROPERTY(BlueprintAssignable, Category = Events)
	FOnActorDropped OnActorDropped;
	
protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	FName PickupName = "Actor";
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	class USkeletalMeshComponent* SkeletalMesh;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	class UStaticMeshComponent* StaticMesh;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UObjectiveMarkerComponent* ObjectiveMarkerComponent = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UInteractableComponent* InteractableComponent = nullptr;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Pickup Actor")
	AActor* PickupInstigator = nullptr;

	// If this function returns true, then we can pick the thing up now
	UFUNCTION(BlueprintPure, Category = PickupActor)
	virtual bool CanPickUpNow(class APlayerCharacter* PickerUpper) { return true; }
	
	// Begin IUseabilityInterface
	virtual void Interact_Implementation(AReadyOrNotCharacter* InteractInstigator, class UInteractableComponent* InInteractableComponent) override; 
	// End IUseabilityInterface
};
