// Copyright Void Interactive, 2022


 #include "GameModes/DefusalGM.h"

#include "CoopGS.h"
#include "DefusalGS.h"
#include "Actors/Door.h"
#include "Actors/Gameplay/EvidenceActor.h"
#include "Actors/Items/Multitool.h"

void ADefusalGM::BeginPlay()
{
	Super::BeginPlay();

	ResetBomb();
	SetDefusalMatchState(EDefusalMatchSate::DMS_Warmup);
}

void ADefusalGM::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (PendingPlayerSpawn.Num() > 0)
	{
		APlayerController* Controller = PendingPlayerSpawn[0];
		if (Controller)
		{
			AReadyOrNotPlayerState* Ps = Cast<AReadyOrNotPlayerState>(Controller->PlayerState);
			if (Ps)
			{
				if (Ps->ServerSavedLoadout.IsValid())
				{
					RespawnPlayer(Controller, false);
					PendingPlayerSpawn.Remove(Controller);
				}
			}
		}
	}
	
	if (GetMatchState() == EMatchState::MS_Playing)
	{
		if (SelectedBombActor)
		{
			GetDefusalGameState()->BombTimeRemaining = FMath::RoundToInt(SelectedBombActor->TimeUntilExplodes);
			switch(SelectedBombActor->BombState)
			{
			case EBombState::BS_None: break;
			case EBombState::BS_Active:
				
				break;
			case EBombState::BS_Disabled:
				RoundWonTeam(ETeamType::TT_SERT_BLUE);
				break;
			case EBombState::BS_Exploded:
				RoundWonTeam(ETeamType::TT_SERT_RED);
				break;
			default: ;
			}
			
		}
		else
		{
			ResetBomb();
		}
	}
	else
	{
		SelectedBombActor = nullptr;
	}

	if (GetNumPlayers() == 0)
	{
		FString LobbyLevel = GetGameInstance<UReadyOrNotGameInstance>()->LobbyLevel;
		ProcessServerTravel(LobbyLevel + "?game=lobby", true);
	}

	TArray<APlayerCharacter*> AliveSuspects;
	TArray<APlayerCharacter*> AliveSwat;
	for (TActorIterator<APlayerCharacter>It(GetWorld()); It; ++It)
	{
		if (It->GetTeam() == ETeamType::TT_SERT_RED && !It->IsDeadOrUnconscious())
			AliveSuspects.Add(*It);

		if (It->GetTeam() == ETeamType::TT_SERT_BLUE && !It->IsDeadOrUnconscious())
			AliveSwat.Add(*It);
		
		if (It->GetTeam() == ETeamType::TT_SERT_RED)
		{
			ABaseItem* LongTactical = It->GetInventoryComponent()->GetInventoryItemOfType(EItemCategory::IC_LongTactical);
			ABaseItem* Helmet = It->GetInventoryComponent()->GetInventoryItemOfType(EItemCategory::IC_Helmet);
			ABaseItem* Armor = It->GetInventoryComponent()->GetInventoryItemOfType(EItemCategory::IC_Armor);
			It->GetInventoryComponent()->DestroyInventoryItem(LongTactical);
			It->GetInventoryComponent()->DestroyInventoryItem(Helmet);
			It->GetInventoryComponent()->DestroyInventoryItem(Armor);
			if (It->GetMesh()->GetAnimInstance())
			{
				FCharacterLookOverride* Override = CharacterLookMap.Find(*It);
				if (Override)
				{
					if (It->CharacterLookOverride.BodyMeshOverride != Override->BodyMeshOverride || It->CharacterLookOverride.FPMeshOverride !=SuspectFPArmsOverride || It->HasCharacterLookOverrideStringSet())
					{
						It->UpdateOverridesFromCharacterLookOverrideDataTable("");
						It->ForceMeshUsingOverride(SuspectFPArmsOverride, Override->BodyMeshOverride, Override->FaceMeshOverride ? Override->FaceMeshOverride : BlankFaceMesh);
					}
				}
			}
			
		}
	}

	if (GetNumPlayers() >= 2)
	{
		if (GetMatchState() == EMatchState::MS_Playing)
		{
			if (AliveSwat.Num() == 0)
			{
				RoundWonTeam(ETeamType::TT_SERT_RED);
			}
			if (AliveSuspects.Num() == 0)
			{
				RoundWonTeam(ETeamType::TT_SERT_BLUE);
			}
		}	
	}	
}


void ADefusalGM::RespawnPlayer(APlayerController* Player, bool bForceSpectator)
{
	if (GetMatchState() != EMatchState::MS_Playing)
		return Super::RespawnPlayer(Player, bForceSpectator);
	
	AReadyOrNotPlayerController* pc = Cast<AReadyOrNotPlayerController>(Player);

	if (SpawnPoints.Num() == 0)
	{
		for (TActorIterator<AAISpawn>It(GetWorld()); It; ++It)
		{
			SpawnPoints.Add(*It);
		}
		
		for (TActorIterator<APlayerStart>It(GetWorld()); It; ++It)
		{
			if (It->PlayerStartTag == "Spawn_1")
			{
				SwatSpawn = *It;
			}
			if (It->PlayerStartTag == "Defusal_SuspectSpawn")
			{
				SuspectSpawn = *It;
			}
		}		
	}

	
	check(SwatSpawn && SpawnPoints.Num() > 0 && SuspectSpawnData.Num() > 0);
	if (SwatSpawn && SpawnPoints.Num() > 0 && SuspectSpawnData.Num() > 0)
	{
		FTransform SuspectSpawnTransform = FTransform(); 
		// If we have suspect spawn points let use them isntead otherwise fallback to 'Defusal_SuspectSpawn'
		if (SpawnPoints.Num() > 0)
		{
			 SuspectSpawnTransform = SpawnPoints[FMath::RandRange(0, SpawnPoints.Num() - 1)]->GetActorTransform();
		}
		TArray<FSpawnData> OutSpawnData;
		APlayerCharacter* PlayerCharacter = SpawnPlayerCharacter(Player, BlueCharacterClass.LoadSynchronous(), pc->GetTeamType() == ETeamType::TT_SERT_RED ? SuspectSpawnTransform : SwatSpawn->GetActorTransform());

		pc->GetRoNPlayerState()->bIsInGame = true;
		FSavedLoadout Loadout = PlayerCharacter->GetInventoryComponent()->GetLastEquippedLoadout();

		if (pc->GetTeamType() == ETeamType::TT_SERT_RED)
		{
			Loadout.Armor = nullptr;
			Loadout.Helmet = nullptr;
			Loadout.LongTactical = nullptr;
			Loadout.CharacterLookOverride = "";
		}

		FCharacterLookOverride LookOverride;
		if (const FAIDataLookupTable* AIData =  SuspectSpawnData[FMath::RandRange(0, SuspectSpawnData.Num() - 1)].SpawnedAI.GetRow<FAIDataLookupTable>("LookupAISpawner"))
		{
			FCharacterMesh RngMesh;
			if (AIData->GetRandomCharacterMeshOverride(RngMesh))
			{
				LookOverride.BodyMeshOverride = RngMesh.Body;
				LookOverride.FaceMeshOverride = RngMesh.Head;
				CharacterLookMap.Add(PlayerCharacter, LookOverride);
			}
		}

		UBpGameplayHelperLib::EquipLoadoutOnPlayer(Loadout, PlayerCharacter, FLoadoutEquipOptions());
		if (PlayerCharacter->GetTeam() == ETeamType::TT_SERT_RED)
		{
			PlayerCharacter->GetInventoryComponent()->DestroyInventoryItem(PlayerCharacter->GetInventoryComponent()->GetInventoryItemOfClass(AMultitool::StaticClass(), false));
		}
	}
}
	

AActor* ADefusalGM::FindPlayerStart_Implementation(AController* Player, const FString& IncomingName)
{
	return Super::FindPlayerStart_Implementation(Player, IncomingName);
}

void ADefusalGM::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	PendingPlayerSpawn.AddUnique(NewPlayer);
}


void ADefusalGM::ResetBomb()
{
	for (TActorIterator<AEvidenceActor>It(GetWorld()); It; ++It)
	{
		It->Destroy();
	}
	
	TArray<ABombActor*> BombActors;
	for (TActorIterator<ABombActor>It(GetWorld()); It; ++It)
	{
		BombActors.Add(*It);
		It->BombState = EBombState::BS_HiddenAndFullyDisabled;
		It->SetActorHiddenInGame(true);
	}

	int32 SelectedBombActorIdx = FMath::RandRange(0, BombActors.Num() - 1);
	ensure(BombActors.IsValidIndex(SelectedBombActorIdx));
	if (BombActors.IsValidIndex(SelectedBombActorIdx))
	{
		SelectedBombActor = BombActors[SelectedBombActorIdx];
		SelectedBombActor->BombState = EBombState::BS_Active;
		SelectedBombActor->TimeUntilExplodes = 60.0f*4.0f;
		SelectedBombActor->SetActorHiddenInGame(false);
		check(SelectedBombActor);
	}

}

void ADefusalGM::ResetAI()
{
	// TODO: AI
	for(TActorIterator<ACyberneticCharacter>It(GetWorld()); It; ++It)
	{
		It->Destroy();
	}
}

void ADefusalGM::RoundWon(TArray<AReadyOrNotPlayerState*> WinningPlayers)
{
	Super::RoundWon(WinningPlayers);
}

void ADefusalGM::StartMatch()
{
	Super::StartMatch();

}

void ADefusalGM::RoundWonTeam(ETeamType WinningTeam)
{
	Super::RoundWonTeam(WinningTeam);
}

void ADefusalGM::NextRound()
{
	bool bHalfTime = GetDefusalGameState()->RoundsPlayed == GetDefusalGameState()->RoundsToPlay - 1;
	if (bHalfTime)
	{
		SwapSides();
	}
	for(TActorIterator<ABaseItem>It(GetWorld()); It; ++It)
	{
		It->Destroy();
	}
	ResetLevel();
	Super::NextRound();
}

ADefusalGS* ADefusalGM::GetDefusalGameState()
{
	return GetGameState<ADefusalGS>();
}

void ADefusalGM::SwapSides()
{
	for (TActorIterator<AReadyOrNotPlayerState>It(GetWorld()); It; ++It)
	{
		if (It->GetTeam() == ETeamType::TT_SERT_BLUE)
		{
			It->Team = ETeamType::TT_SERT_RED;
		} else if (It->GetTeam() == ETeamType::TT_SERT_RED)
		{
			It->Team = ETeamType::TT_SERT_BLUE;
		}
	}
	int32 RedTeamScore = GetDefusalGameState()->RedTeamWins;
	int32 BlueTeamScore = GetDefusalGameState()->BlueTeamWins;
	GetDefusalGameState()->RedTeamWins = BlueTeamScore;
	GetDefusalGameState()->BlueTeamWins = RedTeamScore;
}

void ADefusalGM::SetDefusalMatchState(EDefusalMatchSate NewMatchState)
{
	GetDefusalGameState()->ChangeDefusalMatchState(NewMatchState);
}

EDefusalMatchSate ADefusalGM::GetDefusalMatchState()
{
	return GetDefusalGameState()->GetDefusalMatchstate();
}