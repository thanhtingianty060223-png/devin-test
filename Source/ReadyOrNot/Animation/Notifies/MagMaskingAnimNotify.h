// Copyright Void Interactive, 2021

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "MagMaskingAnimNotify.generated.h"


UENUM(BlueprintType)
enum class EMaskMag : uint8
{
	Mag01,
	Mag02,
	Dummy
};

UENUM(BlueprintType)
enum class EMaskMagState : uint8
{
	Show,
	Hide
};

/**
 * 
 */
UCLASS()
class READYORNOT_API UMagMaskingAnimNotify : public UAnimNotify
{
	GENERATED_BODY()

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

	UPROPERTY(EditAnywhere)
	bool bIsFirstPerson;

	UPROPERTY(EditAnywhere)
	EMaskMag MaskMag;

	UPROPERTY(EditAnywhere)
	EMaskMagState MagState;

	UPROPERTY(EditAnywhere)
	bool bDummyCopyMag02;
};
