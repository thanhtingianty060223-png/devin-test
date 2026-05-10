// Void Interactive, 2020

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_ItemSwitchSocket.generated.h"

UENUM(BlueprintType)
enum class EItemSocket : uint8
{
	Body,
	Hand
};

/**
 * 
 */
UCLASS()
class READYORNOT_API UAnimNotify_ItemSwitchSocket : public UAnimNotify
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	EItemSocket DesiredItemSocket;
	
	void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation);
};
