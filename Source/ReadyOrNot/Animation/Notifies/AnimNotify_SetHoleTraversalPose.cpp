// Copyright Void Interactive, 2022

#include "Animation/Notifies/AnimNotify_SetHoleTraversalPose.h"

#include "Characters/CyberneticCharacter.h"

void UAnimNotify_SetHoleTraversalPose::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::Notify(MeshComp, Animation);

	#if WITH_EDITOR
	if (MeshComp->GetWorld()->WorldType != EWorldType::EditorPreview && MeshComp->GetWorld()->WorldType != EWorldType::Editor)
		ensureAlways(Pose != nullptr);
	#endif

	if (ACyberneticCharacter* OwnerCharacter = MeshComp->GetOwner<ACyberneticCharacter>())
	{
		OwnerCharacter->Rep_HoleTraversalAnimState.LoopAnim = Pose;
	}
}
