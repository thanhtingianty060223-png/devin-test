// Copyright Void Interactive, 2023

#pragma once

#include "Info/Activities/BaseActivity.h"
#include "TargetNextCivilianActivity.generated.h"

UCLASS()
class READYORNOT_API UTargetNextCivilianActivity final : public UBaseActivity
{
	GENERATED_BODY()

public:
	UTargetNextCivilianActivity();
	
protected:
	virtual void StartActivity(AAIController* Owner) override;
	virtual void PerformActivity(float DeltaTime) override;
	virtual void FinishedActivity(bool bSuccess) override;
	virtual bool CanShoot() const override;

	virtual bool OverrideFocalPoint(FVector& FocalPoint) override;

	virtual void ResetData() override;

	virtual bool CanFinishActivity() const override;

	void FindNextCivilian();
	
	ACyberneticCharacter* FindNextClosestAliveCivilian() const;

	UPROPERTY()
	ACyberneticCharacter* TargetingCivilian = nullptr;

	float TimeUntilNextVO = 0.0f;
	float TimeUntilKill = 0.0f;
	float TimeUntilKillOriginal = 0.0f;
};
