// Copyright Void Interactive, 2023

#pragma once

#include "BaseCombatMoveActivity.h"
#include "RepositionCombatMove.generated.h"

UCLASS()
class READYORNOT_API URepositionCombatMove final : public UBaseCombatMoveActivity
{
	GENERATED_BODY()

public:
	URepositionCombatMove();
	
	virtual void StartActivity(AAIController* Owner) override;
	virtual void FinishedActivity(bool bSuccess) override;
	virtual void FinishedActivity_NoOwner(bool bSuccess) override;
	
	virtual void RequestCombatMove(const float DeltaTime) override;

	virtual TSubclassOf<UNavigationQueryFilter> GetNavigationQueryOverride() override;

protected:
	bool TryRepositionFromDoor(ADoor* Door, FName CurrentRoom, bool bMustBeInThreshold = false);

	virtual void ResetData() override;

	virtual void OnPathFound(uint32 PathId, ENavigationQueryResult::Type ResultType, FNavPathSharedPtr NavPath) override;

	void DeactivateDoorBlocker();

	UPROPERTY()
	ADoor* TheDoor = nullptr;
};
