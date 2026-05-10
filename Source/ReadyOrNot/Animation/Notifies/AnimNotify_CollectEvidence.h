// Void Interactive, 2020

#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_CollectEvidence.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UAnimNotify_CollectEvidence : public UAnimNotify
{
	GENERATED_BODY()

protected:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

	UPROPERTY(EditAnywhere)
	uint8 bCollectFinished : 1;
};
