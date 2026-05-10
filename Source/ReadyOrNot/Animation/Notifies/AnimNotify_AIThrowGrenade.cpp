// Copyright Void Interactive, 2023

#include "Animation/Notifies/AnimNotify_AIThrowGrenade.h"

#include "Actors/BaseGrenade.h"

void UAnimNotify_AIThrowGrenade::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::Notify(MeshComp, Animation);

	if (ACyberneticCharacter* AICharacter = Cast<ACyberneticCharacter>(MeshComp->GetOwner()))
	{
		ABaseGrenade* Grenade = Cast<ABaseGrenade>(AICharacter->PendingThrownItem);
		if (Grenade)
		{
			Grenade->AIThrow();
		}
	}
}
