// Copyright Void Interactive, 2023

#include "HUD/Widgets/SwatCommandStatusWidget.h"

#include "TextWidget.h"
#include "Components/HorizontalBox.h"
#include "Components/RichTextBlock.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"

DECLARE_CYCLE_STAT(TEXT("Swat Command Status ~ Update Squad Data"), STAT_UpdateSquadData, STATGROUP_SwatCommandStatusWidget);
DECLARE_CYCLE_STAT(TEXT("Swat Command Status ~ Set Current Command"), STAT_SetCurrentCommand, STATGROUP_SwatCommandStatusWidget);

void USwatCommandStatusWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	Refresh(true);
}

void USwatCommandStatusWidget::Refresh(const bool bUpdateColor)
{
	SizeBox->SetHeightOverride(MaxHeight);

	if (bIsLead)
	{
		const bool bIsUsingGamepad = UReadyOrNotFunctionLibrary::IsUsingGamepad(GetOwningPlayer<AReadyOrNotPlayerController>());
		
		IssueCommand_Hotkey_RichText->SetVisibility(bIsUsingGamepad ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
		IssueCommand_Hotkey_Icon->SetVisibility(bIsUsingGamepad ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	else
	{
		IssueCommand_Hotkey_RichText->SetVisibility(ESlateVisibility::Collapsed);
		IssueCommand_Hotkey_Icon->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (bUpdateColor)
	{
		UpdateSquadData();
	}
}

void USwatCommandStatusWidget::Shrink(const bool bInstant)
{
	if (bInstant)
	{
		TargetHeight = MinHeight;
		TickDesiredHeight();
	}
	else
	{
		StartHeightChange(MinHeight);
	}
}

void USwatCommandStatusWidget::Grow(const bool bInstant)
{
	const float NewHeight = bIsProgress ? MaxHeight : MaxHeight - 5.0f;
	
	if (bInstant)
	{
		TargetHeight = NewHeight;
		TickDesiredHeight();
	}
	else
	{
		StartHeightChange(NewHeight);
	}
}

void USwatCommandStatusWidget::StartHeightChange(const float NewHeight)
{
	TargetHeight = NewHeight;

	if (SizeBox->HeightOverride != TargetHeight)
	{
		if (!bIsLead)
		{
			UReadyOrNotFunctionLibrary::StartTimerForCallback(HeightChangeTimerHandle, this, &USwatCommandStatusWidget::TickDesiredHeight, 0.033f, true);
		}
	}
}

void USwatCommandStatusWidget::StartHealthWidthChange(float NewWidth)
{
	TargetHealthWidth = NewWidth;

	if (UReadyOrNotFunctionLibrary::IsCallbackTimerActive(this, HealthWidthChangeTimerHandle))
		return;
	
	if (HealthStatus_SizeBox->WidthOverride == TargetHealthWidth)
		return;

	if (bIsLead)
		return;

	UReadyOrNotFunctionLibrary::StartTimerForCallback(HealthWidthChangeTimerHandle, this, &USwatCommandStatusWidget::TickDesiredWidth, 0.033f, true);
}

void USwatCommandStatusWidget::UpdateSquadData()
{
	SCOPE_CYCLE_COUNTER(STAT_UpdateSquadData);
	
	SetPlayerHealthStatus(PlayerHealthStatus);

	CurrentCommand_Text->SetTextColor(bIsLead ? ElementTeamColor : FLinearColor(0.904661f, 0.871367f, 0.760525f, 0.9f));

	switch (SquadPosition)
	{
		case ESquadPosition::SP_Alpha:		TeamIndicator_Image->SetColorAndOpacity(BlueTeamColor); break;
		case ESquadPosition::SP_Beta:		TeamIndicator_Image->SetColorAndOpacity(BlueTeamColor); break;
		case ESquadPosition::SP_Charlie:	TeamIndicator_Image->SetColorAndOpacity(RedTeamColor); break;
		case ESquadPosition::SP_Delta:		TeamIndicator_Image->SetColorAndOpacity(RedTeamColor); break;
		case ESquadPosition::SP_Foxtrot:	TeamIndicator_Image->SetColorAndOpacity(ElementTeamColor); break;
		default: break;
	}

	if (bIsLead)
	{
		const bool bIsUsingGamepad = UReadyOrNotFunctionLibrary::IsUsingGamepad(GetOwningPlayer<AReadyOrNotPlayerController>());
		
		IssueCommand_Hotkey_RichText->SetVisibility(bIsUsingGamepad ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
		IssueCommand_Hotkey_Icon->SetVisibility(bIsUsingGamepad ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

		{
			FSlateFontInfo ModifiedFont = CurrentCommand_Text->GetFont();
			ModifiedFont.TypefaceFontName = "Bold";
			ModifiedFont.Size = 14;
			CurrentCommand_Text->SetFont(ModifiedFont);
		}

		{
			FSlateFontInfo ModifiedFont = CurrentCommand_Pulse_Text->GetFont();
			ModifiedFont.TypefaceFontName = "Bold";
			ModifiedFont.Size = 14;
			CurrentCommand_Pulse_Text->SetFont(ModifiedFont);
		}

		TeamIndicator_Box->SetVisibility(ESlateVisibility::Collapsed);
		HealthStatus_SizeBox->SetVisibility(ESlateVisibility::Collapsed);
		CurrentCommand_Status_Text->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void USwatCommandStatusWidget::SetPlayerHealthStatus(EPlayerHealthStatus HealthStatus)
{
	PlayerHealthStatus = HealthStatus;

	switch (HealthStatus)
	{
		case EPlayerHealthStatus::HS_Healthy:			PlayerHealth_Text->SetText(FText::FromStringTable("SwatCommandTable", "Healthy")); PlayerHealth_Text->SetTextColor(NormalColor); break;
		case EPlayerHealthStatus::HS_Injured:			PlayerHealth_Text->SetText(FText::FromStringTable("SwatCommandTable", "Injured")); PlayerHealth_Text->SetTextColor(InjuredColor); break;
		case EPlayerHealthStatus::HS_Downed:			PlayerHealth_Text->SetText(FText::FromStringTable("SwatCommandTable", "Downed")); PlayerHealth_Text->SetTextColor(InjuredColor); break;
		case EPlayerHealthStatus::HS_Incapacitated:		PlayerHealth_Text->SetText(FText::FromStringTable("SwatCommandTable", "Incapacitated")); PlayerHealth_Text->SetTextColor(DeadColor); break;
		case EPlayerHealthStatus::HS_Dead:				PlayerHealth_Text->SetText(FText::FromStringTable("SwatCommandTable", "Dead")); PlayerHealth_Text->SetTextColor(DeadColor); break;
		case EPlayerHealthStatus::HS_Arrested:			PlayerHealth_Text->SetText(FText::FromStringTable("SwatCommandTable", "Arrested")); PlayerHealth_Text->SetTextColor(InjuredColor); break;
		case EPlayerHealthStatus::HS_NotAvailable:		PlayerHealth_Text->SetText(FText::FromStringTable("SwatCommandTable", "Unavailable")); PlayerHealth_Text->SetTextColor(DeadColor); break;
		default: break;
	}

	const bool bDeadOrIncap = HealthStatus == EPlayerHealthStatus::HS_Dead || HealthStatus == EPlayerHealthStatus::HS_Incapacitated;
	
	if (bDeadOrIncap)
	{
		SwatInfo_Box->SetRenderOpacity(FMath::FInterpTo(SwatInfo_Box->GetRenderOpacity(), 0.5f, 0.033f, 6.0f));
	}
	
	CurrentCommand_Box->SetVisibility(bDeadOrIncap ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	CommandStatus_Box->SetVisibility(bDeadOrIncap ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}

void USwatCommandStatusWidget::SetCurrentCommand(FText CommandText, FText ProgressText, bool bWaiting, bool bInProgress)
{
	SCOPE_CYCLE_COUNTER(STAT_SetCurrentCommand);
	
	bIsProgress = bInProgress;
	
	if (bIsLead)
	{
		CurrentCommand_Text->SetText(CommandText);
		CurrentCommand_Pulse_Text->SetText(CommandText);
	}
	else
	{
		const bool bCommandTextVisible = CurrentCommand_Text->IsVisible();

		FText ConditionalText = bWaiting ? FText::FromStringTable("SwatCommandTable", "AwaitingOrder") : CommandText;

		bool bSameText = CurrentCommand_Text->GetText().EqualToCaseIgnored(ConditionalText);

		if (!bCommandTextVisible || !bSameText)
		{
			CurrentCommand_Text->SetText(ConditionalText.ToUpper());
			CurrentCommand_Pulse_Text->SetText(ConditionalText.ToUpper());

			CurrentCommand_Text->SetVisibility(ESlateVisibility::HitTestInvisible);
			CurrentCommand_Status_Text->SetVisibility(ESlateVisibility::HitTestInvisible);

			if (bWaiting)
			{
				/*
				FText ActivityText = FText::FromString(ActivityName + " | Complete");
				
				const bool bCommandTextIsEmpty = CommandText.IsEmpty() || !bIsProgress;
				
				ConditionalText = bCommandTextIsEmpty ? FText::FromString("") : ActivityText;

				bSameText = CurrentCommand_Status_Text->GetText().EqualToCaseIgnored(ConditionalText);

				if (!bSameText)
				{
					CurrentCommand_Status_Text->SetText(ConditionalText);
				}
				*/
				
				CurrentCommand_Status_Text->SetText(FText::FromString(""));

				if (TargetHeight != MinHeight || SwatInfo_Box->GetRenderOpacity() >= 1.0f)
				{
					PlayCommandCompleteAnim();
				}
			}
			else
			{
				StopAnimation(Anim_CommandCompleted);
				Grow(false);

				CurrentCommand_Status_Text->SetText(bIsProgress ? ProgressText.ToUpper() : FText::FromString(""));

				UReadyOrNotFunctionLibrary::StartTimerForCallback(this, &USwatCommandStatusWidget::ShowCommandSubText, 0.1f);
			}
		}
		else
		{
			if (bWaiting)
			{
				if (TargetHeight != MinHeight || SwatInfo_Box->GetRenderOpacity() >= 1.0f)
				{
					PlayCommandCompleteAnim();
				}
			}
			else
			{
				CurrentCommand_Status_Text->SetText(bIsProgress ? ProgressText.ToUpper() : FText::FromString(""));
			}
		}
	}
}

void USwatCommandStatusWidget::SetCommandNameColorFromTeam(ETeamType Team)
{
	if (bIsLead)
	{
		switch (Team)
		{
			case ETeamType::TT_NONE:			CurrentCommand_Text->SetTextColor(FLinearColor(0.904f, 0.8713f, 0.760525f, 0.92f)); break;
			case ETeamType::TT_SERT_RED:		CurrentCommand_Text->SetTextColor(RedTeamColor); break;
			case ETeamType::TT_SERT_BLUE:		CurrentCommand_Text->SetTextColor(BlueTeamColor); break;
			case ETeamType::TT_SQUAD:			CurrentCommand_Text->SetTextColor(ElementTeamColor); break;
			default:							CurrentCommand_Text->SetTextColor(FLinearColor(0.904f, 0.8713f, 0.760525f, 0.92f)); break;
		}
		
		CurrentCommand_Status_Text->SetText(FText::FromString(""));
	}
}

void USwatCommandStatusWidget::SetCommandIconBrush(const FSlateBrush& NewBrush)
{
	IssueCommand_Hotkey_Icon->SetBrush(NewBrush);
}

void USwatCommandStatusWidget::SetPlayerName(FText NewPlayerName)
{
	SwatName_Text->SetText(NewPlayerName);
}

void USwatCommandStatusWidget::PlayCommandIssuedAnim()
{
	PlayAnimation(Anim_CommandIssued);
}

void USwatCommandStatusWidget::PlayCommandCompleteAnim()
{
	if (!IsAnimationPlaying(Anim_CommandCompleted))
	{
		PlayAnimation(Anim_CommandCompleted);
	}
}

void USwatCommandStatusWidget::TickDesiredHeight()
{
	SizeBox->SetHeightOverride(FMath::FInterpTo(SizeBox->HeightOverride, TargetHeight, 0.033f, 7.0f));
	
	if (SizeBox->HeightOverride == TargetHeight)
	{
		UReadyOrNotFunctionLibrary::StopCallbackTimer(this, HeightChangeTimerHandle);
	}
}

void USwatCommandStatusWidget::TickDesiredWidth()
{
	HealthStatus_SizeBox->SetWidthOverride(FMath::FInterpTo(HealthStatus_SizeBox->WidthOverride, TargetHealthWidth, 0.033f, 7.0f));

	if (HealthStatus_SizeBox->WidthOverride == TargetHealthWidth)
	{
		UReadyOrNotFunctionLibrary::StopCallbackTimer(this, HealthWidthChangeTimerHandle);
	}
}

void USwatCommandStatusWidget::OnAnimationFinished_Implementation(const UWidgetAnimation* Animation)
{
	Super::OnAnimationFinished_Implementation(Animation);

	if (Animation == Anim_CommandCompleted)
	{
		StartHeightChange(MinHeight);
	}
}

void USwatCommandStatusWidget::ShowCommandSubText()
{
	CurrentCommand_Box->SetRenderOpacity(1.0f);
	CurrentCommand_Status_Text->SetRenderOpacity(1.0f);
	SwatInfo_Box->SetRenderOpacity(1.0f);

	CurrentCommand_Pulse_Text->SetRenderOpacity(0.0f);
	CurrentCommand_Pulse_Text->SetVisibility(ESlateVisibility::Collapsed);
}

void USwatCommandStatusWidget::SetHotkeyText(FText NewText)
{
	IssueCommand_Hotkey_RichText->SetText(FText::FromString("[<Red_f12>" + NewText.ToString() + "</>]"));
}

void USwatCommandStatusWidget::HideCommandInfo()
{
	CurrentCommand_Box->SetVisibility(ESlateVisibility::Collapsed);
	CommandStatus_Box->SetVisibility(ESlateVisibility::Collapsed);
}

void USwatCommandStatusWidget::SetCommandStatusText(FText NewText)
{
	CurrentCommand_Status_Text->SetText(NewText);
}

void USwatCommandStatusWidget::HideCommandStatus(const bool bShrinkHeight)
{
	CurrentCommand_Text->SetText(FText::FromString(""));
	CurrentCommand_Status_Text->SetText(FText::FromString(""));
	CurrentCommand_Pulse_Text->SetText(FText::FromString(""));

	if (bShrinkHeight)
	{
		Shrink(false);
	}
}
