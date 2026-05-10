// Copyright Void Interactive, 2022

#pragma once

#include "Blueprint/UserWidget.h"
#include "PlayerPaperdollWidget.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UPlayerPaperdollWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	//virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	class UWidgetSwitcher* StanceSwitcher = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	class UWidgetSwitcher* Stand_CarrySwitcher = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	class UWidgetSwitcher* Crouch_CarrySwitcher = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	class UOverlay* Stand_Overlay = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	class UOverlay* Crouch_Overlay = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	class UOverlay* StandCarry_Overlay = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	class UOverlay* StandNoCarry_Overlay = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	class UOverlay* CrouchCarry_Overlay = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	class UOverlay* CrouchNoCarry_Overlay = nullptr;

	// Stand
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	UImage* Outline_Image = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	UImage* Head_Image = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	UImage* Body_Image = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	UImage* RightArm_Image = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	UImage* LeftArm_Image = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	UImage* RightLeg_Image = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	UImage* LeftLeg_Image = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	UImage* Headwear_Image = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	UImage* BodyArmor_Image = nullptr;

	// Stand Carry
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	UImage* Carry_Outline_Image = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	UImage* Carry_Head_Image = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	UImage* Carry_Body_Image = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	UImage* Carry_RightArm_Image = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	UImage* Carry_LeftArm_Image = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	UImage* Carry_BodyArmor_Image = nullptr;

	// Crouch
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	UImage* Outline_Crouch_Image = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	UImage* Head_Crouch_Image = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	UImage* Body_Crouch_Image = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	UImage* RightArm_Crouch_Image = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	UImage* LeftArm_Crouch_Image = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	UImage* RightLeg_Crouch_Image = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	UImage* LeftLeg_Crouch_Image = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	UImage* Headwear_Crouch_Image = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	UImage* BodyArmor_Crouch_Image = nullptr;

	// Crouch Carry
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	UImage* Carry_Outline_Crouch_Image = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	UImage* Carry_Head_Crouch_Image = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	UImage* Carry_Body_Crouch_Image = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	UImage* Carry_RightArm_Crouch_Image = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	UImage* Carry_LeftArm_Crouch_Image = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	UImage* Carry_RightLeg_Crouch_Image = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	UImage* Carry_LeftLeg_Crouch_Image = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	UImage* Carry_BodyArmor_Crouch_Image = nullptr;
	
	void UpdateGearImage(UImage* Image, UTexture2D* InTexture);

	void ToggleHeadwearVisibility(bool bVisible);
	void ToggleBodyArmorVisibility(bool bVisible);

	UFUNCTION(BlueprintCallable)
	void UpdateHealth(ABaseItem* Item);
};
