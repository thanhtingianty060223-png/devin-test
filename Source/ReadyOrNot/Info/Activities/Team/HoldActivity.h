// Void Interactive, 2020

#pragma once

#include "TeamBaseActivity.h"
#include "HoldActivity.generated.h"

UCLASS()
class READYORNOT_API UHoldActivity final : public UTeamBaseActivity
{
	GENERATED_BODY()

	UHoldActivity();
	
protected:
	virtual void StartActivity(AAIController* Owner) override;
	virtual void PerformActivity(float DeltaTime) override;
	virtual void FinishedActivity(bool bSuccess) override;
	virtual void ActivityOverriden(UBaseActivity* OverridingActivity) override;
	virtual void ResumeActivity() override;
	virtual void SwapSquadPositionWith(ESquadPosition SquadPosition, bool bLeadInitiated) override;

	virtual bool CanFinishActivity() const override;

	FVector OriginalHoldLocation = FVector::ZeroVector;
};
