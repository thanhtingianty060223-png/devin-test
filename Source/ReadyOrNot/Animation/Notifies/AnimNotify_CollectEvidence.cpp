// Void Interactive, 2020

#include "AnimNotify_CollectEvidence.h"

void UAnimNotify_CollectEvidence::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::Notify(MeshComp, Animation);

	if (AReadyOrNotCharacter* OwningCharacter = Cast<AReadyOrNotCharacter>(MeshComp->GetOwner()))
	{
		if (bCollectFinished)
			OwningCharacter->CollectPendingEvidence();

		if (const ACyberneticCharacter* OwningAI = Cast<ACyberneticCharacter>(OwningCharacter))
		{
			if (bCollectFinished)
				OwningAI->OnCollectPendingEvidenceEnd_FromAnimNotify.Broadcast();
			else
				OwningAI->OnCollectPendingEvidenceBegin_FromAnimNotify.Broadcast();
		}
	}
}
