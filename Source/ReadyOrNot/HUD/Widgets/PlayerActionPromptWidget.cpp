// Void Interactive, 2020

#include "PlayerActionPromptWidget.h"

#include "Components/RichTextBlock.h"
#include "Components/Overlay.h"

void UPlayerActionPromptWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (IsDesignTime())
	{
		Action_RichText->SetText(ActionText);
	}
	else
	{
		Action_RichText->SetText(FText::FromString(""));
	}
}

void UPlayerActionPromptWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetVisibility(ESlateVisibility::Collapsed);
}

void UPlayerActionPromptWidget::UpdateActionSlot(const FText& InText, const bool clearText, const bool bAnimate, const bool bLoopAnimation)
{
	if (clearText) {
		ActionText = FText::FromString("");

		Action_RichText->SetText(ActionText);

		bInUse = false;

		SetVisibility(ESlateVisibility::Collapsed);
	}
	else {
		ActionText = InText;

		Action_RichText->SetText(InText);


		bInUse = true;

		SetVisibility(ESlateVisibility::HitTestInvisible);

		if (bAnimate)
			PlayAnimation(Anim_OnShow, 0.0f, bLoopAnimation ? 0 : 1, EUMGSequencePlayMode::Forward, 1.25f);

	}
	
}

void UPlayerActionPromptWidget::UpdateText(const FText& InText, const bool bAnimate, const bool bLoopAnimation)
{
	ActionText = InText;

	Action_RichText->SetText(InText);
	
	if (!bInUse)
	{
		bInUse = true;

		SetVisibility(ESlateVisibility::HitTestInvisible);

		if (bAnimate)
			PlayAnimation(Anim_OnShow, 0.0f, bLoopAnimation ? 0 : 1, EUMGSequencePlayMode::Forward, 1.25f);
	}
}

void UPlayerActionPromptWidget::ClearText()
{
	if (bInUse)
	{
		ActionText = FText::FromString("");
		
		Action_RichText->SetText(ActionText);

		bInUse = false;

		SetVisibility(ESlateVisibility::Collapsed);
	}
}
