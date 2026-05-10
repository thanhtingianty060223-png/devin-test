// Void Interactive, 2020

#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_EquipItemOfClass.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UAnimNotify_EquipItemOfClass : public UAnimNotify
{
	GENERATED_BODY()

protected:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

	UPROPERTY(EditAnywhere)
	TSubclassOf<ABaseItem> ItemClass = nullptr;
	
	UPROPERTY(EditAnywhere)
	uint8 bInstant : 1;
	
	UPROPERTY(EditAnywhere)
	uint8 bAIOnly : 1;
};
