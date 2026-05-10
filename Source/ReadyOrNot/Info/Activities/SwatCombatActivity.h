// Void Interactive, 2020

#pragma once

#include "Info/Activities/BaseCombatActivity.h"
#include "SwatCombatActivity.generated.h"

UCLASS()
class READYORNOT_API USwatCombatActivity final : public UBaseCombatActivity
{
	GENERATED_BODY()

public:
	USwatCombatActivity();

	virtual void StartActivity(AAIController* Owner) override;
	virtual void PerformActivity(float DeltaTime) override;
	virtual bool CanShoot() const override;
	virtual bool ShouldStrafe() const override;

	virtual bool EngageEnemy(AReadyOrNotCharacter* EnemyCharacter, float DeltaTime) override;

	virtual void EnterStrafeState() override;

	virtual bool GetStrafeDebugString(FString& OutString) const override;
	virtual void GatherDebugString(FString& OutString) override;

	virtual bool ShouldTrackTarget() const override;

protected:
	virtual bool TryMoveIntoCover(AReadyOrNotCharacter* InstigatorCharacter, float MinDistanceFromInstigator = 0.0f, float ExclusionRadiusAroundInstigator = 0.0f, bool bRequireLOS = true) override;
	virtual bool CanEngageEnemy(AReadyOrNotCharacter* Enemy) const override;

	void TickWeaponSwitch();
	
	float YellDelay = 0.0f;

	float TimeUntilNextLocalPlayerCombatMove = 0.0f;

	bool bObstacleInFront = false;
};
