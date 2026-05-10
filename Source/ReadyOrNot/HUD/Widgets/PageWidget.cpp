// Copyright Void Interactive, 2023


#include "HUD/Widgets/PageWidget.h"
#include "Input/CommonUIInputTypes.h"
#include "ICommonInputModule.h"


void UPageFooter::GetInputActionData(FDataTableRowHandle InputActionRow, FText& ActionName, FKey& ActionKey)
{
	if (InputActionRow.IsNull())
		return;

	// Load the action data from the provided data table and row
	const FCommonInputActionDataBase* InputActionData = CommonUI::GetInputActionData(InputActionRow);

	if (!InputActionData)
		return;

	// Get the current input type info for the input action (keyboard, gamepad, etc)
	ECommonInputType CurrentInputType = ECommonInputType::MouseAndKeyboard;
	const FCommonInputTypeInfo InputInfo = InputActionData->GetCurrentInputTypeInfo(GetInputSubsystem());


	//Return Input Action Data
	ActionKey = InputInfo.GetKey();
	ActionName = InputActionData->DisplayName;

}
