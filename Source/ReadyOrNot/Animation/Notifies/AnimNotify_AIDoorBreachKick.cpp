// Void Interactive, 2020

#include "Animation/Notifies/AnimNotify_AIDoorBreachKick.h"

void UAnimNotify_AIDoorBreachKick::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::Notify(MeshComp, Animation);
	
	if (const ACyberneticCharacter* AICharacter = Cast<ACyberneticCharacter>(MeshComp->GetOwner()))
	{
		AICharacter->OnDoorKickBreach_FromAnimNotify.Broadcast();
	}
}
