// Void Interactive, 2020

#include "CurrentMatchRoundWidget.h"

#include "Components/TextBlock.h"

void UCurrentMatchRoundWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (RONGS)
	{
		CurrentRound_Text->SetText(FText::FromString(FString("Round ") + FString::FromInt(RONGS->RoundsPlayed + 1)));
		CurrentRound_Text_Style2->SetText(FText::AsNumber(RONGS->RoundsPlayed + 1));
	}
}
