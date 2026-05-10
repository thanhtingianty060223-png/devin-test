// Void Interactive, 2020

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/IHttpRequest.h"
#include "UObject/Object.h"
#include "ReadyOrNotBackend.generated.h"

UENUM(BlueprintType)
enum class ELoginState : uint8
{
	LS_None,
	LS_LoggingIn,
	LS_LoggedIn,
	LS_LoggedOut,
	LS_LoginFail
};

/**
 * 
 */
UCLASS()
class READYORNOT_API UReadyOrNotBackend : public UObject
{
	GENERATED_BODY()

	UReadyOrNotBackend();

	FTimerHandle TH_TickLoginDelay;
	UFUNCTION()
	void TickLoginDelay();

	float LoginDelay = 0.0f;
	
	UPROPERTY()
	FString SteamId = "";
	
	UPROPERTY()
	FString SteamName = "";

	UPROPERTY()
	FString Ticket = "";

	UPROPERTY()
	FString CachedDiscordOneTimeUseCode = "";

	FString LoginFailedMsg = "";

	UPROPERTY()
	uint8 rgchToken[1024];

	int32 RetryCount = 0;

	UPROPERTY()
	ELoginState LoginState;

	FString GetSteamID();
	FString GetSteamName();
	FString GetEncodedTicket();
	FString BuildEndPoint();
	FString GetApiKey();
	void SetLoginFailed(FString Msg);
public:
	FString GetLoginFailedMessage() { return LoginFailedMsg; }
	bool IsLoggedIn();
	bool IsLoggingIn();
	bool HasLoginFailed();
	ELoginState GetLoginState() { return LoginState; }
	FString GetCachedOneTimeDiscordCode() { return CachedDiscordOneTimeUseCode; }
	void StartLogin();
	FTimerHandle TH_DoLogin;
	UFUNCTION()
	void DoLogin();
	void Logout();
	void GetOneTimeUseDiscordCode();
	void DownloadHashes();
	void RetrieveVivoxLoginToken(FString PlayerName);
	void RetrieveJoinToken(FString PlayerName, FString ChannelName);
	void CheckDLCOwnership(int32 DLC);
	void CheckIsBanned(FString OtherSteamId);
	void CheckIfIAmBanned();

	// Used to indicate a game has started, and how many players are in each game
	UFUNCTION(BlueprintCallable)
	void OnGameStartedMetric(const FString InMap, const FString InGameType, const int32 InNumPlayers);

	// Used to indicate a game has finished, and what the result was
	UFUNCTION(BlueprintCallable)
	void OnGameFinishedMetric(const FString InMap, const FString InGameType, const FString InGameResult);

	// Used to indicate a game has finished, and how each player performed in the level (e.g fps, distance traveled, weapons used)
	UFUNCTION(BlueprintCallable)
	void OnPlayerGameFinishedMetric(const FString InMap, const FString InGameType, float InAverageFps);
		
	UFUNCTION(BlueprintCallable)
	void OnGameCrashedMetric(const FString InState);
	
	void OnMapAnalyticsActorAdded(const FGuid InGameId, int8 InActorId, const AActor* InActor, const TMap<FString, FString>& InProperties = {}) const;
	void OnMapAnalyticsGameStarted(const FGuid InGameId, const int8 InDataVersion, const FString InLevelName, const FString InMode) const;
	void OnMapAnalyticsGameData(const FGuid InGameId, uint32 PacketIndex, bool IsGamedEnded, const FBufferArchive& InData) const;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnCheckedBanStatus, FString, BannedSteamId, bool, bIsBanned, FString, BanReason, bool, bIsMySteamId);
	FOnCheckedBanStatus OnCheckedBanStatus;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStatsStarted);
	UPROPERTY(BlueprintAssignable)
    FOnStatsStarted OnStatsStarted;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStatsUploadProgress, FString, FileName, float, Percentage);
	UPROPERTY(BlueprintAssignable)
	FOnStatsUploadProgress OnStatsUploadProgress;

	FString UploadedStatsName;
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStatsSaved, bool, bWasSuccessful, FString, StatsName);
	UPROPERTY(BlueprintAssignable)
	FOnStatsSaved OnStatsSaved;

	UPROPERTY(BlueprintReadWrite)
	bool bProfileInProgress = false;

	UFUNCTION(BlueprintCallable)
	void StartCapturingProfile();

	FTimerHandle TH_FinishedCapturingProfile;
	UFUNCTION()
	void OnFinishedCapturingProfile();

	static void LogMessage(FString Message, bool bVerbose = false);
private:
	FTimerHandle TH_Heartbeat;
	void Heartbeat();
	void OnLogin_ResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void OnLogout_ResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void OnOneTimeUseDiscordCode_ResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void OnDownloadedHashes_ResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void OnVivoxLoginToken_ResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void OnJoinToken_ResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void OnHeartbeat_ResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void OnOwnsDLC_ResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void OnIsBanned_ResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void OnStatsSaved_ResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void OnStatsProgress(FHttpRequestPtr Request, int32 SentBytes, int32 ReceivedBytes);
	void DoPostGameMetric(const EGameEventMetric InEventType, TSharedRef<FJsonObject> InObj);
};
