// Void Interactive, 2020

#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_ForceFireWeapon.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UAnimNotify_ForceFireWeapon : public UAnimNotify
{
	GENERATED_BODY()

protected:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

	UPROPERTY(EditAnywhere, Category = "Force Fire", meta = (ClampMin = 0.0f, ClampMax = 1.0f, UIMin = 0.0f, UIMax = 1.0f))
	float Chance = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Force Fire")
	bool bNoEnemyRequired = false;
};
