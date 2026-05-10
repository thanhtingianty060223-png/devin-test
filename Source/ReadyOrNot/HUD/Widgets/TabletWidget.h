// Copyright Void Interactive, 2023

#pragma once

#include "HUD/Widgets/MissionPlanWidget.h"
#include "TabletWidget.generated.h"

UCLASS()
class READYORNOT_API UTabletWidget final : public UMissionPlanWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UTeamViewWidget* TeamView = nullptr;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UWidgetSwitcher* ScreenSwitcher = nullptr;

	UFUNCTION(BlueprintCallable)
	int GetActiveButton(int currentIndex, int navigationDirection, TArray<bool> buttonVisibilities);
	
	bool IsTeamViewFocused() const;
	
protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
};
