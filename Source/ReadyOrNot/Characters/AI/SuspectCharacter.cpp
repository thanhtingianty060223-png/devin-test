// Copyright Void Interactive, 2022

#include "SuspectCharacter.h"

#include "SuspectController.h"
#include "Characters/CyberneticController.h"
#include "Engine/DemoNetDriver.h"
#include "Actors/Gameplay/PlacedC2Explosive.h"
#include "Subsystems/AchievementSubsystem.h"

ASuspectCharacter::ASuspectCharacter()
{
	AIControllerClass = ASuspectController::StaticClass();
}

bool ASuspectCharacter::OnTakeDamage(float& Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const bool bResult = Super::OnTakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
	
	if (CheckWasFlanked(EventInstigator))
	{
		PlayBarkOrStartConversation(VO_SUSPECTS_AND_CIVILIAN::GETTING_FLANKED, true);
	}
	   // We only want the achievement if the C2 was the last damage causer
	bLastDamageCauserIsC2 = IsValid(Cast<APlacedC2Explosive>(DamageCauser));
	return bResult;
}

bool ASuspectCharacter::CheckWasFlanked(AController* Attacker)
{
	AReadyOrNotPlayerController* PC = Cast<AReadyOrNotPlayerController>(Attacker);
	ACyberneticController* CC = Cast<ACyberneticController>(Attacker);

	APlayerCharacter* Character = PC ? Cast<APlayerCharacter>(PC->GetCharacter()) : CC ? Cast<APlayerCharacter>(CC->GetCharacter()) : nullptr;
	
	if (!Character)
		return false;

	bool bAttackedFromFront = FVector::DotProduct(Character->GetActorForwardVector(), GetActorForwardVector()) <= 0;

	if (bAttackedFromFront)
		return false;
	
	return CC ? CC->GetTeam() != GetTeam() : true;
}

void ASuspectCharacter::OnKilled(AReadyOrNotCharacter* InstigatorCharacter)
{
	Super::OnKilled(InstigatorCharacter);
	
	if (!InstigatorCharacter || !InstigatorCharacter->IsOnSWATTeam())
		return;

	if (!InstigatorCharacter->IsPlayerControlled())
	{
		InstigatorCharacter->PlayRawVOWithCooldown(VO_SWAT_GENERAL::CALL_SUSPECT_KILLED);
	}

	UReadyOrNotGameInstance* GameInstance = Cast<UReadyOrNotGameInstance>(GetWorld()->GetGameInstance());
	if(GameInstance)
	{
		if (GetWorld()->GetDemoNetDriver())
		{
			V_LOGM(LogReadyOrNot, "Adding SuspectKilled Replay Event.")
			GameInstance->AddReplayEvent(SuspectKilled, GetActorLocation(), GetWorld()->GetDemoNetDriver()->GetDemoCurrentTime(), "");
		}
	}
}

bool ASuspectCharacter::TryApplyStunDamage(UStunDamage* InStunDamage, float& Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// Suspects should not take stun damage from Pepperspray now
	if (InStunDamage->StunType == EStunType::ST_Pepperspray)
		return false;

	return Super::TryApplyStunDamage(InStunDamage, Damage, DamageEvent, EventInstigator, DamageCauser);
}
