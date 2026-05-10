// Void Interactive, 2020

#include "Animation/Notifies/AnimNotify_AIDoorBreachShotgun.h"

void UAnimNotify_AIDoorBreachShotgun::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::Notify(MeshComp, Animation);

	if (const ACyberneticCharacter* AICharacter = Cast<ACyberneticCharacter>(MeshComp->GetOwner()))
	{
		AICharacter->OnDoorShotgunBreach_FromAnimNotify.Broadcast();
	}
}
