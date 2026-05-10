// Void Interactive, 2020

#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_PlayPostProcessEffect.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UAnimNotify_PlayPostProcessEffect : public UAnimNotify
{
	GENERATED_BODY()

protected:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

	UPROPERTY(EditAnywhere, Category = "Anim Notify")
	FName PostProcessEffectName = NAME_None;
};
