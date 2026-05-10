// Copyright Void Interactive, 2021

#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "DropWeaponAnimNotify.generated.h"

UCLASS()
class READYORNOT_API UDropWeaponAnimNotify : public UAnimNotify
{
	GENERATED_BODY()

protected:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
};
