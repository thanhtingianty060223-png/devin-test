// Copyright Void Interactive, 2022

#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_NextMag.generated.h"

UCLASS()
class READYORNOT_API UAnimNotify_NextMag : public UAnimNotify
{
	GENERATED_BODY()

protected:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
};
