// Copyright Void Interactive, 2023


#include "HUD/Widgets/Console/ItemWheelMagazineElement.h"

#include "Components/TextBlock.h"

PRAGMA_DISABLE_OPTIMIZATION

UItemWheelMagazineElement::UItemWheelMagazineElement(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer), MagazineType(nullptr)
{
	DataTable = UBpGameplayHelperLib::GetAmmoLookupDataTable();
}

bool UItemWheelMagazineElement::IsSelectable()
{
	ABaseMagazineWeapon* MagazineWeapon = PlayerCharacter->GetEquippedWeapon();
	if(!IsValid(MagazineWeapon))
	{
		return false;
	}
	return MagazineWeapon->HasAnyAmmoOfType(AmmoFName);
}

void UItemWheelMagazineElement::SetAmmoType(const FName &AmmoType)
{
	AmmoFName = AmmoType;
	auto const &ammoTypeData = DataTable->FindRow<FAmmoTypeData>(AmmoType, "Ammo Type Lookup");
	DisplayName = ammoTypeData != nullptr ? ammoTypeData->AmmoVariety.ToString() : FString("Unknown");
	this->MagazineType->SetText(FText::FromString(DisplayName));
}

FName UItemWheelMagazineElement::GetAmmoType()
{
	return FName(AmmoFName);
}

FString UItemWheelMagazineElement::GetDisplayName()
{
	return DisplayName;
}

void UItemWheelMagazineElement::NativeConstruct()
{
	UUserWidget::NativeConstruct();
    PlayerCharacter = Cast<APlayerCharacter>(GetOwningPlayerPawn());
}

PRAGMA_ENABLE_OPTIMIZATION
