// Void Interactive, 2020

#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_OnLandmarkProxyUse.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UAnimNotify_OnLandmarkProxyUse : public UAnimNotify
{
	GENERATED_BODY()

protected:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
};
