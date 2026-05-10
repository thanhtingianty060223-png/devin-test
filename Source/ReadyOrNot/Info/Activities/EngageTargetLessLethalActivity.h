// Copyright Void Interactive, 2023

#pragma once

#include "BaseActivity.h"
#include "EngageTargetLessLethalActivity.generated.h"

UCLASS()
class READYORNOT_API UEngageTargetLessLethalActivity final : public UBaseActivity
{
	GENERATED_BODY()

public:
	UEngageTargetLessLethalActivity();

	UPROPERTY()
	AReadyOrNotCharacter* TargetCharacter = nullptr;

	EItemCategory ItemType = EItemCategory::IC_None;

protected:
	virtual void StartActivity(AAIController* Owner) override;
	virtual void PerformActivity(float DeltaTime) override;

	virtual void FinishedActivity(bool bSuccess) override;

	virtual bool CanFinishActivity() const override;

	virtual bool OverrideFocalPoint(FVector& FocalPoint) override;

	virtual float GetDestinationTolerance() const override;

	virtual void ResetData() override;

	float TimeEquipped = 0.0f;
	float TimeUsing = 0.0f;
	
	bool bHasEverFired = false;
};
