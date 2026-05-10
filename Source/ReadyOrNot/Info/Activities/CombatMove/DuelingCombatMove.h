// Void Interactive, 2020

#pragma once

#include "Info/Activities/CombatMove/BaseCombatMoveActivity.h"
#include "DuelingCombatMove.generated.h"

UCLASS()
class READYORNOT_API UDuelingCombatMove : public UBaseCombatMoveActivity
{
	GENERATED_BODY()

public:
	UDuelingCombatMove();

	virtual void StartActivity(AAIController* Owner) override;
	virtual void FinishedActivity(bool bSuccess) override;
	virtual void RequestCombatMove(float DeltaTime) override;
	virtual float GetDestinationTolerance() const override;

	virtual void ResetData() override;

protected:
	virtual void OnPathFound(uint32 PathId, ENavigationQueryResult::Type ResultType, FNavPathSharedPtr NavPath) override;

	FVector FetchNextDuelLocation();

	bool CanFollowTargetVelocity() const;
	
	float TimeUntilNextMove = 0.0f;
	float TimeUntilNextVelocityUpdate = 0.0f;
	float VelocityInputScale = 0.0f;
	
	bool bInvertedVelocity = false;
	bool bMovingUp = false;

	FVector LastVelocityInput = FVector::ZeroVector;

	TArray<FVector> StoredLocations;
};

