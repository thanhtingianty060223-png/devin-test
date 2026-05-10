// Void Interactive, 2020

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_ChangeBodySocket.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UAnimNotify_ChangeBodySocket : public UAnimNotify
{
	GENERATED_BODY()

	// item to swap (this will only swap non equipped items)
	UPROPERTY(EditAnywhere)
	EItemCategory ItemCategory;

	// leave none to return to default socket
	UPROPERTY(EditAnywhere)
	FName Socket;
	

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
};
