// Void Interactive, 2020


#include "Animation/Notifies/AnimNotify_KillMyself.h"

void UAnimNotify_KillMyself::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	AReadyOrNotCharacter* MyCharacter = Cast<AReadyOrNotCharacter>(MeshComp->GetOwner());
	if (MyCharacter)
	{
		if (MyCharacter->HasAuthority())
		{
			// Dont to avoid post eval anim, killing cannot happen in this notify
			MyCharacter->GetWorld()->GetTimerManager().SetTimerForNextTick(MyCharacter, &AReadyOrNotCharacter::Kill);
		}
	}
}
