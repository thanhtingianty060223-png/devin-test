// Void Interactive, 2020

#pragma once

#include "Blueprint/UserWidget.h"
#include "HealthStatusWidget.generated.h"

/**
 * 
 */
UCLASS(meta=(DisableNativeTick))
class READYORNOT_API UHealthStatusWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Health Status Widget")
	void AutoDetermineIconImage();

	UFUNCTION(BlueprintCallable, Category = "Health Status Widget")
	void UpdateIconColor(float CurrentValue, float MinValue, float MaxValue);
	
	UFUNCTION(BlueprintCallable, Category = "Health Status Widget")
	void UpdateHealthPercentage(float CurrentValue, float MaxValue);

protected:
	void NativePreConstruct() override;

	UPROPERTY(BlueprintReadOnly, Category = "Player Health Status Widget|Required Widgets", meta = (BindWidget))
	class UImage* Icon_Image = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Player Health Status Widget|Required Widgets", meta = (BindWidget))
	class UTextBlock* Percentage_Text = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Health Status Widget", meta = (ExposeOnSpawn = true))
	FSlateBrush HealthIconBrush;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Health Status Widget", meta = (ExposeOnSpawn = true))
	FSlateBrush EmptyHealthIconBrush;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Health Status Widget", meta = (ExposeOnSpawn = true))
	FLinearColor ZeroPercentColor = FColor::FromHex("FF2727");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Health Status Widget", meta = (ExposeOnSpawn = true))
	FLinearColor OneHundredPercentColor = FColor::FromHex("A5FF6C");

private:
	void UpdateIconImage(const FSlateBrush& Brush);
};
