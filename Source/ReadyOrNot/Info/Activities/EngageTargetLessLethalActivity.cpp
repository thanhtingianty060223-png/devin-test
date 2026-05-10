// Copyright Void Interactive, 2023

#include "EngageTargetLessLethalActivity.h"

#include "BaseCombatActivity.h"
#include "Characters/CyberneticController.h"

UEngageTargetLessLethalActivity::UEngageTargetLessLethalActivity()
{
	ActivityName = FText::FromStringTable("SwatCommandTable", "EngageTarget");

	bAbortIfTrackingEnemy = false;
	bAbortMoveWhenActivityFinished = true;
	bAbortMoveWhenActivityOverriden = true;
	
	MoveAcceptanceRadius = 150.0f;
}

void UEngageTargetLessLethalActivity::StartActivity(AAIController* Owner)
{
	Super::StartActivity(Owner);

	if (!TargetCharacter)
	{
		ACTIVITY_FAILED("No valid target specified");
		return;
	}

	MoveAcceptanceRadius = 150.0f;
	if (ItemType == EItemCategory::IC_None)
		MoveAcceptanceRadius = 75.0f;
}

void UEngageTargetLessLethalActivity::PerformActivity(const float DeltaTime)
{
	Super::PerformActivity(DeltaTime);

	if (!TargetCharacter)
	{
		ACTIVITY_FAILED("No valid target specified");
		return;
	}
	
	if (TargetCharacter->IsDeadOrUnconscious() || TargetCharacter->IsIncapacitated() || TargetCharacter->IsArrestedOrSurrendered())
	{
		OwningController->FinishActivity(this, true, true);
		return;
	}
	
	if (bHasEverFired)
	{
		TimeUsing += DeltaTime;
	}

	if (TargetCharacter->IsStunned())
	{
		if (TimeUsing > 2.0f)
		{
			OwningController->FinishActivity(this, true, true);
		}
		
		return;
	}

	const bool bHasLOS = GetCharacter()->HasLineOfSightToCharacter(TargetCharacter);

	if (bHasLOS)
	{
		const FVector DirectionToTarget = (TargetCharacter->GetNavAgentLocation() - GetCharacter()->GetActorLocation()).GetSafeNormal();
		float Distance = 150.0f;
		if (ItemType == EItemCategory::IC_None)
			Distance = 50.0f;
		SetLocation(TargetCharacter->GetNavAgentLocation() - DirectionToTarget * Distance, true);
	}
	else
	{
		SetLocation(TargetCharacter->GetNavAgentLocation(), true);
	}
	
	if (HasReachedLocation(GetDestinationTolerance()))
	{
		MaxActivityTime = 10.0f;

		if (ItemType == EItemCategory::IC_None) // try melee
		{
			const FVector DirectionToTarget = (TargetCharacter->GetNavAgentLocation() - GetCharacter()->GetActorLocation()).GetSafeNormal2D();

			const float Dot = FVector::DotProduct(GetCharacter()->GetActorForwardVector(), DirectionToTarget);
			if (Dot > 0.9f)
			{
				GetCharacter()->PendingMeleeTarget = TargetCharacter;
				GetCharacter()->GetEquippedItem()->PlayItemAnimation(GetCharacter()->GetEquippedItem()->AnimationData->MeleeHit, false);
				
				bHasEverFired = true;
				OwningController->FinishActivity(this, true, true);
			}
		}
		else
		{
			if (GetCharacter()->GetInventoryComponent()->IsEquippingItem())
				return;

			if (OwningController->GetCombatActivity()->IsReloading())
				return;
			
			ABaseItem* EquippedItem = nullptr;
			if (ABaseItem* Item = GetCharacter()->GetEquippedItem())
			{
				if (Item->ContainsItemCategory(ItemType))
				{
					EquippedItem = Item;
				}
				else
				{
					EquipItem(ItemType);
				}
			}

			if (EquippedItem)
			{
				TimeEquipped += DeltaTime;
				
				if (bHasLOS && TimeEquipped > 1.0f)
				{
					if (!OwningController->GetCombatActivity()->IsTryingToFireAtScriptedActor())
					{
						bool bIsSingleShot = false;
						
						if (const ABaseMagazineWeapon* Weapon = Cast<ABaseMagazineWeapon>(EquippedItem))
						{
							bIsSingleShot = Weapon->CurrentFireMode == EFireMode::FM_Burst || Weapon->CurrentFireMode == EFireMode::FM_Single;
							
							if (bIsSingleShot)
							{
								TimeEquipped = 0.0f;
							}
							
							OwningController->GetCombatActivity()->ScriptedFireAtActor(TargetCharacter, bIsSingleShot ? 0.05f : 0.5f, true, 0.0f);
						}
						else
						{
							EquippedItem->OnItemPrimaryUse();
						}
						
						OwningController->GetCombatActivity()->ScriptedFireAtActor(TargetCharacter, bIsSingleShot ? 0.05f : 0.5f, true, 0.0f);
						bHasEverFired = true;
					}
				}
			}
		}
	}
}

void UEngageTargetLessLethalActivity::FinishedActivity(bool bSuccess)
{
	Super::FinishedActivity(bSuccess);

	EquipWeapon();
}

bool UEngageTargetLessLethalActivity::CanFinishActivity() const
{
	return false;
}

bool UEngageTargetLessLethalActivity::OverrideFocalPoint(FVector& FocalPoint)
{
	if (HasReachedLocation(300.0f))
	{
		FocalPoint = TargetCharacter->GetMesh()->GetBoneLocation("spine_3");
		return true;
	}

	return false;
}

float UEngageTargetLessLethalActivity::GetDestinationTolerance() const
{
	if (ItemType == EItemCategory::IC_None)
		return 100.0f;
	
	return 400.0f;
}

void UEngageTargetLessLethalActivity::ResetData()
{
	Super::ResetData();

	TargetCharacter = nullptr;
	ItemType = EItemCategory::IC_None;
	bHasEverFired = false;
	TimeEquipped = 0.0f;
	TimeUsing = 0.0f;
}
