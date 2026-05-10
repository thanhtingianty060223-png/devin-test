// Copyright Void Interactive, 2023


#include "Gamepad/SettingsMenuGamepad.h"

void USettingsMenuGamepad::SelectNewTab(TArray<UCommonButtonBase*> TabButtonsArray, int WidgetIndex, bool GoingToPreviousWidget)
{
	UCommonButtonBase* PreviousButton = TabButtonsArray[WidgetIndex];
	UCommonButtonBase* NextButton = TabButtonsArray[WidgetIndex];

	if (GoingToPreviousWidget && (WidgetIndex - 1) >= 0)
	{
		NextButton = TabButtonsArray[WidgetIndex - 1];
	}
	else if (!GoingToPreviousWidget && (WidgetIndex + 1) < TabButtonsArray.Num())
	{
		NextButton = TabButtonsArray[WidgetIndex + 1];
	}
	PreviousButton->ClearSelection();
	NextButton->SetIsSelected(true, false);
}
