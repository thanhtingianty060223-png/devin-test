// Copyright Void Interactive, 2023


#include "HUD/Widgets/Loadout/V2/Loadout_ArmorSelection_V2.h"

#include "lib/ReadyOrNotLoadoutManager.h"

bool ULoadout_ArmorSelection_V2::NativeOnHandleBackAction()
{
	const AReadyOrNotGameState* GameState = GetWorld()->GetGameState<AReadyOrNotGameState>();
	GameState->LoadoutFunctionLibrary->SanitizeActiveLoadout();
	return Super::NativeOnHandleBackAction();
}
