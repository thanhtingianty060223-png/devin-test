// Void Interactive, 2017

#include "ReadyOrNotGameSession.h"
#include "ReadyOrNot.h"
#include "ReadyOrNotGameMode.h"
//#include "ApiMatchmaking.h"
#if defined(WITH_STEAM)
#include "steam/steam_gameserver.h"
#endif
//#include "ZRequest.h"
#include "GenericPlatform/GenericPlatformMisc.h"
//#include "ApiPayload.h"
//#include "ApiLocality.h"
#include "Actors/Environment/MissionPortal.h"


AReadyOrNotGameSession::AReadyOrNotGameSession()
{
	PrimaryActorTick.bCanEverTick = true;
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		OnCreateSessionCompleteDelegate = FOnCreateSessionCompleteDelegate::CreateUObject(this, &AReadyOrNotGameSession::OnCreateSessionComplete);	
	}
}

void AReadyOrNotGameSession::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!SessionData)
	{
		LoadSessionData();
	}

	for (TActorIterator<AReadyOrNotPlayerController>It(GetWorld()); It; ++It)
	{
		if (It->IsAFK(SecondsUntilKickedForAFK) && !It->IsAdmin() && SecondsUntilKickedForAFK > 0.0f)
		{
			KickPlayer(*It, FText::FromString("You have been kicked for being Idle."));
		}
	}

	FString CommandLineOptions = FCommandLine::Get();
	bIsMatchMakeServer = CommandLineOptions.Contains("zeuzmatchmake");
	if (bIsMatchMakeServer)
	{

		// if (!GetWorld()->GetTimerManager().IsTimerActive(RefreshMatchmakeServer_Handle))
		// {
		// 	V_LOGM(LogReadyOrNotMatchmaking, "ZeusMatchMakeServer: Starting refresh matchmake server timer");
		// 	GetWorld()->GetTimerManager().SetTimer(RefreshMatchmakeServer_Handle, this, &AReadyOrNotGameSession::RefreshMatchmakeServer, 10.0f, true, 8.0f);
		// }
		// if (!GetWorld()->GetTimerManager().IsTimerActive(AddServerToMatchmakeQueue_Handle))
		// {
		// 	V_LOGM(LogReadyOrNotMatchmaking, "ZeusMatchMakeServer: Starting add server to matchmake queue timer");
		// 	GetWorld()->GetTimerManager().SetTimer(AddServerToMatchmakeQueue_Handle, this, &AReadyOrNotGameSession::AddServerToMatchMakeQueue, 10.0f, true, 20.0f);
		// }
	}
}

void AReadyOrNotGameSession::RegisterServer()
{


#if UE_SERVER || UE_EDITOR

	V_LOGM(LogReadyOrNot, "Trying to register dedicated server!");


	UWorld* const World = GetWorld();
	check(World);
	AReadyOrNotGameMode* const GameMode = Cast<AReadyOrNotGameMode>(World ? World->GetAuthGameMode() : nullptr);
	if (GameMode)
	{
		ModeName = GameMode->urlShortName;
		V_LOGM(LogReadyOrNot, "Running Game Mode: %s", *ModeName);
		MaxPlayers = MaxConnections;
		if (MaxPlayers > GameMode->MaxPlayersAllowed)
		{
			MaxPlayers = GameMode->MaxPlayersAllowed;
		}

		// For nightly 
#if defined(OVERRIDE_MAX_PLAYERS_FOR_TESTING)
		V_LOGM(LogReadyOrNot, "Overriding Max Players to 16");
		MaxPlayers = 16;
#endif

		GameMode->SetPassword(Password);

		V_LOGM(LogReadyOrNot, "Reading Map List");
		for (FString map : MapList)
		{
			V_LOGM(LogReadyOrNot, "Found map %s", *map);
		}
	}

	V_LOGM(LogReadyOrNot, "Running Map: %s", *GetWorld()->GetMapName());

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		IOnlineSessionPtr SessionInt = Online::GetSessionInterface();
		if (SessionInt.IsValid())
		{
			V_LOGM(LogReadyOrNot, "Trying to register dedicated server! Creating Session!");
			FString CommandLineOptions = FCommandLine::Get();
			bIsMatchMakeServer = CommandLineOptions.Contains("zeuzmatchmake");
			if (bIsMatchMakeServer)
			{
				FString PayloadId, Region, RegionLeft, RegionRight = "None";
				GetRegionAndPayloadID(Region, PayloadId);
				Region.Split(",", &RegionLeft, &RegionRight);
				if (RegionLeft != "")
				{
					Region = RegionLeft;
				}
				V_LOGM(LogReadyOrNot, "ZEUZ_PAYLOADID: %s, ZEUZ REGION: %s", *PayloadId, *Region);
				ServerName = "VOID " + Region + " (" + PayloadId.Left(4) + ")";
				V_LOGM(LogReadyOrNot, "Server Name: %s", *ServerName);
			}
			TSharedPtr<class FOnlineSessionSettings> SessionSettings = MakeShareable(new FOnlineSessionSettings());
			SessionSettings->Set(SETTING_MAPNAME, GetWorld()->GetMapName(), EOnlineDataAdvertisementType::ViaOnlineService);
			SessionSettings->Set(SETTING_GAMEMODE, ModeName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
			SessionSettings->Set("ServerName", ServerName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);			
			SessionSettings->Set("Version", UBpGameplayHelperLib::GetProjectVersion(), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
			UReadyOrNotStatics::GetReadyOrNotGameInstance()->BuildChecksum();
			SessionSettings->Set(SETTING_CHECKSUM, UReadyOrNotStatics::GetReadyOrNotGameInstance()->Checksum, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
			V_LOGM(LogReadyOrNot, "Checksum Setting: %i", UReadyOrNotStatics::GetReadyOrNotGameInstance()->Checksum);
			FString PVPSetting = SETTING_PVP_DISABLED;
			for (TActorIterator<AMissionPortal>It(GetWorld()); It; ++It)
			{
				if (!It->ModeURL.Contains("COOP"))
				{
					PVPSetting = SETTING_PVP_ENABLED;
				}
			}
			V_LOGM(LogReadyOrNot, "PVP Setting: %s", *PVPSetting);
			SessionSettings->Set(SETTING_PVP, PVPSetting, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
			
			bool bChecksumEnabled;
			UBpGameplayHelperLib::LoadServersideChecksum(bChecksumEnabled);
			V_LOGM(LogReadyOrNot, "Checksum Enabled: %i", bChecksumEnabled);
			SessionSettings->Set(SETTING_CHECKSUM_ENABLED, bChecksumEnabled, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
			int32 PlayerCount = 0;
			for (TActorIterator<AReadyOrNotPlayerController> It(GetWorld()); It; ++It)
			{
				AReadyOrNotPlayerController* pc = *It;
				if (pc->bIsReplaySpectator)
					continue;

				PlayerCount++;
			}

			SessionSettings->Set("PlayerCount", PlayerCount, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
			
			SessionSettings->bUsesPresence = false;
			SessionSettings->bIsLANMatch = false;
			SessionSettings->BuildUniqueId = UBpGameplayHelperLib::GetProjectVersionAsInt();
			SessionSettings->bShouldAdvertise = !bIsMatchMakeServer;
			SessionSettings->NumPublicConnections = MaxPlayers;
			SessionSettings->bAllowInvites = !bIsMatchMakeServer;
			SessionSettings->bIsDedicated = true;
			HostSettings = SessionSettings;
			OnCreateSessionCompleteDelegateHandle = SessionInt->AddOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegate);
			SessionInt->CreateSession(0, NAME_GameSession, *HostSettings);

		}
	}
#endif
}


// void AReadyOrNotGameSession::AddServerToMatchMakeQueue()
// {
	//
	// if (!MatchmakeIdServer.IsEmpty())
	// {
	// 	UZeuzApiMatchmaking::MatchmakingClose(MatchmakeIdServer);
	// 	MatchmakeIdServer = "";
	// }
	//
	// FZeuzLocalityRegionGetIn RegionIn;
	// RegionCallback.BindUObject(this, &AReadyOrNotGameSession::OnGetRegions);
	// UZeuzApiLocality::LocalityRegionGet(RegionIn, RegionCallback);	
	//
	// FString PayloadId, Region = "None";
	// GetRegionAndPayloadID(Region, PayloadId);
	// //V_LOGM(LogReadyOrNotMatchmaking, "Searching for payload by id:%s", *PayloadId);
	// FZeuzPayloadGetIn PayloadIn;
	// PayloadIn.PayloadIDs = { PayloadId };
	// PayloadIn.GetActive = true;
	// PayloadIn.GetInactive = true;
	// PayloadCreatePartyCallback.BindUObject(this, &AReadyOrNotGameSession::OnGetPayloadsMatchmakingCreateParty);
	// UZeuzApiPayload::PayloadGet(PayloadIn, PayloadCreatePartyCallback);

// }

// void AReadyOrNotGameSession::UpdateServerMatchmakeQueue()
// {
	// if (!MatchmakeIdServer.IsEmpty())
	// {
	// 	UZeuzApiMatchmaking::MatchmakingUpdate(MatchmakeIdServer, MatchmakingCallback);
	// }
// }


// void AReadyOrNotGameSession::OnGetRegions(const TArray<FZeuzRegion>& ZeuzRegions, FString Error)
// {
	// //V_LOGM(LogReadyOrNotMatchmaking, "Found %d regions ", ZeuzRegions.Num());
	// CachedZeuzRegions = ZeuzRegions;
// }

// void AReadyOrNotGameSession::OnMatchmakingCreateParty(const FZeuzMatchMakingStatus& MatchmakingStatus, FString Error)
// {
	// MatchmakeIdServer = MatchmakingStatus.MatchMakingId;
// }


// void AReadyOrNotGameSession::OnGetPayloadsMatchmakingCreateParty(const FZeuzPayloadGetOut& PayloadOut, FString Error)
// {
	//V_LOGM(LogReadyOrNotMatchmaking, "GetPayloadsBeforeMatchmakingCreateParty");
	// for (FZeuzPayloadInfo Payload : PayloadOut.Items)
	// {
	// 	TArray<FZeuzMatchmakingUser> Party;
	// 	int32 PlayerCount = 0;
	// 	for (TActorIterator<AReadyOrNotPlayerController> It(GetWorld()); It; ++It)
	// 	{
	// 		AReadyOrNotPlayerController* pc = *It;
	// 		if (pc->bIsReplaySpectator)
	// 			continue;
	//
	// 		PlayerCount++;
	// 	}
	//
	// 	FZeuzMatchMakingPartyInit CreatePartyIn;
	// 	CreatePartyIn.ServerConnect = Payload.IP + ":" + FString::FromInt(Payload.PortMapping[0].ExternalPort);
	// 	CreatePartyIn.AllocationID = Payload.AllocationID;
	// 	for (TActorIterator<AReadyOrNotPlayerController> It(GetWorld()); It; ++It)
	// 	{
	// 		AReadyOrNotPlayerController* pc = *It;
	// 		if (pc->bIsReplaySpectator)
	// 			continue;
	//
	// 		AReadyOrNotPlayerState* ps = Cast<AReadyOrNotPlayerState>(pc->PlayerState);
	// 		if (ps)
	// 		{
	// 			FZeuzMatchmakingUser User;
	// 			User.DisplayName = ps->GetPlayerName();
	// 			User.UserID = ps->GetUniqueId().ToString();
	// 			Party.Add(User);
	//
	// 		}
	// 	}
	// 	CreatePartyIn.Party = Party;
	// 	for (FZeuzRegion region : CachedZeuzRegions)
	// 	{
	// 		if (region.RegionID == Payload.Regions[0])
	// 		{
	// 			CreatePartyIn.Region = region.DisplayName;
	// 		}
	// 	}
	// 	//V_LOGM(LogReadyOrNotMatchmaking, "Creating zeuz party matchmake! Players: %d", Party.Num());
	// 	CreatePartyCallback.BindUObject(this, &AReadyOrNotGameSession::OnMatchmakingCreateParty);
	// 	UZeuzApiMatchmaking::MatchmakingCreateparty(CreatePartyIn, CreatePartyCallback);
	// }
// }

// void AReadyOrNotGameSession::RefreshMatchmakeServer()
// {
	//V_LOGM(LogReadyOrNotMatchmaking, "Starting Refresh Matchmake Server");

// 	FString PayloadId = "None";
// #if PLATFORM_LINUX
// 	PayloadId = FLinuxPlatformMisc::GetEnvironmentVariable(TEXT("ZEUZ_PAYLOADID"));
// #endif
//
// 	//V_LOGM(LogReadyOrNotMatchmaking, "Searching for payload by id:%s", *PayloadId);
// 	FZeuzPayloadGetIn PayloadIn;
// 	PayloadIn.PayloadIDs = { PayloadId };
// 	PayloadIn.GetActive = true;
// 	PayloadIn.GetInactive = true;
//
// 	// Try Add Server to matchmake queue
// 	PayloadServerRefreshCallback.BindUObject(this, &AReadyOrNotGameSession::OnGetPayloadServerRefresh);
// 	UZeuzApiPayload::PayloadGet(PayloadIn, PayloadServerRefreshCallback);
//
// }

// void AReadyOrNotGameSession::OnGetPayloadServerRefresh(const FZeuzPayloadGetOut& PayloadOut, FString Error)
// {
//
// 	FString PayloadId = "None";
// #if PLATFORM_LINUX
// 	PayloadId = FLinuxPlatformMisc::GetEnvironmentVariable(TEXT("ZEUZ_PAYLOADID"));
// #endif
// 	
// 	int32 PlayerCount = 0;
// 	for (TActorIterator<AReadyOrNotPlayerController> It(GetWorld()); It; ++It)
// 	{
// 		AReadyOrNotPlayerController* pc = *It;
// 		if (!pc->bIsReplaySpectator)
// 		{
// 			PlayerCount++;
// 		}
// 	}
//
// 	PlayerCount == 0 ? UZeuzApiPayload::PayloadUnreserve(PayloadId) : UZeuzApiPayload::PayloadReserve(PayloadId);
// 	
// 	//V_LOGM(LogReadyOrNotMatchmaking, "Found Payload callback Server Refresh");
// 	for (FZeuzPayloadInfo Payload : PayloadOut.Items)
// 	{
// 		//V_LOGM(LogReadyOrNotMatchmaking, "Found Payload %s with %s", *Payload.PayloadID, *Payload.IP);
// 		
// 		FZeuzMatchMakingServerInfo ServerInfo;
// 		ServerInfo.Allocatable = PlayerCount == 0;
// 		ServerInfo.IP = Payload.IP;
// 		ServerInfo.PayloadID = Payload.PayloadID;
// 		ServerInfo.Regions = Payload.Regions;
// 		for (FZeuzPayloadPortMapping PortMap : Payload.PortMapping)
// 		{
// 			ServerInfo.Ports.Add(PortMap.ExternalPort);
// 		}
// 		ServerInfo.UserCount = PlayerCount;
// 		ServerInfo.Updated = true;
// 		for (TActorIterator<AReadyOrNotPlayerController> It(GetWorld()); It; ++It)
// 		{
// 			AReadyOrNotPlayerController* pc = *It;
// 			pc->Rep_MatchmakingServerInfo = ServerInfo;
// 		}
// 		//V_LOGM(LogReadyOrNotMatchmaking, "Creating zeuz party matchmake! Allocatable: %d, Player Count: %d, Reserved: %d", ServerInfo.Allocatable, PlayerCount, PlayerCount != 0);
// 		UZeuzApiMatchmaking::MatchmakingServerrefresh(ServerInfo);
// 	}
// }

void AReadyOrNotGameSession::OnCreateSessionComplete(FName InSessionName, bool bWasSuccessful)
{
#if UE_SERVER
	V_LOGM(LogReadyOrNot, "OnCreateSessionComplete %s bSuccess: %d", *InSessionName.ToString(), bWasSuccessful);

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		Sessions->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegateHandle);

		IOnlineSessionPtr SessionInt = Online::GetSessionInterface();
		if (SessionInt)
		{
#if defined(WITH_STEAM)
			ISteamGameServer* SteamGameServerPtr = SteamGameServer();
			FNamedOnlineSession* Session = SessionInt->GetNamedSession(NAME_GameSession);
			if (Session != nullptr && SteamGameServerPtr != nullptr)
			{
				if (!ServerName.IsEmpty())
				{
					SteamGameServerPtr->SetServerName(TCHAR_TO_ANSI(*ServerName));
				}
				SteamGameServerPtr->SetDedicatedServer(true);
				SteamGameServerPtr->SetGameDescription(TCHAR_TO_ANSI(ModeName.IsEmpty() ? *FString("Unknown") : *ModeName));
			}
#endif
		}
	}
#endif
}

void AReadyOrNotGameSession::LoadSessionData()
{
	SessionData = Cast<UReadyOrNotSessionData>(UGameplayStatics::CreateSaveGameObject(UReadyOrNotSessionData::StaticClass()));
	SessionData = Cast<UReadyOrNotSessionData>(UGameplayStatics::LoadGameFromSlot(SessionData->SaveSlotName, SessionData->UserIndex));
	if (!SessionData)
	{
		SessionData = UReadyOrNotSessionData::CreateDefaultSavegame("SessionData");
	}
}

void AReadyOrNotGameSession::SaveSessionData()
{
	if (SessionData)
	{
		UGameplayStatics::SaveGameToSlot(SessionData, SessionData->SaveSlotName, SessionData->UserIndex);
	}
}

void AReadyOrNotGameSession::AddTeamKill(FString SteamId)
{
	if (SessionData)
	{
		if (SessionData->SavedTeamKillData.Find(SteamId))
		{
			SessionData->SavedTeamKillData[SteamId]++;
		}
		else
		{
			SessionData->SavedTeamKillData.Add(SteamId, 1);
		}

		if (SessionData->SavedTeamKillData.Find(SteamId))
		{
			if (SessionData->SavedTeamKillData[SteamId] >= MaxTeamKillsBeforeAutoBan && MaxTeamKillsBeforeAutoBan > 0)
			{
				for (TActorIterator<APlayerController>It(GetWorld()); It; ++It)
				{
					if (It->PlayerState->GetUniqueId().ToString() == SteamId)
					{
						BanPlayer(*It, FText::FromString("You have been banned due to excessive team kills."));
						break;
					}
				}
				
			}
		}
	}
}

void AReadyOrNotGameSession::MakeLoadingMapOnlyURL(FString MapURL)
{
	

}

void AReadyOrNotGameSession::UpdateServerDetails(FString MapName, FString GameMode)
{
	V_LOGM(LogReadyOrNot, "Updating Server Details! %s %s", *MapName, *GameMode);
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		IOnlineSessionPtr SessionInt = Online::GetSessionInterface();
		if (SessionInt)
		{
#if defined(WITH_STEAM)
			ISteamGameServer* SteamGameServerPtr = SteamGameServer();
			FNamedOnlineSession* Session = SessionInt->GetNamedSession(NAME_GameSession);
			if (Session != nullptr)
			{
				if (!MapName.IsEmpty())
				{
					Session->SessionSettings.Set(SETTING_MAPNAME, MapName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
				}
				if (!GameMode.IsEmpty())
				{
					Session->SessionSettings.Set(SETTING_GAMEMODE, GameMode, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
				}
				Session->SessionSettings.Set("Version", UBpGameplayHelperLib::GetProjectVersion(), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
				Session->SessionSettings.Set(SETTING_CHECKSUM, UReadyOrNotStatics::GetReadyOrNotGameInstance()->Checksum, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
				bool bChecksumEnabled;
				UBpGameplayHelperLib::LoadServersideChecksum(bChecksumEnabled);
				Session->SessionSettings.Set(SETTING_CHECKSUM_ENABLED, bChecksumEnabled, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
				int32 PlayerCount = 0;
				for (TActorIterator<AReadyOrNotPlayerController> It(GetWorld()); It; ++It)
				{
					AReadyOrNotPlayerController* pc = *It;
					if (pc->bIsReplaySpectator || !pc->GetPlayerState<AReadyOrNotPlayerState>() || pc->bExiting)
						continue;

					PlayerCount++;
				}
	
				V_LOGM(LogReadyOrNot, "Updating Steam Player Count To: %s", *FString::FromInt(PlayerCount));
				Session->SessionSettings.Set("PlayerCount", PlayerCount, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
				AReadyOrNotGameMode* const mode = Cast<AReadyOrNotGameMode>(GetWorld() ? GetWorld()->GetAuthGameMode() : nullptr);
				if (mode)
				{
					Session->SessionSettings.Set(FName(TEXT("PasswordProtected")), !mode->Password.IsEmpty(), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
				}
				if (SteamGameServerPtr)
				{
					SteamGameServerPtr->SetGameDescription(TCHAR_TO_ANSI(ModeName.IsEmpty() ? *FString("Unknown") : *GameMode));
				}
				SessionInt->UpdateSession(SessionName, Session->SessionSettings);
			}
#endif
		}
	}
}

void AReadyOrNotGameSession::AddAdmin(APlayerController* AdminPlayer)
{
	if (AdminPlayer)
	{
		if (AdminPlayer->PlayerState
			&& AdminPlayer->PlayerState->GetUniqueId()->IsValid())
		{
			FString playerId = AdminPlayer->PlayerState->GetUniqueId()->ToString();
			LoggedInAdmins.AddUnique(playerId);
		}

		AReadyOrNotGameState* gs = Cast<AReadyOrNotGameState>(UGameplayStatics::GetGameState(AdminPlayer));
		if (gs)
		{
			gs->AdminPlayerControllers.AddUnique(AdminPlayer);
		}

		WriteOutConfig();
	}
}

void AReadyOrNotGameSession::RemoveAdmin(APlayerController* AdminPlayer)
{
	if (AdminPlayer)
	{
		if (AdminPlayer->PlayerState
			&& AdminPlayer->PlayerState->GetUniqueId()->IsValid())
		{
			FString playerId = AdminPlayer->PlayerState->GetUniqueId()->ToString();
			LoggedInAdmins.Remove(playerId);
		}

		AReadyOrNotGameState* gs = Cast<AReadyOrNotGameState>(UGameplayStatics::GetGameState(AdminPlayer));
		if (gs)
		{
			gs->AdminPlayerControllers.Remove(AdminPlayer);
		}

		WriteOutConfig();
	}
}

bool AReadyOrNotGameSession::BanPlayer(APlayerController* BannedPlayer, const FText& BanReason)
{
	if (Super::BanPlayer(BannedPlayer, BanReason))
	{
		if (BannedPlayer && BannedPlayer->PlayerState)
		{
			FString playerId = BannedPlayer->PlayerState->GetUniqueId()->ToString();
			BanList.AddUnique(playerId);
			if (SessionData)
			{
				SessionData->BanReasonData.Add(playerId, BanReason.ToString());
			}
			WriteOutConfig();
			return true;
		}

	}
	return false;
}

void AReadyOrNotGameSession::SetServerSettings(float NewRoundTimerGameStart, float NewRoundTimerBetweenMaps, float NewReinforcementTimer, float NewTimelimit, int32 NewRoundsPerMap, int32 NewScorelimit, bool NewAiEnabled)
{
	UReadyOrNotGameInstance* gi = Cast<UReadyOrNotGameInstance>(GetWorld()->GetGameInstance());
	if (!gi)
	{
		return;
	}
	gi->bHostedGame = true;
	gi->Saved_RoundTimerGameStart = RoundTimerGameStart = NewRoundTimerGameStart;

	gi->Saved_RoundTimerBetweenMaps = RoundTimerBetweenMaps = NewRoundTimerBetweenMaps;
	gi->Saved_ReinforcementTimer = ReinforcementTimer = NewReinforcementTimer;
	gi->Saved_Timelimit = Timelimit = NewTimelimit;
	gi->Saved_RoundsPerMap = RoundsPerMap = NewRoundsPerMap;
	gi->Saved_Scorelimit = Scorelimit = NewScorelimit;
	gi->Saved_AiEnabled = bAiEnabled = NewAiEnabled;
}

void AReadyOrNotGameSession::BeginPlay()
{
	Super::BeginPlay();

	ReloadConfig(GetClass(), *GetConfigFilePath());

	MapList.Remove("");
	// remove old config stuff so we can add maps the new way
	MapList.Remove("RON_GAS_CORE?game=coop");
	MapList.Remove("RoN_PORT_PVP_V1?game=tdm");
	if (MapList.Num() == 0)
	{
		RefreshMapList();

	}

#if WITH_EDITOR
	MapList.Empty();
	MapList.Add("ron_gas_bombthreat_core?game=de");
#endif

	UReadyOrNotGameInstance* gi = Cast<UReadyOrNotGameInstance>(GetWorld()->GetGameInstance());
	if (!gi || !gi->bHostedGame)
	{
		return;
	}

	RoundTimerGameStart = gi->Saved_RoundTimerGameStart;
	RoundTimerBetweenMaps = gi->Saved_RoundTimerBetweenMaps;
	ReinforcementTimer = gi->Saved_ReinforcementTimer;
	Timelimit = gi->Saved_Timelimit;
	RoundsPerMap = gi->Saved_RoundsPerMap;
	Scorelimit = gi->Saved_Scorelimit;
	bAiEnabled = gi->Saved_AiEnabled;
}

void AReadyOrNotGameSession::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	WriteOutConfig();
}

FString AReadyOrNotGameSession::GetConfigFilePath()
{
	FString configFilePath;
	FParse::Value(FCommandLine::Get(), TEXT("defgameini"), configFilePath);
	// remove any leading = was probably used like -defgameini=[myinifile].ini
	if (configFilePath.Find("=") == 0)
	{
		configFilePath.RemoveAt(0);
	}

	// nothing defined by -defgameini. could be defined by -gameini in command line or left blank for /saved/ folder
	if (configFilePath.IsEmpty())
	{
		configFilePath = GGameIni;
	}
	// will only have slashes if its a full path to the file.
	else if (!configFilePath.Contains("\\") && !configFilePath.Contains("/"))
	{
		// must be using releative path must be running from launch dir
		configFilePath = FPaths::Combine(FPaths::LaunchDir(), configFilePath);
	}
	return configFilePath;
}

void AReadyOrNotGameSession::WriteOutConfig()
{
	if (!GConfig)
		return;

	// write out defaults to config file
	FString configFile = GetConfigFilePath();

	

#if PLATFORM_WINDOWS
	// todo: no linux support atm
#define CONFIG_STRING(x) GConfig->SetString(*SessionSection, TEXT(###x), *x, configFile)
#define CONFIG_ARRAY(x) GConfig->SetArray(*SessionSection, TEXT(###x), x, configFile)
#define CONFIG_INT(x) GConfig->SetInt(*SessionSection, TEXT(###x), x, configFile)
#define CONFIG_FLOAT(x) GConfig->SetFloat(*SessionSection, TEXT(###x), x, configFile)

	CONFIG_STRING(ServerName);
	CONFIG_INT(ReturnToLobbyAfterXMissions);
	CONFIG_ARRAY(MapList);
	CONFIG_STRING(Password);
	//CONFIG_INT(MaxConnections);
	CONFIG_STRING(AdminPassword);
	CONFIG_ARRAY(LoggedInAdmins);
	CONFIG_ARRAY(BanList);
	CONFIG_INT(MaxTeamKillsBeforeAutoKick);
	CONFIG_INT(MaxTeamKillsBeforeAutoBan);
	CONFIG_FLOAT(SecondsUntilKickedForAFK);
	//CONFIG_FLOAT(RoundTimerGameStart);
	//CONFIG_FLOAT(RoundTimerBetweenMaps);
	//CONFIG_FLOAT(ReinforcementTimer);
	//CONFIG_FLOAT(Timelimit);
	//CONFIG_INT(RoundsPerMap);
	CONFIG_INT(ClientNetSpeed);
	CONFIG_FLOAT(SecondsUntilAutostartLobby);
	CONFIG_INT(MinPlayersForAutostart);
	//CONFIG_FLOAT(RespawnTimer);
	
	GConfig->Flush(false, configFile);
#endif
}

void AReadyOrNotGameSession::RefreshMapList()
{
	// add random maps to the rotation...
	TArray<FString> TempMapList = {};
	MapList.Empty();
	
	AMissionPortal* MissionPortal = nullptr;
	for(TActorIterator<AMissionPortal> It(GetWorld()); It; ++It)
	{
		if (AMissionPortal* Portal = *It)
			MissionPortal = Portal;
	}

	if (!IsValid(MissionPortal))
		MissionPortal = GetWorld()->SpawnActor<AMissionPortal>();
	
	for (int32 i = 0; i < 1000; i++)
	{
		MissionPortal->SelectRandomMission(false);
		TempMapList.AddUnique(MissionPortal->MissionURL);
	}

	for (int32 i = 0; i < TempMapList.Num(); i++)
	{
		if (!MissionPortal->DoesLevelExistInBuild(TempMapList[i]))
		{
			TempMapList.RemoveAt(i);
			i--;
		}
	}

	MapList = TempMapList;

}
