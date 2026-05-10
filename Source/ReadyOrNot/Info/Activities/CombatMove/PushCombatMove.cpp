// Copyright Void Interactive, 2022

#include "Info/Activities/CombatMove/PushCombatMove.h"

#include "ReadyOrNotAIConfig.h"
#include "Characters/CyberneticController.h"

#include "Info/Activities/BaseCombatActivity.h"

UPushCombatMove::UPushCombatMove()
{
	bAllowPartialMove = true;
	bRequireMagazineWeapon = true;
}

void UPushCombatMove::StartActivity(AAIController* Owner)
{
	bGlobalCooldownRandomRange = true;
	GlobalCooldownRange = AI_CONFIG_GET_VECTOR2D("SuspectGlobalPushCooldown", FVector2D(4.0f, 8.0f));
	
	Super::StartActivity(Owner);

	GetCharacter()->ReasonsToSprint.AddUnique("pushing");
}

void UPushCombatMove::FinishedActivity(bool bSuccess)
{
	Super::FinishedActivity(bSuccess);
	
	GetCharacter()->ReasonsToSprint.Remove("pushing");
}

void UPushCombatMove::RequestCombatMove(const float DeltaTime)
{
	Super::RequestCombatMove(DeltaTime);
	
	FVector LastKnownEnemyPosition = FVector::ZeroVector;

	if (OwningController->GetTrackedTarget())
	{
		if (OwningController->GetTrackedTarget()->IsActive())
			LastKnownEnemyPosition = OwningController->GetFocalPointOnActor(OwningController->GetTrackedTarget());
	}
	else
	{
		if (OwningController->GetLastTrackedEnemy())
		{
			if (OwningController->GetLastTrackedEnemy()->IsActive())
				LastKnownEnemyPosition = OwningController->GetFocalPointOnActor(OwningController->GetLastTrackedEnemy());
		}
	}
	
	if (LastKnownEnemyPosition == FVector::ZeroVector)
	{
		#if !UE_BUILD_SHIPPING
		UnableToCombatReason = "No last known enemy position";
		#endif
		
		FinishCombatMove(false);
		return;
	}

	Location = LastKnownEnemyPosition;
	
	if (HasReachedLocation() ||
		OwningController->GetTargetingComp()->HasLineOfSightToTrackedTarget() ||
		OwningController->GetTargetingComp()->HasLineOfSightToLastTrackedTarget())
	{
		FinishCombatMove(true);
		return;
	}

	if (!OwningController->IsMoving())
	{
		RequestMoveAsync();
	}
}

bool UPushCombatMove::OverrideFocalPoint(FVector& FocalPoint)
{
	FVector LastKnownEnemyPosition = OwningController->GetTargetingComp()->GetLastKnownEnemyPosition();

	if (OwningController->GetTrackedTarget())
	{
		LastKnownEnemyPosition = OwningController->GetFocalPointOnActor(OwningController->GetTrackedTarget());
	}
	else
	{
		if (OwningController->GetLastTrackedEnemy())
			LastKnownEnemyPosition = OwningController->GetFocalPointOnActor(OwningController->GetLastTrackedEnemy());
	}
	
	if (LastKnownEnemyPosition != FVector::ZeroVector)
	{
		FocalPoint = FVector(LastKnownEnemyPosition.X, LastKnownEnemyPosition.Y, GetCharacter()->GetMesh()->GetBoneLocation("head").Z);
		return true;
	}

	return false;
}
