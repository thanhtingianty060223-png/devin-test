// Copyright Void Interactive, 2021

#pragma once

#include "CoreMinimal.h"
#include "Info/Activities/BaseActivity.h"
#include "DisarmC2Activity.generated.h"

/**
 *
 */
UCLASS()
class READYORNOT_API UDisarmC2Activity : public UBaseActivity
{
	GENERATED_BODY()

		UPROPERTY()
		class APlacedC2Explosive* PlacedC2 = nullptr;

public:
	void StartActivity(AAIController* Owner) override;
	void PerformActivity(float DeltaTime) override;
	virtual bool CanFinishActivity() const override;
	virtual void FinishedActivity(bool bSuccess) override;

	float TimeSpentDisarming = 0.0f;
	
	void SetC2(APlacedC2Explosive* C2) { PlacedC2 = C2; }
	APlacedC2Explosive* GetC2() { return PlacedC2; }
};