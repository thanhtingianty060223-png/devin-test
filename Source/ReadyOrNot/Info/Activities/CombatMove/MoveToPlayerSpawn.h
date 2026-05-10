// Void Interactive, 2020

#pragma once

#include "Info/Activities/CombatMove/BaseCombatMoveActivity.h"
#include "MoveToPlayerSpawn.generated.h"

UCLASS()
class READYORNOT_API UMoveToPlayerSpawn : public UBaseCombatMoveActivity
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = true))
	bool bBeArrestedOnceReachedLocation = false;

	virtual void StartActivity(AAIController* Owner) override;
	virtual void FinishedActivity(bool bSuccess) override;
	
protected:
	virtual void RequestCombatMove(float DeltaTime) override;
	virtual void OnPathFound(uint32 PathId, ENavigationQueryResult::Type ResultType, FNavPathSharedPtr NavPath) override;
};
