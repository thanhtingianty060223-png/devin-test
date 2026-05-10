// Copyright Void Interactive, 2021

#include "WeaponWheelWidget.h"
#include "ReadyOrNot.h"

UWeaponWheelWidget::UWeaponWheelWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Categories.Empty();
	Categories.Add("Assistants");
	Categories.Add("Primary");
	Categories.Add("Tactical Devices");
	Categories.Add("Tablet Devices");
	Categories.Add("Secondary");
	Categories.Add("Grenades");
	
	bCanMoveWhileMenuIsOpened = true;
	bCanAimWhileMenuIsOpened = false;
}

void UWeaponWheelWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UpdateSectorColor(CurrentSelectionIndex, GetCorrectSelectionColor());
}

void UWeaponWheelWidget::OnRadialMenuOpened_Implementation()
{
	Super::OnRadialMenuOpened_Implementation();

	RemoveNullItemsFromAllCategories();
}

bool UWeaponWheelWidget::RemoveNullItemsFromCategory_Implementation(const FName& WeaponWheelCategoryName)
{
	return false;
}

bool UWeaponWheelWidget::RemoveNullItemsFromAllCategories_Implementation()
{
	for (const FName& Category : Categories)
	{
		RemoveNullItemsFromCategory(Category);
	}
	
	Select(CurrentSelectionIndex);

	return true;
}
