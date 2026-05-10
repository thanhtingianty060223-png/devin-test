// Copyright Void Interactive, 2017

#include "FriendsList.h"
#include "OnlineSubsystem.h"
#if defined(TARGET_SONY)
#include "OnlineSubsystemSonyTypes.h"
#endif
#if defined(TARGET_XBOX)
#include "OnlineSubsystemGDKTypes.h"
#endif
#include "AdvancedSteamFriendsLibrary.h"
#include "ReadyOrNot.h"

DEFINE_LOG_CATEGORY(FriendsListLog);

void UFriendsList::NativeConstruct()
{
	Super::NativeConstruct();
}

FBPUniqueNetId UFriendsList::CreateUniqueNetIdFromString(const FString PlatformId)
{
	FBPUniqueNetId netId;

#if defined(TARGET_SONY)
	if (PlatformId.Len() > 0)
	{
		auto id = FUniqueNetIdSony::FromString(PlatformId);
		TSharedPtr<const FUniqueNetId> ValueID(id);
		netId.SetUniqueNetId(ValueID);
	}
	return netId;
#elif defined(TARGET_XBOX)
	if (PlatformId.Len() > 0)
	{
		auto id = FUniqueNetIdGDK::Create(PlatformId);
		TSharedPtr<const FUniqueNetId> ValueID(id);
		netId.SetUniqueNetId(ValueID);
	}
	return netId;
#elif PLATFORM_WINDOWS || PLATFORM_MAC || PLATFORM_LINUX
	if (!(PlatformId.Len() > 0))
	{
		UE_LOG(FriendsListLog, Warning, TEXT("Steam - CreateUniqueNetIdFromString Had a bad UniqueNetId!"));
		return netId;
	}

	if (SteamAPI_Init())
	{
		TSharedPtr<const FUniqueNetId> ValueID(new const FUniqueNetIdSteam2(PlatformId));
		netId.SetUniqueNetId(ValueID);
		return netId;
	}
#endif
	return netId;
}

void UFriendsList::GetFriendsList()
{
	if (GEngine == NULL) return; if (GEngine->GameViewport == NULL) return;
	UWorld* world = GEngine->GameViewport->GetWorld();
	IOnlineFriendsPtr Friends = Online::GetFriendsInterface();
	if (Friends.IsValid() && world)
	{
		AReadyOrNotPlayerController* pc = Cast<AReadyOrNotPlayerController>(UGameplayStatics::GetPlayerController(world, 0));
		ULocalPlayer* Player = pc ? Cast<ULocalPlayer>(pc->Player) : nullptr;
		if (Player)
		{
			Friends->ReadFriendsList(Player->GetControllerId(), EFriendsLists::ToString(EFriendsLists::Default), FOnReadFriendsListComplete::CreateUObject(this, &UFriendsList::OnReadFriendsListComplete));
		}
	}
	else
	{
		OnFailure.Broadcast();
	}
}

void UFriendsList::OnReadFriendsListComplete(int32 LocalUserNum, bool bWasSuccessful, const FString& ListName, const FString& ErrorStr)
{
	//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::White, "Read Friends List");
	if (bWasSuccessful)
	{
		if (!GEngine)
			return;

		if (!GEngine->GameViewport)
			return;

		FriendsList.Empty();

		UWorld* world = GEngine->GameViewport->GetWorld();
		const IOnlineFriendsPtr Friends = Online::GetFriendsInterface();
		if (Friends.IsValid() && world)
		{
			TArray<TSharedRef<FOnlineFriend>> OutFriends;
			Friends->GetFriendsList(LocalUserNum, ListName, OutFriends);
			
			for (int32 i = 0; i < OutFriends.Num(); i++)
			{
				const TSharedRef<FOnlineFriend> Friend = OutFriends[i];
				FFriend BPFriend = FFriend();

				BPFriend.Presence = EOnlinePresenceState::ToLocText(Friend->GetPresence().Status.State).ToString();
				BPFriend.StatusString = Friend->GetPresence().Status.StatusStr;
				BPFriend.StatusState = (int32)Friend->GetPresence().Status.State;
#if defined(TARGET_CONSOLE)
				if (Friend->GetPresence().bIsPlayingThisGame) 
				{
					BPFriend.StatusString = "Playing Ready Or Not";
				}
#else 
				if (BPFriend.StatusString.IsEmpty())
				{
					BPFriend.StatusString = "Playing Ready Or Not";
				}
#endif
				BPFriend.DisplayName = Friend->GetDisplayName();
				BPFriend.RealName = Friend->GetRealName();
				BPFriend.UniqueNetId = Friend->GetUserId()->ToString();
				BPFriend.bRunningSameGame = Friend->GetPresence().bIsPlayingThisGame;
				BPFriend.bHasVoice = Friend->GetPresence().bHasVoiceSupport;
				BPFriend.bJoinable = Friend->GetPresence().bIsJoinable;
				
				if (BPFriend.bJoinable)
				{
					FriendsList.Insert(BPFriend, 0);
				}
				else if (BPFriend.bRunningSameGame)
				{
					FriendsList.Add(BPFriend);
				}
			}

			OnSuccess.Broadcast();
			return;
		}
	}
	
	OnFailure.Broadcast();
}
