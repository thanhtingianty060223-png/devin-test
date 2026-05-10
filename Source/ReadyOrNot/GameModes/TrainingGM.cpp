// Copyright Void Interactive, 2023


#include "GameModes/TrainingGM.h"
#include "ReadyOrNotAIConfig.h"
#include "ReadyOrNotGameSession.h"
#include "Actors/Environment/ActivityTriggerVolume.h"
#include "Actors/Environment/CheckpointActivityTriggerVolume.h"
#include "Commander/CommanderProfile.h"
#include "Commander/MetaGameProfile.h"
#include "Data/ActivityData.h"
#include "Info/SWATManager.h"
#include "Info/TOCManager.h"
#include "HUD/Widgets/SwatCommandWidget.h"
#include "Info/ScoringManager.h"
#include "Navigation/ReadyOrNotNavAreas.h"

TAutoConsoleVariable<int32> CVarRonEndTrainingInEditor(TEXT("a.RonEndTrainingInEditor"), 0, TEXT("Can training end in editor?"));

ATrainingGM::ATrainingGM()
{
	bInitialPlayerRespawn = true;
	bTimelimitUsedInMode = false;
	RespawnMode = ERespawnMode::ImmediateRespawn;
	bIsExfilEnabled = true;
}

void ATrainingGM::BeginPlay()
{
	Super::BeginPlay();

	// Destroy the Scoring Manager
	if (AScoringManager* ScoringManager = AScoringManager::Get())
		ScoringManager->Destroy();

	// Destroy the TOC Manager
	if (ATOCManager* TOCManager = ATOCManager::Get())
		TOCManager->Destroy();
}

void ATrainingGM::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void ATrainingGM::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Hack to disable reporting of AI in training
	for (TActorIterator<ACyberneticCharacter> It(GetWorld()); It; ++It)
	{
		ACyberneticCharacter* AICharacter = *It;
		if (!AICharacter->bHasBeenReported)
		{
			AICharacter->bHasBeenReported = true;
		}
	}
}

void ATrainingGM::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	// Setup variables for Commander Profile
	if (UGameplayStatics::HasOption(Options, "save"))
	{
		const FString CommanderSaveSlot = UGameplayStatics::ParseOption(Options, "save");
		CommanderProfile = UCommanderProfile::LoadProfile(CommanderSaveSlot);
	}
}

void ATrainingGM::StartMatch()
{
	Super::StartMatch();
}

void ATrainingGM::ResetLevel()
{
	Super::ResetLevel();

	// Remove all AI so they can be respawned by triggers
	RemoveAllSpawnedAI();
}

void ATrainingGM::RespawnPlayer(APlayerController* Player, bool bForceSpectator)
{
	AReadyOrNotPlayerController* PlayerController = Cast<AReadyOrNotPlayerController>(Player);
	if (!PlayerController)
		return;

	AReadyOrNotPlayerState* PlayerState = Cast<AReadyOrNotPlayerState>(PlayerController->PlayerState);
	if (!PlayerState)
		return;

	PlayerState->TrySetPendingTeamAsTeam();

	FSavedLoadout Loadout;
	UBpGameplayHelperLib::LoadDefaultLoadout(Loadout, "training");
	PlayerState->Server_SetLoadout(Loadout);

	if (CurrentCheckpoint)
	{
		if (PlayerState->bIsInGame && GetMatchState() == EMatchState::MS_Playing)
		{
			if (PlayerState->GetTeam() == ETeamType::TT_SERT_BLUE)
			{
				SpawnPlayerCharacter(PlayerController, BlueCharacterClass.LoadSynchronous(), CurrentCheckpoint->GetActorTransform());
				return;
			}

			if (PlayerState->GetTeam() == ETeamType::TT_SERT_RED)
			{
				SpawnPlayerCharacter(PlayerController, RedCharacterClass.LoadSynchronous(), CurrentCheckpoint->GetActorTransform());
				return;
			}
		}
	}

	Super::RespawnPlayer(Player, bForceSpectator);
}

void ATrainingGM::PlayerKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter)
{
	// Get the player controller first as the player will be un-possessed during super call
	APlayerController* PlayerController = KilledCharacter->GetController<APlayerController>();

	Super::PlayerKilled(InstigatorCharacter, KilledCharacter);

	ResetLevel();

	// Deactivate all active trigger volumes
	for (int32 i = ActiveTriggerVolumes.Num()-1; i >= 0; i--)
	{
		if (AActivityTriggerVolume* TriggerVolume = ActiveTriggerVolumes[i])
		{
			TriggerVolume->Deactivate();
		}
	}

	if (CurrentCheckpoint)
	{
		float RespawnTime = 5.0f;
		if (const AReadyOrNotGameSession* Session = Cast<AReadyOrNotGameSession>(GameSession))
			RespawnTime = Session->RespawnTimer;

		UReadyOrNotFunctionLibrary::StartTimerForCallback(this, FTimerDelegate::CreateUObject(CurrentCheckpoint, &AActivityTriggerVolume::Activate), RespawnTime);
		UReadyOrNotFunctionLibrary::StartTimerForCallback(this, FTimerDelegate::CreateUObject(this, &ATrainingGM::RespawnPlayer, PlayerController, false), RespawnTime);
	}
}

void ATrainingGM::SpawnPolice(const bool bSpawnWithPlayer)
{
	// If there are already AI spawned, don't spawn more
	for (TActorIterator<ASWATCharacter> It(GetWorld()); It; ++It)
		return;

	APlayerCharacter* LocalPlayer = UBpGameplayHelperLib::GetLocalPlayerCharacter(GetWorld());
	#if WITH_EDITOR
	ensureAlwaysMsgf(LocalPlayer, TEXT("Local player is null"));
	#endif
	if (!LocalPlayer)
		return;

	SpawnedSWATAI.Empty(4);

	const FTransform SpawnPosition = GetAIOfficerSpawnTransform(bSpawnWithPlayer);

	SpawnAIOfficer(ESquadPosition::SP_Alpha, ETeamType::TT_SERT_BLUE, "defaultblueone", SpawnPosition);
	SpawnAIOfficer(ESquadPosition::SP_Beta, ETeamType::TT_SERT_BLUE, "defaultbluetwo", SpawnPosition);
	SpawnAIOfficer(ESquadPosition::SP_Charlie, ETeamType::TT_SERT_RED, "defaultredone", SpawnPosition);
	SpawnAIOfficer(ESquadPosition::SP_Delta, ETeamType::TT_SERT_RED, "defaultredtwo", SpawnPosition);

	if (bSpawnWithPlayer)
	{
		AdjustAIOfficerSpawnLocation();
	}

	if (USWATManager* SwatManager = USWATManager::Get(this))
	{
		SwatManager->SwatAI = SpawnedSWATAI;
		SwatManager->SquadLeader = LocalPlayer;
		SwatManager->OriginalSpawnLocation = LocalPlayer->GetActorLocation();

		#if WITH_EDITOR
		ensureAlways(SwatManager->SquadLeader != nullptr);
		#endif
		
		for (ASWATCharacter* Swat : SpawnedSWATAI)
		{
			Swat->GetCyberneticsController()->GetTargetingComp()->AddKnownFriendly(SwatManager->SquadLeader);

			for (ASWATCharacter* OtherSwat : SpawnedSWATAI)
			{
				if (Swat != OtherSwat)
				{
					Swat->GetCyberneticsController()->GetTargetingComp()->AddKnownFriendly(OtherSwat);
				}
			}
		}

		// If not spawning with the player, force the SWAT to not fall in on the player
		SwatManager->bGivenInitialFallInCommand = !bSpawnWithPlayer;
	}

	ResetSquadLeader();

	// Reset the active team type on the swat command widget
	if (LocalPlayer->SwatCommandWidget)
		LocalPlayer->SwatCommandWidget->SetActiveTeamElement(ETeamType::TT_SQUAD);

	// Recreate the Player's HUD to ensure SWAT elements are functional
	LocalPlayer->CreateHUDWidget();

	// Reset the SWAT Manager to ensure no invalid pointers are stored
	if (USWATManager* SwatManager = USWATManager::Get(this))
	{
		SwatManager->SetupSwatManager();
	}
}

void ATrainingGM::RemoveAllSpawnedAI()
{
	for (TActorIterator<ACyberneticCharacter> It(GetWorld()); It; ++It)
	{
		ACyberneticCharacter* AICharacter = *It;
		if (AICharacter)
		{
			AICharacter->Destroy();
		}
	}
}

void ATrainingGM::ExfiltrateMission(TArray<ASWATCharacter*> ExfilCharacters)
{
	Super::ExfiltrateMission(ExfilCharacters);
	StartTrainingEndTimer(true);
}

void ATrainingGM::StartTrainingEndTimer(const bool bWon)
{
#if WITH_EDITOR
	if (CVarRonEndTrainingInEditor.GetValueOnAnyThread() == 0)
		return;
#endif

	if (!UReadyOrNotFunctionLibrary::IsCallbackTimerActive(this, TrainingEndTimer))
	{
		if (ATOCManager* TOC = ATOCManager::Get())
		{
			TOC->StartTOCResponse(bWon ? VO_TOC::TOC_MISSION_COMPLETION : VO_TOC::TOC_MISSION_FAILED, true, ETOCPriority::ETP_Flush);
		}

		// TODO: implement GameState OnMissionEnd logic similar to the one in ACoopGS::Multicast_OnMissionEnd
	
		UReadyOrNotFunctionLibrary::StartTimerForCallback(TrainingEndTimer, this, FTimerDelegate::CreateUObject(this, &ATrainingGM::TrainingEnd, bWon), 5.0f, false);
	}
}

void ATrainingGM::TrainingEnd(const bool bSuccess)
{
	if (GetMatchState() != EMatchState::MS_Playing)
		return;

	SetMatchState(EMatchState::MS_MatchEnded);

	OnTrainingEnded.Broadcast(bSuccess);

	AReadyOrNotPlayerController* PlayerController = Cast<AReadyOrNotPlayerController>(GetWorld()->GetFirstPlayerController());
	if (!PlayerController)
		return;

	UMetaGameProfile* MetaGameProfile = UMetaGameProfile::GetProfile(GetWorld());
	if (ensure(MetaGameProfile))
	{
		MetaGameProfile->bHasCompletedTutorial = true;
		MetaGameProfile->SaveProfile();
	}

	if (CommanderProfile)
	{
		// Load the lobby level
		const FString MapName = GetGameInstance<UReadyOrNotGameInstance>()->LobbyLevel;
		ULevelStreaming* StreamedLevel;
		FLevelStreamOptions LevelStreamOptions;
		LevelStreamOptions.bStreamInLevelBeforeLoad = true;
		PlayerController->StreamInLevel(MapName, "", StreamedLevel, LevelStreamOptions);
	}
	else
	{
		PlayerController->BP_ReturnToMenu(FText::FromString("Training Completed!"));
	}
}

TArray<FSwatCommandData> ATrainingGM::GetCurrentCommandsToIssue()
{
	TArray<FSwatCommandData> CommandsToIssue;

	/**
	 * Build array of current commands to issue by getting all volumes that include
	 * active and incomplete "Issue SWAT Command" activities, then adding the activity's
	 * SWAT command data to the array.
	 */
	for (const AActivityTriggerVolume* TriggerVolume : ActiveTriggerVolumes)
	{
		if (!TriggerVolume)
			continue;

		for (const UActivityData* Activity : TriggerVolume->GetActivities())
		{
			if (!Activity || Activity->IsComplete())
				continue;

			// If the activity is an "Arrest or Kill" activity, add the arrest command
			if (Activity->Activity == EActivity::A_ArrestOrKillAi)
			{
				FSwatCommandData ArrestCommand;
				ArrestCommand.Command = ESwatCommand::SC_DoArrestTarget;
				CommandsToIssue.Add(ArrestCommand);
			}

			// If the activity is a "Secure Evidence" activity, add the collect evidence command
			if (Activity->Activity == EActivity::A_SecureEvidence)
			{
				FSwatCommandData SecureEvidenceCommand;
				SecureEvidenceCommand.Command = ESwatCommand::SC_DoCollectEvidence;
				CommandsToIssue.Add(SecureEvidenceCommand);
			}

			if (Activity->Activity != EActivity::A_IssueSwatCommand)
				continue;

			CommandsToIssue.Add(Activity->SwatCommandData);
		}
	}

	return CommandsToIssue;
}

void ATrainingGM::SpawnAIOfficer(const ESquadPosition SquadPosition, const ETeamType CommandTeam, const FString& LoadoutName, const FTransform& SpawnTransform)
{
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.bNoFail = true;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// Spawn the AI officer
	if (ASWATCharacter* Officer = GetWorld()->SpawnActor<ASWATCharacter>(SWATAIClass, SpawnTransform, SpawnParameters))
	{
		Officer->AIControllerClass = FriendlyAIController;
		Officer->SetSquadPosition(SquadPosition);
		Officer->SetDefaultTeam(CommandTeam);

		switch (SquadPosition)
		{
			case ESquadPosition::SP_Alpha:		Officer->GetCapsuleComponent()->AreaClass = UNavArea_SwatAlpha::StaticClass(); break;
			case ESquadPosition::SP_Beta:		Officer->GetCapsuleComponent()->AreaClass = UNavArea_SwatBeta::StaticClass();break;
			case ESquadPosition::SP_Charlie:	Officer->GetCapsuleComponent()->AreaClass = UNavArea_SwatCharlie::StaticClass();break;
			case ESquadPosition::SP_Delta:		Officer->GetCapsuleComponent()->AreaClass = UNavArea_SwatDelta::StaticClass(); break;
			default:							Officer->GetCapsuleComponent()->AreaClass = UNavArea_SwatAlpha::StaticClass();
		}

		Officer->GetCapsuleComponent()->bDynamicObstacle = false;
		Officer->GetCharacterMovement()->GetNavAgentPropertiesRef().AgentRadius = 20.0f;
		Officer->GetCapsuleComponent()->SetCanEverAffectNavigation(false);

		FSavedLoadout Loadout;
		UBpGameplayHelperLib::LoadDefaultLoadout(Loadout, "training");
		UBpGameplayHelperLib::EquipLoadoutOnPlayer(Loadout, Officer, FLoadoutEquipOptions());

		UBpGameplayHelperLib::AddDefaultItemsToPlayer(Officer);

		FSavedCustomization Customization = FSavedCustomization();
		SetupOfficerCustomization(Officer, Customization);
		Customization.Sanitize();
		
		Officer->Customization = Customization;
		Customization.ApplyCustomization(Officer);
		Customization.ApplyCustomizationSkins(Officer);

		Officer->GetHealthComponent()->SetMaxResource(AI_CONFIG_GET_FLOAT("SwatHealth", 250.0f));
		Officer->GetHealthComponent()->SetCurrentResourceToMax();

		Officer->SpawnDefaultController();

		Officer->OnCharacterKilled.AddDynamic(this, &ATrainingGM::FriendlyAIKilled);

		SpawnedSWATAI.Add(Officer);
	}
}

void ATrainingGM::SetupOfficerCustomization(ASWATCharacter* Character, FSavedCustomization& OutCustomization)
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
			if (!OutCustomization.ArmorSkin)
				OutCustomization.ArmorSkin = CharacterCustomization->ArmorSkin;
		}
	}

	// TMap<ESquadPosition, FString> Names;
	// Names.Add(ESquadPosition::SP_Alpha, "SWAT_King");
	// Names.Add(ESquadPosition::SP_Beta, "SWAT_Swan");
	// Names.Add(ESquadPosition::SP_Charlie, "SWAT_Prescott");
	// Names.Add(ESquadPosition::SP_Delta, "SWAT_Eli");
	// Names.Add(ESquadPosition::SP_NONE, "");
	
	// Character->UpdateOverridesFromCharacterLookOverrideDataTable(Names[Character->GetSquadPosition()]);
}

void ATrainingGM::ResetSquadLeader()
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

FTransform ATrainingGM::GetAIOfficerSpawnTransform(const bool bSpawnWithPlayer)
{
	if (bSpawnWithPlayer)
	{
		if (const APlayerCharacter* LocalPlayer = UBpGameplayHelperLib::GetLocalPlayerCharacter(GetWorld()))
		{
			// Just spawn them next to the player
			return LocalPlayer->GetActorTransform();
		}
		ensureMsgf(false, TEXT("Local player is null but SWAT should be spawning with player!"));
	}

	// Find the AI spawn point
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		const APlayerStart* CurrentStart = *It;
		if (CurrentStart->PlayerStartTag == FName(SwatSpawnTag))
		{
			return CurrentStart->GetActorTransform();
		}
	}

	ensureMsgf(false, TEXT("Must have a player start with the tag %s"), *SwatSpawnTag);

	// If we can't find a player start, attempt to spawn them at the player's PIE location
	for (TActorIterator<APlayerStartPIE> It(GetWorld()); It; ++It)
	{
		return It->GetActorTransform();
	}

	return FTransform();
}

void ATrainingGM::AdjustAIOfficerSpawnLocation()
{
	APlayerCharacter* LocalPlayer = UBpGameplayHelperLib::GetLocalPlayerCharacter(GetWorld());
	if (!LocalPlayer)
		return;

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

void ATrainingGM::FriendlyAIKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter)
{
	if (USWATManager* SwatManager = USWATManager::Get(this))
	{
		SwatManager->RespondToPlayerTeamKill(InstigatorCharacter);
	}
}
