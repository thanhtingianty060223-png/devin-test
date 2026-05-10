// Void Interactive, 2020

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_WedgeDoor.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UAnimNotify_WedgeDoor : public UAnimNotify
{
	GENERATED_BODY()

protected:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

	UPROPERTY(EditAnywhere)
	uint8 bWedgeDeployFinished : 1;
};
