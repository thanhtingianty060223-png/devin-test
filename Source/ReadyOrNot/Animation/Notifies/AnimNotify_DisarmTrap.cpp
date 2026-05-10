// Void Interactive, 2020

#include "AnimNotify_DisarmTrap.h"

#include "Characters/CyberneticController.h"

void UAnimNotify_DisarmTrap::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::Notify(MeshComp, Animation);

    if (const AReadyOrNotCharacter* OwnerCharacter = Cast<AReadyOrNotCharacter>(MeshComp->GetOwner()))
    {
    	if (bDisarmFinished)
    		OwnerCharacter->OnTrapDisarmEnd_FromAnimNotify.Broadcast();
    	else
    		OwnerCharacter->OnTrapDisarmBegin_FromAnimNotify.Broadcast();
    }
}
