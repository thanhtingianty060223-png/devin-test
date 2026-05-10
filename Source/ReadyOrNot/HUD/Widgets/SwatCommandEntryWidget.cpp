// Copyright Void Interactive, 2023

#include "HUD/Widgets/SwatCommandEntryWidget.h"

#include "TextWidget.h"
#include "Components/Border.h"

void USwatCommandEntryWidget::SetBorderColor()
{
	if (bLast || bBack)
	{
		EntryBorder->SetBrushColor(FLinearColor::Transparent);
		return;
	}

	EntryBorder->SetBrushColor(GetTeamColor());
}

void USwatCommandEntryWidget::SetExtendedImageColor()
{
	ExtendedImage->SetColorAndOpacity(GetTeamColor());
}

void USwatCommandEntryWidget::SetText(const FText& NewText)
{
	CommandText->SetText(NewText);
}

void USwatCommandEntryWidget::SetTextColor()
{
	CommandText->SetTextColor(GetTeamColor());
	KeybindText->SetTextColor(GetTeamColor());
}

void USwatCommandEntryWidget::PlayFlashAnimation()
{
	PlayAnimation(Flash);
}

void USwatCommandEntryWidget::UpdateExtendedImageVisibility()
{
	ExtendedImage->SetVisibility(bBack || bExtended ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void USwatCommandEntryWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	UpdateCommandEntry(SwatCommand, ActiveTeamType);
}

FLinearColor USwatCommandEntryWidget::GetTeamColor() const
{
	switch (ActiveTeamType)
	{
		case ETeamType::TT_NONE:			return FLinearColor(1.0f, 1.0f, 1.0f, 0.9f);
		case ETeamType::TT_SERT_RED:		return RedTeamColor;
		case ETeamType::TT_SERT_BLUE:		return BlueTeamColor;
		case ETeamType::TT_SUSPECT:			return RedTeamColor;
		case ETeamType::TT_CIVILIAN:		return BlueTeamColor;
		case ETeamType::TT_SQUAD:			return GoldTeamColor;
		default:							return FLinearColor(1.0f, 1.0f, 1.0f, 0.9f);
	}
}

void USwatCommandEntryWidget::UpdateCommandEntry(const FSwatCommand& InSwatCommand, const ETeamType Team)
{
	SwatCommand = InSwatCommand;
	ActiveTeamType = Team;

	KeybindText->SetText(InSwatCommand.InputKey.GetDisplayName());
	CommandText->SetText(InSwatCommand.CommandText);

	bExtended = InSwatCommand.CommandType == ESwatCommand::SC_None && InSwatCommand.SubCommands.Num() > 0;

	KeybindText->SetIsEnabled(InSwatCommand.bEnabled);
	CommandText->SetIsEnabled(InSwatCommand.bEnabled);
	ExtendedImage->SetIsEnabled(InSwatCommand.bEnabled);

	SetBorderColor();
	SetExtendedImageColor();
	SetTextColor();

	if (bBack)
	{
		ExtendedImage->SetBrushFromTexture(BackIcon);
	}
	
	UpdateExtendedImageVisibility();
}
