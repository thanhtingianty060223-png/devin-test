// Copyright Void Interactive, 2024

#pragma once

PRAGMA_DISABLE_DEPRECATION_WARNINGS

#include "AdvancedFriendsGameInstance.h"

#if defined(WITH_STEAM)
#include "Steam/isteammatchmaking.h"
#endif

#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "IPlatformFilePak.h"
#include "NetworkReplayStreaming.h"
#include "Engine/StreamableManager.h"
//#include "Channel3DProperties.h"

UENUM()
enum class EAudioFadeModel : uint8
{
	InverseByDistance = 0,
	LinearByDistance,
	ExponentialByDistance
};

#include "Data/LevelData.h"
#include "Info/ReadyOrNotBackend.h"
#include "Templates/SharedPointer.h"
#include "ReadyOrNotGameInstance.generated.h"
PRAGMA_ENABLE_DEPRECATION_WARNINGS


#ifndef RON_DEMO
#define STEAM_APP_ID STEAM_APPID_CORE_GAME
#else
#define STEAM_APP_ID STEAM_APPID_DEMO_GAME
#endif

#define ENABLE_ANTI_PIRACY_CHECKS 0

template<class T>
class CachedProperty
{
public:
	explicit CachedProperty(T value) {
		m_dirty = false;
		m_value = value;
	}

	const T &GetValue() const {
		return m_value;
	}

	void SetValue(const T &value) {
		if(m_value != value) {
			m_value = value;
			m_dirty = true;
		}
	}

	void SetDirty(bool value) {
		m_dirty = value;
	}

	bool IsDirty() const {
		return m_dirty;
	}
protected:
	bool m_dirty;
	T m_value;
};

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="VOIP Settings"))
class READYORNOT_API UReadyOrNotVoipSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UReadyOrNotVoipSettings() {}
	
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="VOIP Settings")
	int32 AudibleDistance = 8100;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="VOIP Settings")
	int32 ConversationalDistance = 270;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="VOIP Settings")
	float FadeIntensityByDistance = 1.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="VOIP Settings")
	EAudioFadeModel FadeModel = EAudioFadeModel::InverseByDistance;
};

UENUM(BlueprintType)
enum class PTTKey : uint8
{
	PTTNoChannel,
	PTTAreaChannel,
	PTTTeamChannel
};

USTRUCT(BlueprintType)
struct FChannelInfo
{
	GENERATED_USTRUCT_BODY()
	
#if defined(WITH_VIVOX)
	ChannelType ChannelType;
#endif
	bool bShouldTransmitOnJoin;
	PTTKey AssignChannelToPTTKey;
	FString ChannelName;
};

UENUM(BlueprintType)
enum class EHighScoreCategory : uint8
{
	HSC_None,
	HSC_COOP_DAILY,
	HSC_COOP_SEASON,
	HSC_PVP_DAILY,
	HSC_PVP_SEASON

};

UENUM(BlueprintType)
enum class ELastMenuStateBeforeJoin : uint8
{
	LM_None,
	LM_ServerBrowser,
	LM_FromFriends,
	LM_SinglePlayer
};

USTRUCT()
struct FPMap
{
	GENERATED_USTRUCT_BODY()
	UPROPERTY()
	TArray<FString> BLPN;
	UPROPERTY()
	TArray<FString> BLPHZ;
	UPROPERTY()
	TArray<FString> BLWT;
	UPROPERTY()
	TArray<FString> BLDLLHZ;
	UPROPERTY()
	TArray<FString> BLDLLN;
};

UENUM(BlueprintType)
enum EReplayEventType
{
	PlayerKilled      UMETA(DisplayName = "Player Killed"),
	SwatKilled      UMETA(DisplayName = "SWAT Killed"),
	SuspectKilled   UMETA(DisplayName = "Suspect Killed"),
	CivilianArrested   UMETA(DisplayName = "Civilian Killed"),
	EvidenceCollected   UMETA(DisplayName = "Evidence Collected")
};

USTRUCT(BlueprintType)
struct FReplayEvent
{
	GENERATED_USTRUCT_BODY()
	
	UPROPERTY(BlueprintReadOnly)
	TEnumAsByte<EReplayEventType> EventType;

	UPROPERTY(BlueprintReadOnly)
	FVector Location;

	UPROPERTY(BlueprintReadOnly)
	FString AdditionalInformation;

	UPROPERTY(BlueprintReadOnly)
	float Timestamp;
	
	FReplayEvent(TEnumAsByte<EReplayEventType> NewEventType, FVector NewLocation, float NewTimestamp, FString NewAdditionalInformation)
	{
		EventType = NewEventType;
		Location = NewLocation;
		Timestamp = NewTimestamp;
		AdditionalInformation = NewAdditionalInformation;
	}

	FReplayEvent()
	{
		EventType = PlayerKilled;
		Location = FVector();
		Timestamp = 0.0f;
		AdditionalInformation = "";
	}
};

USTRUCT(BlueprintType)
struct FReplayData
{
	GENERATED_USTRUCT_BODY()
	
	UPROPERTY(BlueprintReadOnly)
	float ReplayLength;

	UPROPERTY(BlueprintReadOnly)
	FString LetterScore;

	UPROPERTY(BlueprintReadOnly)
	int32 NumericalScore;

	UPROPERTY(BlueprintReadOnly)
	int32 NumPlayers;

	UPROPERTY(BlueprintReadOnly)
	FString LevelName;

	UPROPERTY(BlueprintReadOnly)
	FString LevelRowName;

	UPROPERTY(BlueprintReadOnly)
	FString GamemodeName;
	
	UPROPERTY(BlueprintReadOnly)
	FString Timestamp;

	UPROPERTY(BlueprintReadOnly)
	FString GameVersion;


	UPROPERTY(BlueprintReadOnly)
	TArray<FReplayEvent> ReplayEvents;
	
	FReplayData(float NewReplayLength, FString NewLetterScore, int32 NewNumericalScore, int32 NewNumPlayers, FString NewLevelName, FString NewGamemodeName, FString NewLevelRowName, FString NewTimestamp, FString NewGameVersion, TArray<FReplayEvent> NewReplayEvents)
	{
		ReplayLength = NewReplayLength;
		LetterScore = NewLetterScore;
		NumericalScore = NewNumericalScore;
		NumPlayers = NewNumPlayers;
		LevelName = NewLevelName;
		GamemodeName = NewGamemodeName;
		LevelRowName = NewLevelRowName;
		Timestamp = NewTimestamp;
		GameVersion = NewGameVersion;
		ReplayEvents = NewReplayEvents;
	}

	FReplayData()
	{
		ReplayLength = 0.f;
		LetterScore = "-";
		NumericalScore = 0;
		NumPlayers = 0;
		LevelName = "-";
		GamemodeName = "-";
		LevelRowName = "";
		Timestamp = "-";
		GameVersion = "-";
		ReplayEvents = TArray<FReplayEvent>();
	}
};


#if ENABLE_ANTI_PIRACY_CHECKS
class READYORNOT_API FCheckForSteamEmuThreadWorker : public FRunnable
{	
public:
	class UReadyOrNotGameInstance* GameInstanceReference;

	FCheckForSteamEmuThreadWorker(class UReadyOrNotGameInstance* GameInstance);
	virtual ~FCheckForSteamEmuThreadWorker() override;

	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;
	virtual void StopInstantly();
	
	void EnsureCompletion();

private:
	FRunnableThread* Thread;
	FThreadSafeCounter StopTaskCounter;
};
#endif

class FReadyOrNotBackendOutputDevice : public FStringOutputDevice
{
public:
	virtual void Serialize( const TCHAR* InData, ELogVerbosity::Type Verbosity, const class FName& Category ) override
	{
		return;
		bool bShouldLog = Verbosity == ELogVerbosity::Error || Verbosity == ELogVerbosity::Fatal;
		FString Msg = InData;


		if (Category == "LogPlayerController")
			bShouldLog = false;

		if (Category == "LogMaterial")
			bShouldLog = false;

		if (Category == "LogFMOD")
			bShouldLog = false;

		if (Category == "LogDemo")
			bShouldLog = false;

		if (Category == "LogShaders")
			bShouldLog = false;

		if (Category == "LogHttpReplay")
			bShouldLog = false;

		if (Msg.Contains("ChannelSession::Set3DPosition"))
			bShouldLog = false;

		if (Msg.Contains("Warning SteamNetworkingSockets"))
			bShouldLog = true;

		if (Category == "LogNetPackageMap" && Verbosity == ELogVerbosity::Warning)
			bShouldLog = true;

		if (Category == "LogNet" && Verbosity == ELogVerbosity::Warning)
			bShouldLog = true;

		if (Msg.Contains("No owning connection for actor"))
			bShouldLog = false;

		if (Category == "LogReadyOrNotLoadout")
			bShouldLog = true;

		if (Msg.Contains("Host Migration:"))
			bShouldLog = true;

		if (!bShouldLog)
			return;

		Msg = Category.ToString() + " " + InData;
		if (Verbosity == ELogVerbosity::Error)
		{
			Msg = "Error: " + Msg;
		} else if (Verbosity == ELogVerbosity::Fatal)
		{
			Msg = "Fatal: " + Msg;
		}
		if (!Msg.IsEmpty())
		{
			UReadyOrNotBackend::LogMessage(Msg, true);
		}
		
	}
};

UCLASS(Config = Game)
class READYORNOT_API UReadyOrNotGameInstance : public UAdvancedFriendsGameInstance
#if defined(WITH_STEAM)
	, public ISteamMatchmakingPingResponse
#endif
{
	GENERATED_BODY()

public:
	UReadyOrNotGameInstance(const FObjectInitializer& ObjectInitializer);
	~UReadyOrNotGameInstance();

	UPROPERTY(Config, BlueprintReadOnly)
	FString LobbyLevel = "ron_station_core";

	UPROPERTY(Config, BlueprintReadOnly)
	FString TrainingLevel = "ron_tutorialv3_core";

	UPROPERTY(EditAnywhere)
	TSubclassOf<class ULoadoutManager> LoadoutManagerClass;
	
	UFUNCTION()
	bool OnWindowCloseRequested();
	FReadyOrNotBackendOutputDevice* ReadyOrNotLogOutput;
	
	UPROPERTY(BlueprintReadOnly)
	class UReadyOrNotBackend* ReadyOrNotBackend = nullptr;

	UPROPERTY()
	class ULoadoutManager* LoadoutManager = nullptr;
	
	UPROPERTY()
	class UModioManager* ModioManager = nullptr;
	
	UPROPERTY()
	class UReadyOrNotAIConfig* AIConfig = nullptr;

	// GDK
    FOnLoginCompleteDelegate OnNativeLoginCompleteDelegate;
    FDelegateHandle OnNativeLoginCompleteDelegateHandle;

    void NativeLoginComplete(int32 /*LocalUserNum*/, bool /*bWasSuccessful*/, const FUniqueNetId& /*UserId*/, const FString& /*Error*/);

	// EOS
	FOnLoginCompleteDelegate OnEOSLoginCompleteDelegate;
	FDelegateHandle OnEOSLoginCompleteDelegateHandle;

	void EOSLoginComplete(int32 /*LocalUserNum*/, bool /*bWasSuccessful*/, const FUniqueNetId& /*UserId*/, const FString& /*Error*/);

#if defined(WITH_MODIO)
	void EnableModManager();
	void DisableModManager();
	
	void MountInstalledMods();
#endif

private:
	FPakPlatformFile* PakPlatform;

public:
	FPakPlatformFile* GetPakPlatform() { return PakPlatform; }

	bool bWantsToRestartGameExe = false;

	UPROPERTY(EditAnywhere)
	UMaterialParameterCollection* GlobalMaterialParameterCollection;
	
	UPROPERTY(EditAnywhere)
	UMaterialParameterCollection* WeaponFOVMaterialCollection;
	
	bool bHasEverAuthenticatedThisGame = false;

	virtual void Init() override;
	virtual void StartGameInstance() override;
	virtual ULocalPlayer* CreateInitialPlayer(FString& OutError) override;
	virtual void Shutdown() override;

	void OnPreExit();
	
	bool OnUnmountPak(const FString& PakFile);

	UPROPERTY()
	class UMetaGameProfile* MetaGameProfile;
	
	UPROPERTY(BlueprintReadOnly)
	ESessionType SessionType;

	UFUNCTION(BlueprintCallable)
	bool IsPublicMissionInProgress();

	// Store Ptr to lazy loaded classes, wipe it out when we change maps
	// These are blueprint loaded so they might lose refs
	UPROPERTY()
	TArray<TSoftClassPtr<UClass>> LazyLoadedClasses;

	// Store Ptr to lazy loaded objects, wipe it out when we change maps
	// these are blueprint loaded so they might lose refs
	UPROPERTY()
	TArray<TSoftObjectPtr<UObject>> LazyLoadedObjects;

	// wipe out any load sync objects
	void OnPostWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
	void OnWorldInitalized(const UWorld::FActorsInitializedParams& Params);
	void OnPreWorldInitialization(UWorld* World, const UWorld::InitializationValues IVS);
	void OnLevelChanged(ULevel* Level, UWorld* World);
	void PostLoadMap(UWorld* World);

	void TryGenerateWorld();

	void ApplyWorldSettings();

	void OnWorldBeginPlay();

	UFUNCTION(BlueprintCallable)
	void ApplyDecalSettings();
	
	UFUNCTION(BlueprintPure)
	static bool IsGameModded() { return bIsModded; }
	
	UFUNCTION(BlueprintCallable)
	bool IsSafeMode() const;

	UFUNCTION(BlueprintCallable)
	bool SupportsDisablingMods() const;
	
	/**
	 * @brief Build the checksum of all mods, if any and load any custom data in mods
	 * */
	void BuildChecksum();

	/**
	 * @brief Loads any custom data from a mod, if we know about it (e.g maps, AI Data)
	 */
	void LoadModDataFromPak(const FString& InPakFile);

	void LoadModdedLevelData();
	
	int32 Checksum;

	static bool bIsModded;
	static bool bNoScoring;
	
	UPROPERTY(BlueprintReadWrite, Category = UI)
	TSubclassOf<UUserWidget> SpawnWidgetOnLevelLoad;

	UFUNCTION(BlueprintCallable, Category = Steam)
	FString GetSessionTicket();

	#if ENABLE_ANTI_PIRACY_CHECKS
	void CheckForSteamEmus();
	#endif

	bool bWantsRunPThread = true;

	static bool bIsBuildPirated;

	UFUNCTION(BlueprintPure)
	bool IsLoggedIntoBackend() const;

	UFUNCTION(BlueprintPure)
	uint8 GetBackendState() const;

	UFUNCTION(BlueprintCallable)
	void RetryLogin();

	// UPROPERTY()
	// FPMap Hashes;
	//
	// void LoadHashMap();
	// void SaveHashMap();


	UPROPERTY()
	TMap<int32, bool> OwnedDLCMap;

	UPROPERTY()
	TArray<AActor*> DecalMeshActors;

	// VIVOX IMPLEMENTATION
#if defined(WITH_VIVOX)
    void BindLoginSessionHandlers(bool DoBind, ILoginSession& LoginSession);
    void BindChannelSessionHandlers(bool DoBind, IChannelSession& ChannelSession);

    VivoxCoreError Initialize(int logLevel);
    void Uninitialize();

    void StartVivoxLogin();

	void DoVivoxLogin(FString Token);
	
    void Logout();
#endif

	bool JoinVoiceChannels(FString OnlineSessionId, int32 TeamNum = -1);

#if defined(WITH_VIVOX)
	VivoxCoreError Join(ChannelType ChannelType, bool ShouldTransmitOnJoin, const FString& ChannelName, PTTKey AssignChanneltoPTTKey=PTTKey::PTTNoChannel, FString Token = "");

	TMap<FString, FChannelInfo> PendingChannelJoin;
#endif

	UFUNCTION(BlueprintPure)
	FString GetDiscordOneTimeUseCode();

#if defined(WITH_VIVOX)
	void OnJoinTokenReceived(FString ChannelName, FString Token);
#endif

    void LeaveVoiceChannels();
    void Update3DPosition(APawn* Pawn);

#if defined(WITH_VIVOX)
	void OnLoginSessionStateChanged(LoginState State);
    void OnChannelParticipantAdded(const IParticipant& Participant);
    void OnChannelParticipantRemoved(const IParticipant& Participant);
    void OnChannelParticipantUpdated(const IParticipant& Participant);
    void OnChannelAudioStateChanged(const IChannelConnectionState& State);
    void OnChannelTextStateChanged(const IChannelConnectionState& State);
    void OnChannelStateChanged(const IChannelConnectionState& State);
    void OnChannelTextMessageReceived(const IChannelTextMessage& Message);
#endif

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDelegateSetLocalMutedStateCompleted);
	bool GetMutedState(FString UniqueNetId);
	void SetMutedState(FString UniqueNetId, bool value, FDelegateSetLocalMutedStateCompleted Delegate);

    virtual bool IsInitialized();
    virtual bool IsLoggedIn();

    bool MultiChanPushToTalk(PTTKey Key, bool PTTKeyPressed);
    bool MultiChanToggleChat(PTTKey Key);

#if defined(WITH_VIVOX)
    ILoginSession *GetLoginSessionForRoster();
    TSharedPtr<IChannelSession> GetChannelSessionForRoster();
    ChannelId GetLastKnownTransmittingChannel() { return LastKnownTransmittingChannel; }
    static FString GetVivoxSafePlayerName(FString BaseName);
#endif

	FString CurrentlyRecordingReplayName;

public:
    bool bInitialized;

    bool bVivoxLoggedIn;
    bool bVivoxLoggingIn;

private:
#if defined(WITH_VIVOX)
    IClient* VivoxVoiceClient;
    AccountId LoggedInAccountID;
    FString LoggedInPlayerName;

    TPair<ChannelId, bool> PTTAreaChannel;
    TPair<ChannelId, bool> PTTTeamChannel;
    ChannelId ConnectedPositionalChannel; // You can only be in one Positional channel at a time.
    ChannelId LastKnownTransmittingChannel;
#endif

    /// Cached 3D position and orientation
    CachedProperty<FVector> CachedPosition = CachedProperty<FVector>(FVector());
    CachedProperty<FVector> CachedForwardVector = CachedProperty<FVector>(FVector());
    CachedProperty<FVector> CachedUpVector = CachedProperty<FVector>(FVector());

	

    /// Privates methods to check and clear dirtiness of cached 3D position
    bool Get3DValuesAreDirty() const;
    void Clear3DValuesAreDirty();
	// END VIVOX

	
public:

	// Track this in the game instance so that we don't try join previously joined games
	// string is the game owners steam id
	UPROPERTY()
	TArray<FString> PreviouslyJoinedGames = {};

	// Tracked and recorded so that we can return to the lobby after X games
	UPROPERTY()
	int32 DedicatedServerGamesPlayedWithoutReturningToLobby = 0;

	// always starts at 0 and increments after each game so we can run through the map list
	UPROPERTY()
	int32 DedicatedServerMapIdx = 0;

	UFUNCTION(BlueprintPure)
	bool GetAvailableAudioDevices(TArray<FString>& OutAudioDevices);

	UFUNCTION(BlueprintCallable)
	bool SetInputAudioDevice(FString DeviceName, bool bShouldSave = true);

	void SetInputVolume(float InputVolume);
	void SetOutputVolume(float OutputVolume);

	bool bClientHasFinishedLoading = false;
	// don't remove the loading screen until host has finished loading
	bool bPendingRemoveLoadingScreenAfterHostLoaded = false;

	UFUNCTION()
        uint32 GetLocalNetworkVersion();
	UFUNCTION()	
        bool IsNetworkCompatible( const uint32 LocalNetworkVersion, const uint32 RemoteNetworkVersion );

	FStreamableManager StreamableManager;
	static FStreamableManager& GetStreamableManager();

	// stored here to be persistent no matter what the server does (ie. respawning the player controller)
	bool bForceShowMouseCursor = false;
	
	// used to close/open server matchmake queue for backfill
	FTimerHandle ServerMatchmakeQueueUpdate_Handle;

	// Gratr all text elements. There's no going back
	UFUNCTION(Exec)
	void Gratr();
	void Gratr_Everything();

	FTimerHandle TH_Gratr;

	UFUNCTION(BlueprintCallable)
	TArray<class AReadyOrNotGameMode*> GetAllGameModes();
	
	UFUNCTION(BlueprintCallable)
	TArray<class AReadyOrNotGameState*> GetAllGameStates();

	// link a url ie (BS_COOP to a mode name Barricaded Suspects)
	TMap<FString, FString> UrlToModeNameMap;
	
	UFUNCTION()
	void GenerateURLMap();

	void SetPresenceForLocalPlayers(const FString& StatusStr, FOnlineKeyValuePairs<FPresenceKey, FVariantData> PresenceProperties);
	
	void OnWorldPresave(uint32 Id, UWorld* World);

	virtual void LoadComplete(const float LoadTime, const FString& MapName) override;

	UFUNCTION(BlueprintCallable, Exec)
	void ConnectSteamServer(FString serverConnect);
#if defined(WITH_STEAM)
	// Begin ISteamMatchmakingPingResponse interface
	virtual void ServerResponded(gameserveritem_t& server) override;
	virtual void ServerFailedToRespond() override;
	// End ISteamMatchmakingPingResponse interface
#endif
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameServerPinged, FString, ConnectionAddress);
	UPROPERTY(BlueprintAssignable)
	FOnGameServerPinged OnConnectSteamServerByIP;

	// Unique id of teammate that we want to join onto their team
	FString  TeamUniqueNetId;

	// bound to connect ^ above on game thread
	UFUNCTION()
	void OnConnectSteamServer(FString url);

	TSharedPtr<class FOnlineSessionSearch> SessionSearch;

	FOnFindSessionsCompleteDelegate OnFindSessionsCompleteDelegate;
	FDelegateHandle OnFindSessionsCompleteDelegateHandle;
	void OnFindSessionsComplete(bool bWasSuccessful);

	FOnJoinSessionCompleteDelegate OnJoinSessionCompleteDelegate;
	FDelegateHandle OnJoinSessionCompleteDelegateHandle;

	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

	///////////////////////////////////////////////////////////
	//
	//	Stuff to save the host game options

	UPROPERTY(BlueprintReadWrite, Category = UI)
		bool bHostedGame = false;

	UPROPERTY(BlueprintReadWrite, Category = "UI|Host Game Options")
		float Saved_RoundTimerGameStart;

	UPROPERTY(BlueprintReadWrite, Category = "UI|Host Game Options")
		float Saved_RoundTimerBetweenMaps;

	UPROPERTY(BlueprintReadWrite, Category = "UI|Host Game Options")
		float Saved_ReinforcementTimer;

	UPROPERTY(BlueprintReadWrite, Category = "UI|Host Game Options")
		float Saved_Timelimit;

	UPROPERTY(BlueprintReadWrite, Category = "UI|Host Game Options")
		int32 Saved_RoundsPerMap;

	UPROPERTY(BlueprintReadWrite, Category = "UI|Host Game Options")
		int32 Saved_Scorelimit;

	UPROPERTY(BlueprintReadWrite, Category = "UI|Host Game Options")
		bool Saved_AiEnabled;

	/////////////////////////////////////////////////////////////

	FString GetNextLevel();

	UPROPERTY(BlueprintReadWrite)
	bool bIsSinglePlayerMode = false;

	UFUNCTION(BlueprintCallable, Category = "GameState")
	bool IsSinglePlayer();

	//Pause Game Functionality

	UPROPERTY(BlueprintReadOnly, Category="PauseGame")
	TArray<FString> ActivePauseConditions;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PauseGame")
	void AddPauseGameCondition(const FString&  PauseCondition);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PauseGame")
	void RemovePauseGameCondition(const FString&  PauseCondition);

	UPROPERTY()
	class UHostMigrationManager* HostMigrationManager;
	

	UPROPERTY(BlueprintReadWrite, Category = Level)
		FString NextLevel;

	UPROPERTY()
		FString MainMenuDisplayMessage = "";
	bool bBanned = false;

	void SetBanned(bool bNewBan) { bBanned = bNewBan; }
	
	UFUNCTION(BlueprintCallable, Category = Ban)
	bool GetBanned() { return bBanned; }

	UFUNCTION(BlueprintCallable, Category = "Messages")
		FString GetAndClearMainMenuDisplayMessage();

	UFUNCTION(BlueprintCallable, Category = "Last Join State")
		ELastMenuStateBeforeJoin GetAndClearLastJoinState();

	UFUNCTION(BlueprintCallable, Category = "Last Join State")
		void SetLastJoinState(ELastMenuStateBeforeJoin LastJoiNState);

	ELastMenuStateBeforeJoin LastMenuStateBeforeJoining;

	UFUNCTION(BlueprintCallable, Category = Maplist)
		void BuildMapList();

	UPROPERTY(EditAnywhere)
	FLevelDataLookupTable ModdedMapLookUpData;

	UFUNCTION(BlueprintCallable, Category = Maplist)
	TArray<FString> GetBuiltModdedMapList();

	TArray<FString> BuiltModdedMapList;
	TMap<FName, FLevelDataLookupTable> ModdedLevelData;
	
	UPROPERTY()
	TArray<UModLevelData*> ModdedLevelDataAssets;
	
	UFUNCTION(BlueprintCallable, Category = Maplist)
	TArray<FString> GetBuiltMapList();

	// finds the best guess map name based on the first 31 characters of the URL (because steam drops 32 characters + off)
	UFUNCTION(BlueprintCallable)
	FString GetBestGuessMapName(FString MapName);

	UPROPERTY()
	TArray<FString> BuiltMapList;

	UPROPERTY()
	bool bShowingFPS = false;

	// handle net work failure (display message on main menu etc)
	void HandleNetworkFailure(UWorld *World, UNetDriver *NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);
	void HandleTravelFailure(UWorld* InWorld, ETravelFailure::Type FailureType, const FString& ErrorString = TEXT(""));

	void StartHostMigration();

	bool bWasPlayingDemoReply = false;

	UFUNCTION(BlueprintPure)
	bool IsHostMigrationInProgress(FString& MigratedHostToName);

	/** Start recording a running replay and save it, from blueprint. */
	UFUNCTION(BlueprintCallable, Category = "Replays")
		void StopRecordingReplayFromBP();

	/** Start playback for a previously recorded Replay, from blueprint */
	UFUNCTION(BlueprintCallable, Category = "Replays")
		void PlayReplayFromBP(FString ReplayName);

	/** Start looking for/finding replays on the hard drive */
	UFUNCTION(BlueprintCallable, Category = "Replays")
		TMap<FString, FReplayData> FindReplays();

	/** Apply a new custom name to the replay (for UI only) */
	UFUNCTION(BlueprintCallable, Category = "Replays")
		void RenameReplay(const FString &ReplayName, const FString &NewFriendlyReplayName);

	/** Delete a previously recorded replay */
	UFUNCTION(BlueprintCallable, Category = "Replays")
		void DeleteReplay(const FString &ReplayName);

	/** Opens the replay folder */
	UFUNCTION(BlueprintCallable, Category = "Replays")
		void OpenReplayFolder();

	/** Gets friendly replay name from unfriendly replay name */
	UFUNCTION(BlueprintCallable, Category = "Replays")
		FString GetFriendlyGamemodeName(FString UnfriendlyName);

	/** Adds a replay event to the replay array. Saved after replay recording is stopped. */
	UFUNCTION(BlueprintCallable, Category = "Replays")
	void AddReplayEvent(TEnumAsByte<EReplayEventType> EventType, FVector Location, float Timestamp, FString AdditionalInformation);

	/** Gets all the replay events of the current playing replay. */
	UFUNCTION(BlueprintCallable, Category = "Replays")
	TArray<FReplayEvent> GetReplayEvents();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replays")
	bool bIsRecordingReplay = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replays")
	bool bIsPlayingReplay = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replays")
	float bReplayBeginTime = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replays")
	int32 ReplayNumPlayers = 0;
	
	UPROPERTY()
	UUserWidget* ReplayLoadingScreen;

	UFUNCTION(BlueprintCallable, Category = "Replays")
	void CreateReplayLoadingScreen();
	
	UFUNCTION(BlueprintCallable, Category = "Replays")
	void RemoveReplayLoadingScreen();

	UFUNCTION(BlueprintCallable, Category = "Replays")
	void StartRecordingReplay();

	virtual void StartRecordingReplay(const FString& InName, const FString& FriendlyName, const TArray<FString>& AdditionalOptions = TArray<FString>(), TSharedPtr<IAnalyticsProvider> AnalyticsProvider = nullptr) override;
	
	UFUNCTION(BlueprintCallable, Category = "Replays")
	virtual void StopRecordingReplay() override;

	UFUNCTION(BlueprintCallable, Category = "Replays")
	bool IsReplaySystemEnabled();

	UPROPERTY(BlueprintReadOnly)
	TArray<FReplayEvent> ReplayEvents;

	FTimerHandle GeneratePSOCache_Handle;
	FString CurrentMapNamePSOCache;
	int32 PsoCacheMapIdx = 0;
	int32 PsoThreatIdx = 0;
	TArray<FString> PSOCacheMapList = {
		"RoN_Agency_HostageRescue_Core",
	   "RoN_Club_BarricadedSuspects_Core",
	   "RoN_Datacenter_BarricadedSuspects_Core",
	   "RoN_Dealer_BarricadedSuspects_Core",
	   "RoN_Gas_ActiveShooter_Core",
	   "RoN_Hotel_BarricadedSuspects_Core",
	   "RoN_Importer_BarricadedSuspects_Core",
	   "RoN_Meth_BarricadedSuspects_Core",
	   "RoN_Meth_Raid_Core",
	   "RoN_Penthouse_BarricadedSuspects_Core",
	   "RoN_Ridgeline_BarricadedSuspects_Core",
	   "RoN_Valley_BarricadedSuspects_Core",
	   "RoN_Port_BarricadedSuspects_Core",
	   "RoN_Farm_BarricadedSuspects_Core",
	   "RoN_Farm_Raid_Core",
	   "RoN_Hospital_BarricadedSuspects_Core"
};

	// Generator for pso cache to teleport around the level
	UFUNCTION(Exec)
	void StartGeneratingPSOCache();

	UFUNCTION(Exec)
	void StopGeneratingPSOCache();
	
	void GeneratePSOCache();

	// Deletes a profile from the disk for convenience
	UFUNCTION(Exec)
	void CommanderDeleteProfile(int32 Slot);
	
	// Debug command to complete the next mission in order for the current commander profile
	UFUNCTION(Exec)
	void CommanderCompleteMission(const FString& Mission);

	// Generates an example profile in the specified slot. Will not overwrite existing profiles
	UFUNCTION(Exec)
	void CommanderGenerateProfile(int32 Slot = 0);

private:
	#if ENABLE_ANTI_PIRACY_CHECKS
	FCheckForSteamEmuThreadWorker* SteamEmuThreadWorker = nullptr;
	#endif
};
