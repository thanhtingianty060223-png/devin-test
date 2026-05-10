// Void Interactive, 2020

#pragma once

#include "Info/Activities/CombatMove/BaseCombatMoveActivity.h"
#include "ChargeCombatMove.generated.h"

UCLASS()
class READYORNOT_API UChargeCombatMove : public UBaseCombatMoveActivity
{
	GENERATED_BODY()

public:
	UChargeCombatMove();
	
	virtual void StartActivity(AAIController* Owner) override;
	virtual void FinishedActivity(bool bSuccess) override;
	virtual bool OverrideFocalPoint(FVector& FocalPoint) override;

protected:
	virtual void RequestCombatMove(float DeltaTime) override;
	virtual float GetDestinationTolerance() const override;
};
