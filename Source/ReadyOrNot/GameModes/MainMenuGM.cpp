// Copyright Void Interactive, 2023

#include "MainMenuGM.h"
#include "AdvancedSessionsLibrary.h"
#include "ReadyOrNot.h"
#include "CreateSessionCallbackProxyAdvanced.h"
#include "DestroySessionCallbackProxyAdvanced.h"
#include "FindSessionsCallbackProxyAdvanced.h"
#include "Commander/CommanderProfile.h"
#include "Commander/MetaGameProfile.h"
#include "Info/ModioManager.h"
#include "lib/GameFeatureLibrary.h"
#if defined(TARGET_PS5)
#include "Subsystems/ConsoleMultiplayerSubsystem.h"
#include "Subsystems/PS5ActivitiesSubsystem.h"
#endif

AMainMenuGM::AMainMenuGM()
{	
}

void AMainMenuGM::PlayMainMenuMusic()
{
	MainMenuMusicInst = UFMODBlueprintStatics::PlayEvent2D(GetWorld(), MainMenuMusic, true);
	MainMenuAmbienceInst = UFMODBlueprintStatics::PlayEvent2D(GetWorld(), MainMenuAmbience, true);
}

void AMainMenuGM::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
#if UE_BUILD_SHIPPING && ENABLE_ANTI_PIRACY_CHECKS && PLATFORM_WINDOWS
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		if ( FCString::Atoi(*OnlineSub->GetAppId()) != 840820 && FCString::Atoi(*OnlineSub->GetAppId()) != 1144200)
		{
			AActor* MyNullPtr = nullptr;
			MyNullPtr->SetActorLocation(FVector::ZeroVector);
		}
	}
#endif
	
	UReadyOrNotGameInstance* gi = Cast<UReadyOrNotGameInstance>(GetWorld()->GetGameInstance());
	if (gi)
	{
		gi->BuildMapList();
	}

	Super::InitGame(MapName, Options, ErrorMessage);
}

void AMainMenuGM::BeginPlay()
{
	Super::BeginPlay();
	ApplyRONGameUserSettings();
	bUseSeamlessTravel = true;
	PrimaryActorTick.TickInterval = 0.0f;
	PrimaryActorTick.bCanEverTick = true;

	AReadyOrNotPlayerController* pc = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld());
	ensureMsgf(pc != nullptr, TEXT("MainMenuGM::BeginPlay() - PlayerController is null!"));
	if (!pc)
		return;

	pc->ConsoleCommand("r.VolumetricFog 1");

	pc->RemoveLoadingScreen();

	pc->PlayerCameraManager->StartCameraFade(2.0f, 0.0f, 2.0f, FLinearColor::Black, false, false);

	UDestroySessionCallbackProxyAdvanced* DestroySession = UDestroySessionCallbackProxyAdvanced::DestroySession(GetWorld(), pc);
	DestroySession->Activate();
	
	PlayMainMenuMusic();	

	UReadyOrNotGameInstance* gi = Cast<UReadyOrNotGameInstance>(GetWorld()->GetGameInstance());
	if (gi)
	{
		if (gi->HostMigrationManager)
		{
			gi->HostMigrationManager->SetHostMigrationInProgress(false);
		}
		gi->BuildMapList();
		gi->GenerateURLMap();
		gi->ReadyOrNotBackend->OnCheckedBanStatus.AddDynamic(this, &AMainMenuGM::OnBanStatusChecked);
		gi->LeaveVoiceChannels();

#if defined(WITH_MODIO)
		gi->EnableModManager();
		if (gi->ModioManager)
		{
			gi->ModioManager->OnModInstalledEvent().AddUObject(this, &AMainMenuGM::OnModStateChange);
			gi->ModioManager->OnModUninstalledEvent().AddUObject(this, &AMainMenuGM::OnModStateChange);
			gi->ModioManager->OnModStateUpdated().AddUObject(this, &AMainMenuGM::OnModStateChange);
		}
#endif
	}
	// handled in tick for supporter builds once they have succesfully logged in
#ifndef SUPPORTER_ONLY_BUILD
	CreateMainMenu();
#endif
}

void AMainMenuGM::OnBanStatusChecked(FString SteamId, bool bIsBanned, FString BanReason, bool bIsMySteamId)
{
	if (bIsBanned && bIsMySteamId)
	{
		const FString Message = NSLOCTEXT("MainMenu", "MainMenuExitBtn", "Exit").ToString();
		ShowMessageDisplayBox(BanReason, Message, true);
	}	
}
// TARGET_CONSOLE
void AMainMenuGM::ApplyRONGameUserSettings()
{
#if defined(TARGET_CONSOLE)
	return;
#else
	UReadyOrNotGameUserSettings* Settings = Cast<UReadyOrNotGameUserSettings>( UBpVideoSettingsLib::GetGameUserSettings());
	if (!Settings)
		return;

	if (Settings->AntiAliasingQuality == -1)
		Settings->AntiAliasingQuality = Settings->ScalabilityQuality.AntiAliasingQuality;
	if (Settings->EffectsQuality == -1)
		Settings->EffectsQuality = Settings->ScalabilityQuality.EffectsQuality;
	if (Settings->PostProcessQuality == -1)
		Settings->PostProcessQuality = Settings->ScalabilityQuality.PostProcessQuality;
	if (Settings->ShadowQuality == -1)
		Settings->ShadowQuality = Settings->ScalabilityQuality.ShadowQuality;
	if (Settings->TextureQuality == -1)
		Settings->TextureQuality = Settings->ScalabilityQuality.TextureQuality;
	if (Settings->ViewDistanceQuality == -1)
		Settings->ViewDistanceQuality = Settings->ScalabilityQuality.ViewDistanceQuality;

	Settings->SaveConfig();
#endif
}

void AMainMenuGM::ShowMainMenuMsg()
{
	FString MainMenuMsg = UBpGameplayHelperLib::GetRONGameInstance()->GetAndClearMainMenuDisplayMessage();
	if (!MainMenuMsg.IsEmpty())
	{
		if (MainMenuMsg == "Server is full!")
		{
			FindOnlineSession(true, bPVPSessionSearch);
			FText Msg = NSLOCTEXT("MainMenu", "SearchingFull", "Server Full. Continuing Search...");
			OnUpdateSessionSearch.Broadcast(false, FText::Format(Msg, (int32)DesiredSessionPing), bPVPSessionSearch);
			return;
		}
		const FString Message = NSLOCTEXT("MainMenu", "MainMenuOKBtn", "OK").ToString();
		ShowMessageDisplayBox(MainMenuMsg, Message);
	}
}

void AMainMenuGM::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	UFMODBlueprintStatics::EventInstanceStop(MainMenuMusicInst, true);
	UFMODBlueprintStatics::EventInstanceStop(MainMenuAmbienceInst, true);

#if UE_BUILD_SHIPPING && ENABLE_ANTI_PIRACY_CHECKS && PLATFORM_WINDOWS
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		if ( (FCString::Atoi(*OnlineSub->GetAppId()) != 42041 * 20 && FCString::Atoi(*OnlineSub->GetAppId()) != 57210 * 20) || FCString::Atoi(*OnlineSub->GetAppId()) == 24 * 20)
		{
			FPlatformMisc::RequestExit(true);
		}
		
	}
#endif
	
}

void AMainMenuGM::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	AReadyOrNotPlayerController* pc = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld());
	if (pc)
	{
		pc->bWantsHudClearOnPossess = false;
		if (pc->GetPawn())
		{
			pc->GetPawn()->SetActorTransform(ChoosePlayerStart_Implementation(pc)->GetActorTransform());	
		}
		float MouseX, MouseY;
		int32 ViewPortX, ViewPortY;
		pc->GetMousePosition(MouseX, MouseY);
		pc->GetViewportSize(ViewPortX, ViewPortY);
		for (TActorIterator<ACameraActor>It(GetWorld()); It; ++It)
		{
			if (It->Tags.Contains("LobbyView"))
			{
				FViewTargetTransitionParams TransitionParams;
				TransitionParams.BlendTime = 0.0f;
				TransitionParams.bLockOutgoing = true;
				if (pc->GetViewTarget() != *It)
				{
					OriginalCameraRotation = It->GetActorRotation();
					pc->SetViewTarget(*It, TransitionParams);
				}
				else
				{
					constexpr float MaxPitch = 2.5f;
					constexpr float MaxYaw = 2.5f;
					
					FRotator NewRotation = OriginalCameraRotation;
					NewRotation.Yaw -= FMath::Clamp(((ViewPortX * 0.5f) - MouseX) * 0.001f, -MaxYaw, MaxYaw);
					NewRotation.Pitch += FMath::Clamp(((ViewPortY * 0.5f) - MouseY) * 0.001f, -MaxPitch, MaxPitch);
					It->SetActorRotation(FMath::RInterpTo(It->GetActorRotation(), NewRotation, DeltaSeconds, 1.0f));
				}

				/**
				 * The proper fov for modern screens should be 30.0f for this scene,
				 * however it may behave badly for aspect ratios that are narrower than 16:9.
				 * The fov is increased from 30.0f to 40.0f as the aspect ratio goes between ~16:9 and ~4:3.
				 */
				const float AspectRatio = ViewPortX / (ViewPortY * 1.0f);
				float x = (1.78 - AspectRatio) * 2.0f;
				x = FMath::Clamp(x, 0.0f, 1.0f);
				const float NewFov = FMath::Lerp(30.0f, 40.0f, x);
				const float CurrentFov= pc->PlayerCameraManager->GetFOVAngle();
				if(!((FMath::Abs(CurrentFov - NewFov) - 0.01) < 0.0f))
				{
					pc->PlayerCameraManager->SetFOV(NewFov); 
				}
				GetWorld()->GetFirstLocalPlayerFromController()->AspectRatioAxisConstraint = AspectRatio_MaintainYFOV; 
			}

		}
		
	}

	if (bShouldFindSession)
	{
#if !defined(USE_EOS)		
		TimeUntilFindNextSessionList -= DeltaSeconds;
		if (TimeUntilFindNextSessionList <= 0.0f)
		{
			FindNextSessionList();
		}
#endif
	}	

#ifdef SUPPORTER_ONLY_BUILD
	UReadyOrNotGameInstance* gi = Cast<UReadyOrNotGameInstance>(UReadyOrNotStatics::GetReadyOrNotGameInstance());
	if (gi)
	{
		if (gi->ReadyOrNotBackend->HasLoginFailed())
		{
			if (!bCreatedLoginFailedMsg)
			{
				bCreatedLoginFailedMsg = true;
				ShowMessageDisplayBox(XorString("Failed to connect to the backend service (Connection to the backend service is required for Supporter Only Builds)\nError: ") + gi->ReadyOrNotBackend->GetLoginFailedMessage(), XorString("Exit"), true);
			}
		} else
		{
			if (gi->IsLoggedIntoBackend() && gi->OwnedDLCMap.Find(DLC_SUPPORTER) && !bCreatedMainMenu)
			{
				CreateMainMenu();
			}
		}
		
	}
#endif
	
}

void AMainMenuGM::FindOnlineSession(bool bNewSearch, bool bPVPSession)
{
#ifdef SUPPORTER_ONLY_BUILD
	if (!UGameFeatureLibrary::IsGameVersionEnabled(DLC_SUPPORTER))
	{
		return;
	}
#endif

#if defined(TARGET_PS5)
	UConsoleMultiplayerStatics::PlayerSessionCreate(GetWorld());
	return;
#endif

#ifndef RON_PVP_ENABLED
	bPVPSession = false;
#endif
	bPVPSessionSearch = bPVPSession;
	bPendingCancelSessionSearch = false;
	bShouldFindSession = true;
	
	if (bNewSearch)
	{
		DesiredSessionPing = 60.0f;
	}
	
	if (DesiredSessionPing > 250.0f)
	{
		UReadyOrNotBackend::LogMessage("Failed to find any public servers with acceptable ping (Max: 250 ms)");
		FText Msg = NSLOCTEXT("MainMenu", "FailPing", "Failed to find server with acceptable ping");
		OnUpdateSessionSearch.Broadcast(true, Msg, bPVPSessionSearch);
		bPendingCancelSessionSearch = true;
		UBpGameplayHelperLib::GetRONGameInstance()->MainMenuDisplayMessage = Msg.ToString();
		ShowMainMenuMsg();
		return;
	}
	
	TArray<FSessionsSearchSetting> SearchSettings;
	FSessionsSearchSetting Version, Checksum;
	Version.ComparisonOp = EOnlineComparisonOpRedux::Equals;
	Version.PropertyKeyPair.Key = SETTING_VERSION;
	Version.PropertyKeyPair.Data = *UBpGameplayHelperLib::GetProjectVersion();
	FSessionsSearchSetting PVPSession;
	PVPSession.ComparisonOp = bPVPSessionSearch ?  EOnlineComparisonOpRedux::Equals : EOnlineComparisonOpRedux::NotEquals;
	PVPSession.PropertyKeyPair.Key = SETTING_PVP;
	PVPSession.PropertyKeyPair.Data = *SETTING_PVP_ENABLED;
	SearchSettings.Add(PVPSession);

	SearchSettings.Add(Version);

	if (bPVPSession)
	{
		FindSessionsCallbackProxyAdvanced = UFindSessionsCallbackProxyAdvanced::FindSessionsAdvanced(GetWorld(), UBpGameplayHelperLib::GetLocalPlayerController(GetWorld()), 50, false, EBPServerPresenceSearchType::DedicatedServersOnly, SearchSettings, false, DesiredSessionPing > 100.0f); // only find empty servers after we've first searched decently for low ping/player filled servers
		FindSessionsCallbackProxyAdvanced->OnFailure.AddDynamic(this, &AMainMenuGM::OnFindSessionFailed);
		FindSessionsCallbackProxyAdvanced->OnSuccess.AddDynamic(this, &AMainMenuGM::OnFindSessionSuccess);
#if defined(USE_EOS)
		FindSessionsCallbackProxyAdvanced->UseEOS= true;
#endif
		FindSessionsCallbackProxyAdvanced->Activate();
	}
	else
	{
		FindSessionsCallbackProxyAdvanced = UFindSessionsCallbackProxyAdvanced::FindSessionsAdvanced(GetWorld(), UBpGameplayHelperLib::GetLocalPlayerController(GetWorld()), 50, false, EBPServerPresenceSearchType::AllServers, SearchSettings, false, true);
		FindSessionsCallbackProxyAdvanced->OnFailure.AddDynamic(this, &AMainMenuGM::OnFindSessionFailed);
		FindSessionsCallbackProxyAdvanced->OnSuccess.AddDynamic(this, &AMainMenuGM::OnFindSessionSuccess);
#if defined(USE_EOS)
		FindSessionsCallbackProxyAdvanced->UseEOS= true;
#endif
		FindSessionsCallbackProxyAdvanced->Activate();
	}


	FText Msg = NSLOCTEXT("MainMenu", "Searching", "Searching for servers under {0} MS...");
	OnUpdateSessionSearch.Broadcast(false, FText::Format(Msg, (int32)DesiredSessionPing), bPVPSessionSearch);
}

void AMainMenuGM::CancelSessionSearch()
{
	bShouldFindSession = false;
	bPendingCancelSessionSearch = true;
	OnUpdateSessionSearch.Broadcast(true, FText(), bPVPSessionSearch);
}

bool AMainMenuGM::IsSearchingForSession()
{
	return bShouldFindSession;
}

void AMainMenuGM::OnFindSessionFailed(const TArray<FBlueprintSessionResult>& Results)
{
	FText Msg = NSLOCTEXT("MainMenu", "FailFind", "Failed To Find Sessions");
	OnUpdateSessionSearch.Broadcast(true, Msg, bPVPSessionSearch);
}

void AMainMenuGM::FindNextSessionList()
{
	if (bPendingCancelSessionSearch)
	{
		CancelSessionSearch();
		return;
	}
	TimeUntilFindNextSessionList = 5.0f;
	DesiredSessionPing += 30.0f; FindOnlineSession(false, bPVPSessionSearch);
}

void AMainMenuGM::OnFindSessionSuccess(const TArray<FBlueprintSessionResult>& Results)
{
	if (bPendingCancelSessionSearch)
		return;

#ifdef SUPPORTER_ONLY_BUILD
	if (!UGameFeatureLibrary::IsGameVersionEnabled(DLC_SUPPORTER))
	{
		return;
	}
#endif

	TArray<FBlueprintSessionResult> DesiredResults;
	FBlueprintSessionResult DesiredResult;
	for (FBlueprintSessionResult Session : Results)
	{
		FString Version;
		Session.OnlineResult.Session.SessionSettings.Get(SETTING_VERSION, Version);
		int32 Checksum;
		Session.OnlineResult.Session.SessionSettings.Get(SETTING_CHECKSUM, Checksum);
		bool bServerChecksumEnabled;
		Session.OnlineResult.Session.SessionSettings.Get(SETTING_CHECKSUM_ENABLED, bServerChecksumEnabled);
		FString MigrationGUID;
		Session.OnlineResult.Session.SessionSettings.Get(MIGRATION_GUID, MigrationGUID);

		// don't allow joining migrating games
		if (!MigrationGUID.IsEmpty())
			continue;

		bool bChecksumEnabled;
		UBpGameplayHelperLib::LoadServersideChecksum(bChecksumEnabled);
		// client side checksum is never enabled if game is modded
		if (UReadyOrNotStatics::GetReadyOrNotGameInstance()->bIsModded)
		{
			bChecksumEnabled = false;
		}
		if (bChecksumEnabled || bServerChecksumEnabled)
		{
#if !defined(USE_EOS) && !defined(TARGET_CONSOLE) 
			if (Checksum != UReadyOrNotStatics::GetReadyOrNotGameInstance()->Checksum) {
				UE_LOG_ONLINE_SESSION(Warning, TEXT("MainMenuGM::OnLobbySuccess Checksum mismatch"));
				continue;
			}
#endif
		}
#if !defined(TARGET_CONSOLE) && !defined(USE_EOS) // when debugging		
		if (Version != UBpGameplayHelperLib::GetProjectVersion()) {
			UE_LOG_ONLINE_SESSION(Warning, TEXT("MainMenuGM::OnLobbySuccess Version mismatch"));
			continue;
		}
#endif
		if (Session.OnlineResult.Session.NumOpenPublicConnections <= 0) {
			UE_LOG_ONLINE_SESSION(Verbose, TEXT("MainMenuGM::OnLobbySuccess No open connections"));
			continue;
		}

		float PingInMs = Session.OnlineResult.PingInMs;
	
#if defined(USE_EOS) || defined(TARGET_CONSOLE) // when debugging		
	DesiredResults.Add(Session);
#else 
		if (PingInMs < DesiredSessionPing)
		{
			DesiredResults.Add(Session);
		}
#endif		
	}

	TArray<FString> PreviouslyJoinedGames = UReadyOrNotStatics::GetReadyOrNotGameInstance()->PreviouslyJoinedGames;

#if !defined(TARGET_CONSOLE)
	// only remove previously joined games if they are coop lobbies and not dedis
	if (!bPVPSessionSearch && PreviouslyJoinedGames.Num() > 0)
	{
		for (int32 i = 0; i < DesiredResults.Num(); i++)
		{
			if (DesiredResults[i].OnlineResult.IsValid() && PreviouslyJoinedGames.Contains(DesiredResults[i].OnlineResult.Session.OwningUserId->ToString()))
			{
				DesiredResults.RemoveAt(i);
				if (i > 0)
					i--;
			}
		}
	}
#endif


	if (DesiredResults.Num() > 0)
	{
		DesiredResult = DesiredResults[FMath::RandRange(0, DesiredResults.Num() - 1)];
		
	}

	if (DesiredResult.OnlineResult.IsValid())
	{
#if defined(TARGET_CONSOLE)
		UReadyOrNotStatics::GetReadyOrNotGameInstance()->PreviouslyJoinedGames.AddUnique(DesiredResult.OnlineResult.Session.OwningUserName);
#else
		UReadyOrNotStatics::GetReadyOrNotGameInstance()->PreviouslyJoinedGames.AddUnique(DesiredResult.OnlineResult.Session.OwningUserId->ToString());
#endif
		AReadyOrNotPlayerController* pc = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld());
		if (pc)
		{
			FText Msg = NSLOCTEXT("MainMenu", "JoiningServer", "Joining {0}");
			
			FString GameName;
			DesiredResult.OnlineResult.Session.SessionSettings.Get(SETTING_GAMENAME, GameName);
			if (GameName.IsEmpty())
			{
				GameName = NSLOCTEXT("MainMenu", "DefaultGameName", "Game").ToString();
			}
			UReadyOrNotBackend::LogMessage("Found Public Session: Joining " + GameName + " (" + DesiredResult.OnlineResult.GetSessionIdStr() + ") Ping: " + FString::FromInt(DesiredResult.OnlineResult.PingInMs) + " ms");
			OnUpdateSessionSearch.Broadcast(true, FText::Format(Msg, FText::FromString(GameName)), bPVPSessionSearch);
			ULevelStreaming* OutStreamedLevel;
			pc->StreamInSession(DesiredResult, OutStreamedLevel, true);
		}
	}
}

void AMainMenuGM::OnDestroySessionBeforeStartingLobby()
{
	AReadyOrNotPlayerController* pc = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld());
	if (!pc)
		return;

#ifdef SUPPORTER_ONLY_BUILD
	if (!UGameFeatureLibrary::IsGameVersionEnabled(DLC_SUPPORTER))
	{
		return;
	}
#endif
	
	UReadyOrNotGameInstance* GameInstance = UReadyOrNotStatics::GetReadyOrNotGameInstance();
	if (GameInstance)
	{
		//GameInstance->MountInstalledMods();
		//GameInstance->DisableModManager();
	}	
	if (bPendingLobbyOnlineMode)
	{
		FSessionPropertyKeyPair GameMode, MapName, GameName, Version, Checksum;
		GameMode.Key = SETTING_GAMEMODE;
		GameMode.Data = *FString("Lobby");
		MapName.Key = SETTING_MAPNAME;
		MapName.Data = *FString(GetGameInstance<UReadyOrNotGameInstance>()->LobbyLevel);
		GameName.Key = SETTING_GAMENAME;
		GameName.Data = *FString(pc->GetPlayerState<APlayerState>()->GetPlayerName() + "'s Lobby");
		Version.Key = SETTING_VERSION;
		Version.Data = *FString(UBpGameplayHelperLib::GetProjectVersion());
		CreateSessionCallbackProxyAdvanced = UCreateSessionCallbackProxyAdvanced::CreateAdvancedSession(GetWorld(), {GameMode, MapName, GameName, Version}, UBpGameplayHelperLib::GetLocalPlayerController(GetWorld()), 5, 0,
			false, true, false, true, true, true, bFriendsOnlyLobby, false, false, true);
		CreateSessionCallbackProxyAdvanced->OnSuccess.AddDynamic(this, &AMainMenuGM::OnLobbySuccess);
		CreateSessionCallbackProxyAdvanced->OnFailure.AddDynamic(this, &AMainMenuGM::OnLobbyFailed);
		CreateSessionCallbackProxyAdvanced->Activate();

	} else
	{
		OnLobbySuccess();
		return;
	}
	
#if WITH_EDITOR
	OnLobbySuccess();
	return;
#endif

// Tmp fix due to delegate not triggering
#if defined(USE_EOS)
	OnLobbySuccess();
#endif
}

void AMainMenuGM::GoToCommanderMode(int32 ProfileSlot, bool bIronmanMode, bool bTutorialMode)
{
	UCommanderProfile* Profile = UCommanderProfile::LoadProfile(ProfileSlot);
	if (!Profile)
	{
		Profile = UCommanderProfile::CreateProfile(ProfileSlot, bIronmanMode);
	}
#if defined(TARGET_PS5)
	UPS5ActivitiesStatics::OnLoadGame(GetWorld(), Profile);
#endif
	
	bCommanderMode = true;
	CommanderSaveSlot = Profile->GetSlot();
	
	if (bTutorialMode)
	{
		GoToTraining();
		return;
	}

	// If the player chose not to do the tutorial, mark the profile as having completed it
	UMetaGameProfile* MetaGameProfile = UMetaGameProfile::GetProfile(GetWorld());
	if (MetaGameProfile && !MetaGameProfile->bHasCompletedTutorial)
	{
		MetaGameProfile->bHasCompletedTutorial = true;
		MetaGameProfile->SaveProfile();
	}

	GoToLobby(false, false);
}

void AMainMenuGM::ContinueCommanderMode()
{
	UMetaGameProfile* MetaGameProfile = UMetaGameProfile::GetProfile(GetWorld());
	if (!ensure(MetaGameProfile))
		return;

	UCommanderProfile* CommanderProfile = UCommanderProfile::LoadProfile(MetaGameProfile->LastCampaignSave);
	if (!CommanderProfile)
		return;
	
	bCommanderMode = true;
	CommanderSaveSlot = CommanderProfile->GetSlot();

	GoToLobby(false, false);
}

bool AMainMenuGM::CanContinueCommanderMode()
{
	// Not supported
	if (UGameFeatureLibrary::IsGameDemo())
		return false;
	
	UMetaGameProfile* Profile = UMetaGameProfile::GetProfile(GetWorld());
	if (!ensure(Profile))
		return false;

	return UCommanderProfile::LoadProfile(Profile->LastCampaignSave) != nullptr;
}

bool AMainMenuGM::HasCompletedTraining()
{
	const UMetaGameProfile* Profile = UMetaGameProfile::GetProfile(UBpGameplayHelperLib::GetWorldStatic());
	if (!ensure(Profile))
		return false;

	return Profile->bHasCompletedTutorial;
}

void AMainMenuGM::GoToTraining()
{
	AReadyOrNotPlayerController* pc = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld());
	if (!pc)
		return;

	if (MainMenuMusicInst.Instance)
	{
		MainMenuMusicInst.Instance->setParameterByName("Menu_StartGame", 1.0f);
	}
	if (MainMenuAmbienceInst.Instance)
	{
		MainMenuAmbienceInst.Instance->setParameterByName("Menu_StartGame", 1.0f);
	}

	// Set session type to singleplayer to allow pausing in training
	UReadyOrNotStatics::GetReadyOrNotGameInstance()->SessionType = ESessionType::ST_SinglePlayer;

	OnTrainingSuccess();
}

void AMainMenuGM::OnTrainingSuccess()
{
#ifdef SUPPORTER_ONLY_BUILD
	if (!UGameFeatureLibrary::IsGameVersionEnabled(DLC_SUPPORTER))
	{
		return;
	}
#endif
	
	AReadyOrNotPlayerController* pc = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld());
	if (pc)
	{
		ULevelStreaming* StreamedLevel = nullptr;

		FString Options = "";
		if (bCommanderMode)
		{
			Options = "?save=" + CommanderSaveSlot;
			
			UMetaGameProfile* MetaGameProfile = UMetaGameProfile::GetProfile(GetWorld());
			if (MetaGameProfile)
			{
				MetaGameProfile->LastCampaignSave = CommanderSaveSlot;
				MetaGameProfile->SaveProfile();
			}
		}
		else
		{
			Options = bPendingLobbyOnlineMode ? "?Listen?Port=" + FString::SanitizeFloat(FMath::RandRange(7000, 15000)) : "";
		}
		
		FLevelStreamOptions LevelStreamOptions;
		LevelStreamOptions.bShouldCreateLoadingScreen = false;
		LevelStreamOptions.bShouldRemoveLoadingScreen = false;
		LevelStreamOptions.bStreamInLevelBeforeLoad = true;

		FString Level = GetGameInstance<UReadyOrNotGameInstance>()->TrainingLevel;
		if (!pc->StreamInLevel(Level, Options, StreamedLevel, LevelStreamOptions))
		{
			IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
			if (OnlineSub)
			{
				IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
     
				if (Sessions.IsValid())
				{
					Sessions->DestroySession(NAME_GameSession);   
				
				}
			}
			const FString Message = NSLOCTEXT("MainMenu", "MainMenuFailedToFindLevel", "Failed to Create World (Level has not been found)").ToString();
			const FString ButtonText = NSLOCTEXT("MainMenu", "MainMenuOKBtn", "OK").ToString();
			ShowMessageDisplayBox(Message, ButtonText);
		}
		else
		{			
			pc->Client_CreateLoadingScreen(Level, "training", pc->GetPlayerState<APlayerState>()->GetPlayerName() + "'s Lobby");
		}
	}
}

void AMainMenuGM::GoToLobby(bool bOnlineMode, bool bFriendsOnly )
{
	AReadyOrNotPlayerController* pc = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld());
	if (!pc)
		return;

	if (MainMenuMusicInst.Instance)
	{
		MainMenuMusicInst.Instance->setParameterByName("Menu_StartGame", 1.0f);
	}
	if (MainMenuAmbienceInst.Instance)
	{
		MainMenuAmbienceInst.Instance->setParameterByName("Menu_StartGame", 1.0f);
	}

 	bPendingLobbyOnlineMode = bOnlineMode;
 	bFriendsOnlyLobby = bFriendsOnly;

	DestroySessionCallbackProxyAdvanced = UDestroySessionCallbackProxyAdvanced::DestroySession(GetWorld(), pc);
	DestroySessionCallbackProxyAdvanced->OnFailure.AddDynamic(this, &AMainMenuGM::OnDestroySessionBeforeStartingLobby);
	DestroySessionCallbackProxyAdvanced->OnSuccess.AddDynamic(this, &AMainMenuGM::OnDestroySessionBeforeStartingLobby);
	DestroySessionCallbackProxyAdvanced->Activate();

 	if (bOnlineMode)
 	{
 		UReadyOrNotStatics::GetReadyOrNotGameInstance()->SessionType = bFriendsOnly ? ESessionType::ST_Friends : ESessionType::ST_Public;
 	}
	else
 	{
 		UReadyOrNotStatics::GetReadyOrNotGameInstance()->SessionType = ESessionType::ST_SinglePlayer;
 	}
}

void AMainMenuGM::OnLobbySuccess()
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("MainMenuGM::OnLobbySuccess"));
#ifdef SUPPORTER_ONLY_BUILD
	if (!UGameFeatureLibrary::IsGameVersionEnabled(DLC_SUPPORTER))
	{
		return;
	}
#endif

	AReadyOrNotPlayerController* pc = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld());
	if (pc)
	{
		ULevelStreaming* StreamedLevel = nullptr;

		FString Options = "";
		if (bCommanderMode)
		{
			Options = "?save=" + CommanderSaveSlot;
			
			UMetaGameProfile* MetaGameProfile = UMetaGameProfile::GetProfile(GetWorld());
			if (MetaGameProfile)
			{
				MetaGameProfile->LastCampaignSave = CommanderSaveSlot;
				MetaGameProfile->SaveProfile();
			}
		}
		else
		{
			Options = bPendingLobbyOnlineMode ? "?Listen?Port=" + FString::SanitizeFloat(FMath::RandRange(7000, 15000)) : "";
		}
		
		FLevelStreamOptions LevelStreamOptions;
		LevelStreamOptions.bShouldCreateLoadingScreen = false;
		LevelStreamOptions.bShouldRemoveLoadingScreen = false;
		LevelStreamOptions.bStreamInLevelBeforeLoad = true;

		FString Level = GetGameInstance<UReadyOrNotGameInstance>()->LobbyLevel;

		if (!pc->StreamInLevel(Level, Options, StreamedLevel, LevelStreamOptions))
		{
			IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
			if (OnlineSub)
			{
				IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
     
				if (Sessions.IsValid())
				{
					Sessions->DestroySession(NAME_GameSession);   
				
				}
			}
			const FString Message = NSLOCTEXT("MainMenu", "MainMenuFailedToFindLevel", "Failed to Create World (Level has not been found)").ToString();
			const FString ButtonText = NSLOCTEXT("MainMenu", "MainMenuOKBtn", "OK").ToString();
			ShowMessageDisplayBox(Message, ButtonText);
		}
		else
		{			
			pc->Client_CreateLoadingScreen(Level, "lobby", pc->GetPlayerState<APlayerState>()->GetPlayerName() + "'s Lobby");
		}
	}
}

void AMainMenuGM::OnLobbyFailed()
{
	const FString Message = NSLOCTEXT("MainMenu", "MainMenuFailedSession", "Failed to Create Session (No Connection To Steam)").ToString();
	const FString ButtonText = NSLOCTEXT("MainMenu", "MainMenuOKBtn", "OK").ToString();
	ShowMessageDisplayBox(Message, ButtonText);

	if (MainMenuMusicInst.Instance)
	{
		MainMenuMusicInst.Instance->setParameterByName("Menu_StartGame", 0.0f);
	}

	if (MainMenuAmbienceInst.Instance)
	{
		MainMenuAmbienceInst.Instance->setParameterByName("Menu_StartGame", 0.0f);
	}
}

void AMainMenuGM::CreateAuthenticationMenu()
{
	DestroyAuthenticationMenu();

	AReadyOrNotPlayerController* pc = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld());
	if (pc)
	{
		pc->Client_CreateWidget("Authentication");
	}
}

void AMainMenuGM::DestroyAuthenticationMenu()
{
	UBpGameplayHelperLib::RemoveWidgetFromViewport("Authentication");
}

void AMainMenuGM::CreateMainMenu()
{
	DestroyMainMenu();
	bCreatedMainMenu = true;
#ifdef SUPPORTER_ONLY_BUILD
	if (!UGameFeatureLibrary::IsGameVersionEnabled(DLC_SUPPORTER))
	{
		ShowMessageDisplayBox(XorString("You must own Ready or Not: Supporter Edition to play this build."), XorString("Exit"), true);
		return;
	}
#endif

	UReadyOrNotStatics::GetReadyOrNotGameInstance()->GetTimerManager().SetTimer(TH_ShowMainMenuMsg, this, &AMainMenuGM::ShowMainMenuMsg, 2.0f);
}

void AMainMenuGM::DestroyMainMenu()
{
	UBpGameplayHelperLib::RemoveWidgetFromViewport("Main");
	UBpGameplayHelperLib::RemoveWidgetFromViewport("ModProgressOverlay");
}

AActor* AMainMenuGM::ChoosePlayerStart_Implementation(AController* Player)
{
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		APlayerStart* PlayerStart = *It;
		if (PlayerStart->PlayerStartTag == "LobbyView")
		{
			return PlayerStart;
		}
	}
	return nullptr;
}

AActor* AMainMenuGM::FindPlayerStart_Implementation(AController* Player, const FString& IncomingName /* = TEXT("") */)
{
	return ChoosePlayerStart_Implementation(Player);
}

#if defined(WITH_MODIO)
void AMainMenuGM::OnModStateChange(FModioModID Mod)
{
	// Only display this once...
	UModioManager* const ModManager = UModioManager::GetInstance();
	if(!ModManager)
		return;
	
	// if (bDisplayedRestart || !ModManager->IsRestartRequired())
	// 	return;

	// const AReadyOrNotPlayerController* PlayerController = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld());
	// if (!PlayerController)
	// 	return;

	// CreateRestartWidget();
	ModManager->SetRestartRequired();
	// bDisplayedRestart = true;
}
#endif
