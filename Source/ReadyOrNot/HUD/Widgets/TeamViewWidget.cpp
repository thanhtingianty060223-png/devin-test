// Void Interactive, 2020

#include "HUD/Widgets/TeamViewWidget.h"

#include "Actors/PlayerViewActor.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Characters/AI/SWATCharacter.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

UTeamViewWidget::UTeamViewWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	TeamColorMap.Empty(6);
	TeamColorMap.Add(ETeamType::TT_NONE, FSlateColor(FColor::White));
	TeamColorMap.Add(ETeamType::TT_SQUAD, FSlateColor(FColor::Yellow));
	TeamColorMap.Add(ETeamType::TT_SERT_RED, FSlateColor(FColor::Red));
	TeamColorMap.Add(ETeamType::TT_SUSPECT, FSlateColor(FColor::Red));
	TeamColorMap.Add(ETeamType::TT_SERT_BLUE, FSlateColor(FColor::Blue));
	TeamColorMap.Add(ETeamType::TT_CIVILIAN, FSlateColor(FColor::Blue));
	
	HealthStatusColorMap.Empty(7);
	HealthStatusColorMap.Add(EPlayerHealthStatus::HS_NotAvailable, FSlateColor(FColor::White));
	HealthStatusColorMap.Add(EPlayerHealthStatus::HS_Healthy, FSlateColor(FColor::White));
	HealthStatusColorMap.Add(EPlayerHealthStatus::HS_Injured, FSlateColor(FColor::Yellow));
	HealthStatusColorMap.Add(EPlayerHealthStatus::HS_Dead, FSlateColor(FColor::Red));
	HealthStatusColorMap.Add(EPlayerHealthStatus::HS_Incapacitated, FSlateColor(FColor::Yellow));
	HealthStatusColorMap.Add(EPlayerHealthStatus::HS_Downed, FSlateColor(FColor::Yellow));
	HealthStatusColorMap.Add(EPlayerHealthStatus::HS_Arrested, FSlateColor(FColor::Yellow));
}

void UTeamViewWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UpdateInputText();
}

void UTeamViewWidget::TickTeamView(float DeltaTime)
{
	if (const APlayerCharacter* OwnerCharacter = GetOwningPlayerPawn<APlayerCharacter>())
	{
		const AReadyOrNotCharacter* ViewCharacter = OwnerCharacter->CurrentViewCharacter;

		if (IsValid(ViewCharacter))
		{
			SetVisibility(ESlateVisibility::HitTestInvisible);
			
			PlayerHealthStatusText->SetText(PlayerHealthStatusToString(ViewCharacter->GetHealthStatus()));
			PlayerHealthStatusText->SetColorAndOpacity(HealthStatusColorMap[ViewCharacter->GetHealthStatus()]);

			UpdateInputText();

			Tick_TeamViewOn();
		}
		else
		{
			SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UTeamViewWidget::OnViewSwitched()
{
	if (const APlayerCharacter* OwnerCharacter = GetOwningPlayerPawn<APlayerCharacter>())
	{
		const AReadyOrNotCharacter* ViewCharacter = OwnerCharacter->CurrentViewCharacter;
		
		if (IsValid(ViewCharacter))
		{
			// Update brush to point to current view character
			{
				FSlateBrush PlayerViewBrush;
				PlayerViewBrush.SetResourceObject(OwnerCharacter->PlayerViewActor->GetSceneCaptureComponent()->TextureTarget);
				PlayerViewBrush.ImageSize = FVector2D(400.0f, 225.0f);
				PlayerViewBrush.DrawAs = ESlateBrushDrawType::Image;
				PlayerViewBrush.Tiling = ESlateBrushTileType::NoTile;
				PlayerViewBrush.Mirroring = ESlateBrushMirrorType::NoMirror;
				
				PlayerViewImage->SetBrush(PlayerViewBrush);
			}

			FText ViewPlayerName;
			
			// Human player
			if (const AReadyOrNotPlayerState* PS = ViewCharacter->GetPlayerState<AReadyOrNotPlayerState>())
			{
				// For some reason GetPlayerName returns an empty string when a player is dead
				//if (!PS->GetPlayerName().IsEmpty())
				//{
					ViewPlayerName = FText::FromString(PS->GetPlayerName());
				//}
			}
			// AI player
			else
			{
				const ASWATCharacter* SwatCharacter = Cast<ASWATCharacter>(ViewCharacter);
				if (SwatCharacter)
				{
					ViewPlayerName = SwatCharacter->GetSwatCharacterName();
				}
			}

			// Update text to current view character data
			{
				PlayerNameText->SetText(ViewPlayerName);
				PlayerTeamIndicatorImage->SetColorAndOpacity(TeamColorMap[ViewCharacter->GetTeam()].GetSpecifiedColor());
			}

			// Update current view index
			{
				const int32 TotalPlayers = OwnerCharacter->GetAvaliablePlayersForTeamView().Num();
				CurrentViewIndexText->SetText(FText::Format(NSLOCTEXT("TeamViewWidget", "CurrentViewIndex", "{0}/{1}"), FText::AsNumber(OwnerCharacter->CurrentTeamViewIndex+1), FText::AsNumber(TotalPlayers)));
			}
		}
	}
}

void UTeamViewWidget::UpdateInputText()
{
	const FRonKey TeamViewKey = UReadyOrNotFunctionLibrary::ConvertUnrealKeyToRonKey(UReadyOrNotFunctionLibrary::GetKeyFromInputActionName("TeamView"));
			
	SwitchViewText->SetText(FText::Format(NSLOCTEXT("TeamViewWidget", "SwitchViewText", "Press [{0}]: Switch View"), FText::FromString(TeamViewKey.InputName)));
	CloseViewText->SetText(FText::Format(NSLOCTEXT("TeamViewWidget", "CloseViewText", "Hold [{0}]: Close"), FText::FromString(TeamViewKey.InputName)));
}
