// Copyright Void Interactive, 2017

#pragma once

#include "GameFramework/DamageType.h"
#include "TrapDamage.generated.h"

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class READYORNOT_API UTrapDamage : public UDamageType
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	uint8 bDestroyAllDoorChunks : 1;
};
