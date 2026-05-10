// Void Interactive, 2020

#pragma once

#include "Info/Activities/BaseCombatActivity.h"
#include "SuspectCombatActivity.generated.h"

UCLASS()
class READYORNOT_API USuspectCombatActivity : public UBaseCombatActivity
{
	GENERATED_BODY()
	
public:
	USuspectCombatActivity();
	
	virtual void StartActivity(AAIController* Owner) override;
	
	virtual bool RunEngagementLogic(float DeltaTime) override;
	
	virtual bool CanPerformAction() const override;

	virtual void OnCoverExit() override;
	
	float TimeUntilRaisingGun = 0.0f;
	float TimeUntilDraw = 0.0f;
	float TimeAiming = 0.0f;
	float ForceCrouchTime = 0.0f;
	float TimeCrouching = 0.0f;
	float TimeNotCrouching = 0.0f;
	
	uint8 bPlayingDraw : 1; 
	uint8 bFirstTimeSeeingEnemy : 1;
	uint8 bHasEverHesitated : 1;
	uint8 bHasRaisedWeapon : 1;
};
