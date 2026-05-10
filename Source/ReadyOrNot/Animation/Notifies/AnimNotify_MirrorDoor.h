// Void Interactive, 2020

#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_MirrorDoor.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UAnimNotify_MirrorDoor : public UAnimNotify
{
	GENERATED_BODY()

protected:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
	
	UPROPERTY(EditAnywhere)
	uint8 bMirrorFinish : 1;
};
