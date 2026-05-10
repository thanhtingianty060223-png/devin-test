#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "FootstepNativeAnimNotify.generated.h"

UCLASS()
class UFootstepNativeAnimNotify : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
};