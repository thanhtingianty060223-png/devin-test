#include "KingOfTheHillGM.h"
#include "ReadyOrNot.h"
#include "Actors/Gameplay/TugOfWarMover.h"
#include "Actors/Environment/TugOfWarZone.h"

TAutoConsoleVariable<int32> CVarForceWinKingOfTheHill(TEXT("a.ForceWinKingOfTheHill"), 0, TEXT("Force win king of the hill"));

void AKingOfTheHillGM::BeginPlay()
{
	Super::BeginPlay();

	if (MatchLoopMusic)
	{
		UFMODBlueprintStatics::PlayEvent2D(GetWorld(), MatchLoopMusic, true);
	}

}

void AKingOfTheHillGM::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (GetMatchState() == EMatchState::MS_Playing && CVarForceWinKingOfTheHill.GetValueOnAnyThread() == 1)
	{
		RoundWonTeam(ETeamType::TT_SERT_RED);
	}
}

void AKingOfTheHillGM::StartMatch()
{
	Super::StartMatch();

	if (MatchStartMusic)
	{
		UFMODBlueprintStatics::PlayEvent2D(GetWorld(), MatchStartMusic, true);
	}
}

void AKingOfTheHillGM::RoundEnd()
{
	Super::RoundEnd();
	
}

void AKingOfTheHillGM::MatchEnd()
{
	Super::MatchEnd();

	if (MatchEndMusic)
	{
		UFMODBlueprintStatics::PlayEvent2D(GetWorld(), MatchEndMusic, true);
	}
}

void AKingOfTheHillGM::ResetClientScores(bool bBetweenRounds)
{
	if (bBetweenRounds)
	{
		return;
	}

	Super::ResetClientScores(bBetweenRounds);
}

void AKingOfTheHillGM::ResetLevel()
{
	Super::ResetLevel();
	for (TActorIterator<ATugOfWarMover> It(GetWorld()); It; ++It)
	{
		// reset the mover
		ATugOfWarMover* tm = *It;
		tm->MoverCurrentPosition = 0.5f;
		tm->MoverStartingPosition = 0.5f;
	}
	TArray<ATugOfWarZone*> Zones;
	for (TActorIterator<ATugOfWarZone> It(GetWorld()); It; ++It)
	{
		ATugOfWarZone* zo = *It;
		zo->bZoneDisabled = true;
		Zones.Add(zo);
	}
	if (Zones.Num() > 0)
	{
		Zones[FMath::RandRange(0, Zones.Num() - 1)]->bZoneDisabled = false;
	}
	
}

bool AKingOfTheHillGM::AreAllPlayersOnTeamArrested(ETeamType Team)
{
	TArray<AActor*> Actors;

	bool bAllPlayersArrested = true;

	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerCharacter::StaticClass(), Actors);
	for (int32 i = 0; i < Actors.Num(); i++)
	{
		APlayerCharacter* pc = Cast<APlayerCharacter>(Actors[i]);
		if (pc && pc->GetTeam() == Team && !pc->IsDeadOrUnconscious() && !pc->IsArrested() && pc->GetController())
		{
			bAllPlayersArrested = false;
			break;
		}
	}

	return bAllPlayersArrested && DeadPlayers.Num() == 0;
}

int32 AKingOfTheHillGM::GetNumberOfArrestedPlayersOnTeam(ETeamType Team)
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

void AKingOfTheHillGM::PlayerArrested(AReadyOrNotCharacter* ArrestedCharacter, AReadyOrNotCharacter* InstigatorCharacter)
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
		// // give godmode to the arrested character so they can't be killed
		// ArrestedCharacter->bNoTeamDamage = true;

		return;
	}

	
}

void AKingOfTheHillGM::PlayerFreed(ACharacter* Freed, ACharacter* Freer)
{
	APlayerCharacter* FreedPc = Cast<APlayerCharacter>(Freed);
	if (!FreedPc)
	{
		return;
	}

	// // Turn off god mode for the freed player
	// FreedPc->bGodMode = false;

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

void AKingOfTheHillGM::CheckVictoryConditions()
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

void AKingOfTheHillGM::TimeLimitVictoryConditions_Implementation()
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

AActor* AKingOfTheHillGM::FindPlayerStart_Implementation(AController* Player, const FString& IncomingName /* = TEXT("") */)
{
	if (!Player)
		return nullptr;

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