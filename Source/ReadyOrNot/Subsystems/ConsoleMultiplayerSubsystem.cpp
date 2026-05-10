#include "ConsoleMultiplayerSubsystem.h"
#include "OnlineSubsystemSessionSettings.h"
//
const FName NAME_LobbySession(TEXT("LobbySession"));
const FName HostPartySessionId(TEXT("HostPartySessionId"));
const FName HostEOSAddr(TEXT("HostEOSAddr"));
const FName EOSName(TEXT("EOS"));

void UConsoleMultiplayerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
#if defined(TARGET_PS5)
	Super::Initialize(Collection);
	UE_LOG_ONLINE(Log, TEXT("Initialize UConsoleMultiplayerSubsystem"));

	// for using matchmaking
	SonyMatchmakingHostOnCompleteDelegate = FOnStartMatchmakingComplete::CreateUObject(this, &ThisClass::SonyMatchmakingHostOnComplete);
	SonyMatchmakingClientOnCompleteDelegate = FOnStartMatchmakingComplete::CreateUObject(this, &ThisClass::SonyMatchmakingClientOnComplete);

	// Public Lobby Host
	LobbySessionCreateCompleteDelegate = FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::LobbySessionOnCreateCompleted);
	LobbySessionStartCompleteDelegate = FOnStartSessionCompleteDelegate::CreateUObject(this, &ThisClass::LobbySessionOnStartCompleted);
	LobbySessionParticipantsChangeDelegate = FOnSessionParticipantsChangeDelegate::CreateUObject(this, &ThisClass::LobbySessionParticipantsChange);
	LobbySessionUnregisterPlayersDelegate = FOnUnregisterPlayersCompleteDelegate::CreateUObject(this,&ThisClass::LobbySessionUnregisterPlayers);
	LobbySessionRegisterPlayersDelegate = FOnRegisterPlayersCompleteDelegate::CreateUObject(this,&ThisClass::LobbySessionRegisterPlayers);
	
	// Public Lobby Client
	// rename this to Client..
	LobbySessionOnJoinDelegate = FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::LobbySessionOnJoinCompleted);

#endif
}

void UConsoleMultiplayerSubsystem::FindPublicPlayers(int numSlots)
{
#if defined(TARGET_PS5)
	UE_LOG_ONLINE(Log, TEXT("Create public lobby session with %d slots"), numSlots);
	LobbySessionCreate(numSlots);
#endif
}

void UConsoleMultiplayerSubsystem::UpdateAvailableSlots(int numSlots)
{
#if defined(TARGET_PS5)
	UE_LOG_ONLINE(Log, TEXT("Update the number of available slots in the lobby to %d slots"), numSlots);
	// Update the number of available slots in the Lobby
	// Will be needed if we want to allow switching between private and public
#endif
}

void UConsoleMultiplayerSubsystem::JoinPublicSession()
{
#if defined(TARGET_PS5)
	UE_LOG_ONLINE(Log, TEXT("JoinPublicSession"));

	// Rename this to ClientFindLobbySessionsOnComplete
	FOnFindSessionsCompleteDelegate FindSessionsCompleteDelegate = FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::FindSessionsOnComplete);
	FOnlineSubsystemBPCallHelperAdvanced Helper(TEXT("JoinPublicSession"), GetWorld());

	AReadyOrNotPlayerController* PlayerController = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld());

	Helper.QueryIDFromPlayerController(PlayerController);

	if (Helper.OnlineSub != nullptr)
	{
		const IOnlineSessionPtr Sessions = Helper.OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			SearchObject = MakeShareable(new FOnlineSessionSearch);
			SearchObject->MaxSearchResults = 10; // we currently on use the first 
			SearchObject->bIsLanQuery = false;
			SearchObject->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);

			Sessions->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);
			Sessions->FindSessions(*Helper.UserID, SearchObject.ToSharedRef());
		}
	}
#endif
}

void UConsoleMultiplayerSubsystem::FindSessionsOnComplete(bool bWasSuccessful)
{
#if defined(TARGET_PS5)
	UE_LOG_ONLINE(Log, TEXT("FindSessionsOnComplete success:%s"), bWasSuccessful ? TEXT("true") : TEXT("false"));

	if (!bWasSuccessful)
	{
		UE_LOG_ONLINE(Log, TEXT("FindSessionsOnComplete failed"));
		return;
	}

	if (SearchObject->SearchResults.Num() == 0)
	{
		UE_LOG_ONLINE(Log, TEXT("No lobbysessions found"));
		return;
	}

	FOnlineSubsystemBPCallHelperAdvanced Helper(TEXT("LobbySessionCreate"), GetWorld());
	AReadyOrNotPlayerController* PlayerController = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld());

	Helper.QueryIDFromPlayerController(PlayerController);

	if (Helper.OnlineSub != nullptr)
	{
		auto Sessions = Helper.OnlineSub->GetSessionInterface();

		// used first one for now

		const FOnlineSessionSearchResult& SearchResult = SearchObject->SearchResults[0];

		LobbySessionOnJoinDelegateHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(LobbySessionOnJoinDelegate);
		Sessions->JoinSession(*Helper.UserID, NAME_LobbySession, SearchResult);
	}
#endif
}

void UConsoleMultiplayerSubsystem::LobbySessionOnJoinCompleted(FName, EOnJoinSessionCompleteResult::Type Result)
{
#if defined(TARGET_PS5)
	FOnlineSubsystemBPCallHelperAdvanced Helper(TEXT("JoinPublicSession"), GetWorld());

	AReadyOrNotPlayerController* PlayerController = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld());

	Helper.QueryIDFromPlayerController(PlayerController);
	if (Helper.OnlineSub != nullptr)
	{
		auto Sessions = Helper.OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			Sessions->ClearOnJoinSessionCompleteDelegate_Handle(LobbySessionOnJoinDelegateHandle);

			if (Result == EOnJoinSessionCompleteResult::Type::Success)
			{
				UE_LOG_ONLINE(Log, TEXT("LobbySessionOnJoinCompleted completeted successfully."));

				auto LobbySession = Sessions->GetNamedSession(NAME_LobbySession);

				FOnlineSessionSetting* PlayerSessionIdSetting = LobbySession->SessionSettings.Settings.Find(HostPartySessionId);
				UE_LOG(LogTemp, Warning, TEXT("FindSessionsOnComplete %s"), *LobbySession->SessionInfo->GetSessionId().ToString());
				UE_LOG(LogTemp, Warning, TEXT("FindSessionsOnComplete %s"), *PlayerSessionIdSetting->Data.ToString());

				FOnSingleSessionResultCompleteDelegate TaskComplete = FOnSingleSessionResultCompleteDelegate::CreateUObject(this, &ThisClass::FindPartySessionByIdCompleted);
				TSharedPtr<const FUniqueNetId> SessionUniqueNetId = MakeShareable(new FUniqueNetIdString(*PlayerSessionIdSetting->Data.ToString()));

				Sessions->FindSessionById(*Helper.UserID, *SessionUniqueNetId, *Helper.UserID, TaskComplete);
			}
		}
		UE_LOG(LogTemp, Warning, TEXT("LobbySessionOnJoinCompleted success"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("LobbySessionOnJoinCompleted failed"));
	}
#endif
}

void UConsoleMultiplayerSubsystem::FindPartySessionByIdCompleted(int32, bool WasSuccessful, const FOnlineSessionSearchResult& Result)
{
#if defined(TARGET_PS5)
	UE_LOG(LogTemp, Warning, TEXT("FindPartySessionByIdCompleted"));

	// check for success
	FOnlineSubsystemBPCallHelperAdvanced Helper(TEXT("FindPartySessionByIdCompleted"), GetWorld());

	AReadyOrNotPlayerController* PlayerController = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld());

	ULevelStreaming* OutStreamedLevel;
	FBlueprintSessionResult PartySessionToJoin;
	PartySessionToJoin.OnlineResult = Result;

	bool destroyFirst = false;
	
	// leave the current PartySession
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid() && (Sessions->GetNamedSession(NAME_PartySession) != nullptr))
		{
			destroyFirst = true;
			Sessions->DestroySession(NAME_PartySession, FOnDestroySessionCompleteDelegate::CreateLambda([this, PlayerController, PartySessionToJoin,&OutStreamedLevel](FName SessionName, bool bWasSuccessful)
			{
				UE_LOG(LogTemp, Log, TEXT("Existing session destroyed."));
				PlayerController->StreamInSession(PartySessionToJoin, OutStreamedLevel, true);
			}));
		}
	}	

	if (!destroyFirst)
	{
		PlayerController->StreamInSession(PartySessionToJoin, OutStreamedLevel, true);
	}
#endif
}

void UConsoleMultiplayerSubsystem::LobbySessionCreate(int numSlots)
{
#if defined(TARGET_PS5)
	FOnlineSubsystemBPCallHelperAdvanced Helper(TEXT("LobbySessionCreate"), GetWorld());

	IOnlineSubsystem* OSS = Online::GetSubsystem(GetWorld(), EOSName);
	FString EOSHostAddr;

	if (OSS)
	{
		auto Sessions = OSS->GetSessionInterface();
		if (Sessions.IsValid()) {
			Sessions->GetP2PSocketAddr(EOSHostAddr);
		}
	}

	AReadyOrNotPlayerController* PlayerController = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld());

	Helper.QueryIDFromPlayerController(PlayerController);

	if (Helper.OnlineSub != nullptr)
	{
		auto Sessions = Helper.OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			// Update partysession with server adress
			FNamedOnlineSession* Session = Sessions->GetNamedSession(NAME_PartySession);
			if (Session != nullptr)
			{
				FOnlineSessionSetting EOSHostAddrSetting;
				EOSHostAddrSetting.Data = FVariantData(EOSHostAddr);
				EOSHostAddrSetting.AdvertisementType = EOnlineDataAdvertisementType::ViaOnlineService;
				Session->SessionSettings.Settings.Add(HostEOSAddr, EOSHostAddrSetting);
				UE_LOG_ONLINE_SESSION(Log, TEXT("Setting connectstring in session: %s"), *EOSHostAddr);
				Sessions->UpdateSession(NAME_PartySession, Session->SessionSettings);
			}

			LobbySessionCreateCompleteDelegateHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(LobbySessionCreateCompleteDelegate);

			FOnlineSessionSettings Settings;
			Settings.NumPublicConnections = numSlots;
			Settings.NumPrivateConnections = numSlots;
			Settings.bAllowJoinInProgress = true;
			Settings.bIsLANMatch = false;
			Settings.bAllowJoinViaPresence = true;
			Settings.bIsDedicated = false;
			Settings.bUsesPresence = true;
			Settings.bUseLobbiesIfAvailable = false;
			Settings.bAllowJoinViaPresenceFriendsOnly = false;
			Settings.bAntiCheatProtected = false;
			Settings.bUsesStats = false;
			Settings.bShouldAdvertise = true;
			Settings.bAllowInvites = false;

			FNamedOnlineSession* const PartySessionPtr = Sessions->GetNamedSession(NAME_PartySession);

			FOnlineSessionSetting PlayerSessionIdSetting;
			PlayerSessionIdSetting.Data = FVariantData(PartySessionPtr->SessionInfo->GetSessionId().ToString());
			PlayerSessionIdSetting.AdvertisementType = EOnlineDataAdvertisementType::ViaOnlineService;
			Settings.Settings.Add(HostPartySessionId, PlayerSessionIdSetting);

			FOnlineSessionSetting SearchIndexSetting;
			SearchIndexSetting.Data = FVariantData(FString("lobby_index"));
			Settings.Settings.Add(SETTING_SEARCHINDEX, SearchIndexSetting);

			if (Helper.UserID.IsValid())
			{
				Sessions->CreateSession(*Helper.UserID, NAME_LobbySession, Settings);
			}
			else
			{
				FFrame::KismetExecutionMessage(TEXT("Invalid Player controller when attempting to start a session"), ELogVerbosity::Warning);
				Sessions->ClearOnCreateSessionCompleteDelegate_Handle(LobbySessionCreateCompleteDelegateHandle);
			}
		}
	}
#endif	
}

void UConsoleMultiplayerSubsystem::LobbySessionOnCreateCompleted(FName SessionName, bool bWasSuccessful)
{
#if defined(TARGET_PS5)
	FOnlineSubsystemBPCallHelperAdvanced Helper(TEXT("LobbySessionCreateCompleted"), GetWorld());

	if (Helper.OnlineSub != nullptr)
	{
		auto Sessions = Helper.OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			Sessions->ClearOnCreateSessionCompleteDelegate_Handle(LobbySessionCreateCompleteDelegateHandle);

			if (bWasSuccessful)
			{
				LobbySessionStartCompleteDelegateHandle = Sessions->AddOnStartSessionCompleteDelegate_Handle(LobbySessionStartCompleteDelegate);
				Sessions->StartSession(SessionName);

				// OnStartCompleted will get called, nothing more to do now
				return;
			}
		}
	}

	if (!bWasSuccessful)
	{
		// Handle this
	}
#endif
}

void UConsoleMultiplayerSubsystem::LobbySessionOnStartCompleted(FName SessionName, bool bWasSuccessful)
{
#if defined(TARGET_PS5)
	FOnlineSubsystemBPCallHelperAdvanced Helper(TEXT("SonyMatchmakingSessionOnStartCompleted"), GetWorld());

	if (Helper.OnlineSub != nullptr)
	{
		auto Sessions = Helper.OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			Sessions->ClearOnStartSessionCompleteDelegate_Handle(LobbySessionStartCompleteDelegateHandle);
			LobbySessionParticipantsChangeDelegateHandle = Sessions->AddOnSessionParticipantsChangeDelegate_Handle(LobbySessionParticipantsChangeDelegate);
			LobbySessionUnregisterPlayersDelegateHandle = Sessions->AddOnUnregisterPlayersCompleteDelegate_Handle(LobbySessionUnregisterPlayersDelegate);
			LobbySessionRegisterPlayersDelegateHandle = Sessions->AddOnRegisterPlayersCompleteDelegate_Handle(LobbySessionRegisterPlayersDelegate);
		}
	}
#endif	
}

void UConsoleMultiplayerSubsystem::LobbySessionParticipantsChange(FName SessionName, const FUniqueNetId& UserId, bool bWasSuccessful)
{
#if defined(TARGET_PS5)
	UE_LOG_ONLINE(Log, TEXT("LobbySessionParticipantsChange session: %s, userid: %s, success:%s"), *SessionName.ToString(), *UserId.ToDebugString(), bWasSuccessful ? TEXT("true") : TEXT("false"));

	if (SessionName == NAME_PartySession)
	{
		// someone left the party
	}
	if (SessionName == NAME_LobbySession)
	{

	}
#endif
}

void UConsoleMultiplayerSubsystem::LobbySessionUnregisterPlayers(FName SessionName, const TArray<FUniqueNetIdRef>& Users, bool bWasSuccessful)
{
#if defined(TARGET_PS5)
	for (int i = 0; i < Users.Num(); i++)
	{
		UE_LOG_ONLINE(Log, TEXT("LobbySessionUnregisterPlayers session: %s, userid: %s, success:%s"), *SessionName.ToString(), *Users[i]->ToDebugString(), bWasSuccessful ? TEXT("true") : TEXT("false"));
	}

	if (SessionName == NAME_PartySession)
	{
		// someone left the party
	}
	if (SessionName == NAME_LobbySession)
	{

	}
#endif
}

void UConsoleMultiplayerSubsystem::LobbySessionRegisterPlayers(FName SessionName, const TArray<FUniqueNetIdRef>& Users, bool bWasSuccessful)
{
#if defined(TARGET_PS5)
	for (int i = 0; i < Users.Num(); i++)
	{
		UE_LOG_ONLINE(Log, TEXT("LobbySessionRegisterPlayers session: %s, userid: %s, success:%s"), *SessionName.ToString(), *Users[i]->ToDebugString(), bWasSuccessful ? TEXT("true") : TEXT("false"));
	}

	if (SessionName == NAME_PartySession)
	{
		// someone left the party
	}
	if (SessionName == NAME_LobbySession)
	{

	}
#endif
}

// ** PLAYERSESSION **

void UConsoleMultiplayerSubsystem::PlayerSessionCreate()
{
#if defined(TARGET_PS5)
	FOnlineSubsystemBPCallHelperAdvanced Helper(TEXT("PlayerSessionCreate"), GetWorld());

	PlayerSessionCreateCompleteDelegate = FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::PlayerSessionOnCreateCompleted);
	PlayerSessionStartCompleteDelegate = FOnStartSessionCompleteDelegate::CreateUObject(this, &ThisClass::PlayerSessionOnStartCompleted);

	AReadyOrNotPlayerController* PlayerController = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld());

	Helper.QueryIDFromPlayerController(PlayerController);

	if (Helper.OnlineSub != nullptr)
	{
		auto Sessions = Helper.OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			PlayerSessionCreateCompleteDelegateHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(PlayerSessionCreateCompleteDelegate);

			FOnlineSessionSettings Settings;
			Settings.NumPublicConnections = 5;
			Settings.NumPrivateConnections = 5;
			Settings.bAllowJoinInProgress = true;
			Settings.bIsLANMatch = false;
			Settings.bAllowJoinViaPresence = true;
			Settings.bIsDedicated = false;
			Settings.bUsesPresence = true;
			Settings.bUseLobbiesIfAvailable = false;
			Settings.bAntiCheatProtected = false;
			Settings.bUsesStats = false;
			Settings.bShouldAdvertise = true;
			Settings.bAllowInvites = true;

			if (Helper.UserID.IsValid())
			{
				Sessions->CreateSession(*Helper.UserID, NAME_PartySession, Settings);
			}
			else
			{
				FFrame::KismetExecutionMessage(TEXT("Invalid Player controller when attempting to start a session"), ELogVerbosity::Warning);
				Sessions->ClearOnCreateSessionCompleteDelegate_Handle(PlayerSessionCreateCompleteDelegateHandle);
			}
		}
	}
#endif	
}

void UConsoleMultiplayerSubsystem::UpdateSessionConnectString()
{
	FOnlineSubsystemBPCallHelperAdvanced Helper(TEXT("UpdateSessionConnectString"), GetWorld());

	IOnlineSubsystem* OSS = Online::GetSubsystem(GetWorld(), EOSName);
	FString EOSHostAddr;

	if (OSS) 
	{
		auto Sessions = OSS->GetSessionInterface();
		if (Sessions.IsValid()) 
		{
			// ##UE5UPGRADE## TODO Needs fix in Engine
			// Sessions->GetP2PSocketAddr(EOSHostAddr);
		}
	}

	if (Helper.OnlineSub != nullptr) 
	{
		auto Sessions = Helper.OnlineSub->GetSessionInterface();
		if (Sessions.IsValid()) 
		{
			// Update gamesession with server adress
			FNamedOnlineSession* Session = Sessions->GetNamedSession(NAME_GameSession);
			if (Session != nullptr) 
			{
				FOnlineSessionSetting EOSHostAddrSetting;
				EOSHostAddrSetting.Data = FVariantData(EOSHostAddr);
				EOSHostAddrSetting.AdvertisementType = EOnlineDataAdvertisementType::ViaOnlineService;
				Session->SessionSettings.Settings.Add(HostEOSAddr, EOSHostAddrSetting);
				UE_LOG_ONLINE_SESSION(Log, TEXT("Setting connectstring in session: %s"), *EOSHostAddr);
				Sessions->UpdateSession(NAME_GameSession, Session->SessionSettings);
			}
		}
	}
}

int UConsoleMultiplayerSubsystem::GetMaxPlayers()
{
	UE_LOG(LogTemp, Warning, TEXT("MaxPlayers: %d"), MaxPlayers);
	return MaxPlayers;
}

void UConsoleMultiplayerSubsystem::SetMaxPlayers(int maxPlayers)
{
	MaxPlayers = maxPlayers;
}

void UConsoleMultiplayerSubsystem::PlayerSessionOnCreateCompleted(FName SessionName, bool bWasSuccessful)
{
#if defined(TARGET_PS5)
	FOnlineSubsystemBPCallHelperAdvanced Helper(TEXT("PartySessionCreateCompleted"), GetWorld());

	if (Helper.OnlineSub != nullptr)
	{
		auto Sessions = Helper.OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			Sessions->ClearOnCreateSessionCompleteDelegate_Handle(PlayerSessionCreateCompleteDelegateHandle);

			if (bWasSuccessful)
			{
				PlayerSessionStartCompleteDelegateHandle = Sessions->AddOnStartSessionCompleteDelegate_Handle(PlayerSessionStartCompleteDelegate);
				Sessions->StartSession(SessionName);

				// OnStartCompleted will get called, nothing more to do now
				return;
			}
		}
	}

	if (!bWasSuccessful)
	{
		// Handle this
	}
#endif
}

void UConsoleMultiplayerSubsystem::PlayerSessionOnStartCompleted(FName SessionName, bool bWasSuccessful)
{
#if defined(TARGET_PS5)
	FOnlineSubsystemBPCallHelperAdvanced Helper(TEXT("SonyMatchmakingSessionOnStartCompleted"), GetWorld());

	if (Helper.OnlineSub != nullptr)
	{
		auto Sessions = Helper.OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			Sessions->ClearOnStartSessionCompleteDelegate_Handle(PlayerSessionStartCompleteDelegateHandle);
		}
		JoinPublicSession();
	}
#endif	
}

// ** MATCHMAKING **
// matchmaking is experimental and since it creates a new gamesession, its incompatible with using playersessions
void UConsoleMultiplayerSubsystem::StartMatchMaking(bool bIsHost)
{
#if defined(TARGET_PS5)
	UE_LOG(LogTemp, Warning, TEXT("StartMatchmaking"));

	AReadyOrNotPlayerController* PlayerController = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld());

	FOnlineSubsystemBPCallHelperAdvanced Helper(TEXT("SonyMatchmakingSessionCreate"), GetWorld());

	Helper.QueryIDFromPlayerController(PlayerController);

	if (Helper.OnlineSub != nullptr)
	{
		auto Sessions = Helper.OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			TArray<FSessionMatchmakingUser> LocalPlayers;
			if (Helper.UserID.IsValid()) {
				FSessionMatchmakingUser MatchmakingUser = { Helper.UserID.ToSharedRef() };
				LocalPlayers.Emplace(MatchmakingUser);
			}

			FOnlineSessionSettings NewSessionSettings;

			NewSessionSettings.NumPublicConnections = 5;
			NewSessionSettings.NumPrivateConnections = 5;
			NewSessionSettings.bShouldAdvertise = true;
			NewSessionSettings.bAllowJoinInProgress = true;
			NewSessionSettings.bIsLANMatch = true;

			TSharedRef<FOnlineSessionSearch> SearchSettings = MakeShareable(new FOnlineSessionSearch);
			SearchSettings->bIsLanQuery = false;
			SearchSettings->TimeoutInSeconds = 600.0f;
			FString RuleSet = "public-coop";
			SearchSettings->QuerySettings.Set(SEARCH_MATCHMAKING_QUEUE, RuleSet, EOnlineComparisonOp::Equals);

			if (bIsHost) {
				bool bStartMatchmakingSuccess = Sessions->StartMatchmaking(LocalPlayers, NAME_GameSession, NewSessionSettings, SearchSettings, SonyMatchmakingHostOnCompleteDelegate);
			}
			else {
				bool bStartMatchmakingSuccess = Sessions->StartMatchmaking(LocalPlayers, NAME_GameSession, NewSessionSettings, SearchSettings, SonyMatchmakingClientOnCompleteDelegate);
			}
		}
	}
#endif
}

void UConsoleMultiplayerSubsystem::SonyMatchmakingClientOnComplete(FName SessionName, const struct FOnlineError& ErrorDetails, const struct FSessionMatchmakingResults& Results)
{
#if defined(TARGET_PS5)	
	if (ErrorDetails.WasSuccessful())
	{
		UE_LOG(LogTemp, Warning, TEXT("SonyMatchmakingStartMatchmakingClientOnComplete - SUCCESS"));
		AReadyOrNotPlayerController* pc = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld());
		if (pc)
		{
			FOnlineSubsystemBPCallHelperAdvanced Helper(TEXT("SonyMatchmakingSessionOnStartCompleted"), GetWorld());

			if (Helper.OnlineSub != nullptr)
			{
				auto Sessions = Helper.OnlineSub->GetSessionInterface();
				if (Sessions.IsValid())
				{
					FString ConnectString;
					if (Sessions->GetResolvedConnectString(NAME_GameSession, ConnectString))
					{
						UE_LOG_ONLINE_SESSION(Log, TEXT("Join session: traveling to %s"), *ConnectString);
						pc->ClientTravel(ConnectString, TRAVEL_Absolute);
					}
				}
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("SonyMatchmakingStartMatchmakingClientOnComplete - FAIL"));
	}
#endif	
}

void UConsoleMultiplayerSubsystem::SonyMatchmakingHostOnComplete(FName SessionName, const struct FOnlineError& ErrorDetails, const struct FSessionMatchmakingResults& Results)
{
#if defined(TARGET_PS5)
	if (ErrorDetails.WasSuccessful())
	{
		UE_LOG(LogTemp, Warning, TEXT("SonyMatchmakingStartMatchmakingHostOnComplete - SUCCESS"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("SonyMatchmakingStartMatchmakingHostOnComplete - FAIL=[%s]"), *ErrorDetails.ToLogString());
	}
#endif	
}

void UConsoleMultiplayerSubsystem::Tick( float DeltaTime )
{
}


/* UConsoleMultiplayerStatics */

void UConsoleMultiplayerStatics::SetMaxPlayers(UObject* WorldContextObject, int maxPlayers)
{
	if (UConsoleMultiplayerSubsystem* ConsoleMultiplayerSubsystem = UConsoleMultiplayerStatics::GetConsoleMultiplayerSubsystem(WorldContextObject))
	{
		ConsoleMultiplayerSubsystem->SetMaxPlayers(maxPlayers);
	}
	
}

int UConsoleMultiplayerStatics::GetMaxPlayers(UObject* WorldContextObject)
{
	if (UConsoleMultiplayerSubsystem* ConsoleMultiplayerSubsystem = UConsoleMultiplayerStatics::GetConsoleMultiplayerSubsystem(WorldContextObject))
	{
		return ConsoleMultiplayerSubsystem->GetMaxPlayers();
	}
	return 5;
}


UConsoleMultiplayerSubsystem* UConsoleMultiplayerStatics::GetConsoleMultiplayerSubsystem(UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (!World)
		return nullptr;
	
	UConsoleMultiplayerSubsystem* ConsoleMultiplayerSubsystem = World->GetGameInstance()->GetSubsystem<UConsoleMultiplayerSubsystem>();
	if (!ConsoleMultiplayerSubsystem)
		return nullptr;

	return ConsoleMultiplayerSubsystem;
}

void UConsoleMultiplayerStatics::FindPublicPlayers(UObject* WorldContextObject, int numSlots)
{
	if (UConsoleMultiplayerSubsystem* ConsoleMultiplayerSubsystem = UConsoleMultiplayerStatics::GetConsoleMultiplayerSubsystem(WorldContextObject))
	{
		ConsoleMultiplayerSubsystem->FindPublicPlayers(numSlots);
	}
}

void UConsoleMultiplayerStatics::PlayerSessionCreate(UObject* WorldContextObject)
{
	if (UConsoleMultiplayerSubsystem* ConsoleMultiplayerSubsystem = UConsoleMultiplayerStatics::GetConsoleMultiplayerSubsystem(WorldContextObject))
	{
		ConsoleMultiplayerSubsystem->PlayerSessionCreate();
	}
}

void UConsoleMultiplayerStatics::StartMatchMaking(UObject* WorldContextObject,bool bIsHost)
{
	if (UConsoleMultiplayerSubsystem* ConsoleMultiplayerSubsystem = UConsoleMultiplayerStatics::GetConsoleMultiplayerSubsystem(WorldContextObject))
	{
		ConsoleMultiplayerSubsystem->StartMatchMaking(bIsHost);
	}
}

void UConsoleMultiplayerStatics::UpdateSessionConnectString(UObject* WorldContextObject)
{
	if (UConsoleMultiplayerSubsystem* ConsoleMultiplayerSubsystem = UConsoleMultiplayerStatics::GetConsoleMultiplayerSubsystem(WorldContextObject))
	{
		ConsoleMultiplayerSubsystem->UpdateSessionConnectString();
	}
}
