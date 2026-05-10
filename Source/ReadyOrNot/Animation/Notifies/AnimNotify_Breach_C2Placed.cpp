// Void Interactive, 2020

#include "AnimNotify_Breach_C2Placed.h"

void UAnimNotify_Breach_C2Placed::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::Notify(MeshComp, Animation);
	
	AReadyOrNotCharacter* Character = Cast<AReadyOrNotCharacter>(MeshComp->GetOwner());
	if (Character)
	{
		Character->OnC2Placed_FromAnimNotify.Broadcast();
	}
}
