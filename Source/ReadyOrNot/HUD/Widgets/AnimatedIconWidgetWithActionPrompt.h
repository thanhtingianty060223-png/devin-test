// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "AnimatedIconWidget.h"
#include "Blueprint/UserWidget.h"

#include "AnimatedIconWidgetWithActionPrompt.generated.h"

UCLASS(Abstract)
class READYORNOT_API UAnimatedIconWidgetWithActionPrompt : public UAnimatedIconWidget
{
	GENERATED_BODY()

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Animated Icon Widget|Required Widgets", meta = (BindWidget), Transient)
	class UVerticalBox* VerticalBox = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Animated Icon Widget|Required Widgets", meta = (BindWidget), Transient)
	class UOverlay* InteractIcon_Overlay = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Animated Icon Widget|Required Widgets", meta = (BindWidget), Transient)
	class UScaleBox* PlayerActionPrompt_ScaleBox = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Animated Icon Widget|Required Widgets", meta = (BindWidget), Transient)
	class UPlayerActionPromptWidget* PlayerActionPrompt_Widget = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Animated Icon Widget|Required Widgets", meta = (BindWidget), Transient)
	class UPlayerActionPromptWidget* PlayerActionPrompt_Widget2 = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Animated Icon Widget|Required Widgets", meta = (BindWidget), Transient)
	class UPlayerActionPromptWidget* PlayerActionPrompt_Widget3 = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Animated Icon Widget|Required Widgets", meta = (BindWidget), Transient)
	class UPlayerActionPromptWidget* PlayerActionPrompt_Widget4 = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Animated Icon Widget|Required Widgets", meta = (BindWidget), Transient)
	class UImage* ActionPromptBackground_Image = nullptr;

public:
	void ShowActionPrompt(bool bShow, int32 SlotIndex);
	void SetActionPromptText(FText& Text, int32 SlotIndex);
	void EnableActionPromptBackground(bool bEnable);
	void ShowInteractIcon(bool bShow);
};
