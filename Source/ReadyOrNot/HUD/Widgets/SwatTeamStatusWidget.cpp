// Copyright Void Interactive, 2023

#include "HUD/Widgets/SwatTeamStatusWidget.h"

#include "SwatCommandStatusWidget.h"
#include "SwatCommandWidget.h"
#include "TextWidget.h"
#include "Commander/RosterManager.h"
#include "Components/HorizontalBox.h"
#include "Components/VerticalBox.h"
#include "Info/SWATManager.h"

DECLARE_CYCLE_STAT(TEXT("Swat Team Status ~ Update Status"), STAT_UpdateStatus, STATGROUP_SwatTeamStatusWidget);
DECLARE_CYCLE_STAT(TEXT("Swat Team Status ~ Update Status Command Name"), STAT_UpdateStatusCommandName, STATGROUP_SwatTeamStatusWidget);

void USwatTeamStatusWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	const USWATManager* SwatManager = USWATManager::Get(this);
	if (!SwatManager)
	{
		SWAT_Status_Container->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	
	SWAT_Status_Container->SetVisibility(ESlateVisibility::HitTestInvisible);
	
	InitialSquadMap.Add(ESquadPosition::SP_Alpha, SwatManager->GetSwatCharacterAtSquadPosition(ESquadPosition::SP_Alpha));
	InitialSquadMap.Add(ESquadPosition::SP_Beta, SwatManager->GetSwatCharacterAtSquadPosition(ESquadPosition::SP_Beta));
	InitialSquadMap.Add(ESquadPosition::SP_Charlie, SwatManager->GetSwatCharacterAtSquadPosition(ESquadPosition::SP_Charlie));
	InitialSquadMap.Add(ESquadPosition::SP_Delta, SwatManager->GetSwatCharacterAtSquadPosition(ESquadPosition::SP_Delta));
	InitialSquadMap.Add(ESquadPosition::SP_Foxtrot, SwatManager->GetSwatCharacterAtSquadPosition(ESquadPosition::SP_Foxtrot));
	
	UpdateSwatStatusPlayerNameFromSquadPosition(SWAT_Alpha_Status, ESquadPosition::SP_Alpha);
	UpdateSwatStatusPlayerNameFromSquadPosition(SWAT_Beta_Status, ESquadPosition::SP_Beta);
	UpdateSwatStatusPlayerNameFromSquadPosition(SWAT_Charlie_Status, ESquadPosition::SP_Charlie);
	UpdateSwatStatusPlayerNameFromSquadPosition(SWAT_Delta_Status, ESquadPosition::SP_Delta);
	UpdateSwatStatusPlayerNameFromSquadPosition(SWAT_Lead_Status, ESquadPosition::SP_Foxtrot);

	if (APlayerCharacter* Player = GetOwningPlayerPawn<APlayerCharacter>())
	{
		Player->OnDefaultCommandIssued.RemoveAll(this);
		Player->OnDefaultCommandIssued.AddDynamic(this, &USwatTeamStatusWidget::OnDefaultCommandIssued);
	}

	UReadyOrNotFunctionLibrary::StartTimerForCallback(this, &USwatTeamStatusWidget::UpdateStatus, 0.15f, true);
}

void USwatTeamStatusWidget::UpdateSwatStatusPlayerNameFromSquadPosition(USwatCommandStatusWidget* StatusWidget, ESquadPosition Position)
{
	if (StatusWidget->bIsLead)
	{
		StatusWidget->SetPlayerName(FText::FromStringTable("SwatCommandTable", "Judge"));
	}
	else
	{
		if (const ASWATCharacter* Swat = GetInitialCharacterFromPosition(Position))
		{
			StatusWidget->SetPlayerName(Swat->GetSwatCharacterName());
		}
	}
}

void USwatTeamStatusWidget::UpdateSwatStatusPlayerHealth(USwatCommandStatusWidget* StatusWidget, ESquadPosition Position)
{
	if (StatusWidget->bIsLead)
	{
		if (const APlayerCharacter* Player = GetOwningPlayerPawn<APlayerCharacter>())
		{
			StatusWidget->SetPlayerHealthStatus(Player->GetHealthStatus());
		}
		else
		{
			StatusWidget->SetPlayerHealthStatus(EPlayerHealthStatus::HS_NotAvailable);
		}
	}
	else
	{
		if (const ASWATCharacter* Swat = GetInitialCharacterFromPosition(Position))
		{
			StatusWidget->SetPlayerHealthStatus(Swat->GetHealthStatus());
		}
		else
		{
			StatusWidget->SetPlayerHealthStatus(EPlayerHealthStatus::HS_NotAvailable);
		}
	}
}

void USwatTeamStatusWidget::UpdateSwatStatusCommandName(USwatCommandStatusWidget* StatusWidget, UBaseActivity*& SquadActivity, ESquadPosition Position)
{
	SCOPE_CYCLE_COUNTER(STAT_UpdateStatusCommandName);
	
	const USWATManager* SwatManager = USWATManager::Get(this);
	if (!SwatManager)
	{
		return;
	}
	
	if (StatusWidget->bIsLead)
	{
		if (SwatManager->IsSWATTeamDead(ETeamType::TT_SQUAD))
		{
			StatusWidget->HideCommandInfo();
			StatusWidget->CurrentCommand_Box->SetVisibility(ESlateVisibility::Collapsed);
			StatusWidget->CommandStatus_Box->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			const FText DefaultCommand = UReadyOrNotFunctionLibrary::SwatCommandToText(SwatManager->CurrentDefaultCommand);
			StatusWidget->SetCurrentCommand(DefaultCommand, FText::FromStringTable("SwatCommandTable", "InProgress"), false, false);

			const bool bIsUsingGamepad = UReadyOrNotFunctionLibrary::IsUsingGamepad(GetOwningPlayer<AReadyOrNotPlayerController>());
			if (bIsUsingGamepad)
			{
				const FString PlatformName = UGameplayStatics::GetPlatformName();
				
				if (const UDataTable* Table = UBpGameplayHelperLib::GetInputKeyGamepadIconLookupDataTable())
				{
					if (const FRonInputKeyGamePadIconTable* Data = Table->FindRow<FRonInputKeyGamePadIconTable>("Gamepad_RightShoulder", "TeamStatusWidget"))
					{
						if (PlatformName == "PS5" || PlatformName == "PS4")
						{
							StatusWidget->SetCommandIconBrush(Data->PS5);
						}
						else
						{
							StatusWidget->SetCommandIconBrush(Data->XSX);
						}
					}
				}
			}
			else
			{
				const FKey Key = UReadyOrNotFunctionLibrary::GetKeyFromInputActionName("IssueDefaultCommand");
				const FText InputText = Key.IsValid() ? Key.GetDisplayName() : FText::FromString("Unbound");
				
				StatusWidget->SetHotkeyText(InputText.ToUpper());
			}

			StatusWidget->SetCommandNameColorFromTeam(SwatManager->ActiveCommandTeam);
			StatusWidget->Refresh(false);
		}
	}
	else
	{
		if (const ASWATCharacter* Swat = GetInitialCharacterFromPosition(Position))
		{
			if (!Swat->GetCyberneticsController() || Swat->IsDeadOrUnconscious() || Swat->IsIncapacitated())
			{
				StatusWidget->HideCommandStatus(true);
			}
			else
			{
				UBaseActivity* CurrentActivity = Swat->GetCyberneticsController()->GetCurrentActivity();

				SquadActivity = CurrentActivity;

				const ESwatCommand QueuedCommand = SwatManager->GetQueuedSwatCommandForSquadPosition(Position);
				if (QueuedCommand != ESwatCommand::SC_None)
				{
					const FText ActivityName = FText::Format(FText::FromStringTable("SwatCommandTable", "QueuedCommand"), UReadyOrNotFunctionLibrary::SwatCommandToText(QueuedCommand));
					StatusWidget->SetCurrentCommand(ActivityName, FText::FromStringTable("SwatCommandTable", "InProgress"), false, false);

					if (CurrentActivity)
					{
						//TODO (Max): Make ActivityName FText
						FText StatusText = CurrentActivity->ActivityName;
						FString StatusTextKey = CurrentActivity->IsActivityComplete() ? "StatusComplete" : CurrentActivity->IsProgressActivity() ? "StatusInProgress" : "";
						
						if (StatusTextKey != "")
							StatusText = FText::Format(FText::FromStringTable("SwatCommandTable", StatusTextKey),CurrentActivity->ActivityName); 
						
						StatusWidget->SetCommandStatusText(StatusText);
					}
				}
				else
				{
					if (CurrentActivity)
					{
						const bool bIsWaiting = CurrentActivity->ActivityName.IsEmpty() || CurrentActivity->IsActivityComplete();
						const bool bIsProgress = CurrentActivity->IsProgressActivity();
						
						StatusWidget->SetCurrentCommand(CurrentActivity->ActivityName, CurrentActivity->GetProgressState(), bIsWaiting, bIsProgress);
					}
					else
					{
						if (SquadActivity)
						{
							StatusWidget->SetCurrentCommand(SquadActivity->ActivityName, SquadActivity->GetProgressState(), true, SquadActivity->IsProgressActivity());
						}
						else
						{
							StatusWidget->SetCurrentCommand(FText::GetEmpty(), FText::GetEmpty(), true, false);
						}
					}
				}
			}
		}
		else
		{
			StatusWidget->HideCommandStatus(true);
		}
	}
}

void USwatTeamStatusWidget::OnDefaultCommandIssued(APlayerCharacter* Issuer, ESwatCommand CommandIssued)
{
	SWAT_Lead_Status->PlayCommandIssuedAnim();
}

void USwatTeamStatusWidget::UpdateStatus()
{
	SCOPE_CYCLE_COUNTER(STAT_UpdateStatus);

	if (!IsVisible())
		return;
	
	const USWATManager* SwatManager = USWATManager::Get(this);
	if (!SwatManager)
	{
		SWAT_Status_Container->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	if (SwatManager->GetSWATCount() == 0)
	{
		SWAT_Status_Container->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	SWAT_Status_Container->SetVisibility(ESlateVisibility::HitTestInvisible);

	UpdateSwatStatusPlayerHealth(SWAT_Alpha_Status, ESquadPosition::SP_Alpha);
	UpdateSwatStatusPlayerHealth(SWAT_Beta_Status, ESquadPosition::SP_Beta);
	UpdateSwatStatusPlayerHealth(SWAT_Charlie_Status, ESquadPosition::SP_Charlie);
	UpdateSwatStatusPlayerHealth(SWAT_Delta_Status, ESquadPosition::SP_Delta);
	UpdateSwatStatusPlayerHealth(SWAT_Lead_Status, ESquadPosition::SP_Foxtrot);

	if (SwatManager->IsSWATTeamDead())
	{
		SWAT_Alpha_Status->StartHealthWidthChange(38.0f);
		SWAT_Beta_Status->StartHealthWidthChange(38.0f);
		SWAT_Charlie_Status->StartHealthWidthChange(38.0f);
		SWAT_Delta_Status->StartHealthWidthChange(38.0f);
		
		SWAT_Alpha_Status->Shrink(true);
		SWAT_Beta_Status->Shrink(true);
		SWAT_Charlie_Status->Shrink(true);
		SWAT_Delta_Status->Shrink(true);
		
		SWAT_Alpha_Status->HideCommandInfo();
		SWAT_Beta_Status->HideCommandInfo();
		SWAT_Charlie_Status->HideCommandInfo();
		SWAT_Delta_Status->HideCommandInfo();
		SWAT_Lead_Status->HideCommandInfo();
		
		SWAT_Lead_Status->Shrink(true);
	}
	else
	{
		// Update swat status commands
		if (SWAT_Status_Container->IsVisible())
		{
			UpdateSwatStatusCommandName(SWAT_Alpha_Status, AlphaActivity, ESquadPosition::SP_Alpha);
			UpdateSwatStatusCommandName(SWAT_Beta_Status, BetaActivity, ESquadPosition::SP_Beta);
			UpdateSwatStatusCommandName(SWAT_Charlie_Status, CharlieActivity, ESquadPosition::SP_Charlie);
			UpdateSwatStatusCommandName(SWAT_Delta_Status, DeltaActivity, ESquadPosition::SP_Delta);
			UpdateSwatStatusCommandName(SWAT_Lead_Status, LeadActivity, ESquadPosition::SP_Foxtrot);

			// Hide Alpha command status when Beta is same activity as Alpha (reduces text repetition)
			if (!IsSwatDead(ESquadPosition::SP_Alpha) && !IsSwatDead(ESquadPosition::SP_Beta))
			{
				HideDuplicateCommandStatus(AlphaActivity, BetaActivity, SWAT_Alpha_Status);
			}

			// Hide Charlie command status when Delta is same activity as Charlie (reduces text repetition)
			if (!IsSwatDead(ESquadPosition::SP_Charlie) && !IsSwatDead(ESquadPosition::SP_Delta))
			{
				HideDuplicateCommandStatus(CharlieActivity, DeltaActivity, SWAT_Charlie_Status);
			}
		}
	}
	
	InvalidateLayoutAndVolatility();
}

ASWATCharacter* USwatTeamStatusWidget::GetInitialCharacterFromPosition(ESquadPosition Position)
{
	ASWATCharacter** Swat = InitialSquadMap.Find(Position);
	return Swat ? *Swat : nullptr;
}

void USwatTeamStatusWidget::HideDuplicateCommandStatus(UBaseActivity*& ActivityA, UBaseActivity*& ActivityB, USwatCommandStatusWidget*& SwatWidgetToHide)
{
	if (!ActivityA)
	{
		SwatWidgetToHide->HideCommandStatus(true);
		return;
	}
	
	if (!ActivityB)
	{
		return;
	}
	
	const bool bSameName = ActivityA->ActivityName.EqualTo(ActivityB->ActivityName);
	
	SwatWidgetToHide->CurrentCommand_Text->SetVisibility(bSameName ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	SwatWidgetToHide->CurrentCommand_Status_Text->SetVisibility(bSameName ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);

	if (bSameName)
	{
		const bool bIsWaiting = ActivityA->ActivityName.IsEmpty() || ActivityA->IsActivityComplete();
		
		SwatWidgetToHide->Shrink(!bIsWaiting);
	}
	else
	{
		SwatWidgetToHide->Grow(false);
	}
}

bool USwatTeamStatusWidget::IsSwatDead(ESquadPosition Position)
{
	if (const ASWATCharacter* Swat = GetInitialCharacterFromPosition(Position))
	{
		return Swat->IsDeadOrUnconscious() || Swat->IsIncapacitated();
	}

	return false;
}
