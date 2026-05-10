// Copyright Void Interactive, 2023

#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_ArrestComplete.generated.h"

UCLASS()
class READYORNOT_API UAnimNotify_ArrestComplete final : public UAnimNotify
{
	GENERATED_BODY()

protected:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
};
