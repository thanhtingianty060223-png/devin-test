// Copyright Void Interactive, 2020

#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_ApplyMeleeDamage.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UAnimNotify_ApplyMeleeDamage : public UAnimNotify
{
	GENERATED_BODY()

protected:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
};
