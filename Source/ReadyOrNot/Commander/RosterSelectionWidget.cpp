// Copyright Void Interactive, 2023

#include "RosterSelectionWidget.h"

#include "CommanderGM.h"
#include "CommanderProfile.h"
#include "RosterManager.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "GameModes/LobbyGM.h"

URosterSelectionWidget::FOnRosterSelectionOpened URosterSelectionWidget::OnRosterSelectionOpened;
URosterSelectionWidget::FOnRosterSelectionClosed URosterSelectionWidget::OnRosterSelectionClosed;

void URosterSelectionWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!GetWorld())
		return;
	
	if (ACommanderGM* CommanderGM = GetWorld()->GetAuthGameMode<ACommanderGM>())
	{
		CommanderProfile = CommanderGM->CommanderProfile;
		RosterManager = CommanderGM->RosterManager;
	}
	else if (ALobbyGM* LobbyGM = GetWorld()->GetAuthGameMode<ALobbyGM>())
	{
		CommanderProfile = LobbyGM->CommanderProfile;
		RosterManager = LobbyGM->RosterManager;
	}

	if (!CommanderProfile || !RosterManager)
		return;
	
	InitializeRoster();
}

void URosterSelectionWidget::NativeConstruct()
{
	Super::NativeConstruct();
	OnRosterSelectionOpened.Broadcast();
}

void URosterSelectionWidget::NativeDestruct()
{
	Super::NativeDestruct();
	OnRosterSelectionClosed.Broadcast();
}

void URosterSelectionWidget::RefreshRoster()
{
	RefreshRosterEvent();
}

TArray<URosterCharacter*> URosterSelectionWidget::GetAllCharacters()
{
	if (!RosterManager)
		return {};
	
	return RosterManager->GetAllCharacters();
}

TMap<ERosterSquadPosition, URosterCharacter*> URosterSelectionWidget::GetSquadCharacters()
{
	if (!RosterManager)
		return {};
	
	return RosterManager->GetSquadCharacters();
}

TArray<URosterCharacter*> URosterSelectionWidget::GetRecruitableCharacters()
{
	if (!RosterManager)
		return {};

	return RosterManager->GetRecruitableCharacters();
}

int32 URosterSelectionWidget::GetCurrentRosterSize() const
{
	if (!RosterManager)
		return 0;
	
	return RosterManager->GetCurrentRosterSize();
}

int32 URosterSelectionWidget::GetMaximumRosterSize() const
{
	if (!RosterManager)
		return 0;
	
	return RosterManager->GetMaximumRosterSize();
}

TArray<int32> URosterSelectionWidget::GetUnlockableSlotMissionsRemaining() const
{
	if (!RosterManager)
		return {};

	return RosterManager->GetUnlockableSlotsMissionsRemaining();
}

FRosterLoadout URosterSelectionWidget::GetCharacterLoadout(URosterCharacter* Character)
{
	if (!RosterManager)
		return FRosterLoadout();

	return RosterManager->GetCharacterLoadout(Character);
}

void URosterSelectionWidget::SetSquadCharacter(URosterCharacter* Character, ERosterSquadPosition Position)
{
	if (!RosterManager || !CommanderProfile)
		return;

	RosterManager->SetCharacterSquadPosition(Character, Position);
	
	OnCharactersUpdated();
}

void URosterSelectionWidget::RecruitCharacter(URosterCharacter* Character)
{
	if (!RosterManager)
		return;

	RosterManager->RecruitCharacter(Character);
}

void URosterSelectionWidget::SendCharacterToTherapist(URosterCharacter* Character)
{
	if (!RosterManager)
		return;

	RosterManager->SetCharacterInTherapy(Character);
	
	UpdateAllRosterWidgets(GetWorld());
}

void URosterSelectionWidget::FireCharacter(URosterCharacter* Character)
{
	if (!RosterManager)
		return;

	RosterManager->FireCharacter(Character);
	
	//OnCharactersUpdated();
	UpdateAllRosterWidgets(GetWorld());
}

bool URosterSelectionWidget::CanUseTherapist(URosterCharacter* RosterCharacter)
{
	if (!RosterManager || !RosterCharacter)
		return false;

	return RosterManager->CanUseTherapist(RosterCharacter);
}

int32 URosterSelectionWidget::GetNumCharactersInTherapy()
{
	if (!RosterManager)
		return 0;

	return RosterManager->GetNumCharactersInTherapy();
}

int32 URosterSelectionWidget::GetMaxCharactersInTherapy()
{
	if (!RosterManager)
		return 0;

	return RosterManager->GetMaxCharactersInTherapy();
}

bool URosterSelectionWidget::CanFireCharacter(URosterCharacter* Character)
{
	if (!RosterManager || !Character)
		return false;

	return RosterManager->CanFireCharacter();
}

TArray<FRosterActiveTraitInfo> URosterSelectionWidget::GetActiveTraits() const
{
	if (!RosterManager)
		return {};
	
	TMap<FName, URosterTrait*> PossibleTraits = RosterManager->GetPossibleTraits();
	
	TArray<FRosterActiveTraitInfo> TraitInfos;
	for (const TPair<FName, URosterTrait*>& Pair : PossibleTraits)
	{
		int32 Count = 0;
		float Value = RosterManager->GetSquadTraitValue(Pair.Key, Count);

		if (Count <= 0)
			continue;
		
		FRosterActiveTraitInfo& TraitInfo = TraitInfos.AddZeroed_GetRef();
		TraitInfo.Trait = Pair.Value;
		TraitInfo.Count = Count;
		TraitInfo.Value = Value;
	}
	return TraitInfos;
}

void URosterSelectionWidget::UpdateAllRosterWidgets(UWorld* World)
{
	if (!World)
		return;
	
	TArray<UUserWidget*> UserWidgets;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, UserWidgets, URosterSelectionWidget::StaticClass(), false);

	for (UUserWidget* UserWidget : UserWidgets)
	{
		URosterSelectionWidget* RosterSelectionWidget = Cast<URosterSelectionWidget>(UserWidget);
		if (!RosterSelectionWidget)
			continue;

		RosterSelectionWidget->OnCharactersUpdated();
	}
}
