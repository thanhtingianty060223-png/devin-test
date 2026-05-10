// Copyright Void Interactive, 2023

#include "ReadyOrNotGameMode.h"
#include "ReadyOrNotGameSession.h"

#include "Actors/Gameplay/ReadyOrNotPlayerState.h"
#include "Actors/DynamicWorldActor.h"
#include "Actors/Vehicles/UnmannedVehicle.h"

#include "Characters/ReadyOrNotPlayerController.h"
#include "Characters/SpectatePawn.h"
#include "Characters/CyberneticCharacter.h"

#include "Components/CharacterHealthComponent.h"

#include "NavigationSystem.h"

#include "Actors/Door.h"
#include "Actors/Gameplay/EvidenceActor.h"
#include "Actors/Gameplay/TrapActorAttachedToDoor.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Characters/CyberneticController.h"
#include "Characters/AI/SWATCharacter.h"
#include "Characters/AI/TrailerSWATCharacter.h"
#include "Components/DestructibleDoorChunkComponent.h"
#include "Info/MapStatisticsSystem.h"
#include "Info/MissionPlanManager.h"
#include "Info/ScoringManager.h"
#include "Info/TOCManager.h"
#include "lib/ReadyOrNotFunctionLibrary.h"
#include "lib/CompetitionHelperLib.h"
#if defined(TARGET_CONSOLE)
#include "Subsystems/ConsoleMultiplayerSubsystem.h"
#include "lib/ReadyOrNotConsoleFunctionLibrary.h"
#endif

TAutoConsoleVariable<int32> CVarRonStartGameInstantly(TEXT("a.RonStartGameInstantly"), 1, TEXT("Starts the game instantly for debugging purposes"));
TAutoConsoleVariable<float> CVarRonEditorLoadoutCountdown(TEXT("a.RonEditorLoadoutCountdown"), 2.5, TEXT("Time for loadout to countdown"));
TAutoConsoleVariable<float> CVarRonEditorLoadoutShowTruckScene(TEXT("a.RonEditorLoadoutShowTruckScene"), 0, TEXT("Lock the camera on the truck scene"));

DECLARE_CYCLE_STAT(TEXT("RoN ~ GM BeginPlay"), STAT_RoNGameModeBeginPlay, STATGROUP_RONGM);
DECLARE_CYCLE_STAT(TEXT("RoN ~ GM Tick"), STAT_RoNGameModeTick, STATGROUP_RONGM);

AReadyOrNotGameMode::AReadyOrNotGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickInterval = 0.0167f;
	bUseSeamlessTravel = true;
	
	GameSessionClass = AReadyOrNotGameSession::StaticClass();

	PlayerStartClass = APlayerStart::StaticClass();

#if WITH_EDITOR
	if (IsInGameThread())
	{
		GameModeSettings.DataTable =  UBpGameplayHelperLib::GetGameModeSettingsLookupDataTable();
		GameModeSettings.RowName = "Default";
	}
#endif
}

void AReadyOrNotGameMode::BeginPlay()
{
	Super::BeginPlay();

	SCOPE_CYCLE_COUNTER(STAT_RoNGameModeBeginPlay);

	#if !WITH_EDITOR
	UReadyOrNotStatics::GetReadyOrNotGameInstance()->ReadyOrNotBackend->OnCheckedBanStatus.AddDynamic(this, &AReadyOrNotGameMode::OnBanStatusChecked);
	#endif

	#ifdef RONEngine
	V_LOGM(LogReadyOrNotInit, "RONEngine Initialized.");
	#endif

	/*
	if (USteamworksIntegration::IsDLCInstalled(STEAM_DLC_PREORDER_BONUS)) V_LOGM(LogReadyOrNotInit, "Ready Or Not - Pre-Order Bonuses DLC Loaded");
	if (USteamworksIntegration::IsDLCInstalled(STEAM_DLC_SUPPORTER)) V_LOGM(LogReadyOrNotInit, "Ready Or Not - Supporter Edition DLC Loaded");
	if (USteamworksIntegration::IsDLCInstalled(888332)) V_LOGM(LogReadyOrNotInit, "Obviously Fake DLC Loaded");
	*/

	if (GetWorld())
	{
		if (AReadyOrNotGameState* state = GetWorld()->GetGameState<AReadyOrNotGameState>())
		{
			state->ModeURL_Replicated = urlShortName;
			state->RoundsToPlay = GetGameModeSettings().RoundsPerMap;

			state->ScoringManager = GetWorld()->SpawnActor<AScoringManager>();
			state->ScoringManager->SetOwner(this);
			state->ScoringManager->SetReplicates(true);
			state->ScoringManager->bIsOfficialScoring = true;

			// Create the level objectives now that the scoring manager has been created
			state->CreateLevelObjectives();
			
			state->TOCManager = GetWorld()->SpawnActor<ATOCManager>();
			state->TOCManager->SetOwner(this);
			state->TOCManager->SetReplicates(true);
		}
	}

#if defined(TARGET_CONSOLE)
	UReadyOrNotConsoleFunctionLibrary::ConsoleApplyLevelSpecificSettings("", false);
#else
	UReadyOrNotGameUserSettings* gsettings = Cast<UReadyOrNotGameUserSettings>(GEngine->GameUserSettings);
	if (gsettings)
	{
		gsettings->ApplyCommandLineOverrides();
	}
#endif

	AReadyOrNotGameSession* Session = Cast<AReadyOrNotGameSession>(GameSession);
	if (Session)
	{
		Session->RoundsPerMap = GetGameModeSettings().RoundsPerMap;
		if (UBpGameplayHelperLib::GetLocalPlayerController(GetWorld()))
		{
			UBpGameplayHelperLib::GetLocalPlayerController(GetWorld())->SetNetSpeed(Session->ClientNetSpeed);
		}

		AReadyOrNotGameState* state = Cast<AReadyOrNotGameState>(GetWorld()->GetGameState());
		if (state)
			state->RoundTimeRemaining = Session->Timelimit;
	}
	
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		IOnlineSessionPtr SessionInt = Online::GetSessionInterface();
		if (SessionInt)
		{
#if defined(TARGET_PS5)
			if (SessionInt->GetSessionSettings(NAME_PartySession))
			{
				V_LOGM(LogReadyOrNot, "Created Session (Max Players: %d)", SessionInt->GetSessionSettings(NAME_PartySession)->NumPublicConnections);
			}
#else
			if (SessionInt->GetSessionSettings(NAME_GameSession))
			{
				V_LOGM(LogReadyOrNot, "Created Session (Max Players: %d)", SessionInt->GetSessionSettings(NAME_GameSession)->NumPublicConnections);
			}
#endif
		}
	}
	
	// Clear out the list of whitelisted dynamic level generation labels and regenerate them.
	AReadyOrNotGameState* gs = GetGameState<AReadyOrNotGameState>();
	if (gs == nullptr)
	{
		return;
	}
	gs->WhitelistedLabels.Empty();

	FLevelDataLookupTable LevelData = UBpGameplayHelperLib::GetLevelData(GetWorld());
	for (int32 i = 0; i < LevelData.DynamicLevelSpawnRosters.Num(); i++)
	{
		FDynamicLevelRoster& Roster = LevelData.DynamicLevelSpawnRosters[i];

		if (Roster.MaximumPicks < Roster.MinimumPicks)
		{
			// Fix the roster in case the artist made a mistake.
			int32 NewMax = Roster.MinimumPicks;
			Roster.MinimumPicks = Roster.MaximumPicks;
			Roster.MaximumPicks = NewMax;
		}

		if (Roster.SpawnLevel.Num() <= 0 || Roster.MaximumPicks <= 0)
		{
			// Not allowed to spawn anything - the artist specified invalid data.
			continue;
		}

		int32 NumThisRoster = FMath::RandRange(Roster.MinimumPicks, Roster.MaximumPicks);
		if (NumThisRoster <= 0)
		{
			// Not allowed to spawn anything - we rolled 0.
			continue;
		}

		if (FMath::RandRange(0.0f, 1.0f) > Roster.OverallSpawnPercent)
		{
			// Not allowed to spawn anything - we rolled higher than the overall spawn percent.
			continue;
		}

		// Iterate through all of the spawnables, and add up the chances.
		int32 OverallChance = 0;
		for (int32 j = 0; j < Roster.SpawnLevel.Num(); j++)
		{
			OverallChance += Roster.SpawnLevel[j].Chance;
		}

		// Pick a random number from 0 to Chance. Then loop through the SpawnLevel of the roster to find which one we got.
		int32 ChancePick = FMath::RandRange(0, OverallChance);
		int32 ChanceIterator = 0;
		for (int32 j = 0; j < Roster.SpawnLevel.Num(); j++)
		{
			if (ChanceIterator + Roster.SpawnLevel[j].Chance >= ChancePick)
			{
				gs->WhitelistedLabels.Add(Roster.SpawnLevel[j].Label);
				break;
			}
			ChanceIterator += Roster.SpawnLevel[j].Chance;
		}
	}

#if WITH_EDITOR
	if (GEditor)
	{
		if (GEditor->IsSimulatingInEditor())
		{
			StartMatch();
			GEngine->Exec(GetWorld(), TEXT("a.RonPauseSignificance 1"));
		}
	}
#endif

#if defined(TARGET_PS5)
	ESessionType SessionType = UReadyOrNotStatics::GetReadyOrNotGameInstance()->SessionType;
	if (SessionType == ESessionType::ST_Public)
	{
		UConsoleMultiplayerStatics::FindPublicPlayers(GetWorld(),UConsoleMultiplayerStatics::GetMaxPlayers(GetWorld()));
	}
#endif
#if defined(TARGET_XB1) || defined(TARGET_XSX)
	ESessionType SessionType = UReadyOrNotStatics::GetReadyOrNotGameInstance()->SessionType;
	if (SessionType != ESessionType::ST_SinglePlayer)
	{
		UConsoleMultiplayerStatics::UpdateSessionConnectString(GetWorld());
	}
#endif
}

void AReadyOrNotGameMode::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	SCOPE_CYCLE_COUNTER(STAT_RoNGameModeTick);

	AReadyOrNotGameState* gs = Cast<AReadyOrNotGameState>(GetWorld()->GetGameState());
	if (!gs)
		return;
	
	if (CurrentMatchState == EMatchState::MS_MatchEnded)
	{
		if (bShouldUseCountdown)
		{
			TimeRemainingToNextMap -= DeltaSeconds;
			gs->ServerTimeUntilNextMap = TimeRemainingToNextMap;
			if (TimeRemainingToNextMap <= 0.0f)
			{
				NextGame();				
			}
		}
	}

	AReadyOrNotGameSession* Session = Cast<AReadyOrNotGameSession>(GameSession);
	if (Session)
	{
		// for ready or not competition server

		//set password from session config
		EventID = Session->EventID;
		if (Password.IsEmpty() && !Session->Password.IsEmpty())
		{
			SetPassword(Session->Password);
			V_LOGM(LogReadyOrNot, "Setting Password to %s", *Password);
		}

		// set rounds from session config
		//gs->RoundsToPlay = Session->RoundsPerMap;
	}

	// for ready or not competition server
#if WITH_EDITOR
	EventID = 10;
#endif

	gs->bRunWarmup = bRunWarmup;
	gs->MatchState = GetMatchState();

	// Check Starting Conditions......
	if (GetMatchState() == EMatchState::MS_Warmup && bRunWarmup)
	{
		// Temp set to true, set to false if there is an unready player
		bool bAllPlayersReady = true;

		if (GetNumPlayers() == 0)
			return;

		TArray<APlayerState*> Players = gs->PlayerArray;
		for (int32 i = 0; i < Players.Num(); i++)
		{
			AReadyOrNotPlayerState* ps = Cast<AReadyOrNotPlayerState>(Players[i]);
			if (ps)
			{
				if (!ps->IsABot() && !ps->IsSpectator() && !ps->IsOnlyASpectator())
				{
					if (!ps->bReady)
					{
						bAllPlayersReady = false;
						gs->TimeTillGameStartCountdown = gs->GetClass()->GetDefaultObject<AReadyOrNotGameState>()->TimeTillGameStartCountdown;
					}
				}
			}
		}

		#if WITH_EDITOR
		MinimumPlayersToStart = 1;
		#endif

		if (Players.Num() >= MinimumPlayersToStart)
		{
			#if WITH_EDITOR
			const bool bStartGameInstantly = CVarRonStartGameInstantly.GetValueOnAnyThread() == 1 && !GetWorld()->GetNetDriver();
			#else
			const bool bStartGameInstantly = false;
			#endif
			
			if (bStartGameInstantly)
			{
				for (int32 i = 0; i < Players.Num(); i++)
				{
					AReadyOrNotPlayerState* ps = Cast<AReadyOrNotPlayerState>(Players[i]);
					if (ps)
					{
						//ps->Server_SetLoadout(DefaultLoadoutIfNothingLoaded);
						ps->bReady = true;
					}
				}
				
				StartMatch();
			}

			if (GetGameTimeSinceCreation() > 5.0f && HasEveryoneFinishedLoading())
			{
				for (int32 i = 0; i < Players.Num(); i++)
				{
					AReadyOrNotPlayerState* ps = Cast<AReadyOrNotPlayerState>(Players[i]);
					if (ps)
					{
						ps->bReady = true;
					}
				}
				
				gs->bWaitingForPlayers = false;

				#if WITH_EDITOR
				// add a slight delay to give clients a chance to connect first
				gs->TimeTillGameStartCountdown = CVarRonEditorLoadoutCountdown.GetValueOnAnyThread() - GetGameTimeSinceCreation();
				if (CVarRonEditorLoadoutShowTruckScene.GetValueOnAnyThread() == 1)
				{
					gs->TimeTillGameStartCountdown = 5.0f;
				}
				#endif

				
				gs->TimeTillGameStartCountdown -= DeltaSeconds;
				UHostMigrationManager* HostMigrationManager = UReadyOrNotStatics::GetReadyOrNotGameInstance()->HostMigrationManager;
				if (HostMigrationManager && HostMigrationManager->IsMigratingHost())
				{
					if (gs->PlayerArray.Num() < HostMigrationManager->ExpectedPlayerCount && GetGameTimeSinceCreation() < 60.0f)
					{
						// don't remove any time!
						gs->TimeTillGameStartCountdown += DeltaSeconds;
					}
				}
				
				if (gs->TimeTillGameStartCountdown <= 0.0f)
				{
					StartMatch();
				}
			}
			else if (!bAllPlayersReady && Players.Num() >= MinimumPlayersForTimer)
			{
				gs->TimeTillGameStartCountdown = gs->GetClass()->GetDefaultObject<AReadyOrNotGameState>()->TimeTillGameStartCountdown;
				if (Players.Num() >= MinimumPlayersForTimer)
				{
					gs->bWaitingForPlayers = false;
					gs->PlanningTimeLeft = FMath::Max(gs->PlanningTimeLeft - DeltaSeconds, 0.0f);
					if (!bPlayedMLOCountdownAnnouncement && gs->PlanningTimeLeft < 10.0f)
					{
						bPlayedMLOCountdownAnnouncement = true;
						gs->PlayAnnouncerForTeam("CountdownFrom10", ETeamType::TT_SERT_RED);
					}
					if (!bPlayedSWATCountdownAnnouncement && gs->PlanningTimeLeft < 10.0f)
					{
						bPlayedSWATCountdownAnnouncement = true;
						gs->PlayAnnouncerForTeam("CountdownFrom10", ETeamType::TT_SERT_BLUE);
					}
				}
				else
				{
					gs->bWaitingForPlayers = true;
				}
			}
		}
	}

	if (CurrentMatchState == EMatchState::MS_Playing)
	{
		TimeSinceGameStart += DeltaSeconds;

		// tick AI
		{
			uint8 NumTickedThisFrame = 0;
			
			while (CurrentAIIndex < gs->AllAICharacters.Num())
			{
				const ACyberneticCharacter* AI = gs->AllAICharacters[CurrentAIIndex];
				
				if (AI && AI->GetCyberneticsController() && !AI->IsOnSWATTeam())
				{
					ACyberneticController* Controller = AI->GetCyberneticsController();
					const EAIAwarenessState AwarenessState = Controller->GetAwarenessState();

					if (AI->IsActiveForThinking())
					{
						// update tick rate based on awareness state
						if (AwarenessState == EAIAwarenessState::Alerted)
						{
							Controller->UtilityAIThinkRate = 0.05f;
							Controller->SetActorTickInterval(0.05f);
						}
						else if (AwarenessState == EAIAwarenessState::Suspicious)
						{
							Controller->UtilityAIThinkRate = 0.1f;
							Controller->SetActorTickInterval(0.1f);
						}
						else
						{
							Controller->UtilityAIThinkRate = 0.2f;
							Controller->SetActorTickInterval(0.2f);
						}
						
						if (Controller->TickUtilityAI(DeltaSeconds))
						{
							NumTickedThisFrame++;
							
							if (NumTickedThisFrame >= 10)
								break;
						}
					}
				}
				
				CurrentAIIndex++;
			}
			
			if (CurrentAIIndex >= gs->AllAICharacters.Num())
			{
				CurrentAIIndex = 0;

				LOCAL_PLAYER;
				if (LocalPlayer)
				{
					gs->AllAICharacters.Sort([&](const ACyberneticCharacter& Lhs, const ACyberneticCharacter& Rhs)
					{
						return FVector::DistSquared(LocalPlayer->GetActorLocation(), Lhs.GetActorLocation()) < FVector::DistSquared(LocalPlayer->GetActorLocation(), Rhs.GetActorLocation());
					});
				}
			}
		}
	}
	
	if (!bHasEverHadControllerInGame)
	{
		// slight optimization, we don't need to do this if we already know we had a controller in the game --eez
		for (TActorIterator<APlayerController> It(GetWorld()); It;)
		{
			bHasEverHadControllerInGame = true;
			break;
		}
	}
	
	if (bHasEverHadControllerInGame && GetNumPlayers() == 0)
	{
		#if WITH_EDITOR
		if (GEditor->IsSimulatingInEditor())
		{
			return;
		}
		#endif
		
		FString LobbyLevel = GetGameInstance<UReadyOrNotGameInstance>()->LobbyLevel;
		ProcessServerTravel(LobbyLevel + "?game=lobby");
	}
}

void AReadyOrNotGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	if (MapName.IsEmpty() || (MapName.Contains("MainMenu") && (GetWorld()->GetNetMode() == NM_DedicatedServer)))
	{
		V_LOGM(LogReadyOrNot, "Using Map List.. loaded into entry map (%s) going to next map..", *MapName);
		NextGame();
	}
	FString ProjectVersionStr = UBpGameplayHelperLib::GetProjectVersion();
	ProjectVersionStr = ProjectVersionStr.Replace(TEXT("."), TEXT(""));
	int32 nvn = FCString::Atoi(*ProjectVersionStr);
	V_LOGM(LogReadyOrNot, "Using NetworkVersionOveride: NETWORK_VERSION_NUMBER: %d", nvn);

	if (GetWorld()->GetNetMode() == NM_ListenServer)
	{
		GetReadyOrNotGameSession()->MapList = {MapName + Options};
	}
}

void AReadyOrNotGameMode::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	if (!MissionPlanManager && GetWorld())
	{
		MissionPlanManager = GetWorld()->SpawnActor<AMissionPlanManager>();
	}
}

void AReadyOrNotGameMode::GetSeamlessTravelActorList(bool bToTransition, TArray<AActor*>& ActorList)
{
	Super::GetSeamlessTravelActorList(bToTransition, ActorList);

	if (MissionPlanManager)
	{
		ActorList.Add(MissionPlanManager);
	}
	
	V_LOGM(LogReadyOrNot, "Transferring %d Actors in seamless travel", ActorList.Num() );
	for (AActor* Actor : ActorList)
	{
		V_LOGM(LogReadyOrNot, "Moving %s to new world", *Actor->GetName());
	}
}

void AReadyOrNotGameMode::OnBanStatusChecked(FString SteamId, bool bIsBanned, FString BanReason, bool bIsMySteamId)
{
	if (bIsBanned)
	{
		if (bIsMySteamId)
		{
			UReadyOrNotStatics::GetReadyOrNotPlayerController()->ClientReturnToMainMenuWithTextReason(FText::FromString(""));
		} else
		{
			BannedSteamIds.Add(SteamId, BanReason);
			for (TActorIterator<AReadyOrNotPlayerController>It(GetWorld()); It; ++It)
			{
				APlayerState* ps = It->GetRoNPlayerState();
#if UE_BUILD_DEVELOPMENT
				V_LOGM(LogReadyOrNot, "Banned User Found! Checking %s == %s", *ps->GetUniqueId().ToString(), *SteamId);
#endif
				if (ps && ps->GetUniqueId().ToString() == SteamId)
				{
					It->ClientReturnToMainMenuWithTextReason(FText::FromString(BanReason));
				}
			}
		}
	}

}

bool AReadyOrNotGameMode::ShouldAlertSuspectWhenLastAlive() const
{
	return true;
}

bool AReadyOrNotGameMode::ShouldAlertCivilianWhenLastAlive() const
{
	return true;
}

bool AReadyOrNotGameMode::HasEveryoneFinishedLoading()
{
	int32 PlayersInGame = 0, PlayersLoading = 0, PlayersFinishedLoading = 0;

	for (TActorIterator<AReadyOrNotPlayerState> It(GetWorld()); It; ++It)
	{
		if (IsValid(*It) && It->GetUniqueId().IsValid())
		{
			It->bHasFinishedLoading ? PlayersFinishedLoading++ : PlayersLoading++;
			PlayersInGame++;	
		}
	}

	#if !UE_BUILD_SHIPPING
	FString DebugStr = "(PlayerState) Players {0} Loading {1} Finished Loading {2}";
	DebugStr = FString::Format(*DebugStr, {PlayersInGame, PlayersLoading, PlayersFinishedLoading});
	GEngine->AddOnScreenDebugMessage(11988, 2.0f, FColor::White, DebugStr);
	#endif

	return PlayersInGame == PlayersFinishedLoading;
}

void AReadyOrNotGameMode::RefreshSession()
{
	AReadyOrNotGameSession* Session = Cast<AReadyOrNotGameSession>(GameSession);
	if (Session)
	{
		FJoinabilitySettings OutSettings;
		Session->GetSessionJoinability(NAME_GameSession, OutSettings);
		Session->UpdateSessionJoinability(NAME_GameSession, OutSettings.bPublicSearchable, OutSettings.bAllowInvites, OutSettings.bJoinViaPresence, OutSettings.bJoinViaPresenceFriendsOnly);
		Session->UpdateServerDetails("", "");
	}
}

void AReadyOrNotGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (UReadyOrNotGameInstance* gi = Cast<UReadyOrNotGameInstance>(GetWorld()->GetGameInstance()))
	{
		if (gi->HostMigrationManager)
			gi->HostMigrationManager->SetHostMigrationInProgress(false);
	}
}

AReadyOrNotGameSession* AReadyOrNotGameMode::GetReadyOrNotGameSession()
{
	return Cast<AReadyOrNotGameSession>(GameSession);
}

AReadyOrNotGameState* AReadyOrNotGameMode::GetReadyOrNotGameState()
{
	return GetGameState<AReadyOrNotGameState>();
}

FGameModeSettings AReadyOrNotGameMode::GetGameModeSettings()
{
	UDataTable* dt = UBpGameplayHelperLib::GetGameModeSettingsLookupDataTable();
	if (dt)
	{
		FGameModeSettings* data = GameModeSettings.GetRow<FGameModeSettings>("GameModeSettingsLookupData");
		if (data)
		{
			return *data;
		}
	}
	return FGameModeSettings();
}

void AReadyOrNotGameMode::RespawnPlayerInMatch(APlayerController* Player)
{

	if (Player && !bCanRespawn)
	{
		
		Player->PlayerState->SetIsOnlyASpectator(true);
		RestartPlayer(Player);
	}
	else
	{	
		
		RestartPlayer(Player);
	}
}

void AReadyOrNotGameMode::RestartPlayer(AController* NewPlayer)
{
	Super::RestartPlayer(NewPlayer);
}

void AReadyOrNotGameMode::RestartPlayerAtPlayerStart(AController* NewPlayer, AActor* StartSpot)
{
	Super::RestartPlayerAtPlayerStart(NewPlayer, StartSpot);
}

void AReadyOrNotGameMode::RestartPlayerAtTransform(AController* NewPlayer, const FTransform& SpawnTransform)
{
	Super::RestartPlayerAtTransform(NewPlayer, SpawnTransform);
}

void AReadyOrNotGameMode::FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation)
{
	if (Cast<APlayerController>(NewPlayer))
	{
		FTransform spawnTransform;
		AActor* startSpot = FindPlayerStart(NewPlayer);
		if (NewPlayer->GetPawn())
		{
			 spawnTransform = NewPlayer->GetPawn()->GetTransform();
			 NewPlayer->GetPawn()->Destroy();
			 NewPlayer->UnPossess();
		}
		else if (startSpot)
		{
			spawnTransform = startSpot->GetActorTransform();
		}

		//RespawnPlayer(Cast<APlayerController>(NewPlayer)); //todo: check if commenting this doesnt break anything
 
		if (NewPlayer->GetPawn())
		{
			NewPlayer->GetPawn()->SetActorTransform(spawnTransform);
		}

	}
}

void AReadyOrNotGameMode::ResetLevel()
{
	UE_LOG(LogGameMode, Verbose, TEXT("Reset %s"), *GetName());

	// Reset ALL controllers first
	for (FConstControllerIterator Iterator = GetWorld()->GetControllerIterator(); Iterator; ++Iterator)
	{
		AController* Controller = Iterator->Get();
		APlayerController* PlayerController = Cast<APlayerController>(Controller);
		if (PlayerController && PlayerController->GetPlayerState<AReadyOrNotPlayerState>())
		{
			PlayerController->ClientReset();
			Controller->Reset();
		}
	}

	TArray<AActor*> PreMissionPlanningActors;
	for (ULevelStreaming* Level : GetWorld()->GetStreamingLevels())
	{
		if (Level->GetWorldAssetPackageName().Contains("SubPreMissionPlanning"))
		{
			if (Level->GetLoadedLevel())
			{
				PreMissionPlanningActors = Level->GetLoadedLevel()->Actors;
			}
		}
	}
	
	// Reset all actors (except controllers, the GameMode, and any other actors specified by ShouldReset())
	for (FActorIterator It(GetWorld()); It; ++It)
	{
		AActor* A = *It;
		if (PreMissionPlanningActors.Contains(A))
			continue;
		
		// ##UE5UPGRADE##
		if (A && IsValid(A) && A != this && !A->IsA<AController>() && ShouldReset(A))
		{
			A->Reset();
		}
	}

	// Reset the GameMode
	Reset();

	// Notify the level script that the level has been reset
	ALevelScriptActor* LevelScript = GetWorld()->GetLevelScriptActor();
	if (LevelScript)
	{
		LevelScript->LevelReset();
	}
}

bool AReadyOrNotGameMode::CanTakeDamage(AController* EventInstigator, AController* DamageReceiver) const
{
	// If we dont have anyone instigating the damage it might be 'world' damage
	if (!EventInstigator)
		return true;

	// if we don't have a receiver controller then it must be a dummy pawn or something, let us try  damage it through the normal means
	if (!DamageReceiver)
		return true;

	AReadyOrNotCharacter* InstigatorCharacter = Cast<AReadyOrNotCharacter>(EventInstigator->GetPawn());
	AReadyOrNotCharacter* DamageReceiverCharacter = Cast<AReadyOrNotCharacter>(DamageReceiver->GetPawn());
	if (InstigatorCharacter	&& DamageReceiverCharacter)
	{
		if (DamageReceiverCharacter->bNoTeamDamage &&
			AReadyOrNotCharacter::IsOnSameTeam(InstigatorCharacter, DamageReceiverCharacter))
		{
			return false;
		}
	}
	
	return true;
}

void AReadyOrNotGameMode::AutoAssignTeam(AController* Player)
{
	AReadyOrNotGameState* gs = GetGameState<AReadyOrNotGameState>();
	AReadyOrNotPlayerState* ps = Cast<AReadyOrNotPlayerState>(Player->PlayerState);
	if (gs && ps)
	{
		if (gs->bPvPMode)
		{
			if (gs->GetPlayerStatesOfTeam(ETeamType::TT_SERT_BLUE).Num() <= gs->GetPlayerStatesOfTeam(ETeamType::TT_SERT_RED).Num())
			{
				//GEngine->AddOnScreenDebugMessage(KEY_NONE, 1.0f, FColor::Blue, ps->GetPlayerName() + " is joining the blue team!");
				ps->Server_SetTeam(ETeamType::TT_SERT_BLUE);
			}
			else
			{
				//GEngine->AddOnScreenDebugMessage(KEY_NONE, 1.0f, FColor::Red, ps->GetPlayerName() + " is joining the red team!");
				ps->Server_SetTeam(ETeamType::TT_SERT_RED);
			}
			V_LOGM(LogReadyOrNot, "Auto assigning %s to team %s", *ps->GetPlayerName(), *FString(RON_ENUM_TO_STRING(ETeamType, ps->GetTeam())));
		}
		else
		{
			if (gs->GetPlayerStatesOfTeam(ETeamType::TT_SERT_BLUE).Num() < 4)
			{
				//GEngine->AddOnScreenDebugMessage(KEY_NONE, 1.0f, FColor::Blue, ps->GetPlayerName() + " is joining the blue team!");
				ps->Server_SetTeam(ETeamType::TT_SERT_BLUE);
			}
			else
			{
				//GEngine->AddOnScreenDebugMessage(KEY_NONE, 1.0f, FColor::Red, ps->GetPlayerName() + " is joining the red team!");
				ps->Server_SetTeam(ETeamType::TT_SERT_RED);
			}
		}
	}
}

AActor* AReadyOrNotGameMode::FindPlayerStart_Implementation(AController* Player, const FString& IncomingName /*= TEXT("")*/)
{
	if (!Player)
		return nullptr;
	
	UWorld* World = GetWorld();

	// If incoming start is specified, then just use it
	if (!IncomingName.IsEmpty())
	{
		const FName IncomingPlayerStartTag = FName(*IncomingName);
		for (TActorIterator<APlayerStart> It(World); It; ++It)
		{
			APlayerStart* Start = *It;
			if (Start->PlayerStartTag == IncomingPlayerStartTag)
			{
				if (Player->PlayerState)
				{
					V_LOGM(LogReadyOrNot, "Found player start %s with tag %s for %s", *Start->GetName(), *Start->PlayerStartTag.ToString(), *Player->PlayerState->GetPlayerName());
				}
				
				return Start;
			}
		}
	}

	if (bIsCampaignTransitioning)
	{
		for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
		{
			APlayerStart* PlayerStart = *It;
			if (PlayerStart && PlayerStart->PlayerStartTag == "CampaignTransition")
			{
				return PlayerStart;
			}
		}
	}

	// Always pick StartSpot at start of match
	if (ShouldSpawnAtStartSpot(Player))
	{
		if (AActor* PlayerStartSpot = Player->StartSpot.Get())
		{
			if (Player->PlayerState)
			{
				V_LOGM(LogReadyOrNot, "Found player start actor %s for %s", *PlayerStartSpot->GetName(), *Player->PlayerState->GetPlayerName());
			}
			return PlayerStartSpot;
		}
		else
		{
			UE_LOG(LogGameMode, Error, TEXT("FindPlayerStart: ShouldSpawnAtStartSpot returned true but the Player StartSpot was null."));
		}
	}

	AActor* BestStart = ChoosePlayerStart(Player);
	if (BestStart == nullptr)
	{
		// No player start found
		UE_LOG(LogGameMode, Log, TEXT("FindPlayerStart: PATHS NOT DEFINED or NO PLAYERSTART with positive rating"));

		// This is a bit odd, but there was a complex chunk of code that in the end always resulted in this, so we may as well just 
		// short cut it down to this.  Basically we are saying spawn at 0,0,0 if we didn't find a proper player start
		BestStart = World->GetWorldSettings();
	}

	if (Player->PlayerState)
	{
		V_LOGM(LogReadyOrNot, "Found player start actor %s for %s", *BestStart->GetName(), *Player->PlayerState->GetPlayerName());
	}
	return BestStart;
}

void AReadyOrNotGameMode::RestartGame()
{
	if (GameSession->CanRestartGame())
	{
		UGameplayStatics::SetGamePaused(this, false);
		UReadyOrNotFunctionLibrary::PauseFMOD(false);

		if (GetNumPlayers() > 1)
		{
			FString Message = "Restarting Multiplayer Mission: " + GetWorld()->GetMapName() + " Players: " + FString::FromInt(GetNumPlayers());
			for (TActorIterator<APlayerState>It(GetWorld()); It; ++It)
			{
				if (*It == UReadyOrNotStatics::GetReadyOrNotPlayerController()->GetPlayerState<APlayerState>())
					continue;
				
				Message += "\n" + It->GetPlayerName() + " (" + It->GetUniqueId().ToString() + ")";
			}
			UReadyOrNotBackend::LogMessage(Message);
		}
		
		V_LOGM(LogReadyOrNot, "Restarting Game!");
		SetMatchState(EMatchState::MS_GoingToNextLevel);
		for (TActorIterator<AReadyOrNotPlayerController> It(GetWorld()); It; ++It)
		{
			AReadyOrNotPlayerController* pc = *It;
			pc->Client_RemoveWidget(UBpGameplayHelperLib::GetWidgetDataFromLookupData("Escape").WidgetClass);
		}
		for (TActorIterator<AReadyOrNotPlayerState>It(GetWorld()); It; ++It)
		{
			AReadyOrNotPlayerState* ps = *It;
			ps->bIsInGame = false;
			ps->bReady = false; 
		}

		GetWorld()->ServerTravel("?restart", false);
	}
}

// NextGame = all of the rounds have been exhausted, we are traveling to the next game, and optionally changing the map along the way
// this is always accompanied by a loadscreen, even if we aren't changing maps
void AReadyOrNotGameMode::NextGame()
{
	
	UReadyOrNotGameInstance* gi = Cast<UReadyOrNotGameInstance>(GetWorld()->GetGameInstance());
	if (gi)
	{
		
		if (UKismetSystemLibrary::IsDedicatedServer(GetWorld()))
		{
			int GamesPlayed = ++UReadyOrNotStatics::GetReadyOrNotGameInstance()->DedicatedServerGamesPlayedWithoutReturningToLobby;
			UReadyOrNotStatics::GetReadyOrNotGameInstance()->DedicatedServerMapIdx++;
			AReadyOrNotGameSession* session = Cast<AReadyOrNotGameSession>(UReadyOrNotStatics::GetReadyOrNotGameMode()->GameSession);
			if (session)
			{
				if (GamesPlayed >= session->ReturnToLobbyAfterXMissions)
				{
					FString LobbyLevel = GetGameInstance<UReadyOrNotGameInstance>()->LobbyLevel;
					ProcessServerTravel(LobbyLevel + "?game=lobby");
					return;
				}
			}
		}
		
		FString NextMapURL = GetNextURL();
		FString map;
		FString mode;

		AReadyOrNotGameSession* gs = Cast<AReadyOrNotGameSession>(GameSession);
		if (gs)
		{
			TArray<FString> LoadedMapList;
			int32 Length;
			V_LOGM(LogReadyOrNot, "Looking for Zeuz MapList at %s../zeuzmaplist.ini", *FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()));
			UReadyOrNotFunctionLibrary::LoadStringArrayFromFile(LoadedMapList, Length, FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()) + "../zeuzmaplist.ini", true);
			if (LoadedMapList.Num() > 0)
			{
				V_LOGM(LogReadyOrNot, "Using Zeuz MapList");

				for (int32 i = 0; i > LoadedMapList.Num(); i++)
				{
					V_LOGM(LogReadyOrNot, "Loaded Map: %s", *LoadedMapList[i]);
				}
				
				gs->MapList = LoadedMapList;

				// Shuffle gamemode types
				int32 LastIndex = gs->MapList.Num() - 1;
				for (int32 i = 0; i <= LastIndex; ++i)
				{
					int32 Index = FMath::RandRange(i, LastIndex);
					if (i != Index)
					{
						FString tmp = gs->MapList[i];
						gs->MapList[i] = gs->MapList[Index];
						gs->MapList[Index] = tmp;
					}
				}
				NextMapURL = LoadedMapList[FMath::RandRange(0, LoadedMapList.Num() - 1)];
				V_LOGM(LogReadyOrNot, "Selected Zeuz MapList %s from loaded maplist", *NextMapURL);
			}
		}

		NextMapURL.Split("?game=", &map, &mode, ESearchCase::IgnoreCase);
		SetMatchState(EMatchState::MS_GoingToNextLevel);
		if (gs)
		{
			gs->UpdateServerDetails(map, mode);
		}
		// GET THE FULL PATH FROM THE RELEATIVE PATH
		FURL DefaultURL;
		FURL URL(&DefaultURL, *NextMapURL, TRAVEL_Partial);
		V_LOGM(LogReadyOrNot, "Next Map URL Found, traveling to %s", *(URL.Map + "?game=" + mode));
		GetWorld()->ServerTravel(NextMapURL.IsEmpty() ? "?Restart?" : (URL.Map + "?game=" + mode), true);
		// tell the clients to go to a loading screen
		for (TActorIterator<AReadyOrNotPlayerController> It(GetWorld()); It; ++It)
		{
			AReadyOrNotPlayerController* pc = *It;
			pc->Client_RemoveWidget(UBpGameplayHelperLib::GetWidgetDataFromLookupData("Escape").WidgetClass);
			pc->Client_ClearHUDWidgets();
			IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
			if (OnlineSub)
			{
				IOnlineSessionPtr SessionInt = Online::GetSessionInterface();
				if (SessionInt)
				{
					FNamedOnlineSession* Session = SessionInt->GetNamedSession(NAME_GameSession);
					if (Session != nullptr)
					{
						FString Map, GameMode, SessionName;
						Session->SessionSettings.Get(SETTING_MAPNAME, GameMode);
						Session->SessionSettings.Get(SETTING_GAMEMODE, Map);
						Session->SessionSettings.Get("GAMENAME", SessionName);
						pc->Client_CreateLoadingScreen(Map, GameMode, SessionName);
					}
				}
			}
		}
		SetMatchState(EMatchState::MS_GoingToNextLevel);
		ResetClientScores(false);

	}
}

void AReadyOrNotGameMode::OnOutOfBoundsTimeLimitEnded_Implementation()
{
	APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (PlayerCharacter)
	{
		PlayerCharacter->LockAllActions();
		PlayerCharacter->Kill();
	}
}

void AReadyOrNotGameMode::AddAbuse(AReadyOrNotCharacter* Abuser, ACyberneticCharacter* Abused)
{
	if (!Abuser || !Abused)
		return;
	
	UWorld* WorldContext = Abuser->GetWorld();
	/*if (!WorldContext)
	{
		if (!GEngine)
			return;

		if (GEngine->GameViewport)
		{
			WorldContext = GEngine->GameViewport->GetWorld();
		}
	}*/

	if (!WorldContext)
	{
		return;
	}

	if (Abused)
	{
		Abused->AbuseCount++;
	}

	AReadyOrNotGameMode* gm = Cast<AReadyOrNotGameMode>(WorldContext->GetAuthGameMode());
	if (gm)
	{
		gm->AbuseCountMap.FindOrAdd(Abuser) += 1;

		AReadyOrNotGameState* gs = gm->GetGameState<AReadyOrNotGameState>();
		if (gs)
		{
			gs->TotalMissionAbuseCount++;
		}
	}
	
}

FString AReadyOrNotGameMode::GetNextURL()
{
	FString NextMapURL = "";
	UReadyOrNotGameInstance* gi = Cast<UReadyOrNotGameInstance>(GetWorld()->GetGameInstance());
	if (gi)
	{

		
		AReadyOrNotGameSession* gs = Cast<AReadyOrNotGameSession>(GameSession);
		if (gs)
		{

			//V_LOGM(LogReadyOrNot, "Compatible Server URLs are below");
			//V_LOGM(LogReadyOrNot, "-------------");
			//for (int32 y = 0; y < gi->GetBuiltMapList().Num(); y++)
			//{
				//V_LOGM(LogReadyOrNot, "%s", *gi->GetBuiltMapList()[y]);
			//}
			//V_LOGM(LogReadyOrNot, "-------------");

			for (int32 i = 0; i < gs->MapList.Num(); i++)
			{
				if (gs->MapList[i].Contains(GetWorld()->GetMapName(), ESearchCase::IgnoreCase) && gs->MapList[i].Contains(urlShortName, ESearchCase::IgnoreCase))
				{
					if (gs->MapList.IsValidIndex(i + 1))
					{
						NextMapURL = gs->MapList[i + 1];
					}
					else
					{
						NextMapURL = gs->MapList[0];
					}
				}
			}


			if (NextMapURL.IsEmpty() && gs->MapList.Num() > 0)
			{
				NextMapURL = gs->MapList[0];
			}


		}
	}
	return NextMapURL;

}

void AReadyOrNotGameMode::SetPassword(FString newPassword)
{
	Password = newPassword;
}

bool AReadyOrNotGameMode::KickPlayer(APlayerController* KickedPlayer, const FText& KickReason)
{
	// Do not kick logged admins / hosting player
	if (KickedPlayer != nullptr && KickedPlayer->GetNetMode() != NM_ListenServer)
	{
		V_LOGM(LogReadyOrNot, "Kicking %s for reason: %s", *KickedPlayer->GetName(), *KickReason.ToString());
		if (KickedPlayer->GetPawn() != nullptr)
		{
			KickedPlayer->GetPawn()->Destroy();
		}

		KickedPlayer->ClientWasKicked(KickReason);

		if (KickedPlayer != nullptr && KickedPlayer->GetLocalRole() < ROLE_Authority)
		{
			KickedPlayer->Destroy();
		}

		return true;
	}
	return false;
}

AActor* AReadyOrNotGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	// Choose a player start
	APlayerStart* FoundPlayerStart = nullptr;
	UClass* PawnClass = GetDefaultPawnClassForController(Player);
	APawn* PawnToFit = PawnClass ? PawnClass->GetDefaultObject<APawn>() : nullptr;
	TArray<APlayerStart*> UnOccupiedStartPoints;
	TArray<APlayerStart*> OccupiedStartPoints;
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		APlayerStart* PlayerStart = *It;

		if (PlayerStart->IsA<APlayerStartPIE>())
		{
			// Always prefer the first "Play from Here" PlayerStart, if we find one while in PIE mode
			FoundPlayerStart = PlayerStart;
			break;
		}
		else
		{
			FVector ActorLocation = PlayerStart->GetActorLocation();
			const FRotator ActorRotation = PlayerStart->GetActorRotation();
			if (!GetWorld()->EncroachingBlockingGeometry(PawnToFit, ActorLocation, ActorRotation))
			{
				UnOccupiedStartPoints.Add(PlayerStart);
			}
			else if (GetWorld()->FindTeleportSpot(PawnToFit, ActorLocation, ActorRotation))
			{
				OccupiedStartPoints.Add(PlayerStart);
			}
		}
	}

	if (FoundPlayerStart == nullptr)
	{
		if (UnOccupiedStartPoints.Num() > 0)
		{
			FoundPlayerStart = UnOccupiedStartPoints[FMath::RandRange(0, UnOccupiedStartPoints.Num() - 1)];
		}
		else if (OccupiedStartPoints.Num() > 0)
		{
			FoundPlayerStart = OccupiedStartPoints[FMath::RandRange(0, OccupiedStartPoints.Num() - 1)];
		}
	}
	return FoundPlayerStart;
}

void AReadyOrNotGameMode::ResetClientScores(bool bBetweenRounds)
{
	AReadyOrNotGameState* gs = GetGameState<AReadyOrNotGameState>();
	if (gs)
	{
		for (int32 i = 0; i < gs->PlayerArray.Num(); i++)
		{
			AReadyOrNotPlayerState* ps = Cast<AReadyOrNotPlayerState>(gs->PlayerArray[i]);
			if (ps)
			{
				ps->Kills = 0;
				ps->TeamKills = 0;
				ps->Arrests = 0;
				ps->Objectives = 0;
				ps->Deaths = 0;
				ps->DamageDealt = 0;
				ps->DamageReceived = 0;
				ps->PointsFromKills = 0;
				ps->PointsFromDamage = 0;
				ps->PointsFromArrests = 0;
				ps->PointsFromObjective = 0;
				ps->SetScore(0);
				ps->GunGameIdx = 0;
				ps->KillsSinceUpgrade = 0;
			}
		}
	}
}

void AReadyOrNotGameMode::StartMatch()
{
	V_LOGM(LogReadyOrNot, "Starting Match!");
	
	ResetLevel();
	
	AReadyOrNotGameState* gs = GetGameState<AReadyOrNotGameState>();
	if (gs)
	{
		gs->TimeTillGameStartCountdown = 0.0f;
		gs->BlueTeamPlayers.Empty();
		gs->RedTeamPlayers.Empty();
	}

	SetMatchState(EMatchState::MS_Playing);

	// Try to spawn all dynamic actors.
	for (TActorIterator<ADynamicWorldActor> It(GetWorld()); It; ++It)
	{
		ADynamicWorldActor* DWA = *It;
		V_LOGM(LogReadyOrNot, "Spawning Dynamic World Actor %s", *DWA->GetName());
		// We add a delay here in case the stuff required  for the dynamci spawn is not yet valid
		FTimerHandle CheckDynamicSpawn_Handle;
		GetWorld()->GetTimerManager().SetTimer(CheckDynamicSpawn_Handle, DWA, &ADynamicWorldActor::CheckDynamicSpawn, 1.0f, false);
	}


	if (bInitialPlayerRespawn)
	{
		RespawnAllPlayers();
	}

	if (AReadyOrNotGameState* GS = GetGameState<AReadyOrNotGameState>())
	{
		GS->Multicast_OnGameStarted();
	}

	OnMatchStarted.Broadcast();

	
	if (!GetWorld()->GetMapName().Contains("MainMenu") && !GetWorld()->GetMapName().Contains("Lobby"))
	{
		FString SteamId, SteamName; 

		APlayerController* pc = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld());
		if (pc)
		{
			if (pc->PlayerState)
			{
				if (pc->PlayerState->GetUniqueId().IsValid())
				{
					SteamId = pc->PlayerState->GetUniqueId().ToString();
					SteamName = pc->PlayerState->GetPlayerName();
				}

			}
		}

		if (SteamName.IsEmpty())
		{
			SteamName = "Unknown";
		}

#ifdef HOST_MIGRATION_ENABLED
		UReadyOrNotFunctionLibrary::StartTimerForCallback(this, &AReadyOrNotGameMode::LoadHostMigrationData, 1.0f);
#endif

		
		if(AMapStatisticsSystem* Statistics = UBpGameplayHelperLib::GetMapStatistics())
		{
			FString InternalMap, TranslatedMap, Mode;
			UBpGameplayHelperLib::GetFriendlyMapAndModeFromName(GetWorld()->GetLocalURL(), InternalMap, TranslatedMap, Mode);
			Statistics->StartRecording(InternalMap, Mode);
		}
	}
}

void AReadyOrNotGameMode::LoadHostMigrationData()
{
#ifndef HOST_MIGRATION_ENABLED
	return;
#endif
	UHostMigrationManager* HostMigrationManager = UReadyOrNotStatics::GetReadyOrNotGameInstance()->HostMigrationManager;
	if (HostMigrationManager && HostMigrationManager->IsMigratingHost())
	{
		APlayerCharacter* LocalPlayer = Cast<APlayerCharacter>(UReadyOrNotStatics::GetReadyOrNotPlayerController()->GetPawn());
		if (!LocalPlayer)
			return;

		// we have finished the migration.. make the lobby not joinable again
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
		
		HostMigrationManager->SetHostMigrationInProgress(false);
		// no swat allowed when migrating
		for (TActorIterator<ASWATCharacter>It(GetWorld()); It; ++It)
		{
			It->Destroy();
		}
		TArray<FHm_PlayerInformation> PlayerInformations;
		TArray<FHm_CyberneticsInformation> CyberneticsInformations;
		TArray<FHm_DoorInformation> DoorInformations;
		TArray<FHm_BombInformation> BombInformations;
		TArray<FHm_BaggedEvidence> BaggedEvidences;
		TArray<FHm_DroppedEvidence> DroppedEvidences;
		TArray<FHm_Objectives> Objectives;
		TArray<FString> ActiveEvidence;
		HostMigrationManager->LoadState(PlayerInformations, CyberneticsInformations, DoorInformations, BombInformations, ActiveEvidence, BaggedEvidences, DroppedEvidences, Objectives);

		APlayerCharacter* FirstPlayerCharacter = nullptr;
		for (TActorIterator<APlayerCharacter>It(GetWorld()); It; ++It)
		{
			APlayerCharacter* Pc = *It;
			if (!Pc->GetPlayerState())
				continue;

			if (!Pc->GetPlayerState()->GetUniqueId().IsValid())
				continue;
			
			FUniqueNetIdPtr UserID = Pc->GetPlayerState()->GetUniqueId().GetUniqueNetId();
			if (!UserID.IsValid())
				continue;

			if (!FirstPlayerCharacter)
			{
				FirstPlayerCharacter = Pc;
			}
			
			for (FHm_PlayerInformation Pi : PlayerInformations)
			{
				
				if (UserID->IsValid() && Pi.UniqueId == UserID->ToString())
				{
					V_LOGM(LogReadyOrNot, "Loading Migration Data onto %s", *Pc->GetName());
					Pc->Client_SetControlRotation(Pi.ControlRotation);
					Pc->SetActorTransform(Pi.CharacterTransform);
					Pc->GetHealthComponent()->Server_SetResource(Pi.Health);
					if (Pi.Health <= 0.0f)
					{
						UGameplayStatics::ApplyDamage(Pc, 100000.0f, Pc->GetController(), Pc, UDamageType::StaticClass());
					}
					if (Pi.bHasBeenReported)
					{
						Pc->Server_ReportToTOC(Pc);
						Pc->Server_ReportTarget(Pc);
					}
				
					Pc->GetInventoryComponent()->EquipItemOfClass(Pi.EquippedItemClass);
					int32 CountedGrenades = 0;
					int32 CountedTacticalDevices = 0;
					for (int32 i = 0; i < Pc->GetInventoryComponent()->GetInventoryItems().Num(); i++)
					{
						ABaseItem* Item = Pc->GetInventoryComponent()->GetInventoryItems()[i];
						if (!Item)
							continue;
						
						for (FHm_InventoryInformation SavedInventory : Pi.Inventory)
						{
							if (SavedInventory.Class == Item->GetClass())
							{
								ABaseMagazineWeapon* Bmw = Cast<ABaseMagazineWeapon>(Item);
								if (Bmw)
								{
									Bmw->Magazines = SavedInventory.Magazines;
									Bmw->MagIndex = SavedInventory.MagIndex;
								}
							}
						}

						if (Item->ContainsItemCategory(EItemCategory::IC_Grenade))
						{
							CountedGrenades++;
							if (CountedGrenades > Pi.TotalGrenades)
							{
								Pc->GetInventoryComponent()->DestroyInventoryItem(Item);
								
								if (i > 0)
									i--;
							}
						}
						if (Item->ContainsItemCategory(EItemCategory::IC_TacticalDevice))
						{
							CountedTacticalDevices++;
							if (CountedTacticalDevices > Pi.TotalDevices)
							{
								Pc->GetInventoryComponent()->DestroyInventoryItem(Item);
								if (i > 0)
									i--;
							}
						}
					}
				}
			}
		}

		int32 Idx = 0;
		for (TActorIterator<ACyberneticCharacter>It(GetWorld()); It; ++It)
		{
			ACyberneticCharacter* Cc = *It;
			if (CyberneticsInformations.IsValidIndex(Idx))	
			{
				V_LOGM(LogReadyOrNot, "Loading Migration Data onto %s", *Cc->GetName());
				FHm_CyberneticsInformation Ci = CyberneticsInformations[Idx];
				Cc->SetActorTransform(Ci.CharacterTransform);
				Cc->Tags = Ci.Tags;
				Cc->SetDefaultTeam(Ci.TeamType);
				Cc->CharacterMeshData = Ci.CharacterMeshData;
				Cc->OnRep_CharacterMeshData();
				Cc->GetHealthComponent()->SetResource(Ci.Health);
				Cc->GetInventoryComponent()->DestroyAllEquippedItems();
				if (Ci.bIsArrested)
				{
					Cc->ArrestComplete(LocalPlayer, (AZipcuffs*)LocalPlayer->GetInventoryComponent()->GetInventoryItemOfClass(AZipcuffs::StaticClass()));
					Cc->StopTPAnimMontage();
				} else if (Ci.bIsSurrendered)
				{
					Cc->Surrender();
					Cc->StopTPAnimMontage();
				}
				if (Ci.bHasBeenReported)
				{
					Cc->bHasBeenReported = true;
					IScoringInterface::Execute_GetScoringComponent(Cc)->GiveAllScores();
				}
				if (Ci.EquippedItemClass)
				{
					ABaseMagazineWeapon* Weapon = GetWorld()->SpawnActor<ABaseMagazineWeapon>(Ci.EquippedItemClass);
					if (Weapon)
					{
						
						Cc->GetInventoryComponent()->AddInventoryItem(Weapon);
						Cc->GetInventoryComponent()->PutItemInHands(Weapon);
					}
				}
				
				if (Ci.Health <= 0.0f)
				{
					UGameplayStatics::ApplyDamage(Cc, 100000.0f, Cc->GetController(), Cc, UDamageType::StaticClass());
				}

			}
			Idx++;
		}

		for (TActorIterator<ADoor>It(GetWorld()); It; ++It)
		{
			ADoor* Door = *It;
			for (FHm_DoorInformation DoorInfo : DoorInformations)
			{
				if (Door->GetName() == DoorInfo.Name)
				{
					V_LOGM(LogReadyOrNot, "Loading Migration Data onto %s", *Door->GetName());
					Door->OpenDoor_SpecificAngle(nullptr, DoorInfo.OpenCloseAmount, true, false);
					if (DoorInfo.bIsSimulatingPhysics)
					{
						Door->GetDoorMesh()->SetSimulatePhysics(true);
						Door->GetDoorMesh()->SetWorldTransform(DoorInfo.DoorMeshTransform);
					}
					if (DoorInfo.bIsBroken)
					{
						Door->BreakDoor(true);
					}
					for (int32 i = 0; i < 9; i++)
					{
						if (Door->GetChunkComponents().IsValidIndex(i))
						{
							if (DoorInfo.DoorChunkInformations[i].bIsSimulating)
							{
								Door->GetChunkComponents()[i]->OnChunkDestroyed();
								Door->GetChunkComponents()[i]->SetWorldTransform(DoorInfo.DoorChunkInformations[i].Transform);
								Door->BreakDoorHandles();
							}
						}
					}
					// Will be NAME_None for no trap
					if (Door->GetAttachedTrap())
					{
						Door->GetAttachedTrap()->Destroy();
					}
					Door->SetTypeOfTrapRowName(DoorInfo.TrapName);
					Door->SetupTrap();
				}
			}
		}

		for (TActorIterator<ABombActor>It(GetWorld()); It; ++It)
		{
			for (FHm_BombInformation Bomb : BombInformations)
			{
				if (Bomb.BombName == It->GetName())
				{
					It->TimeUntilExplodes = Bomb.TimeRemaining;
					It->BombState = Bomb.BombState;
				}
			}
		}

		for (TActorIterator<AEvidenceActor>It(GetWorld()); It; ++It)
		{
			// its been collected
			if (!ActiveEvidence.Contains(It->GetName()))
			{
				if (FirstPlayerCharacter)
				{
					FirstPlayerCharacter->PickupEvidence(*It);
				}
			}
		}

		for (TActorIterator<ABaseItem>It(GetWorld()); It; ++It)
		{
			ABaseItem* Item = *It;
			if (Item->IsEvidence() || Item->GetItemMesh()->IsSimulatingPhysics())
			{
				if (FirstPlayerCharacter)
				{
					FirstPlayerCharacter->PickupEvidence(*It);
				}
			}
		}

		for (FHm_BaggedEvidence Bag : BaggedEvidences)
		{
			LocalPlayer->SpawnEvidenceCollectionBag(Bag.Transform);
		}
		
		for (FHm_DroppedEvidence DroppedItem : DroppedEvidences)
		{
			ABaseItem* Item = GetWorld()->SpawnActor<ABaseItem>(DroppedItem.Class);
			if (Item)
			{
				Item->SetActorTransform(DroppedItem.Transform);
				Item->MarkAsEvidence(true);
			}
		}

		TArray<AActor*> OutObjectives;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AObjective::StaticClass(), OutObjectives);
		for (int32 i = 0; i < Objectives.Num(); i++)
		{
			if (OutObjectives.IsValidIndex(i))
			{
				AObjective* Obj = Cast<AObjective>(OutObjectives[i]);
				if (Obj)
				{
					Obj->SetObjectiveStatus(Objectives[i].ObjectiveStatus);
				}
			}
		}
	}
}

int32 AReadyOrNotGameMode::GetNumberOfArrestedPlayersOnTeam(ETeamType Team)
{
	TArray<AActor*> Actors;

	int32 NumberArrestedPlayers = 0;

	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerCharacter::StaticClass(), Actors);
	for (int32 i = 0; i < Actors.Num(); i++)
	{
		APlayerCharacter* pc = Cast<APlayerCharacter>(Actors[i]);
		if (pc && pc->GetTeam() == Team && pc->IsArrested())
		{
			NumberArrestedPlayers++;
		}
	}

	return NumberArrestedPlayers;
}

int32 AReadyOrNotGameMode::GetNumberOfFreePlayersOnTeam(ETeamType Team)
{
	int32 Count = 0;
	for (TActorIterator<APlayerCharacter>It(GetWorld()); It; ++It)
	{
		APlayerCharacter* pc = *It;
		if (pc->GetTeam() == Team && !pc->IsDeadOrUnconscious() && !pc->IsArrested() && pc->GetController())
		{
			Count++;
		}
	}
	return Count;
}

bool AReadyOrNotGameMode::AreAllPlayersOnTeamArrested(const ETeamType Team)
{
	TArray<AActor*> Actors;

	bool bAllPlayersArrested = true;
	
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerCharacter::StaticClass(), Actors);
	for (int32 i = 0; i < Actors.Num(); i++)
	{
		if (APlayerCharacter* pc = Cast<APlayerCharacter>(Actors[i]))
		{
			if (pc && pc->LastPlayerState && pc->LastPlayerState->GetTeam() == Team && !pc->IsArrested())
			{
				bAllPlayersArrested = false;
				break;
			}
		}
	}

	return bAllPlayersArrested;
}

bool AReadyOrNotGameMode::AreAllPlayersOnTeamDead(const ETeamType Team)
{
	TArray<AActor*> Actors;

	bool bAllPlayersDead = true;

	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerCharacter::StaticClass(), Actors);
	for (int32 i = 0; i < Actors.Num(); i++)
	{
		APlayerCharacter* pc = Cast<APlayerCharacter>(Actors[i]);
		if (pc && pc->GetTeam() == Team && !pc->IsDeadOrUnconscious())
		{
			bAllPlayersDead = false;
			break;
		}
	}

	return bAllPlayersDead;
}

bool AReadyOrNotGameMode::AreAllPlayersOnTeamDowned(const ETeamType Team)
{
	TArray<AActor*> Actors;

	bool bAllPlayersDowned = true;

	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerCharacter::StaticClass(), Actors);
	for (int32 i = 0; i < Actors.Num(); i++)
	{
		APlayerCharacter* pc = Cast<APlayerCharacter>(Actors[i]);
		if (pc && pc->GetTeam() == Team && !pc->IsDowned())
		{
			bAllPlayersDowned = false;
			break;
		}
	}

	return bAllPlayersDowned;
}

bool AReadyOrNotGameMode::AreAllPlayersOnTeamDowned(APlayerCharacter* PlayerCharacter)
{
	TArray<AActor*> Actors;

	bool bAllPlayersDowned = true;

	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerCharacter::StaticClass(), Actors);
	for (int32 i = 0; i < Actors.Num(); i++)
	{
		APlayerCharacter* pc = Cast<APlayerCharacter>(Actors[i]);
		if (pc && pc != PlayerCharacter && pc->GetTeam() == PlayerCharacter->GetTeam() && !pc->IsDowned())
		{
			bAllPlayersDowned = false;
			break;
		}
	}

	return bAllPlayersDowned;
}

bool AReadyOrNotGameMode::AreAllPlayersOnTeamDead(APlayerCharacter* PlayerCharacter)
{
	TArray<AActor*> Actors;

	bool bAllPlayersDead = true;

	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerCharacter::StaticClass(), Actors);
	for (int32 i = 0; i < Actors.Num(); i++)
	{
		APlayerCharacter* pc = Cast<APlayerCharacter>(Actors[i]);
		if (pc && pc != PlayerCharacter && pc->GetTeam() == PlayerCharacter->GetTeam() && !pc->IsDeadOrUnconscious())
		{
			bAllPlayersDead = false;
			break;
		}
	}

	return bAllPlayersDead;
}

FString AReadyOrNotGameMode::InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal /* = TEXT("") */)
{
	AReadyOrNotPlayerState* ps = Cast<AReadyOrNotPlayerState>(NewPlayerController->PlayerState);
	if (ps)
	{
		
		AutoAssignTeam(NewPlayerController);
		if (GetMatchState() == EMatchState::MS_Playing)
		{
			ps->bJoinInProgress = true;
		}
	}
	return Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);
}

void AReadyOrNotGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);

	FString pw = UGameplayStatics::ParseOption(Options, TEXT("password"));
	if (pw != Password && !Password.IsEmpty())
		ErrorMessage = "Invalid Password!";

	int32 PlayersInGame = 0;
	for (TActorIterator<APlayerController> It(GetWorld()); It; ++It)
	{
		PlayersInGame++;	
	}

	UReadyOrNotStatics::GetReadyOrNotGameInstance()->ReadyOrNotBackend->CheckIsBanned(UniqueId.ToString());

	int32 MaxPlayers = 5;
#if defined(TARGET_CONSOLE)
	MaxPlayers = UConsoleMultiplayerStatics::GetMaxPlayers(GetWorld());
#endif
	
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

	if (PlayersInGame + 1 > MaxPlayers)
	{
		ErrorMessage  = "Server is full!";
	}		

	NameOptionsMap.Add(UGameplayStatics::ParseOption(Options, "name"), Options);
}

void AReadyOrNotGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	
	if (GetReadyOrNotGameState())
	{
		GetReadyOrNotGameState()->Rep_GameModeSettings = GameModeSettings;
	}
	
	if (NewPlayer && NewPlayer->PlayerState)
	{
		FString SteamId = NewPlayer->PlayerState->GetUniqueId().ToString();
		V_LOGM(LogReadyOrNot, "Player Joined (%s) (%s)", *NewPlayer->PlayerState->GetPlayerName(), *SteamId);
		if (BannedSteamIds.Contains(SteamId))
		{
			KickPlayer(NewPlayer, FText::FromString(BannedSteamIds[SteamId]));
			return;
		}
	}
	
	AReadyOrNotGameSession* Session = Cast<AReadyOrNotGameSession>(GameSession);
	AReadyOrNotGameState* gs = Cast<AReadyOrNotGameState>(GetWorld()->GetGameState());
	if (NewPlayer && gs)
	{
		if (Session)
		{
			// ##UE5UPGRADE##
			Session->RegisterPlayer(NewPlayer, NewPlayer->PlayerState->GetUniqueId(), false);
		}
		
		if (GetMatchState() == EMatchState::MS_Warmup)
		{
			// Hide their TP and face meshes
			APlayerCharacter* pc = Cast<APlayerCharacter>(NewPlayer->GetPawn());
			if (pc)
			{
				pc->GetMesh()->SetVisibility(false, true);
				pc->Multicast_HideThirdPerson();
				pc->Multicast_HideThirdPerson_Implementation();
			}
		}

		gs->Multicast_BroadcastChatMessage(FRChatMessage("SYSTEM", NewPlayer->PlayerState->GetPlayerName() + " has joined the game!", FLinearColor::Green, nullptr));
	
		if (Session)
		{
			FJoinabilitySettings OutSettings;
			Session->GetSessionJoinability(NAME_GameSession, OutSettings);
			Session->UpdateSessionJoinability(NAME_GameSession, OutSettings.bPublicSearchable, OutSettings.bAllowInvites, OutSettings.bJoinViaPresence, OutSettings.bJoinViaPresenceFriendsOnly);
			Session->UpdateServerDetails("", "");
			if (NewPlayer &&
				NewPlayer->PlayerState
				&& NewPlayer->PlayerState->GetUniqueId().IsValid())
			{
				FString playerId = NewPlayer->PlayerState->GetUniqueId().ToString();
				if (Session->LoggedInAdmins.Contains(playerId))
				{
					gs->AdminPlayerControllers.AddUnique(NewPlayer);
				}
				if (Session->BanList.Contains(playerId))
				{
					bool bFoundSessionData = false;
					if (Session->SessionData)
					{
						if (Session->SessionData->BanReasonData.Find(playerId))
						{
							bFoundSessionData = true;
							Session->KickPlayer(NewPlayer, FText::FromString(Session->SessionData->BanReasonData[playerId]));
						}
					}
					if (!bFoundSessionData)
					{
						Session->KickPlayer(NewPlayer, NSLOCTEXT("ReadyOrNot", "ReadyOrNot", "You have been kicked by an admin."));
					}
					
				}
			}
		}
	}


	// Required so the dedicated server can update the player count
	// dedicated server can't authenticate players as it is not running steam
	AReadyOrNotPlayerController* pc = Cast<AReadyOrNotPlayerController>(NewPlayer);
	if (pc)
	{
		if (Session)
		{
			pc->ClientSetNetSpeed_Implementation(Session->ClientNetSpeed);
			pc->ClientSetNetSpeed(Session->ClientNetSpeed);
		}
		pc->Client_PostLogin();
		if (GetMatchState() == EMatchState::MS_Playing)
		{
			pc->ClientStartOnlineGame();
		}
		
		if (const IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get())
		{
			const IOnlineSessionPtr SessionSubsystem = OnlineSubsystem->GetSessionInterface();
			if (SessionSubsystem.Get())
			{
				if (const FNamedOnlineSession* NamedGameSession = SessionSubsystem->GetNamedSession(NAME_GameSession))
				{
					if (NamedGameSession->SessionInfo.Get())
					{
						const FString OnlineSessionId = NamedGameSession->SessionInfo->GetSessionId().ToString();
						pc->ClientJoinVoice(OnlineSessionId, 0);
					}
				}
			}
		}
	}

	RespawnPlayer(NewPlayer);

	if (NewPlayer->GetPawn())
	{
		AActor* start = FindPlayerStart(NewPlayer);
		if (start)
		{
			NewPlayer->GetPawn()->SetActorTransform(start->GetTransform());
		}
	}
}

void AReadyOrNotGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	APlayerController* PlayerController = Cast<APlayerController>(Exiting);
	if (PlayerController)
	{
		if (Exiting && Exiting->PlayerState && !Exiting->PlayerState->IsABot())
		{
			V_LOGM(LogReadyOrNot, "Player Left (%s)", *Exiting->PlayerState->GetPlayerName());

			if (!Exiting->PlayerState->GetPlayerName().IsEmpty())
			{
				//Informs the others about leaving from the game.
				AReadyOrNotGameState* gs = GetGameState<AReadyOrNotGameState>();
				if (gs)
				{
					FRChatMessage Message;
					Message.Color = FColor::Red;
					Message.Message = Exiting->PlayerState->GetPlayerName() + " left the game";
					gs->Multicast_BroadcastChatMessage(Message);
				}
			}
		}			
	}

	AReadyOrNotPlayerController* ReadyOrNotPlayerController = Cast<AReadyOrNotPlayerController>(Exiting);
	if (ReadyOrNotPlayerController)
	{
		ReadyOrNotPlayerController->bExiting = true;
	}
	
	AReadyOrNotGameSession* Session = Cast<AReadyOrNotGameSession>(GameSession);
	if (Session)
	{
		if (PlayerController && PlayerController->PlayerState)
		{
			Session->UnregisterPlayer(NAME_GameSession, PlayerController->PlayerState->GetUniqueId().GetUniqueNetId());
		}
		FJoinabilitySettings OutSettings;
		Session->GetSessionJoinability(NAME_GameSession, OutSettings);
		Session->UpdateSessionJoinability(NAME_GameSession, OutSettings.bPublicSearchable, OutSettings.bAllowInvites, OutSettings.bJoinViaPresence, OutSettings.bJoinViaPresenceFriendsOnly);
		Session->UpdateServerDetails("", "");
	}

	if (Exiting->PlayerState)
	{
		Exiting->PlayerState->Destroy();
	}	
	Exiting->Destroy();	
}

void AReadyOrNotGameMode::ProcessServerTravel(const FString& URL, bool bAbsolute /*= false*/)
{
	CurrentMatchState = EMatchState::MS_GoingToNextLevel;

	FString InternalMap, TranslatedMap, Mode;
	UBpGameplayHelperLib::GetFriendlyMapAndModeFromName(URL, InternalMap, TranslatedMap, Mode);
	
	AReadyOrNotGameState* gs = GetGameState<AReadyOrNotGameState>();
	if (gs)
	{
		TArray<APlayerState*> Players = gs->PlayerArray;
		for (int32 i = 0; i < Players.Num(); i++)
		{
			AReadyOrNotPlayerState* ps = Cast<AReadyOrNotPlayerState>(Players[i]);
			if (ps)
			{
				// TODO SP: Reset metrics for the player
				ps->bReady = false;
				ps->bHasFinishedLoading = false;
			}
		}
	}
	AReadyOrNotGameSession* gsesh = Cast<AReadyOrNotGameSession>(GameSession);
	if (gsesh)
	{
		if (gsesh)
		{
			FString map, mode;
			URL.Split("?game=", &map, &mode, ESearchCase::IgnoreCase);
			gsesh->UpdateServerDetails(map, mode);
			
		}
	}
	
	if(gs && gsesh)
	{
		UReadyOrNotStatics::GetReadyOrNotGameInstance()->ReadyOrNotBackend->OnGameStartedMetric(InternalMap, Mode, gs->PlayerArray.Num());
	}
	
	Super::ProcessServerTravel(URL, bAbsolute);
}

void AReadyOrNotGameMode::PostSeamlessTravel()
{
	Super::PostSeamlessTravel();

	SetMatchState(EMatchState::MS_Warmup);
	for (TActorIterator<AReadyOrNotPlayerController> It(GetWorld()); It; ++It)
	{
		AReadyOrNotPlayerController* pc = *It;
		pc->bPlayerIsWaiting = false;
		RespawnPlayer(*It);
	}

	for (TActorIterator<AReadyOrNotPlayerController>It(GetWorld()); It; ++It)
	{
		AReadyOrNotPlayerController* PlayerController = *It;
		FString OnlineSessionId;
		IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
		if (OnlineSubsystem)
		{
			IOnlineSessionPtr SessionSubsystem = OnlineSubsystem->GetSessionInterface();
			FNamedOnlineSession* Session = SessionSubsystem->GetNamedSession(NAME_GameSession);
			if (Session)
			{
				OnlineSessionId = Session->SessionInfo->GetSessionId().ToString();
			}
			PlayerController->ClientJoinVoice(OnlineSessionId, 0);
		}
	}

	for (TActorIterator<AReadyOrNotPlayerController>It(GetWorld()); It; ++It)
	{
		if (It->bIsReplaySpectator)
		{
			It->Destroy();
		}
	}
}

bool AReadyOrNotGameMode::DoesLevelRequireGeneration()
{
	return false;	
}

void AReadyOrNotGameMode::SetMatchState(EMatchState newMatchState)
{
	V_LOGM(LogReadyOrNot, "Matchstate set to %s", *RON_ENUM_TO_STRING(EMatchState, newMatchState))
	
	CurrentMatchState = newMatchState;
	OnMatchStateChanged.Broadcast(newMatchState);
	
	if (AReadyOrNotGameState* gs = GetGameState<AReadyOrNotGameState>())
	{
		gs->MatchState = newMatchState;

		if (newMatchState == EMatchState::MS_MatchEnded)
		{
			gs->Multicast_OnGameEnded();
		}
	}
}

EMatchState AReadyOrNotGameMode::GetMatchState()
{
	return CurrentMatchState;
}

bool AReadyOrNotGameMode::AreAllPlayersDead()
{
	bool bAllPlayersDead = true;
	for (TActorIterator<APlayerController> It(GetWorld()); It; ++It)
	{
		APlayerController* pc = *It;
		APlayerCharacter* character = Cast<APlayerCharacter>(pc->GetPawn());
		if ((character && character->GetHealthComponent()->GetCurrentResource() > 0.0f))
		{
			bAllPlayersDead = false;
		}
	}
	
	return bAllPlayersDead;
}

bool AReadyOrNotGameMode::IsTeamDead(ETeamType Team, bool bIncludeArrestedAsDead)
{
	bool bTeamDead = true;
	for (TActorIterator<APlayerController> It(GetWorld()); It; ++It)
	{
		APlayerController* pc = *It;
		if (pc)
		{
			AReadyOrNotPlayerState* ps = Cast<AReadyOrNotPlayerState>(pc->PlayerState);
			if (ps && ps->GetTeam() == Team)
			{
				APlayerCharacter* character = Cast<APlayerCharacter>(pc->GetPawn());
				if (character)
				{
					if (bIncludeArrestedAsDead)
					{
						if (!character->IsArrested() && character->GetHealthComponent()->GetCurrentResource() > 0.0f)
							bTeamDead = false;
					}
					else
					{
						if (character->GetHealthComponent()->GetCurrentResource() > 0.0f)
							bTeamDead = false;
					}

				}
			}
		}
	}
	return bTeamDead;
}

APlayerController* AReadyOrNotGameMode::ProcessClientTravel(FString& FURL, bool bSeamless, bool bAbsolute)
{
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PlayerController = Iterator->Get();
		AReadyOrNotPlayerController* pc = Cast<AReadyOrNotPlayerController>(PlayerController);
		if (pc)
		{
			pc->Client_ClearHUDWidgets();
			pc->SetInputMode(FInputModeGameOnly());
		}
	}
	
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	return Super::ProcessClientTravel(FURL, bSeamless, bAbsolute);
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
}

void AReadyOrNotGameMode::SwapPlayerTeams()
{
	for (TActorIterator<APlayerController> It(GetWorld()); It; ++It)
	{
		APlayerController* pc = *It;
		AReadyOrNotPlayerState* ps = Cast<AReadyOrNotPlayerState>(pc->PlayerState);
		if (ps)
		{
			if (ps->GetTeam() == ETeamType::TT_SERT_BLUE)
			{
				ps->Server_SetTeam(ETeamType::TT_SERT_RED);
			}
			else if (ps->GetTeam() == ETeamType::TT_SERT_RED)
			{
				ps->Server_SetTeam(ETeamType::TT_SERT_BLUE);
			}
		}
	}
}

void AReadyOrNotGameMode::PlayerKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter)
{
	AReadyOrNotPlayerState* killedPlayerState = Cast<AReadyOrNotPlayerState>(KilledCharacter->GetPlayerState());
	AReadyOrNotPlayerState* InstigatorPlayerState = nullptr;
	if (InstigatorCharacter)
	{
		InstigatorPlayerState = Cast<AReadyOrNotPlayerState>(InstigatorCharacter->GetPlayerState());
	}

	AReadyOrNotGameState* gs = GetGameState<AReadyOrNotGameState>();

	if (InstigatorCharacter)
	{
		APlayerCharacter* InstigatorPlayerCharacter = Cast<APlayerCharacter>(InstigatorCharacter);
		if (InstigatorPlayerCharacter)
		{
			if (UBpGameplayHelperLib::GetDistanceBetweenActors(InstigatorCharacter, KilledCharacter) < 500.0f && InstigatorCharacter != KilledCharacter)
			{
				InstigatorPlayerCharacter->Server_PlayPVPSpeech("ConfirmedEnemyKilled", InstigatorPlayerCharacter->GetTeam());
			}
		}
		if (InstigatorPlayerState && killedPlayerState)
		{
			if ((!gs->bFreeForAll &&
				InstigatorPlayerState->GetTeam() == killedPlayerState->GetTeam()) || (killedPlayerState->GetTeam() == ETeamType::TT_CIVILIAN))
			{
				InstigatorPlayerState->Kills -= 1;
				InstigatorPlayerState->KillsThisLife -= 1;
				if (InstigatorPlayerState != killedPlayerState)
				{
					if (InstigatorPlayerState != killedPlayerState)
					{
						InstigatorPlayerState->TeamKills += 1;
					}
					InstigatorPlayerState->SetScore(InstigatorPlayerState->GetScore() - 1);

					if (GetReadyOrNotGameSession())
					{
						GetReadyOrNotGameSession()->AddTeamKill(InstigatorPlayerState->GetUniqueId().ToString());
						if (InstigatorPlayerState->TeamKills >= GetReadyOrNotGameSession()->MaxTeamKillsBeforeAutoKick)
						{
							KickPlayer(Cast<APlayerController>(InstigatorCharacter->GetController()), FText::FromString("You have been kicked due to excessive team kills."));
						}
					}
				}


			}
			else
			{
				InstigatorPlayerState->KillsThisLife += 1;
				InstigatorPlayerState->Kills += 1;
				InstigatorPlayerState->SetScore(InstigatorPlayerState->GetScore() + 1);
				if (InstigatorCharacter->GetPlayerState()->GetUniqueId().IsValid())
				{
					FString SteamId = InstigatorCharacter->GetPlayerState()->GetUniqueId().ToString();
					FString SteamName = InstigatorCharacter->GetPlayerState()->GetPlayerName();
					UCompetitionHelperLib::AddKill(EventID, SteamId, SteamName, KilledCharacter->GetPlayerState() ? KilledCharacter->GetPlayerState()->GetPlayerName() : "Unknown");
				}
			}
		}
	}
	

	if (KilledCharacter)
	{
		AReadyOrNotPlayerController* pc = Cast<AReadyOrNotPlayerController>(KilledCharacter->GetController());
		if (pc)
		{
			APlayerCharacter* KilledPlayerCharacter = Cast<APlayerCharacter>(KilledCharacter);
			if (KilledPlayerCharacter)
			{
				if (KilledPlayerCharacter->IsArrested())
				{
					gs->PlayAnnouncerForTeam("KilledArrestedPerson", GetOtherTeam(KilledPlayerCharacter->GetTeam()));
				}
				KilledPlayerCharacter->LocalDeathFeed(pc);
			}
			gs->Multicast_OnCharacterDied(Cast<APlayerCharacter>(KilledCharacter), Cast<APlayerCharacter>(InstigatorCharacter), InstigatorCharacter);
			pc->SetLastKilledCharacter(KilledCharacter);
			pc->FlushPressedKeys();
			pc->UnPossess();
			DeadPlayers.AddUnique(pc);
			switch (RespawnMode)
			{
				case ERespawnMode::NoRespawn:
				break;

				case ERespawnMode::ImmediateRespawn:
				{
					float RespawnTime = 5.0f;
					if (AReadyOrNotGameSession* Session = Cast<AReadyOrNotGameSession>(GameSession))
					{
						RespawnTime = Session->RespawnTimer;
					}

					//FTimerHandle RespawnPlayer_Handle;
					//GetWorld()->GetTimerManager().SetTimer(RespawnPlayer_Handle, FTimerDelegate::CreateUObject(this, &AReadyOrNotGameMode::RespawnPlayer, (APlayerController*)pc, false), RespawnTime, false);

					UReadyOrNotFunctionLibrary::StartTimerForCallback(this, FTimerDelegate::CreateUObject(this, &AReadyOrNotGameMode::RespawnPlayer, Cast<APlayerController>(pc), false), RespawnTime);

					pc->NotifyRespawnTime(RespawnTime);
				}
				break;

				case ERespawnMode::DelayedRespawn:
					//FTimerHandle AddDeadPlayer_Handle;
					//GetWorld()->GetTimerManager().SetTimer(AddDeadPlayer_Handle, FTimerDelegate::CreateUObject(this, &AReadyOrNotGameMode::AddPlayerToReinformcementTimer, (APlayerController*)pc), 1.0f, false);

					UReadyOrNotFunctionLibrary::StartTimerForCallback(this, FTimerDelegate::CreateUObject(this, &AReadyOrNotGameMode::AddPlayerToReinformcementTimer, Cast<APlayerController>(pc)), 1.0f, false);
				break;

				default: ;
			}

			FTransform LastActorTransform = KilledCharacter->GetActorTransform();
			ASpectatePawn* spectator = Cast<ASpectatePawn>(SpawnSpectator(pc, DeadSpectatorClass, LastActorTransform));
			
			if (spectator)
			{
				if (bSpectateKillerOnDeath)
				{
					spectator->AddActorLocalOffset(FVector(0, 0, 200), true);
					spectator->Killer = Cast<APlayerCharacter>(InstigatorCharacter);
					spectator->KilledCharacter = Cast<APlayerCharacter>(KilledCharacter);
					
				}
				else
				{
					spectator->AddActorLocalOffset(FVector(0, 0, 200), true);
					spectator->Killer = Cast<APlayerCharacter>(KilledCharacter);
					spectator->KilledCharacter = Cast<APlayerCharacter>(KilledCharacter);
				}
			}
		}
		
		if (killedPlayerState)
		{
			killedPlayerState->Deaths += 1;
			if (killedPlayerState->GetUniqueId().IsValid())
			{
				FString SteamId = killedPlayerState->GetUniqueId().ToString();
				FString SteamName = killedPlayerState->GetPlayerName();
				if (InstigatorCharacter)
				{
					UCompetitionHelperLib::AddDeath(EventID, SteamId, SteamName, InstigatorCharacter->GetPlayerState() ? InstigatorCharacter->GetPlayerState()->GetPlayerName() : "Unknown");
				}
			}
		}
	}
}

void AReadyOrNotGameMode::PlayerDowned(AReadyOrNotCharacter* DownedCharacter, AReadyOrNotCharacter* InstigatorCharacter)
{
}

void AReadyOrNotGameMode::AddPlayerToReinformcementTimer(APlayerController* DeadPlayer)
{
	RespawnableDeadPlayers.AddUnique(DeadPlayer);
}

void AReadyOrNotGameMode::PlayerArrested(AReadyOrNotCharacter* ArrestedCharacter, AReadyOrNotCharacter* InstigatorCharacter)
{
	if (InstigatorCharacter)
	{
		AReadyOrNotPlayerState* ps = Cast<AReadyOrNotPlayerState>(InstigatorCharacter->GetPlayerState());
		if (ps)
		{
			if (!bFirstArrestAnnouncementPlayed)
			{
				bFirstArrestAnnouncementPlayed = true;
				GetReadyOrNotGameState()->PlayAnnouncerForTeam("FirstArrest", ps->GetTeam());
			}
			else
			{
				GetReadyOrNotGameState()->PlayAnnouncerForTeam("EnemyArrested", ps->GetTeam());
				if (GetNumberOfArrestedPlayersOnTeam(ETeamType::TT_SERT_BLUE) > GetNumberOfArrestedPlayersOnTeam(ETeamType::TT_SERT_RED) + 2)
				{
					GetReadyOrNotGameState()->PlayAnnouncerForTeam("ArrestStreak", ETeamType::TT_SERT_BLUE);
				}
				if (GetNumberOfArrestedPlayersOnTeam(ETeamType::TT_SERT_RED) > GetNumberOfArrestedPlayersOnTeam(ETeamType::TT_SERT_BLUE) + 2)
				{
					GetReadyOrNotGameState()->PlayAnnouncerForTeam("ArrestStreak", ETeamType::TT_SERT_RED);
				}
				if (GetNumberOfFreePlayersOnTeam(ETeamType::TT_SERT_BLUE) == 1 && GetNumberOfArrestedPlayersOnTeam(ETeamType::TT_SERT_BLUE) != 0)
				{
					GetReadyOrNotGameState()->PlayAnnouncerForTeam("OneFreeTeamMateLeft", ETeamType::TT_SERT_BLUE);
				}
				if (GetNumberOfFreePlayersOnTeam(ETeamType::TT_SERT_RED) == 1 && GetNumberOfArrestedPlayersOnTeam(ETeamType::TT_SERT_RED) != 0)
				{
					GetReadyOrNotGameState()->PlayAnnouncerForTeam("OneFreeTeamMateLeft", ETeamType::TT_SERT_RED);
				}
				if (GetNumberOfFreePlayersOnTeam(ETeamType::TT_SERT_BLUE) == 1 && GetNumberOfArrestedPlayersOnTeam(ETeamType::TT_SERT_BLUE) != 0)
				{
					GetReadyOrNotGameState()->PlayAnnouncerForTeam("OnFreeEnemyLeft", ETeamType::TT_SERT_RED);
				}
				if (GetNumberOfFreePlayersOnTeam(ETeamType::TT_SERT_RED) == 1 && GetNumberOfArrestedPlayersOnTeam(ETeamType::TT_SERT_RED) != 0)
				{
					GetReadyOrNotGameState()->PlayAnnouncerForTeam("OnFreeEnemyLeft", ETeamType::TT_SERT_BLUE);
				}
				

			}
			ps->ArrestsThisLife += 1;
			ps->Arrests += 1;
			ps->SetScore(ps->GetScore() + 5);
			if (ps->GetUniqueId().IsValid())
			{
				FString SteamId = ps->GetUniqueId().ToString();
				FString SteamName = ps->GetPlayerName();
				UCompetitionHelperLib::AddArrest(EventID, SteamId, SteamName, ArrestedCharacter->GetPlayerState() ? ArrestedCharacter->GetPlayerState()->GetPlayerName() : "Unknown" );
			}
		}
	}

	if (ArrestedCharacter)
	{
		AReadyOrNotPlayerState* ps = Cast<AReadyOrNotPlayerState>(ArrestedCharacter->GetPlayerState());
		if (ps)
		{
			ps->TimesArrested += 1;
		}
	}
}

void AReadyOrNotGameMode::PlayerFreed(ACharacter* Freed, ACharacter* Freer)
{
	APlayerCharacter* FreedCharacter = Cast<APlayerCharacter>(Freed);
	if (!FreedCharacter)
	{
		return;
	}

	// Hide the objective marker
	//FreedCharacter->Multicast_HideObjectiveMarker();
}

void AReadyOrNotGameMode::PlayerTakenDamage(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* DamagedCharacter, AActor* DamageCauser, float Damage, float HealthRemaining)
{
	if (InstigatorCharacter)
	{
		AReadyOrNotPlayerState* ps = Cast<AReadyOrNotPlayerState>(InstigatorCharacter->GetPlayerState());
		if (ps)
		{
			ps->DamageDealt += Damage;
		}
	}

	if (DamagedCharacter)
	{
		AReadyOrNotPlayerState* ps = Cast<AReadyOrNotPlayerState>(DamagedCharacter->GetPlayerState());
		if (ps)
		{
			ps->DamageReceived += Damage;
		}
	}
}

// SpawnPlayerCharacter is used when initially spawning the player into the round, RespawnPlayerCharacter is used for actual respawns
APlayerCharacter* AReadyOrNotGameMode::SpawnPlayerCharacter(APlayerController* Controller, const TSubclassOf<APlayerCharacter> Class, const FTransform SpawnTransform)
{
	if (Controller && Class)
	{
		APawn* Pawn = Controller->GetPawn();
		
		Controller->UnPossess();
		
		if (Pawn)
		{
			Pawn->Destroy();
		}

		AReadyOrNotPlayerController* ronController = Cast<AReadyOrNotPlayerController>(Controller);
		if (ronController)
		{
			ronController->ClientSpawned();
			ronController->ClientSpawned_Implementation();
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		SpawnParams.bNoFail = true;
		
		V_LOGM(LogReadyOrNot, "Spawning Player Character for %s of class %s at %s", *Controller->PlayerState->GetPlayerName(), *Class->GetName(), *SpawnTransform.ToString());
		if (APlayerCharacter* NewChar = GetWorld()->SpawnActor<APlayerCharacter>(Class, SpawnTransform, SpawnParams))
		{
			if (AReadyOrNotPlayerState* ps = Cast<AReadyOrNotPlayerState>(Controller->PlayerState))
			{
				ps->bSpawnLoadout = true;
				ps->LastCharacter = NewChar;
			}

			Controller->Possess(NewChar);
			Controller->SetPawn(NewChar);
			LastPlayerSpawnPoint = SpawnTransform;
			
			if (AReadyOrNotPlayerController* pc = Cast<AReadyOrNotPlayerController>(Controller))
			{
				//FTimerHandle DestroyLastKilledCharacter_Handle;
				//GetWorld()->GetTimerManager().SetTimer(DestroyLastKilledCharacter_Handle, pc, &AReadyOrNotPlayerController::DestroyLastKilledCharacter, 60.0f, false);

				if (GetReadyOrNotGameState()->bPvPMode)
				{
					UReadyOrNotFunctionLibrary::StartTimerForCallback(pc, &AReadyOrNotPlayerController::DestroyLastKilledCharacter, 60.0f, false, true);
				}

				
				pc->Client_SetControlRotation(SpawnTransform.GetRotation().Rotator());
				
				pc->Client_ClearHUDWidgets();
				//pc->Client_CreateWidget("CharacterHUD", false, false);
				pc->Client_DisableUIMouse();

				//NewChar->HUD_Widget = CharacterHUD;
			}
			
			NewChar->OnCharacterKilled.AddDynamic(this, &AReadyOrNotGameMode::PlayerKilled);
			NewChar->OnPlayerDowned.AddDynamic(this, &AReadyOrNotGameMode::PlayerDowned);
			NewChar->OnPlayerArrested.AddDynamic(this, &AReadyOrNotGameMode::PlayerArrested);
			NewChar->OnCharacterTakeDamage.AddDynamic(this, &AReadyOrNotGameMode::PlayerTakenDamage);
			NewChar->OnPlayerFreed.AddDynamic(this, &AReadyOrNotGameMode::PlayerFreed);
			
			OnPlayerRespawned.Broadcast(NewChar, Controller);
			
			AReadyOrNotGameState* gs = GetGameState<AReadyOrNotGameState>();
			if (gs)
			{
				gs->Client_BindCharacterEvents(NewChar);
				gs->bInPlanningMenu = false;
				
				if (NewChar->GetTeam() == ETeamType::TT_SERT_BLUE)
				{
					gs->BlueTeamPlayers.AddUnique(NewChar);
				}
				else
				{
					gs->RedTeamPlayers.AddUnique(NewChar);
				}
			}
			
			return NewChar;
		}
	}
	
	return nullptr;
}

void AReadyOrNotGameMode::InitGameState()
{
	GameState->GameModeClass = GetClass();
	GameState->ReceivedGameModeClass();
	GameState->SpectatorClass = NormalSpectatorPawn;
	GameState->ReceivedSpectatorClass();

	for (TActorIterator<AMissionPlanManager> It(GetWorld()); It; ++It)
	{
		MissionPlanManager = *It;
		break;
	}
}

ASpectatorPawn* AReadyOrNotGameMode::SpawnSpectator(APlayerController* Controller, TSubclassOf<ASpectatorPawn> Class, FTransform SpawnTransform)
{
	if (Controller && Class)
	{
		if (Controller->GetPawn())
			Controller->GetPawn()->Destroy();

		ASpectatorPawn* newSpectatePawn;
		if(Class != DeadSpectatorClass)
		{
			newSpectatePawn = GetWorld()->SpawnActor<ASpectatorPawn>(NormalSpectatorPawn, SpawnTransform);
		}
		else
		{
			newSpectatePawn = GetWorld()->SpawnActor<ASpectatorPawn>(Class, SpawnTransform);
		}
		
		if (newSpectatePawn)
		{
			if (Controller->PlayerState)
			{
				V_LOGM(LogReadyOrNot, "Spawning spectator pawn for %s of class %s at %s", *Controller->PlayerState->GetPlayerName(), *Class->GetName(), *SpawnTransform.ToString());
			}
			Controller->Possess(newSpectatePawn);
			
			
			return newSpectatePawn;
			
		}
	}
	return nullptr;
}

bool AReadyOrNotGameMode::RemoveDeadPlayer(APlayerController* InPlayerController)
{
	DeadPlayers.Remove(InPlayerController);
	RespawnableDeadPlayers.Remove(InPlayerController);
	return InPlayerController != nullptr;
}

bool AReadyOrNotGameMode::RemoveDeadPlayerAt(const int32 Index)
{
	DeadPlayers.RemoveAt(Index);
	RespawnableDeadPlayers.RemoveAt(Index);
	return DeadPlayers.IsValidIndex(Index);
}

void AReadyOrNotGameMode::RespawnDeadPlayers()
{
	

	int32 RedSpawned = 0;
	int32 BlueSpawned = 0;

	for (TActorIterator<AReadyOrNotPlayerController> It(GetWorld()); It; ++It)
	{
		AReadyOrNotPlayerController* pc = *It;
		AReadyOrNotPlayerState* ps = Cast<AReadyOrNotPlayerState>(pc->PlayerState);
		if (ps)
		{
			// respawn any players who have just joined the game..
			if (ps->bReady && !ps->bIsInGame && !ps->IsOnlyASpectator())
			{
				RespawnableDeadPlayers.AddUnique(pc);
			}
		}
	}
	for (int32 i = 0; i < RespawnableDeadPlayers.Num(); i++)
	{
		if (!RespawnableDeadPlayers[i])
		{
			RespawnableDeadPlayers.RemoveAt(i);
			i--;
			continue;
		}

		FTransform SpawnTransform;
		AActor* startPoint = FindPlayerStart(RespawnableDeadPlayers[i]);
		if (startPoint)
		{
			SpawnTransform = startPoint->GetActorTransform();
			if (RespawnableDeadPlayers[i])
			{
				AReadyOrNotPlayerState* ps = Cast<AReadyOrNotPlayerState>(RespawnableDeadPlayers[i]->PlayerState);
				if (ps)
				{
					ps->bIsInGame = true;
					if (ps->GetTeam() == ETeamType::TT_SERT_BLUE)
					{
						RespawnPlayer(RespawnableDeadPlayers[i]);
						BlueSpawned++;
					}
					else if (ps->GetTeam() == ETeamType::TT_SERT_RED)
					{
						RespawnPlayer(RespawnableDeadPlayers[i]);
						RedSpawned++;
					}

				}
			}
			DeadPlayers.Remove(RespawnableDeadPlayers[i]);
			RespawnableDeadPlayers.RemoveAt(i);
			i--;
		}
	}

	AReadyOrNotGameState* gs = Cast<AReadyOrNotGameState>(GetWorld()->GetGameState());
	if (gs)
	{
		if (BlueSpawned > 0 && RedSpawned > 0)
		{
			//gs->PlayAnnouncerForTeam("AllReinforcementsSpawned", ETeamType::TT_NONE);
		}
		else if (BlueSpawned > 0)
		{
			//gs->PlayAnnouncerForTeam("EnemyReinforcementsSpawned", ETeamType::TT_SERT_RED);
			//gs->PlayAnnouncerForTeam("TeamReinforcementsSpawned", ETeamType::TT_SERT_BLUE);
		}
		else if (RedSpawned > 0)
		{
			//gs->PlayAnnouncerForTeam("EnemyReinforcementsSpawned", ETeamType::TT_SERT_BLUE);
			//gs->PlayAnnouncerForTeam("TeamReinforcementsSpawned", ETeamType::TT_SERT_RED);
		}
	}
	
}

//
void AReadyOrNotGameMode::RespawnAllPlayers()
{
	V_LOGM(LogReadyOrNot, "Respawning ALL players!");
	// We can safely empty this we are already spawning all of the players
	DeadPlayers.Empty();
	RespawnableDeadPlayers.Empty();

	if (AReadyOrNotGameState* GS = GetGameState<AReadyOrNotGameState>())
	{
		GS->BlueTeamPlayers.Empty();
		GS->RedTeamPlayers.Empty();
	}
	
	for (TActorIterator<AReadyOrNotPlayerController> It(GetWorld()); It; ++It)
	{
		AReadyOrNotPlayerController* controller = *It;
		controller->DestroyLastKilledCharacter();

		AReadyOrNotPlayerState* ps = Cast<AReadyOrNotPlayerState>(controller->PlayerState);
		if (ps)
		{
			if(ps->bReady || ps->bJoinInProgress)
			{
				ps->bIsInGame = true;
				RespawnPlayer(controller);
#if !WITH_EDITOR
				controller->ClientSetCameraFade(true, FColor::Black, FVector2D(1.0f, 1.0f), START_MATCH_FADE_TIME, false);
#endif
			}
			else
			{
				// don't set the player to IsInGame, respawn as spectator
				RespawnPlayer(controller, true);
			}
		}

		APlayerCharacter* pc = Cast<APlayerCharacter>(controller->GetPawn());
		if (pc)
		{
			pc->Multicast_ShowThirdPerson();
			pc->Multicast_ShowThirdPerson_Implementation();
		}
	}
}

void AReadyOrNotGameMode::RespawnAllPlayersOnTeam(ETeamType Team)
{
	V_LOGM(LogReadyOrNot, "Respawning team players!");
	for (TActorIterator<AReadyOrNotPlayerController> It(GetWorld()); It; ++It)
	{
		AReadyOrNotPlayerController* controller = *It;
		AReadyOrNotPlayerState* ps = Cast<AReadyOrNotPlayerState>(controller->PlayerState);
		if (ps && ps->Team == Team)
		{
			if (ps->bReady || ps->bJoinInProgress)
			{
				ps->bIsInGame = true;
				RespawnPlayer(controller);
			}
			else
			{
				// don't set the player to IsInGame, respawn as spectator
				RespawnPlayer(controller, true);
			}
		}
		else
		{
			continue;
		}

		APlayerCharacter* pc = Cast<APlayerCharacter>(controller->GetPawn());
		if (pc)
		{
			pc->Multicast_ShowThirdPerson();
			pc->Multicast_ShowThirdPerson_Implementation();
		}
	}
}

void AReadyOrNotGameMode::RespawnDeadPlayersOnTeam(ETeamType Team)
{
	for (TActorIterator<AReadyOrNotPlayerController> It(GetWorld()); It; ++It)
	{
		AReadyOrNotPlayerController* pc = *It;
		AReadyOrNotPlayerState* ps = Cast<AReadyOrNotPlayerState>(pc->PlayerState);
		if (ps && ps->Team == Team)
		{
			// respawn any players who have just joined the game..
			if (ps->bReady && !ps->bIsInGame && !ps->IsOnlyASpectator())
			{
				RespawnableDeadPlayers.AddUnique(pc);
			}
		}
	}

	for (int32 i = 0; i < RespawnableDeadPlayers.Num(); i++)
	{
		if (!RespawnableDeadPlayers[i])
		{
			RespawnableDeadPlayers.RemoveAt(i);
			i--;
			continue;
		}

		FTransform SpawnTransform;
		AActor* startPoint = FindPlayerStart(RespawnableDeadPlayers[i]);
		if (startPoint)
		{
			SpawnTransform = startPoint->GetActorTransform();
			if (RespawnableDeadPlayers[i])
			{
				AReadyOrNotPlayerState* ps = Cast<AReadyOrNotPlayerState>(RespawnableDeadPlayers[i]->PlayerState);
				if (ps)
				{
					if (ps->Team != Team)
					{
						continue;
					}
					ps->bIsInGame = true;
					RespawnPlayer(RespawnableDeadPlayers[i]);
				}
			}
			DeadPlayers.Remove(RespawnableDeadPlayers[i]);
			RespawnableDeadPlayers.RemoveAt(i);
			i--;
		}
	}
}



void AReadyOrNotGameMode::RespawnPlayer(APlayerController* Player, bool bForceSpecator)
{
	AReadyOrNotPlayerController* pc = Cast<AReadyOrNotPlayerController>(Player);
	if (pc)
	{
#if !WITH_EDITOR
		AReadyOrNotGameState* gs = Cast<AReadyOrNotGameState>(GetWorld()->GetGameState());
		if (gs)
		{
			if (gs->bPvPMode)
			{
				// auto balance some blue onto red
				AReadyOrNotPlayerState* RandomSwapPS = Cast<AReadyOrNotPlayerState>(Player->PlayerState);
				if (RandomSwapPS)
				{
					if (!RandomSwapPS->bJoinedOnSquadLeader)
					{
						if (gs->GetPlayerStatesOfTeam(ETeamType::TT_SERT_RED).Num() > gs->GetPlayerStatesOfTeam(ETeamType::TT_SERT_BLUE).Num() + 1)
						{
							RandomSwapPS->Team = ETeamType::TT_SERT_BLUE;
							gs->Multicast_BroadcastChatMessage(FRChatMessage("SYSTEM", "Team balancing has auto-balanced"
							+ RandomSwapPS->GetPlayerName() + "to SWAT", FLinearColor::Blue, pc));
						} else if (gs->GetPlayerStatesOfTeam(ETeamType::TT_SERT_BLUE).Num() > gs->GetPlayerStatesOfTeam(ETeamType::TT_SERT_RED).Num() + 1)
						{
							RandomSwapPS->Team = ETeamType::TT_SERT_RED;
							gs->Multicast_BroadcastChatMessage(FRChatMessage("SYSTEM", "Team balancing has auto-balanced"
							+ RandomSwapPS->GetPlayerName() + "to MLO", FLinearColor::Red, pc));
						}
					}
				}
			}
		}
#endif
		
		AReadyOrNotPlayerState* ps = Cast<AReadyOrNotPlayerState>(pc->PlayerState);
		
		if (ps)
			V_LOGM(LogReadyOrNot, "Respawning player %s", *ps->GetPlayerName());
		
		// order is important here set the team before getting the spawnpoint
		if (ps) ps->TrySetPendingTeamAsTeam();
		if (ps) PlayerSpawnTag = ps->PlayerSpawnTag;

		/*
		AActor* startPoint = GetThisPlayersStartPointByTag(pc,PlayerSpawnTag); //ChoosePlayerStart(pc); // Was FindPlayerStart
		if(startPoint == nullptr)
        {
			startPoint = FindPlayerStart(pc);
        }
		*/
			
		AActor* StartPoint = nullptr;
		
		for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
		{
			APlayerStart* CurrentStart = *It;
			if (CurrentStart)
			{
				if (CurrentStart->PlayerStartTag == FName(PlayerSpawnTag))
				{
					StartPoint = CurrentStart;
					break;
				}
			}
		}

		//AActor* startPoint = GetThisPlayersStartPointByTag(Controller, PlayerSpawnTag); //ChoosePlayerStart(pc); // Was FindPlayerStart
		if (StartPoint == nullptr)
		{
			StartPoint = FindPlayerStart(pc);
		}
		
		if (StartPoint && ps)
		{
			const FTransform SpawnTransform = StartPoint->GetActorTransform();
			if ((GetMatchState() == EMatchState::MS_Warmup || GetMatchState() == EMatchState::MS_MatchEnded) && !bForceSpecator)
			{
				SpawnSpectator(pc, SpectatorClass, SpawnTransform);
				AReadyOrNotPlayerController* pcontroller = Cast<AReadyOrNotPlayerController>(pc);
				if (pcontroller)
				{
					pcontroller->Client_SetControlRotation(SpawnTransform.GetRotation().Rotator());
				}
			}
			else if ((ps && !ps->bIsInGame && GetMatchState() == EMatchState::MS_Playing) || bForceSpecator)
			{
				SpawnSpectator(pc, SpectatorClass, SpawnTransform);
				AReadyOrNotPlayerController* pcontroller = Cast<AReadyOrNotPlayerController>(pc);
				if (pcontroller)
				{
					FTimerHandle DestroyLastKilledCharacter_Handle;
					GetWorld()->GetTimerManager().SetTimer(DestroyLastKilledCharacter_Handle, pcontroller, &AReadyOrNotPlayerController::DestroyLastKilledCharacter, 60.0f, false);
					pcontroller->Client_SetControlRotation(SpawnTransform.GetRotation().Rotator());
				}
			}
			else if (GetMatchState() == EMatchState::MS_Playing)
			{
				ps->bIsInGame = true;
				
				if (ps->GetTeam() == ETeamType::TT_SERT_BLUE)
				{
					SpawnPlayerCharacter(pc, BlueCharacterClass.LoadSynchronous(), SpawnTransform);
				}
				else if (ps->GetTeam() == ETeamType::TT_SERT_RED)
				{
					SpawnPlayerCharacter(pc, RedCharacterClass.LoadSynchronous(), SpawnTransform);
				}
			}
		}
	}
}

AActor* AReadyOrNotGameMode::GetThisPlayersStartPointByTag( APlayerController* Player, const FString& IncomingName)
{
	const FName CurrentStartTag = FName(IncomingName);
	AActor* SpawnPoint = nullptr;
	TArray<AActor*> SpawnArray;
	SpawnArray.Empty();
	UWorld* World = GetWorld();
	if (World)
	{
		if (IncomingName.IsEmpty())
		{
			for (TActorIterator<APlayerStart> It(World); It; ++It)
			{
				APlayerStart* CurrentStart = *It;
				if (CurrentStart)
				{
					if(CurrentStart->PlayerStartTag == FName("None")||CurrentStart->PlayerStartTag.GetStringLength()<=0||CurrentStart->PlayerStartTag == FName("Default"))
					{
						SpawnArray.Add(CurrentStart);
					}
				}
			}
		}
		else
		{
			//get all spawn points by tag together in an array
            			
			for (TActorIterator<APlayerStart> It(World); It; ++It)
			{
				APlayerStart* CurrentStart = *It;
				if (CurrentStart)
				{
					if(CurrentStart->PlayerStartTag == CurrentStartTag)
					{
						SpawnArray.Add(CurrentStart);
					}
				}
			}
		}
	}
	
	//sort through player spawn points by comparing their array index to the gamestates array of players, this is to prevent double ups on the same spawn point. 
	TArray<APlayerState*> Players = GetWorld()->GetGameState()->PlayerArray;
	if((SpawnArray.Num()>0)&&(Players.Num()>0))
	{
		for(int i = 0; i < Players.Num(); ++i)
		{
			if(i >= SpawnArray.Num())
			{
				continue;
			}
			
			if(IsValid(Players[i])&&IsValid(SpawnArray[i]))
			{
				if(Player->PlayerState)
				{
					if(Players[i]==Player->PlayerState)
					{
						SpawnPoint = SpawnArray[i];
					}
				}
			}
		}
	}
	
	return SpawnPoint;
}

AActor* AReadyOrNotGameMode::GetRandomSafeStart()
{
	V_LOGM(LogReadyOrNot, "Getting Random Safe Start!");
	TArray<AActor*> compatiblePlayerStarts;
	TArray<AActor*> safeStarts;

	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		APlayerStart* start = *It;
		if (start)
		{
			bool bIsSafeSpawn = true;
			for (TActorIterator<APlayerCharacter> PcIt(GetWorld()); PcIt; ++PcIt)
			{
				if (UBpGameplayHelperLib::HasLineOfSight(*PcIt, start))
				{
					bIsSafeSpawn = false;
				}
			}
			if (bIsSafeSpawn)
			{
				safeStarts.Add(start);
			}
			compatiblePlayerStarts.Add(start);
		}
		else if (start->PlayerStartTag == "RandomSpawn")
		{
			compatiblePlayerStarts.Add(start);
		}
	}

	if (safeStarts.Num() > 0)
	{
		return safeStarts[FMath::RandRange(0, safeStarts.Num() - 1)];
	}
	else if (compatiblePlayerStarts.Num() > 0)
	{
		return compatiblePlayerStarts[FMath::RandRange(0, compatiblePlayerStarts.Num() - 1)];
	}

	return nullptr;
}

APlayerStart* AReadyOrNotGameMode::FindPlayerStartWithTag(const FName& Tag) const
{
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		APlayerStart* PS = *It;
		if (PS->PlayerStartTag == Tag)
		{
			return PS;
		}
	}

	return nullptr;
}

TArray<APlayerCharacter*> AReadyOrNotGameMode::GetAllPlayerCharactersInWorld() const
{
	TArray<APlayerCharacter*> PlayerCharacters;

	for (TActorIterator<APlayerCharacter> It(GetWorld()); It; ++It)
	{
		PlayerCharacters.Add(Cast<APlayerCharacter>(*It));
	}

	return PlayerCharacters;
}

void AReadyOrNotGameMode::CheckToAnnounceTeamkill_Implementation(ACharacter* InstigatorCharacter, ACharacter* KilledCharacter)
{
	APlayerCharacter* KilledPc = Cast<APlayerCharacter>(KilledCharacter);
	if (GetMatchState() == EMatchState::MS_Playing && InstigatorCharacter && KilledPc)
	{
		AReadyOrNotPlayerState* ips = InstigatorCharacter->GetPlayerState<AReadyOrNotPlayerState>();
		AReadyOrNotPlayerState* kps = KilledPc->LastPlayerState;
		AReadyOrNotGameState* gs = GetGameState<AReadyOrNotGameState>();

		if (gs && ips && kps && ips->GetTeam() == kps->GetTeam())
		{
			gs->PlayAnnouncerForTeam("EnemyFriendlyFire", ips->GetTeam() == ETeamType::TT_SERT_BLUE ? ETeamType::TT_SERT_RED : ETeamType::TT_SERT_BLUE);
		}
	}
}

void AReadyOrNotGameMode::RequestNewLoadout(AReadyOrNotCharacter* Character, FSavedLoadout Loadout)
{
	if (Character)
	{
		FLoadoutEquipOptions LoadoutEquipOptions;
		if (Character->GetEquippedItem())
		{
			LoadoutEquipOptions.EquipItemCategory = Character->GetEquippedItem()->ItemCategories.Contains(EItemCategory::IC_Secondary) ? EItemCategory::IC_Secondary : EItemCategory::IC_Primary;
		}
		Character->GetInventoryComponent()->DestroyAllEquippedItems();
		UBpGameplayHelperLib::EquipLoadoutOnPlayer(Loadout, Character, LoadoutEquipOptions);
	}
}

bool AReadyOrNotGameMode::GetIsExfilEnabled()
{
	return bIsExfilEnabled;
}

void AReadyOrNotGameMode::SetExfilEnabled(bool bEnabled)
{
	bIsExfilEnabled = bEnabled;
	OnExfilEnabledChange.Broadcast(bEnabled);
}

void AReadyOrNotGameMode::ExfiltrateMission(TArray<ASWATCharacter*> ExfilCharacters)
{
	bMissionExfiltrated = true;
	SetExfilEnabled(false);
	OnExfiltrateMission.Broadcast();
}
