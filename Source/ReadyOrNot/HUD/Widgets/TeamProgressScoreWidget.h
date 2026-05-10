// Void Interactive, 2020

#pragma once

#include "HUD/Widgets/BaseWidget.h"
#include "TeamProgressScoreWidget.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UTeamProgressScoreWidget : public UBaseWidget
{
	GENERATED_BODY()

protected:
	void NativePreConstruct() override;
	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exposed Properties", meta = (ExposeOnSpawn = true))
	ETeamType Team = ETeamType::TT_NONE;
	
	UPROPERTY(BlueprintReadOnly, Category = "Match Time Remaining Widget|Required Widgets", meta = (BindWidget))
	class UProgressBar* ProgressBar_LeftAligned = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Match Time Remaining Widget|Required Widgets", meta = (BindWidget))
	class UTextBlock* Score_Text_LeftAligned = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Match Time Remaining Widget|Required Widgets", meta = (BindWidget))
	class UProgressBar* ProgressBar_RightAligned = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Match Time Remaining Widget|Required Widgets", meta = (BindWidget))
	class UTextBlock* Score_Text_RightAligned = nullptr;

private:
	void UpdateProgressScore(const int32& CurrentScore, const int32& MaxScore);
};
