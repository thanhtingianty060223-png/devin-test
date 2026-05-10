// Void Interactive, 2020

#include "MatchTimeRemainingWidget.h"

#include "Components/TextBlock.h"

void UMatchTimeRemainingWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (RONGS)
	{
		RoundTimeRemaining = RONGS->RoundTimeRemaining;
	}
}

void UMatchTimeRemainingWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	MatchTimeRemaining_Text->SetText(FText::FromString(UBpGameplayHelperLib::ConvertFloatToStringMinutes(RoundTimeRemaining)));

	if (RONGS)
	{
		RoundTimeRemaining = FMath::Clamp(RONGS->RoundTimeRemaining, 0.0f, RoundTimeRemaining);
	}
}
