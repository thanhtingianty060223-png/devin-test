// Void Interactive, 2020

#pragma once

#include "Objectives/Objective.h"
#include "DefuseBombThreats.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API ADefuseBombThreats : public AObjective
{
	GENERATED_BODY()

	ADefuseBombThreats();

	virtual void OnObjectiveCreated_Implementation() override;
	virtual void OnObjectiveCompleted_Implementation() override;
	
	virtual void Tick(float DeltaSeconds) override;

	virtual void TickObjective() override;

protected:
	UFUNCTION()
	void OnBombDefused(class ABombActor* DefusedBomb);
	
	void ResetTOCWarning();
	
	uint8 bHasTOCWarned : 1;
	float TOCWarningCooldown = 30.0f;
};
