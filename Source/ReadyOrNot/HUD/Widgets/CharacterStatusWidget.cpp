// Copyright Void Interactive, 2023

#include "CharacterStatusWidget.h"

#include "Characters/AI/SWATCharacter.h"
#include "GameModes/CoopGM.h"
#include "GameModes/LobbyGS.h"

#define LOCTEXT_NAMESPACE "CharacterStatusWidget"

void UCharacterStatusWidget::NativeOnInitialized()
{
	CharacterProxies.Empty();
	
	AReadyOrNotGameState* GameState = GetPlayerContext().GetGameState<AReadyOrNotGameState>();
	if (GameState)
	{
		for (APlayerState* PlayerState : GameState->PlayerArray)
		{
			AReadyOrNotPlayerState* ReadyOrNotPlayerState = Cast<AReadyOrNotPlayerState>(PlayerState);
			if (!ReadyOrNotPlayerState)
				continue;
			
			AReplayCameraPawn* ReplayCameraPawn = Cast<AReplayCameraPawn>(PlayerState->GetPawn());
			if (!ReadyOrNotPlayerState->GetUniqueId().IsValid() || ReadyOrNotPlayerState->bIsReplaySpectator || ReplayCameraPawn)
				continue;
			
			UPlayerCharacterProxy* PlayerProxy = NewObject<UPlayerCharacterProxy>();
			PlayerProxy->PlayerState = PlayerState;

			CharacterProxies.Add(PlayerProxy);
		}
		
		GameState->OnPlayerStateAdded.AddUObject(this, &UCharacterStatusWidget::HandlePlayerAdded);
		GameState->OnPlayerStateRemoved.AddUObject(this, &UCharacterStatusWidget::HandlePlayerRemoved);
	}

	// Singleplayer only
	ACoopGM* CoopGM = GetWorld() ? GetWorld()->GetAuthGameMode<ACoopGM>() : nullptr;
	if (CoopGM)
	{
		for (ASWATCharacter* Swat : CoopGM->SpawnedSWATAI)
		{
			UCoopCharacterProxy* SwatProxy = NewObject<UCoopCharacterProxy>();
			SwatProxy->SwatCharacter = Swat;

			CharacterProxies.Add(SwatProxy);
		}
	}

	Super::NativeOnInitialized();
}

void UCharacterStatusWidget::HandlePlayerAdded(APlayerState* PlayerState)
{
	AReadyOrNotPlayerState* ReadyOrNotPlayerState = Cast<AReadyOrNotPlayerState>(PlayerState);
	if (!ReadyOrNotPlayerState)
		return;
			
	AReplayCameraPawn* ReplayCameraPawn = Cast<AReplayCameraPawn>(PlayerState->GetPawn());
	if (!ReadyOrNotPlayerState->GetUniqueId().IsValid() || ReadyOrNotPlayerState->bIsReplaySpectator || ReplayCameraPawn)
		return;
	
	UPlayerCharacterProxy* PlayerProxy = NewObject<UPlayerCharacterProxy>();
	PlayerProxy->PlayerState = PlayerState;

	CharacterProxies.Add(PlayerProxy);
	OnCharacterAdded(PlayerProxy);
}

void UCharacterStatusWidget::HandlePlayerRemoved(APlayerState* PlayerState)
{
	for (UCharacterProxy* CharacterProxy : CharacterProxies)
	{
		UPlayerCharacterProxy* PlayerProxy = Cast<UPlayerCharacterProxy>(CharacterProxy);
		if (!PlayerProxy)
			continue;

		if (PlayerProxy->PlayerState == PlayerState)
			OnCharacterRemoved(PlayerProxy);
	}
}

FText UCharacterProxy::StatusFromCharacter(AReadyOrNotCharacter* Character)
{
	if (!IsValid(Character))
		return FText();
	
	return PlayerHealthStatusToString(Character->GetHealthStatus());
}

float UCharacterProxy::HealthFromCharacter(AReadyOrNotCharacter* Character)
{
	if (!IsValid(Character) || !IsValid(Character->GetHealthComponent()))
		return 0.0f;

	return Character->GetHealthComponent()->GetNormalizedResource();
}

FRosterLoadout UCharacterProxy::LoadoutFromCharacter(AReadyOrNotCharacter* Character)
{
	if (!Character || !Character->GetInventoryComponent())
		return FRosterLoadout();

	FSpawnedGear& Gear = Character->GetInventoryComponent()->GetSpawnedGear();
	
	FRosterLoadout Loadout;
	Loadout.Primary = IsValid(Gear.Primary) ? Gear.Primary->GetClass() : nullptr;
	Loadout.Secondary = IsValid(Gear.Secondary) ? Gear.Secondary->GetClass() : nullptr;
	Loadout.LongTactical = IsValid(Gear.LongTactical) ? Gear.LongTactical->GetClass() : nullptr;
	
	Loadout.GrenadeSlots.Reserve(Gear.Grenades.Num());
	for (ABaseItem* Grenade : Gear.Grenades)
	{
		if (IsValid(Grenade))
			Loadout.GrenadeSlots.Add(Grenade->GetClass());
	}
	
	Loadout.TacticalSlots.Reserve(Gear.TacticalDevices.Num());
	for (ABaseItem* Tactical : Gear.TacticalDevices)
	{
		if (IsValid(Tactical))
			Loadout.TacticalSlots.Add(Tactical->GetClass());
	}

	return Loadout;
}

FText UPlayerCharacterProxy::GetName()
{
	if (!IsValid(PlayerState))
		return Super::GetName();
	
	return FText::FromString(PlayerState->GetPlayerName());
}

ETeamType UPlayerCharacterProxy::GetTeam()
{
	AReadyOrNotPlayerState* ReadyOrNotPlayerState = Cast<AReadyOrNotPlayerState>(PlayerState);
	if (!IsValid(ReadyOrNotPlayerState))
		return ETeamType::TT_NONE;

	return ReadyOrNotPlayerState->GetTeam();
}

int32 UPlayerCharacterProxy::GetNumber()
{
	AReadyOrNotPlayerState* ReadyOrNotPlayerState = Cast<AReadyOrNotPlayerState>(PlayerState);
	if (!IsValid(ReadyOrNotPlayerState))
		return 0;

	return ReadyOrNotPlayerState->GetPlanningPlayerNumber();
}

FText UPlayerCharacterProxy::GetStatus()
{
	AReadyOrNotPlayerState* ReadyOrNotPlayerState = Cast<AReadyOrNotPlayerState>(PlayerState);
	if (!IsValid(ReadyOrNotPlayerState))
		return FText();

	UWorld* World = ReadyOrNotPlayerState->GetWorld();
	ALobbyGS* LobbyGS = World ? World->GetGameState<ALobbyGS>() : nullptr;
	if (LobbyGS)
	{
		return ReadyOrNotPlayerState->bReady ? LOCTEXT("Ready", "Ready") : LOCTEXT("NotReady", "Not Ready");
	}
	
	return StatusFromCharacter(ReadyOrNotPlayerState->LastCharacter);
}

float UPlayerCharacterProxy::GetHealth()
{
	AReadyOrNotPlayerState* ReadyOrNotPlayerState = Cast<AReadyOrNotPlayerState>(PlayerState);
	if (!IsValid(ReadyOrNotPlayerState))
		return 0.0f;
	
	return HealthFromCharacter(ReadyOrNotPlayerState->LastCharacter);
}

FRosterLoadout UPlayerCharacterProxy::GetLoadout()
{
	AReadyOrNotPlayerState* ReadyOrNotPlayerState = Cast<AReadyOrNotPlayerState>(PlayerState);
	if (!IsValid(ReadyOrNotPlayerState))
		return FRosterLoadout();

	AReadyOrNotCharacter* Character = Cast<AReadyOrNotCharacter>(ReadyOrNotPlayerState->LastCharacter);
	return LoadoutFromCharacter(Character);
}

bool UPlayerCharacterProxy::IsPlayer()
{
	return IsValid(PlayerState);
}

bool UPlayerCharacterProxy::IsLocalPlayer()
{
	AReadyOrNotPlayerState* ReadyOrNotPlayerState = Cast<AReadyOrNotPlayerState>(PlayerState);
	if (!IsValid(ReadyOrNotPlayerState))
		return false;

	return ReadyOrNotPlayerState->HasLocalNetOwner();
}

FText UCoopCharacterProxy::GetName()
{
	if (!IsValid(SwatCharacter))
		return FText();

	return SwatCharacter->GetSwatCharacterName();
}

FText UCoopCharacterProxy::GetFirstName()
{
	if (!IsValid(SwatCharacter))
		return FText();

	URosterCharacter* RosterCharacter = SwatCharacter->GetRosterCharacter();
	if (RosterCharacter)
		return RosterCharacter->FirstName;

	return FText();
}

ETeamType UCoopCharacterProxy::GetTeam()
{
	if (!IsValid(SwatCharacter))
		return ETeamType::TT_NONE;
	
	return SwatCharacter->GetTeam();
}

FText UCoopCharacterProxy::GetStatus()
{
	return StatusFromCharacter(SwatCharacter);
}

float UCoopCharacterProxy::GetHealth()
{
	return HealthFromCharacter(SwatCharacter);
}

FRosterLoadout UCoopCharacterProxy::GetLoadout()
{
	return LoadoutFromCharacter(SwatCharacter);
}

URosterTrait* UCoopCharacterProxy::GetTrait(bool& bIsUnlocked)
{
	if (!IsValid(SwatCharacter) || !IsValid(SwatCharacter->GetRosterCharacter()))
		return nullptr;

	URosterCharacter* RosterCharacter = SwatCharacter->GetRosterCharacter();

	bIsUnlocked = RosterCharacter->bTraitUnlocked;
	return RosterCharacter->Trait;
}

TSoftObjectPtr<UTexture2D> UCoopCharacterProxy::GetImage()
{
	if (!IsValid(SwatCharacter))
		return nullptr;

	UCustomizationCharacter* CustomizationCharacter = Cast<UCustomizationCharacter>(SwatCharacter->Customization.Character);
	return CustomizationCharacter ? CustomizationCharacter->ProfileImage : nullptr;
}

#undef LOCTEXT_NAMESPACE
