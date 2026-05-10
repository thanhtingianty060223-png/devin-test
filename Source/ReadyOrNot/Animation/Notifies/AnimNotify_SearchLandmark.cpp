// Copyright Void Interactive, 2023

#include "Animation/Notifies/AnimNotify_SearchLandmark.h"

#include "Characters/CyberneticController.h"

#include "Info/Activities/SearchLandmarkActivity.h"

void UAnimNotify_SearchLandmark::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::Notify(MeshComp, Animation);
	
	if (const ACyberneticCharacter* OwnerCharacter = Cast<ACyberneticCharacter>(MeshComp->GetOwner()))
	{
		if (const ACyberneticController* CyberneticController = OwnerCharacter->GetCyberneticsController())
		{
			if (USearchLandmarkActivity* Activity = CyberneticController->GetCurrentActivity<USearchLandmarkActivity>())
			{
				Activity->Notify_SearchLandmark();
			}
		}
	}
}
