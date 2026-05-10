// Void Interactive, 2020

#include "LoudnessMeterWidget.h"

#include "Components/WidgetSwitcher.h"

void ULoudnessMeterWidget::NativeConstruct()
{
	Super::NativeConstruct();

	PlayerCharacter = Cast<APlayerCharacter>(GetOwningPlayerPawn());
}

void ULoudnessMeterWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (PlayerCharacter)
	{
		if (PlayerCharacter->IsMoving())
		{
			SetRenderOpacity(FMath::FInterpConstantTo(GetRenderOpacity(), 1.0f, InDeltaTime, 20.0f));
			FadeOutDelayTime = 0.5f;
		}
		else
		{
			if (FadeOutDelayTime <= 0.0f)
				SetRenderOpacity(FMath::FInterpConstantTo(GetRenderOpacity(), 0.0f, InDeltaTime, 20.0f));
		}
		
		MovementSound_WidgetSwitcher->SetActiveWidgetIndex((PlayerCharacter->CurrentRunSpeedPercent >= 0.8f ? 2 : PlayerCharacter->CurrentRunSpeedPercent >= 0.26f ? 1 : PlayerCharacter->CurrentRunSpeedPercent >= 0.0f ? 0 : 0));
		
		FadeOutDelayTime -= InDeltaTime;
	}
}
