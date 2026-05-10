// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LoadoutUnitSelectWidget.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API ULoadoutUnitSelectWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void Init();
};
