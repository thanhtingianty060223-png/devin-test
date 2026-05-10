// Copyright Void Interactive, 2023

#include "ReadyOrNotGameState.h"

#include "FMODAmbientSound.h"
#include "ObjectPoolerWorldSubsystem.h"
#include "ReadyOrNotGameMode.h"
#include "ReadyOrNotLevelScript.h"
#include "ReadyOrNotPlayerCameraManager.h"
#include "Actors/CoverLandmark.h"
#include "Actors/Door.h"
#include "Actors/WorldDataGenerator.h"

#include "Net/UnrealNetwork.h"

#include "Actors/Gameplay/ReadyOrNotPlayerState.h"

#include "Characters/ReadyOrNotPlayerController.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Characters/CyberneticController.h"
#include "Components/InteractableComponent.h"
#include "Engine/DemoNetDriver.h"
#include "GameModes/CoopGS.h"

#include "HUD/Widgets/PlanningMapWidget.h"

#include "Metagame/ChallengeManager.h"
#include "Metagame/Profile.h"

#include "LevelSequence/Public/LevelSequence.h"

#include "lib/ReadyOrNotFunctionLibrary.h"

#include "HUD/Widgets/PreMissionPlanning.h"
#include "Info/MapStatisticsSystem.h"
#include "Info/MissionPlanManager.h"
#include "Info/SWATManager.h"
#include "Interfaces/IHttpResponse.h"
#include "Objectives/DefuseBombThreats.h"

#if (WITH_EDITOR || UE_BUILD_DEVELOPMENT || UE_BUILD_DEBUG)
#include "Debug/BadAIAction.h"
#endif

TAutoConsoleVariable<int32> CVarRonSyncedTimeDilation(TEXT("a.RonSyncedTimeDilation"), 1, TEXT("Turn on synced time dialation across clients."));
TAutoConsoleVariable<int32> CVarRonEnableLoadoutInEditor(TEXT("a.RonEnableLoadoutInEditor"), 0, TEXT("Turn on the premission/loadout screen."));

TAutoConsoleVariable<int32> CVarRonEditorRecordGameplay(TEXT("a.RonEditorRecordGameplay"), 0, TEXT("Record demos automatically in the editor"));

DECLARE_CYCLE_STAT(TEXT("RoN ~ Game State Tick"), STAT_GameStateTick, STATGROUP_ReadyOrNotGameState);
DECLARE_CYCLE_STAT(TEXT("RoN ~ AI Debug Tick"), STAT_AIDebugTick, STATGROUP_ReadyOrNotGameState);

#define DEFAULT_PARTICLE_POOL_SIZE 255

void AReadyOrNotGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AReadyOrNotGameState, NextHost);
	DOREPLIFETIME(AReadyOrNotGameState, MigrationGUID);
	DOREPLIFETIME(AReadyOrNotGameState, ScoringManager);
	DOREPLIFETIME(AReadyOrNotGameState, RandomStreamSeed);
	
	DOREPLIFETIME(AReadyOrNotGameState, NextURLReplicated);
	DOREPLIFETIME(AReadyOrNotGameState, WhitelistedLabels);

	DOREPLIFETIME(AReadyOrNotGameState, MatchState);
	DOREPLIFETIME(AReadyOrNotGameState, RoundWinningTeam);
	DOREPLIFETIME(AReadyOrNotGameState, MatchWinningTeam);
	DOREPLIFETIME(AReadyOrNotGameState, MissionDescription);
	DOREPLIFETIME(AReadyOrNotGameState, MissionName);
	DOREPLIFETIME(AReadyOrNotGameState, MissionObjectives);
	DOREPLIFETIME(AReadyOrNotGameState, PlanningTimeLeft);
	DOREPLIFETIME(AReadyOrNotGameState, Scorelimit);
	DOREPLIFETIME(AReadyOrNotGameState, EndPlayTimer);
	DOREPLIFETIME(AReadyOrNotGameState, RoundTimeRemaining)
	DOREPLIFETIME(AReadyOrNotGameState, bUseTimelimit);
	DOREPLIFETIME(AReadyOrNotGameState, bWaitingForPlayers);
	DOREPLIFETIME(AReadyOrNotGameState, RoundWinners);
	DOREPLIFETIME(AReadyOrNotGameState, RoundsPlayed);
	DOREPLIFETIME(AReadyOrNotGameState, RoundsToPlay);
	DOREPLIFETIME(AReadyOrNotGameState, bRunWarmup);
	DOREPLIFETIME(AReadyOrNotGameState, RedTeamWins);
	DOREPLIFETIME(AReadyOrNotGameState, BlueTeamWins);
	DOREPLIFETIME(AReadyOrNotGameState, TotalMissionAbuseCount);
	DOREPLIFETIME(AReadyOrNotGameState, bHasHostFinishedLoading);
	
	DOREPLIFETIME(AReadyOrNotGameState, ServerTimeUntilNextMap);
	
	DOREPLIFETIME(AReadyOrNotGameState, CurrentReferendum);
	
	DOREPLIFETIME(AReadyOrNotGameState, BlueTeamPlayers);
	DOREPLIFETIME(AReadyOrNotGameState, RedTeamPlayers);

	DOREPLIFETIME(AReadyOrNotGameState, DrawingPointData);

	DOREPLIFETIME(AReadyOrNotGameState, ModeURL_Replicated);

	DOREPLIFETIME(AReadyOrNotGameState, Reinforcements_TimeRemaining);
	DOREPLIFETIME(AReadyOrNotGameState, bUseReinforcements);

	DOREPLIFETIME(AReadyOrNotGameState, CustomTimeDilationApplied);
	DOREPLIFETIME(AReadyOrNotGameState, AdminPlayerControllers);

	DOREPLIFETIME(AReadyOrNotGameState, Rep_GameModeSettings);
	DOREPLIFETIME(AReadyOrNotGameState, TimeTillGameStartCountdown);
	DOREPLIFETIME(AReadyOrNotGameState, TOCManager);
}

void AReadyOrNotGameState::AddPlayerState(APlayerState* PlayerState)
{
	Super::AddPlayerState(PlayerState);
	
	AMissionPlanManager::AddPlayer(this, Cast<AReadyOrNotPlayerState>(PlayerState));
	OnPlayerStateAdded.Broadcast(PlayerState);
}

void AReadyOrNotGameState::RemovePlayerState(APlayerState* PlayerState)
{
	Super::RemovePlayerState(PlayerState);

	OnPlayerStateRemoved.Broadcast(PlayerState);
}

AReadyOrNotGameState::AReadyOrNotGameState()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickInterval = 0.0f;
	
	IFMODStudioModule::Get();

	AnnouncerAudioComponent = CreateDefaultSubobject<UFMODAudioComponent>(TEXT("Announcer Audio Component"));

	if (AnnouncerAudioComponent)
	{
		if (!AnnouncerAudioComponent->Event) 
		{
			ConstructorHelpers::FObjectFinder<UFMODEvent> AnnouncerEventFinder(TEXT("FMODEvent'/Game/FMOD/Events/Dialogue/AnnouncerLines.AnnouncerLines'"));
			AnnouncerAudioComponent->SetEvent(AnnouncerEventFinder.Object);
		}
	}

	if (!ReplenishAllAmmoSound) 
	{
		ConstructorHelpers::FObjectFinder<UFMODEvent> ReplenishAllAmmoFinder(TEXT("FMODEvent'/Game/FMOD/Events/Weapons/870/870_Reload_Start.870_Reload_Start'"));
		ReplenishAllAmmoSound = ReplenishAllAmmoFinder.Object;
	}

	/// this will be replicated from the game mode
	MatchState = EMatchState::MS_None;

	SubPreMissionPlanningLevel = FSoftObjectPath("World'/Game/ReadyOrNot/Level/PremissionPlanning/SubPreMissionPlanning_ReduxV2.SubPreMissionPlanning_ReduxV2'");
}

void AReadyOrNotGameState::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	SCOPE_CYCLE_COUNTER(STAT_GameStateTick);
	
	#if (WITH_EDITOR || UE_BUILD_DEVELOPMENT || UE_BUILD_DEBUG)
	//TickBadAIAction();
	#endif
	
	#if !UE_BUILD_SHIPPING
	{
		SCOPE_CYCLE_COUNTER(STAT_AIDebugTick);
		
		for (const ACyberneticCharacter* AICharacter : AllAICharacters)
		{
			if (IsValid(AICharacter) && AICharacter->IsActiveForThinking())
			{
				AICharacter->GetCyberneticsController()->DisplayAIDebugInfo(DeltaSeconds);
				AICharacter->GetCyberneticsController()->GetTargetingComp()->TickComponent_Debug(DeltaSeconds);
			}
		}
	}
	#endif

	/*
    if (AReadyOrNotGameState* GS = GetWorld()->GetGameState<AReadyOrNotGameState>())
    {
		for (FRoom& Room : GS->RoomData->Rooms)
		{
			if (Room.bClearedBySwat)
				ULog::Bool(Room.bClearedBySwat, Room.Name.ToString());
		}
    }
    */
	
	// Game instance should exist given we're in gamestate but may as well check
	AReadyOrNotPlayerController* pc = GetGameInstance() ? Cast<AReadyOrNotPlayerController>(GetGameInstance()->GetFirstLocalPlayerController()) : nullptr;
	if (!IsValid(pc))
		return;

	AReadyOrNotCharacter* LocalPlayer = Cast<AReadyOrNotCharacter>(pc->GetPawn());
	
	if (USWATManager::Get(this))
	{
		if (Cast<ASWATCharacter>(USWATManager::Get(this)->SquadLeader))
			LocalPlayer = USWATManager::Get(this)->SquadLeader;
	}

	// Adjust the tick rate on all doors and interactables. this helps performance quite a bit when we're far away from a particular door/interatable
	if (LocalPlayer)
	{
		for (UInteractableComponent* InteractableComponent : AllInteractableComponents)
		{
			const float DistanceToLocalPlayer = FVector::Distance(LocalPlayer->GetActorLocation(), InteractableComponent->GetComponentLocation());
			const float NewTickRate = FMath::GetMappedRangeValueClamped(FVector2D(InteractableComponent->ShowPromptAtDistance*2.0f, InteractableComponent->ShowPromptAtDistance*4.0f), FVector2D(0.033f, 1.0f), DistanceToLocalPlayer);
			//LOG_NUMBER(NewTickRate);
			InteractableComponent->SetComponentTickInterval(NewTickRate);
		}
	}

	if (!HasAuthority())
	{
		// allow smoove countdowns for clients while waiting for rep
		int32 OutTotal = 0, OutLoading = 0, OutLoaded = 0;
		if (AReadyOrNotPlayerState::HasEveryoneFinishedLoading(OutTotal, OutLoading, OutLoaded))
		{
			TimeTillGameStartCountdown -= DeltaSeconds;
			TimeTillGameStartCountdown = FMath::Max(0.0f, TimeTillGameStartCountdown);
		}
	}

	UpdateActiveControllers();

	#if UE_SERVER
	bHasHostFinishedLoading = true;
	#endif

	if (TimeUntilRefreshNextHost <= 0.0f || !NextHost)
	{
		RefreshNextHost();
		TimeUntilRefreshNextHost = 30.0f;
	}
	else
	{
		TimeUntilRefreshNextHost -= DeltaSeconds;
	}

	#if !UE_BUILD_SHIPPING
	if (CVarRonSyncedTimeDilation.GetValueOnAnyThread() == 1)
		SetTimeDilationSynced(UKismetMathLibrary::FInterpTo(UGameplayStatics::GetGlobalTimeDilation(GetWorld()), InterpToTimeDialtion, DeltaSeconds, 1.0f));
	#endif

	if (bPendingWinsUpdate && OnWinsUpdated.IsBound())
	{
		OnWinsUpdated.Broadcast();
		bPendingWinsUpdate = false;
	}

	if (pc->IsLocalController() && pc->PlayerCameraManager)
	{
		if (MatchState == EMatchState::MS_None)
		{
			pc->PlayerCameraManager->StartCameraFade(1.0f, 1.0f, 1.0f, FLinearColor::Black, true, true);
		}
	}

	// game is starting.. or about to start.
	if (MatchState == EMatchState::MS_Playing)
	{
		RestoreLevelEffects();
		
		if (!bSpawnedMatchStartWidget && GetNetMode() != ENetMode::NM_DedicatedServer)
		{
			if (!pc->bIsReplaySpectator)
			{
				AReadyOrNotPlayerState* ps = pc ? Cast<AReadyOrNotPlayerState>(pc->PlayerState) : nullptr;
				if (ps && ps->bIsInGame)
				{
					bSpawnedMatchStartWidget = true;
					pc->Client_CreateWidget("GameStartInformation");

					if (!bHasEverPlayedGameIntroRules)
					{
						bHasEverPlayedGameIntroRules = true;
						Multicast_PlayAnnouncerForTeam_Implementation(GameRulesIntroAnnouncerRowName, ps->GetTeam());
					}
				}
			}
		}
		
		TimeSinceMatchStarted += DeltaSeconds;
		RoundTimeElapsed += DeltaSeconds;
	}
	else if (MatchState == EMatchState::MS_Warmup)
	{
		PreGamePlayingStateLogic();
	}
}

void AReadyOrNotGameState::ResetReplicatedTimers()
{
	// set planning time left based on session settings
	if (GetLocalRole() >= ROLE_Authority)
	{
		AReadyOrNotGameMode* gm = GetWorld()->GetAuthGameMode<AReadyOrNotGameMode>();
		PlanningTimeLeft = gm ? gm->GetGameModeSettings().RoundStartTime : 0;
		//Scorelimit = gm->GetGameModeSettings().ScoreLimit;
		EndPlayTimer = 30.0f;
	}
}

void AReadyOrNotGameState::BeginPlay()
{
	Super::BeginPlay();

	AllItems.Reserve(100);
	AllReadyOrNotCharacters.Reserve(50);
	AllAICharacters.Reserve(50);
	
	// load all data
	if (!GetWorld()->GetName().Contains("MainMenu"))
	{
		LoadDataTables();
	}

	if (HasAuthority())
	{
		RandomStreamSeed = FMath::Rand();
		OnRep_StreamSeed();
	}
	
	#if WITH_EDITOR
	GetWorld()->Exec(GetWorld(), TEXT("net pktlag=200"));
	GetWorld()->Exec(GetWorld(), TEXT("net pktloss=20"));

	if (!UReadyOrNotStatics::GetReadyOrNotGameInstance()->HostMigrationManager)
	{
		// Init this in editor only as StartGameInstance is caleld for the editor windows but not the game clients
		UReadyOrNotStatics::GetReadyOrNotGameInstance()->HostMigrationManager = NewObject<UHostMigrationManager>(UReadyOrNotStatics::GetReadyOrNotGameInstance());
		UReadyOrNotStatics::GetReadyOrNotGameInstance()->HostMigrationManager->Init();
	}
	#endif

	ResetReplicatedTimers();
	
	VoiceConfig = NewObject<UReadyOrNotVoiceConfig>(this);
	VoiceConfig->ReloadConfig();
	VoiceConfig->Init();
	
	if (!IsRunningDedicatedServer())
	{
		// Create challenge manager
		/*
		if (!ChallengeManager && ChallengeManagerClass)
		{
			ChallengeManager = NewObject<UChallengeManager>(this, ChallengeManagerClass.Get());
			if (ChallengeManager)
			{
				ChallengeManager->InitChallenges(this, UBpGameplayHelperLib::GetLevelData(GetWorld()));
			}
		}
		*/
		
		UChallengeManager::Get(this)->InitChallenges(this, UBpGameplayHelperLib::GetLevelData(GetWorld()));
		
		if (UObjectPoolerWorldSubsystem* ObjectPoolerSubsystem = GetWorld()->GetSubsystem<UObjectPoolerWorldSubsystem>())
		{
			ObjectPoolerSubsystem->InitializeObjectPools();
		}

		// initliaze the particle component pool for use with anim notifies
		/*
		if (FApp::CanEverRender() && !GetWorld()->IsNetMode(NM_DedicatedServer))
		{
			ParticleComponentPool_AnimNotify_Inactive.Reserve(255);
			for (uint32 i = 0; i < DEFAULT_PARTICLE_POOL_SIZE; i++)
			{
				if (UParticleSystemComponent* PSC = NewObject<UParticleSystemComponent>(GetWorld()))
				{
					PSC->bAutoDestroy = false;
					PSC->bAllowAnyoneToDestroyMe = false;
					PSC->SecondsBeforeInactive = 0.0f;
					PSC->bAutoActivate = false;
					PSC->SetTemplate(nullptr);
					PSC->bOverrideLODMethod = false;
					PSC->OnSystemFinished.AddDynamic(this, &AReadyOrNotGameState::OnAnimNotifyPoolParticleFinished);
					
					ParticleComponentPool_AnimNotify_Inactive.Add(PSC);
				}
			}
		}
		*/
	}

	if (SceneCapturePlayerCameraClass)
	{
		GetWorld()->SpawnActor(SceneCapturePlayerCameraClass);
	}

	GetWorld()->GetTimerManager().SetTimer(SerialCheck_Handle, this, &AReadyOrNotGameState::SerialCheck, 30.0f, true, 5.0f);

	if (!GetWorld()->GetMapName().Contains("MainMenu") && !GetWorld()->GetMapName().Contains("Station"))
	{
		FString SteamName; 

		APlayerController* pc = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		if (pc)
		{
			if (pc->PlayerState)
			{
				if (pc->PlayerState->GetUniqueId().IsValid())
				{
					FString SteamId = pc->PlayerState->GetUniqueId().ToString();
					SteamName = pc->PlayerState->GetPlayerName();
				}
			}
		}

		if (SteamName.IsEmpty())
		{
			SteamName = "Unknown";
		}
		
		#ifdef REPLAY_SYSTEM
		UReadyOrNotGameInstance* gi = Cast<UReadyOrNotGameInstance>(GetWorld()->GetGameInstance());
		bool bReplayEnabled;
		UBpGameplayHelperLib::LoadReplayEnabled(bReplayEnabled);
		if(gi && !GetWorld()->GetName().Contains("TransitionMap") && bReplayEnabled)
		{
			gi->StartRecordingReplay();
		}
		#endif
	}
	
	if (AWorldDataGenerator* WorldDataGenerator = AWorldDataGenerator::Get(GetWorld()))
	{
		RoomData = &WorldDataGenerator->RoomData;
	}

	GetWorld()->GetTimerManager().SetTimer(TH_UpdateDoorTickIntervals, this, &AReadyOrNotGameState::UpdateDoorTickIntervals, 0.5f, true, 0.f);
}

void AReadyOrNotGameState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (SubPreMissionPlanningLevel)
	{
		GetWorld()->RemoveStreamingLevel(PreMissionStreamedLevel);
	}

	for (TActorIterator<APlayerCharacter>It(GetWorld()); It; ++It)
	{
		APlayerCharacter* PlayerCharacter = *It;
		PlayerCharacter->Destroy();
	}

	UReadyOrNotFunctionLibrary::StopAllAudio(GetWorld());

	UChallengeManager::Get(this)->SaveChallenges();

#ifdef REPLAY_SYSTEM
	UReadyOrNotGameInstance* gi = Cast<UReadyOrNotGameInstance>(GetWorld()->GetGameInstance());
	bool bReplayEnabled;
	UBpGameplayHelperLib::LoadReplayEnabled(bReplayEnabled);
	if(gi && bReplayEnabled)
	{
		gi->StopRecordingReplay();
	}
#endif

	if (AMapStatisticsSystem* Statistics = UBpGameplayHelperLib::GetMapStatistics())
	{
		Statistics->EndLevel();
	}
	
	if (UObjectPoolerWorldSubsystem* ObjectPoolerSubsystem = GetWorld()->GetSubsystem<UObjectPoolerWorldSubsystem>())
	{
		ObjectPoolerSubsystem->DestroyObjectPools();
	}
}

void AReadyOrNotGameState::LoadDataTables()
{
	const UDataSingleton* Dt = UBpGameplayHelperLib::GetRoNData();
	LoadedDataTables.Empty();
	LoadedDataTables.Add(Dt->AIDataLookupTable);
	LoadedDataTables.Add(Dt->LevelDataLookupTable);
	LoadedDataTables.Add(Dt->AnimationDataLookupTable);
	LoadedDataTables.Add(Dt->AnimatedIconLookupTable);
	LoadedDataTables.Add(Dt->DoorDataLookupTable);
	LoadedDataTables.Add(Dt->TrapDataLookupTable);
	LoadedDataTables.Add(Dt->ConversationLookupTable);
	LoadedDataTables.Add(Dt->GameSettingsLookupTable);
	LoadedDataTables.Add(Dt->CharacterLookOverrideTable);
	LoadedDataTables.Add(Dt->RonInputKeyTable);
	LoadedDataTables.Add(Dt->WidgetDataLookupTable);
}

void AReadyOrNotGameState::UpdateActiveControllers()
{
	NumSuspectsActive = 0;
	NumCiviliansActive = 0;
	NumSwatActive = 0;
	
	for (const ACyberneticCharacter* AI : AllAICharacters)
	{
		if (IsValid(AI) && AI->IsActive())
		{
			const ETeamType Team = AI->GetTeam();
			if (Team == ETeamType::TT_SUSPECT)
			{
				NumSuspectsActive++;
			}
			else if (Team == ETeamType::TT_CIVILIAN)
			{
				NumCiviliansActive++;
			}
			else if (Team == ETeamType::TT_SQUAD || Team == ETeamType::TT_SERT_RED || Team == ETeamType::TT_SERT_BLUE)
			{
				NumSwatActive++;
			}
		}
	}

	bCharactersDirty = false;
}

void AReadyOrNotGameState::SetGlobalSuspendVoiceOver(bool bEnable)
{
	bGlobalSuspendVoiceOver = bEnable;
}

void AReadyOrNotGameState::OnRep_StreamSeed()
{
	RandomStream = FRandomStream(RandomStreamSeed);
}

int32 AReadyOrNotGameState::GetPlayerCharactersNum()
{
	int32 OutNum = 0;
	for (TActorIterator<APlayerCharacter>It(GetWorld()); It; ++It)
	{
		OutNum++;
	}
	return OutNum;
}

void AReadyOrNotGameState::OnRep_NextHost()
{
	UReadyOrNotGameInstance* GameInstance = UReadyOrNotStatics::GetReadyOrNotGameInstance();
	if (GameInstance && GameInstance->HostMigrationManager)
	{
		// only sync the expected players once the game is going
		GameInstance->HostMigrationManager->SetNextHost(NextHost, MigrationGUID);
	}
}

void AReadyOrNotGameState::RefreshNextHost()
{
	AReadyOrNotPlayerController* pc = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld());
	if (!pc)
		return;
	
	if (HasAuthority())
	{
		if (MigrationGUID.IsEmpty())
		{
			MigrationGUID = FGuid::NewGuid().ToString();
		}
		// max uint8 (real ping divided by 4)
		uint8 LowestPing = 255;
		for (APlayerState* ps : PlayerArray)
		{
			// ##UE5UPGRADE
			if (ps != pc->PlayerState && ps->GetPingInMilliseconds() < LowestPing)
			{
				NextHost = ps;
				LowestPing = ps->GetPingInMilliseconds();
			}
		}

		OnRep_NextHost();
		
		
	} else
	{
		OnRep_NextHost();
	}
}

#if (WITH_EDITOR || UE_BUILD_DEVELOPMENT || UE_BUILD_DEBUG)
void AReadyOrNotGameState::TickBadAIAction()
{
	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0))
		{
			FVector CameraLocation;
			FRotator CameraRotation;
			CameraManager->GetCameraViewPoint(CameraLocation, CameraRotation);
			
			const FVector SpawnLoc = CameraLocation + (CameraRotation.Vector() * 10000.0f);

			BadAIActionActors.Remove(nullptr);
			
			if (PlayerController->WasInputKeyJustPressed(EKeys::RightBracket))
			{
				if (!UBpGameplayHelperLib::HasWidgetInViewport("BadAIActionReporter"))
	            {
					FHitResult HitResult;
					FCollisionQueryParams CollisionQueryParams;
					CollisionQueryParams.AddIgnoredActor(PlayerController->GetPawnOrSpectator());
					
					FCollisionObjectQueryParams CollisionObjectQueryParams;
					CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
					CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_DOOR);
					CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_DOORWAY);

					if (GetWorld()->LineTraceSingleByObjectType(HitResult, CameraLocation, SpawnLoc, CollisionObjectQueryParams))
					{
						const FVector Location = HitResult.Location + HitResult.ImpactNormal * 50.0f;

						if (ABadAIAction* BadAIActionActor = GetWorld()->SpawnActor<ABadAIAction>(ABadAIAction::StaticClass(), Location, FRotator::ZeroRotator))
						{
							BadAIActionActors.AddUnique(BadAIActionActor);
							
							const FWidgetLookupData WidgetData = UBpGameplayHelperLib::GetWidgetDataFromLookupData("BadAIActionReporter");

							if (UUserWidget* WidgetInstance = CreateWidget<UUserWidget>(GetWorld(), WidgetData.WidgetClass))
							{
								WidgetInstance->AddToViewport(99);
							}
						}
					}
				}
			}
			else if (PlayerController->WasInputKeyJustPressed(EKeys::LeftBracket))
			{
				if (!UBpGameplayHelperLib::HasWidgetInViewport("BadAIActionReporter"))
				{
					if (BadAIActionActors.Num() > 0)
					{
						if (ABadAIAction* BadAIAction = BadAIActionActors.Last())
						{
							BadAIAction->RemoveReport();
						}

						BadAIActionActors.RemoveAt(BadAIActionActors.Num() - 1);
						BadAIActionActors.Shrink();
					}
				}
			}
		}
	}
}
#endif

/*
void AReadyOrNotGameState::OnAnimNotifyPoolParticleFinished(UParticleSystemComponent* PSystem)
{
	ParticleComponentPool_AnimNotify_Inactive.AddUnique(PSystem);
	ParticleComponentPool_AnimNotify_Active.Remove(PSystem);
}

UParticleSystemComponent* AReadyOrNotGameState::GetAvailableParticleComponent_AnimNotify()
{
	if (ParticleComponentPool_AnimNotify_Inactive.Num() > 0)
	{
		UParticleSystemComponent* ParticleComp = ParticleComponentPool_AnimNotify_Inactive.Pop(false);
		ParticleComponentPool_AnimNotify_Active.Add(ParticleComp);
		return ParticleComp;
	}
	
	if (UParticleSystemComponent* PSC = NewObject<UParticleSystemComponent>(GetWorld()))
	{
		PSC->bAutoDestroy = false;
		PSC->bAllowAnyoneToDestroyMe = false;
		PSC->SecondsBeforeInactive = 0.0f;
		PSC->bAutoActivate = false;
		PSC->SetTemplate(nullptr);
		PSC->bOverrideLODMethod = false;
		PSC->OnSystemFinished.AddDynamic(this, &AReadyOrNotGameState::OnAnimNotifyPoolParticleFinished);
		
		ParticleComponentPool_AnimNotify_Active.Add(PSC);
		return PSC;
	}

	return nullptr;
}
*/

int32 AReadyOrNotGameState::GetNumPlayers()
{
	int32 NumPlayers = 0;
	for (TActorIterator<AReadyOrNotPlayerState>It(GetWorld()); It; ++It)
	{
		AReadyOrNotPlayerState* ps = *It;
		if (!ps->IsABot())
		{
			NumPlayers++;
		}
	}
	return NumPlayers;
}

TArray<AReadyOrNotPlayerState*> AReadyOrNotGameState::GetPlayersAvailableForVote() const
{
	TArray<AReadyOrNotPlayerState*> Controllers;
	for (TActorIterator<AReadyOrNotPlayerState> It(GetWorld()); It; ++It)
	{
		AReadyOrNotPlayerState* PlayerState = *It;
		const AReadyOrNotCharacter* PlayerCharacter = PlayerState->GetPawn<AReadyOrNotCharacter>();

		if (!PlayerCharacter)
			continue;
	
		// Dead characters and spectators are not counted for votes
		const bool bIsSpectator = PlayerState->IsSpectator(); 
	
		if (PlayerState->bIsReplaySpectator || bIsSpectator)
			continue;
	
		if (!PlayerCharacter->IsDeadOrUnconscious())
		{
			Controllers.AddUnique(PlayerState);
		}
	}

	return Controllers;
}

TArray<AReadyOrNotPlayerController*> AReadyOrNotGameState::GetControllersAvailableForVote() const
{
	TArray<AReadyOrNotPlayerController*> Controllers;
	for (TActorIterator<AReadyOrNotPlayerController> It(GetWorld()); It; ++It)
	{
		AReadyOrNotPlayerController* PlayerController = *It;
		const AReadyOrNotCharacter* PlayerCharacter = PlayerController->GetPawn<AReadyOrNotCharacter>();

		if (!PlayerCharacter)
			continue;
	
		// Dead characters and spectators are not counted for votes
		const bool bIsSpectator = PlayerController->GetPlayerState<AReadyOrNotPlayerState>() && PlayerController->GetPlayerState<AReadyOrNotPlayerState>()->IsSpectator();
	
		if (PlayerController->bIsReplaySpectator || bIsSpectator)
			continue;
	
		if (!PlayerCharacter->IsDeadOrUnconscious())
		{
			Controllers.AddUnique(PlayerController);
		}
	}

	return Controllers;
}

void AReadyOrNotGameState::OnAlphaAccessChecked(bool bBanned, FString BanReason)
{
	//dedicated server is always allowed
	if (UKismetSystemLibrary::IsDedicatedServer(GetWorld()))
		bBanned = false;

	if (!bBanned)
		return;

	if (GetWorld())
	{
		if (GetWorld()->GetMapName().Contains("MainMenu"))
		{
			// already handled
			return;
		}
		else
		{
			AReadyOrNotPlayerController* pc = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld());
			if (pc)
			{
				UReadyOrNotGameInstance* gi = Cast<UReadyOrNotGameInstance>(GetWorld()->GetGameInstance());
				if (gi) gi->SetBanned(true);
				pc->BP_ReturnToMenu(FText::FromString(BanReason));
			}
		}
	}
}

void AReadyOrNotGameState::CreateLevelObjectives()
{
	TArray<TSoftClassPtr<AObjective>>* Objectives = nullptr;
	
	if (AReadyOrNotLevelScript* LS = Cast<AReadyOrNotLevelScript>(GetWorld()->GetLevelScriptActor()))
	{
		Objectives = &LS->LevelData.Objectives;
	}
	
	if (Objectives)
	{
		if (MissionObjectives.Num() == 0)
		{
			bool bObjectivesChanged = false;
			
			#if !WITH_EDITOR
			if (UReadyOrNotFunctionLibrary::GetCOOPMode() != ECOOPMode::CM_None)
			{
				ensureMsgf(Objectives->Num() > 0, TEXT("World %s running mode %s has no objectives."), *GetWorld()->GetName(), *RON_ENUM_TO_STRING(ECOOPMode, UReadyOrNotFunctionLibrary::GetCOOPMode()));
			}
			#endif

			for (TSoftClassPtr<AObjective>& ObjectiveSoftPtr : *Objectives)
			{
				AObjective* DefaultObjective = ObjectiveSoftPtr.LoadSynchronous() ? ObjectiveSoftPtr.LoadSynchronous()->GetDefaultObject<AObjective>() : nullptr;
				if (DefaultObjective)
				{
					if (DefaultObjective->LockedToMode != ECOOPMode::CM_None && UReadyOrNotFunctionLibrary::GetCOOPMode() != DefaultObjective->LockedToMode)
						continue;
				}
				
				AObjective* NewObjective = GetWorld()->SpawnActor<AObjective>(ObjectiveSoftPtr.LoadSynchronous());
				if (NewObjective)
				{
					NewObjective->SetOwner(this);
					NewObjective->SetReplicates(true);
					NewObjective->OnObjectiveCreated();

					MissionObjectives.Add(NewObjective);
					bObjectivesChanged = true;
				}
			}

			bool bHasBomb = false;
			for (TActorIterator<ABombActor> It(GetWorld()); It; ++It)
			{
				if (!It->bPVPBombOnly)
				{
					bHasBomb = true;
					break;
				}
			}

			if (bHasBomb)
			{
				// Bomb threat objective
				bool bHasBombThreatObj = false;
				for (AObjective* Obj : MissionObjectives)
				{
					if (Obj && Obj->IsA(ADefuseBombThreats::StaticClass()))
					{
						bHasBombThreatObj = true;
					}
				}
				
				if (!bHasBombThreatObj)
				{
					AObjective* NewObjective = GetWorld()->SpawnActor<AObjective>(ADefuseBombThreats::StaticClass());
					if (NewObjective)
					{
						NewObjective->SetOwner(this);
						NewObjective->SetReplicates(true);

						NewObjective->OnObjectiveCreated();

						MissionObjectives.Add(NewObjective);
						bObjectivesChanged = true;
					}
				}
			}

			if (bObjectivesChanged)
				OnMissionObjectivesUpdated.Broadcast();

			// All objectives have been created, can let them know so they can bind to events of other objectives now
			// Was gonna just bind to the above delegate call but may as well just do it here
			for (AObjective* Objective : MissionObjectives)
			{
				Objective->OnMissionObjectivesCreated();
			}
		}
	}
}

FGameModeSettings AReadyOrNotGameState::GetGameModeSettings()
{
	UDataTable* dt = UBpGameplayHelperLib::GetGameModeSettingsLookupDataTable();
	if (dt)
	{
		FGameModeSettings* data = Rep_GameModeSettings.GetRow<FGameModeSettings>("GameModeSettingsLookupData");
		if (data)
		{
			return *data;
		}
	}
	return FGameModeSettings();
}

void AReadyOrNotGameState::GetPlayerStatesOnTeamOrderedByScore(ETeamType Team, TArray<AReadyOrNotPlayerState*>& PlayerStates)
{
	PlayerStates = GetPlayerStatesOfTeam(Team);
	for (int32 i = 0; i < PlayerStates.Num(); i++)
	{
		for (int32 y = 1; y < PlayerStates.Num(); y++)
		{
			if (PlayerStates[i]->GetScore() < PlayerStates[y-1]->GetScore())
			{
				AReadyOrNotPlayerState* tmp = PlayerStates[y-1];
				PlayerStates[y-1] = PlayerStates[i];
				PlayerStates[i] = tmp;
			}
		}
	}

}

ABadAIAction* AReadyOrNotGameState::GetMostRecentBadAIActionReport() const
{
	if (BadAIActionActors.Num() == 0)
		return nullptr;

	return BadAIActionActors.Last();
}

void AReadyOrNotGameState::PreGamePlayingStateLogic()
{
	AReadyOrNotPlayerController* pc = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld());
	if (pc && pc->IsLocalController() && pc->PlayerCameraManager && pc->GetReadyOrNotPlayerState())
	{
		if (!bCompletedInitialLoad)
		{
			// load all data
			if (!GetWorld()->GetName().Contains("MainMenu"))
			{
#if WITH_EDITOR
				if (CVarRonEnableLoadoutInEditor.GetValueOnAnyThread() == 0)
				{
					pc->RemoveLoadingScreen();
					pc->GetRoNPlayerState()->Server_SetReady(true, pc->GetRoNPlayerState()->GetLoadout());
					return;
				
				}
#endif
			}



			if (TryLoadSubPreMissionPlanning())
			{
				pc->PlayerCameraManager->StartCameraFade(10.0f, 0.0f, 5.0f, FLinearColor::Black, false, false);
				bCompletedInitialLoad = true;
				pc->RemoveLoadingScreen();

				HideLevelEffects();
			}
			else
			{
				pc->PlayerCameraManager->StartCameraFade(1.0f, 1.0f, 5.0f, FLinearColor::Black, false, true);
			}

				
		}
		if (TimeTillGameStartCountdown <= 0.5f)
		{		
			bNearReadyFadenIn = true;
			pc->PlayerCameraManager->StartCameraFade(1.0f, 1.0f, 0.5f, FLinearColor::Black, false, false);
		}
		else if (bNearReadyFadenIn && TimeTillGameStartCountdown > 0.5f)
		{
			bNearReadyFadenIn = false;
			pc->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, 0.6f, FLinearColor::Black, false, false);
		}
	}		
	bSpawnedMatchStartWidget = false;
	TimeSinceMatchStarted = 0.0f;
}

void AReadyOrNotGameState::OnLoadoutFinished()
{
	bHasLeftLoadOut = true;
}

bool AReadyOrNotGameState::TryLoadSubPreMissionPlanning()
{
#if WITH_EDITOR
	if (GEditor)
	{
		if (GEditor->IsSimulatingInEditor())
		{
			return true;
		}
	}
#endif
	if (!PreMissionStreamedLevel)
	{
		// Remove any existing sub premission planning levels as we now load it directly instead of via the levels window
		for (int32 i = 0; i < GetWorld()->GetStreamingLevels().Num(); i++)
		{
			ULevelStreaming* Level = GetWorld()->GetStreamingLevels()[i];
			if (Level->GetWorldAssetPackageName().Contains("SubPreMissionPlanning"))
			{
				Level->SetShouldBeVisible(false);
				GetWorld()->RemoveStreamingLevel(Level);
			}
		}
		PreMissionStreamedLevel = NewObject<ULevelStreaming>(GetWorld(), ULevelStreamingDynamic::StaticClass(), NAME_None, RF_NoFlags);
		PreMissionStreamedLevel->SetWorldAsset(SubPreMissionPlanningLevel);
		GetWorld()->AddStreamingLevel(PreMissionStreamedLevel);

	} else
	{

#if WITH_EDITOR
		// hack so we don't try load the premission planning on the client in PIE (which doesn't work) should still allow the use of the menu etc
		if (GetWorld()->WorldType == EWorldType::PIE)
		{
			if (!HasAuthority())
			{
				return true;
			}
		}
#endif
		if (PreMissionStreamedLevel->GetCurrentState() != ULevelStreaming::ECurrentState::LoadedVisible)
		{
			PreMissionStreamedLevel->SetShouldBeVisible(true);
			PreMissionStreamedLevel->SetShouldBeLoaded(true);
		}
		else if (PreMissionStreamedLevel->GetCurrentState() == ULevelStreaming::ECurrentState::LoadedVisible)
		{
			if (PreMissionStreamedLevel->GetLoadedLevel())
			{
				PreMissionStreamedLevel->GetLoadedLevel()->bClientOnlyVisible = true;
				return true;
			}
		}
	}
	return false;
}

void AReadyOrNotGameState::SerialCheck()
{
#ifndef WITH_EDITOR
	UBpGameplayHelperLib::GetGameInstance(GetWorld())->OnSerialValidated.RemoveDynamic(this, &AReadyOrNotGameState::OnAuthenticationResponse);
	UBpGameplayHelperLib::GetGameInstance(GetWorld())->OnSerialValidated.AddDynamic(this, &AReadyOrNotGameState::OnAuthenticationResponse);
	URoNGameInstance::GS_IsSerialValid();
#endif
}

FString AReadyOrNotGameState::GetMapURL()
{
	return GetWorld()->GetMapName() + "?game=" + ModeURL_Replicated;
}

void AReadyOrNotGameState::PlayAnnouncerForTeam(FString SpeechRowName, ETeamType TeamType)
{
	Multicast_PlayAnnouncerForTeam(SpeechRowName, TeamType);
}

void AReadyOrNotGameState::Multicast_PlayAnnouncerForTeam_Implementation(const FString& SpeechRowName, ETeamType TeamType)
{
	AReadyOrNotPlayerController* pc = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld());
	if (pc)
	{
		if (pc->GetRoNPlayerState())
		{
			if (pc->GetRoNPlayerState()->GetTeam() != TeamType && TeamType != ETeamType::TT_NONE)
			{
				return;
			}

			FString Announcer = bPvPMode && pc->GetRoNPlayerState()->GetTeam() == ETeamType::TT_SERT_RED ? "MLOAnnouncer" : "SWATAnnouncer";
			FString OutFileName, OutFilePath;
			if (VoiceConfig->GetRandomVoiceLine(SpeechRowName, Announcer, OutFilePath, OutFileName))
			{
				AnnouncerAudioComponent->SetProgrammerSound(nullptr);
				AnnouncerAudioComponent->SetProgrammerSoundName(OutFilePath);
				AnnouncerAudioComponent->SetVolume(1.0f);
				AnnouncerAudioComponent->Play();
			}
		}
	}
}

//void AReadyOrNotGameState::OnRep_MissionObjectives()
//{
//	 //Do something when we receive a replicated objective
//}

void AReadyOrNotGameState::OnAuthenticationResponse(bool bSuccess, bool bSerialFound, bool bSerialValid, FString failedReason)
{
	if (!bSuccess || !bSerialValid || !bSerialFound)
	{
		UReadyOrNotGameInstance* gi = Cast<UReadyOrNotGameInstance>(GetWorld()->GetGameInstance());
		if (gi)
		{
			gi->MainMenuDisplayMessage = "You have been kicked due to serial key being invalid.";
		}
		UGameplayStatics::OpenLevel(GetWorld(), "TransitionMap", true);
	}
}

void AReadyOrNotGameState::AddDeathListener(TScriptInterface<IListenForDeath> DeathListener)
{
	DeathListeners.AddUnique(DeathListener);
}

void AReadyOrNotGameState::RemoveDeathListener(TScriptInterface<IListenForDeath> DeathListener)
{
	DeathListeners.Remove(DeathListener);
}

void AReadyOrNotGameState::AddGameStartListener(TScriptInterface<IListenForGameStart> GameStartListener)
{
	GameStartListeners.AddUnique(GameStartListener);
}

void AReadyOrNotGameState::RemoveGameStartListener(TScriptInterface<IListenForGameStart> GameStartListener)
{
	GameStartListeners.Remove(GameStartListener);
}

void AReadyOrNotGameState::AddGameEndListener(TScriptInterface<IListenForGameEnd> GameEndListener)
{
	GameEndListeners.AddUnique(GameEndListener);
}

void AReadyOrNotGameState::RemoveGameEndListener(TScriptInterface<IListenForGameEnd> GameEndListener)
{
	GameEndListeners.Remove(GameEndListener);
}

void AReadyOrNotGameState::OverWriteModeNameText(FText newModeName)
{
	ModeName = newModeName;
}

void AReadyOrNotGameState::OnRep_MatchState()
{
	if (MatchState == EMatchState::MS_Playing)
	{
		UHostMigrationManager* HostMigrationManager = UReadyOrNotStatics::GetReadyOrNotGameInstance()->HostMigrationManager;
		if (HostMigrationManager)
		{
			HostMigrationManager->SetHostMigrationInProgress(false);
		}
	}
}

void AReadyOrNotGameState::OnCharacterDied(AReadyOrNotCharacter* Victim, AReadyOrNotCharacter* Killer, AActor* Inflictor)
{
	for (int32 i = 0; i < DeathListeners.Num(); i++)
	{
		if (DeathListeners[i].GetObjectRef())
			IListenForDeath::Execute_OnCharacterDied(DeathListeners[i].GetObjectRef(), Victim, Killer, Inflictor);
	}
}

void AReadyOrNotGameState::OnGameStarted()
{
	for (int32 i = 0; i < GameStartListeners.Num(); i++)
	{
		if (GameStartListeners[i].GetObjectRef())
		{	
			IListenForGameStart::Execute_OnGameStarted(GameStartListeners[i].GetObjectRef());
		}
	}
}

void AReadyOrNotGameState::OnGameEnded()
{
	for (int32 i = 0; i < GameEndListeners.Num(); i++)
	{
		if (GameEndListeners[i].GetObjectRef())
			IListenForGameEnd::Execute_OnGameEnded(GameEndListeners[i].GetObjectRef());
	}
}

TArray<FDeploymentStatus> AReadyOrNotGameState::GetDeploymentStatusOfPlayers()
{
	TArray<FDeploymentStatus> ReturnStatus;

	TArray<AReadyOrNotPlayerState*> PlayerStates;
	
	if (!bPvPMode)
	{
		PlayerStates = GetPlayerStatesOfTeam(ETeamType::TT_NONE);
	}
	else
	{
		AReadyOrNotPlayerState* LocalPlayerState = UBpGameplayHelperLib::GetLocalPlayerState(GetWorld());
		if (!LocalPlayerState)
		{
			return ReturnStatus;
		}

		PlayerStates = GetPlayerStatesOfTeam(LocalPlayerState->GetTeam());
	}

	int32 RedCount = 1;
	int32 BlueCount = 1;

	for (int32 i = 0; i < PlayerStates.Num(); i++)
	{
		AReadyOrNotPlayerState* ps = PlayerStates[i];
		if (!ps)
			continue;

		FDeploymentStatus ds;
		ds.PlayerState = ps;
		if (ps->bIsInGame)
		{
			ds.Status = EPlayerStatus::PS_Deployed;
		}
		else
		{
			ds.Status = ps->bReady ? EPlayerStatus::PS_Ready : EPlayerStatus::PS_NotReady;
		}

		if (!ps->IsSquadLeader())
		{
			if (ps->GetTeam() == ETeamType::TT_SERT_RED)
			{
				ds.Position = FText::Format(NSLOCTEXT("ReadyOrNotGameState", "PositionRed", "Red {0}"), FText::AsNumber(RedCount));
				ReturnStatus.IsValidIndex(RedCount) ? ReturnStatus.Insert(ds, RedCount) : ReturnStatus.Add(ds);
				RedCount += 1;
			}
			else if (ps->GetTeam() == ETeamType::TT_SERT_BLUE)
			{
				ds.Position = FText::Format(NSLOCTEXT("ReadyOrNotGameState", "PositionBlue", "Blue {0}"), FText::AsNumber(BlueCount));
				ReturnStatus.Add(ds);
				BlueCount += 1;
			}
		}
		else
		{
			ds.Position = NSLOCTEXT("ReadyOrNotGameState", "PositionLeader", "Leader");
			ReturnStatus.Insert(ds, 0);
		}
	}
	return ReturnStatus;
}

void AReadyOrNotGameState::Client_BindCharacterEvents_Implementation(APlayerCharacter* Character)
{
	if (Character)
	{
		Character->OnCharacterKilled.AddDynamic(this, &AReadyOrNotGameState::PlayerKilled);
		Character->OnPlayerArrested.AddDynamic(this, &AReadyOrNotGameState::PlayerArrested);
	}
}

void AReadyOrNotGameState::PlayerKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter)
{
	// Kill Feed stuff.
	//GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, "Player Killed.");
	OnUpdateKillFeed.Broadcast(InstigatorCharacter, InstigatorCharacter, KilledCharacter);
}

void AReadyOrNotGameState::PlayerArrested(AReadyOrNotCharacter* ArrestedCharacter, AReadyOrNotCharacter* InstigatorCharacter)
{
	if (bPvPMode)
	{
		UFMODBlueprintStatics::PlayEvent2D(this, ReplenishAllAmmoSound, true);

		APlayerCharacter* InstigatorPlayerCharacter = Cast<APlayerCharacter>(InstigatorCharacter);
		if (InstigatorPlayerCharacter)
		{
			InstigatorPlayerCharacter->ReplenishAllMagazineAmmo();
			InstigatorPlayerCharacter->ReplenishAllGrenadeAmmo();
		}
	}
}

float AReadyOrNotGameState::GetWinningScore(bool& bUsesScoring)
{
	bUsesScoring = Scorelimit > 0;
	return Scorelimit;
}

float AReadyOrNotGameState::GetTeamScore(ETeamType Team)
{
	float teamPoints = 0.0f;
	TArray<AReadyOrNotPlayerState*> teamPlayerStates = GetPlayerStatesOfTeam(Team);
	for (int32 i = 0; i < teamPlayerStates.Num(); i++)
	{
		AReadyOrNotPlayerState* ps = teamPlayerStates[i];
		if (ps && ps->GetScore() > 0.0f)
		{
			teamPoints += ps->GetScore();
		}
	}
	return teamPoints;
}

TArray<AReadyOrNotPlayerState*> AReadyOrNotGameState::GetPlayerStatesOfTeam(ETeamType Team)
{
	TArray<AReadyOrNotPlayerState*> teamPlayerStates;
	for (TActorIterator<AReadyOrNotPlayerState> It(GetWorld()); It; ++It)
	{
		AReadyOrNotPlayerState* ps = *It;
		if (ps->bIsReplaySpectator || ps->IsSpectator() || ps->IsOnlyASpectator())
			continue;
		if (ps && ps->GetTeam() == Team && (ps->bIsInGame || bRunWarmup || MatchState == EMatchState::MS_Warmup))
		{
			teamPlayerStates.Add(ps);
		}
		else if (Team == ETeamType::TT_NONE)
		{
			teamPlayerStates.Add(ps);
		}
	}
	return teamPlayerStates;
}

void AReadyOrNotGameState::LoadStartupWidgetsAfterLoadingScreen()
{	
	FString StartupWidget = bPvPMode ? "MatchStart_PVP" : "MatchStart_COOP";
#if WITH_EDITOR
	if (CVarRonEnableLoadoutInEditor.GetValueOnAnyThread() == 0)
	{
		return;
	}
#endif
		
#if defined DMO_BUILD
	StartupWidget += "_DMO";
#endif
		
	StartupWidget = "PreMissionPlanning";
	for (TActorIterator<APlayerCharacter> It(GetWorld()); It; ++It)
	{
		APlayerCharacter* c = *It;
		if (c->Tags.Contains("PreviewCharacter"))
		{
			c->SetReplicates(false);
			c->TearOff();
		}
	}

	if (GetWorld()->GetMapName().Contains("MainMenu"))
	{
		StartupWidget = "Menu";
	}

	if (GetName().Contains("FreeMode"))
	{
		StartupWidget = "";
	}
	
	AReadyOrNotPlayerController* pc = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld());
	if (pc && !pc->bIsReplaySpectator)
	{
		FWidgetLookupData WidgetData = UBpGameplayHelperLib::GetWidgetDataFromLookupData(StartupWidget);
		if (WidgetData.WidgetClass)
		{
			if (Cast<ACoopGS>(this)&& MatchState == EMatchState::MS_Playing)
			{
			
			}
			else
			{
				UBpGameplayHelperLib::RemoveWidgetFromViewport("Escape");
				
				pc->Client_ClearHUDWidgets_Implementation();
                pc->Client_CreateWidget_Implementation(StartupWidget);
			}
		}
	}

}

void AReadyOrNotGameState::OnRep_WinsUpdated()
{
	bPendingWinsUpdate = true;
}

UReadyOrNotProfile* AReadyOrNotGameState::GetCurrentProfile()
{
	if (IsNetMode(ENetMode::NM_DedicatedServer))
	{
		return nullptr;
	}
	
	return UBpGameplayHelperLib::GetMultiplayerProfile();
}

int32 AReadyOrNotGameState::GetRemainingRounds()
{
	return (RoundsToPlay - RoundsPlayed) + 1;
}

bool AReadyOrNotGameState::IsEveryoneReady()
{
	for (TActorIterator<AReadyOrNotPlayerState> It(GetWorld()); It; ++It)
	{
		AReadyOrNotPlayerState* p = *It;
		if (!p->bReady)
			return false;
	}
	return true;
}

void AReadyOrNotGameState::Multicast_PlaySequence_Implementation(class ULevelSequence* Sequence)
{
	AReadyOrNotLevelScript* ls = Cast<AReadyOrNotLevelScript>(GetWorld()->GetLevelScriptActor());
	if (ls)
	{
		AReadyOrNotPlayerController* pc = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld());
		if (pc)
		{
			if (pc->PlayerCameraManager != nullptr)
			{
				FTimerHandle FadeInHandle;
				GetWorld()->GetTimerManager().SetTimer(FadeInHandle, FTimerDelegate::CreateUObject(pc->PlayerCameraManager.Get(), &APlayerCameraManager::StartCameraFade, 0.0f, 1.0f, 0.5f, FLinearColor::Black, false, true), 3.0f, false);
				FTimerHandle FadeOutHandle;
				GetWorld()->GetTimerManager().SetTimer(FadeOutHandle, FTimerDelegate::CreateUObject(pc->PlayerCameraManager.Get(), &APlayerCameraManager::StartCameraFade, 1.0f, 0.0f, 5.0f, FLinearColor::Black, false, true), 5.0f, false);
			}
			APlayerCharacter* character = Cast<APlayerCharacter>(pc->GetPawn());
			if (character)
			{
				character->bFadeToGray = true;
				InterpToTimeDialtion = 0.2f;
			}
		}
		FTimerHandle PlayMVPSequenceHandle;
		GetWorld()->GetTimerManager().SetTimer(PlayMVPSequenceHandle, ls, &AReadyOrNotLevelScript::PlayMVPSequence, 3.5f, false);
		EndPlayTimer += ls->LevelSequenceMVP->GetMovieScene()->GetPlaybackRange().GetUpperBoundValue() / ls->LevelSequenceMVP->GetMovieScene()->GetTickResolution();
	}
}

void AReadyOrNotGameState::Multicast_StopSequence_Implementation(class ULevelSequence* Sequence)
{

	AReadyOrNotLevelScript* ls = Cast<AReadyOrNotLevelScript>(GetWorld()->GetLevelScriptActor());
	if (ls)
	{
		// TODO: add support for more types of sequences
		ls->StopMVPSequence();
	}
}

void AReadyOrNotGameState::SkipMVPScreen()
{
	AReadyOrNotLevelScript* ls = Cast<AReadyOrNotLevelScript>(GetWorld()->GetLevelScriptActor());
	if (ls)
	{
		AReadyOrNotPlayerController* pc = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld());
		if (pc)
		{
			GetWorld()->GetTimerManager().SetTimer(ls->FadeToBlackAfterMVP, FTimerDelegate::CreateUObject(pc->PlayerCameraManager.Get(), &APlayerCameraManager::StartCameraFade, 0.0f, 1.0f, 0.5f, FLinearColor::Black, false, true), 0.01f, false);
			GetWorld()->GetTimerManager().SetTimer(ls->FadeFromBlackAfterMVP, FTimerDelegate::CreateUObject(pc->PlayerCameraManager.Get(), &APlayerCameraManager::StartCameraFade, 1.0f, 0.0f, 5.0f, FLinearColor::Black, false, true), 2.0f, false);
		}
		FTimerHandle SkipMVPHandle;
		GetWorld()->GetTimerManager().SetTimer(SkipMVPHandle, FTimerDelegate::CreateUObject(ls, &AReadyOrNotLevelScript::OnMVPSequenceFinished), 1.0f, false);
	}
}

void AReadyOrNotGameState::GetNextMapMode(FString& Map, FString& Mode)
{
	if (!NextURLReplicated.IsEmpty())
	{
		NextURLReplicated.Split("?game=", &Map, &Mode, ESearchCase::IgnoreCase);
		FLevelDataLookupTable ld = UBpGameplayHelperLib::GetMapDetailsFromName(Map);
		Map = ld.FriendlyLevelName.ToString();

	}
	else 
	{
		FLevelDataLookupTable ld = UBpGameplayHelperLib::GetMapDetailsFromName(UGameplayStatics::GetCurrentLevelName(GetWorld(), true));
		Map = ld.FriendlyLevelName.ToString();
		Mode = ModeURL_Replicated;
	}
}

void AReadyOrNotGameState::Multicast_BroadcastChatMessage_Implementation(FRChatMessage ChatMessage)
{

	V_LOGM(LogReadyOrNotChat, "(%s) %s : %s", *ChatMessage.TimeStamp.ToString(),*ChatMessage.SenderName, *ChatMessage.Message);
	APlayerController* controller = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld());

	if (ChatMessage.TargetPlayerController)
	{
		if (ChatMessage.TargetPlayerController != controller)
			return;
	}
	else if (!bFreeForAll && ChatMessage.TargetTeam != ETeamType::TT_NONE && ChatMessage.TargetTeam != ETeamType::TT_SQUAD)
	{
		AReadyOrNotPlayerState* ps = Cast<AReadyOrNotPlayerState>(controller->PlayerState);
		if (ps)
		{
			if (!ps->IsSquadLeader() && ps->GetTeam() != ChatMessage.TargetTeam)
			{
				return;
			}
		}
	}
	else if (ChatMessage.SenderName == "SYSTEM")
	{
		if (ChatMessage.TargetPlayerController && ChatMessage.TargetPlayerController != controller)
		{
			return;
		}
	}

	AReadyOrNotPlayerController* ReadyOrNotPlayerController = UBpGameplayHelperLib::GetLocalRoNPlayerController(GetWorld());
	if (ReadyOrNotPlayerController)
	{
		ReadyOrNotPlayerController->SaveChatMessage(ChatMessage);
	}
	SavedChatMessages.Add(ChatMessage);
	
	OnChatMessageReceived.Broadcast(ChatMessage);
}

void AReadyOrNotGameState::OnRep_DrawPointDataChanged()
{
	TArray<UUserWidget*> OutWidgets;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(GetWorld(), OutWidgets, UPlanningMapWidget::StaticClass(), false);

	for (UUserWidget* widget : OutWidgets)
	{
		UPlanningMapWidget* mapWidget = Cast<UPlanningMapWidget>(widget);
		if (mapWidget)
		{
			mapWidget->DrawPointData = DrawingPointData;
		}
	}
}

void AReadyOrNotGameState::SetTimeDilationSynced(float TimeDilation)
{
	if (GetLocalRole() < ROLE_Authority)
		return;

	CustomTimeDilationApplied = FMath::Clamp(TimeDilation, 0.01f, 1.0f);
	OnRep_CustomTimeDilation();
}

void AReadyOrNotGameState::OnRep_CustomTimeDilation()
{
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), CustomTimeDilationApplied);
}

bool AReadyOrNotGameState::IsAdminPlayerController(APlayerController* PlayerController)
{
#if UE_SERVER
	return AdminPlayerControllers.Contains(PlayerController);
#endif
	// local player controller always admin
	if (PlayerController->GetLocalRole() >= ROLE_Authority && UBpGameplayHelperLib::GetLocalPlayerController(GetWorld()) == PlayerController)
		return true;
	else
		return AdminPlayerControllers.Contains(PlayerController);
}

void AReadyOrNotGameState::OnPostBugReportResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (!Response.IsValid())
		return;

	FString ResponseText = Response->GetContentAsString();

	V_LOGM(LogReadyOrNot, "%s", *ResponseText);
}

void AReadyOrNotGameState::Multicast_OnRoundReset_Implementation()
{
	RoundTimeElapsed = 0.0f;
}

void AReadyOrNotGameState::HideLevelEffects()
{
	if (!bHideLevelEffectsInPreMission)
		return;
	
	bool bAnyEffectsHidden = false;
	
	// Unbound post processing volumes
	if (WorldPostProcessVolumes.Num() <= 0)
	{
		for (TActorIterator<APostProcessVolume> It(GetWorld()); It; ++It)
		{
			APostProcessVolume* PostProcessVolume = *It;
			if (!IsValid(PostProcessVolume))
				continue;

			if (PostProcessVolume->bUnbound)
			{
				PostProcessVolume->bUnbound = false;
				WorldPostProcessVolumes.Add(PostProcessVolume);

				bAnyEffectsHidden = true;
				UE_LOG(LogReadyOrNot, Verbose, TEXT("Temporarily hiding PostProcessVolume %s"), *GetNameSafe(PostProcessVolume));
			}
		}
	}

	// Exponential height fogs
	if (WorldExponentialHeightFogs.Num() <= 0)
	{
		for (TActorIterator<AExponentialHeightFog> It(GetWorld()); It; ++It)
		{
			AExponentialHeightFog* ExponentialHeightFog = *It;
			if (!IsValid(ExponentialHeightFog))
				continue;

			if (ExponentialHeightFog->ActorHasTag("PreMissionFog"))
				continue;
			
			if (ExponentialHeightFog->GetComponent()->IsVisible())
			{
				ExponentialHeightFog->GetComponent()->SetVisibility(false);
				WorldExponentialHeightFogs.Add(ExponentialHeightFog);

				bAnyEffectsHidden = true;
				UE_LOG(LogReadyOrNot, Verbose, TEXT("Temporarily hiding ExponentialHeightFog %s"), *GetNameSafe(ExponentialHeightFog));
			}
		}
	}
	
	if (bAnyEffectsHidden)
	{
		int32 PostProcessVolumesNum = WorldPostProcessVolumes.Num();
		int32 ExponentialHeightFogsNum = WorldExponentialHeightFogs.Num();
		UE_LOG(LogReadyOrNot, Log, TEXT("Level effects hidden (%d PostProcessVolumes) (%d ExponentialHeightFogs)"),
			PostProcessVolumesNum, ExponentialHeightFogsNum);
	}
}

void AReadyOrNotGameState::RestoreLevelEffects()
{
	if (!bHideLevelEffectsInPreMission)
		return;
	
	int32 PostProcessVolumesNum = WorldPostProcessVolumes.Num();
	int32 ExponentialHeightFogsNum = WorldExponentialHeightFogs.Num();

	if (PostProcessVolumesNum > 0 || ExponentialHeightFogsNum > 0)
	{
		UE_LOG(LogReadyOrNot, Log, TEXT("Restoring level effects (%d PostProcessVolumes) (%d ExponentialHeightFogs)"),
			PostProcessVolumesNum, ExponentialHeightFogsNum);
	}
	
	// Unbound post processing volumes
	if (WorldPostProcessVolumes.Num() > 0)
	{
		for (APostProcessVolume* PostProcessVolume : WorldPostProcessVolumes)
		{
			if (!IsValid(PostProcessVolume))
				continue;
				
			PostProcessVolume->bUnbound = true;
			UE_LOG(LogReadyOrNot, Verbose, TEXT("Restoring unbound PostProcessVolume %s"), *GetNameSafe(PostProcessVolume));
		}
		WorldPostProcessVolumes.Empty();
	}

	// Exponential height fogs
	if (WorldExponentialHeightFogs.Num() > 0)
	{
		for (AExponentialHeightFog* ExponentialHeightFog : WorldExponentialHeightFogs)
		{
			if (!IsValid(ExponentialHeightFog))
				continue;

			ExponentialHeightFog->GetComponent()->SetVisibility(true);
			UE_LOG(LogReadyOrNot, Verbose, TEXT("Restoring ExponentialHeightFog %s"), *GetNameSafe(ExponentialHeightFog));
		}
		WorldExponentialHeightFogs.Empty();
	}

	// Hide premission fog
	for (TActorIterator<AExponentialHeightFog> It(GetWorld()); It; ++It)
	{
		AExponentialHeightFog* ExponentialHeightFog = *It;
		if (!IsValid(ExponentialHeightFog))
			continue;

		if (ExponentialHeightFog->ActorHasTag("PreMissionFog"))
		{
			ExponentialHeightFog->GetComponent()->SetVisibility(false);
		}
	}
}

void AReadyOrNotGameState::UpdateDoorTickIntervals()
{
	AReadyOrNotPlayerController* pc = GetGameInstance() ? Cast<AReadyOrNotPlayerController>(GetGameInstance()->GetFirstLocalPlayerController()) : nullptr;
	if (!IsValid(pc))
		return;

	// AActor cause we want to update door freq even if we're spectating
	AActor* LocalPlayer = Cast<AActor>(pc->GetPawn());
	
	const bool bIsDemoNetDriverPlaying = GetWorld()->GetDemoNetDriver() && GetWorld()->GetDemoNetDriver()->IsPlaying();
	if (!bIsDemoNetDriverPlaying && !GIsAutomationTesting && IsValid(LocalPlayer) && !Cast<ASWATCharacter>(LocalPlayer))
	{
		// Server Needs to tick doors based on closest player, as server replicates the door state, and needs to rep quicker for close clients
		TArray<FVector> PlayerLocations;
		if (HasAuthority())
		{
			for (AReadyOrNotCharacter* Character : AllReadyOrNotCharacters)
			{
				if (!IsValid(Character) || !Character->IsOnSWATTeam())
					continue;

				PlayerLocations.Emplace(Character->GetActorLocation());
			}
		}

		// Client (And a just-in-case for server)
		if (!PlayerLocations.Num())
			PlayerLocations.Emplace(LocalPlayer->GetActorLocation());
			
		for (ADoor* Door : AllDoors)
		{
			if (!IsValid(Door))
				continue;
				
			//Default to Upper end of range (10000^2)
			float ClosestPlayerDistanceSq = 100000000;
			for (FVector PlayerLocation : PlayerLocations)
			{
				float DistSquared = FVector::DistSquared(PlayerLocation, Door->GetActorLocation());
				if (DistSquared < ClosestPlayerDistanceSq)
					ClosestPlayerDistanceSq = DistSquared;
			}
				
			const float Dist = FMath::Sqrt(ClosestPlayerDistanceSq);
			float NewTickRate = FMath::GetMappedRangeValueClamped<float,float>({2000.0f, 10000.0f}, {0.0f, 0.5f}, Dist);
			Door->PrimaryActorTick.TickInterval = NewTickRate;
		}
	}
	else
	{
		for (ADoor* Door : AllDoors) // force tick every frame, to simulate the player being close to the door when automation testing or in replay viewer
		{
			if (!IsValid(Door))
				continue;
				
			Door->PrimaryActorTick.TickInterval = 0.0f;
		}
	}
}


