// Void Interactive, 2020

#pragma once

#include "Info/Activities/Team/TeamBaseActivity.h"
#include "TeamCoverAreaActivity.generated.h"

UCLASS()
class READYORNOT_API UTeamCoverAreaActivity final : public UTeamBaseActivity
{
	GENERATED_BODY()

public:
	UTeamCoverAreaActivity();
	
	virtual bool ShouldForceNoStrafe() const override;
	virtual bool ShouldForceStrafe() const override;

protected:
	virtual void PerformActivity(float DeltaTime) override;
	virtual void FinishedActivity(bool bSuccess) override;

	virtual bool OverrideFocalPoint(FVector& FocalPoint) override;
};
