// Copyright Void Interactive, 2022 

#include "HUD/Widgets/Console/ConsoleHotkeys.h"

bool UConsoleHotkeys::Initialize()
{
	const bool Initialized = Super::Initialize();
	ActiveLayout = DefaultLayout;

	PlayerCharacter = Cast<APlayerCharacter>(GetOwningPlayerPawn());
	if (PlayerCharacter != nullptr)
	{
		PlayerCharacter->GetInventoryComponent()->OnItemHolstered.AddDynamic(this, &UConsoleHotkeys::ItemEquipped);
		bIsNVGActive = PlayerCharacter->HasNVG();
		bHasFireModes = PlayerCharacter->EquippedWeaponHasFireModes();
		bHasLaserAttachment = PlayerCharacter->EquippedWeaponHasLaserAttachment();
		bHasChemlightInInventory = PlayerCharacter->HasChemlightsInInventory();

		if(const UInventoryComponent* InventoryComponent = PlayerCharacter->GetInventoryComponent())
		{
			const ABaseItem* DefaultItem = InventoryComponent->GetInventoryItemOfType(EItemCategory::IC_Zipcuffs);
			FSlateBrush Brush;
			Brush.SetResourceObject(DefaultItem->Visuals.ItemIcon);
			Hotkey_ItemWheel->SetIconBrush(Brush);
		}
		
	}


	return Initialized;
}

void UConsoleHotkeys::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (PlayerCharacter != nullptr)
	{
		if (PlayerCharacter->GetItemWheelActive())
		{
			SetLayout(ItemWheelLayout);
		}
		else if (PlayerCharacter->GetCommandWheelActive())
		{
			SetLayout(CommandLayout);
		}
		else
		{
			SetLayout(DefaultLayout);
		}
	}
}

void UConsoleHotkeys::ItemEquipped(ABaseItem* Item)
{
	if (!Item || Item == LastEquippedItem)
		return;
	if (Item->ContainsItemCategory(EItemCategory::IC_TacticalDevice) ||
		Item->ContainsItemCategory(EItemCategory::IC_Grenade) ||
		Item->ContainsItemCategory(EItemCategory::IC_Zipcuffs) ||
		Item->ContainsItemCategory(EItemCategory::IC_Multitool) ||
		Item->ContainsItemCategory(EItemCategory::IC_Detonator))
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Item->Visuals.ItemIcon);
		Hotkey_ItemWheel->SetIconBrush(Brush);
		InvalidateLayoutAndVolatility();
		
		LastEquippedItem = Item;
	}
}

void UConsoleHotkeys::SetLayout(EConsoleHotkeysLayout Layout)
{
	if (this->ActiveLayout == Layout 
		&& bIsNVGActive == PlayerCharacter->HasNVG() 
		&& bHasFireModes == PlayerCharacter->EquippedWeaponHasFireModes() 
		&& bHasLaserAttachment == PlayerCharacter->EquippedWeaponHasLaserAttachment()
		&& bHasChemlightInInventory == PlayerCharacter->HasChemlightsInInventory())
	{
		return;
	}
	bIsNVGActive = PlayerCharacter->HasNVG();
	bHasFireModes = PlayerCharacter->EquippedWeaponHasFireModes();
	bHasLaserAttachment = PlayerCharacter->EquippedWeaponHasLaserAttachment();
	bHasChemlightInInventory = PlayerCharacter->HasChemlightsInInventory();
	this->ActiveLayout = Layout;
	RefreshHotkeys();
}

void UConsoleHotkeys::RefreshHotkeys()
{
	Hotkey_NVG->SetVisibility(ESlateVisibility::Collapsed);
	Hotkey_Laser->SetVisibility(ESlateVisibility::Collapsed);
	Hotkey_Firemode->SetVisibility(ESlateVisibility::Collapsed);
	Hotkey_TeamView->SetVisibility(ESlateVisibility::Collapsed);
	Hotkey_Chemlight->SetVisibility(ESlateVisibility::Collapsed);
	Hotkey_ItemWheel->SetVisibility(ESlateVisibility::Collapsed);
	if (ActiveLayout == ItemWheelLayout)
	{
	}
	else if (ActiveLayout == CommandLayout)
	{
		Hotkey_TeamView->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		Hotkey_NVG->SetVisibility(PlayerCharacter->HasNVG() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		Hotkey_Chemlight->SetVisibility(PlayerCharacter->HasChemlightsInInventory() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		Hotkey_Firemode->SetVisibility(PlayerCharacter->EquippedWeaponHasFireModes() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		Hotkey_Laser->SetVisibility(PlayerCharacter->EquippedWeaponHasLaserAttachment() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		Hotkey_ItemWheel->SetVisibility(ESlateVisibility::Visible);
	}
	
	Invalidate(EInvalidateWidgetReason::LayoutAndVolatility);
}
