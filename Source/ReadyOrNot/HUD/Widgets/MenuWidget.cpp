#include "MenuWidget.h"

void UMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	bAutoActivate = true;
}

void UMenuWidget::NativeOnShow()
{
	BP_OnShow();
}

void UMenuWidget::PlayWidgetAnimation_Internal(UWidgetAnimation* InWidgetAnimation, const bool bRestartIfAlreadyPlaying)
{
	if (!IsAnimationPlaying(InWidgetAnimation))
	{
		PlayAnimation(InWidgetAnimation);
		return;
	}
	
	if (bRestartIfAlreadyPlaying)
	{
		StopAnimation(InWidgetAnimation);
		PlayAnimation(InWidgetAnimation);
	}
}
