// Void Interactive, 2020

#include "ChargeCombatMove.h"
#include "DuelingCombatMove.h"

#include "Actors/Environment/FlankingAvoidanceVolume.h"

#include "Characters/CyberneticCharacter.h"
#include "Characters/CyberneticController.h"

#include "Info/Activities/BaseCombatActivity.h"

#include "Navigation/ReadyOrNotNavQueries.h"

UChargeCombatMove::UChargeCombatMove()
{
	bAllowRePathOnInvalidation = true;
}

void UChargeCombatMove::StartActivity(AAIController* Owner)
{
	Super::StartActivity(Owner);
	
	GetCharacter()->ReasonsToSprint.AddUnique("charging");
}

void UChargeCombatMove::FinishedActivity(const bool bSuccess)
{
	Super::FinishedActivity(bSuccess);

	GetCharacter()->ReasonsToSprint.Remove("charging");
}

bool UChargeCombatMove::OverrideFocalPoint(FVector& FocalPoint)
{
	if (const AReadyOrNotCharacter* TrackedTarget = OwningController->GetTrackedTarget())
	{
		FocalPoint = OwningController->GetFocalPointOnActor(TrackedTarget);
		return true;
	}

	FocalPoint = FVector::ZeroVector;
	return false;
}

void UChargeCombatMove::RequestCombatMove(const float DeltaTime)
{
	Super::RequestCombatMove(DeltaTime);

	AReadyOrNotCharacter* TrackedTarget = OwningController->GetTrackedTarget();
	if (!TrackedTarget)
	{
		TrackedTarget = OwningController->GetLastTrackedEnemy();
		if (!TrackedTarget)
		{
			#if !UE_BUILD_SHIPPING
			UnableToCombatReason = "Not tracking a target";
			#endif
			
			FinishCombatMove(false);
			return;
		}
	}

	if (Location != FVector::ZeroVector && HasReachedLocation(100.0f))
	{
		Location = FVector::ZeroVector;
		GetCharacter()->MeleeVictim(TrackedTarget);
		return;
	}

	Location = TrackedTarget->GetNavAgentLocation();
	
	PlayAISpeech(VO_SUSPECTS_AND_CIVILIAN::CHARGING, true);
}

float UChargeCombatMove::GetDestinationTolerance() const
{
	return 50.0f;
}
