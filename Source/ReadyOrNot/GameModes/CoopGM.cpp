// Copyright Void Interactive, 2023

#include "CoopGM.h"
#include "CoopGS.h"
#include "NavigationSystem.h"

#include "Characters/ReplayController.h"

#include "ReadyOrNotLevelScript.h"

#include "Actors/Gameplay/AISpawn.h"
#include "Actors/DeployableDepot.h"
#include "Actors/HighgroundVolume.h"
#include "Actors/Gameplay/SniperSpawn.h"
#include "Actors/Gameplay/ReadyOrNotPlayerState.h"
#include "Actors/Gameplay/RosterScenarioSpawner.h"
#include "Actors/WorldDataGenerator.h"

#include "Characters/ReadyOrNotPlayerController.h"
#include "Characters/CyberneticCharacter.h"
#include "Characters/AI/SWATCharacter.h"
#include "Characters/AI/SuspectCharacter.h"
#include "Characters/AI/CivilianCharacter.h"
#include "Characters/CyberneticController.h"

#include "Data/LevelData.h"

#include "Info/ScoringManager.h"
#include "Info/TOCManager.h"

#include "lib/ReadyOrNotFunctionLibrary.h"

#include "NavModifierVolume.h"
#include "ReadyOrNotAIConfig.h"
#include "ReadyOrNotGameSession.h"
#include "Actors/Door.h"
#include "Actors/Environment/ExfilPortal.h"
#include "Actors/Environment/MissionPortal.h"
#include "Actors/Gameplay/BombActor.h"
#include "Actors/Gameplay/IncapacitatedHuman.h"
#include "Characters/AI/TrailerSWATCharacter.h"
#include "Commander/MetaGameProfile.h"
#include "Components/MoraleComponent.h"
#include "Info/ReadyOrNotSignificanceManager.h"
#include "Info/SWATManager.h"
#include "Navigation/ReadyOrNotNavAreas.h"

#include "Algo/RandomShuffle.h"
#include "Commander/CampaignData.h"
#include "Commander/CommanderGM.h"
#include "Commander/RosterManager.h"

DECLARE_CYCLE_STAT(TEXT("COOP ~ Begin Play"), STAT_COOPBeginPlay, STATGROUP_COOPGameMode);
DECLARE_CYCLE_STAT(TEXT("COOP ~ Tick"), STAT_COOPTick, STATGROUP_COOPGameMode);
DECLARE_CYCLE_STAT(TEXT("COOP ~ Start Match"), STAT_COOPStartMatch, STATGROUP_COOPGameMode);
DECLARE_CYCLE_STAT(TEXT("COOP ~ Reset Level"), STAT_COOPResetLevel, STATGROUP_COOPGameMode);
DECLARE_CYCLE_STAT(TEXT("COOP ~ Mission End"), STAT_COOPMissionEnd, STATGROUP_COOPGameMode);
DECLARE_CYCLE_STAT(TEXT("COOP ~ Find Player Start"), STAT_COOPFindPlayerStart, STATGROUP_COOPGameMode);
DECLARE_CYCLE_STAT(TEXT("COOP ~ Find Player Start For Team"), STAT_COOPFindPlayerStartForTeam, STATGROUP_COOPGameMode);
DECLARE_CYCLE_STAT(TEXT("COOP ~ Find New Squad Leader"), STAT_COOPFindNewSquadLeader, STATGROUP_COOPGameMode);
DECLARE_CYCLE_STAT(TEXT("COOP ~ Can Take Damage"), STAT_COOPCanTakeDamage, STATGROUP_COOPGameMode);
DECLARE_CYCLE_STAT(TEXT("COOP ~ Enemy AI Killed"), STAT_COOPEnemyAIKilled, STATGROUP_COOPGameMode);
DECLARE_CYCLE_STAT(TEXT("COOP ~ Player Killed"), STAT_COOPPlayerKilled, STATGROUP_COOPGameMode);
DECLARE_CYCLE_STAT(TEXT("COOP ~ Friendly AI Killed"), STAT_COOPFriendlyAIKilled, STATGROUP_COOPGameMode);
DECLARE_CYCLE_STAT(TEXT("COOP ~ Enemy AI Arrested"), STAT_COOPEnemyAIArrested, STATGROUP_COOPGameMode);
DECLARE_CYCLE_STAT(TEXT("COOP ~ Enemy AI Bullet Hit"), STAT_COOPEnemyAIBulletHit, STATGROUP_COOPGameMode);
DECLARE_CYCLE_STAT(TEXT("COOP ~ Spawn Traps"), STAT_COOPSpawnTraps, STATGROUP_COOPGameMode);
DECLARE_CYCLE_STAT(TEXT("COOP ~ Spawn Deployables"), STAT_COOPSpawnDeployables, STATGROUP_COOPGameMode);
DECLARE_CYCLE_STAT(TEXT("COOP ~ Spawn Police"), STAT_COOPSpawnPolice, STATGROUP_COOPGameMode);
DECLARE_CYCLE_STAT(TEXT("COOP ~ Spawn AI Officer"), STAT_COOPSpawnAIOfficer, STATGROUP_COOPGameMode);
DECLARE_CYCLE_STAT(TEXT("COOP ~ Spawn AI Trailer Officer"), STAT_COOPSpawnAITrailerOfficer, STATGROUP_COOPGameMode);
DECLARE_CYCLE_STAT(TEXT("COOP ~ Spawn Personnel"), STAT_COOPSpawnPersonnel, STATGROUP_COOPGameMode);
DECLARE_CYCLE_STAT(TEXT("COOP ~ Spawn Single Personnel"), STAT_COOPSpawnSinglePersonnel, STATGROUP_COOPGameMode);
DECLARE_CYCLE_STAT(TEXT("COOP ~ Spawn Personnel Highground"), STAT_COOPSpawnPersonnel_Highground, STATGROUP_COOPGameMode);
DECLARE_CYCLE_STAT(TEXT("COOP ~ Remove All Spawned AI"), STAT_COOPRemoveAllSpawnedAI, STATGROUP_COOPGameMode);
DECLARE_CYCLE_STAT(TEXT("COOP ~ Check Win Conditions"), STAT_COOPCheckWinConditions, STATGROUP_COOPGameMode);
DECLARE_CYCLE_STAT(TEXT("COOP ~ Find Spot Upgrade SWAT"), STAT_COOPFindSpotUpgradeSWAT, STATGROUP_COOPGameMode);
DECLARE_CYCLE_STAT(TEXT("COOP ~ Spawn Suspects And Hostages"), STAT_COOPSpawnSuspectsAndHostages, STATGROUP_COOPGameMode);

TAutoConsoleVariable<int32> CVarRonNoSpawnSwat(TEXT("a.RonNoSpawnSWAT"), 0, TEXT("Disable to stop swat from spawning on game start."));
TAutoConsoleVariable<int32> CVarRonSpawnSwatInIncompatibleMode(TEXT("a.RonSpawnSwatInIncompatibleMode"), 0, TEXT("Toggle on to always spawn the swat (even if the level won't allow it or its a multplayer game)"));
TAutoConsoleVariable<int32> CVarRonEndMissionInEditor(TEXT("a.RonEndMissioninEditor"), 1, TEXT("Can the mission end in co-op?"));
TAutoConsoleVariable<int32> CVarRonSpawnNoSuspects(TEXT("a.RonSpawnNoSuspects"), 0, TEXT("Spawn no suspects in the mission"));

ACoopGM::ACoopGM()
{
	bInitialPlayerRespawn = false;
	bTimelimitUsedInMode = false;
	RespawnMode = ERespawnMode::NoRespawn;
	bIsExfilEnabled = true;
}

void ACoopGM::BeginPlay()
{
	Super::BeginPlay();
	
	SCOPE_CYCLE_COUNTER(STAT_COOPBeginPlay);

	UMetaGameProfile* MetaGameProfile = UMetaGameProfile::LoadProfile();
	if (ensureAlways(MetaGameProfile))
	{
		MetaGameProfile->ClearTemporaryData();
		MetaGameProfile->SaveProfile();
	}
	
	if (ACoopGS* gs = GetWorld()->GetGameState<ACoopGS>())
	{
		gs->Mode = Mode;
		gs->OnRep_COOPMode();
	}

	FString MapName = GetWorld()->GetMapName();
	MapName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);
	MapName.RemoveFromEnd("_Core");
	switch(Mode)
	{
		case ECOOPMode::CM_BombThreat:			MapName += "BombThreat"; break;
		case ECOOPMode::CM_ActiveShooter:		MapName += "ActiveShooter"; break;
		case ECOOPMode::CM_HostageRescue:		MapName += "HostageRescue"; break;
		case ECOOPMode::CM_BarricadedSuspects:	MapName += "BarricadedSuspects"; break;
		case ECOOPMode::CM_Raid:				MapName += "Raid"; break;
		default: ;
	}
	
	MapName += "_Core";

	if (UReadyOrNotStatics::DoesMapExist(MapName))
	{
		ProcessServerTravel(MapName);
		return;
	}
}

void ACoopGM::StartMatch()
{
	SCOPE_CYCLE_COUNTER(STAT_COOPStartMatch);

	Super::StartMatch();

	ResetLevel();
	RespawnAllPlayers();
	
	// always spawn police here as they spawn behind the player (and it will be singleplayer anyway)
	#if WITH_EDITOR
	if (CVarRonNoSpawnSwat.GetValueOnAnyThread() == 0)
		SpawnPolice();
	#else
		SpawnPolice();
	#endif

	UReadyOrNotFunctionLibrary::StartTimerForCallback(CheckWinConditions_Handle, this, &ACoopGM::CheckWinConditions, 1.0f, true, false, 10.0f);
	
	InitWorld();
	InitAI();

	ResetSquadLeader();
}

void ACoopGM::ResetLevel()
{
	Super::ResetLevel();

	SCOPE_CYCLE_COUNTER(STAT_COOPResetLevel);

	GetWorld()->GetTimerManager().ClearTimer(FindSpotUpgrade_Handle);

	// AI is spawned during warmup no
	if (GetMatchState() > EMatchState::MS_Playing)
	{
		RemoveAllSpawnedAI();
	}
}

void ACoopGM::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void ACoopGM::PostSeamlessTravel()
{
	Super::PostSeamlessTravel();

	ResetSquadLeader();	
}

void ACoopGM::ResetSquadLeader()
{
	for (TActorIterator<AReadyOrNotPlayerController>It(GetWorld()); It; ++It)
	{
		AReadyOrNotPlayerController* PlayerController = *It;
		if (PlayerController->GetRoNPlayerState())
		{
			PlayerController->GetRoNPlayerState()->bSquadLeader = false;
		}
	}
	for (TActorIterator<AReadyOrNotPlayerController>It(GetWorld()); It; ++It)
	{
		AReadyOrNotPlayerController* PlayerController = *It;
		if (PlayerController->GetRoNPlayerState())
		{
			PlayerController->GetRoNPlayerState()->bSquadLeader = true;
			break;
		}
	}
}

void ACoopGM::AutoAssignTeam(AController* Player)
{
	if (AReadyOrNotPlayerState* ps = Cast<AReadyOrNotPlayerState>(Player->PlayerState))
	{
		ps->Server_SetTeam(ETeamType::TT_SERT_BLUE);
	}
}

void ACoopGM::OnNavigationInitDone()
{
	TryGenerateWorld();
}

void ACoopGM::TryGenerateWorld()
{
	// for modded maps!
	#if !WITH_EDITOR
	if (UWorld* World = GetWorld())
	{
		const bool bIsValidMap = !World->GetMapName().Contains("TransitionMap") && !World->GetMapName().Contains("Lobby") && !World->GetMapName().Contains("Station");
		if (bIsValidMap && DoesLevelRequireGeneration())
		{
			if (!WorldDataGenerator)
				WorldDataGenerator = World->SpawnActor<AWorldDataGenerator>(AWorldDataGenerator::StaticClass());
				
			if (WorldDataGenerator)
			{
				WorldDataGenerator->LoadGenerationFromFile();
			}
		}
	}
	#endif
}

bool ACoopGM::DoesLevelRequireGeneration()
{
	if (WorldDataGenerator)
	{
		return !WorldDataGenerator->bHasWorldEverBeenGenerated;
	}
		
	return true;
}

void ACoopGM::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	SCOPE_CYCLE_COUNTER(STAT_COOPTick);

	if (!GetWorld() || (GetWorld() && GetWorld()->bIsTearingDown))
		return;

	ACoopGS* gs = Cast<ACoopGS>(GetWorld()->GetGameState());
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
				int GamesPlayed = ++UReadyOrNotStatics::GetReadyOrNotGameInstance()->DedicatedServerGamesPlayedWithoutReturningToLobby;
				UReadyOrNotStatics::GetReadyOrNotGameInstance()->DedicatedServerMapIdx++;
				AReadyOrNotGameSession* session = Cast<AReadyOrNotGameSession>(UReadyOrNotStatics::GetReadyOrNotGameMode()->GameSession);
				if (session)
				{
					if (GamesPlayed >= session->ReturnToLobbyAfterXMissions)
					{
						FString LobbyLevel = GetGameInstance<UReadyOrNotGameInstance>()->LobbyLevel;
						ProcessServerTravel(LobbyLevel + "?game=lobby");
					}
					else
					{
						
						AMissionPortal* MissionPortal = GetWorld()->SpawnActor<AMissionPortal>();
						MissionPortal->SelectRandomMission();
						FString MissionURL;
						MissionPortal->GetSelectedMission(MissionURL);
						ProcessServerTravel(MissionURL, false);
					}
				}
				
			}
		}
	}

#if !UE_BUILD_SHIPPING
	if (CVarRonNoSpawnSwat.GetValueOnAnyThread() == 1)
	{
		for (TActorIterator<ASWATCharacter> It(GetWorld()); It; ++It)
		{
			ASWATCharacter* Swat = *It;
			Swat->Destroy();
		}
	}

	if (CVarRonSpawnNoSuspects.GetValueOnAnyThread() == 1)
	{
		for (TActorIterator<ASuspectCharacter>It(GetWorld()); It; ++It)
		{
			It->Destroy();
		}
		
		for (TActorIterator<ACivilianCharacter>It(GetWorld()); It; ++It)
		{
			It->Destroy();
		}
	}
#endif

	if (bHiddenTigerCrouchingDragon)
	{
		Cast<ACoopGS>(gs)->bCrouchingTigerHiddenDragon = bHiddenTigerCrouchingDragon;
		for (TActorIterator<ACyberneticCharacter>It(GetWorld()); It; ++It)
		{
			It->Destroy();
		}
	}
}

void ACoopGM::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
#if !UE_BUILD_DEVELOPMENT
	ErrorMessage = "Unable to join. Game is in progress";
#endif

	if (ErrorMessage.IsEmpty())
	{
		Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
	}
}

bool ACoopGM::AreAllPlayersDead()
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
	
	for (TActorIterator<ASWATCharacter> It(GetWorld()); It; ++It)
	{
		ASWATCharacter* Swat = *It;
		if (Swat && Swat->IsActive() && !Cast<ATrailerSWATCharacter>(Swat))
		{
			bAllPlayersDead = false;
			break;
		}
	}

	return bAllPlayersDead;
}

void ACoopGM::ReturnToStation()
{
	FString Level = GetGameInstance<UReadyOrNotGameInstance>()->LobbyLevel;
	FString GameMode = "?game=lobby";
	FString Grade = "?grade=" + AScoringManager::Get()->CalculateGradeLetterFromPlayerScore();

	FString URL = Level + GameMode + Grade;
	ProcessServerTravel(URL);
}

bool ACoopGM::ShouldAlertSuspectWhenLastAlive() const
{
	switch (GetCOOPMode())
	{
		case ECOOPMode::CM_ActiveShooter:
		return false;

		case ECOOPMode::CM_HostageRescue:
		return false;

		default:
		return true;
	}
}

void ACoopGM::InitWorld()
{
	if (bInitedWorld)
		return;
	
	if (UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		if (!NavSys->MainNavData)
		{
			NavSys->OnNavigationInitDone.AddUObject(this, &ACoopGM::OnNavigationInitDone);
		}
		else
		{
			TryGenerateWorld();
		}
	}

	RemoveAllBombsExceptDesignated();
	SpawnPersonnel();
	ResetDoors();
	SetupKeycards();

	for (TActorIterator<AThreatAwarenessActor>It(GetWorld()); It; ++It)
	{
		It->GenerateUniqueExits();
	}

	for (TActorIterator<AActor>It(GetWorld()); It; ++It)
	{
		switch (Mode)
		{
		case ECOOPMode::CM_None: break;
		case ECOOPMode::CM_BombThreat:
			if (It->Tags.Contains("DestroyInBombThreat"))
				It->Destroy();
			break;
		case ECOOPMode::CM_ActiveShooter:
			if (It->Tags.Contains("DestroyInActiveShooter"))
				It->Destroy();
			break;
		case ECOOPMode::CM_HostageRescue:
			if (It->Tags.Contains("DestroyInHostageRescue"))
				It->Destroy();
			break;
		case ECOOPMode::CM_BarricadedSuspects:
			if (It->Tags.Contains("DestroyInBarricadedSuspects"))
				It->Destroy();
			break;
		case ECOOPMode::CM_Raid:
			if (It->Tags.Contains("DestroyInRaid"))
				It->Destroy();
			break;
		default:;
		}
	}
	
	bInitedWorld = true;

}

void ACoopGM::InitAI()
{
	if (bInitedAI)
		return;

	SpawnSuspectsAndHostages();

	for (TActorIterator<AAIFactionManager> It(GetWorld()); It; ++It)
	{
		AAIFactionManager* FactionManager = *It;
		FactionManager->OnAllAISpawned();
	}

	bInitedAI = true;
	OnAllAISpawned.Broadcast();
}

void ACoopGM::EnableHiddenTigerCrouchingDragon()
{
	if (bHiddenTigerCrouchingDragon)
		return;

	bHiddenTigerCrouchingDragon = true;
	TArray<AActor*> OutSpawns;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AAISpawn::StaticClass(), OutSpawns);
	if (OutSpawns.Num() > 0)
	{
		for (TActorIterator<APlayerController>It(GetWorld()); It; ++It)
		{
			SpawnPlayerCharacter(*It, BlueCharacterClass.LoadSynchronous(), OutSpawns[FMath::RandRange(0, OutSpawns.Num() - 1)]->GetActorTransform());
		}
	}
}

AActor* ACoopGM::FindPlayerStart_Implementation(AController* Player, const FString& IncomingName /* = TEXT("") */)
{
	SCOPE_CYCLE_COUNTER(STAT_COOPFindPlayerStart);

	AReadyOrNotPlayerState* ps = Cast<AReadyOrNotPlayerState>(Player->PlayerState);
	if (!ps)
		return nullptr;

	// Find the desired entry point and spawn there if it exists
	FString EntryPoint = UGameplayStatics::ParseOption(OptionsString, "entrypoint");
	if (!EntryPoint.IsEmpty())
	{
		APlayerStart* PlayerStart = FindPlayerStartWithTag(FName(*EntryPoint));
		if (PlayerStart)
			return PlayerStart;

		UE_LOG(LogReadyOrNot, Warning, TEXT("Entrypoint %s not found, using default spawn behavior"), *EntryPoint);
	}
	
	if (GetMatchState() == EMatchState::MS_Warmup || (ps && !ps->bIsInGame && GetMatchState() == EMatchState::MS_Playing))
	{
		for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
		{
			APlayerStart* start = *It;
			if (start)
			{
				if (start->PlayerStartTag == RedCustomizationStartTag)
				{
					//GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, "Found Player Start: " + start->GetPathName());
					return start;
				}
			}
		}
	}
	else if (GetMatchState() == EMatchState::MS_Playing)
	{
		for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
		{
			APlayerStart* PlayerStart = *It;

			if (PlayerStart->IsA<APlayerStartPIE>())
			{
				// Always prefer the first "Play from Here" PlayerStart, if we find one while in PIE mode
				return PlayerStart;
			}

		}
		AActor* FoundStart = FindPlayerStartForTeam(ps->Team);
		if (FoundStart != nullptr)
		{
			return FoundStart;
		}
	}
	//GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, "Unable to find player start.");
	return Super::FindPlayerStart_Implementation(Player, IncomingName);
}

bool ACoopGM::IsROEDisabled() const
{
	switch (Mode)
	{
		case ECOOPMode::CM_None:				return false;
		case ECOOPMode::CM_BombThreat:			return false;
		case ECOOPMode::CM_ActiveShooter:		return true;
		case ECOOPMode::CM_HostageRescue:		return false;
		case ECOOPMode::CM_BarricadedSuspects:	return false;
		case ECOOPMode::CM_Raid:				return true;
		default:								return false;
	}
}

void ACoopGM::PreInitializeComponents()
{
	Super::PreInitializeComponents();
	
	if (!WorldDataGenerator && GetWorld())
	{
		WorldDataGenerator = AWorldDataGenerator::Get(GetWorld());
		if (!WorldDataGenerator)
			WorldDataGenerator = GetWorld()->SpawnActor<AWorldDataGenerator>(AWorldDataGenerator::StaticClass());
	}
}

void ACoopGM::OnMissionCompleted()
{
	// Save level progression
	FString MapName = GetWorld()->GetMapName();
	MapName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);
	MapName = MapName.ToLower();
	
	MapName.ReplaceInline(TEXT("_BarricadedSuspects"), TEXT(""));
	MapName.ReplaceInline(TEXT("_ActiveShooter"), TEXT(""));
	MapName.ReplaceInline(TEXT("_BombThreat"), TEXT(""));
	MapName.ReplaceInline(TEXT("_HostageRescue"), TEXT(""));
	MapName.ReplaceInline(TEXT("_Raid"), TEXT(""));
	
	UMetaGameProfile* MetaGameProfile = UMetaGameProfile::GetProfile(GetWorld());
	if (ensure(MetaGameProfile))
	{
		MetaGameProfile->AddCompletedLevel(MapName);

		TSet<FName> ProgressionTags;
		CheckProgression(ProgressionTags);

		MetaGameProfile->AddProgressionTags(ProgressionTags);

		MetaGameProfile->SaveProfile();
	}
}

void ACoopGM::CheckProgression(TSet<FName>& InProgressionTags)
{
	ACoopGS* CoopGS = GetGameState<ACoopGS>();
	if (CoopGS)
	{
		float ScorePercentage = AScoringManager::Get()->GetFinalGradePercentage();
		TSet<FName> LevelProgressionTags = CoopGS->GetLevelProgressionTags(ScorePercentage);
		
		InProgressionTags.Append(LevelProgressionTags);
		CoopGS->CheckAllLevelsCompleted(InProgressionTags);
		
		CoopGS->Multicast_GrantProgressionTags(ScorePercentage);
	}
}

void ACoopGM::RemoveAllBombsExceptDesignated()
{
	TArray<AActor*> OutActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABombActor::StaticClass(), OutActors);

	int32 MaxBombs = AI_CONFIG_GET_INT("MaxBombs");

	TArray<ABombActor*> SelectedBombActors;
	while (OutActors.Num() > 0 && SelectedBombActors.Num() < MaxBombs)
	{
		int32 RndIdx = FMath::RandRange(0, OutActors.Num() - 1);
		SelectedBombActors.AddUnique(Cast<ABombActor>(OutActors[RndIdx]));
		OutActors.RemoveAt(RndIdx);
	}

	for (TActorIterator<ABombActor>It(GetWorld()); It; ++It)
	{
		if (!SelectedBombActors.Contains(*It))
		{
			It->Destroy();
		}
	}
	V_LOGM(LogReadyOrNot, "Spawned %d Bombs!", SelectedBombActors.Num());
}

void ACoopGM::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
}

bool ACoopGM::CanTakeDamage(AController* EventInstigator, AController* DamageReceiver) const
{
	SCOPE_CYCLE_COUNTER(STAT_COOPCanTakeDamage);

	ACyberneticController* InstigatorCb = Cast<ACyberneticController>(EventInstigator);
	ACyberneticController* ReceiverCb = Cast<ACyberneticController>(DamageReceiver);

	if (!InstigatorCb || !ReceiverCb)
	{
		return Super::CanTakeDamage(EventInstigator, DamageReceiver);
	}

	// swat cannot damage SWAT
	if (InstigatorCb->IsSWAT() && ReceiverCb->IsSWAT())
	{
		return false;
	}

	return Super::CanTakeDamage(EventInstigator, DamageReceiver);
}

void ACoopGM::CheckWinConditions()
{
	SCOPE_CYCLE_COUNTER(STAT_COOPCheckWinConditions);

	ACoopGS* GS = GetGameState<ACoopGS>();
	if (!GS)
		return;

	if (bHiddenTigerCrouchingDragon)
		return;

	if (bMissionExfiltrated)
		return;

	if (GetMatchState() == EMatchState::MS_Playing)
	{
		if (AreAllPlayersDead())
		{
			StartMissionEndTimer(false);
			return;
		}

		bool bAllObjectivesCompleted = true;
		//AScoringManager::Get()->RemoveInvalidObjects();
		AScoringManager::Get()->UpdateObjectives();

		// Check to see if all of the objectives are complete
		for (AObjective* Objective : GS->MissionObjectives)
		{
			if (Objective)
			{
				if (Objective->IsObjectiveFailed() && Objective->bFailureEndsMission)
				{
					StartMissionEndTimer(false);
					return;
				}
				if (Objective->IsObjectiveInProgress() && Objective->ScoringComponent->ObjectiveLevel == EObjectiveLevel::PrimaryObjective)
				{
					bAllObjectivesCompleted = false;
					break;
				}
					
				/*switch (Objective->ObjectiveStatus)
				{
				case EObjectiveStatus::Objective_Failed:
					if (Objective->bFailureEndsMission)
					{
						StartMissionEndTimer(false);
						return;
					}
					break;

				case EObjectiveStatus::Objective_InProgress:
					bAllObjectivesCompleted = false;
					break;

				default:
					break;
				}*/
			}
		}

		AScoringManager::Get()->CacheHasClearedMission();
		bool bHasClearedMission, bHasSoftClearedMission, bMissionFailed;
		AScoringManager::Get()->HasClearedMission(bHasClearedMission, bHasSoftClearedMission, bMissionFailed);
		
		GS->bMissionSucceded = !bMissionFailed;
		GS->bMissionSoftCompleted = bAllObjectivesCompleted || bHasSoftClearedMission;	// If all objectives are completed then soft complete

		bool bEndingMission = bHasClearedMission || GS->MissionEndVoteState == EMissionEndVoteState::VS_MajorityYes;
		if (bEndingMission)
		{
			if (!bMissionCompleted)
			{
				bMissionCompleted = true;
				OnMissionCompleted();
			}
			
			StartMissionEndTimer(!bMissionFailed);
		}
		else
		{
			if (GS->bMissionSoftCompleted && GS->MissionEndVoteState == EMissionEndVoteState::VS_NotStarted)
			{
				Server_SoftClearVoteCheck();
				GS->MissionEndVoteState = EMissionEndVoteState::VS_InProgress;

				// Tell Exfil portals to switch to end mission
				// TODO: Add delegates for OnMissionSoftComplete and OnMissionFullComplete
				for (TActorIterator<AExfilPortal> It(GetWorld()); It; ++It)
				{
					if (AExfilPortal* ExfilPortal = Cast<AExfilPortal>(*It))
					{
						ExfilPortal->OnMissionSoftComplete();
					}
				}
			}
		}
	}
}

void ACoopGM::Server_SoftClearVoteCheck_Implementation()
{
	for (TActorIterator<APlayerController> It(GetWorld()); It; ++It)
	{
		if (AReadyOrNotPlayerController* Controller = Cast<AReadyOrNotPlayerController>(*It))
		{
			//if (!Controller->MyVoteData.VoteEnabled)
			//{
			Controller->BeginVote_Implementation("Mission Soft Complete", "End The Mission?", false);
			Controller->BeginVote("Mission Soft Complete", "End The Mission?", false);
			//}
		}
	}
}

void ACoopGM::StartMissionEndTimer(const bool bWon)
{
#if WITH_EDITOR
	if (CVarRonEndMissionInEditor.GetValueOnAnyThread() == 0)
		return;
#endif

	if (!UReadyOrNotFunctionLibrary::IsCallbackTimerActive(this, MissionEndTimer_Handle))
	{
		if (ATOCManager* TOC = ATOCManager::Get())
		{
			TOC->StartTOCResponse(bWon ? VO_TOC::TOC_MISSION_COMPLETION : VO_TOC::TOC_MISSION_FAILED, true, ETOCPriority::ETP_Flush);
		}

		if (ACoopGS* GS = GetGameState<ACoopGS>())
		{
			GS->Multicast_OnMissionEnd(bWon);
		}

		UReadyOrNotFunctionLibrary::StartTimerForCallback(MissionEndTimer_Handle, this, FTimerDelegate::CreateUObject(this, &ACoopGM::MissionEnd, bWon), 5.0f, false);
	}
}

#if WITH_EDITOR
void ACoopGM::DebugEndMission()
{
	StartMissionEndTimer(bDebugWonMission);
}
#endif

void ACoopGM::SpawnSuspectsAndHostages()
{
	SCOPE_CYCLE_COUNTER(STAT_COOPSpawnSuspectsAndHostages);

	const AReadyOrNotLevelScript* LevelScript = Cast<AReadyOrNotLevelScript>(GetWorld()->GetCurrentLevel()->GetLevelScriptActor());
	if (!LevelScript)
	{
		#if WITH_EDITOR
		ULog::Error("ACoopGM::SpawnSuspectsAndHostages: Custom level script not found");
		#endif

		// artist has not defined level data for this level, don't spawn anything
		return;
	}

	//V_LOGM(LogReadyOrNot, "World: %s |Level: %s| LevelScriptActor: %s", *GetWorld()->GetName(), *GetWorld()->GetCurrentLevel()->GetName(), *GetWorld()->GetCurrentLevel()->GetLevelScriptActor()->GetName());

	if (LevelScript->WorldGenerationType == EGenerationType::GT_RandomScenarios)
	{
		ARosterScenarioSpawner* ScenarioSpawner = nullptr;
		for (const TActorIterator<ARosterScenarioSpawner> It(GetWorld()); It;)
		{
			ScenarioSpawner = *It;
			break;
		}

		if (ScenarioSpawner)
		{
			#if !UE_BUILD_SHIPPING
			ULog::Info("Performing roster scenario spawn...");
			#endif

			ScenarioSpawner->DoRoster();
		}
		else
		{
			#if !UE_BUILD_SHIPPING
			ULog::Error("Failed to spawn AI. Could not find a roster scenario spawner in level (" + GetWorld()->GetCurrentLevel()->GetName() + ")");
			#endif
		}
	}
	else
	{
		#if !UE_BUILD_SHIPPING
		ULog::Info("Spawning AI with world gen type of None");
		#endif

		for (TActorIterator<AAISpawn> It(GetWorld()); It; ++It)
		{
			AAISpawn* Spawner = *It;

			if (Spawner->SpawnArray.Num() > 0)
			{
				Spawner->SpawnData = Spawner->SpawnArray[FMath::RandRange(0, Spawner->SpawnArray.Num()-1)];
			}
			
			if (Spawner->DoSpawn())
			{
				#if !UE_BUILD_SHIPPING
				ULog::Info(Spawner->GetName() + ": AI Spawned -> " + Spawner->SpawnedCharacter->GetName() + " at " + Spawner->SpawnedCharacter->GetActorLocation().ToCompactString());
				#endif
			}
		}
	}

	#if WITH_EDITOR
	if (CVarRonSpawnNoSuspects.GetValueOnAnyThread() > 0)
	{
		for (TActorIterator<ACyberneticCharacter> It(GetWorld()); It; ++It)
		{
			if (It->IsSuspect())
			{
				It->Destroy();
			}
		}
	}
	#endif
}

void ACoopGM::SpawnPolice()
{
	//SCOPE_CYCLE_COUNTER(STAT_COOPSpawnPolice);

	int32 TotalOfficers = 0;
	int32 TotalAIOfficers = 0;
	
	SpawnedSWATAI.Empty(4);
	
	// always spawn swat if this is true
	bool bShouldSpawnSwat = false;
	int32 PlayersCount = 0;
	for (TActorIterator<AReadyOrNotPlayerController> It(GetWorld()); It; ++It)
	{
		if (!(*It)->bIsReplaySpectator && !Cast<AReplayController>(*It))
		{
			PlayersCount++;
		}
	}

	// Human players replace AI
	if (PlayersCount <= 1 && GetWorld()->GetNetMode() != NM_DedicatedServer)
	{
		bShouldSpawnSwat = true;
	}
	else
	{
		TotalOfficers = PlayersCount;
	}

#if !WITH_EDITOR
	// If we're not in single player, don't spawn SWAT. Check NetDriver since demo net driver will tell us we are in multiplayer
	// Ignore in the editor since the code above should handle not spawning SWAT for us in editor MP testing
	if (GetWorld()->GetNetDriver())
	{
		bShouldSpawnSwat = false;
	}
#endif

	if (bShouldSpawnSwat || CVarRonSpawnSwatInIncompatibleMode.GetValueOnAnyThread() == 1)
	{
		SpawnAIOfficer(ESquadPosition::SP_Alpha, ETeamType::TT_SERT_BLUE, "defaultblueone", SwatAlphaClass);
		SpawnAIOfficer(ESquadPosition::SP_Beta, ETeamType::TT_SERT_BLUE, "defaultbluetwo", SwatBetaClass);
		SpawnAIOfficer(ESquadPosition::SP_Charlie, ETeamType::TT_SERT_RED, "defaultredone", SwatCharlieClass);
		SpawnAIOfficer(ESquadPosition::SP_Delta, ETeamType::TT_SERT_RED, "defaultredtwo", SwatDeltaClass);
		
		SpawnAITrailerOfficer(ESquadPosition::SP_Alpha, ETeamType::TT_SERT_BLUE, "trailer");
		SpawnAITrailerOfficer(ESquadPosition::SP_Beta, ETeamType::TT_SERT_BLUE, "trailer");
		SpawnAITrailerOfficer(ESquadPosition::SP_Charlie, ETeamType::TT_SERT_RED, "trailer");
		SpawnAITrailerOfficer(ESquadPosition::SP_Delta, ETeamType::TT_SERT_RED, "trailer");
		
		TotalAIOfficers = SpawnedSWATAI.Num();
		TotalOfficers = TotalAIOfficers+1;

		if (LOCAL_PLAYER)
		{
			// Do two set locations, one that puts them just behind the player and another that moves them to the correct place with a sweep.. so they can't go into walls

			FVector RedOneSpawn = LocalPlayer->GetActorLocation() + LocalPlayer->GetActorForwardVector() * -100.0f;
			FVector RedTwoSpawn = LocalPlayer->GetActorLocation() + LocalPlayer->GetActorForwardVector() * -100.0f;
			FVector BlueOneSpawn = LocalPlayer->GetActorLocation() + LocalPlayer->GetActorForwardVector() * -100.0f;
			FVector BlueTwoSpawn = LocalPlayer->GetActorLocation() + LocalPlayer->GetActorForwardVector() * -100.0f;

			SpawnedSWATAI[0]->SetActorLocation(BlueOneSpawn);
			SpawnedSWATAI[1]->SetActorLocation(BlueTwoSpawn);
			SpawnedSWATAI[2]->SetActorLocation(RedOneSpawn);
			SpawnedSWATAI[3]->SetActorLocation(RedTwoSpawn);

			RedOneSpawn = LocalPlayer->GetActorLocation() + (LocalPlayer->GetActorForwardVector() * -100.0f + LocalPlayer->GetActorRightVector() * -100.0f);
			RedTwoSpawn = LocalPlayer->GetActorLocation() + (LocalPlayer->GetActorForwardVector() * -200.0f + LocalPlayer->GetActorRightVector() * -50.0f);
			BlueOneSpawn = LocalPlayer->GetActorLocation() + (LocalPlayer->GetActorForwardVector() * -200.0f + LocalPlayer->GetActorRightVector() * 100.0f);
			BlueTwoSpawn = LocalPlayer->GetActorLocation() + (LocalPlayer->GetActorForwardVector() * -200.0f + LocalPlayer->GetActorRightVector() * 50.0f);

			SpawnedSWATAI[0]->SetActorLocation(BlueOneSpawn, true);
			SpawnedSWATAI[1]->SetActorLocation(BlueTwoSpawn, true);
			SpawnedSWATAI[2]->SetActorLocation(RedOneSpawn, true);
			SpawnedSWATAI[3]->SetActorLocation(RedTwoSpawn, true);
		}

		for (ATrailerSWATCharacter* Trailer : SpawnedTrailerSWATAI)
		{
			Trailer->SetActorLocation(FVector(0.0f, 0.0f, -10000.0f));
		}
	}

	ACoopGS* gs = GetWorld()->GetGameState<ACoopGS>();
	
	gs->TotalOfficers = TotalOfficers;
	gs->TotalAIOfficers = TotalAIOfficers;
	
	for (ASWATCharacter* swat : SpawnedSWATAI)
	{
		swat->OnCharacterKilled.AddDynamic(this, &ACoopGM::FriendlyAIKilled);
	}

	if (USWATManager* SWATManager = USWATManager::Get(this))
	{
		SWATManager->SwatAI = SpawnedSWATAI;
		SWATManager->SwatTrailers = SpawnedTrailerSWATAI;
		
		if (LOCAL_PLAYER)
		{
			SWATManager->SquadLeader = LocalPlayer;
			SWATManager->OriginalSpawnLocation = LocalPlayer->GetNavAgentLocation();
		}
		else
		{
			SWATManager->SquadLeader = SpawnedSWATAI[0];
			SWATManager->OriginalSpawnLocation = SpawnedSWATAI[0]->GetNavAgentLocation();
		}
		
		for (ASWATCharacter* Swat : SpawnedSWATAI)
		{
			Swat->GetCyberneticsController()->GetTargetingComp()->AddKnownFriendly(SWATManager->SquadLeader);
			
			for (ASWATCharacter* OtherSwat : SpawnedSWATAI)
			{
				if (Swat != OtherSwat)
				{
					Swat->GetCyberneticsController()->GetTargetingComp()->AddKnownFriendly(OtherSwat);
				}
			}
		}
		
		#if WITH_EDITOR && !WITH_AUTOMATION_TESTS
		ensureAlways(SWATManager->SquadLeader != nullptr);
		#endif
	}
}

void ACoopGM::SpawnAIOfficer(ESquadPosition SquadPosition, ETeamType CommandTeam, FString LoadoutName, TSubclassOf<ACyberneticCharacter> Class)
{
	SCOPE_CYCLE_COUNTER(STAT_COOPSpawnAIOfficer);

	// We need to determine whether this squad member and the player are at the same spawn point. This has a number of implications.
	bool bSpawnsWithPlayer = false;
	APlayerCharacter* pc = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
	ACoopGS* gs = GetWorld()->GetGameState<ACoopGS>();
	if (!gs)
	{
		return;
	}

	if (pc)
	{
		if (pc->GetTeam() == CommandTeam)
		{	// We have the same team as the player, so we spawn with them.
			bSpawnsWithPlayer = true;
		}
		else if (gs->SelectedBlueSpawnPoint == gs->SelectedRedSpawnPoint)
		{	// Both teams use the same spawn point, so we MUST spawn with the player.
			bSpawnsWithPlayer = true;
		}
	}

	FTransform SpawnTransform;
	if (bSpawnsWithPlayer)
	{
		// Just spawn them next to the player
		SpawnTransform = pc->GetActorTransform();
	}
	else
	{
		// Find the spawn point that matches their team
		APlayerStart* FoundStart = FindPlayerStartForTeam(CommandTeam);
		if (FoundStart == nullptr)
		{
			for (TActorIterator<APlayerStart> It(GetWorld()); It;)
			{
				FoundStart = *It;
				break;
			}
			
			if (!FoundStart)
			{
				V_LOGM(LogReadyOrNot, "Could not find spawn point\n");

				SpawnTransform = FTransform(FVector(0.0f, 0.0f, 200.0f));
			}
			else
			{
				SpawnTransform = FoundStart->GetActorTransform();
			}
		}
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.bNoFail = true;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	if (ASWATCharacter* ai = GetWorld()->SpawnActor<ASWATCharacter>(Class, SpawnTransform, SpawnParameters))
	{
		ai->AIControllerClass = FriendlyAIController;
		ai->SetSquadPosition(SquadPosition);
		ai->SetDefaultTeam(CommandTeam);
		//ai->GetCapsuleComponent()->SetCanEverAffectNavigation(false);
		
		switch (SquadPosition)
		{
			case ESquadPosition::SP_Alpha:		ai->GetCapsuleComponent()->AreaClass = UNavArea_SwatAlpha::StaticClass(); break;
			case ESquadPosition::SP_Beta:		ai->GetCapsuleComponent()->AreaClass = UNavArea_SwatBeta::StaticClass();break;
			case ESquadPosition::SP_Charlie:	ai->GetCapsuleComponent()->AreaClass = UNavArea_SwatCharlie::StaticClass();break;
			case ESquadPosition::SP_Delta:		ai->GetCapsuleComponent()->AreaClass = UNavArea_SwatDelta::StaticClass(); break;
			default:							ai->GetCapsuleComponent()->AreaClass = UNavArea_SwatAlpha::StaticClass();
		}
		
		ai->GetCapsuleComponent()->bDynamicObstacle = false;
		ai->GetCharacterMovement()->GetNavAgentPropertiesRef().AgentRadius = 20.0f;
		ai->GetCapsuleComponent()->SetCanEverAffectNavigation(false);

		if (bAIEquipSameLoadoutAsPlayer)
		{
			FSavedLoadout DefaultLoadout;
			UBpGameplayHelperLib::LoadLoadoutAndEquipPlayer(DefaultLoadout, ai, "default");
		}
		else
		{
			FSavedLoadout Loadout;
			UBpGameplayHelperLib::LoadLoadoutAndEquipPlayer(Loadout, ai, LoadoutName);
		}

		UBpGameplayHelperLib::AddDefaultItemsToPlayer(ai);

		FSavedCustomization Customization = FSavedCustomization();
		SetupOfficerCustomization(ai, Customization);
		Customization.Sanitize();

		ai->Customization = Customization;
		Customization.ApplyCustomization(ai);
		Customization.ApplyCustomizationSkins(ai);

		float SwatHealth = AI_CONFIG_GET_FLOAT("SwatHealth", 250.0f);
		SwatHealth += SwatHealth * URosterManager::GetSquadTraitValue("Nutritionist", GetWorld());
		ai->GetHealthComponent()->SetMaxResource(SwatHealth);
		ai->GetHealthComponent()->SetCurrentResourceToMax();

		ai->SpawnDefaultController();

		SpawnedSWATAI.Add(ai);
	}
}

void ACoopGM::SpawnAITrailerOfficer(ESquadPosition SquadPosition, ETeamType CommandTeam, FString LoadoutName)
{
	SCOPE_CYCLE_COUNTER(STAT_COOPSpawnAITrailerOfficer);

	// We need to determine whether this squad member and the player are at the same spawn point. This has a number of implications.
	bool bSpawnsWithPlayer = false;
	APlayerCharacter* pc = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
	ACoopGS* gs = GetWorld()->GetGameState<ACoopGS>();
	if (!gs)
	{
		return;
	}

	if (pc)
	{
		if (pc->GetTeam() == CommandTeam)
		{	// We have the same team as the player, so we spawn with them.
			bSpawnsWithPlayer = true;
		}
		else if (gs->SelectedBlueSpawnPoint == gs->SelectedRedSpawnPoint)
		{	// Both teams use the same spawn point, so we MUST spawn with the player.
			bSpawnsWithPlayer = true;
		}
	}

	FTransform SpawnTransform;
	if (bSpawnsWithPlayer)
	{
		// Just spawn them next to the player
		SpawnTransform = pc->GetActorTransform();
	}
	else
	{
		// Find the spawn point that matches their team
		APlayerStart* FoundStart = FindPlayerStartForTeam(CommandTeam);
		if (FoundStart == nullptr)
		{
			for (TActorIterator<APlayerStartPIE>It(GetWorld()); It; ++It)
			{
				FoundStart = *It;
			}
			
			if (!FoundStart)
			{
				V_LOGM(LogReadyOrNot, "Could not find spawn point\n");

				SpawnTransform = FTransform(FVector(0.0f, 0.0f, 200.0f));
			}
			else
			{
				SpawnTransform = FoundStart->GetActorTransform();
			}
		}
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.bNoFail = true;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	if (ATrailerSWATCharacter* ai = GetWorld()->SpawnActor<ATrailerSWATCharacter>(SwatTrailerClass, SpawnTransform, SpawnParameters))
	{
		ai->AIControllerClass = FriendlyAIController;
		ai->SetSquadPosition(SquadPosition);
		ai->SetDefaultTeam(CommandTeam);
		
		switch (SquadPosition)
		{
			case ESquadPosition::SP_Alpha:		ai->GetCapsuleComponent()->AreaClass = UNavArea_SwatAlpha::StaticClass(); break;
			case ESquadPosition::SP_Beta:		ai->GetCapsuleComponent()->AreaClass = UNavArea_SwatBeta::StaticClass();break;
			case ESquadPosition::SP_Charlie:	ai->GetCapsuleComponent()->AreaClass = UNavArea_SwatCharlie::StaticClass();break;
			case ESquadPosition::SP_Delta:		ai->GetCapsuleComponent()->AreaClass = UNavArea_SwatDelta::StaticClass(); break;
			default:							ai->GetCapsuleComponent()->AreaClass = UNavArea_SwatAlpha::StaticClass();
		}
		
		ai->GetCapsuleComponent()->bDynamicObstacle = false;
		ai->GetCharacterMovement()->GetNavAgentPropertiesRef().AgentRadius = 20.0f;
		ai->GetCapsuleComponent()->SetCanEverAffectNavigation(false);

		FSavedLoadout DefaultLoadout;
		UBpGameplayHelperLib::LoadDefaultLoadout(DefaultLoadout, "trailer");
		UBpGameplayHelperLib::EquipLoadoutOnPlayer(DefaultLoadout, ai, FLoadoutEquipOptions());

		UBpGameplayHelperLib::AddDefaultItemsToPlayer(ai);
		
		UItemData* ItemData = UBpGameplayHelperLib::GetItemData(GetWorld());
		if (ItemData)
		{
			FSavedCustomization Customization = ItemData->TrailerCustomization;

			int32 NumTrailerCharacters = ItemData->TrailerCharacters.Num();
			if (NumTrailerCharacters > 0)
			{
				Customization.Character = ItemData->TrailerCharacters[FMath::RandRange(0, NumTrailerCharacters - 1)];
			}
			
			Customization.Sanitize();

			ai->Customization = Customization;
			Customization.ApplyCustomization(ai);
			Customization.ApplyCustomizationSkins(ai);
		}
		
		ai->GetHealthComponent()->SetMaxResource(AI_CONFIG_GET_FLOAT("SwatHealth", 250.0f));
		ai->GetHealthComponent()->SetCurrentResourceToMax();
		ai->GetHealthComponent()->SetUnlimitedResource(true);

		ai->SpawnDefaultController();
		ai->GetController<ACyberneticController>()->GetAIPerceptionComponent()->OnPerceptionUpdated.Clear();
		ai->GetController<ACyberneticController>()->bDisableSensePerception = true;

		UReadyOrNotSignificanceManager::RegisterActorWithSignificanceManager(ai);
		UReadyOrNotSignificanceManager::ForceActorNotRelevant(ai);
		ai->GetMesh()->bNoSkeletonUpdate = true;
		ai->RemoveVocalChords();
		ai->GetCharacterMovement()->DisableMovement();
		ai->GetCapsuleComponent()->SetEnableGravity(false);
		ai->bDeactivated = true;
		ai->SetActorTickEnabled(false);

		SpawnedTrailerSWATAI.Add(ai);
	}
}

void ACoopGM::SetupOfficerCustomization(ASWATCharacter* Character, FSavedCustomization& OutCustomization)
{
	Character->CharacterLookOverride = FCharacterLookOverride();
	
	EEquippingSwat EquippingSwat = EEquippingSwat::ES_None;
	switch (Character->GetSquadPosition())
	{
	case ESquadPosition::SP_Alpha: EquippingSwat = EEquippingSwat::ES_BlueOne; break;
	case ESquadPosition::SP_Beta: EquippingSwat = EEquippingSwat::ES_BlueTwo; break;
	case ESquadPosition::SP_Charlie: EquippingSwat = EEquippingSwat::ES_RedOne; break;
	case ESquadPosition::SP_Delta: EquippingSwat = EEquippingSwat::ES_RedTwo; break;
	default: EquippingSwat = EEquippingSwat::ES_None;
	}
	
	UBaseProfile* BaseProfile = UBaseProfile::GetCurrentProfile();
	if (BaseProfile)
	{
		FSavedCustomization* SavedCustomization = BaseProfile->Customizations.Find(EquippingSwat);
		if (SavedCustomization)
			OutCustomization = *SavedCustomization;
	}
	
	UItemData* ItemData = UBpGameplayHelperLib::GetItemData();
	if (ItemData)
	{
		FDefaultCharacterCustomization* CharacterCustomization = ItemData->DefaultCharacters.Find(EquippingSwat);
		if (CharacterCustomization)
		{
			OutCustomization.Character = CharacterCustomization->Character;
			OutCustomization.Voice = CharacterCustomization->Voice;

			// Set a different default armor skin if we aren't using one
			//if (!OutCustomization.ArmorSkin)
				//OutCustomization.ArmorSkin = CharacterCustomization->ArmorSkin;
		}
	}
	
	// TMap<ESquadPosition, FString> Names;
	// Names.Add(ESquadPosition::SP_Alpha, "SWAT_King");
	// Names.Add(ESquadPosition::SP_Beta, "SWAT_Swan");
	// Names.Add(ESquadPosition::SP_Charlie, "SWAT_Prescott");
	// Names.Add(ESquadPosition::SP_Delta, "SWAT_Eli");
	// //Names.Add(ESquadPosition::SP_Foxtrot, "SWAT_King");
	// //Names.Add(ESquadPosition::SP_Golf, "SWAT_Swan");
	// //Names.Add(ESquadPosition::SP_Hotel, "SWAT_Prescott");
	// //Names.Add(ESquadPosition::SP_India, "SWAT_Eli");
	// Names.Add(ESquadPosition::SP_NONE, "");
	
	//Character->UpdateOverridesFromCharacterLookOverrideDataTable(Names[Character->GetSquadPosition()]);
}

void ACoopGM::ResetDoors()
{
	TArray<AActor*> OutActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ADoor::StaticClass(), OutActors);

	Algo::RandomShuffle(OutActors);
	
	/**
	*	Trap doors
	*/
	TArray<ADoor*> TrapDoors;
	
	const TArray<FString> TrapTypes = AI_CONFIG_GET_STRING_ARRAY_SINGLE_LINE("TrapType");
	if (TrapTypes.Num() > 0)
	{
		TArray<ADoor*> PossibleTrapDoors;
		for (AActor* Actor : OutActors)
		{
			ADoor* Door = Cast<ADoor>(Actor);

			// Only place traps on eligible doors
			if (Door && !Door->GetSubDoor() && !Door->IsDoorwayOnly() && Door->CanSpawnTrap())
			{
				Door->SetTypeOfTrapRowName(NAME_None);
				PossibleTrapDoors.Add(Door);
			}
		}

		// TODO(killo): we currently do not consider which spawns were actually used when spawning traps near AI
		// this is because we set up doors before we ever spawn AI, if we spawn AI and then traps they could be walking into a trap as they spawn
		// this also doesn't consider that spawning traps closer to AI spawns exclusively might make a lot more trapped AI

		const bool bPrePlaceTrapsNearAISpawns = AI_CONFIG_GET_BOOL("PrePlaceTrapsNearAISpawns");
		const float PrePlaceTrapsNearAISpawnChance = AI_CONFIG_GET_FLOAT("PrePlaceTrapsNearAISpawnChance");

		// Either use doors closest to AI spawns, or place completely randomly
		TMap<ADoor*, double> TrapDoorDistanceMap;
		if (bPrePlaceTrapsNearAISpawns)
		{
			TArray<AActor*> OutSpawns;
			UGameplayStatics::GetAllActorsOfClass(GetWorld(), AAISpawn::StaticClass(), OutSpawns);

			// Use the combined distance to all AI spawns from this door as our value
			for (ADoor* Door : PossibleTrapDoors)
			{
				double CombinedDistance = 0.0;
				for (AActor* Spawn : OutSpawns)
				{
					double Distance = FVector::DistSquared(Spawn->GetActorLocation(), Door->GetActorLocation());
					CombinedDistance += Distance;
				}

				TrapDoorDistanceMap.Add(Door, CombinedDistance);
			}
		}
		else
		{
			// Add all trap doors with a random value, effectively randomizing them during sort
			for (ADoor* Door : PossibleTrapDoors)
			{
				TrapDoorDistanceMap.Add(Door, FMath::FRand());
			}
		}

		// Sort our trap doors, ascending by our value set above
		TrapDoorDistanceMap.ValueSort([](double A, double B)
		{
			return A < B;
		});

		// Max pre-placed traps is limited by MaxTrapsPrePlaced but also by MaxTraps
		int32 MinTraps = FMath::Min(AI_CONFIG_GET_FLOAT("MinTrapsPrePlaced"), AI_CONFIG_GET_FLOAT("MaxTraps"));
		int32 MaxTraps = FMath::Min(AI_CONFIG_GET_FLOAT("MaxTrapsPrePlaced"), AI_CONFIG_GET_FLOAT("MaxTraps"));

		MaxTraps = FMath::Max(MinTraps, MaxTraps);
		
		// We're limited by the number of possible trap doors we have as well...
		MinTraps = FMath::Min(TrapDoorDistanceMap.Num(), MinTraps);
		MaxTraps = FMath::Min(TrapDoorDistanceMap.Num(), MaxTraps);

		const int32 DesiredTraps = FMath::RandRange(MinTraps, MaxTraps);
		
		// Setup traps until we hit the trap limit
		for (auto& TrapDoorDistancePair : TrapDoorDistanceMap)
		{
			if (TrapDoors.Num() >= DesiredTraps)
				break;

			// Chance to skip traps near AI spawns for randomness
			if (bPrePlaceTrapsNearAISpawns && FMath::FRand() > PrePlaceTrapsNearAISpawnChance)
				continue;

			ADoor* Door = TrapDoorDistancePair.Key;
			Door->SetTypeOfTrapRowName(*TrapTypes[FMath::RandRange(0, TrapTypes.Num() - 1)]);
			
			TrapDoors.Add(Door);
		}

		// Setup all trap doors, including doors with NONE trap
		for (ADoor* Door : PossibleTrapDoors)
		{
			Door->SetupTrap();
		}
	}

	/**
	*	Open doors
	*/
	TArray<ADoor*> OpenDoors;
	for (AActor* Actor : OutActors)
	{
		ADoor* Door = Cast<ADoor>(Actor);
		if (Door && Door->bRandomlyOpenAtGameStart && !Door->GetAttachedTrap() && !Door->IsLocked() && !Door->IsDoorwayOnly())
		{
			OpenDoors.Add(Door);
		}
	}

	const int32 MaxOpenDoors = AI_CONFIG_GET_FLOAT("MaxOpenDoorsPercentage") * OpenDoors.Num();

	if (OpenDoors.Num() > MaxOpenDoors)
	{
		TArray<ADoor*> UsedDoors;
		while (UsedDoors.Num() < MaxOpenDoors)
		{
			ADoor* OpenDoor = OpenDoors[FMath::RandRange(0, OpenDoors.Num() - 1)];
			if (!UsedDoors.Contains(OpenDoor))
			{
				OpenDoor->OpenDoorFullyInstantly(FMath::RandBool());
				UsedDoors.Add(OpenDoor);
			}
		}
	}
	else
	{
		for (ADoor* Door : OpenDoors)
		{
			Door->OpenDoorFullyInstantly(FMath::RandBool());
		}
	}
	
	/**
	*	Locked doors
	*/
	TArray<ADoor*> LockDoors;

	for (AActor* Actor : OutActors)
	{
		ADoor* Door = Cast<ADoor>(Actor);

		// Electronic door chance
		if (Door->ElectronicLockChance > 0.0f)
		{
			if (Door->ElectronicLockChance >= FMath::FRand())
			{
				Door->SetElectronicallyLocked(true);
			}

			// No possibly electronic doors should be locked
			continue;
		}

		// No trapped doors should ever be locked
		if (TrapDoors.Contains(Door))
			continue;
		
		if (Door && !Door->IsDoorwayOnly() && !Door->IsElectronicDoor())
		{
			Door->SetTypeOfTrapRowName(NAME_None);
			Door->Setup();
			if (!Door->IsLockChanceOverridden())
				Door->UnlockDoor();
			LockDoors.Add(Door);
		}
	}

	const int32 MaxLockedDoors = AI_CONFIG_GET_FLOAT("MaxLockedDoorsPercentage") * LockDoors.Num();

	if (LockDoors.Num() > MaxLockedDoors)
	{
		TArray<ADoor*> UsedDoors;
		while (UsedDoors.Num() < MaxLockedDoors)
		{
			ADoor* LockedDoor = LockDoors[FMath::RandRange(0, LockDoors.Num() - 1)];
			if (!UsedDoors.Contains(LockedDoor))
			{
				LockedDoor->SetLocked(true);
				UsedDoors.Add(LockedDoor);
			}
		}
	}
	else
	{
		for (ADoor* Door : LockDoors)
		{
			Door->SetLocked(true);
		}
	}

	for (TActorIterator<ADoor> It(GetWorld()); It; ++It)
	{
		ADoor* Door = *It;
		
		if (Door->GetStartingOpenAngle() != 0.0f)
			Door->OpenDoor_SpecificAngle(nullptr, Door->GetStartingOpenAngle(), true, false);

		if (Door->GetAttachedTrap())
			Door->CloseDoor(nullptr, true, false);

		if (Door->IsAlwaysLocked())
		{
			Door->SetLocked(true);
		}
		else
		{
			if (Door->IsOverridingLockChance())
				Door->SetLocked(FMath::FRand() <= Door->GetOverrideLockChance());
		}
	}
}

void ACoopGM::SetupKeycards()
{
	if (!KeycardClass)
		return;

	TArray<AActor*> OutActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), KeycardClass, OutActors);

	const int32 MaxKeycards = FMath::Min(AI_CONFIG_GET_INT("MaxKeycards"), OutActors.Num());
	while (OutActors.Num() > MaxKeycards)
	{
		int32 KeycardIndex = FMath::RandRange(0, OutActors.Num() - 1);
		AActor* Keycard = OutActors[KeycardIndex];

		Keycard->Destroy(true);
		OutActors.RemoveAt(KeycardIndex);
	}
}

void ACoopGM::RemoveAllSpawnedAI()
{
	SCOPE_CYCLE_COUNTER(STAT_COOPRemoveAllSpawnedAI);

	for (TActorIterator<ACyberneticCharacter> It(GetWorld()); It; ++It)
	{
		ACyberneticCharacter* AICharacter = *It;
		if (AICharacter)
		{
			AICharacter->Destroy();
		}
	}
}

void ACoopGM::SpawnPersonnel()
{
	SCOPE_CYCLE_COUNTER(STAT_COOPSpawnPersonnel);

	FLevelDataLookupTable LevelData = UBpGameplayHelperLib::GetLevelData(GetWorld());
	ACoopGS* GS = GetWorld()->GetGameState<ACoopGS>();

	if (!GS)
	{
		return;
	}

	for (int32 i = 0; i < LevelData.AllPersonnel.Num(); i++)
	{
		if (GS->IsPersonnelEnabled(i))
		{
			SpawnSinglePersonnel(i);
		}
	}
}

void ACoopGM::SpawnSinglePersonnel(int32 PersonnelNum)
{
	SCOPE_CYCLE_COUNTER(STAT_COOPSpawnSinglePersonnel);

	FLevelDataLookupTable LevelData = UBpGameplayHelperLib::GetLevelData(GetWorld());
	ACoopGS* GS = GetWorld()->GetGameState<ACoopGS>();
	const FPersonnelEntry& PersonnelData = LevelData.AllPersonnel[PersonnelNum];

	switch (PersonnelData.PersonnelType)
	{
	case EPersonnel::PERS_Negotiator:
		Personnel_SpawnNegotiator();
		break;
	case EPersonnel::PERS_FloodlightOperator:
	case EPersonnel::PERS_NoisemakerOperator:
		Personnel_SpawnOperator(PersonnelNum, GS->PersonnelMapping[PersonnelNum], PersonnelData.PersonnelType == EPersonnel::PERS_NoisemakerOperator);
		break;
	case EPersonnel::PERS_PowerCrew:
		Personnel_SpawnPowerCrew();
		break;
	case EPersonnel::PERS_Marksman:
	case EPersonnel::PERS_Sniper:
	case EPersonnel::PERS_Spotter:
		Personnel_SpawnHighground(PersonnelNum, GS->PersonnelMapping[PersonnelNum],
			PersonnelData.PersonnelType == EPersonnel::PERS_Spotter,
			PersonnelData.PersonnelType == EPersonnel::PERS_Marksman,
			PersonnelData.PersonnelType == EPersonnel::PERS_Sniper);
		break;
	case EPersonnel::PERS_TruckDriver:
		Personnel_SpawnTruckDriver(PersonnelNum, GS->PersonnelMapping[PersonnelNum]);
		break;
	case EPersonnel::PERS_VentilationExpert:
		Personnel_SpawnVentilation(PersonnelNum, GS->PersonnelMapping[PersonnelNum]);
		break;
	default:;
	}
}

void ACoopGM::Personnel_SpawnHighground(int32 PersonnelNum, int32 MapPointNum, bool bSpotter, bool bMarksman, bool bSniper)
{
	SCOPE_CYCLE_COUNTER(STAT_COOPSpawnPersonnel_Highground);

	TArray<AActor*> HighgroundVolumes;
	TArray<AActor*> HighgroundSpawns; // for ryan <3 -- but i wound up coding this instead of ryan, so clearly love works in mysterious ways

	//GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, "Spawned High Ground.");

	FLevelDataLookupTable LevelData = UBpGameplayHelperLib::GetLevelData(GetWorld());
	FName SearchLabel = LevelData.AllPersonnelMapPoints[MapPointNum].VolumeLabel;

	// Find all High Ground Volumes, enable them, and assign them the next available designation.

	for (TActorIterator<AHighgroundVolume> It(GetWorld()); It; ++It)
	{
		AHighgroundVolume* Volume = *It;
		if (!Volume)
		{
			continue;
		}

		if (Volume->VolumeLabel == SearchLabel)
		{
			Volume->EnableHighgroundVolume(NextHighgroundDesignation);
		}
	}

	SearchLabel = LevelData.AllPersonnelMapPoints[MapPointNum].ActorLabel;

	// Find all Sniper Spawns, spawn what we're looking for, and assign them the next available designation.
	for (TActorIterator<ASniperSpawn> It(GetWorld()); It; ++It)
	{
		ASniperSpawn* Spawn = *It;
		if (!Spawn)
		{
			continue;
		}

		if (Spawn->SpawnLabel == SearchLabel)
		{
			if (bSpotter)
			{
				Spawn->SpawnSpotterHere(NextHighgroundDesignation);
			}
			else if (bMarksman)
			{
				Spawn->SpawnMarksmanHere(NextHighgroundDesignation);
			}
			else if (bSniper)
			{
				Spawn->SpawnSniperHere(NextHighgroundDesignation);
			}
		}
	}

	// Make sure that the next highground spawns with the proper designation
	NextHighgroundDesignation++;
}

void ACoopGM::Personnel_SpawnOperator(int32 PersonnelNum, int32 MapPointNum, bool bNoisemaker)
{
	//GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, "Spawned ground operator.");
}

void ACoopGM::Personnel_SpawnVentilation(int32 PersonnelNum, int32 MapPointNum)
{
	//GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, "Spawned ventilation expert.");
}

void ACoopGM::Personnel_SpawnTruckDriver(int32 PersonnelNum, int32 MapPointNum)
{
	//GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, "Spawned truck driver.");
}

void ACoopGM::Personnel_SpawnNegotiator()
{
	//GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, "Spawned negotiator.");
	bNegotiatorActive = true;
}

void ACoopGM::Personnel_SpawnPowerCrew()
{
	//GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, "Spawned power crew.");
}

void ACoopGM::DeactivateAllAI()
{
	for (ACyberneticCharacter* AI : GetGameState<ACoopGS>()->AllAICharacters)
	{
		if (AI)
		{
			AI->bDeactivated = true;
		}
	}
}

void ACoopGM::MissionEnd(const bool bSuccess)
{
	SCOPE_CYCLE_COUNTER(STAT_COOPMissionEnd);

	if (GetMatchState() == EMatchState::MS_Playing)
	{
		SetMatchState(EMatchState::MS_MatchEnded);

		if (ACoopGS* GS = GetGameState<ACoopGS>())
		{
			GS->Multicast_OnMissionEnd(bSuccess);
		}

		OnMissionEnded.Broadcast(bSuccess);

		DeactivateAllAI();

		for (TActorIterator<AReadyOrNotPlayerController> It(GetWorld()); It; ++It)
		{
			if (AReadyOrNotPlayerController* PC = *It)
			{
				PC->FlushPressedKeys();
				PC->SetInputMode(FInputModeUIOnly());
				PC->bShowMouseCursor = true;
				
				CreateMatchEndWidgets(PC);

				if (APlayerCharacter* Pawn = Cast<APlayerCharacter>(PC->GetPawn()))
				{
					Pawn->DisableInput(PC);
					Pawn->ConsumeMovementInputVector();
					Pawn->bAiming = false;
					Pawn->LockAllActions();
				}
			}
		}
		
		if (UKismetSystemLibrary::IsDedicatedServer(GetWorld()))
		{
			bShouldUseCountdown = true;
		}
	}
}

void ACoopGM::CreateMatchEndWidgets(class AReadyOrNotPlayerController* PlayerController)
{
	PlayerController->Client_ClearHUDWidgets();
	PlayerController->Client_CreateWidget("MatchEnd_COOP");
}

void ACoopGM::PlayerKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter)
{
	SCOPE_CYCLE_COUNTER(STAT_COOPPlayerKilled);

	// hidden pvp mode
	if (bHiddenTigerCrouchingDragon)
	{
		if (KilledCharacter)
		{
			KilledCharacter->SetLifeSpan(10.0f);
			AReadyOrNotPlayerController* pc = Cast<AReadyOrNotPlayerController>(KilledCharacter->GetController());
			if (pc)
			{
				pc->UnPossess();
				TArray<AActor*> OutSpawns;
				UGameplayStatics::GetAllActorsOfClass(GetWorld(), AAISpawn::StaticClass(), OutSpawns);
				APlayerCharacter* SpawnedCharacter = SpawnPlayerCharacter(pc, BlueCharacterClass.LoadSynchronous(), OutSpawns[FMath::RandRange(0, OutSpawns.Num() - 1)]->GetActorTransform());
				if (SpawnedCharacter && InstigatorCharacter)
				{
					AReadyOrNotGameState* gs = Cast<AReadyOrNotGameState>(GetWorld()->GetGameState());
					ABaseItem* Item = InstigatorCharacter->GetEquippedItem();
					if (InstigatorCharacter->GetController() && Item)
					{
						gs->Multicast_BroadcastChatMessage(FRChatMessage("SYSTEM", InstigatorCharacter->GetController()->PlayerState->GetPlayerName() + " has killed " + pc->PlayerState->GetPlayerName() + " with " + Item->ItemName.ToString(), FLinearColor::Red, nullptr));
					}
					SpawnedCharacter->Client_SetControlRotation(UKismetMathLibrary::FindLookAtRotation(SpawnedCharacter->GetActorLocation(), InstigatorCharacter->GetActorLocation()));
				}
			}
		}
		return;
	}

	AReadyOrNotPlayerState* ps = Cast<AReadyOrNotPlayerState>(KilledCharacter->GetPlayerState());
	if (ps)
	{
		if (ps->bSquadLeader)
		{
			ps->bSquadLeader = false;
			FindNewSquadLeader();
		}
	}

	Super::PlayerKilled(InstigatorCharacter, KilledCharacter);
}

void ACoopGM::AIKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter)
{
	SCOPE_CYCLE_COUNTER(STAT_COOPEnemyAIKilled);
	
	TArray<ASWATCharacter*> SwatTeam = USWATManager::Get(this)->SwatAI;
	SwatTeam.Remove(Cast<ASWATCharacter>(KilledCharacter));
	
	if (SwatTeam.Num() > 0)
	{
		if (ASWATCharacter* Swat = SwatTeam[FMath::RandRange(0, SwatTeam.Num()-1)])
		{
			if (Swat->GetCyberneticsController()->GetTargetingComp()->CanCharacterBeSeen(KilledCharacter))
			{
				if (KilledCharacter->IsOnSWATTeam())
					Swat->PlayRawVO(VO_SWAT_GENERAL::CALL_REPORT_DEAD_SWAT);// TODO: only on report?
				else if (KilledCharacter->IsSuspect())
					Swat->PlayRawVO(VO_SWAT_GENERAL::CALL_REPORT_DEAD_SUSPECT);// TODO: only on report?
				else
					Swat->PlayRawVO(VO_SWAT_GENERAL::CALL_REPORT_DEAD_CIVILIAN);// TODO: only on report?
			}
		}
	}
	
	//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::White, "AI Killed. " + FString::SanitizeFloat(RemainingAICharacters.Num() - 1) = " ai remaining. ");

	AReadyOrNotPlayerState* ps = InstigatorCharacter ? Cast<AReadyOrNotPlayerState>(InstigatorCharacter->GetPlayerState()) : nullptr;

	if (ACyberneticCharacter* ai = Cast<ACyberneticCharacter>(KilledCharacter))
	{
		if (ps)
		{
			ps->Kills++;
			ps->KillsThisLife++;
		}

		if (Mode != ECOOPMode::CM_ActiveShooter &&
			Mode != ECOOPMode::CM_HostageRescue)
		{
			const float Damage = AI_CONFIG_GET_FLOAT("AIKilledMorale.Damage");
			const float InnerRadius = AI_CONFIG_GET_FLOAT("AIKilledMorale.DamageInnerRadius");
			const float OuterRadius = AI_CONFIG_GET_FLOAT("AIKilledMorale.DamageOuterRadius");
			const EEasingFunc::Type Curve = UReadyOrNotFunctionLibrary::StringToEasingFunc(AI_CONFIG_GET_STRING("AIKilledMorale.DamageFalloffCurve"));

			FMoraleDamageTraceParameters LOSParameters;
			LOSParameters.bEnable = true;
			UMoraleComponent::ApplyRadialMoraleDamageWithFalloff(this, ai->GetActorLocation(), Damage, InnerRadius, OuterRadius, LOSParameters, {ETeamType::TT_CIVILIAN, ETeamType::TT_SUSPECT}, Curve, "AI Killed");
		}

		if (ai->KilledByHeadshot())
		{
			if (ps)
			{
				ps->Headshots += 1;
			}
		}
		
		if (ai->IsSuspect() || ai->IsCivilian())
		{
			const bool bHadActivity = ai->GetCyberneticsController() ? ai->GetCyberneticsController()->GetCurrentActivity() != nullptr : false;

			if (!bHadActivity && ai->TimeSinceLastAggressiveForce > 1.0f)
			{
				if (ai->IsArrested() || ai->IsSurrenderedFor(5.0f))
				{
					if (InstigatorCharacter)
					{
						FriendlyAIKilled(InstigatorCharacter, KilledCharacter);
					}
				}
			}
		}

		if (!ai->bBroadcastedStressIncrease)
		{
			ai->bBroadcastedStressIncrease = true;
			for (ACyberneticCharacter* AI : GetWorld()->GetGameState<AReadyOrNotGameState>()->AllAICharacters)
			{
				if (AI != ai && ai->GetTeam() == AI->GetTeam())
				{
					AI->Stress += FMath::Clamp(AI_CONFIG_GET_FLOAT("FriendlyKilledStress", 0.0f), 0.0f, 1.0f);
				}
			}
		}
	}
}

void ACoopGM::AIIncapacitated(AReadyOrNotCharacter* IncapacitatedCharacter, AReadyOrNotCharacter* InstigatorCharacter)
{
	TArray<ASWATCharacter*> SwatTeam = USWATManager::Get(this)->SwatAI;
	SwatTeam.Remove(Cast<ASWATCharacter>(IncapacitatedCharacter));

	if (SwatTeam.Num() > 0)
	{
		if (ASWATCharacter* Swat = SwatTeam[FMath::RandRange(0, SwatTeam.Num()-1)])
		{
			if (Swat->GetCyberneticsController()->GetTargetingComp()->CanCharacterBeSeen(IncapacitatedCharacter))
			{
				if (IncapacitatedCharacter->IsOnSWATTeam())
					Swat->PlayRawVO(VO_SWAT_GENERAL::CALL_REPORT_INCAPACITATED_SWAT);// TODO: only on report?
				else if (IncapacitatedCharacter->IsSuspect())
					Swat->PlayRawVO(VO_SWAT_GENERAL::CALL_REPORT_INCAPACITATED_SUSPECT);// TODO: only on report?
				else
					Swat->PlayRawVO(VO_SWAT_GENERAL::CALL_REPORT_INCAPACITATED_CIVILIAN);// TODO: only on report?
			}
		}
	}
	
	if (IncapacitatedCharacter->LastDamageEvent.Instigator)
	{
		if (APlayerCharacter* IncapacitatedBy = IncapacitatedCharacter->LastDamageEvent.Instigator->GetPawn<APlayerCharacter>())
		{
			if (ACyberneticCharacter* ai = Cast<ACyberneticCharacter>(IncapacitatedCharacter))
			{
				if (ai->IsSuspect() || ai->IsCivilian())
				{
					const bool bHadActivity = ai->GetCyberneticsController() ? ai->GetCyberneticsController()->GetCurrentActivity() != nullptr : false;

					if (!bHadActivity && ai->TimeSinceLastAggressiveForce > 1.0f)
					{
						if (ai->IsArrested() || ai->IsSurrenderedFor(5.0f))
						{
							FriendlyAIKilled(IncapacitatedBy, ai);
						}
					}
				}
			}
		}
	}

	if (ACyberneticCharacter* ai = Cast<ACyberneticCharacter>(IncapacitatedCharacter))
	{
		if (!ai->bBroadcastedStressIncrease)
		{
			ai->bBroadcastedStressIncrease = true;
			for (ACyberneticCharacter* AI : GetWorld()->GetGameState<AReadyOrNotGameState>()->AllAICharacters)
			{
				if (AI != ai && ai->GetTeam() == AI->GetTeam())
				{
					AI->Stress += FMath::Clamp(AI_CONFIG_GET_FLOAT("FriendlyKilledStress", 0.0f), 0.0f, 1.0f);
				}
			}
		}
	}
}

void ACoopGM::FriendlyAIKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter)
{
	SCOPE_CYCLE_COUNTER(STAT_COOPFriendlyAIKilled);

	if (USWATManager* SwatManager = USWATManager::Get(this))
	{
		SwatManager->RespondToPlayerTeamKill(InstigatorCharacter);
	}
}

void ACoopGM::IncapHumanKilled(AReadyOrNotCharacter* InstigatorCharacter, AIncapacitatedHuman* KilledHuman)
{
	if (USWATManager* SwatManager = USWATManager::Get(this))
	{
		SwatManager->RespondToPlayerTeamKill(InstigatorCharacter, KilledHuman->IsChild());
	}
}

void ACoopGM::AIArrested(AReadyOrNotCharacter* ArrestedCharacter, AReadyOrNotCharacter* InstigatorCharacter)
{
	SCOPE_CYCLE_COUNTER(STAT_COOPEnemyAIArrested);
	
	if (!InstigatorCharacter)
		return;
	
	if (AReadyOrNotPlayerState* ps = Cast<AReadyOrNotPlayerState>(InstigatorCharacter->GetPlayerState()))
	{
		ps->Arrests++;
	}
}

void ACoopGM::AISurrendered(AReadyOrNotCharacter* Character)
{
}

APlayerStart* ACoopGM::FindPlayerStartForTeam(ETeamType Team)
{
	SCOPE_CYCLE_COUNTER(STAT_COOPFindPlayerStartForTeam);

	ACoopGS* gs = GetWorld()->GetGameState<ACoopGS>();
	FLevelDataLookupTable LevelData = UBpGameplayHelperLib::GetLevelData(GetWorld());
	ESelectedSpawn SpawnPoint = ESelectedSpawn::SS_None;
	if (gs)
	{
		if (Team == ETeamType::TT_SERT_RED || Team == ETeamType::TT_SQUAD)
		{
			SpawnPoint = gs->SelectedRedSpawnPoint;
		}
		else if (Team == ETeamType::TT_SERT_BLUE)
		{
			SpawnPoint = gs->SelectedBlueSpawnPoint;
		}
	}

	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		APlayerStart* start = *It;
		if (start)
		{
			switch (SpawnPoint)
			{
			default:
			case ESelectedSpawn::SS_FirstSpawn:
				if (start->PlayerStartTag == LevelData.Spawn_1.SpawnLabel)
				{
					return start;
				}
				break;
			case ESelectedSpawn::SS_SecondSpawn:
				if (start->PlayerStartTag == LevelData.Spawn_2.SpawnLabel)
				{
					return start;
				}
				break;
			case ESelectedSpawn::SS_ThirdSpawn:
				if (start->PlayerStartTag == LevelData.Spawn_3.SpawnLabel)
				{
					return start;
				}
				break;
			case ESelectedSpawn::SS_FourthSpawn:
				if (start->PlayerStartTag == LevelData.Spawn_4.SpawnLabel) {
					return start;
				}
				break;
			}
		}
	}
	return nullptr;
}

void ACoopGM::FindNewSquadLeader()
{
	SCOPE_CYCLE_COUNTER(STAT_COOPFindNewSquadLeader);

	ACoopGS* gs = GetGameState<ACoopGS>();
	if (!gs)
	{
		return;
	}

	for (TActorIterator<APlayerCharacter> It(GetWorld()); It; ++It)
	{
		APlayerCharacter* pc = *It;
		if (pc && !pc->IsDeadOrUnconscious())
		{
			AReadyOrNotPlayerState* SqlPS = Cast<AReadyOrNotPlayerState>(pc->GetPlayerState());
			if (SqlPS)
			{
				SqlPS->bSquadLeader = true;
				gs->Multicast_BroadcastNewSquadLeader(pc);
				return;
			}

		}
	}
}

void ACoopGM::ExfiltrateMission(TArray<ASWATCharacter*> ExfilCharacters)
{
	Super::ExfiltrateMission(ExfilCharacters);
	GetWorld()->GetTimerManager().ClearTimer(CheckWinConditions_Handle);
	StartMissionEndTimer(false);
}
