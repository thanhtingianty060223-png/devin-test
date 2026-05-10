// Void Interactive, 2020

#pragma once

#include "Actors/BaseItem.h"
#include "Detonator.generated.h"

UCLASS(Blueprintable, BlueprintType, Abstract)
class READYORNOT_API ADetonator : public ABaseItem
{
	GENERATED_BODY()

public:
	virtual void OnItemPrimaryUse() override;

	virtual void OnItemEndSecondaryUse() override;
	
	bool bSecondaryReleased = true;
	
	// The C2 charges that have been placed. Detonation will be attempted on these C2 charges.
	UPROPERTY(BlueprintReadOnly)
	TArray<class APlacedC2Explosive*> PlacedCharges;
	
	//Placed charges count
	UPROPERTY(BlueprintReadOnly)
	int32 PlacedChargesCount = 0;

	// The maximum distance that we can be from the placed charges before we can no longer blow them.
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	float MaxDetonationDistance = 2000.0f;

	virtual bool IsBlockingAnimationPlaying(TArray<EBlockingAnimationExclusion> Exclusions = {}) const override;

	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category = "Detonator")
			void Server_DetonateC2();
	virtual void Server_DetonateC2_Implementation();
	virtual bool Server_DetonateC2_Validate() { return true; }

	void EquipLastEquippedWeapon();
};
