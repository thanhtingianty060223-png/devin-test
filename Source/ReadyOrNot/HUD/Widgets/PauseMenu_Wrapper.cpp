// Copyright Void Interactive, 2023


#include "HUD/Widgets/PauseMenu_Wrapper.h"

#include "CommonActivatableWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

void UPauseMenu_Wrapper::OpenPauseMenu()
{

	if (AReadyOrNotPlayerController* PlayerController = Cast<AReadyOrNotPlayerController>(GetOwningPlayer()))
	{
		SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		PlayerController->ChangeInputMode(false, true);
		if (UReadyOrNotStatics::GetReadyOrNotGameInstance())
		{
			if (UReadyOrNotStatics::GetReadyOrNotGameInstance()->SessionType == ESessionType::ST_SinglePlayer)
			{
				UReadyOrNotStatics::GetReadyOrNotGameInstance()->AddPauseGameCondition("PauseMenuActive");
				UGameplayStatics::SetGamePaused(GetWorld(), true);
				UReadyOrNotFunctionLibrary::PauseFMOD(true);
			}
		}
		PlayerController->HideHUD();
		if (PauseMenu && !PauseMenu->IsActivated())
		{
			PauseMenu->ActivateWidget();
		}
		UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(PlayerController);
	}
	else
	{
		ULog::Error("No ReadyOrNotPlayerController! Failed to open Pause  Menu!");
	}
#ifdef WITH_EDITOR
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 10.0f, FColor::Magenta, "Open Pause Menu");
#endif
}

void UPauseMenu_Wrapper::ClosePauseMenu()
{
	
	if (AReadyOrNotPlayerController* PlayerController = Cast<AReadyOrNotPlayerController>(GetOwningPlayer()))
	{
		SetVisibility(ESlateVisibility::Collapsed);
		PlayerController->ChangeInputMode(true, false);
		if (UReadyOrNotStatics::GetReadyOrNotGameInstance())
		{
			if (UReadyOrNotStatics::GetReadyOrNotGameInstance()->SessionType == ESessionType::ST_SinglePlayer)
			{
				UReadyOrNotStatics::GetReadyOrNotGameInstance()->RemovePauseGameCondition("PauseMenuActive");
				UGameplayStatics::SetGamePaused(GetWorld(), false);
				UReadyOrNotFunctionLibrary::PauseFMOD(false);
			}
		}
		PlayerController->ShowHUD();
		if (PauseMenu && PauseMenu->IsActivated())
		{
			PauseMenu->DeactivateWidget();
		}
		UWidgetBlueprintLibrary::SetInputMode_GameOnly(PlayerController);
	}
	else
	{
		ULog::Error("No ReadyOrNotPlayerController! Failed to close Pause Menu!");
	}

	RemoveFromViewport();
	OnPauseMenuClosed.Broadcast();
#ifdef WITH_EDITOR
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 10.0f, FColor::Magenta, "Close Pause Menu");
#endif
}
