// Copyright Void Interactive, 2022

#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_SetHoleTraversalPose.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UAnimNotify_SetHoleTraversalPose : public UAnimNotify
{
	GENERATED_BODY()
	
protected:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup")
	UAnimSequence* Pose = nullptr;
};
