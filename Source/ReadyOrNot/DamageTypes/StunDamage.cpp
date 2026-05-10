// Copyright Void Interactive, 2017

#include "StunDamage.h"

#include "ReadyOrNot.h"

#include "Characters/AI/SWATCharacter.h"

void UStunDamage::ScriptedStunEvent(class AReadyOrNotCharacter* Victim, float& Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (!Victim)
		return;

	if (Victim->IsDeadOrUnconscious())
		return;

	ASWATCharacter* SwatCharacter = Cast<ASWATCharacter>(Victim);
	if (SwatCharacter)
		return;
	
	if (APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(Victim))
	{
		PlayerCharacter->PendingRecoil += CameraRotationOffset;
		PlayerCharacter->RecoilSpeed = AppliedSpeed;
		PlayerCharacter->GetFirstPersonCameraComponent()->AddExtraPostProcessBlend(PostProcessSettings, 1.0f);
		
		if (APlayerController * PlayerController = Cast<APlayerController>(PlayerCharacter->GetController()))
		{
			PlayerController->ClientStartCameraShake(StunShake);
		}
	}

	Victim->StartStun(StunType, DamageCauser);
}