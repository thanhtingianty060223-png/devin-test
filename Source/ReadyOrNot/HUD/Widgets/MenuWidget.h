#pragma once

#include "CommonActivatableWidget.h"
#include "MenuWidget.generated.h"

UCLASS()
class UMenuWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnShow();
	UFUNCTION(BlueprintImplementableEvent, Category = "Menu Widget|Events", meta = (DisplayName = "On Show"))
	void BP_OnShow() const;

protected:
	virtual void NativeConstruct() override;
	
	UFUNCTION(BlueprintCallable, Category = "Menu Widget|Animation")
	void PlayWidgetAnimation_Internal(class UWidgetAnimation* InWidgetAnimation, bool bRestartIfAlreadyPlaying = false);
};
