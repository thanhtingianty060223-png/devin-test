// Copyright Void Interactive, 2023


#include "HUD/Widgets/Loadout/V2/Loadout_Carousel_V3.h"

#include "Components/TextBlock.h"

void ULoadout_Carousel_V3::SetText(FText InputText)
{
	if (Text)
	{
		Text->SetText(InputText);
	}
}
