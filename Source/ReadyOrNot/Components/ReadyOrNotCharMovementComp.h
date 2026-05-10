// Copyright Void Interactive, 2017

#pragma once

#include "GameFramework/CharacterMovementComponent.h"
#include "ReadyOrNotCharMovementComp.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UReadyOrNotCharMovementComp : public UCharacterMovementComponent
{
	GENERATED_BODY()

	UReadyOrNotCharMovementComp();

protected:
	virtual void PerformMovement(float DeltaTime) override;
	virtual void CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration) override;
	virtual void SimulateMovement(float DeltaTime) override;
	
public:
	void SetReplicatedAcceleration(const FVector& InAcceleration);

	UPROPERTY(Transient)
	bool bHasReplicatedAcceleration = false;
};
