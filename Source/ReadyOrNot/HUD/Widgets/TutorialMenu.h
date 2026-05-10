// Void Interactive, 2020

#pragma once

#include "BaseWidget.h"
#include "lib/BpGameplayHelperLib.h"
#include "TutorialMenu.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UTutorialMenu : public UBaseWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;

	// total seconds  the menu has  been opened
	float TotalSeconds = 0.0f;
};
