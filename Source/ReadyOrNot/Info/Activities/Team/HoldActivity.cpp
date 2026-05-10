// Void Interactive, 2020

#include "HoldActivity.h"

#include "Characters/CyberneticController.h"
#include "Characters/CyberneticCharacter.h"
#include "Info/SWATManager.h"

UHoldActivity::UHoldActivity()
{
	ActivityName = 	FText::FromStringTable("SwatCommandTable", "Hold");
	bIsProgressActivity = false;
}

void UHoldActivity::StartActivity(AAIController* Owner)
{
	Super::StartActivity(Owner);

	GetCharacter()->ReasonsToStandStill.AddUnique("Hold Position");

	OriginalHoldLocation = GetCharacter()->GetNavAgentLocation();
}

void UHoldActivity::PerformActivity(float DeltaTime)
{
	Super::PerformActivity(DeltaTime);

	if (!Cast<APlayerCharacter>(GetSquadLeader()))
	{
		GetCharacter()->ReasonsToStandStill.Remove("Hold Position");
		OwningController->FinishActivity(this, true, true);
		return;
	}

	OverrideSquadPosition = (ESquadPosition)USWATManager::Get(this)->FallInSwat.Find(GetCharacter<ASWATCharacter>());
	
	if (HasReachedLocation(GetDestinationTolerance()) && !bIsSwapping)
	{
		GetCharacter()->ReasonsToStandStill.AddUnique("Hold Position");
	}
}

void UHoldActivity::FinishedActivity(const bool bSuccess)
{
	Super::FinishedActivity(bSuccess);
	
	GetCharacter()->ReasonsToStandStill.Remove("Hold Position");
}

void UHoldActivity::ActivityOverriden(UBaseActivity* OverridingActivity)
{
	Super::ActivityOverriden(OverridingActivity);
	
	GetCharacter()->ReasonsToStandStill.Remove("Hold Position");

	OriginalHoldLocation = GetCharacter()->GetNavAgentLocation();
}

void UHoldActivity::ResumeActivity()
{
	Super::ResumeActivity();

	SetLocation(OriginalHoldLocation, false);
}

void UHoldActivity::SwapSquadPositionWith(ESquadPosition SquadPosition, bool bLeadInitiated)
{
	GetCharacter()->ReasonsToStandStill.Remove("Hold Position");
	
	if (ACyberneticCharacter* C = GetCharacterAtSquadPosition(SquadPosition))
	{
		C->ReasonsToStandStill.Remove("Hold Position");
	}

	Super::SwapSquadPositionWith(SquadPosition, bLeadInitiated);
}

bool UHoldActivity::CanFinishActivity() const
{
	return false;
}
