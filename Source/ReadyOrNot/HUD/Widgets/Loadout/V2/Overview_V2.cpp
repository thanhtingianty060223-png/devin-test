// Copyright Void Interactive, 2023

#include "HUD/Widgets/Loadout/V2/Overview_V2.h"

void UOverview_V2::SavePresets(FSavedLoadout loadout, bool isNPC)
{
	UBpGameplayHelperLib::SanitizeLoadout(loadout);
	UBpGameplayHelperLib::SaveLoadout(loadout, loadout.Name);
}