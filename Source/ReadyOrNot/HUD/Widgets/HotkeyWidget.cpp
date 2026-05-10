// Void Interactive, 2020

#include "HotkeyWidget.h"

#include "Components/TextBlock.h"
#include "Components/Image.h"

void UHotkeyWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	
	SetHotkeyText(InputName);
	SetHotkeyImage(Icon);

	RemainingUses_Text->SetVisibility(bShowRemainingUses ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	
	if (IsDesignTime())
	{
		SetHotkeyRemainingUses(99);
	}
}

void UHotkeyWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetHotkeyText(InputName);
	SetHotkeyImage(Icon);

	RemainingUses_Text->SetVisibility(bShowRemainingUses ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);

	#if WITH_EDITOR
	ensureAlways(Hotkey_Text && Hotkey_Image);
	#endif
}

void UHotkeyWidget::RefreshHotkey()
{
	SetHotkeyText(InputName);
}

void UHotkeyWidget::PlayPressedAnimation()
{
	if (!bCustomAnimation)
		PlayAnimationForward(Anim_Pressed);
}

void UHotkeyWidget::PlayReleasedAnimation()
{
	if (!bCustomAnimation)
		PlayAnimationForward(Anim_Released);
}

void UHotkeyWidget::SetHotkeyText(const FName& HotkeyInputName)
{
	const FString RonKeyName = UReadyOrNotFunctionLibrary::ConvertUnrealKeyNameToRonKeyName(UReadyOrNotFunctionLibrary::GetKeyFromInputActionName(HotkeyInputName));

	Hotkey_Text->SetText(FText::FromString(RonKeyName));
}

void UHotkeyWidget::SetHotkeyImage(const FSlateBrush& Brush)
{
	Hotkey_Image->SetBrush(Brush);
}

void UHotkeyWidget::SetHotkeyRemainingUses(const int32 InRemainingUses)
{
	if (InRemainingUses <= 0 || !bShowRemainingUses)
	{
		RemainingUses_Text->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	FString RemainingUsesString = FString::FromInt(InRemainingUses);
	
	if (InRemainingUses > 99)
		RemainingUsesString += "+";
	
	RemainingUses_Text->SetText(FText::FromString(RemainingUsesString));
	RemainingUses_Text->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}
