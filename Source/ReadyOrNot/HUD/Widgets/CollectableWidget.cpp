// Copyright Void Interactive, 2023

#include "CollectableWidget.h"

#include "Actors/CollectableViewController.h"
#include "Input/CommonInputMode.h"

FReply UCollectableWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		if (IsValid(ParentController))
		{
			FVector2D Delta = InMouseEvent.GetCursorDelta();
			ParentController->AddRotationInput(Delta);

			return FReply::Handled();
		}
	}
	
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UCollectableWidget::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (IsValid(ParentController))
	{
		float Delta = InMouseEvent.GetWheelDelta();
		ParentController->AddZoomInput(Delta, false);
		
		return FReply::Handled();
	}
	
	return Super::NativeOnMouseWheel(InGeometry, InMouseEvent);
}

TOptional<FUIInputConfig> UCollectableWidget::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::All, EMouseCaptureMode::NoCapture);
}

void UCollectableWidget::CloseCollectableWidget()
{
	if (!ensure(IsValid(ParentController)))
	{
		RemoveFromParent();
		return;
	}

	ParentController->CloseViewer();
}
