// Copyright Void Interactive, 2022

#pragma once

#include "Engine/DataAsset.h"
#include "AIActionPresetData.generated.h"

UCLASS(BlueprintType, NotBlueprintable)
class READYORNOT_API UAIActionPresetData final : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Preset")
	FAIActionData Action;
};
