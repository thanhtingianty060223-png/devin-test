// Copyright Void Interactive, 2018
// Author: Alexander Mijalkovski

#pragma once

#include "Animation/AnimInstance.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/PoseAsset.h"
#include "ReadyOrNotFaceAnimInstance.generated.h"

// used for directional animation blending


UCLASS(transient, Blueprintable, hideCategories = AnimInstance, BlueprintType)
class UReadyOrNotFaceAnimInstance : public UAnimInstance
{
	GENERATED_UCLASS_BODY()

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	APlayerCharacter* TryGetAnyOwner();

	// use for last tick values
	virtual void NativeLastTick(float DeltaSeconds);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Core")
	USkeletalMeshComponent* BodyDriverMesh;

	/* Retrieve current character face ROM based on data asset entry */
	UFUNCTION(BlueprintPure, Category = Animation, meta = (BlueprintThreadSafe))
	UPoseAsset* GetFaceROM() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Core")
	UPoseAsset* DefaultFaceROMData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	FRotator FocalTargetLookRotation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	FRotator HeadLookRotation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	float EyeTargetLookLeft;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	float EyeTargetLookRight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	float EyeTargetLookUp;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	float EyeTargetLookDown;
};
