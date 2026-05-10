// Void Interactive, 2020

#include "InvestigateActivity.h"

#include "Characters/CyberneticController.h"
#include "Characters/CyberneticCharacter.h"

UInvestigateActivity::UInvestigateActivity()
{
	bOneShotAnimationDataTable = true;
	TableMontageName = "tp_lookaround_upperbody";
	bAbortIfTrackingEnemy = false;
	bShouldHolsterWeapon = false;
	bShouldSurrenderFromActivity = false;
	MaxActivityTime = 19.0f;
}

void UInvestigateActivity::PerformActivity(float DeltaTime)
{
	if (bAbortDueToPendingSurrender || bAbortNow || GetCharacter()->IsStunned())
	{
		GetCharacter()->StopTPMontage(TableMontageAnim);
		OwningController->FinishActivity(this, true, true);
		return;
	}
	
	if (HasReachedLocation(120.0f))
	{
		if (!GetCharacter()->Is3PMontagePlaying(TableMontageAnim))
		{
			ElapsedActivityTime = 0.0f;
		} else
		{
			if (GetCharacter()->GetCurrentMontage())
			{
				MaxActivityTime = GetCharacter()->GetCurrentMontage()->GetPlayLength();
			}
			if (ElapsedActivityTime > MaxActivityTime)
			{
				OwningController->FinishActivity(this, true, true);
			}
		}
		
	} else
	{
		ElapsedActivityTime = 0.0f;
	}

	if (OwningController->GetTrackedTarget())
	{
		// skip the world building stuff!
		Super::Super::PerformActivity(DeltaTime);
	} else
	{
		Super::PerformActivity(DeltaTime);
	}
}

bool UInvestigateActivity::ShouldForceStrafe() const
{
	return false;
}

bool UInvestigateActivity::ShouldForceNoStrafe() const
{
	return false;
}

void UInvestigateActivity::FinishedActivity(bool bSuccess)
{
	Super::FinishedActivity(bSuccess);
	GetCharacter()->StopTPMontage(TableMontageAnim);
	
}
