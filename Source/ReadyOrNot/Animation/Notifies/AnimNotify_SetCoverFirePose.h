// Void Interactive, 2020

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_SetCoverFirePose.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UAnimNotify_SetCoverFirePose : public UAnimNotify
{
	GENERATED_BODY()

protected:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup")
	UAnimSequence* Pose = nullptr;
};
