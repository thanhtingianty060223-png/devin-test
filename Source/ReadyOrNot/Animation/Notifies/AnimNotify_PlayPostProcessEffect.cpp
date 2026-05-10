// Void Interactive, 2020

#include "AnimNotify_PlayPostProcessEffect.h"

#include "Components/PlayerPostProcessing.h"

void UAnimNotify_PlayPostProcessEffect::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::Notify(MeshComp, Animation);
	
	if (APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(MeshComp->GetOwner()))
	{
		if (PlayerCharacter->GetPlayerPostProcessing())
		{
			PlayerCharacter->GetPlayerPostProcessing()->PlayPostProcessEffect_Name(PostProcessEffectName);
			PlayerCharacter->GetPlayerPostProcessing()->StopBleedingEffect();
		}
	}
}
