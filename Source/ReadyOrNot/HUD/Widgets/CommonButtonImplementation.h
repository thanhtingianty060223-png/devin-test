#pragma once

#include "CommonButtonFMOD.h"
#include "CommonButtonImplementation.generated.h"

UCLASS(Abstract, meta = (DisableNativeTick))
class UCommonButtonImplementation : public UCommonButtonFMOD
{
	GENERATED_BODY()

protected:
	virtual void NativeOnCurrentTextStyleChanged() override;

	UFUNCTION(BlueprintImplementableEvent, Category = CommonButtonImplementation, meta = (DisplayName = "Get Button Label"))
	UCommonTextBlock* BP_GetButtonLabel() const;
};
