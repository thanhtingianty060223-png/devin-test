// Void Interactive, 2020

#pragma once

#include "HUD/Widgets/BaseWidget.h"
#include "LoudnessMeterWidget.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API ULoudnessMeterWidget : public UBaseWidget
{
	GENERATED_BODY()

protected:
	void NativeConstruct() override;
	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(BlueprintReadOnly, Category = "Loudness Meter|Required Widgets", meta = (BindWidget))
	class UWidgetSwitcher* MovementSound_WidgetSwitcher = nullptr;
	
	UPROPERTY(BlueprintReadWrite, Category = "Loudness Meter")
	class APlayerCharacter* PlayerCharacter = nullptr;

private:
	float FadeOutDelayTime = 0.5f;
};
