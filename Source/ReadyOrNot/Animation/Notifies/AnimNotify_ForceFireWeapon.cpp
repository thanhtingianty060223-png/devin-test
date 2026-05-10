// Void Interactive, 2020

#include "AnimNotify_ForceFireWeapon.h"

#include "Characters/CyberneticController.h"

void UAnimNotify_ForceFireWeapon::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::Notify(MeshComp, Animation);

	if (ACyberneticCharacter* AI = Cast<ACyberneticCharacter>(MeshComp->GetOwner()))
	{
		AI->OnWeaponForceFire_FromAnimNotify.Broadcast();
		
		const ACyberneticController* Controller = Cast<ACyberneticController>(AI->GetController());
		if ((Controller && Controller->GetTargetingComp()->GetTrackedTarget() != nullptr) || bNoEnemyRequired)
		{
			AI->ForceFireGun(Chance);
		}
	}
}
