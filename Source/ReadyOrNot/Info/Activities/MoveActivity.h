// Copyright Void Interactive, 2021

#pragma once

#include "Team/TeamBaseActivity.h"
#include "MoveActivity.generated.h"

UCLASS()
class READYORNOT_API UMoveActivity final : public UTeamBaseActivity
{
	GENERATED_BODY()

public:
	UMoveActivity();
	
	virtual void ResetData() override;

protected:
	virtual void StartActivity(AAIController* Owner) override;
	virtual void PerformActivity(float DeltaTime) override;
	virtual void ResumeActivity() override;
	virtual bool CanFinishActivity() const override;
	
	bool bReachedInitialLocation = false;

	FVector OriginalMoveToLocation = FVector::ZeroVector;
};
