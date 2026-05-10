// Void Interactive, 2020

#pragma once

#include "HUD/Widgets/BaseWidget.h"
#include "MatchTimeRemainingWidget.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UMatchTimeRemainingWidget : public UBaseWidget
{
	GENERATED_BODY()

protected:
	void NativeConstruct() override;
	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(BlueprintReadOnly, Category = "Match Time Remaining Widget|Required Widgets", meta = (BindWidget))
	class UTextBlock* MatchTimeRemaining_Text = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Match Time Remaining Widget|Data")
	float RoundTimeRemaining = 0.0f;
};
