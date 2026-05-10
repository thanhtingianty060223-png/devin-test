// Void Interactive, 2020


#include "AnimNotify_ToggleBoneVis.h"

void UAnimNotifyState_HideBoneVis::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration)
{
	MeshComp->HideBoneByName(BoneNameToHide, PBO_None);
}

void UAnimNotifyState_HideBoneVis::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
 MeshComp->UnHideBoneByName(BoneNameToHide);
}