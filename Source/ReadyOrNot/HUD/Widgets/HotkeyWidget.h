// Void Interactive, 2020

#pragma once

#include "Blueprint/UserWidget.h"
#include "HotkeyWidget.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UHotkeyWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Hotkey Widget")
	void RefreshHotkey();

	UFUNCTION(BlueprintCallable, Category = "Hotkey Widget")
	void SetHotkeyImage(const FSlateBrush& Brush);
	
	UFUNCTION(BlueprintCallable, Category = "Hotkey Widget")
	void SetHotkeyRemainingUses(int32 InRemainingUses);
	
	UFUNCTION(BlueprintCallable, Category = "Hotkey Widget|Animation")
	void PlayPressedAnimation();
	
	UFUNCTION(BlueprintCallable, Category = "Hotkey Widget|Animation")
    void PlayReleasedAnimation();

	FORCEINLINE class UImage* GetDividerImage() const { return HotkeyDivider_Image; }
	
protected:
	void NativePreConstruct() override;
	void NativeConstruct() override;
	
	UPROPERTY(BlueprintReadOnly, Category = "Hotkey Widget|Required Widgets", meta = (BindWidget), Transient)
	class UTextBlock* Hotkey_Text = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Hotkey Widget|Required Widgets", meta = (BindWidget), Transient)
	class UImage* Hotkey_Image = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Hotkey Widget|Required Widgets", meta = (BindWidget), Transient)
	class UImage* HotkeyDivider_Image = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Hotkey Widget|Required Widgets", meta = (BindWidget), Transient)
	class UTextBlock* RemainingUses_Text = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Hotkey Widget|Required Widget Animations", meta = (BindWidgetAnim), Transient)
	UWidgetAnimation* Anim_Pressed = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Hotkey Widget|Required Widget Animations", meta = (BindWidgetAnim), Transient)
	UWidgetAnimation* Anim_Released = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hotkey Widget")
	FName InputName = NAME_None;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hotkey Widget")
	FSlateBrush Icon;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hotkey Widget")
	uint8 bCustomAnimation : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hotkey Widget")
	uint8 bShowRemainingUses : 1;

private:
	void SetHotkeyText(const FName& HotkeyInputName);
};
