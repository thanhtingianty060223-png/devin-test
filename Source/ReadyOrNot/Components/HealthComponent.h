// Copyright Void Interactive, 2021

#pragma once

#include "CoreMinimal.h"
#include "ResourceComponent.h"
#include "HealthComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class READYORNOT_API UHealthComponent : public UResourceComponent
{
	GENERATED_BODY()

public:
	UHealthComponent();
};
