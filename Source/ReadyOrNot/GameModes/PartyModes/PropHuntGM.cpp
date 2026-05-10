// Copyright Void Interactive, 2023

#include "GameModes/PartyModes/PropHuntGM.h"

#include "PropHuntGS.h"
#include "Algo/RandomShuffle.h"

void APropHuntGM::StartMatch()
{
	Super::StartMatch();
}

void APropHuntGM::OnRoundStarted_Implementation()
{
	Super::OnRoundStarted_Implementation();
	
	if (APropHuntGS* GS = GetWorld()->GetGameState<APropHuntGS>())
	{
		if (const UDataTable* LevelTable = UBpGameplayHelperLib::GetLevelLookupDataTable())
		{
			if (const FLevelDataLookupTable* Row = LevelTable->FindRow<FLevelDataLookupTable>(UReadyOrNotFunctionLibrary::GetCurrentLevelNameForLookupTable(GetWorld()), "StartMatch"))
			{
				GS->AvailableProps = Row->PropHuntMeshes;
			}
		}
	}
}

void APropHuntGM::NextRound()
{
	TArray<AReadyOrNotPlayerController*> Temp = Props;
	Props = Hunters;
	Hunters = Temp;
	
	Super::NextRound();
}

void APropHuntGM::RespawnAllPlayers()
{
	V_LOGM(LogReadyOrNot, "[Prop Hunt] Respawning ALL players!");
	
	// We can safely empty this we are already spawning all of the players
	DeadPlayers.Empty();
	RespawnableDeadPlayers.Empty();

	int32 NumPlayers = 0;
	if (AReadyOrNotGameState* GS = GetGameState<AReadyOrNotGameState>())
	{
		GS->BlueTeamPlayers.Empty(10);
		GS->RedTeamPlayers.Empty(10);
		NumPlayers = GS->GetNumPlayers();
	}

	if (NumPlayers <= 0)
		return;
	
	TArray<AReadyOrNotPlayerController*> All;
	for (TActorIterator<AReadyOrNotPlayerController> It(GetWorld()); It; ++It)
	{
		AReadyOrNotPlayerController* controller = *It;
		controller->DestroyLastKilledCharacter();
		
		All.AddUnique(controller);
	}

	if (Props.Num() == 0 || Hunters.Num() == 0)
	{
		int32 i = 0;
		Hunters.Empty();
		Props.Empty();
		Algo::RandomShuffle(All);
		for (AReadyOrNotPlayerController* controller : All)
		{
			if (i < NumPlayers/2)
			{
				Hunters.AddUnique(controller);
			}
			else
			{
				Props.AddUnique(controller);
			}
			
			i++;
		}
	}

	uint8 NumHunters = 0;
	uint8 NumProps = 0;
	for (TActorIterator<AReadyOrNotPlayerController> It(GetWorld()); It; ++It)
	{
		AReadyOrNotPlayerController* controller = *It;
		AReadyOrNotPlayerState* ps = Cast<AReadyOrNotPlayerState>(controller->PlayerState);
		if (ps)
		{
			if (ps->bReady || ps->bJoinInProgress)
			{
				ps->bIsInGame = true;

				if (Props.Contains(controller))
				{
					NumProps++;
					ps->PlayerSpawnTag = "Prop_" + FString::FromInt(NumProps);
					RespawnPlayerProp(controller);
				}
				else
				{
					NumHunters++;
					ps->PlayerSpawnTag = "Hunter_" + FString::FromInt(NumHunters);
					RespawnPlayer(controller);
				}
				
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

void APropHuntGM::RespawnPlayerProp(APlayerController* Controller)
{
	if (AReadyOrNotPlayerController* ronController = Cast<AReadyOrNotPlayerController>(Controller))
	{
		AReadyOrNotPlayerState* ps = Cast<AReadyOrNotPlayerState>(Controller->PlayerState);
		
		if (ps)
			V_LOGM(LogReadyOrNot, "Respawning player %s", *ps->GetPlayerName());
		
		// order is important here set the team before getting the spawnpoint
		if (ps) ps->PendingTeam = ETeamType::TT_SERT_RED;
		if (ps) ps->TrySetPendingTeamAsTeam();
		if (ps) PlayerSpawnTag = ps->PlayerSpawnTag;
		
		APawn* Pawn = Controller->GetPawn();
		
		Controller->UnPossess();
		
		if (Pawn)
		{
			Pawn->Destroy();
		}

		ronController->ClientSpawned();
		ronController->ClientSpawned_Implementation();

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
			StartPoint = FindPlayerStart(Controller);
		}
		
		if (StartPoint)
		{
			FTransform SpawnTransform = StartPoint->GetActorTransform();
			SpawnTransform.SetRotation(FRotator(0.0f, SpawnTransform.GetRotation().Z, 0.0f).Quaternion());

			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
			SpawnParams.bNoFail = true;
		
			V_LOGM(LogReadyOrNot, "Spawning Player Character for %s of class %s at %s", *Controller->PlayerState->GetPlayerName(), *PropHuntCharacterClass->GetName(), *SpawnTransform.ToString());
			
			if (ACharacter* NewChar = GetWorld()->SpawnActor<ACharacter>(PropHuntCharacterClass, SpawnTransform, SpawnParams))
			{
				if (ps)
				{
					ps->bSpawnLoadout = true;
				}

				Controller->Possess(NewChar);
				Controller->SetPawn(NewChar);
				LastPlayerSpawnPoint = SpawnTransform;
			
				if (AReadyOrNotPlayerController* pc = Cast<AReadyOrNotPlayerController>(Controller))
				{
					if (GetReadyOrNotGameState()->bPvPMode)
					{
						UReadyOrNotFunctionLibrary::StartTimerForCallback(pc, &AReadyOrNotPlayerController::DestroyLastKilledCharacter, 60.0f, false, true);
					}

					pc->Client_SetControlRotation(SpawnTransform.GetRotation().Rotator());
				
					pc->Client_ClearHUDWidgets();
					pc->Client_DisableUIMouse();
				}
			}
		}
	}
}

void APropHuntGM::TimeLimitVictoryConditions_Implementation()
{
	bool bAnyPropsAlive = false;
	
	for (const AReadyOrNotPlayerController* Controller : Props)
	{
		if (Controller && Controller->GetPawn())
		{
			if (const UHealthComponent* HealthComponent = Cast<UHealthComponent>(Controller->GetPawn()->GetComponentByClass(UHealthComponent::StaticClass())))
			{
				if (!HealthComponent->IsDepleted())
				{
					bAnyPropsAlive = true;
					break;
				}
			}
		}
	}

	if (bAnyPropsAlive)
	{
		RoundWonTeam(ETeamType::TT_SERT_RED);
	}
	else
	{
		RoundWonTeam(ETeamType::TT_SERT_BLUE);
	}
}

void APropHuntGM::CheckVictoryConditions()
{
	bool bAllPropsDead = true;
	
	for (const AReadyOrNotPlayerController* Controller : Props)
	{
		if (Controller && Controller->GetPawn())
		{
			if (const UHealthComponent* HealthComponent = Cast<UHealthComponent>(Controller->GetPawn()->GetComponentByClass(UHealthComponent::StaticClass())))
			{
				if (!HealthComponent->IsDepleted())
				{
					bAllPropsDead = false;
					break;
				}
			}
		}
	}

	if (bAllPropsDead)
	{
		RoundWonTeam(ETeamType::TT_SERT_BLUE);
	}
}
