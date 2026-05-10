// Copyright Void Interactive, 2023

#include "Animation/Notifies/AnimNotify_AIDoorBreachRam.h"

void UAnimNotify_AIDoorBreachRam::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::Notify(MeshComp, Animation);

	if (const ACyberneticCharacter* AICharacter = Cast<ACyberneticCharacter>(MeshComp->GetOwner()))
	{
		AICharacter->OnDoorRamBreach_FromAnimNotify.Broadcast();
	}
}
