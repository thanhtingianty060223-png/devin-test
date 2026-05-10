// Copyright Void Interactive, 2021

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "FireOnDroppedAnimNotify.generated.h"

UCLASS()
class READYORNOT_API UFireOnDroppedAnimNotify : public UAnimNotify
{
	GENERATED_BODY()
	
protected:
	void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire On Dropped Notify", meta = (ClampMin = 0.0f, ClampMax = 100.0f))
	float ChanceToFire = 30.0f;
};
