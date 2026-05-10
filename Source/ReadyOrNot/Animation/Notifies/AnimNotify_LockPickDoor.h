// Void Interactive, 2020

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_LockPickDoor.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UAnimNotify_LockPickDoor : public UAnimNotify
{
	GENERATED_BODY()

protected:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

	UPROPERTY(EditAnywhere)
	uint8 bLockPickFinished : 1;
};
