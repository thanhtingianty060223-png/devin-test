// Copyright Void Interactive, 2021

#include "BringOrderToChaos.h"

#include "Info/ScoringManager.h"

ABringOrderToChaos::ABringOrderToChaos()
{
	ObjectiveName = NSLOCTEXT("BringOrderToChaos", "Bring Order to Chaos", "Bring Order to Chaos");
	ObjectiveDescription = NSLOCTEXT("BringOrderToChaos", "Arrest or neutralize any contact at the scene.", "Arrest or neutralize any contact at the scene.");
}

void ABringOrderToChaos::TickObjective()
{
	if (AScoringManager::Get()->AllSuspectsKilledOrArrested())
	{
		ObjectiveCompleted();
	}
}
