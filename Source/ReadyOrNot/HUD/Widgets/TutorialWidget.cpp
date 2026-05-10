// Copyright Void Interactive, 2023

#include "TutorialWidget.h"
#include "CommonInputSubsystem.h"

void UTutorialWidget::SetData(const FTutorialWidgetData& InData)
{
	Data = InData;

	RefreshWidget();
}

void UTutorialWidget::NativeConstruct()
{
	BindInputMethodChangedDelegate();
	
	Super::NativeConstruct();
}

void UTutorialWidget::NativeDestruct()
{
	Super::NativeDestruct();

	UnbindInputMethodChangedDelegate();
}

void UTutorialWidget::BindInputMethodChangedDelegate()
{
	if (UCommonInputSubsystem* InputSubsystem = GetInputSubsystem())
	{
		bUsingGamepad = InputSubsystem->GetCurrentInputType() == ECommonInputType::Gamepad;
		InputSubsystem->OnInputMethodChangedNative.AddUObject(this, &UTutorialWidget::OnInputMethodChanged);
	}
}

void UTutorialWidget::UnbindInputMethodChangedDelegate()
{
	if (UCommonInputSubsystem* InputSubsystem = GetInputSubsystem())
	{
		InputSubsystem->OnInputMethodChangedNative.RemoveAll(this);
	}
}

void UTutorialWidget::OnInputMethodChanged(const ECommonInputType InputMethod)
{
	bUsingGamepad = InputMethod == ECommonInputType::Gamepad;

	if (IsActivated())
	{
		RefreshWidget();
	}
}

FText UTutorialWidget::GetTitle() const
{
	return Data.Title;
}

FText UTutorialWidget::GetDescription() const
{
	FText Description = bUsingGamepad && Data.bGamepadDescription ? Data.GamepadDescription : Data.Description;
	TArray<FTutorialDescriptionInput> DescriptionInputs = bUsingGamepad && Data.bGamepadDescription ? Data.GamepadDescriptionInputs : Data.DescriptionInputs;

	for (FTutorialDescriptionInput InputAction : DescriptionInputs)
	{
		// Get the input action/axis key from the provided name and index
		FKey InputActionKey = UReadyOrNotFunctionLibrary::GetKeyFromInputActionName(InputAction.InputName, bUsingGamepad, InputAction.KeyIndex);
		if (!InputActionKey.IsValid())
			InputActionKey = UReadyOrNotFunctionLibrary::GetKeyFromInputAxisName(InputAction.InputName, bUsingGamepad, InputAction.KeyIndex);

		if (InputActionKey.IsValid())
		{
			FText InputActionText;

			// Generate the input action rich text from the key
			if (bUsingGamepad)
			{
				// TODO KobeT: Get the correct platform icon to display based on controller type
				const FString PlatformName = UGameplayStatics::GetPlatformName();
				const FText Key = PlatformName.Contains("PS") ? FText::FromString(InputActionKey.ToString() + "_PS") : FText::FromString(InputActionKey.ToString());

				const FTextFormat Format = FTextFormat::FromString("<img id=\"{Key}\"/>");
				InputActionText = FText::Format(Format, Key);
			}
			else
			{
				const FTextFormat Format = FTextFormat::FromString("<emphasisyellow>{Key}</>");
				InputActionText = FText::Format(Format, InputActionKey.GetDisplayName().ToUpper());
			}

			// Format the input action key into the description
			Description = FText::Format(FTextFormat(Description), InputActionText);
		}
	}

	return Description;
}
