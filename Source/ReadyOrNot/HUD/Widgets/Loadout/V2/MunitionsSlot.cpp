// Copyright Void Interactive, 2023


#include "HUD/Widgets/Loadout/V2/MunitionsSlot.h"
#include "HUD/Widgets/Loadout/V2/MunitionsSlotElement.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "lib/ReadyOrNotLoadoutManager.h"

void UMunitionsSlot::NativeConstruct()
{
	Super::NativeConstruct();

	gs = GetWorld()->GetGameState<AReadyOrNotGameState>();
	LoadoutFunctionLibrary = gs->LoadoutFunctionLibrary;
}

void UMunitionsSlot::UpdateSlotCount()
{
	//Update text to display current amount of slots in use
	//FString SlotCount = FString::FromInt(LoadoutFunctionLibrary->GetCurrentSlotCount());
	//SlotsSubtext->SetText(FText::FromString(SlotCount + " SLOTS"));
	int CurrentAmmo = LoadoutFunctionLibrary->GetCurrentSlotCount();
	int MaxAmmo = LoadoutFunctionLibrary->GetMaximumSlotCount();

	UpdateSlotText(CurrentAmmo, MaxAmmo);
}

void UMunitionsSlot::UpdateElementContainer()
{
	//Update munitions icons
	UpdateSlotCount();
	ElementContainer->ClearChildren();
	SetPrimaryAmmoSlot();
	SetSecondaryAmmoSlot();
	SetGrenadeSlot();
	SetTacticalSlot();
}

void UMunitionsSlot::SetPrimaryAmmoSlot()
{
	//Get amount of munition currently in use based on type
	//Create elements of each type with either the amount of icons (e.g. ammo) or a number (e.g. everything else) signifying the amount
	bHasElementsPrevious = false;
	TArray<FName> AmmoTypes = LoadoutFunctionLibrary->GetPrimaryAmmoTypes();

	for (int i = 0; i < AmmoTypes.Num(); i++)
	{
		int AmmoCount = LoadoutFunctionLibrary->GetPrimarySlotCount(AmmoTypes[i]);
		if (AmmoCount > 0)
		{
			bHasElementsPrevious = true;
			CreateElement(PrimaryAmmoIcon, AmmoCount, true, FText::FromName(AmmoTypes[i]));
			//UMunitionsSlotElement* PrimaryAmmo = CreateWidget<UMunitionsSlotElement>(GetOwningPlayer(), UMunitionsSlotElement::StaticClass()); //bindwidget variables are not set when using createwidget
			/*PrimaryAmmo->ElementImage->SetBrushFromTexture(PrimaryAmmoIcon); 
			PrimaryAmmo->ElementText->SetText(FText::FromString("AP x" + AmmoCount));
			ElementContainer->AddChild(PrimaryAmmo);*/ 
		}
	}
}

void UMunitionsSlot::SetSecondaryAmmoSlot()
{
	TArray<FName> AmmoTypes = LoadoutFunctionLibrary->GetSecondaryAmmoTypes();
	int			  FoundAmmo = 0;
	for (int i = 0; i < AmmoTypes.Num(); i++)
	{
		int	AmmoCount = LoadoutFunctionLibrary->GetSecondarySlotCount(AmmoTypes[i]);
		if (AmmoCount > 0)
		{
			if (FoundAmmo == 0)
			{
				CreateSeparator(bHasElementsPrevious);
			}
			FoundAmmo++;
			bHasElementsNext = true;
			CreateElement(SecondaryAmmoIcon, AmmoCount, true, FText::FromName(AmmoTypes[i]));
		}
	}
	if (FoundAmmo == 0)
	{
		bHasElementsNext = false;
	}
}

void UMunitionsSlot::SetGrenadeSlot()
{
	//temp until we decide if we want separate icons for every grenade
	int GrenadeCount = LoadoutFunctionLibrary->GetGrenadeSlotCount(SlotItems[0]) + LoadoutFunctionLibrary->GetGrenadeSlotCount(SlotItems[1]) + LoadoutFunctionLibrary->GetGrenadeSlotCount(SlotItems[2]);
	if (GrenadeCount > 0)
	{
		CreateSeparator(bHasElementsPrevious || bHasElementsNext ? true : false);
		bHasElementsPrevious = bHasElementsNext;
		bHasElementsNext = true;
		CreateElement(GrenadeIcon, GrenadeCount);
	}
	else
	{
		bHasElementsNext = false;
	}
}

void UMunitionsSlot::SetTacticalSlot()
{
	int TacticalCount = LoadoutFunctionLibrary->GetTacticalSlotCount(SlotItems[3]);
	if (TacticalCount > 0)
	{
		CreateSeparator(bHasElementsPrevious || bHasElementsNext ? true : false);
		bHasElementsPrevious = false;
		bHasElementsNext = false;
		CreateElement(C2ChargeIcon, TacticalCount);
	}

	TacticalCount = LoadoutFunctionLibrary->GetTacticalSlotCount(SlotItems[4]);
	if (TacticalCount > 0)
	{
		CreateSeparator(bHasElementsPrevious || bHasElementsNext ? true : false);
		bHasElementsPrevious = false;
		bHasElementsNext = false;
		CreateElement(WedgeIcon, TacticalCount);
	}

	TacticalCount = LoadoutFunctionLibrary->GetTacticalSlotCount(SlotItems[5]);
	if (TacticalCount > 0)
	{
		CreateSeparator(bHasElementsPrevious || bHasElementsNext ? true : false);
		bHasElementsPrevious = false;
		bHasElementsNext = false;
		CreateElement(PeppersprayIcon, TacticalCount);
	}

	TacticalCount = LoadoutFunctionLibrary->GetTacticalSlotCount(SlotItems[6]);
	if (TacticalCount > 0)
	{
		CreateSeparator(bHasElementsPrevious || bHasElementsNext ? true : false);
		bHasElementsPrevious = false;
		bHasElementsNext = false;
		CreateElement(LockpickGunIcon, TacticalCount);
	}

	TacticalCount = LoadoutFunctionLibrary->GetTacticalSlotCount(SlotItems[7]);
	if (TacticalCount > 0)
	{
		CreateSeparator(bHasElementsPrevious || bHasElementsNext ? true : false);
		bHasElementsPrevious = false;
		bHasElementsNext = false;
		CreateElement(TaserIcon, TacticalCount);
	}
}
