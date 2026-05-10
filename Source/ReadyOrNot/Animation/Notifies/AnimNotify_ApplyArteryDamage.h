// Copyright Void Interactive, 2022

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_ApplyArteryDamage.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UAnimNotify_ApplyArteryDamage : public UAnimNotify
{
	GENERATED_BODY()

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

	UPROPERTY(EditAnywhere)
	FName ArteryBoneName = NAME_None;
};
