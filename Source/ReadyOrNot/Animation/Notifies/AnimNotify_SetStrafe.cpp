// Void Interactive, 2020

#include "AnimNotify_SetStrafe.h"

#include "Characters/CyberneticCharacter.h"

void UAnimNotify_SetStrafe::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	// Done automatically by the AI
	/*
	ACyberneticCharacter* Character = Cast<ACyberneticCharacter>(MeshComp->GetOwner());
	if (Character)
	{
		Character->SetIsStrafing(bSetStrafe, false);
	}
	*/
}
