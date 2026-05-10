// Copyright Void Interactive, 2021

#pragma once

#include "CoreMinimal.h"
#include "RadialWidgetBase.h"
#include "WeaponWheelWidget.generated.h"

/**
 * 
 */
UCLASS(Abstract)
class READYORNOT_API UWeaponWheelWidget : public URadialWidgetBase
{
	GENERATED_BODY()

public:
	UWeaponWheelWidget(const FObjectInitializer& ObjectInitializer);
	
protected:
	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	void OnRadialMenuOpened_Implementation() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon Wheel | Settings")
	TArray<FName> Categories;
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Weapon Wheel | Interaction")
		bool RemoveNullItemsFromCategory(const FName& WeaponWheelCategoryName);
	virtual bool RemoveNullItemsFromCategory_Implementation(const FName& WeaponWheelCategoryName);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Weapon Wheel | Interaction")
			bool RemoveNullItemsFromAllCategories();
    virtual bool RemoveNullItemsFromAllCategories_Implementation();
};
