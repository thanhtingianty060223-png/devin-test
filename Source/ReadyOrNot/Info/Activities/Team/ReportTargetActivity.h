// Void Interactive, 2020

#pragma once

#include "TeamBaseActivity.h"
#include "ReportTargetActivity.generated.h"

UCLASS()
class READYORNOT_API UReportTargetActivity : public UBaseActivity
{
	GENERATED_BODY()

public:
	UReportTargetActivity();

	UPROPERTY()
	TScriptInterface<class IReportable> ReportTarget = nullptr;
	
protected:
	virtual void PerformActivity(float DeltaTime) override;

	virtual bool CanFinishActivity() const override;
	virtual bool OverrideFocalPoint(FVector& FocalPoint) override;
};
