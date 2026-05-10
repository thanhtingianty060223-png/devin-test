// Copyright Void Interactive, 2021

#include "ScriptedLevelEvents.h"
#include "Characters/CyberneticController.h"
#include "Characters/CyberneticCharacter.h"
#include "NavigationSystem.h"
#include "Info/WorldBuildingPlacementActor.h"

ACyberneticCharacter* UScriptedLevelEvents::GetCyberneticsCharacterByTag(FName Tag)
{
	for (TActorIterator<ACyberneticCharacter> It(UBpGameplayHelperLib::GetWorldStatic()); It; ++It)
	{
		ACyberneticCharacter* pc = *It;
		if (pc->Tags.Contains(Tag))
			return pc;
	}
	return nullptr;
}

ACyberneticController* UScriptedLevelEvents::GetCyberneticsControllerByTag(FName Tag)
{
	ACyberneticCharacter* cc = GetCyberneticsCharacterByTag(Tag);
	if (cc)
	{
		if (Cast<ACyberneticController>(cc->GetController()))
		{
			return Cast<ACyberneticController>(cc->GetController());
		}
	}
	for (TActorIterator<ACyberneticController>It(UBpGameplayHelperLib::GetWorldStatic()); It; ++It)
	{
		ACyberneticController* cb = *It;
		if (cb->Tags.Contains(Tag))
			return cb;
	}
	return nullptr;
}

void UScriptedLevelEvents::GiveWorldBuildingActivityByTag(class ACyberneticController* Controller, FName Tag, float TimeDoingActivity)
{
	if (Controller)
	{
		for (TActorIterator<AWorldBuildingPlacementActor> It(Controller->GetWorld()); It; ++It)
		{
			AWorldBuildingPlacementActor* wb = *It;
			if (wb->Tags.Contains(Tag))
			{
				wb->GiveActivityForControllerWithoutRoute(Controller, TimeDoingActivity == 0.0f ? 9999999 : 0.0f);
				break;
			}

		}
	}
}
