// Copyright Void Interactive, 2020

#include "AnimNotify_ApplyMeleeDamage.h"

void UAnimNotify_ApplyMeleeDamage::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::Notify(MeshComp, Animation);
	
	if (AReadyOrNotCharacter* Character = Cast<AReadyOrNotCharacter>(MeshComp->GetOwner()))
	{
		Character->DoMelee(); // Local only, no stun logic, just visual effects
		Character->Server_DoMelee(); // Server only, applies stun logic
	}
}
