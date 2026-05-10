// Void Interactive, 2020

#pragma once

#include "BaseWidget.h"
#include "CurrentMatchRoundWidget.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UCurrentMatchRoundWidget : public UBaseWidget
{
	GENERATED_BODY()

protected:
	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(BlueprintReadOnly, Category = "Current Match Round Widget|Required Widgets", meta = (BindWidget))
	class UTextBlock* CurrentRound_Text = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Current Match Round Widget|Required Widgets", meta = (BindWidget))
	class UTextBlock* CurrentRound_Text_Style2 = nullptr;
};
