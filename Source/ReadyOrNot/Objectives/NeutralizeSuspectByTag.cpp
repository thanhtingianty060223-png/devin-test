// Copyright Void Interactive, 2021

#include "NeutralizeSuspectByTag.h"

#include "GameModes/CoopGM.h"


void ANeutralizeSuspectByTag::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	
	ACoopGM* GM = Cast<ACoopGM>(GetWorld()->GetAuthGameMode());
	if (!IsValid(GM))
		return;

	GM->OnAllAISpawned.AddDynamic(this, &ANeutralizeSuspectByTag::OnAISpawned);
}

void ANeutralizeSuspectByTag::TickObjective()
{
	if (!HasAuthority())
		return;
	
	bool bWasArrested = false;
	
	if (HasNeutralizedAIByTag(bWasArrested))
	{
		if (bRequireArrest && !bWasArrested)
		{
			ObjectiveFailed();
		}
		else
		{
			ObjectiveCompleted();
		}
	}
}

bool ANeutralizeSuspectByTag::HasNeutralizedAIByTag(bool& bArrested)
{
	if (!IsValid(Suspect))
		return false;
	
	if (!Suspect->IsDeadOrUnconscious())
	{
		if (Suspect->IsArrested())
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

void ANeutralizeSuspectByTag::OnSuspectKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter)
{
	// If suspect was killed and the objective was requiring arrest, switch the objective to failed
	// This will fail the mission even if it was previously completed
	if (bRequireArrest || KilledCharacter->bArrestComplete)
		ObjectiveFailed();
}

void ANeutralizeSuspectByTag::OnAISpawned()
{
	// Now that AI have been spawned in, we can grab a reference to the relevant suspect
	for (TActorIterator<ACyberneticCharacter> It(GetWorld()); It; ++It)
	{
		ACyberneticCharacter* AICharacter = *It;
		if (AICharacter->GetTeam() != ETeamType::TT_SUSPECT && AICharacter->GetTeam() != ETeamType::TT_CIVILIAN)
			continue;
		
		if (!AICharacter->Tags.Contains(SuspectTag))
			continue;

		if (IsValid(AICharacter))
		{
			Suspect = AICharacter;
			// Bind to on death so we can fail objective if need be even after objective has completed and stopped ticking
			Suspect->OnCharacterKilled.AddDynamic(this, &ANeutralizeSuspectByTag::OnSuspectKilled);
		}
	}
}