// Copyright Void Interactive, 2022

#pragma once

#include "Info/Activities/CombatMove/BaseCombatMoveActivity.h"
#include "PushCombatMove.generated.h"

UCLASS()
class READYORNOT_API UPushCombatMove : public UBaseCombatMoveActivity
{
	GENERATED_BODY()

public:
	UPushCombatMove();
	
	virtual void StartActivity(AAIController* Owner) override;
	virtual void FinishedActivity(bool bSuccess) override;
	
	virtual void RequestCombatMove(float DeltaTime) override;

	virtual bool OverrideFocalPoint(FVector& FocalPoint) override;
};
