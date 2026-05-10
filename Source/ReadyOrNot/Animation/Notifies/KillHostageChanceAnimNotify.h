// Copyright Void Interactive, 2021

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "KillHostageChanceAnimNotify.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UKillHostageChanceAnimNotify : public UAnimNotify
{
	GENERATED_BODY()

		void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
	
};
