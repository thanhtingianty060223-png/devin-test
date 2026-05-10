// Copyright Void Interactive, 2022

#pragma once
#include "ConsoleFireMode.h"

#include "ConsoleFireModes.generated.h"


UCLASS()
class READYORNOT_API UConsoleFireModes : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual bool Initialize() override;

	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	UUserWidget* FireModeSafe;

	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	UUserWidget* FireModeSingle; 

	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	UUserWidget* FireModeBurst; 

	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	UUserWidget* FireModeAuto;

	UFUNCTION()
	void OnEquipped(ABaseItem* Item);
	UFUNCTION()
	void OnFireModeChanged(APlayerCharacter* PlayerCharacter, EFireMode NewFireMode, EFireMode LastFireMode);
	void UpdateSelectedFireMode(UUserWidget* FireMode, EFireMode WidgetFireMode, EFireMode NewFireMode);
	void UpdateAvailableFireModes(ABaseWeapon* Weapon);
};
