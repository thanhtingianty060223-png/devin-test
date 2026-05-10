// Copyright Void Interactive, 2023

#include "HUD/Widgets/Console/PlayersList.h"

#include "Interfaces/OnlineSessionInterface.h"
#include "Interfaces/OnlineUserInterface.h"
#include "Interfaces/OnlinePresenceInterface.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineSubsystemTypes.h"
#include "ReadyOrNot.h"

void UPlayersList::GetPlayersList() 
{
	FOnlineSubsystemBPCallHelperAdvanced Helper(TEXT("LobbySessionCreateCompleted"), GetWorld());
	const IOnlinePresencePtr Presence = Helper.OnlineSub->GetPresenceInterface();
	if (GetWorld()->GetGameState())
	{
		PlayersList.Empty();
		for (int i = 0; i < GetWorld()->GetGameState()->PlayerArray.Num(); i++)
		{
			APlayerState* PlayerState = GetWorld()->GetGameState()->PlayerArray[i];
			if (PlayerState->GetUniqueId().IsValid())
			{
				FLobbyPlayer player;
				player.DisplayName = PlayerState->GetPlayerName();
				player.UniqueNetId = PlayerState->GetUniqueId()->ToString();
				player.bIsMuted = PlayerState->GetUniqueId().IsValid() ? GetMutedState(PlayerState->GetUniqueId()->ToString()) : false;

				if (Presence.IsValid())
				{
					TSharedPtr<FOnlineUserPresence> OutPresence;
					auto CacheResult = Presence->GetCachedPresence(*PlayerState->GetUniqueId(), OutPresence);
					if (CacheResult == EOnlineCachedResult::Success)
					{
						player.StatusString = OutPresence->Status.StatusStr;
					}
					else
					{
						//FOnPresenceTaskCompleteDelegate myDelegate;
						Presence->QueryPresence(*PlayerState->GetUniqueId(), nullptr);
					}
				}

				PlayersList.Emplace(player);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Inalid player in PlayersList %s"), *PlayerState->GetPlayerName())
			}
		}

		OnSuccess.Broadcast();
		return;
	}
	OnFailure.Broadcast();
}

APlayerState* UPlayersList::GetPlayerStateFromUniqueId(FString UniqueId)
{
        const UWorld *World = GetWorld();
	if (!World)
		return nullptr;
	
        const AGameStateBase *GameStateBase = World->GetGameState();
	if(!GameStateBase || !GameStateBase->PlayerArray.Num())
		return nullptr;
	
	for (int i = 0; i < GameStateBase->PlayerArray.Num(); i++)
	{
		if (APlayerState* PlayerState = GameStateBase->PlayerArray[i])
		{
			FUniqueNetIdRepl PlayerStateUniqueId = PlayerState->GetUniqueId();

			if (PlayerStateUniqueId.IsValid() && PlayerStateUniqueId->ToString() == UniqueId)
			{
				return PlayerState;
			}
		}
	}
	return nullptr;
}

bool UPlayersList::GetMutedState(const FString UniqueNetId)
{
	UReadyOrNotGameInstance* GameInstance = GetWorld()->GetGameInstance<UReadyOrNotGameInstance>();
	if (GameInstance) 
	{
		return GameInstance->GetMutedState(UniqueNetId);
	}
	return false;
}

void UPlayersList::SetMutedState(const FString UniqueNetId, bool value)
{
	for (int i = 0; i < PlayersList.Num(); i++) 
	{
		if (PlayersList[i].UniqueNetId == UniqueNetId)
		{
			PlayersList[i].bIsMuted = value;
		}
	}
	UReadyOrNotGameInstance* GameInstance = GetWorld()->GetGameInstance<UReadyOrNotGameInstance>();
	if (GameInstance)
	{
		UReadyOrNotGameInstance::FDelegateSetLocalMutedStateCompleted localMuteDelegate;
		localMuteDelegate.RemoveAll(this);
		localMuteDelegate.AddDynamic(this, &UPlayersList::UpdatedMutedState);
		GameInstance->SetMutedState(UniqueNetId, value, localMuteDelegate);
	}
}

void UPlayersList::UpdatedMutedState()
{
	OnMuteStateDelegate.Broadcast();
}

bool UPlayersList::VivoxAvailable()
{
#if defined(WITH_VIVOX)
	return true;
#else
	return false;
#endif
}
