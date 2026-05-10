// Copyright Void Interactive, 2021

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_SetControlRotationToCamBone.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API USetControlRotationToCamBoneAnimNotify : public UAnimNotify
{
	GENERATED_BODY()

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

	UPROPERTY(EditAnywhere, Category = "Main")
	FName CameraBoneName = "fp_camera";
};
