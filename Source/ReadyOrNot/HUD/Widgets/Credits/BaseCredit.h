// Copyright Void Interactive, 2023

#pragma once

#include "BaseCredit.generated.h"

UCLASS()
class READYORNOT_API UBaseCredit : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category=Credits)
	bool bRevealed = false;

	UPROPERTY(BlueprintReadWrite, Category=Credits)
	float fAnimationSpeed = 1.0;
};