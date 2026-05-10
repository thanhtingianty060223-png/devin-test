// Void Interactive, 2020

#include "Animation/Notifies/AnimNotify_GetupComplete.h"

void UAnimNotify_GetupComplete::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::Notify(MeshComp, Animation);

	if (const AReadyOrNotCharacter* OwnerCharacter = Cast<AReadyOrNotCharacter>(MeshComp->GetOwner()))
	{
		OwnerCharacter->OnGetupComplete.Broadcast();
	}
}
