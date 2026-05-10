// Void Interactive, 2020

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_DoorKickBreak.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UAnimNotify_DoorKickBreak : public UAnimNotify
{
	GENERATED_BODY()

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
	
};
