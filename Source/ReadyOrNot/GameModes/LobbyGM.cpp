// Copyright Void Interactive, 2023

#include "GameModes/LobbyGM.h"

#include "LobbyGS.h"
#include "Actors/Environment/MissionSelect.h"
#include "Actors/Triggers/LobbyFiringRangeArea.h"
#include "Commander/CommanderProfile.h"
#include "Commander/MetaGameProfile.h"
#include "Commander/RosterManager.h"
#include "Info/MissionPlanManager.h"

extern TAutoConsoleVariable<int32> CVarForceCommanderMode;

ALobbyGM::ALobbyGM()
{
	urlShortName = "lobby";
}

void ALobbyGM::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	
	if (UGameplayStatics::HasOption(Options, "save"))
	{
		FString CommanderSaveSlot = UGameplayStatics::ParseOption(Options, "save");
		CommanderProfile = UCommanderProfile::LoadProfile(CommanderSaveSlot);
	}

#if WITH_EDITOR
	if (CVarForceCommanderMode.GetValueOnAnyThread() != 0)
	{
		CommanderProfile = UCommanderProfile::GetDebugProfile();
	}
#endif

	if (CommanderProfile)
	{
		RosterManager = NewObject<URosterManager>();
		RosterManager->LoadFromProfile(CommanderProfile);
	}

	UMetaGameProfile* GameProfile = UMetaGameProfile::GetProfile(GetWorld());
	if(GameProfile)
	{
		GameProfile->TotalLobbyLogins++;
	}
}

void ALobbyGM::InitGameState()
{
	Super::InitGameState();
	
	ALobbyGS* LobbyGS = Cast<ALobbyGS>(GameState);
	if (LobbyGS && UGameplayStatics::HasOption(OptionsString, "grade"))
	{
		FString Grade = UGameplayStatics::ParseOption(OptionsString, "grade");
		LobbyGS->SetPreviousMissionGrade(Grade);
	}
}

void ALobbyGM::BeginPlay()
{
	Super::BeginPlay();
	V_LOGM(LogReadyOrNot, "Started Lobby!");
	StartMatch();
	
	RespawnMode = ERespawnMode::NoRespawn; // See ALobbyGM::PlayerKilled

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		IOnlineSessionPtr SessionInt = Online::GetSessionInterface();
		if (SessionInt)
		{
			FNamedOnlineSession* Session = SessionInt->GetNamedSession(NAME_GameSession);
			if (Session != nullptr)
			{
				Session->SessionSettings.bAllowJoinInProgress = true;
				SessionInt->UpdateSession(NAME_GameSession, Session->SessionSettings);
				
			}
		}
	}

	if (CommanderProfile && CommanderProfile->bReturningFromMission)
	{
		CommanderProfile->bReturningFromMission = false;
	}

	AReadyOrNotPlayerController* PlayerController = Cast<AReadyOrNotPlayerController>(
		GEngine->GetFirstLocalPlayerController(GetWorld()));
	PlayerController->OnLoadingScreenCleared.AddUniqueDynamic(this, &ALobbyGM::OnLoadingScreenCleared);
    	
	// Mission plan is brought over from seamless travel, ensure it is cleared when returning to lobby
	if (MissionPlanManager)
		MissionPlanManager->ClearPlan();
}

void ALobbyGM::OnLoadingScreenCleared()
{
	const AReadyOrNotPlayerController* PlayerController = Cast<AReadyOrNotPlayerController>(
		GEngine->GetFirstLocalPlayerController(GetWorld()));
	if (PlayerController && CommanderProfile && CommanderProfile->TotalPlaytime == 0)
	{
		const FWidgetLookupData WidgetData = UBpGameplayHelperLib::GetWidgetDataFromLookupData("CommanderIntro", false);
		if (WidgetData.WidgetClass)
		{
			UCommonActivatableWidget* CommanderIntro = CreateWidget<UCommonActivatableWidget>(GetWorld(), WidgetData.WidgetClass);
			CommanderIntro->AddToViewport(9999);
		}
	}
}

void ALobbyGM::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	if (CommanderProfile)
	{
		FString MapName = GetWorld()->GetMapName();
		MapName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);
		MapName = MapName.ToLower();
		
		CommanderProfile->LobbySaveData.LevelName = MapName;
		CommanderProfile->LobbySaveData.bPlayerTransformSet = bLastKnownGoodPlayerTransformSet;
		CommanderProfile->LobbySaveData.PlayerLocation = LastKnownGoodPlayerTransform.GetLocation();
		CommanderProfile->LobbySaveData.PlayerRotation = LastKnownGoodPlayerTransform.GetRotation().Rotator();
		CommanderProfile->LobbySaveData.PlayerCameraRotation = LastKnownGoodPlayerCameraRotation;

		RosterManager->SaveToProfile(CommanderProfile);
		CommanderProfile->SaveProfile();
	}
}

void ALobbyGM::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (GetWorld()->IsInSeamlessTravel())
	{
		// ##UE5UPGRADE## Fix
		//V_LOGM(LogReadyOrNot, "Game mode in seamless travel!!!");
		return;
	}
	
	RemoveUnableToMigrateHostSetting();
	
	bool bAnyAI = false;
	for (TActorIterator<ACyberneticCharacter>It(GetWorld()); It; ++It)
	{
		bAnyAI = true;
		if (It->IsArrested() || It->IsArrestedAndDead())
		{
			if (It->GetLifeSpan() == 0.0f)
			{
				It->SetLifeSpan(15.0f);
			}
		}
	}

	if (!bAnyAI)
	{
		for (TActorIterator<AAISpawn>It(GetWorld()); It; ++It)
		{
			It->DoSpawn();
		}
	}

	AReadyOrNotPlayerController* PlayerController = GetWorld()->GetFirstPlayerController<AReadyOrNotPlayerController>();
	if (PlayerController)
	{
		APlayerCharacter* PlayerCharacter = PlayerController->GetPawn<APlayerCharacter>();
		if (PlayerCharacter && !PlayerCharacter->IsDeadOrUnconscious() &&
			PlayerCharacter->GetVelocity().IsNearlyZero(5.0f))
		{
			bLastKnownGoodPlayerTransformSet = true;
			LastKnownGoodPlayerTransform = PlayerController->GetPawn()->GetActorTransform();
			LastKnownGoodPlayerCameraRotation = PlayerController->GetControlRotation();
		}
	}
	
	// wait till we receive the loadout from the client then instantly spawn them!
	for (TActorIterator<AReadyOrNotPlayerController> It(GetWorld()); It; ++It)
	{
		AReadyOrNotPlayerController* PC = *It;
		if (!InitalizedPlayerControllers.Contains(PC))
		{
			V_LOGM(LogReadyOrNot, "PC GameTimeSinceCreation: %f", PC->GetGameTimeSinceCreation())
		}

		// Ignore already processed or not yet valid player controllers
		if (InitalizedPlayerControllers.Contains(PC) || !PC->GetRoNPlayerState() || !PC->GetRoNPlayerState()->GetLoadout().IsValid())
			continue;

		APlayerCharacter* SpawnedCharacter = nullptr;

		// Prioritize spawning at last known location if this is commander mode
		if (CommanderProfile && GetWorld()->GetFirstPlayerController<AReadyOrNotPlayerController>() == *It)
		{
			// Wait until the roster selection has been completed before spawning player (not functionally important)
			if (UBpGameplayHelperLib::HasWidgetInViewport("RosterSelection") ||
				UBpGameplayHelperLib::HasWidgetInViewport("RosterReview"))
				continue;
			
			if (CommanderProfile && CommanderProfile->LobbySaveData.IsValid())
			{
				const FLobbySaveData& LobbySaveData = CommanderProfile->LobbySaveData;

				FString MapName = GetWorld()->GetMapName();
				MapName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);
				MapName = MapName.ToLower();

				// Only spawn character if the last lobby level matched
				if (MapName == LobbySaveData.LevelName)
				{
					FTransform SpawnTransform(LobbySaveData.PlayerRotation, LobbySaveData.PlayerLocation);
				
					SpawnedCharacter = SpawnPlayerCharacter(PC, BlueCharacterClass.LoadSynchronous(), SpawnTransform);
					It->SetControlRotation(LobbySaveData.PlayerCameraRotation);
				}
			}
		}
		
		// Otherwise try spawning using a player start
		if (!SpawnedCharacter)
		{
			TArray<APlayerStart*> PossibleStarts;
			for (TActorIterator<APlayerStart> PlayerStartIt(GetWorld()); PlayerStartIt; ++PlayerStartIt)
			{
				if (PlayerStartIt->PlayerStartTag == "LobbyStart")
				{
					PossibleStarts.Add(*PlayerStartIt);
				}
			}

#if WITH_EDITOR
			// Try spawning on the "current camera location" start if it exists
			for (TActorIterator<APlayerStartPIE> PlayerStartIt(GetWorld()); PlayerStartIt; ++PlayerStartIt)
			{
				PossibleStarts.Empty();
				PossibleStarts.Add(*PlayerStartIt);
			}
#endif

			if (PossibleStarts.Num() > 0)
			{
				AActor* Start = PossibleStarts[FMath::RandRange(0, PossibleStarts.Num() - 1)];
				SpawnedCharacter = SpawnPlayerCharacter(PC, BlueCharacterClass.LoadSynchronous(), Start->GetTransform());
			}
		}

		if (SpawnedCharacter)
		{
			PC->GetRoNPlayerState()->bIsInGame = true;
			InitalizedPlayerControllers.AddUnique(PC);
			return;
		}
	}
}

void ALobbyGM::RemoveUnableToMigrateHostSetting()
{
	if (GetGameTimeSinceCreation() > 30.0f)
	{
		IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
		if (OnlineSub)
		{
			IOnlineSessionPtr SessionInt = Online::GetSessionInterface();
			if (SessionInt)
			{
				FNamedOnlineSession* Session = SessionInt->GetNamedSession(NAME_GameSession);
				if (Session != nullptr)
				{
					FString Guid;
					Session->SessionSettings.Get(MIGRATION_GUID, Guid);
					if (!Guid.IsEmpty())
					{
						Session->SessionSettings.Remove(MIGRATION_GUID);
						SessionInt->UpdateSession(NAME_GameSession, Session->SessionSettings);
					}
				}
			}
		}
	}
	
}

void ALobbyGM::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	if (!bIsServerTraveling)
	{
		int32 MaxPlayers = 5;
		IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
		if (OnlineSub)
		{
			IOnlineSessionPtr SessionInt = Online::GetSessionInterface();
			if (SessionInt)
			{
				if (SessionInt->GetSessionSettings(NAME_GameSession))
				{
					MaxPlayers = SessionInt->GetSessionSettings(NAME_GameSession)->NumPublicConnections;
				}
			}
		}
		
		int32 PlayersInGame = 0;
		for (TActorIterator<APlayerController> It(GetWorld()); It; ++It)
		{
			PlayersInGame++;	
		}

		if (PlayersInGame - 1 < MaxPlayers)
		{
			if (OnlineSub)
			{
				IOnlineSessionPtr SessionInt = Online::GetSessionInterface();
				if (SessionInt)
				{
					FNamedOnlineSession* Session = SessionInt->GetNamedSession(NAME_GameSession);
					if (Session != nullptr)
					{
						Session->SessionSettings.bAllowJoinInProgress = true;
						SessionInt->UpdateSession(NAME_GameSession, Session->SessionSettings);
					}
				}
			}
		}		
	}
}

void ALobbyGM::ProcessServerTravel(const FString& URL, bool bAbsolute)
{
	bIsServerTraveling = true;
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		IOnlineSessionPtr SessionInt = Online::GetSessionInterface();
		if (SessionInt)
		{
			FNamedOnlineSession* Session = SessionInt->GetNamedSession(NAME_GameSession);
			if (Session != nullptr)
			{
				Session->SessionSettings.bAllowJoinInProgress = false;
				SessionInt->UpdateSession(NAME_GameSession, Session->SessionSettings);
			}
		}
	}
	
	Super::ProcessServerTravel(URL, bAbsolute);
}

bool ALobbyGM::ShouldAlertSuspectWhenLastAlive() const
{
	return false;
}

bool ALobbyGM::ShouldAlertCivilianWhenLastAlive() const
{
	return false;
}

void ALobbyGM::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId,
						FString& ErrorMessage)
{
	 Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
	if (ErrorMessage.IsEmpty())
	{
		int32 MaxPlayers = 5;
		IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
		if (OnlineSub)
		{
			IOnlineSessionPtr SessionInt = Online::GetSessionInterface();
			if (SessionInt)
			{
				if (SessionInt->GetSessionSettings(NAME_GameSession))
				{
					MaxPlayers = SessionInt->GetSessionSettings(NAME_GameSession)->NumPublicConnections;
				}
			}
		}
		
		int32 PlayersInGame = 0;
		for (TActorIterator<APlayerController> It(GetWorld()); It; ++It)
		{
			PlayersInGame++;	
		}

		if (PlayersInGame + 1 > MaxPlayers)
		{
			if (OnlineSub)
			{
				IOnlineSessionPtr SessionInt = Online::GetSessionInterface();
				if (SessionInt)
				{
					FNamedOnlineSession* Session = SessionInt->GetNamedSession(NAME_GameSession);
					if (Session != nullptr)
					{
						Session->SessionSettings.bAllowJoinInProgress = false;
						SessionInt->UpdateSession(NAME_GameSession, Session->SessionSettings);
					}
				}
			}
		}		
	}
}

void ALobbyGM::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	
	if (AReadyOrNotPlayerController* PC = Cast<AReadyOrNotPlayerController>(NewPlayer))
	{
		if (const IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get())
		{
			const IOnlineSessionPtr SessionSubsystem = OnlineSubsystem->GetSessionInterface();
			if (SessionSubsystem.Get())
			{
				if (const FNamedOnlineSession* Session = SessionSubsystem->GetNamedSession(NAME_GameSession))
				{
					if (Session->SessionInfo.Get())
					{
						const FString OnlineSessionId = Session->SessionInfo->GetSessionId().ToString();
						PC->ClientJoinVoice(OnlineSessionId, 0);
					}
				}
			}
		}
	}
}

void ALobbyGM::PlayerKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter)
{
	if (KilledCharacter)
	{
		KilledCharacter->SetLifeSpan(10.0f);
		
		for (TActorIterator<AReadyOrNotPlayerController> It(GetWorld()); It; ++It)
		{
			if (*It == KilledCharacter->GetController())
			{
				#if WITH_EDITOR
				for (const TActorIterator<APlayerStartPIE> PlayerStartIt(GetWorld()); PlayerStartIt;)
				{
					SpawnPlayerCharacter(*It, BlueCharacterClass.LoadSynchronous(), PlayerStartIt->GetTransform());
					return;
				}
				#endif

				APlayerStart* KillhouseStart = nullptr;
				APlayerStart* LobbyStart = nullptr;
				for (TActorIterator<APlayerStart> PlayerStartIt(GetWorld()); PlayerStartIt; ++PlayerStartIt)
				{
					if (PlayerStartIt->PlayerStartTag == "DeathStart")
						KillhouseStart = *PlayerStartIt;

					if (PlayerStartIt->PlayerStartTag == "LobbyStart")
						LobbyStart = *PlayerStartIt;
				}

				// If we were killed by another player, try the killhouse spawn, otherwise use the default one
				if (InstigatorCharacter && KillhouseStart)
				{
					SpawnPlayerCharacter(*It, BlueCharacterClass.LoadSynchronous(), KillhouseStart->GetTransform());
				}
				else if (LobbyStart)
				{
					SpawnPlayerCharacter(*It, BlueCharacterClass.LoadSynchronous(), LobbyStart->GetTransform());
				}
				return;
			}
		}
	}
}

bool ALobbyGM::CanTakeDamage(AController* EventInstigator, AController* DamageReceiver) const
{
	// no damage in the lobby bois!
	if (DamageReceiver && !DamageReceiver->IsPlayerController())
		return true;
	if (EventInstigator && DamageReceiver && EventInstigator != DamageReceiver)
	{
		if (ALobbyFiringRangeArea::IsInFiringRange(EventInstigator->GetPawn()) && ALobbyFiringRangeArea::IsInFiringRange(DamageReceiver->GetPawn()))
		{
			return true;
		}		
		return false;
	}
	
	// unless you're dying to smth random
	return true;
}

void ALobbyGM::OpenLobbyWidget(FString Name)
{
	if (!GetWorld() || !GetGameInstance())
		return;

	if (UBpGameplayHelperLib::HasWidgetInViewport(Name))
		return;
	
	AReadyOrNotPlayerController* PlayerController = Cast<AReadyOrNotPlayerController>(GetGameInstance()->GetFirstLocalPlayerController());
	if (PlayerController)
		PlayerController->Client_CreateWidget(Name);
}

void ALobbyGM::OpenRosterSelection()
{
	OpenLobbyWidget("RosterSelection");
}

void ALobbyGM::OpenMissionSelect()
{
	for (TActorIterator<AMissionSelect> It(GetWorld()); It; ++It)
	{
		AMissionSelect* MissionSelect = *It;
		if (!IsValid(MissionSelect))
			continue;

		MissionSelect->OpenMissionSelect();
	}
}

void ALobbyGM::GenericPlayerInitialization(AController* C)
{
	Super::GenericPlayerInitialization(C);

	if (AReadyOrNotPlayerController* ReadyOrNotPlayerController = Cast<AReadyOrNotPlayerController>(C))
	{
		OnGenericPlayerInitialization.Broadcast(ReadyOrNotPlayerController);
	}
}
