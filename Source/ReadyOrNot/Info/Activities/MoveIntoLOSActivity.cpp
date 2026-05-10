// Void Interactive, 2020

#include "Info/Activities/MoveIntoLOSActivity.h"

#include "Characters/CyberneticController.h"

UMoveIntoLOSActivity::UMoveIntoLOSActivity()
{
	bAbortIfTrackingEnemy = true;
	bAbortMoveWhenActivityFinished = true;
	bAbortMoveWhenActivityOverriden = true;
	bAbortActivityIfCannotReachLocation = true;
}

void UMoveIntoLOSActivity::PerformActivity(const float DeltaTime)
{
	Super::PerformActivity(DeltaTime);

	if (!OwningController->IsMoving())
	{
		RequestMoveAsync();
		return;
	}
	
	if (Location != FVector::ZeroVector)
	{
		if (GetCharacter()->HasLineOfSightTo(Location))
			OwningController->FinishActivity(this, true, true);
	}
	
	if (OwningController->GetTrackedTarget() ||
		!OwningController->HasBeenExposedToAggressiveNoise(15.0f, 0.0f, true))
	{
		OwningController->FinishActivity(this, true, true);
	}
}

float UMoveIntoLOSActivity::GetDestinationTolerance() const
{
	return 400.0f;
}

bool UMoveIntoLOSActivity::CanFinishActivity() const
{
	return false;
	//return HasReachedLocation(GetDestinationTolerance()) && OwningController->LineOfSightTo(LOSActor);
	//return HasReachedLocation(GetDestinationTolerance()) || GetCharacter()->HasLineOfSightTo(Location);
}
