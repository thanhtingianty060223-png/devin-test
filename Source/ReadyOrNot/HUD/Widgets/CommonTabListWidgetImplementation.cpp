#include "CommonTabListWidgetImplementation.h"

void UCommonTabListWidgetImplementation::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	AnimationSwitcher = NewObject<UCommonTabListAnimationSwitcher>();
	SetLinkedSwitcher(AnimationSwitcher);
	SetListeningForInput(true);
}
