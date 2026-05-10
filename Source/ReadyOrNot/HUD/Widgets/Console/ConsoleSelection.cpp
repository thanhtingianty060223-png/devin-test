// Copyright Void Interactive, 2022 

#include "ConsoleSelection.h"

#include "ConsoleSelectionItem.h"
#include "Algo/Transform.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"

PRAGMA_DISABLE_OPTIMIZATION

void UConsoleSelection::AddInitialInventory()
{
	const UInventoryComponent* Inventory = GetPlayerInventory();
	TArray<ABaseItem*> InventoryItems = Inventory->GetInventoryItemsOfType(ItemCategory);
	for (ABaseItem* InventoryItem : InventoryItems)
	{
		ItemAdded(InventoryItem);
	}
}

void UConsoleSelection::ItemAdded(ABaseItem* Item)
{
	UConsoleSelectionItem* Widget = TryGetWidgetForItem(Item);
	if (Item->ItemCategories.Contains(ItemCategory))
	{
		if (Widget == nullptr)
		{
			// If we don't have a widget for this item we need to create one.
			const FWidgetLookupData WidgetData = UBpGameplayHelperLib::GetWidgetDataFromLookupData("ConsoleSelectionItem");
			UConsoleSelectionItem* NewConsoleWidget = CreateWidget<UConsoleSelectionItem>(GetWorld(), WidgetData.WidgetClass); 
			NewConsoleWidget->Name = Item->ItemName.ToString();
			NewConsoleWidget->Count = 1;
			FSlateBrush Brush;
			Brush.SetResourceObject(Item->Visuals.ItemIcon);
			NewConsoleWidget->SetImage(Brush);
			Container->AddChildToHorizontalBox(NewConsoleWidget);
		}
		else
		{
			if (++Widget->Count > 0)
			{
				Widget->SetVisibility(ESlateVisibility::Visible);
			}
		}
	}
}

void UConsoleSelection::ItemRemoved(ABaseItem* Item)
{
	UConsoleSelectionItem* Widget = TryGetWidgetForItem(Item);
	if (Widget != nullptr)
	{
		if (--Widget->Count <= 0)
		{
			Widget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UConsoleSelection::InitializeButton()
{
	const FSlateBrush Brush = UReadyOrNotFunctionLibrary::GetIconFromInputKeyName(FName(ButtonName));
	Button->SetBrush(Brush);
}

UConsoleSelectionItem* UConsoleSelection::TryGetWidgetForItem(ABaseItem* Item) const
{
	const TArray<UWidget*> Children = Container->GetAllChildren();
	TArray<UConsoleSelectionItem*> ConsoleWidgets;
	Algo::Transform(Children, ConsoleWidgets, [](UWidget* Child)
	{
		return static_cast<UConsoleSelectionItem*>(Child);
	});

	UConsoleSelectionItem** WidgetPtr = ConsoleWidgets.FindByPredicate(
		[Item](const UConsoleSelectionItem* ConsoleWidget) { return ConsoleWidget->Name.Equals(Item->ItemName.ToString()); });
	if (WidgetPtr == nullptr)
	{
		return nullptr;
	}
	return *WidgetPtr;
}

bool UConsoleSelection::Initialize()
{
	const bool Initialized = Super::Initialize();
	if (Initialized)
	{
		SetMinimumDesiredSize(FVector2D(60, 60));
	}
	UInventoryComponent* Inventory = GetPlayerInventory();
	if (Inventory == nullptr)
	{
		return Initialized;
	}
	AddInitialInventory();
	InitializeButton();
	Inventory->OnItemAddedToInventory.AddDynamic(this, &UConsoleSelection::ItemAdded);
	Inventory->OnItemRemovedFromInventory.AddDynamic(this, &UConsoleSelection::ItemRemoved);
	Inventory->OnItemEquipped.AddDynamic(this, &UConsoleSelection::ItemEquipped);
	return Initialized;
}

void UConsoleSelection::NativePreConstruct()
{
	Super::NativePreConstruct();
}

void UConsoleSelection::NativeConstruct()
{
	Super::NativeConstruct();
}

void UConsoleSelection::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
}

void UConsoleSelection::ItemEquipped(ABaseItem* Item)
{
	if (Item != nullptr && Item->ItemCategories.Contains(ItemCategory))
	{
		for (UWidget* Child : Container->GetAllChildren())
		{
			UConsoleSelectionItem* ConsoleSelectionItem = static_cast<UConsoleSelectionItem*>(Child);
			ConsoleSelectionItem->SetSelected(ConsoleSelectionItem->GetItemName().Equals(Item->ItemName.ToString()));
		}
	}
}

UInventoryComponent* UConsoleSelection::GetPlayerInventory()
{
	const APlayerCharacter* PlayerCharacter = static_cast<APlayerCharacter*>(UGameplayStatics::GetPlayerCharacter(
		GetWorld(), 0));
	if (PlayerCharacter == nullptr)
	{
		return nullptr;
	}
	return PlayerCharacter->GetInventoryComponent();
}

PRAGMA_ENABLE_OPTIMIZATION
