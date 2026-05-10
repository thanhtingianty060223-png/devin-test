// Copyright Void Interactive, 2023

#include "MemorialWidget.h"

#include "Commander/RosterManager.h"
#include "GameModes/LobbyGM.h"

void UMemorialWidget::NativeOnInitialized()
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

	MemorialCharacters.Empty();
	for (URosterCharacter* RosterCharacter : RosterManager->GetPreviousCharacters())
	{
		if (RosterCharacter->State == ERosterCharacterState::Deceased &&
			RosterCharacter->RemovalReason == ERosterRemovalReason::Deceased)
		{
			MemorialCharacters.Add(RosterCharacter);
		}
	}
}

void UMemorialWidget::CloseMemorialWidget()
{
	AReadyOrNotPlayerController* PlayerController = GetOwningPlayer<AReadyOrNotPlayerController>();
	if (!PlayerController)
	{
		RemoveFromParent();
		return;
	}

	PlayerController->RemoveWidgetFromStack("MemorialWidget");
	PlayerController->ShowHUD();
}
