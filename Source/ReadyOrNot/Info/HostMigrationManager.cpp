// Void Interactive, 2020


#include "Info/HostMigrationManager.h"

#include "CreateSessionCallbackProxyAdvanced.h"
#include "DestroySessionCallbackProxyAdvanced.h"
#include "FindSessionsCallbackProxyAdvanced.h"
#include "Actors/Door.h"
#include "Actors/Gameplay/CollectedEvidenceActor.h"
#include "Actors/Gameplay/EvidenceActor.h"
#include "Actors/Gameplay/TrapActorAttachedToDoor.h"
#include "Characters/CyberneticCharacter.h"
#include "Components/DestructibleDoorChunkComponent.h"


UHostMigrationManager::UHostMigrationManager()
{
}

void UHostMigrationManager::Init()
{
	//UReadyOrNotStatics::GetReadyOrNotGameInstance()->GetTimerManager().SetTimer(TH_SaveState, this, &UHostMigrationManager::SaveState, 5.0f, true);
}

void UHostMigrationManager::SaveState()
{
	if (!GetWorld() || IsMigratingHost())
		return;

	V_LOGM(LogReadyOrNot, "Host Migration: Saving State");


	PlayerInformations.Empty();
	for (TActorIterator<APlayerCharacter>It(GetWorld()); It; ++It)
	{
		APlayerCharacter* Pc = *It;
		if (!Pc->LastKnownPlayerState)
			continue;
		FHm_PlayerInformation PlayerInformation = FHm_PlayerInformation();
		PlayerInformation.UniqueId = Pc->LastKnownPlayerState->GetUniqueId().ToString();
		PlayerInformation.CharacterTransform = Pc->GetActorTransform();
		PlayerInformation.ControlRotation = Pc->ReplicatedControlRotation;
		PlayerInformation.Health = Pc->GetHealthComponent()->GetCurrentResource();
		PlayerInformation.bHasBeenReported = Pc->HasBeenReported();
		if (Pc->GetEquippedItem())
		{
			PlayerInformation.EquippedItemClass = Pc->GetEquippedItem()->GetClass(); 
		}
		
		for (ABaseItem* i : Pc->GetInventoryComponent()->GetInventoryItems())
		{
			if (!i)
				continue;

			FHm_InventoryInformation Item = FHm_InventoryInformation();
			Item.Class = i->GetClass();
			ABaseMagazineWeapon* Bmw = Cast<ABaseMagazineWeapon>(i);
			if (Bmw)
			{
				Item.Magazines = Bmw->Magazines;
				Item.MagIndex = Bmw->MagIndex;
			}
			if (i->ContainsItemCategory(EItemCategory::IC_Grenade))
			{
				PlayerInformation.TotalGrenades++;
			}
			if (i->ContainsItemCategory(EItemCategory::IC_TacticalDevice))
			{
				PlayerInformation.TotalDevices++;
			}
			
			PlayerInformation.Inventory.Add(Item);
		}
		PlayerInformations.Add(PlayerInformation);	
	}

	CyberneticsInformations.Empty();
	for (TActorIterator<ACyberneticCharacter>It(GetWorld()); It; ++It)
	{
		ACyberneticCharacter* Cc = *It;
		FHm_CyberneticsInformation CyberneticsInformation = FHm_CyberneticsInformation();
		CyberneticsInformation.CharacterTransform = It->GetActorTransform();
		CyberneticsInformation.CharacterMeshData = It->CharacterMeshData;
		CyberneticsInformation.TeamType = It->GetTeam();
		CyberneticsInformation.Health = It->GetCurrentHealth();
		CyberneticsInformation.bIsArrested = It->IsArrested();
		CyberneticsInformation.bIsSurrendered = It->IsSurrendered();
		CyberneticsInformation.bHasBeenReported = It->HasBeenReported();
		CyberneticsInformation.Tags = It->Tags;
		if (It->GetEquippedItem())
		{
			CyberneticsInformation.EquippedItemClass = It->GetEquippedItem()->GetClass();
		}
		CyberneticsInformations.Add(CyberneticsInformation);
	}

	DoorInformations.Empty();
	for (TActorIterator<ADoor>It(GetWorld()); It; ++It)
	{
		ADoor* Door = *It;
		FHm_DoorInformation DoorInformation = FHm_DoorInformation();
		DoorInformation.Name = Door->GetName();
		DoorInformation.bIsBroken = Door->IsDoorBroken();
		DoorInformation.OpenCloseAmount = Door->GetOpenAmount();
		DoorInformation.bIsSimulatingPhysics = Door->GetDoorMesh()->IsSimulatingPhysics();
		DoorInformation.DoorMeshTransform = Door->GetDoorMesh()->GetComponentTransform();
		for (int32 i = 0; i < 9; i++)
		{
			FHm_DoorChunkInformation ChunkInformation;
			if (Door->GetChunkComponents().IsValidIndex(i))
			{
				ChunkInformation.Transform = Door->GetChunkComponents()[i]->GetComponentTransform();
				ChunkInformation.bIsSimulating = Door->GetChunkComponents()[i]->IsSimulatingPhysics();
				DoorInformation.DoorChunkInformations.Add(ChunkInformation);
			}
		}
		if (Door->GetAttachedTrap() && Door->GetAttachedTrap()->TrapStatus == ETrapState::TS_Live)
		{
			DoorInformation.TrapName = Door->GetTypeOfTrapRowName();
		}
		DoorInformations.Add(DoorInformation);
	}

	BombInformations.Empty();
	for (TActorIterator<ABombActor>It(GetWorld()); It; ++It)
	{
		ABombActor* Bomb = *It;
		FHm_BombInformation BombInfo;
		BombInfo.BombName = Bomb->GetName();
		BombInfo.TimeRemaining = Bomb->GetTimeUntilExplodes();
		BombInfo.BombState = Bomb->GetBombState();
		BombInformations.Add(BombInfo);
	}

	ActiveEvidence.Empty();
	for (TActorIterator<AEvidenceActor>It(GetWorld()); It; ++It)
	{
		ActiveEvidence.Add(It->GetName());
	}

	BaggedEvidenceInformations.Empty();
	for (TActorIterator<ACollectedEvidenceActor>It(GetWorld()); It; ++It)
	{
		FHm_BaggedEvidence BaggedEvidence;
		BaggedEvidence.Transform = It->GetActorTransform();
		BaggedEvidenceInformations.Add(BaggedEvidence);
	}

	DroppedEvidenceInformations.Empty();
	for (TActorIterator<ABaseItem>It(GetWorld()); It; ++It)
	{
		FHm_DroppedEvidence DroppedEvidence;
		if (It->IsEvidence())
		{
			DroppedEvidence.Transform = It->GetItemMesh()->GetComponentTransform();
			DroppedEvidence.Class = It->GetClass();
			DroppedEvidenceInformations.Add(DroppedEvidence);
		}

	}

	ObjectiveInformations.Empty();
	for (TActorIterator<AObjective>It(GetWorld()); It; ++It)
	{
		FHm_Objectives Objectives;
		Objectives.Name = It->ObjectiveName.ToString();
		Objectives.ObjectiveStatus = It->GetObjectiveStatus();
		ObjectiveInformations.Add(Objectives);
	}
	
}

bool UHostMigrationManager::IsNextHost(AReadyOrNotPlayerController* PlayerController)
{
	if (!PlayerController)
		return false;

	if (!NextHost)
	{
		ReturnToMainMenu();
		return false;
	}

	return NextHost == PlayerController->PlayerState;
}

bool UHostMigrationManager::IsMigratingHost()
{
	return bHostMigrationInProgress;
}

void UHostMigrationManager::SetHostMigrationInProgress(bool bInProgress)
{
	bHostMigrationInProgress = bInProgress;
}

void UHostMigrationManager::SetNextHost(APlayerState* NewHost, FString GUID)
{
	NextHost = NewHost;
	MigrationGUID = GUID;
	if (NextHost)
	{
		NextHostName = NextHost->GetPlayerName();
	}	
}

void UHostMigrationManager::LoadState(TArray<FHm_PlayerInformation>& OutPlayerInformation, TArray<FHm_CyberneticsInformation>& OutCyberneticsInformation,
	TArray<FHm_DoorInformation>& OutDoorInformation, TArray<FHm_BombInformation>& OutBombInformation, TArray<FString>& OutActiveEvidence,
	TArray<FHm_BaggedEvidence>& OutBaggedEvidences, TArray<FHm_DroppedEvidence>& OutDroppedEvidence, TArray<FHm_Objectives>& OutObjectivesInformations)
{
	OutPlayerInformation = PlayerInformations;
	OutCyberneticsInformation = CyberneticsInformations;
	OutDoorInformation = DoorInformations;
	OutBombInformation = BombInformations;
	OutActiveEvidence = ActiveEvidence;
	OutBaggedEvidences = BaggedEvidenceInformations;
	OutDroppedEvidence = DroppedEvidenceInformations;
	OutObjectivesInformations = ObjectiveInformations;
}

void UHostMigrationManager::StartMigration(bool bAsHost)
{
	AReadyOrNotPlayerController* PlayerController = UReadyOrNotStatics::GetReadyOrNotPlayerController();
	PlayerController->Client_CreateLoadingScreen("", "");

	FindSessionAttempt = 0;
		
	MapName = GetWorld()->GetMapName();
	MapName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);
	AReadyOrNotGameState* Gs = GetWorld()->GetGameState<AReadyOrNotGameState>();
	if (Gs)
	{
		ExpectedPlayerCount = Gs->PlayerArray.Num() - 1;
		ModeName = Gs->ModeURL_Replicated;
	}

#if WITH_EDITOR

	if (bAsHost)
	{
		SaveState();
		PlayerController->ConsoleCommand("open " + MapName + "?listen?game=" + Gs->ModeURL_Replicated);
	} else
	{
		PlayerController->ConsoleCommand("open 127.0.0.1");
	}
	SetHostMigrationInProgress(true);
	return;
#endif


	UDestroySessionCallbackProxyAdvanced* DestroySessionCallbackProxy = UDestroySessionCallbackProxyAdvanced::DestroySession(GetWorld(), UReadyOrNotStatics::GetReadyOrNotPlayerController());
	if (bAsHost)
	{
		SaveState();
		DestroySessionCallbackProxy->OnFailure.AddDynamic(this, &UHostMigrationManager::CreateMigrationSession);
		DestroySessionCallbackProxy->OnSuccess.AddDynamic(this, &UHostMigrationManager::CreateMigrationSession);
		
	} else
	{
		
		DestroySessionCallbackProxy->OnFailure.AddDynamic(this, &UHostMigrationManager::FindMigrationSession);
		DestroySessionCallbackProxy->OnSuccess.AddDynamic(this, &UHostMigrationManager::FindMigrationSession);
	}

	V_LOGM(LogReadyOrNot, "Host Migration: Destroying Session");
	DestroySessionCallbackProxy->Activate();
	SetHostMigrationInProgress(true);
}

void UHostMigrationManager::CreateMigrationSession()
{
	V_LOGM(LogReadyOrNot, "Host Migration: Creating new session");
	FSessionPropertyKeyPair GameMode, MapNamePair, GUID, Version, Checksum;
	AReadyOrNotGameState* Gs = Cast<AReadyOrNotGameState>(GetWorld()->GetGameState());
	if (Gs)
	{
		GameMode.Key = SETTING_GAMEMODE;
		GameMode.Data = *Gs->ModeURL_Replicated;
	}
	MapNamePair.Key = SETTING_MAPNAME;
	MapNamePair.Data = *FString(GetWorld()->GetMapName());
	GUID.Key = MIGRATION_GUID;
	GUID.Data = *MigrationGUID;
	Version.Key = SETTING_VERSION;
	Version.Data = *FString(UBpGameplayHelperLib::GetProjectVersion());
	UCreateSessionCallbackProxyAdvanced* AdvancedSessionCallback = UCreateSessionCallbackProxyAdvanced::CreateAdvancedSession(GetWorld(), {GameMode, MapNamePair, GUID, Version}, UBpGameplayHelperLib::GetLocalPlayerController(GetWorld()), 5, 0,
		false, true, false, true, true, true, false, false, false, true);
	AdvancedSessionCallback->OnSuccess.AddDynamic(this, &UHostMigrationManager::OnLobbySuccess);
	AdvancedSessionCallback->OnFailure.AddDynamic(this, &UHostMigrationManager::ReturnToMainMenu);
	AdvancedSessionCallback->Activate();
}

void UHostMigrationManager::OnLobbySuccess()
{
	FString Url = "open " + MapName + "?listen" + "?game=" + ModeName;
	GEngine->Exec(GetWorld(), *Url);
	V_LOGM(LogReadyOrNot, "Host Migration: Success! Opening World %s", *Url);
}

void UHostMigrationManager::FindMigrationSession()
{
	if (FindSessionAttempt > 30)
	{
		V_LOGM(LogReadyOrNot, "Host Migration: Unable to Migrate %s (Attempt = %d)", *MigrationGUID, FindSessionAttempt);
		ReturnToMainMenu();
		return;
	}
	
	TArray<FSessionsSearchSetting> SearchSettings;
	FSessionsSearchSetting Version, GUID;
	Version.ComparisonOp = EOnlineComparisonOpRedux::Equals;
	Version.PropertyKeyPair.Key = SETTING_VERSION;
	Version.PropertyKeyPair.Data = *UBpGameplayHelperLib::GetProjectVersion();
	GUID.PropertyKeyPair.Key = MIGRATION_GUID;
	GUID.PropertyKeyPair.Data = *MigrationGUID;


	SearchSettings.Add(Version);
	SearchSettings.Add(GUID);
	UFindSessionsCallbackProxyAdvanced* AdvancedSessionCallback = UFindSessionsCallbackProxyAdvanced::FindSessionsAdvanced(GetWorld(), UBpGameplayHelperLib::GetLocalPlayerController(GetWorld()), 1, false, EBPServerPresenceSearchType::AllServers, SearchSettings, false, true);
	AdvancedSessionCallback->OnSuccess.AddDynamic(this, &UHostMigrationManager::OnMigrationSessionFoundSuccess);
	AdvancedSessionCallback->OnFailure.AddDynamic(this, &UHostMigrationManager::OnMigrationSessionFoundSuccess);
	AdvancedSessionCallback->Activate();
	V_LOGM(LogReadyOrNot, "Host Migration: Finding Sessions with %s", *MigrationGUID);
}

void UHostMigrationManager::OnMigrationSessionFoundSuccess(const TArray<FBlueprintSessionResult>& Results)
{
	TArray<FBlueprintSessionResult> DesiredResults;
	FBlueprintSessionResult DesiredResult;
	V_LOGM(LogReadyOrNot, "Host Migration: Found %d Sessions with %s", Results.Num(), *MigrationGUID);
	for (FBlueprintSessionResult Session : Results)
	{
		FString ServerMigrationGUID;
		Session.OnlineResult.Session.SessionSettings.Get(MIGRATION_GUID, ServerMigrationGUID);
		if (MigrationGUID != ServerMigrationGUID)
		{
			// this shouldn't happen?
			continue;
		}

		AReadyOrNotPlayerController* pc = UReadyOrNotStatics::GetReadyOrNotPlayerController();
		if (pc)
		{
			bFoundSession = true;
			ULevelStreaming* LevelStreaming;
			V_LOGM(LogReadyOrNot, "Host Migration: Joining Success! %s (Attempts = %d)", *MigrationGUID, FindSessionAttempt);
			pc->StreamInSession(Session, LevelStreaming, false);
		}
	}
	
	FindSessionAttempt++;
	FindMigrationSession();
}

void UHostMigrationManager::OnMigrationSessionFoundFailed(const TArray<FBlueprintSessionResult>& Results)
{
	ReturnToMainMenu();
}

void UHostMigrationManager::ReturnToMainMenu()
{
	APlayerController* pc = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld());
	if (pc)
	{
		SetHostMigrationInProgress(false);
		pc->ClientReturnToMainMenuWithTextReason(FText::FromString(XorString("Unable to migrate Host. Error: Retry count exceeded.")));
	}
}
