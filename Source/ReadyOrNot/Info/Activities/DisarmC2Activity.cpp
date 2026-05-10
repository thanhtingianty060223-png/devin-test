// Copyright Void Interactive, 2021

#include "DisarmC2Activity.h"
#include "ReadyOrNot.h"
#include "Actors/Gameplay/PlacedC2Explosive.h"
#include "Characters/CyberneticController.h"
#include "Characters/CyberneticCharacter.h"

void UDisarmC2Activity::StartActivity(AAIController* Owner)
{
	Super::StartActivity(Owner);
}

void UDisarmC2Activity::PerformActivity(float DeltaTime)
{
	Super::PerformActivity(DeltaTime);
	if (OwningController)
	{
		TimeSpentDisarming += DeltaTime;
	}
}

bool UDisarmC2Activity::CanFinishActivity() const
{
	return TimeSpentDisarming > 6.0f;
}

void UDisarmC2Activity::FinishedActivity(bool bSuccess)
{
	if (bSuccess)
	{
		if (PlacedC2)
		{
			// destroy the actual C2
			PlacedC2->RemoveFromTarget();
		}
	}
}