// Copyright Void Interactive, 2021

#pragma once

#include "Actors/BaseItem.h"
#include "C2Explosive.generated.h"

/**
 * 
 */
UCLASS(Abstract)
class READYORNOT_API AC2Explosive : public ABaseItem
{
	GENERATED_BODY()
	
public:
	AC2Explosive();

	virtual void OnItemPrimaryUse() override;
	virtual void OnItemSecondaryUsed() override;
	virtual bool CanEquip(AReadyOrNotCharacter* ToCharacter) const override;
	virtual bool IsBlockingAnimationPlaying(TArray<EBlockingAnimationExclusion> Exclusions = {}) const override;

	UPROPERTY(BlueprintReadOnly, Category = "C2 Explosive")
	bool bIsValidPlacement = false;
	
	UPROPERTY(BlueprintReadOnly, Category = "C2 Explosive")
	FHitResult LastGoodPlacement;

	// Server - start placing the C2 explosive. Literally just calls the multicast function.
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = C2)
			void Server_StartC2Placement(AActor* Actor);
	virtual void Server_StartC2Placement_Implementation(AActor* Actor);
	virtual bool Server_StartC2Placement_Validate(AActor* Actor) { return true; }

	// Server - finishes placing the C2 explosive.
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = C2)
			void Server_FinishC2Placement();
	virtual void Server_FinishC2Placement_Implementation();
	virtual bool Server_FinishC2Placement_Validate() { return true; }

	// Client - explosive placement finished, make the icon vanish
	UFUNCTION(Client, Reliable, BlueprintCallable, Category = C2)
			void Client_C2PlacementFinished();
	virtual void Client_C2PlacementFinished_Implementation();
	
	// Multicast - start placing the C2 explosive. This makes the owning player play a placement animation.
	UFUNCTION(NetMulticast, Reliable, Category = C2)
			void Multicast_StartPlaceC2Explosive();
	virtual void Multicast_StartPlaceC2Explosive_Implementation();
	
	UFUNCTION()
	void OnRep_LastPlacedC2Explosive();
	
	UPROPERTY(ReplicatedUsing = OnRep_LastPlacedC2Explosive)
	class APlacedC2Explosive* LastPlacedC2Explosive = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "C2 Explosive")
	float MaxPlacementDistance = 100.0f;

	UPROPERTY(BlueprintReadOnly, Replicated)
	AActor* CurrentActorPlacement = nullptr;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = C2)
	void EquipDetonator(bool bFromExplosives) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C2 Explosive")
	TSubclassOf<class APlacedC2Explosive> PlacedC2Class;
};
