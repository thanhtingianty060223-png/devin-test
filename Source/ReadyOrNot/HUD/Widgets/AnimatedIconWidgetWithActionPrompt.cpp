// Copyright Void Interactive, 2023


#include "AnimatedIconWidgetWithActionPrompt.h"

#include "PlayerActionPromptWidget.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/ScaleBox.h"


void UAnimatedIconWidgetWithActionPrompt::ShowActionPrompt(bool bShow, int32 SlotIndex)
{
	UPlayerActionPromptWidget* ActionPromptWidget = nullptr;
	switch(SlotIndex)
	{
	case 0:
		ActionPromptWidget = PlayerActionPrompt_Widget;
		break;
	case 1:
		ActionPromptWidget = PlayerActionPrompt_Widget2;
		break;
	case 2:
		ActionPromptWidget = PlayerActionPrompt_Widget3;
		break;
	case 4:
		ActionPromptWidget = PlayerActionPrompt_Widget4;
		break;
	default:
		break;
	}

	if (!ActionPromptWidget)
		return;
	
	ActionPromptWidget->SetVisibility(bShow ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);


	// If no action prompts are visible, hide the whole action prompt box
	if (PlayerActionPrompt_Widget->GetVisibility() == ESlateVisibility::Collapsed && PlayerActionPrompt_Widget2->GetVisibility() == ESlateVisibility::Collapsed && PlayerActionPrompt_Widget3->GetVisibility() == ESlateVisibility::Collapsed && PlayerActionPrompt_Widget4->GetVisibility() == ESlateVisibility::Collapsed)
	{
		PlayerActionPrompt_ScaleBox->SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{
		PlayerActionPrompt_ScaleBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}

void UAnimatedIconWidgetWithActionPrompt::SetActionPromptText(FText& Text,int32 SlotIndex)
{
	// If we don't have any text to show, hide the action prompt
	if (Text.IsEmpty() || Text.ToString() == "None")
	{
		ShowActionPrompt(false, SlotIndex);
		return;
	}

	UPlayerActionPromptWidget* ActionPromptWidget = nullptr;
	switch(SlotIndex)
	{
	case 0:
		ActionPromptWidget = PlayerActionPrompt_Widget;
		break;
	case 1:
		ActionPromptWidget = PlayerActionPrompt_Widget2;
		break;
	case 2:
		ActionPromptWidget = PlayerActionPrompt_Widget3;
		break;
	case 4:
		ActionPromptWidget = PlayerActionPrompt_Widget4;
		break;
	default:
		break;
	}

	if (!ActionPromptWidget)
		return;

	ActionPromptWidget->UpdateText(Text, true, false);
	
}

void UAnimatedIconWidgetWithActionPrompt::EnableActionPromptBackground(bool bEnable)
{
	ActionPromptBackground_Image->SetVisibility(bEnable ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
}

void UAnimatedIconWidgetWithActionPrompt::ShowInteractIcon(bool bShow)
{
	InteractIcon_Overlay->SetVisibility(bShow ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
}