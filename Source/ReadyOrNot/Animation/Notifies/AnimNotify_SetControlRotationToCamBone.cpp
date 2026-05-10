// Copyright Void Interactive, 2021

#include "AnimNotify_SetControlRotationToCamBone.h"

#include "Actors/Door.h"
#include "ReadyOrNot.h"

void USetControlRotationToCamBoneAnimNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	APlayerCharacter* pc = Cast<APlayerCharacter>(MeshComp->GetOwner());
	if (pc)
	{
		FRotator CurrentControlRotation = pc->GetControlRotation();

		pc->Client_SetControlRotation(FRotator(0.0f, CurrentControlRotation.Yaw, CurrentControlRotation.Roll));
	}
}
