// Copyright Void Interactive, 2023

#include "ItemSlotTactical_V2.h" 
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "lib/ReadyOrNotLoadoutManager.h"

FText UItemSlotTactical_V2::GetItemName()
{
	const AReadyOrNotGameState* GameState = Cast<AReadyOrNotGameState>(GetWorld()->GetGameState());
	if (!GameState)
		return FText();

	UReadyOrNotLoadoutManager* LoadoutFunctionLibrary = GameState->LoadoutFunctionLibrary;
	if (!LoadoutFunctionLibrary)
		return FText();
	
	if (SlotType == PrimaryAmmunition || SlotType == SecondaryAmmunition)
	{
		return LoadoutFunctionLibrary->GetAmmunitionDisplayName(SlotAmmunitionName);
	}
	if (SlotItem)
	{
		return SlotName;
	}

	return FText();
}

FText UItemSlotTactical_V2::GetItemDescription()
{
	const AReadyOrNotGameState* GameState = Cast<AReadyOrNotGameState>(GetWorld()->GetGameState());
	if (!GameState)
		return FText();

	UReadyOrNotLoadoutManager* LoadoutFunctionLibrary = GameState->LoadoutFunctionLibrary;
	if (!LoadoutFunctionLibrary)
		return FText();
	
	if (SlotType == PrimaryAmmunition || SlotType == SecondaryAmmunition)
	{
		return LoadoutFunctionLibrary->GetAmmunitionDescription(SlotAmmunitionName);
	}
	if (SlotItem)
	{
		return SlotItem->GetDefaultObject<ABaseItem>()->ItemDescription;
	}

	return FText();
}

void UItemSlotTactical_V2::NativePreConstruct()
{
	Super::NativePreConstruct();
	ItemName->SetText(SlotName.ToUpper());
}

void UItemSlotTactical_V2::Refresh()
{
	const AReadyOrNotGameState* GameState = Cast<AReadyOrNotGameState>(GetWorld()->GetGameState());
	int SlotCount = 0;
	if (GameState)
	{
		if (SlotType == TacticalSlot || SlotType == GrenadeSlot)
		{
			SlotCount = GameState->LoadoutFunctionLibrary->GetSlotCount(SlotItem);
			ItemCount->SetText(FText::Format(FText::FromString("{0}"), SlotCount));
		}
		else if (SlotType == PrimaryAmmunition)
		{
			SlotCount = GameState->LoadoutFunctionLibrary->GetPrimarySlotCount(SlotAmmunitionName);
			SlotName = FText::FromName(SlotAmmunitionName);
			ItemName->SetText(
				GameState->LoadoutFunctionLibrary->GetAmmunitionDisplayName(FName(SlotAmmunitionName.ToString())));
			ItemType->SetText(
				GameState->LoadoutFunctionLibrary->GetAmmunitionCaliber(FName(SlotAmmunitionName.ToString())));
			ItemCount->SetText(FText::Format(FText::FromString("{0}"), SlotCount));
		}
		else if (SlotType == SecondaryAmmunition)
		{
			SlotCount = GameState->LoadoutFunctionLibrary->GetSecondarySlotCount(SlotAmmunitionName);
			SlotName = FText::FromName(SlotAmmunitionName);
			ItemName->SetText(
				GameState->LoadoutFunctionLibrary->GetAmmunitionDisplayName(FName(SlotAmmunitionName.ToString())));
			ItemType->SetText(
				GameState->LoadoutFunctionLibrary->GetAmmunitionCaliber(FName(SlotAmmunitionName.ToString())));
			ItemCount->SetText(FText::Format(FText::FromString("{0}"), SlotCount));
		}
	}
	if (!NavigationDelegate.IsBound())
	{
		NavigationDelegate.BindDynamic(this, &UItemSlotTactical_V2::OnNavigateLeft);
		SetNavigationRuleCustom(EUINavigation::Left, NavigationDelegate);

		NavigationDelegate.BindDynamic(this, &UItemSlotTactical_V2::OnNavigateRight);
		SetNavigationRuleCustom(EUINavigation::Right, NavigationDelegate);
	}
	if (SlotCount == 0)
	{
		LeftArrow->SetVisibility(ESlateVisibility::Hidden);
	}
	else
	{
		LeftArrow->SetVisibility(ESlateVisibility::Visible);
	}
	OnSlotsUpdated();
}

UWidget* UItemSlotTactical_V2::OnNavigateLeft(EUINavigation NavigationDirection)
{
	const AReadyOrNotGameState* GameState = Cast<AReadyOrNotGameState>(GetWorld()->GetGameState());
	if (GameState)
	{
		if (SlotType == PrimaryAmmunition && NavigationDirection == EUINavigation::Left)
		{
			GameState->LoadoutFunctionLibrary->DecrementPrimarySlotCount(SlotAmmunitionName);
		}
		else if (SlotType == SecondaryAmmunition && NavigationDirection == EUINavigation::Left)
		{
			GameState->LoadoutFunctionLibrary->DecrementSecondarySlotCount(SlotAmmunitionName);
		}
		else if (SlotType == TacticalSlot && NavigationDirection == EUINavigation::Left)
		{
			GameState->LoadoutFunctionLibrary->DecrementTacticalSlotCount(SlotItem);
		}
		else if (SlotType == GrenadeSlot && NavigationDirection == EUINavigation::Left)
		{
			GameState->LoadoutFunctionLibrary->DecrementGrenadeSlotCount(SlotItem);
		}
	}
	Refresh();
	return this;
}

UWidget* UItemSlotTactical_V2::OnNavigateRight(EUINavigation NavigationDirection)
{
	const AReadyOrNotGameState* GameState = Cast<AReadyOrNotGameState>(GetWorld()->GetGameState());
	if (GameState)
	{
		if (SlotType == PrimaryAmmunition && NavigationDirection == EUINavigation::Right)
		{
			GameState->LoadoutFunctionLibrary->IncrementPrimarySlotCount(SlotAmmunitionName);
		}
		else if (SlotType == SecondaryAmmunition && NavigationDirection == EUINavigation::Right)
		{
			GameState->LoadoutFunctionLibrary->IncrementSecondarySlotCount(SlotAmmunitionName);
		}
		else if (SlotType == TacticalSlot && NavigationDirection == EUINavigation::Right)
		{
			GameState->LoadoutFunctionLibrary->IncrementTacticalSlotCount(SlotItem);
		}
		else if (SlotType == GrenadeSlot && NavigationDirection == EUINavigation::Right)
		{
			GameState->LoadoutFunctionLibrary->IncrementGrenadeSlotCount(SlotItem);
		}
	}
	Refresh();
	return this;
}
