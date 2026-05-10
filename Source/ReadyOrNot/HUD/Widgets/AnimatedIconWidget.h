// Copyright Void Interactive, 2023

#pragma once

#include "Blueprint/UserWidget.h"
#include "AnimatedIconWidget.generated.h"

DECLARE_STATS_GROUP(TEXT("Animated Icon Widget"), STATGROUP_AnimatedIconWidget, STATCAT_Advanced);

UCLASS(Abstract)
class READYORNOT_API UAnimatedIconWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;
	
	void SetParentComponent(UInteractableComponent* NewParent) { ParentComponent = NewParent; }

	void ResetAnim();
	
	UFUNCTION(BlueprintImplementableEvent, Category = "Icon Widget")
	void SetInteractState(bool bValid);
	
	UFUNCTION(BlueprintCallable, Category = "Icon Widget")
	void SetInteractIconSize(float InInteractCircleSize, float InInteractIconSize = 48.0f);
	
	UFUNCTION(BlueprintCallable, Category = "Icon Widget")
    void PlayInteractAnim();
    
	UFUNCTION(BlueprintCallable, Category = "Icon Widget")
	void StopInteractAnim();
	
	UFUNCTION(BlueprintCallable, Category = "Icon Widget")
	void PlayFocusAnim(bool bReverse = false);
	
	UFUNCTION(BlueprintCallable, Category = "Icon Widget")
	void StopFocusAnim();
	
	UFUNCTION(BlueprintCallable, Category = "Icon Widget")
	void PauseIconAnim();
	
	UFUNCTION(BlueprintCallable, Category = "Icon Widget")
	void UnPauseIconAnim();

	UFUNCTION(BlueprintCallable, Category = "Icon Widget")
	void SetActiveIcon(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Icon Widget")
	void SetCurrentProgress(float Percent);

	UPROPERTY(BlueprintReadOnly, Category = "Animated Icon Widget")
	TArray<UImage*> IconImages;
	
	UPROPERTY(BlueprintReadOnly, Category = "Animated Icon Widget")
	int32 CurrentIndex = 0;
	
	UPROPERTY(BlueprintReadOnly, Category = "Animated Icon Widget")
	float ElapsedTime = 0.0f;
	
	UPROPERTY(BlueprintReadOnly, Category = "Animated Icon Widget")
	uint8 bPaused : 1;

protected:
	virtual void NativeConstruct() override;
	virtual void OnAnimationStarted_Implementation(const UWidgetAnimation * Animation) override;
	virtual void OnAnimationFinished_Implementation(const UWidgetAnimation * Animation) override;

	UPROPERTY(BlueprintReadOnly, Category = "Animated Icon Widget|Required Widgets", meta = (BindWidget), Transient)
	class UOverlay* InteractCircle_Overlay = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Animated Icon Widget|Required Widgets", meta = (BindWidget), Transient)
	class USizeBox* InteractIcon_SizeBox = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Animated Icon Widget|Required Widgets", meta = (BindWidget), Transient)
	class UWidgetSwitcher* IconSwitcher = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Animated Icon Widget|Required Widgets", meta = (BindWidget), Transient)
	class UImage* Frame_1 = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Animated Icon Widget|Required Widgets", meta = (BindWidget), Transient)
	class UImage* Frame_2 = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Animated Icon Widget|Required Widgets", meta = (BindWidget), Transient)
	class UImage* Frame_3 = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Animated Icon Widget|Required Widgets", meta = (BindWidget), Transient)
	class UImage* Frame_4 = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Animated Icon Widget|Required Widgets", meta = (BindWidget), Transient)
	class UImage* Frame_5 = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Animated Icon Widget|Required Widgets", meta = (BindWidget), Transient)
	class UImage* Frame_6 = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Animated Icon Widget|Required Widgets", meta = (BindWidget), Transient)
	class UImage* Frame_7 = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Animated Icon Widget|Required Widgets", meta = (BindWidget), Transient)
	class UImage* Frame_8 = nullptr;

	UPROPERTY()
	class UImage* FrameImages[8];

	UPROPERTY(BlueprintReadOnly, Category = "Animated Icon Widget|Required Widgets", meta = (BindWidget), Transient)
	class UImage* ProgressCircle_Image = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Icon Widget|Required Widgets", meta = (BindWidgetAnim), Transient)
    class UWidgetAnimation* Anim_Interact = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Icon Widget|Required Widgets", meta = (BindWidgetAnim), Transient)
    class UWidgetAnimation* Anim_Focus = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Setup")
	FLinearColor InteractionInvalidTintColor = FLinearColor::Red;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Setup")
	class UMaterialInterface* ProgressCircleMaterial = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Setup")
	FName ProgressParamName = NAME_None;

	// Set from UInteractableComponent, used to access data from interactables
	UPROPERTY(BlueprintReadOnly, Category = "Animated Icon Widget")
	class UInteractableComponent* ParentComponent = nullptr;
	
	UPROPERTY()
	class UMaterialInstanceDynamic* MID_ProgressCircle = nullptr;
};
