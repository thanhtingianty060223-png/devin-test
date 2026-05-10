// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ConsoleMultiplayerSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogReadyOrConsoleMultiplayer, Log, All);

UCLASS()
class READYORNOT_API UConsoleMultiplayerSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual ETickableTickType GetTickableTickType() const override
	{
		return ETickableTickType::Always;
	}

	virtual TStatId GetStatId() const override
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT( UConsoleMultiplayerSubsystem, STATGROUP_Tickables );
	}

	virtual bool IsTickableWhenPaused() const
	{
		return true;
	}
	virtual bool IsTickableInEditor() const
	{
		return false;
	}
	
	virtual void Tick( float DeltaTime ) override;	
private:
	bool bInitialized = false;

	int MaxPlayers = 5;

	uint32 LastFrameNumberWeTicked = INDEX_NONE;

	void UpdateAvailableSlots(int numSlots);
	void JoinPublicSession();
	void FindSessionsOnComplete(bool bWasSuccessful);
	
	TSharedPtr<FOnlineSessionSearch> SearchObject;
	
	FOnCreateSessionCompleteDelegate LobbySessionCreateCompleteDelegate;
	FOnStartSessionCompleteDelegate LobbySessionStartCompleteDelegate;
	FOnSessionParticipantsChangeDelegate LobbySessionParticipantsChangeDelegate;
	FOnUnregisterPlayersCompleteDelegate LobbySessionUnregisterPlayersDelegate; 
	FOnRegisterPlayersCompleteDelegate LobbySessionRegisterPlayersDelegate;
	
	FDelegateHandle LobbySessionCreateCompleteDelegateHandle;
	FDelegateHandle LobbySessionStartCompleteDelegateHandle;
	FDelegateHandle LobbySessionParticipantsChangeDelegateHandle;
	FDelegateHandle LobbySessionUnregisterPlayersDelegateHandle;
	FDelegateHandle LobbySessionRegisterPlayersDelegateHandle;
	
	void LobbySessionCreate(int /*numSlots*/);
	void LobbySessionOnCreateCompleted(FName /*SessionName*/, bool /*bWasSuccessful*/);
	void LobbySessionOnStartCompleted(FName /*SessionName*/, bool /*bWasSuccessful*/);
	void LobbySessionParticipantsChange(FName, const FUniqueNetId&, bool);
	void LobbySessionUnregisterPlayers(FName, const TArray< FUniqueNetIdRef >&, bool);
	void LobbySessionRegisterPlayers(FName, const TArray< FUniqueNetIdRef >&, bool);
	
	/* LOBBYSESSION - CLIENT */
	void LobbySessionOnJoinCompleted(FName /*SessionName*/, EOnJoinSessionCompleteResult::Type /*Result*/);
	
	FOnJoinSessionCompleteDelegate LobbySessionOnJoinDelegate;
	FDelegateHandle LobbySessionOnJoinDelegateHandle;
	
	FOnSingleSessionResultCompleteDelegate FindPartySessionByIdDelegate;
	FDelegateHandle FindPartySessionByIdDelegateHandle;
	
	// FOnSingleSessionResultCompleteDelegate
	void FindPartySessionByIdCompleted(int32, bool, const FOnlineSessionSearchResult&);
	
	/* PLAYERSESSION */
	FOnCreateSessionCompleteDelegate PlayerSessionCreateCompleteDelegate;
	FOnStartSessionCompleteDelegate PlayerSessionStartCompleteDelegate;
	FOnSessionParticipantsChangeDelegate PlayerSessionParticipantsChangeDelegate;
	
	FDelegateHandle PlayerSessionCreateCompleteDelegateHandle;
	FDelegateHandle PlayerSessionStartCompleteDelegateHandle;
	FDelegateHandle PlayerSessionParticipantsChangeDelegateHandle;
	
	void PlayerSessionOnCreateCompleted(FName SessionName, bool bWasSuccessful);
	void PlayerSessionOnStartCompleted(FName SessionName, bool bWasSuccessful);
	
	/* MATCHMAKING */
	void SonyMatchmakingHostOnComplete(FName /*SessionName*/, const struct FOnlineError& /*ErrorDetails*/, const struct FSessionMatchmakingResults& /*Results*/);
	void SonyMatchmakingClientOnComplete(FName /*SessionName*/, const struct FOnlineError& /*ErrorDetails*/, const struct FSessionMatchmakingResults& /*Results*/);
	
	FOnStartMatchmakingComplete SonyMatchmakingHostOnCompleteDelegate;
	FOnStartMatchmakingComplete SonyMatchmakingClientOnCompleteDelegate;
	
	FDelegateHandle SonyMatchmakingStartMatchmakingCompleteHostDelegate;
	FDelegateHandle SonyMatchmakingClientOnCompleteDelegateHandle;
public:
	void FindPublicPlayers(int numSlots);
	void PlayerSessionCreate();
	void StartMatchMaking(bool bIsHost);
	void UpdateSessionConnectString();
	int GetMaxPlayers();
	void SetMaxPlayers(int maxPlayers);
};


UCLASS()
class READYORNOT_API UConsoleMultiplayerStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

private:
	static UConsoleMultiplayerSubsystem* GetConsoleMultiplayerSubsystem(UObject* WorldContextObject);
	
public:
	static void FindPublicPlayers(UObject* WorldContextObject, int numSlots);
	static void PlayerSessionCreate(UObject* WorldContextObject);
	static void StartMatchMaking(UObject* WorldContextObject, bool bIsHost);
	static void UpdateSessionConnectString(UObject* WorldContextObject);
	static int GetMaxPlayers(UObject* WorldContextObject);
	static void SetMaxPlayers(UObject* WorldContextObject, int maxPlayers);
};