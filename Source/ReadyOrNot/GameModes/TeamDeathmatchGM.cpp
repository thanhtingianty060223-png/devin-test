// ÂCopyright Void Interactive, 2017

#include "TeamDeathmatchGM.h"
#include "ReadyOrNot.h"
#include "TeamDeathmatchGS.h"



void ATeamDeathmatchGM::BeginPlay()
{
	Super::BeginPlay();

	if (MatchLoopMusic)
	{
		UFMODBlueprintStatics::PlayEvent2D(GetWorld(), MatchLoopMusic, true);
	}
}

void ATeamDeathmatchGM::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
}

void ATeamDeathmatchGM::StartMatch()
{
	Super::StartMatch();

	if (MatchStartMusic)
	{
		UFMODBlueprintStatics::PlayEvent2D(GetWorld(), MatchStartMusic, true);
	}
}

void ATeamDeathmatchGM::RoundEnd()
{
	Super::RoundEnd();

	bSuddenDeath = false;
}

void ATeamDeathmatchGM::MatchEnd()
{
	Super::MatchEnd();

	if (MatchEndMusic)
	{
		UFMODBlueprintStatics::PlayEvent2D(GetWorld(), MatchEndMusic, true);
	}
// 	else if (MatchLoopMusic)
// 	{
// 		UFMODBlueprintStatics::EventInstanceStop(MatchLoopMusic, true);
// 	}
	
}

void ATeamDeathmatchGM::PlayerArrested(AReadyOrNotCharacter* ArrestedCharacter, AReadyOrNotCharacter* InstigatorCharacter)
{
	AReadyOrNotGameState* gs = GetGameState<AReadyOrNotGameState>();
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
		CheckWinConditions();
	}
}

void ATeamDeathmatchGM::PlayerKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter)
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
		CheckWinConditions();
	}
}

void ATeamDeathmatchGM::CheckWinConditions()
{
	AReadyOrNotGameState* gs = GetWorld()->GetGameState<AReadyOrNotGameState>();
	if (gs && gs->Scorelimit > 0)
	{
		if (GetMatchState() == EMatchState::MS_Playing)
		{
			if (gs->GetTeamScore(ETeamType::TT_SERT_RED) >= gs->Scorelimit && gs->GetTeamScore(ETeamType::TT_SERT_BLUE) >= gs->Scorelimit)
			{
				RoundWonTeam(ETeamType::TT_NONE);
			}
			else if (gs->GetTeamScore(ETeamType::TT_SERT_RED) >= gs->Scorelimit)
			{
				RoundWonTeam(ETeamType::TT_SERT_RED);
			}
			else if (gs->GetTeamScore(ETeamType::TT_SERT_BLUE) >= gs->Scorelimit)
			{
				RoundWonTeam(ETeamType::TT_SERT_BLUE);
			}
		}
	}
}

void ATeamDeathmatchGM::TimeLimitVictoryConditions_Implementation()
{
	Super::TimeLimitVictoryConditions_Implementation();

	ATeamDeathmatchGS* gs = GetWorld()->GetGameState<ATeamDeathmatchGS>();
	if (gs)
	{
		if (gs->GetTeamScore(ETeamType::TT_SERT_RED) > gs->GetTeamScore(ETeamType::TT_SERT_BLUE))
		{
			RoundWonTeam(ETeamType::TT_SERT_RED);
		}
		else if (gs->GetTeamScore(ETeamType::TT_SERT_BLUE) > gs->GetTeamScore(ETeamType::TT_SERT_RED))
		{
			RoundWonTeam(ETeamType::TT_SERT_BLUE);
		}
		else
		{
			bSuddenDeath = true;

			// post status to chat - fixme, this needs to be localized
			FRChatMessage Message;

			Message.Color = FColor::White;
			Message.Message = "Sudden death!";
			gs->Multicast_BroadcastChatMessage(Message);
		}
	}
}

AActor* ATeamDeathmatchGM::FindPlayerStart_Implementation(AController* Player, const FString& IncomingName /* = TEXT("") */)
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
