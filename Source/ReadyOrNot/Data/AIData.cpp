// Copyright Void Interactive, 2022

#include "AIData.h"
#include "ReadyOrNot.h"

FSavedLoadout UAIData::GetLoadout(const FAIDataLookupTable* LookupRow)
{
	FSavedLoadout Loadout = LookupRow->DefaultLoadout;
	
	if (LookupRow->bUseRandomLoadout)
	{
		const TArray<TSubclassOf<ABaseItem>>& Loadouts = LookupRow->AIWeaponSelection;
		if (Loadouts.Num() > 0)
		{
			Loadout.Primary = Loadouts[FMath::RandRange(0, Loadouts.Num() - 1)];
		}
	}

	return Loadout;
}

