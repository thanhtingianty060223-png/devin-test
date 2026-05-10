// Void Interactive, 2020

#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_AIThrowPendingItem.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UAnimNotify_AIThrowPendingItem : public UAnimNotify
{
	GENERATED_BODY()

protected:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

	UPROPERTY(EditAnywhere)
	FName BoneToSpawnOn = "hand_RI";
	
	UPROPERTY(EditAnywhere)
	FVector LandingLocation = FVector::ZeroVector;
	
	UPROPERTY(EditAnywhere)
	FTransform RelativeTransform;
	
	UPROPERTY(EditAnywhere)
	uint8 bCustomThrowDirection : 1;
	
	UPROPERTY(EditAnywhere, meta = (EditCondition = "bCustomThrowDirection"))
	FRotator ThrowDirection = FRotator::ZeroRotator;
};
