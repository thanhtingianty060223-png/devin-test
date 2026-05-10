// Void Interactive, 2020


#include "AnimNotify_PlayMontage.h"

void UAnimNotify_PlayMontage::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(MeshComp->GetOwner());
	if (PlayerCharacter)
	{
		if (MeshComp == PlayerCharacter->GetMesh1P())
		{
			PlayerCharacter->Play1PMontage_NonClient(Montage);
		} else if (MeshComp == PlayerCharacter->GetMesh())
		{
			PlayerCharacter->PlayLocal3PMontage(Montage);
		}
	} else
	{
		AReadyOrNotCharacter* Character = Cast<AReadyOrNotCharacter>(MeshComp->GetOwner());
		if (Character)
		{
			Character->PlayLocal3PMontage(Montage);
		}
	}
}
