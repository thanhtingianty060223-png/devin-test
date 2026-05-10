// Copyright Void Interactive, 2021

#include "FireOnDroppedAnimNotify.h"

void UFireOnDroppedAnimNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::Notify(MeshComp, Animation);
	
	if (const AReadyOrNotCharacter* Character = Cast<AReadyOrNotCharacter>(MeshComp->GetOwner()))
	{
		if (ABaseMagazineWeapon* Weapon = Cast<ABaseMagazineWeapon>(Character->GetInventoryComponent()->GetLastEquippedWeapon()))
		{
			if (Weapon->HasAmmo())
			{
				const float RandomChance = FMath::FRandRange(0.0f, 100.0f);
				
				if (RandomChance > 0.0f && RandomChance <= ChanceToFire)
				{
					Weapon->bAIFireAtBulletSpawn = true;
					Weapon->OnFireAtBulletSpawn();
					Weapon->bAIFireAtBulletSpawn = false;
				}
			}
		}
	}
}
