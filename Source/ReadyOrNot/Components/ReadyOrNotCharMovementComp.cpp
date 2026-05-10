// Copyright Void Interactive, 2017

#include "ReadyOrNotCharMovementComp.h"

#include "Animation/MoveStyle/RoNMoveStyleComponent.h"

#include "Characters/CyberneticCharacter.h"
#include "Characters/CyberneticController.h"

UReadyOrNotCharMovementComp::UReadyOrNotCharMovementComp()
{
	bUpdateOnlyIfRendered = false;
	NavAgentProps.bCanCrouch = true;
	CrouchedHalfHeight = 75.0f;
}

void UReadyOrNotCharMovementComp::PerformMovement(const float DeltaTime)
{
	Super::PerformMovement(DeltaTime);
	
	AReadyOrNotCharacter* OwnerCharacter = Cast<AReadyOrNotCharacter>(GetOwner());
	if (!IsValid(OwnerCharacter))
	{
		return;
	}
	
	if (ACyberneticController* CyberneticController = Cast<ACyberneticController>(OwnerCharacter->GetController()))
	{
		CyberneticController->LookAt(DeltaTime);
		
		if (ACyberneticCharacter* AI = Cast<ACyberneticCharacter>(OwnerCharacter))
		{
			AI->UpdateAimOffset();
		}
	}
}

void UReadyOrNotCharMovementComp::SimulateMovement(const float DeltaTime)
{
	if (bHasReplicatedAcceleration)
	{
		// Preserve our replicated acceleration
		const FVector OriginalAcceleration = Acceleration;
		Super::SimulateMovement(DeltaTime);
		Acceleration = OriginalAcceleration;
	}
	else
	{
		Super::SimulateMovement(DeltaTime);
	}
}

void UReadyOrNotCharMovementComp::SetReplicatedAcceleration(const FVector& InAcceleration)
{
	bHasReplicatedAcceleration = true;
	Acceleration = InAcceleration;
}

void UReadyOrNotCharMovementComp::CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration)
{
	Super::CalcVelocity(DeltaTime, Friction, bFluid, BrakingDeceleration);

	if (const AReadyOrNotCharacter* OwnerCharacter = Cast<AReadyOrNotCharacter>(GetOwner()))
	{
		if (OwnerCharacter->IsMovementLocked() || OwnerCharacter->IsCarried())
		{
			Velocity = FVector::ZeroVector;
			return;
		}
	}
}
