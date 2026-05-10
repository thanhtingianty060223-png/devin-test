// Copyright Void Interactive, 2023

#pragma once

#include "Blueprint/UserWidget.h"
#include "AnimatedIconWidget_Imprint.generated.h"

UCLASS(Abstract)
class READYORNOT_API UAnimatedIconWidget_Imprint : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Icon")
	void Init(FVector InWorldLocation, UTexture2D* InIconImage);
	
	UFUNCTION(BlueprintCallable, Category = "Icon")
	void SetIconImage(UTexture2D* NewIconImage);
	
protected:
	virtual void OnAnimationFinished_Implementation(const UWidgetAnimation* Animation) override;

	UPROPERTY(BlueprintReadOnly, Category = "Data", meta = (BindWidget))
	class UImage* Icon_Image = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Data", meta = (BindWidgetAnim), Transient)
	class UWidgetAnimation* ImprintAnimation = nullptr;
};
