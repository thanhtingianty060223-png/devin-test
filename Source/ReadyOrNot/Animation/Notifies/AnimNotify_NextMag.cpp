// Copyright Void Interactive, 2022


#include "Animation/Notifies/AnimNotify_NextMag.h"

void UAnimNotify_NextMag::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::Notify(MeshComp, Animation);

	if (const AReadyOrNotCharacter* OwnerCharacter = Cast<AReadyOrNotCharacter>(MeshComp->GetOwner()))
	{
		if (ABaseMagazineWeapon* EquippedWeapon = OwnerCharacter->GetEquippedItem<ABaseMagazineWeapon>())
		{
			EquippedWeapon->Server_NextMagazine();
		}
	}
}
