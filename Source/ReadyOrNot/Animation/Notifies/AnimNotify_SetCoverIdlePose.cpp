// Void Interactive, 2020

#include "Animation/Notifies/AnimNotify_SetCoverIdlePose.h"

#include "Characters/CyberneticCharacter.h"

void UAnimNotify_SetCoverIdlePose::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::Notify(MeshComp, Animation);

	#if WITH_EDITOR
	if (MeshComp->GetWorld()->WorldType != EWorldType::EditorPreview && MeshComp->GetWorld()->WorldType != EWorldType::Editor)
		ensureAlways(Pose != nullptr);
	#endif

	if (ACyberneticCharacter* OwnerCharacter = MeshComp->GetOwner<ACyberneticCharacter>())
	{
		OwnerCharacter->ActiveCoverIdlePose = Pose;
	}
}
