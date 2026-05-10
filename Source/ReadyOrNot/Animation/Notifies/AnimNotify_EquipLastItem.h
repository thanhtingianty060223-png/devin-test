// Void Interactive, 2020

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_EquipLastItem.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UAnimNotify_EquipLastItem : public UAnimNotify
{
	GENERATED_BODY()

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

	//UPROPERTY(EditAnywhere)
	//EHolsterAnimationType HolsterType = EHolsterAnimationType::HAT_Normal;
	UPROPERTY(EditAnywhere)
	uint8 bInstant : 1;
};
