// Void Interactive, 2020

#pragma once

#include "Info/Activities/BaseCombatActivity.h"
#include "CivilianCombatActivity.generated.h"

UCLASS()
class READYORNOT_API UCivilianCombatActivity : public UBaseCombatActivity
{
	GENERATED_BODY()

public:
	virtual void StartActivity(AAIController* Owner) override;
	virtual void PerformActivity(float DeltaTime) override;
	
protected:
	virtual bool TryFlee() override;

	UPROPERTY()
	class UCivilianFleeCombatMove* CivilianFleeCombatMove = nullptr;
};
