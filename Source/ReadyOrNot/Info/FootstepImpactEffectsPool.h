// Copyright Void Interactive, 2023

#pragma once

#include "ObjectPoolBase.h"
#include "FootstepImpactEffectsPool.generated.h"

UCLASS(Abstract, BlueprintType, Blueprintable)
class READYORNOT_API UFootstepImpactEffectsPool : public UObjectPoolBase
{
	GENERATED_BODY()

public:
	UFootstepImpactEffectsPool();
};
