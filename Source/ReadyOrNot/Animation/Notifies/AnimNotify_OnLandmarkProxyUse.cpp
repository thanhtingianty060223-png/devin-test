// Void Interactive, 2020

#include "Animation/Notifies/AnimNotify_OnLandmarkProxyUse.h"

#include "Characters/CyberneticCharacter.h"
#include "Characters/CyberneticController.h"

#include "Info/Activities/TakeCoverAtLandmarkActivity.h"

void UAnimNotify_OnLandmarkProxyUse::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::Notify(MeshComp, Animation);

	if (const ACyberneticCharacter* OwnerCharacter = Cast<ACyberneticCharacter>(MeshComp->GetOwner()))
	{
		if (const ACyberneticController* CyberneticController = OwnerCharacter->GetCyberneticsController())
		{
			if (UTakeCoverAtLandmarkActivity* TakeCoverAtLandmarkActivity = CyberneticController->GetCurrentActivity<UTakeCoverAtLandmarkActivity>())
			{
				TakeCoverAtLandmarkActivity->Notify_OnProxyUse();
			}
		}
	}
}
