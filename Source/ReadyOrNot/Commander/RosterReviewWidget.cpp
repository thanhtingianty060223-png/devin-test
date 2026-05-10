// Copyright Void Interactive, 2023

#include "RosterReviewWidget.h"

#include "CommanderGM.h"
#include "GameModes/LobbyGM.h"

void URosterReviewWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!GetWorld())
		return;
	
	if (ACommanderGM* CommanderGM = GetWorld()->GetAuthGameMode<ACommanderGM>())
	{
		RosterManager = CommanderGM->RosterManager;
	}
	else if (ALobbyGM* LobbyGM = GetWorld()->GetAuthGameMode<ALobbyGM>())
	{
		RosterManager = LobbyGM->RosterManager;
	}
	
	if (!RosterManager)
		return;

	const TArray<URosterCharacter*>& Characters = RosterManager->GetAllCharacters();

	TArray<URosterCharacter*> RemovedCharacters;
	TArray<URosterCharacter*> IncapacitatedCharacters;
	TArray<URosterCharacter*> ReturningCharacters;
	
	for (URosterCharacter* Character : Characters)
	{
		ERosterCharacterState CurrentState = Character->State;
		ERosterCharacterState PreviousState = Character->PreviousState;

		if (PreviousState == CurrentState)
			continue;

		if (CurrentState == ERosterCharacterState::Deceased)
		{
			RemovedCharacters.AddUnique(Character);
		}
		else if (CurrentState == ERosterCharacterState::Incapacitated)
		{
			IncapacitatedCharacters.AddUnique(Character);
		}
		else if (CurrentState == ERosterCharacterState::Available)
		{
			ReturningCharacters.AddUnique(Character);
		}
	}
	
	AddSquadCharacters(RosterManager->GetSquadCharacters());
	AddRemovedCharacters(RemovedCharacters);
	AddIncapacitatedCharacters(IncapacitatedCharacters);
	AddReturningCharacters(ReturningCharacters);
}

TArray<URosterCharacter*> URosterReviewWidget::GetAllCharacters()
{
	if (!RosterManager)
		return {};

	return RosterManager->GetAllCharacters();
}

TArray<URosterCharacter*> URosterReviewWidget::GetCharactersSortedByReviewScore() const
{
	TArray<URosterCharacter*> Characters;
	return Characters;
}

void URosterReviewWidget::OpenRoster()
{
	if (!GetWorld())
		return;
	
	ALobbyGM* LobbyGM = GetWorld()->GetAuthGameMode<ALobbyGM>();
	if (!LobbyGM || !LobbyGM->CommanderProfile)
		return;

	LobbyGM->OpenRosterSelection();
}

FText URosterReviewWidget::GetTherapistReminderPrompt()
{
	if (!RosterManager)
		return FText();

	const URosterEventData* EventData = RosterManager->GetEventData();
	if (!EventData)
		return FText();

	// Calculate the average stress for the current squad
	float AverageSquadStress = 0.0f;
	for (auto SquadCharacterPair : RosterManager->GetSquadCharacters())
	{
		if (!IsValid(SquadCharacterPair.Value))
			continue;
		
		AverageSquadStress += SquadCharacterPair.Value->StressLevel;
	}
	AverageSquadStress /= RosterManager->GetSquadCharacters().Num();

	// Find eligible events
	TArray<FText> EligibleEvents;
	for (const FTherapistReminderEvent& Event : EventData->TherapistReminderEvents)
	{
		if (AverageSquadStress >= Event.SquadAverageStressRequired)
			EligibleEvents.Add(Event.EventText);
	}

	// Return event text
	if (EligibleEvents.Num() <= 0)
		return FText();
	
	return EligibleEvents[FMath::RandRange(0, EligibleEvents.Num() - 1)];
}

