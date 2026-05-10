// Copyright Void Interactive, 2023


#include "HUD/Widgets/Loadout/V2/ItemSlot_V2.h"

#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"

void UItemSlot_V2::SetItem(ABaseItem* Item)
{
	BaseItem = Item;
	if (BaseItem)
	{
		ItemName->SetText(BaseItem->ItemName.ToUpper());
		ItemType->SetText(FText::FromString(ItemClassEnumToString(BaseItem->ItemClass)).ToUpper());
	}
}

void UItemSlot_V2::SetArmorMaterial(UArmourMaterial* Armor)
{
	ArmorMaterial = Armor;
	if (ArmorMaterial)
	{
		ItemName->SetText(ArmorMaterial->DisplayName.ToUpper());
		ItemType->SetText(FText::FromString("ARMOR MATERIAL"));
	}
}

void UItemSlot_V2::SetTexts(FText Name,  FText Type)
{
	PresetName = Name.ToString();
	
	ItemName->SetText(Name.ToUpper());
	if(Type.IsEmpty())
	{
		PrefixText->SetVisibility(ESlateVisibility::Hidden);		
	}
	ItemType->SetText(Type.ToUpper());
}
