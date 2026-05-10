// Void Interactive, 2020

#pragma once

#include "MapActorWidget.h"
#include "MapActorIconWidget.generated.h"

/**
 * 
 */
UCLASS(Abstract)
class READYORNOT_API UMapActorIconWidget : public UMapActorWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Map Actor Icon Widget")
	void SetIconBrushStyle(FSlateBrush InIconBrush, FLinearColor InIconColor);
	
	UFUNCTION(BlueprintCallable, Category = "Map Actor Icon Widget")
	void SetIconColor(FLinearColor InIconColor);

protected:
	void UpdateMapActorTranslation() override;
	
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	class UImage* Icon_Image = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
    class UImage* Icon_Image_BG = nullptr;
};
