// Copyright Void Interactive, 2023

#include "HUD/Widgets/TextWidget.h"

#include "CommonTextBlock.h"

void UTextWidget::SetText(const FText& NewText)
{
	txt_Text->SetText(NewText);
}

void UTextWidget::SetTextColor(const FLinearColor& NewColor)
{
	txt_Text->SetColorAndOpacity(FSlateColor(NewColor));
}

void UTextWidget::SetFont(const FSlateFontInfo& NewFont)
{
	CurrentFont = NewFont;
	txt_Text->SetFont(NewFont);
}

FText UTextWidget::GetText() const
{
	return txt_Text->GetText();
}

FSlateFontInfo UTextWidget::GetFont() const
{
	return txt_Text->Font;
}
