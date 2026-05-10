// Copyright Void Interactive, 2023

#include "HUD/Widgets/TabletWidget.h"

#include "Components/WidgetSwitcher.h"

#include "TeamViewWidget.h"

bool UTabletWidget::IsTeamViewFocused() const
{
	return ScreenSwitcher->GetActiveWidget() == TeamView;
}

void UTabletWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (ScreenSwitcher && ScreenSwitcher->GetActiveWidget() == TeamView)
	{
		APlayerCharacter* OwningPlayer = GetOwningPlayerPawn<APlayerCharacter>();
		if (OwningPlayer->CurrentTeamViewIndex <= -1)
		{
			OwningPlayer->NextPlayerView(false);
		}
	}
	
	TeamView->OnViewSwitched();
	TeamView->TickTeamView(InDeltaTime);
}

int UTabletWidget::GetActiveButton(int currentButtonIndex, int navigationDirection, TArray<bool> buttonVisibilities)
{
	if(navigationDirection != 1 && navigationDirection != -1) {
        return currentButtonIndex;
    }

	auto newButtonIndex = currentButtonIndex;

	while(true) {
		newButtonIndex += navigationDirection;
		if(newButtonIndex < 0 || newButtonIndex > buttonVisibilities.Num() - 1) {
			return currentButtonIndex;
		}
		if(buttonVisibilities[newButtonIndex]) {
			break;
		}
	}

	return newButtonIndex;
}