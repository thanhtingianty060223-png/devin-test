// Copyright Void Interactive, 2023

#include "TherapistWidget.h"

#include "RosterManager.h"
#include "GameModes/LobbyGM.h"

void UTherapistWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!GetWorld())
		return;
	
	ALobbyGM* CommanderGM = GetWorld()->GetAuthGameMode<ALobbyGM>();
	if (!CommanderGM)
		return;

	URosterManager* RosterManager = CommanderGM->RosterManager;
	if (!RosterManager)
		return;

	AReadyOrNotPlayerController* PlayerController = GetOwningPlayer<AReadyOrNotPlayerController>();
	if (!PlayerController)
		return;

	PlayerController->HideHUD();

	CharactersInTherapy.Empty();
	for (URosterCharacter* RosterCharacter : RosterManager->GetAllCharacters())
	{
		if (RosterCharacter->State == ERosterCharacterState::InTherapy)
		{
			CharactersInTherapy.Add(RosterCharacter);
		}
	}
}

FText UTherapistWidget::GetTherapistText()
{
	if (!GetWorld())
		return FText();

	URosterManager* RosterManager = nullptr;
	if (ALobbyGM* LobbyGM = GetWorld()->GetAuthGameMode<ALobbyGM>())
	{
		RosterManager = LobbyGM->RosterManager;
	}

	if (!RosterManager)
		return FText();

	const URosterEventData* EventData = RosterManager->GetEventData();
	if (!EventData)
		return FText();
	
	if (CharactersInTherapy.Num() <= 0)
	{
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
	else
	{
		for (URosterCharacter* Character : RosterManager->GetAllCharacters())
		{
			if (IsValid(Character) && Character->State == ERosterCharacterState::InTherapy)
				return Character->MostRecentEventText;
		}
	}
	
	return FText::FromString("Hello world");
}

void UTherapistWidget::CloseTherapistWidget()
{
	AReadyOrNotPlayerController* PlayerController = GetOwningPlayer<AReadyOrNotPlayerController>();
	if (!PlayerController)
	{
		RemoveFromParent();
		return;
	}

	PlayerController->RemoveWidgetFromStack("TherapistWidget");
	PlayerController->ShowHUD();
}
