// Void Interactive, 2020

#pragma once

#include "Info/Activities/CombatMove/BaseCombatMoveActivity.h"
#include "SoftCoverCombatMove.generated.h"

UCLASS()
class READYORNOT_API USoftCoverCombatMove : public UBaseCombatMoveActivity
{
	GENERATED_BODY()

public:
	FORCEINLINE bool WantsToBeVisible() const { return bWantsToBeVisible; }
	FORCEINLINE bool CanSeeEnemyLastKnownPosition() const { return bCanSeeLastKnownPosition; }
	
protected:
	virtual void RequestCombatMove(float DeltaTime) override;
	
	float TimeUntilToggleVis = 3.0f;
	
	bool bWantsToBeVisible = false;
	bool bCanSeeLastKnownPosition = false;
};
