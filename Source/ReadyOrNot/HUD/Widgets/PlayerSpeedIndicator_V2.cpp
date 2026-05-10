// Void Interactive, 2020

#include "PlayerSpeedIndicator_V2.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"

UPlayerSpeedIndicator_V2::UPlayerSpeedIndicator_V2()
{
}

void UPlayerSpeedIndicator_V2::NativeConstruct()
{
	Super::NativeConstruct();

	PlayerCharacter = Cast<APlayerCharacter>(GetOwningPlayerPawn());
}

void UPlayerSpeedIndicator_V2::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (PlayerCharacter)
	{
		if (LastSetRunSpeedPercent != PlayerCharacter->CurrentRunSpeedPercent)
		{
			MinRunSpeedPercent = PlayerCharacter->MinWalkSpeedPercent;
			MaxRunSpeedPercent = PlayerCharacter->MaxRunSpeedPercent;
			LastSetRunSpeedPercent = PlayerCharacter->CurrentRunSpeedPercent;
			NormalizedRunSpeedPercent = UKismetMathLibrary::NormalizeToRange(LastSetRunSpeedPercent, MinRunSpeedPercent, MaxRunSpeedPercent);

			BaselineOpacity = 4.0f;
		}

		Twenty_Image->SetOpacity(FMath::FInterpConstantTo(Twenty_Image->ColorAndOpacity.A, BaselineOpacity, InDeltaTime, FadeSpeed));
		Fourty_Image->SetOpacity(FMath::FInterpConstantTo(Fourty_Image->ColorAndOpacity.A, BaselineOpacity, InDeltaTime, FadeSpeed));
		Sixty_Image->SetOpacity(FMath::FInterpConstantTo(Sixty_Image->ColorAndOpacity.A, BaselineOpacity, InDeltaTime, FadeSpeed));
		Eighty_Image->SetOpacity(FMath::FInterpConstantTo(Eighty_Image->ColorAndOpacity.A, BaselineOpacity, InDeltaTime, FadeSpeed));
		OneHundred_Image->SetOpacity(FMath::FInterpConstantTo(OneHundred_Image->ColorAndOpacity.A, BaselineOpacity, InDeltaTime, FadeSpeed));

		SpeedPercentage_Text->SetOpacity(FMath::FInterpConstantTo(OneHundred_Image->ColorAndOpacity.A, BaselineOpacity, InDeltaTime, FadeSpeed));

		FNumberFormattingOptions NumberFormattingOptions;
		NumberFormattingOptions.MinimumIntegralDigits = 1;
		NumberFormattingOptions.MaximumIntegralDigits = 3;
		NumberFormattingOptions.MinimumFractionalDigits = 0;
		NumberFormattingOptions.MaximumFractionalDigits = 0;
		SpeedPercentage_Text->SetText(FText::AsPercent(FMath::GridSnap(LastSetRunSpeedPercent, 0.2f), &NumberFormattingOptions));

		SetSpeedBlockBoxVisibility(Twenty_Box, MaxRunSpeedPercent >= 0.0f);
		SetSpeedBlockBoxVisibility(Fourty_Box, MaxRunSpeedPercent >= 0.3f);
		SetSpeedBlockBoxVisibility(Sixty_Box, MaxRunSpeedPercent >= 0.5f);
		SetSpeedBlockBoxVisibility(Eighty_Box, MaxRunSpeedPercent >= 0.7f);
		SetSpeedBlockBoxVisibility(OneHundred_Box, MaxRunSpeedPercent >= 0.9f);
		
		SetSpeedBlockImageVisibility(Twenty_Image, LastSetRunSpeedPercent >= 0.0f);
		SetSpeedBlockImageVisibility(Fourty_Image, LastSetRunSpeedPercent >= 0.3f);
		SetSpeedBlockImageVisibility(Sixty_Image, LastSetRunSpeedPercent >= 0.5f);
		SetSpeedBlockImageVisibility(Eighty_Image, LastSetRunSpeedPercent >= 0.7f);
		SetSpeedBlockImageVisibility(OneHundred_Image, LastSetRunSpeedPercent >= 0.9f);

		BaselineOpacity = FMath::FInterpConstantTo(BaselineOpacity, 0.65f, InDeltaTime, FadeSpeed);
	}
}

void UPlayerSpeedIndicator_V2::SetSpeedBlockImageVisibility(UImage* InImage, const bool bVisible)
{
	if (InImage)
	{
		InImage->SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Hidden);
	}
}

void UPlayerSpeedIndicator_V2::SetSpeedBlockBoxVisibility(USizeBox* InSizeBox, const bool bVisible)
{
	if (InSizeBox)
	{
		InSizeBox->SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}
