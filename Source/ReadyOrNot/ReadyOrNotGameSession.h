// Void Interactive, 2017

#pragma once

//#include "ApiLocality.h"
#include "CoreMinimal.h"
#include "GameFramework/GameSession.h"
#include "OnlineSessionSettings.h"
//#include "ApiMatchmaking.h"
//#include "ApiPayload.h"
#include "ReadyOrNotGameSession.generated.h"

/**
 *
 */
UCLASS(config=Game, BlueprintType)
class READYORNOT_API AReadyOrNotGameSession : public AGameSession
{
	GENERATED_BODY()

	AReadyOrNotGameSession();

	virtual void Tick(float DeltaSeconds) override;
	virtual void RegisterServer() override;

	bool bIsMatchMakeServer = false;


	/** Delegate for creating a new session */
	FOnCreateSessionCompleteDelegate OnCreateSessionCompleteDelegate;
	/** Delegate after starting a session */
	FOnStartSessionCompleteDelegate OnStartSessionCompleteDelegate;
	/** Delegate for destroying a session */
	FOnDestroySessionCompleteDelegate OnDestroySessionCompleteDelegate;
	/** Delegate for searching for sessions */
	FOnFindSessionsCompleteDelegate OnFindSessionsCompleteDelegate;
	/** Delegate after joining a session */
	FOnJoinSessionCompleteDelegate OnJoinSessionCompleteDelegate;

	/** Handles to various registered delegates */
	FDelegateHandle OnStartSessionCompleteDelegateHandle;
	FDelegateHandle OnCreateSessionCompleteDelegateHandle;
	FDelegateHandle OnDestroySessionCompleteDelegateHandle;
	FDelegateHandle OnFindSessionsCompleteDelegateHandle;
	FDelegateHandle OnJoinSessionCompleteDelegateHandle;


	/** Current host settings */
	TSharedPtr<class FOnlineSessionSettings> HostSettings;
	/** Current search settings */
	TSharedPtr<class FOnlineSessionSettings> SearchSettings;

	virtual void OnCreateSessionComplete(FName InSessionName, bool bWasSuccessful);

	FString ModeName = "";

public:

	// FTimerHandle RefreshMatchmakeServer_Handle;
	// UFUNCTION()
	// void RefreshMatchmakeServer();
	//
	// FTimerHandle AddServerToMatchmakeQueue_Handle;
	// UFUNCTION()
	// void AddServerToMatchMakeQueue();
	//
	// UFUNCTION()
	// void UpdateServerMatchmakeQueue();

//	FString MatchmakeIdServer;
//	TArray<FZeuzRegion> CachedZeuzRegions;
//	FTimerHandle ServerMatchmakeQueueUpdate_Handle;

	//UZeuzApiLocality::FDelegateLocalityRegionGet RegionCallback;
	//UFUNCTION()
	//void OnGetRegions(const TArray<FZeuzRegion>& ZeuzRegions, FString Error);

	//UZeuzApiPayload::FDelegatePayloadGet PayloadCreatePartyCallback;
	//UFUNCTION()
	//void OnGetPayloadsMatchmakingCreateParty(const FZeuzPayloadGetOut& PayloadOut, FString Error);

	//UZeuzApiMatchmaking::FDelegateMatchmakingCreateparty CreatePartyCallback;
	//UFUNCTION()
	//void OnMatchmakingCreateParty(const FZeuzMatchMakingStatus& MatchmakingStatus, FString Error);

	//UZeuzApiPayload::FDelegatePayloadGet PayloadServerRefreshCallback;

	//UFUNCTION()
	//void OnGetPayloadServerRefresh(const FZeuzPayloadGetOut& PayloadOut, FString Error);


	//UZeuzApiMatchmaking::FDelegateMatchmakingUpdate MatchmakingCallback;

	UPROPERTY()
	UReadyOrNotSessionData* SessionData = nullptr;
	
	void LoadSessionData();
	void SaveSessionData();

	void AddTeamKill(FString SteamId);



	UFUNCTION(BlueprintCallable, Category = "Host Options")
	void MakeLoadingMapOnlyURL(FString MapURL);
	void UpdateServerDetails(FString MapName, FString GameMode);

	virtual void AddAdmin(APlayerController* AdminPlayer) override;
	virtual void RemoveAdmin(APlayerController* AdminPlayer) override;

	virtual bool BanPlayer(APlayerController* BannedPlayer, const FText& BanReason) override;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = Settings)
		void SetServerSettings(float NewRoundTimerGameStart = 480.0f, float NewRoundTimerBetweenMaps = 30.0f, float NewReinforcementTimer = 30.0f, float NewTimelimit = 900.0f,
			int32 NewRoundsPerMap = 2, int32 NewScorelimit = 150, bool NewAiEnabled = true);

	FString GetConfigFilePath();

	// add any new configs below to this function
	UFUNCTION(BlueprintCallable, Category = Settings)
	void WriteOutConfig();

	int TestSessionChange = 0;


	// the section this class exists in the config
	FString SessionSection = "/Script/ReadyOrNot.ReadyOrNotGameSession";

	// server name
	UPROPERTY(config)
	FString ServerName = "Ready Or Not Dedicated Server";

	// the map list to the server, must include the game you want to run otherwise the default game mode will be run.
	// game modes currently supported (2/10/22)
	UPROPERTY(config)
		TArray<FString> MapList = {};

	// if set to anything except 0 then it will return to the lobby after X missions
	UPROPERTY(config)
	int ReturnToLobbyAfterXMissions = 1;

	// the password to the server (if any)
	UPROPERTY(config)
		FString Password = "";

	// max connections to the server (basically the same as max players)
	UPROPERTY(config)
		int32 MaxConnections = 16;

	// the admin password to check and add new admins with
	UPROPERTY(config)
		FString AdminPassword = "";

	// the ID's of the logged in administrators (so if they have logged in in the past they will still be an administrator if the server restarts
	UPROPERTY(config)
		TArray<FString> LoggedInAdmins;

	// list of steam ids that are banned
	UPROPERTY(config)
		TArray<FString> BanList;

	UPROPERTY(Config)
	int32 MaxTeamKillsBeforeAutoKick = 2;
	
	UPROPERTY(Config)
	int32 MaxTeamKillsBeforeAutoBan = 4;

	UPROPERTY(Config)
	float SecondsUntilKickedForAFK = 0.0f;

	UPROPERTY(Config)
	float SecondsUntilAutostartLobby = -1.0f;

	UPROPERTY(Config)
	int32 MinPlayersForAutostart = 1;

	// how much time players have before the game starts to ready up
	UPROPERTY(config, BlueprintReadOnly)
		float RoundTimerGameStart = 90.0f;

	// how much time players have between maps
	UPROPERTY(config, BlueprintReadOnly)
		float RoundTimerBetweenMaps = 30.0f;

	// how much time between reinforcement waves
	UPROPERTY(config, BlueprintReadOnly)
		float ReinforcementTimer = 30.0f;

	// how much time for respawn if reinforcements isn't used
	UPROPERTY(config, BlueprintReadOnly)
		float RespawnTimer = 8.0f;

	// How much time the server will spend on this map (only applicable in PvP modes)
	UPROPERTY(config, BlueprintReadOnly)
		float Timelimit = 1200.0f;

	// Fraglimit for TDM and FFA modes
	UPROPERTY(config, BlueprintReadOnly)
		int32 Scorelimit = 150;

	// How many rounds per map
	UPROPERTY(config, BlueprintReadOnly)
		int32 RoundsPerMap = 2;

	// Whether or not the AI are enabled
	UPROPERTY(config, BlueprintReadOnly)
		bool bAiEnabled = true;

	UPROPERTY(config, BlueprintReadOnly)
		int32 EventID = 0;

	UPROPERTY(config, BlueprintReadOnly)
		int32 ClientNetSpeed = 250000;

	void RefreshMapList();

	FORCEINLINE void GetRegionAndPayloadID(FString& Region, FString& Payload)
	{
#if PLATFORM_LINUX
		Payload = FLinuxPlatformMisc::GetEnvironmentVariable(TEXT("ZEUZ_PAYLOADID"));
		Region = FLinuxPlatformMisc::GetEnvironmentVariable(TEXT("ZEUZ_REGIONS"));
#endif
	}
};