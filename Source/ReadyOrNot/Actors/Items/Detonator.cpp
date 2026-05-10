// Void Interactive, 2020

#include "Detonator.h"

#include "Actors/Gameplay/PlacedC2Explosive.h"
#include "Components/InventoryComponent.h"

#include "lib/ReadyOrNotFunctionLibrary.h"

void ADetonator::OnItemPrimaryUse()
{
	if (IsBlockingAnimationPlaying())
		return;
	
	if (const AReadyOrNotCharacter* Character = GetOwnerCharacter())
	{
		if (Character->IsCrouching())
		{
			PlayItemAnimation(AnimationData->Crouch_FireSingle[0], false);
		}
		else
		{
			PlayItemAnimation(AnimationData->FireSingle[0], false);
		}
	}

	Super::OnItemPrimaryUse();
}

void ADetonator::OnItemEndSecondaryUse()
{
	Super::OnItemEndSecondaryUse();
	
	// Switch back to C2, if we can.
	if (APlayerCharacter* pc = Cast<APlayerCharacter>(GetOwner()))
	{
		pc->GetInventoryComponent()->EquipItemOfType(EItemCategory::IC_C2Explosive);
	}
}

bool ADetonator::IsBlockingAnimationPlaying(TArray<EBlockingAnimationExclusion> Exclusions) const
{
	if (!AnimationData)
		return false;
	
	if (const APlayerCharacter* OwnerCharacter = GetOwnerPlayerCharacter())
	{
		if (AnimationData->FireSingle.Num() > 0)
		{
			if (OwnerCharacter->Is1PMontagePlaying(AnimationData->FireSingle[0].Body_FP))
			{
				return true;
			}
		}
		
		if (AnimationData->Crouch_FireSingle.Num() > 0)
		{
			if (OwnerCharacter->Is1PMontagePlaying(AnimationData->Crouch_FireSingle[0].Body_FP))
			{
				return true;
			}
		}
	}

	return Super::IsBlockingAnimationPlaying(Exclusions);
}

void ADetonator::Server_DetonateC2_Implementation()
{
	if (!GetOwner())
		return;
	
	for (int32 i = 0; i < PlacedCharges.Num(); i++)
	{
		if (PlacedCharges[i] != nullptr)
		{
			if (FVector::Dist(GetOwner()->GetActorLocation(), PlacedCharges[i]->GetActorLocation()) < MaxDetonationDistance)
			{
				PlacedCharges[i]->Server_DetonateC2();
				PlacedCharges[i] = nullptr;								
			}
		}		
	}

	PlacedChargesCount = 0;
	for (int32 i = 0; i < PlacedCharges.Num(); i++)
	{
		if (PlacedCharges[i] != nullptr) {
			PlacedChargesCount++;
		}
	}

	EquipLastEquippedWeapon();
	OnItemUseCompleted.Broadcast(this);
}

void ADetonator::EquipLastEquippedWeapon()
{
	if (APlayerCharacter* PC = Cast<APlayerCharacter>(GetOwner()))
	{
		PC->GetInventoryComponent()->EquipLastEquippedWeapon();
	}
}
