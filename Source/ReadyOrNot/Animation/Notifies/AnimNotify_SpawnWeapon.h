// Void Interactive, 2020

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_SpawnWeapon.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UAnimNotify_SpawnWeapon : public UAnimNotify
{
	GENERATED_BODY()


	UPROPERTY(EditAnywhere)
		TArray<TSubclassOf<ABaseWeapon>> PotentialWeapons;
	
	protected:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
	
};
