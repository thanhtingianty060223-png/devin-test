// Void Interactive, 2020

#include "HealthStatusWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"

void UHealthStatusWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	UpdateIconImage(HealthIconBrush);
}

void UHealthStatusWidget::UpdateIconImage(const FSlateBrush& Brush)
{
	Icon_Image->SetBrush(Brush);
}

void UHealthStatusWidget::AutoDetermineIconImage()
{
	if (Icon_Image->ColorAndOpacity.Equals(ZeroPercentColor, 0.001))
	{
		Icon_Image->SetBrush(EmptyHealthIconBrush);
	}
	else if (Icon_Image->Brush != HealthIconBrush)
	{
		Icon_Image->SetBrush(HealthIconBrush);
	}
}

void UHealthStatusWidget::UpdateIconColor(const float CurrentValue, const float MinValue, const float MaxValue)
{
	Icon_Image->SetColorAndOpacity(UKismetMathLibrary::LinearColorLerp(ZeroPercentColor, OneHundredPercentColor, UKismetMathLibrary::NormalizeToRange(CurrentValue, MinValue, MaxValue)));
}

void UHealthStatusWidget::UpdateHealthPercentage(const float CurrentValue, const float MaxValue)
{
	if (MaxValue > 0.0f)
	{
		FNumberFormattingOptions NumberFormattingOptions;
		NumberFormattingOptions.MinimumFractionalDigits = 0;
		NumberFormattingOptions.MaximumFractionalDigits = 0;
		NumberFormattingOptions.MinimumIntegralDigits = 1;
		NumberFormattingOptions.MaximumIntegralDigits = 4;
		
		Percentage_Text->SetText(FText::AsPercent(CurrentValue/MaxValue, &NumberFormattingOptions));
	}
}
