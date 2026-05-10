// Void Interactive, 2020

#pragma once

#include "Actors/Gameplay/ThrownItem.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_SpawnThrownItem.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UAnimNotify_SpawnThrownItem : public UAnimNotify
{
	GENERATED_BODY()

protected:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

	UPROPERTY(EditAnywhere)
	TSubclassOf<ABaseItem> ItemClass = nullptr;
	
	UPROPERTY(EditAnywhere)
	uint8 bLocalOnly : 1;

	UPROPERTY(EditAnywhere)
	uint8 bNonLocalOnly : 1;
	
	UPROPERTY(EditAnywhere)
	FName BoneToSpawnOn = "hand_RI";
	
	UPROPERTY(EditAnywhere)
	uint8 bCustomThrowDirection : 1;
	
	UPROPERTY(EditAnywhere, meta = (EditCondition = "bCustomThrowDirection"))
	FRotator ThrowDirection = FRotator::ZeroRotator;
};
