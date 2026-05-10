// Void Interactive, 2020

#include "Animation/Notifies/AnimNotify_RemovePlacedC2.h"

#include "Actors/Gameplay/PlacedC2Explosive.h"

void UAnimNotify_RemovePlacedC2::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::Notify(MeshComp, Animation);

	if (APlayerCharacter* OwnerCharacter = MeshComp->GetOwner<APlayerCharacter>())
	{
		OwnerCharacter->RemovePendingC2();
	}
}
