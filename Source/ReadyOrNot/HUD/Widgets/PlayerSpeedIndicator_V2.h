// Void Interactive, 2020

#pragma once

#include "CoreMinimal.h"
#include "HUD/Widgets/BaseWidget.h"
#include "PlayerSpeedIndicator_V2.generated.h"

/**
 * A widget that visualizes the player's current movement speed
 */
UCLASS()
class READYORNOT_API UPlayerSpeedIndicator_V2 : public UBaseWidget
{
	GENERATED_BODY()

public:
	UPlayerSpeedIndicator_V2();

protected:
	void NativeConstruct() override;
	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	
	UPROPERTY(BlueprintReadOnly, Category = "Optional Widgets", meta = (BindWidgetOptional))
	class USizeBox* Twenty_Box = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	class UImage* Twenty_Image = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Optional Widgets", meta = (BindWidgetOptional))
	class USizeBox* Fourty_Box = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	class UImage* Fourty_Image = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Optional Widgets", meta = (BindWidgetOptional))
	class USizeBox* Sixty_Box = nullptr;
		
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	class UImage* Sixty_Image = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Optional Widgets", meta = (BindWidgetOptional))
	class USizeBox* Eighty_Box = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	class UImage* Eighty_Image = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Optional Widgets", meta = (BindWidgetOptional))
	class USizeBox* OneHundred_Box = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	class UImage* OneHundred_Image = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	class UTextBlock* SpeedPercentage_Text = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Data")
	class APlayerCharacter* PlayerCharacter = nullptr;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings")
	float BaselineOpacity = 0.65f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings")
	float FadeSpeed = 9.0f;
	
	UPROPERTY(BlueprintReadOnly, Category = "Data")
	float LastSetRunSpeedPercent = 0.0f;
	
	UPROPERTY(BlueprintReadOnly, Category = "Data")
	float MinRunSpeedPercent = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Data")
	float MaxRunSpeedPercent = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Data")
	float NormalizedRunSpeedPercent = 0.0f;

private:
	void SetSpeedBlockImageVisibility(class UImage* InImage, bool bVisible);
	void SetSpeedBlockBoxVisibility(class USizeBox* InSizeBox, bool bVisible);
};
