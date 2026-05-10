// Copyright Void Interactive, 2021

#pragma once

#include "HUD/Widgets/BaseWidget.h"
#include "RoundEndWidget_PVP.generated.h"

/**
 * 
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class READYORNOT_API URoundEndWidget_PVP : public UBaseWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Round End Widget PVP")
	void OnGameModeRoundEnded();

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Round End Widget PVP", meta = (BindWidgetAnim), Transient)
	UWidgetAnimation* Anim_FadeIn;

	virtual void OnGameModeRoundEnded_Implementation();
	
private:
	void FadeIn();
};
