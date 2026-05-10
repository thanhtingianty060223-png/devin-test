// Void Interactive, 2020

#include "MatchStatusCardWidget.h"

#include "CurrentMatchRoundWidget.h"

#include "GameModes/TeamDeathmatchGS.h"

void UMatchStatusCardWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (RONGS)
	{
		//const bool bHideCurrentRoundWidget = RONGS->IsA(ATeamDeathmatchGS::StaticClass()); 

		//CurrentMatchRound->SetVisibility(bHideCurrentRoundWidget ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
		CurrentMatchRound->SetVisibility(ESlateVisibility::Collapsed);
	}
}
