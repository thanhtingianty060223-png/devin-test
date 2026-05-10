// Void Interactive, 2020

#pragma once

#include "Info/Activities/BaseActivity.h"
#include "MoveIntoLOSActivity.generated.h"

UCLASS()
class READYORNOT_API UMoveIntoLOSActivity : public UBaseActivity
{
	GENERATED_BODY()

public:
	UMoveIntoLOSActivity();
	
	virtual void PerformActivity(float DeltaTime) override;

	virtual float GetDestinationTolerance() const override;
	virtual bool CanFinishActivity() const override;

	UPROPERTY()
	AActor* LOSActor = nullptr;
};
