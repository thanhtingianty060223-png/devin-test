// Void Interactive, 2020

#pragma once

#include "Info/Activities/WorldBuildingActivity.h"
#include "InvestigateActivity.generated.h"

UCLASS()
class READYORNOT_API UInvestigateActivity : public UWorldBuildingActivity
{
	GENERATED_BODY()

	UInvestigateActivity();

	virtual void PerformActivity(float DeltaTime) override;
	virtual bool ShouldForceStrafe() const override;
	virtual bool ShouldForceNoStrafe() const override;
	virtual void FinishedActivity(bool bSuccess) override;
};
