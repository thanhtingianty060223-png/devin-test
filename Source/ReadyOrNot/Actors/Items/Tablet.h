// Copyright Void Interactive, 2023

#pragma once

#include "Actors/BaseItem.h"
#include "Tablet.generated.h"

// DECLARE_DELEGATE_OneParam(FTabletSoundEventDelegate, class UFMODEvent* /* Event */)
// DECLARE_DELEGATE(FTabletEventDelegate);

UCLASS(Abstract)
class READYORNOT_API ATablet : public ABaseItem
{
	GENERATED_BODY()

public:
	ATablet();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	float ScaleTime = 0.0f;
	float MoveTime = 0.0f;
	uint8 bStartedPlayingDraw : 1;
	uint8 bStartedPlayingHolster : 1;

	virtual bool ShouldAttachToOwner() const override;
	virtual bool PlayDraw(bool bDrawFirst) override;
	virtual bool PlayHolster() override;
	virtual void OnDrawComplete() override;
	virtual void OnHolsterComplete() override;
	virtual void UpdateFOVShader(float DeltaTime) override {}

	void SetTabletFocusBlend(float Blend);
	float CalculateTabletFov() const;
	
	UFUNCTION()
	void OnMissionSelected();
	
	UFUNCTION(BlueprintImplementableEvent, Category = "Tablet")
	void WakeScreen();

	UFUNCTION(BlueprintImplementableEvent, Category = "Tablet")
	void SleepScreen();

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void PlaySoundEvent(UFMODEvent* Event);

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void PlayNotificationEvent();

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void PlayVibrationEvent();
	
	UPROPERTY(BlueprintReadWrite)
	class UWidgetComponent* WidgetComponent;

	UPROPERTY(BlueprintReadOnly)
	bool bIsTabletAwake = false;
	
	// static FTabletSoundEventDelegate TriggerTabletSoundEvent;
	// static FTabletEventDelegate TriggerTabletNotification;
	// static FTabletEventDelegate TriggerTabletVibration;

	UPROPERTY(EditAnywhere)
	FRotator FocusedCameraRotation = FRotator(-27.0f, 90.0f, 0.0f);

	UPROPERTY(EditAnywhere)
	FRotator FocusedItemRotation;
	
	UPROPERTY(EditAnywhere)
	float FocusedMinimumHorizontalFov = 44.0f;

	UPROPERTY(EditAnywhere)
	float FocusedTargetVerticalFov = 21.0f;

	UPROPERTY(EditAnywhere)
	float TabletFocusInterpSpeed = 7.5f;
	
	// This is used for tracking in multi-weapon equipped setup
	bool bTabletDrawn = false;
	
private:
	void UpdateLocationAndScale();
	
	void OpenTabletPressed();
	void OpenTabletReleased();

	void TryNextPlayerView_Pressed();
	void TryNextPlayerView_Released();
	
	bool bTabletButtonHeld = false;
	float TabletFocusTimer = 0.0f;
};
