// Void Interactive, 2020

#include "AnimNotify_Breach_C2Detonate.h"

//#include "Characters/CyberneticController.h"
//#include "Info/Activities/Team/TeamBreachAndClearActivity.h"

void UAnimNotify_Breach_C2Detonate::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::Notify(MeshComp, Animation);
	
	AReadyOrNotCharacter* Character = Cast<AReadyOrNotCharacter>(MeshComp->GetOwner());
	if (Character)
	{
		Character->OnC2Detonate_FromAnimNotify.Broadcast();
		
		//ACyberneticController* CyberneticController = Cast<ACyberneticController>(PlayerCharacter->GetController());
		//if (CyberneticController)
		//{
		//	UTeamBreachAndClearActivity* DeployWedgeActivity = CyberneticController->GetCurrentActivity<UTeamBreachAndClearActivity>();
		//	if (DeployWedgeActivity)
		//	{
		//		DeployWedgeActivity->OnC2Detonated();
		//	}
		//}
	}
}
