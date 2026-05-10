// Void Interactive, 2020

#pragma once

#include "HUD/Widgets/BaseWidget.h"
#include "MatchStatusCardWidget.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UMatchStatusCardWidget : public UBaseWidget
{
	GENERATED_BODY()

protected:
	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(BlueprintReadOnly, Category = "Match Time Remaining Widget|Required Widgets", meta = (BindWidget))
	class UMatchTimeRemainingWidget* MatchTimeRemaining = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Match Time Remaining Widget|Required Widgets", meta = (BindWidget))
	class UCurrentMatchRoundWidget* CurrentMatchRound = nullptr;
};
