// Copyright Void Interactive, 2021

#include "RescueAllOfTheCivilians.h"

#include "GameModes/CoopGM.h"
#include "Info/ScoringManager.h"
#include "Characters/AI/CivilianCharacter.h"

ARescueAllOfTheCivilians::ARescueAllOfTheCivilians()
{
	ObjectiveName = NSLOCTEXT("RescueAllOfTheCivilians", "Rescue All of the Civilians", "Rescue All of the Civilians");
	ObjectiveDescription = NSLOCTEXT("RescueAllOfTheCivilians", "Detain any unarmed contacts at the scene.", "Detain any unarmed contacts at the scene.");
}

void ARescueAllOfTheCivilians::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	ACoopGM* GM = Cast<ACoopGM>(GetWorld()->GetAuthGameMode());
	if (!IsValid(GM))
		return;

	GM->OnAllAISpawned.AddDynamic(this, &ARescueAllOfTheCivilians::OnAISpawned);
}

void ARescueAllOfTheCivilians::TickObjective()
{
	if (!HasAuthority())
		return;

	if (!IsObjectiveInProgress())
		return;

	if (AScoringManager::Get()->AreAllCiviliansRestrained())
	{
		ObjectiveCompleted();
	}
}

void ARescueAllOfTheCivilians::OnAISpawned()
{
	for (TActorIterator<ACivilianCharacter> It(GetWorld()); It; ++It)
	{
		ACivilianCharacter* CivilianCharacter = *It;
		if (!IsValid(CivilianCharacter))
			continue;
		
		if (CivilianCharacter->GetTeam() != ETeamType::TT_CIVILIAN)
			continue;
		
		CivilianCharacter->OnCharacterKilled.AddDynamic(this, &ARescueAllOfTheCivilians::OnCivilianKilled);
	}
}

void ARescueAllOfTheCivilians::OnCivilianKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter)
{
	ObjectiveFailed();
}