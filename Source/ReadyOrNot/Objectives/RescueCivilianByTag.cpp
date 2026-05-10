// Copyright Void Interactive, 2021

#include "RescueCivilianByTag.h"

#include "GameModes/CoopGM.h"

void ARescueCivilianByTag::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	ACoopGM* GM = Cast<ACoopGM>(GetWorld()->GetAuthGameMode());
	if (!IsValid(GM))
		return;

	GM->OnAllAISpawned.AddDynamic(this, &ARescueCivilianByTag::OnAISpawned);
}

void ARescueCivilianByTag::TickObjective()
{
	if (!HasAuthority())
		return;

	bool bArrested = false;
	if (HasNeutralizedCivilianByTag(bArrested))
	{
		if (!bArrested)
		{
			ObjectiveFailed();	
		}
		else
		{
			ObjectiveCompleted();
		}
	}
}

bool ARescueCivilianByTag::HasNeutralizedCivilianByTag(bool& bArrested)
{
	if (!IsValid(Civilian))
		return false;
	
	if (!Civilian->IsDeadOrUnconscious())
	{
		if (Civilian->IsArrested())
		{
			bArrested = true;
			return true;
		}
	}
	else
	{
		bArrested = false;
		return true;
	}

	return false;
}

void ARescueCivilianByTag::OnCivilianKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter)
{
	if (IsValid(KilledCharacter) && IsValid(Civilian) && KilledCharacter == Civilian)
	{
		ObjectiveFailed();
	}
}

void ARescueCivilianByTag::OnAISpawned()
{
	// Now that AI have been spawned in, we can grab a reference to the relevant civilian
	for (TActorIterator<ACyberneticCharacter> It(GetWorld()); It; ++It)
	{
		ACyberneticCharacter* AICharacter = *It;
		if (AICharacter->GetTeam() != ETeamType::TT_SUSPECT && AICharacter->GetTeam() != ETeamType::TT_CIVILIAN)
			continue;
		
		if (!AICharacter->Tags.Contains(CivilianTag))
			continue;

		if (IsValid(AICharacter))
		{
			Civilian = AICharacter;
			// Bind to on death so we can fail objective if need be even after objective has completed and stopped ticking
			Civilian->OnCharacterKilled.AddDynamic(this, &ARescueCivilianByTag::OnCivilianKilled);
		}
	}
}