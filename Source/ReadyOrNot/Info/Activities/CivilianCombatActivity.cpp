// Void Interactive, 2020

#include "CivilianCombatActivity.h"

#include "Characters/CyberneticController.h"

#include "Info/Activities/CombatMove/CivilianFleeCombatMove.h"
#include "Info/Activities/CombatMove/FleeingCombatMove.h"

void UCivilianCombatActivity::StartActivity(AAIController* Owner)
{
	Super::StartActivity(Owner);

	if (!IsValid(CivilianFleeCombatMove))
	{
		CivilianFleeCombatMove = UActivityManager::CreateActivity<UCivilianFleeCombatMove>(this);
		if (CivilianFleeCombatMove)
		{
			CivilianFleeCombatMove->InitActivity(Owner);
		}
	}
}

void UCivilianCombatActivity::PerformActivity(float DeltaTime)
{
	Super::PerformActivity(DeltaTime);
}

bool UCivilianCombatActivity::TryFlee()
{
	StartRunningCombatMove(FleeingCombatMove);

	return CombatMoveActivity == FleeingCombatMove;
}
