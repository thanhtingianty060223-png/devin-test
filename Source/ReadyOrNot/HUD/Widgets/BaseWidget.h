// Copyright Void Interactive, 2021

#pragma once

#include "Blueprint/UserWidget.h"
#include "BaseWidget.generated.h"

/**
* Base class for all Ready or Not widgets
*/
UCLASS()
class READYORNOT_API UBaseWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Base Widget")
    void ToggleWidgetVisibility(bool bNotHitTestable = true);
	
	UFUNCTION(BlueprintCallable, Category = "Base Widget")
    void ShowWidget(bool bNotHitTestable = true);
	
	UFUNCTION(BlueprintCallable, Category = "Base Widget")
	void HideWidget();
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Base Widget|Debug", meta = (DevelopmentOnly))
	bool UpdateDebugInfo();
	
protected:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category = "Base Widget|Animation")
	void PlayWidgetAnimation_Internal(class UWidgetAnimation* InWidgetAnimation, bool bRestartIfAlreadyPlaying = false);
	
	UFUNCTION(BlueprintCallable, Category = "Base Widget|Animation")
	void PauseWidgetAnimation_Internal(class UWidgetAnimation* InWidgetAnimation);
	
	UFUNCTION(BlueprintCallable, Category = "Base Widget|Animation")
	void StopWidgetAnimation_Internal(class UWidgetAnimation* InWidgetAnimation);
	
	virtual bool UpdateDebugInfo_Implementation();

	// Retrieves the mouse axis delta values
	UFUNCTION(BlueprintPure, Category = "Base Widget|Mouse")
	FVector2D GetMouseDelta() const;

	// Retrieves the actual mouse position, if a mouse device is available
	UFUNCTION(BlueprintPure, Category = "Base Widget|Mouse")
    FVector2D GetMousePosition() const;

	// Has the mouse moved, ignoring any delta threshold values
	UFUNCTION(BlueprintPure, Category = "Base Widget|Mouse")
    bool HasMouseMoved() const;

	// Retrieves the center screen coordinates based on the current viewport size
	UFUNCTION(BlueprintPure, Category = "Base Widget|General")
    FVector2D GetCenterScreenPosition();

	// Are any blocking animations playing that would prevent the player from selecting anything in the menu
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Base Widget|General")
			bool IsBlockingAnimationPlaying();
	virtual bool IsBlockingAnimationPlaying_Implementation();

	// Is the mouse delta beyond the specified MouseAxisDeltaThreshold value
	UFUNCTION(BlueprintPure, Category = "Base Widget|Mouse")
    bool IsMouseAxisBeyondThreshold(const FVector2D& InMouseDelta);

	// Is the gamepad axis delta beyond the specified GamepadAxisDeltaThreshold value
	UFUNCTION(BlueprintPure, Category = "Base Widget|Gamepad")
    bool IsGamepadAxisBeyondThreshold(const FVector2D& InGamepadAxis);

	// Plays a 2D sound, given an FMOD Event
	UFUNCTION(BlueprintCallable, Category = "Base Widget|Sound")
    void PlaySoundEffect(UFMODEvent* SoundEffectToPlay);
	
	// The amount the mouse axis must move before being recognized by this widget
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Base Widget|Mouse")
	FVector2D MouseAxisDeltaThreshold = FVector2D(0.7f);

	// The amount the gamepad axis must move before being recognized by this widget
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Base Widget|Gamepad")
	FVector2D GamepadAxisDeltaThreshold = FVector2D(0.35f);

	UPROPERTY(BlueprintReadWrite, Category = "Base Widget|Data")
	class AReadyOrNotGameState* RONGS = nullptr;
};
