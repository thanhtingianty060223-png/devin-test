// Void Interactive, 2020

#pragma once

#include "Info/Activities/CombatMove/BaseCombatMoveActivity.h"
#include "CivilianFleeCombatMove.generated.h"

UCLASS()
class READYORNOT_API UCivilianFleeCombatMove : public UBaseCombatMoveActivity
{
	GENERATED_BODY()

public:
	virtual FName GetMoveStyleOverride_Implementation() const override;

protected:
	virtual void OnPathFound(uint32 PathId, ENavigationQueryResult::Type ResultType, FNavPathSharedPtr NavPath) override;
	virtual void RequestCombatMove(float DeltaTime) override;
	
	bool TryFindNearestHidingSpot();
	
	bool bTryFindHidingSpot = false;
	bool bFoundHidingSpot = false;
};
