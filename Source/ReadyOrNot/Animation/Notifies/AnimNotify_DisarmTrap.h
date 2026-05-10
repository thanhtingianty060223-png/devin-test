// Void Interactive, 2020

#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_DisarmTrap.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UAnimNotify_DisarmTrap : public UAnimNotify
{
	GENERATED_BODY()

protected:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

	UPROPERTY(EditAnywhere)
	uint8 bDisarmFinished : 1;
};
