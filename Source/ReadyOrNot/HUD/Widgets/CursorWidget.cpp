// Void Interactive, 2020

#include "CursorWidget.h"

#include "Components/WidgetSwitcher.h"

#include "Blueprint/WidgetLayoutLibrary.h"

void UCursorWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	float MouseX, MouseY;
	UWidgetLayoutLibrary::GetMousePositionScaledByDPI(GetOwningPlayer(), MouseX, MouseY);
	SetPositionInViewport({MouseX, MouseY}, false);
}
