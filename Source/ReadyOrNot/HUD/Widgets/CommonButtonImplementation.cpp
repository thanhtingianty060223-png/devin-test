#include "CommonButtonImplementation.h"
#include "CommonTextBlock.h"

void UCommonButtonImplementation::NativeOnCurrentTextStyleChanged()
{
	Super::NativeOnCurrentTextStyleChanged();
	
	UCommonTextBlock* Label = BP_GetButtonLabel();

	if (Label != nullptr)
		Label->SetStyle(GetCurrentTextStyleClass());
}
