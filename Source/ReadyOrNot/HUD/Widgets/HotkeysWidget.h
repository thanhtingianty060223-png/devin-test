// Void Interactive, 2020

#pragma once

#include "HUD/Widgets/BaseWidget.h"
#include "HotkeysWidget.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UHotkeysWidget : public UBaseWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Hotkeys Widget")
	void RefreshHotkeyInput();

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	
	virtual void RefreshHotkeyInput_Implementation();

	UFUNCTION(BlueprintCallable, Category = "Hotkeys Widget")
	void SetHotkeyVisibility(UWidget* Widget, bool bCondition);
	
	UPROPERTY(BlueprintReadOnly, Category = "Hotkeys Widget|Required Widgets", meta = (BindWidget))
	class UOverlay* Hotkeys_Overlay = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Hotkeys Widget|Required Widgets", meta = (BindWidget))
	class UHorizontalBox* Hotkeys_Container = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Hotkeys Widget|Required Widgets", meta = (BindWidgetOptional))
	class UHotkeyWidget* Hotkey_Grenade = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Hotkeys Widget|Required Widgets", meta = (BindWidgetOptional))
    class UHotkeyWidget* Hotkey_Chemlight = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Hotkeys Widget|Required Widgets", meta = (BindWidgetOptional))
    class UHotkeyWidget* Hotkey_NVG = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Hotkeys Widget|Required Widgets", meta = (BindWidgetOptional))
    class UHotkeyWidget* Hotkey_Scope = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Hotkeys Widget|Required Widgets", meta = (BindWidgetOptional))
	class UWidgetSwitcher* IlluminatorAttachment_Switcher = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Hotkeys Widget|Required Widgets", meta = (BindWidgetOptional))
    class UHotkeyWidget* Hotkey_Laser = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Hotkeys Widget|Required Widgets", meta = (BindWidgetOptional))
    class UHotkeyWidget* Hotkey_Flashlight = nullptr;
		
	UPROPERTY(BlueprintReadWrite, Category = "Hotkeys Widget")
	class APlayerCharacter* PlayerCharacter = nullptr;

private:
	TArray<UWidget*> GetAllUncollapsedWidgets() const;

	void SetHotkeyVisibility(class UHotkeyWidget* HotkeyWidget, UWidget* ParentWidget);
};
