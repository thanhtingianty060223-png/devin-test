// Copyright Void Interactive, 2022

#pragma once

#include "ConsoleSelection.h"
#include "ConsoleFireModes.h"
#include "ConsoleMagSelection.h"
#include "Components/WidgetSwitcher.h"
#include "ConsoleContextSwitcher.generated.h"

UCLASS()
class READYORNOT_API UConsoleContextSwitcher : public UBaseWidget
{
	GENERATED_BODY()

protected:
	virtual bool Initialize() override;
	UFUNCTION()
	void SwapToGrenades();
	UFUNCTION()
	void SwapToTactical();
	UFUNCTION()
	void SwapToFireModes();
	UFUNCTION()
	void SwapToMag(ABaseMagazineWeapon* MagazineWeapon);

	void PlayFadeIn();

	void QueueFadeOut();
	void PlayFadeOut();

	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	UWidgetSwitcher* ContextSwitcher;

	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	UConsoleSelection* GrenadeSelection;

	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	UConsoleSelection* TacticalSelection;

	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	UConsoleFireModes* FireModes;

	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	UConsoleMagSelection* MagSelection;

	UPROPERTY(BlueprintReadOnly, Category = "Required Animations", meta = (BindWidgetAnim), Transient)
	UWidgetAnimation* FadeIn;

	UPROPERTY(BlueprintReadOnly, Category = "Required Animations", meta = (BindWidgetAnim), Transient)
	UWidgetAnimation* FadeOut;

private:
	FTimerHandle AnimationTimer;
};
