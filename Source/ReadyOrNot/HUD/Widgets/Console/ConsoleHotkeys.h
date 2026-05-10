// Copyright Void Interactive, 2022

#pragma once

#include "ConsoleHotkey.h"
#include "Blueprint/UserWidget.h"
#include "ConsoleHotkeys.generated.h"


UENUM()
enum EConsoleHotkeysLayout
{
	DefaultLayout,
	CommandLayout,
	ItemWheelLayout,
};

UCLASS()
class READYORNOT_API UConsoleHotkeys : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual bool Initialize() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	UConsoleHotkey* Hotkey_NVG = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	UConsoleHotkey* Hotkey_Laser = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	UConsoleHotkey* Hotkey_Firemode = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	UConsoleHotkey* Hotkey_TeamView = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	UConsoleHotkey* Hotkey_Chemlight = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	UConsoleHotkey* Hotkey_ItemWheel = nullptr;

	UFUNCTION()
	void ItemEquipped(ABaseItem* Item);

public:
	UFUNCTION()
	void SetLayout(EConsoleHotkeysLayout Layout);
	UFUNCTION(BlueprintCallable)
	void RefreshHotkeys();

private:
	EConsoleHotkeysLayout ActiveLayout = DefaultLayout;

	UPROPERTY()
	APlayerCharacter* PlayerCharacter;

	UPROPERTY()
	ABaseItem* LastEquippedItem;

	bool bIsNVGActive = false;
	bool bHasFireModes = false;
	bool bHasLaserAttachment = false;
	bool bHasChemlightInInventory = false;
};
