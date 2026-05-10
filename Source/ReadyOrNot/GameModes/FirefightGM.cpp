#include "FirefightGM.h"
#include "ReadyOrNot.h"
#include "FirefightGS.h"

AFirefightGM::AFirefightGM() : Super()
{
	bArrestSpectator = false;
}

void AFirefightGM::BeginPlay()
{
	Super::BeginPlay();

	RegenerateRandomLoadouts();
}

void AFirefightGM::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void AFirefightGM::StartMatch()
{
	RegenerateRandomLoadouts();

	Super::StartMatch();
}

void AFirefightGM::RoundEnd()
{
	Super::RoundEnd();

	bSuddenDeath = false;

	RegenerateRandomLoadouts();
}

void AFirefightGM::MatchEnd()
{
	Super::MatchEnd();
}

void AFirefightGM::TimeLimitVictoryConditions_Implementation()
{
	Super::TimeLimitVictoryConditions_Implementation();

	AReadyOrNotGameState* gs = GetGameState<AReadyOrNotGameState>();
	if (!gs)
	{
		return;
	}

	// If the timelimit has been reached, we give victory to the team with most number of players remaining
	if (GetNumberOfActivePlayersOnTeam(ETeamType::TT_SERT_BLUE) > GetNumberOfActivePlayersOnTeam(ETeamType::TT_SERT_RED))
	{
		// more players on blue = blue victory
		RoundWonTeam(ETeamType::TT_SERT_BLUE);
	}
	else if (GetNumberOfActivePlayersOnTeam(ETeamType::TT_SERT_RED) > GetNumberOfActivePlayersOnTeam(ETeamType::TT_SERT_BLUE))
	{
		RoundWonTeam(ETeamType::TT_SERT_RED);
	}
	else
	{
		// we go into Sudden Death.
		bSuddenDeath = true;

		// post status to chat - fixme, this needs to be localized
		FRChatMessage Message;

		Message.Color = FColor::White;
		Message.Message = "Sudden death!";
		gs->Multicast_BroadcastChatMessage(Message);
	}
}

// Get the number of "alive" players on a team
int32 AFirefightGM::GetNumberOfActivePlayersOnTeam(ETeamType Team)
{
	int32 ActivePlayers = 0;

	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerCharacter::StaticClass(), Actors);

	for (int32 i = 0; i < Actors.Num(); i++)
	{
		APlayerCharacter* pc = Cast<APlayerCharacter>(Actors[i]);
		if (pc->GetTeam() == Team)
		{
			if (!pc->IsDeadOrUnconscious() && !pc->IsArrested())
			{
				ActivePlayers++;
			}
		}
	}

	return ActivePlayers;
}

void AFirefightGM::CheckVictoryConditions()
{
	AReadyOrNotGameState* gs = GetGameState<AReadyOrNotGameState>();
	if (!gs)
	{
		return;
	}

	if (GetNumberOfActivePlayersOnTeam(ETeamType::TT_SERT_RED) <= 0)
	{
		// no active players left on red = blue team wins
		RoundWonTeam(ETeamType::TT_SERT_BLUE);
	}
	else if (GetNumberOfActivePlayersOnTeam(ETeamType::TT_SERT_BLUE) <= 0)
	{
		// no active players left on blue = red team wins
		RoundWonTeam(ETeamType::TT_SERT_RED);
	}
}

void AFirefightGM::PlayerArrested(AReadyOrNotCharacter* ArrestedCharacter, AReadyOrNotCharacter* InstigatorCharacter)
{
	AFirefightGS* gs = GetGameState<AFirefightGS>();
	if (!gs)
	{
		return;
	}

	Super::PlayerArrested(ArrestedCharacter, InstigatorCharacter);

	if (bSuddenDeath)
	{
		if (InstigatorCharacter && InstigatorCharacter->GetTeam() == ETeamType::TT_SERT_BLUE)
		{
			RoundWonTeam(ETeamType::TT_SERT_BLUE);
		}
		else if (InstigatorCharacter && InstigatorCharacter->GetTeam() == ETeamType::TT_SERT_RED)
		{
			RoundWonTeam(ETeamType::TT_SERT_RED);
		}
	}
	else
	{
		CheckVictoryConditions();

		if (GetMatchState() == EMatchState::MS_Playing)
		{
			// Respawn all the dead players if the game is still going on
			RespawnDeadPlayersOnTeam(InstigatorCharacter->GetTeam());

			// Put an objective marker on the arrested guy
			//ArrestedCharacter->Multicast_StartShowingObjectiveMarker(EPlayerObjectiveMarkerType::POMT_Free, ArrestedCharacter->GetTeam());

			// // Mark them as freeable
			// ArrestedCharacter->bArrestedButFreeable = true;
			//
			// // give godmode to the arrested character so they can't be killed
			// ArrestedCharacter->bGodMode = true;
		}
	}
}

void AFirefightGM::PlayerKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter)
{
	AReadyOrNotGameState* gs = GetGameState<AReadyOrNotGameState>();
	if (!gs)
	{
		return;
	}

	Super::PlayerKilled(InstigatorCharacter, KilledCharacter);

	if (bSuddenDeath)
	{
		APlayerCharacter* killerPC = Cast<APlayerCharacter>(InstigatorCharacter);
		APlayerCharacter* killedPC = Cast<APlayerCharacter>(KilledCharacter);

		if (!killerPC || !killedPC)
		{
			return;
		}

		if (killerPC->GetTeam() == killedPC->GetTeam())
		{
			if (killerPC->GetTeam() == ETeamType::TT_SERT_RED)
			{
				RoundWonTeam(ETeamType::TT_SERT_BLUE);
			}
			else if (killerPC->GetTeam() == ETeamType::TT_SERT_BLUE)
			{
				RoundWonTeam(ETeamType::TT_SERT_RED);
			}
		}
		else if (killerPC->GetTeam() == ETeamType::TT_SERT_RED)
		{
			RoundWonTeam(ETeamType::TT_SERT_RED);
		}
		else if (killerPC->GetTeam() == ETeamType::TT_SERT_BLUE)
		{
			RoundWonTeam(ETeamType::TT_SERT_BLUE);
		}
	}
	else
	{
		CheckVictoryConditions();
		CheckToAnnounceTeamkill(InstigatorCharacter, KilledCharacter);
	}
}

AActor* AFirefightGM::FindPlayerStart_Implementation(AController* Player, const FString& IncomingName)
{
	FName playerSpawnTag = "";
	AReadyOrNotPlayerState* ps = Cast<AReadyOrNotPlayerState>(Player->PlayerState);
	if (ps)
	{
		//GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::White, "Finding Player start for " + ps->GetPlayerName());
		//GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::White, "Player is on team: " + FString::FromInt((uint8)ps->GetTeam()));
		switch (ps->GetTeam())
		{
		case ETeamType::TT_SERT_BLUE:
			playerSpawnTag = SWATBlueStartTag;
			break;
		case ETeamType::TT_SERT_RED:
			playerSpawnTag = SWATRedStartTag;
			break;
		case ETeamType::TT_SUSPECT:
			playerSpawnTag = SuspectStartTag;
			break;
		}
		//GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::White, playerSpawnTag.ToString());
	}

	TArray<AActor*> compatibleStarts;
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		APlayerStart* start = *It;
		if (start)
		{
			if (start->PlayerStartTag == playerSpawnTag)
			{
				compatibleStarts.Add(start);
			}
		}
	}

	if (compatibleStarts.Num() > 0)
	{
		return compatibleStarts[FMath::FRandRange(0.0f, compatibleStarts.Num() - 1)];
	}
	else
	{
		return GetRandomSafeStart();
	}

	return nullptr;
}

void AFirefightGM::RespawnPlayer(APlayerController* Player, bool bForceSpectator)
{
	APlayerController* pc = Cast<APlayerController>(Player);

	if (GeneratedLoadouts.Num() <= 0)
	{
		RegenerateRandomLoadouts();
	}

	if (pc && (GetMatchState() == EMatchState::MS_Playing || GetMatchState() == EMatchState::MS_Warmup))
	{

		AReadyOrNotPlayerState* ps = Cast<AReadyOrNotPlayerState>(pc->PlayerState);
		FTransform SpawnTransform;

		if (bForceSpectator)
		{
			SpawnSpectator(pc, (TSubclassOf<ASpectatorPawn>)(SpectatorClass), SpawnTransform);
		}
		else
		{

			AActor* startPoint = FindPlayerStart(pc);
			if (startPoint)
			{
				SpawnTransform = startPoint->GetActorTransform();
				if (ps)
				{
					FSavedLoadout RandomLoadout;

					if (GetMatchState() == EMatchState::MS_Playing)
						ps->bIsInGame = true;

					if (ps->Team == ETeamType::TT_SERT_RED)
					{
						RandomLoadout = GeneratedLoadouts[NumRedSpawned++];
						NumRedSpawned %= GeneratedLoadouts.Num();
					}
					else if (ps->Team == ETeamType::TT_SERT_BLUE)
					{
						RandomLoadout = GeneratedLoadouts[NumBlueSpawned++];
						NumBlueSpawned %= GeneratedLoadouts.Num();
					}

					APlayerCharacter* character = SpawnPlayerCharacter(pc, BlueCharacterClass.LoadSynchronous(), SpawnTransform);
					UBpGameplayHelperLib::EquipLoadoutOnPlayer(RandomLoadout, character, FLoadoutEquipOptions());
					if (ps)
					{
						ps->LastLoadout = RandomLoadout;
					}
				}
			}
		}

	}
}

void AFirefightGM::ResetClientScores(bool bBetweenRounds)
{
	if (bBetweenRounds)
		return;

	Super::ResetClientScores(bBetweenRounds);
}

void AFirefightGM::RegenerateRandomLoadouts()
{
	NumRedSpawned = NumBlueSpawned = 0;
	GeneratedLoadouts.Empty();

	TArray<AActor*> PlayerStates;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AReadyOrNotPlayerState::StaticClass(), PlayerStates);

	// generate N random loadouts
	for (int32 i = 0; i < PlayerStates.Num(); i++)
	{
		GeneratedLoadouts.Add(RandomLoadouts[FMath::RandRange(0, RandomLoadouts.Num() - 1)]);
	}
}