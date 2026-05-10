// Copyright Void Interactive, 2023

#pragma once

#include "Info/Activities/BaseActivity.h"
#include "MoveToExitActivity.generated.h"

UCLASS()
class READYORNOT_API UMoveToExitActivity final : public UBaseActivity
{
	GENERATED_BODY()

protected:
	virtual void FinishedActivity(bool bSuccess) override;
	virtual void PerformActivity(float DeltaTime) override;
	virtual bool CanBeCleared() override;
	virtual bool CanBeOverridenBy(UBaseActivity* InOverridingActivity) override;
	virtual bool CanOverrideActivity() const override;

	virtual TSubclassOf<UNavigationQueryFilter> GetNavigationQueryOverride() override;

	virtual FName GetMoveStyleOverride_Implementation() const override;
};
