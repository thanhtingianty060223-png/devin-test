// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "Input/CommonBoundActionButton.h"
#include "CommonIconButtonBase.generated.h"

class UCommonActionWidget;
/**
 * 
 */
UCLASS()
class READYORNOT_API UCommonIconButtonBase : public UCommonBoundActionButton
{
	GENERATED_BODY()
	
public:
	virtual bool Initialize() override;
};
