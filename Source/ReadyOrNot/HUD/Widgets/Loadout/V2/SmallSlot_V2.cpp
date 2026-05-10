// Copyright Void Interactive, 2023


#include "HUD/Widgets/Loadout/V2/SmallSlot_V2.h"

#include "Components/TextBlock.h"

void USmallSlot_V2::SetEquipped(bool IsEquipped)
{
	Equipped = IsEquipped;
	OnEquipped();
}

bool USmallSlot_V2::GetEquipped()
{
	return Equipped;
}

void USmallSlot_V2::SetText(FText Text)
{
	if (ItemName)
	{
		ItemName->SetText(Text);
	}
}
