#pragma once

#include "CommonTabListWidgetBase.h"
#include "CommonTabListAnimationSwitcher.h"
#include "CommonTabListWidgetImplementation.generated.h"

UCLASS(Abstract, meta = (DisableNativeTick))
class UCommonTabListWidgetImplementation : public UCommonTabListWidgetBase
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	
	UCommonTabListAnimationSwitcher *AnimationSwitcher;
};
