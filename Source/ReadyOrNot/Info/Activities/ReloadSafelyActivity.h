// Copyright Void Interactive, 2021

#pragma once

#include "Info/Activities/BaseActivity.h"
#include "ReloadSafelyActivity.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UReloadSafelyActivity : public UBaseActivity
{
	GENERATED_BODY()

protected:
	UReloadSafelyActivity();

	virtual void StartActivity(AAIController* Owner) override;
	virtual void FinishedActivity(bool bSuccess) override;
	virtual void PerformActivity(float DeltaTime) override;
	virtual bool OverrideFocalPoint(FVector& FocalPoint) override;

	virtual bool CanFinishActivity() const override;
	virtual bool CanShoot() const override;

private:
	UFUNCTION()
	void ReloadFinished();

	UPROPERTY()
	UAnimMontage* ReloadMontage = nullptr;
	
	FTimerHandle ReloadFinished_Handle;
};
