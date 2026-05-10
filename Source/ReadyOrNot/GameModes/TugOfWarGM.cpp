#include "TugOfWarGM.h"
#include "ReadyOrNot.h"
#include "Actors/Gameplay/TugOfWarMover.h"

void ATugOfWarGM::BeginPlay()
{
	Super::BeginPlay();

	if (MatchLoopMusic)
	{
		UFMODBlueprintStatics::PlayEvent2D(GetWorld(), MatchLoopMusic, true);
	}

}

void ATugOfWarGM::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void ATugOfWarGM::StartMatch()
{
	Super::StartMatch();

	if (MatchStartMusic)
	{
		UFMODBlueprintStatics::PlayEvent2D(GetWorld(), MatchStartMusic, true);
	}
}

void ATugOfWarGM::RoundEnd()
{
	Super::RoundEnd();
	
}

void ATugOfWarGM::MatchEnd()
{
	Super::MatchEnd();

	if (MatchEndMusic)
	{
		UFMODBlueprintStatics::PlayEvent2D(GetWorld(), MatchEndMusic, true);
	}
}

void ATugOfWarGM::ResetClientScores(bool bBetweenRounds)
{
	if (bBetweenRounds)
	{
		return;
	}

	Super::ResetClientScores(bBetweenRounds);
}

bool ATugOfWarGM::AreAllPlayersOnTeamArrested(ETeamType Team)
{
	TArray<AActor*> Actors;

	bool bAllPlayersArrested = true;

	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerCharacter::StaticClass(), Actors);
	for (int32 i = 0; i < Actors.Num(); i++)
	{
		APlayerCharacter* pc = Cast<APlayerCharacter>(Actors[i]);
		if (pc && pc->GetTeam() == Team && !pc->IsDeadOrUnconscious() && !pc->IsArrested())
		{
			bAllPlayersArrested = false;
			break;
		}
	}

	return bAllPlayersArrested;
}

int32 ATugOfWarGM::GetNumberOfArrestedPlayersOnTeam(ETeamType Team)
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

void ATugOfWarGM::PlayerArrested(AReadyOrNotCharacter* ArrestedCharacter, AReadyOrNotCharacter* InstigatorCharacter)
{
	Super::PlayerArrested(ArrestedCharacter, InstigatorCharacter);
	AReadyOrNotPlayerState* ps = Cast<AReadyOrNotPlayerState>(ArrestedCharacter->GetPlayerState());
	if (ps)
	{
		ps->TimesArrested += 1;

		if (InstigatorCharacter)
		{
			AReadyOrNotPlayerState* InstigatorPS = Cast<AReadyOrNotPlayerState>(InstigatorCharacter->GetPlayerState());
			if (InstigatorPS)
			{
				InstigatorPS->ArrestsThisLife += 1;
				InstigatorPS->Arrests += 1;
				InstigatorPS->SetScore(InstigatorPS->GetScore() + 25);
			}
		}

		CheckVictoryConditions();

		// Put an objective marker on the arrested guy
		//ArrestedCharacter->Multicast_StartShowingObjectiveMarker(EPlayerObjectiveMarkerType::POMT_Free, ArrestedCharacter->GetTeam());

		// // Mark them as freeable
		// ArrestedCharacter->bArrestedButFreeable = true;
		//
		// ArrestedCharacter->bNoTeamDamage = true;
	}

	
}

void ATugOfWarGM::PlayerFreed(ACharacter* Freed, ACharacter* Freer)
{
	APlayerCharacter* FreedPc = Cast<APlayerCharacter>(Freed);
	if (!FreedPc)
	{
		return;
	}

	// Turn off god mode for the freed player
	FreedPc->SetGodMode(false);

	// Add 5 points to the person who freed
	if (Freer)
	{
		AReadyOrNotPlayerState* ps = Cast<AReadyOrNotPlayerState>(Freer->GetPlayerState());
		if (ps)
		{
			ps->SetScore(ps->GetScore() + 25);
		}
	}

	Super::PlayerFreed(Freed, Freer);
}

void ATugOfWarGM::CheckVictoryConditions()
{
	AReadyOrNotGameState* gs = GetGameState<AReadyOrNotGameState>();
	if (!gs)
	{
		return;
	}

	if (AreAllPlayersOnTeamArrested(ETeamType::TT_SERT_RED))
	{
		// all players on red arrested = blue victory
		RoundWonTeam(ETeamType::TT_SERT_BLUE);
	}
	else if (AreAllPlayersOnTeamArrested(ETeamType::TT_SERT_BLUE))
	{
		// all players on blue arrested = red victory
		RoundWonTeam(ETeamType::TT_SERT_RED);

	}
}

void ATugOfWarGM::TimeLimitVictoryConditions_Implementation()
{
	Super::TimeLimitVictoryConditions_Implementation();

	AReadyOrNotGameState* gs = GetGameState<AReadyOrNotGameState>();
	if (!gs)
	{
		return;
	}

	if (GetNumberOfArrestedPlayersOnTeam(ETeamType::TT_SERT_RED) > GetNumberOfArrestedPlayersOnTeam(ETeamType::TT_SERT_BLUE))
	{
		// announce blue victory
		RoundWonTeam(ETeamType::TT_SERT_BLUE);
	}
	else if (GetNumberOfArrestedPlayersOnTeam(ETeamType::TT_SERT_BLUE) > GetNumberOfArrestedPlayersOnTeam(ETeamType::TT_SERT_RED))
	{
		// announce red victory
		RoundWonTeam(ETeamType::TT_SERT_RED);
	}
	else
	{
		for (TActorIterator<ATugOfWarMover> It(GetWorld()); It; ++It)
		{
			ATugOfWarMover* mover = *It;
			if (mover)
			{
				if (mover->MoverCurrentPosition < 0.5f)
				{
					RoundWonTeam(mover->bInvertVictoryPositions ? ETeamType::TT_SERT_RED : ETeamType::TT_SERT_BLUE);
				}
				else if (mover->MoverCurrentPosition > 0.5f)
				{
					RoundWonTeam(mover->bInvertVictoryPositions ? ETeamType::TT_SERT_BLUE : ETeamType::TT_SERT_RED);
				}
				else
				{
					RoundWonTeam(ETeamType::TT_NONE);
				}
				mover->MoverCurrentPosition = 0.5f;
			}
		}
	}
}

AActor* ATugOfWarGM::FindPlayerStart_Implementation(AController* Player, const FString& IncomingName /* = TEXT("") */)
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