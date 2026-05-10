// Void Interactive, 2020

#include "TeamProgressScoreWidget.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UTeamProgressScoreWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	switch (Team)
	{
		case ETeamType::TT_NONE:
			ProgressBar_LeftAligned->SetFillColorAndOpacity(FColor::White);
			ProgressBar_RightAligned->SetFillColorAndOpacity(FColor::White);
		
			Score_Text_LeftAligned->SetColorAndOpacity(FSlateColor(FColor::White));
			Score_Text_RightAligned->SetColorAndOpacity(FSlateColor(FColor::White));
		break;

		case ETeamType::TT_SERT_RED:
			ProgressBar_LeftAligned->SetFillColorAndOpacity(FColor::FromHex("FF2C35"));
			ProgressBar_RightAligned->SetFillColorAndOpacity(FColor::FromHex("FF2C35"));

			Score_Text_LeftAligned->SetColorAndOpacity(FSlateColor(FColor::FromHex("FF2C35")));
			Score_Text_RightAligned->SetColorAndOpacity(FSlateColor(FColor::FromHex("FF2C35")));
		break;

		case ETeamType::TT_SERT_BLUE:
			ProgressBar_LeftAligned->SetFillColorAndOpacity(FColor::FromHex("1C85F4"));
			ProgressBar_RightAligned->SetFillColorAndOpacity(FColor::FromHex("1C85F4"));
			
			Score_Text_LeftAligned->SetColorAndOpacity(FSlateColor(FColor::FromHex("1C85F4")));
			Score_Text_RightAligned->SetColorAndOpacity(FSlateColor(FColor::FromHex("1C85F4")));
		break;

		case ETeamType::TT_SUSPECT:
			ProgressBar_LeftAligned->SetFillColorAndOpacity(FColor::FromHex("FF2C35"));
			ProgressBar_RightAligned->SetFillColorAndOpacity(FColor::FromHex("FF2C35"));
			
			Score_Text_LeftAligned->SetColorAndOpacity(FSlateColor(FColor::FromHex("FF2C35")));
			Score_Text_RightAligned->SetColorAndOpacity(FSlateColor(FColor::FromHex("FF2C35")));
		break;

		case ETeamType::TT_CIVILIAN:
			ProgressBar_LeftAligned->SetFillColorAndOpacity(FColor::FromHex("1C85F4"));
			ProgressBar_RightAligned->SetFillColorAndOpacity(FColor::FromHex("1C85F4"));
			
			Score_Text_LeftAligned->SetColorAndOpacity(FSlateColor(FColor::FromHex("1C85F4")));
			Score_Text_RightAligned->SetColorAndOpacity(FSlateColor(FColor::FromHex("1C85F4")));
		break;

		case ETeamType::TT_SQUAD:
			ProgressBar_LeftAligned->SetFillColorAndOpacity(FColor::FromHex("F4D143"));
			ProgressBar_RightAligned->SetFillColorAndOpacity(FColor::FromHex("F4D143"));
			
			Score_Text_LeftAligned->SetColorAndOpacity(FSlateColor(FColor::FromHex("F4D143")));
			Score_Text_RightAligned->SetColorAndOpacity(FSlateColor(FColor::FromHex("F4D143")));
		break;
		
		default:
			ProgressBar_LeftAligned->SetFillColorAndOpacity(FColor::White);
			ProgressBar_RightAligned->SetFillColorAndOpacity(FColor::White);
			
			Score_Text_LeftAligned->SetColorAndOpacity(FSlateColor(FColor::White));
			Score_Text_RightAligned->SetColorAndOpacity(FSlateColor(FColor::White));
		break;
	}
}

void UTeamProgressScoreWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (RONGS)
	{
		if (Team == ETeamType::TT_SERT_BLUE || Team == ETeamType::TT_CIVILIAN)
		{
			UpdateProgressScore(RONGS->GetCurrentSwatScore(), RONGS->GetMaxSwatScore());
		}
		else if (Team == ETeamType::TT_SERT_RED || Team == ETeamType::TT_SUSPECT)
		{
			UpdateProgressScore(RONGS->GetCurrentSuspectScore(), RONGS->GetMaxSuspectScore());
		}
	}
}

void UTeamProgressScoreWidget::UpdateProgressScore(const int32& CurrentScore, const int32& MaxScore)
{
	ProgressBar_LeftAligned->SetPercent(static_cast<float>(CurrentScore)/static_cast<float>(MaxScore));
	ProgressBar_RightAligned->SetPercent(static_cast<float>(CurrentScore)/static_cast<float>(MaxScore));

	FNumberFormattingOptions NumberFormattingOptions;
	NumberFormattingOptions.MinimumIntegralDigits = 1;
	NumberFormattingOptions.MaximumIntegralDigits = 2;
	Score_Text_LeftAligned->SetText(FText::AsNumber(CurrentScore, &NumberFormattingOptions));
	Score_Text_RightAligned->SetText(FText::AsNumber(CurrentScore, &NumberFormattingOptions));
}
