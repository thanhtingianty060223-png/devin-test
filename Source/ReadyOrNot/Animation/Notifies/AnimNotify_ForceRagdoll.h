// Void Interactive, 2022
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_ForceRagdoll.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UAnimNotify_ForceRagdoll : public UAnimNotify
{
	GENERATED_BODY()	
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
};
