
// Void Interactive, 2020

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_ToggleEquipmentVis.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UAnimNotify_ToggleEquipmentVis : public UAnimNotify
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	EToggleInventoryVis InventroyVis;

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
	
};
