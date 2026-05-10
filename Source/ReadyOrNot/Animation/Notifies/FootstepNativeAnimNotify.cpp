#include "FootstepNativeAnimNotify.h"

void UFootstepNativeAnimNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	//GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Purple, __FUNCTION__);

	if (MeshComp)
	{
		if (AReadyOrNotCharacter* CurPlayerChar = Cast<AReadyOrNotCharacter>(MeshComp->GetOwner()))
		{
			CurPlayerChar->SpawnFootstepEffect();
		}
	}
}