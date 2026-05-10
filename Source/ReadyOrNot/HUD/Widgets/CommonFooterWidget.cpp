// Copyright Void Interactive, 2023

#include "CommonFooterWidget.h"
// ##UE5UPGRADE## CommonUI
#include "CommonInputSubsystem.h"
#include "Input/CommonUIInputTypes.h"

FText UCommonFooterWidget::GetFooterText() const
{
	if (DismissInputActionSettings.IsNull())
		return FText::FromString("No Input Action Selected");

	// Load the action data from the provided data table and row
	const FCommonInputActionDataBase* DismissInputActionData = CommonUI::GetInputActionData(DismissInputActionSettings);

	if (!DismissInputActionData)
		return FText::FromString("No Input Action Selected");

	// Get the current input type info for the input action (keyboard, gamepad, etc)
	const FCommonInputTypeInfo DismissInputInfo = DismissInputActionData->GetCurrentInputTypeInfo(GetInputSubsystem());

	const FText BaseFormat = GetBaseFormat(DismissInputActionData, DismissInputInfo);
	const FText InputKey = GetInputKey(DismissInputInfo);

	return FText::Format(BaseFormat, InputKey);
}

void UCommonFooterWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
}

void UCommonFooterWidget::NativeConstruct()
{
	OnActivated().AddUObject(this, &UCommonFooterWidget::BindDismissInputAction);
	OnActivated().AddUObject(this, &UCommonFooterWidget::BindInputMethodChangedDelegate);

	OnDeactivated().AddUObject(this, &UCommonFooterWidget::UnbindDismissInputAction);
	OnDeactivated().AddUObject(this, &UCommonFooterWidget::UnbindInputMethodChangedDelegate);

	Super::NativeConstruct();
}

void UCommonFooterWidget::NativeDestruct()
{
	Super::NativeDestruct();
}

void UCommonFooterWidget::BindDismissInputAction()
{
	if (DismissInputActionSettings.IsNull())
		return;

	if (!DismissInputActionHandle.IsValid())
	{
		FBindUIActionArgs BindArgs = FBindUIActionArgs(DismissInputActionSettings, false, FSimpleDelegate::CreateUObject(this, &UCommonFooterWidget::OnDismissInputActionNative));
		BindArgs.OnHoldActionProgressed.BindUObject(this, &UCommonFooterWidget::OnDismissInputActionProgressNative);
		BindArgs.bIsPersistent = bIsPersistent;
		BindArgs.bConsumeInput = bConsumeInput;

		DismissInputActionHandle = RegisterUIActionBinding(BindArgs);
	}
}

void UCommonFooterWidget::UnbindDismissInputAction()
{
	if (DismissInputActionSettings.IsNull())
		return;

	if (DismissInputActionHandle.IsValid())
	{
		DismissInputActionHandle.Unregister();
	}
}

void UCommonFooterWidget::BindInputMethodChangedDelegate()
{
	if (UCommonInputSubsystem* InputSubsystem = GetInputSubsystem())
	{
		OnInputMethodChanged(InputSubsystem->GetCurrentInputType());
		InputSubsystem->OnInputMethodChangedNative.AddUObject(this, &UCommonFooterWidget::OnInputMethodChanged);
	}
}

void UCommonFooterWidget::UnbindInputMethodChangedDelegate()
{
	if (UCommonInputSubsystem* InputSubsystem = GetInputSubsystem())
	{
		InputSubsystem->OnInputMethodChangedNative.RemoveAll(this);
	}
}

void UCommonFooterWidget::OnInputMethodChanged(const ECommonInputType InputMethod)
{
	bUsingGamepad = InputMethod == ECommonInputType::Gamepad;

	RefreshWidget();
}

void UCommonFooterWidget::OnDismissInputActionNative()
{
	BP_OnDismissInputAction();

	OnDismissInputAction.Broadcast();

	RefreshWidget();
}

void UCommonFooterWidget::OnDismissInputActionProgressNative(float HeldPercent)
{
	BP_DismissInputActionProgress(HeldPercent);
}

FText UCommonFooterWidget::GetBaseFormat(const FCommonInputActionDataBase* DismissInputActionData, const FCommonInputTypeInfo& DismissInputInfo) const
{
	// If the input action has an override text, use that
	if (bOverrideInputActionText)
		return OverrideInputActionText;

	// Get the base format for the text from the input action settings (either hold or normal name)
	return DismissInputInfo.bActionRequiresHold ? DismissInputActionData->HoldDisplayName : DismissInputActionData->DisplayName;
}

FText UCommonFooterWidget::GetInputKey(const FCommonInputTypeInfo& DismissInputInfo) const
{
	const FKey InputActionKey = DismissInputInfo.GetKey();

	if (bUsingGamepad)
	{
		const FTextFormat GamepadFormat = FTextFormat::FromString("<img id=\"{Key}\"/>");
		return FText::Format(GamepadFormat, FText::FromString(InputActionKey.ToString()));
	}

	const FTextFormat KeyboardFormat = FTextFormat::FromString("<Red>{Key}</>");
	return FText::Format(KeyboardFormat, InputActionKey.GetDisplayName().ToUpper());
}
