// Void Interactive, 2020

#include "AnimNotify_AIThrowGrenadeComplete.h"

#include "Actors/BaseGrenade.h"

void UAnimNotify_AIThrowGrenadeComplete::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::Notify(MeshComp, Animation);
	
	ACyberneticCharacter* AICharacter = Cast<ACyberneticCharacter>(MeshComp->GetOwner());
	if (AICharacter)
	{
		ABaseGrenade* Grenade = AICharacter->GetInventoryComponent()->GetEquippedItem<ABaseGrenade>();
		if (Grenade)
		{
			Grenade->bAIThrowComplete = true;
			Grenade->bUsed = true;
			Grenade->StartDetonationTimer();
		}
	}
}
