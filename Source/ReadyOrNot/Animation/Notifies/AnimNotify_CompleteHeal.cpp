// Void Interactive, 2020


#include "AnimNotify_CompleteHeal.h"

#include "Components/BleedComponent.h"

void UAnimNotify_CompleteHeal::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::Notify(MeshComp, Animation);
	APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(MeshComp->GetOwner());
	if (PlayerCharacter)
	{
		if (PlayerCharacter->GetBleedComponent())
		{
			PlayerCharacter->GetBleedComponent()->CompleteHeal();
		}
	}
	
}
