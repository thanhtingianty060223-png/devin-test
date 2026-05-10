// Void Interactive, 2020

#pragma once

#include "Info/Activities/CombatMove/BaseCombatMoveActivity.h"
#include "SuppressionCombatMove.generated.h"

UCLASS()
class READYORNOT_API USuppressionCombatMove : public UBaseCombatMoveActivity
{
	GENERATED_BODY()

public:
	USuppressionCombatMove();
	
	virtual void StartActivity(AAIController* Owner) override;
	virtual bool OverrideFocalPoint(FVector& FocalPoint) override;

	virtual void ResetData() override;

	#if !UE_BUILD_SHIPPING
	virtual void PerformActivity_Debug(float DeltaTime) override;
	virtual void GatherDebugString(FString& OutString) override;
	#endif
	
protected:
	virtual void RequestCombatMove(float DeltaTime) override;

	FVector GenLocationInLOS;

	float TimeUntilNextSuppressionMove = 0.0f;
	float TimeUntilNextSuppressionFire = 0.0f;

	uint8 bHasBrokenLOS : 1;
};
