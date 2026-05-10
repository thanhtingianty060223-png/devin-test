// Copyright Void Interactive, 2021

#include "MoveActivity.h"

#include "Characters/CyberneticController.h"
#include "Characters/CyberneticCharacter.h"

UMoveActivity::UMoveActivity()
{
	ActivityName = FText::FromStringTable("SwatCommandTable", "MoveTo");
	
	bFinishActivityWhenOverriden = true;
	bAbortActivityIfCannotReachLocation = true;
	bAbortIfTrackingEnemy = true;
}

bool UMoveActivity::CanFinishActivity() const
{
	if (Location == OriginalMoveToLocation)
		return HasReachedLocation(GetDestinationTolerance());
	
	return false;
}

void UMoveActivity::StartActivity(AAIController* Owner)
{
	Super::StartActivity(Owner);
	
	OriginalMoveToLocation = Location;

	GetCharacter()->ReasonsToStandStill.Empty();
}

void UMoveActivity::PerformActivity(const float DeltaTime)
{
	Super::PerformActivity(DeltaTime);

	constexpr float Threshold = 500.0f;
	
	const bool bNearMoveToLocation = FVector::Distance(OriginalMoveToLocation, GetCharacter()->GetNavAgentLocation()) < Threshold;
	
	const uint8 Index = (uint8)OverrideSquadPosition;
	if (const ACyberneticCharacter* Leader = GetCharacterAtSquadPosition((ESquadPosition)(Index-1)))
	{
		if (bNearMoveToLocation)
		{
			SetLocation(OriginalMoveToLocation);
		}
		else
		{
			SetLocation(Leader->GetNavAgentLocation(), false);
		}
	}
}

void UMoveActivity::ResumeActivity()
{
	Super::ResumeActivity();

	GetCharacter()->ReasonsToStandStill.Empty();

	Location = OriginalMoveToLocation;
	RequestMoveAsync();
}

void UMoveActivity::ResetData()
{
	Super::ResetData();

	bReachedInitialLocation = false;
	
	OriginalMoveToLocation = FVector::ZeroVector;
}
