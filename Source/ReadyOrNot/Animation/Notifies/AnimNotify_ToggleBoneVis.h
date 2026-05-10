// Void Interactive, 2020

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotify_ToggleBoneVis.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UAnimNotifyState_HideBoneVis : public UAnimNotifyState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FName BoneNameToHide = NAME_None;
	
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

	
};
