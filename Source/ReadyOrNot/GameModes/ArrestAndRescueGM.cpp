#include "ArrestAndRescueGM.h"
#include "ReadyOrNot.h"
#include "Characters/PlayerCharacter.h"
#include "ArrestAndRescueGS.h"
#include "ReadyOrNotGameSession.h"
#include "ReadyOrNotGameMode.h"

class AArrestAndRescueGS* AArrestAndRescueGM::GetAARGS()
{
	return Cast<AArrestAndRescueGS>(GetWorld()->GetGameState());
}

void AArrestAndRescueGM::BeginPlay()
{
	Super::BeginPlay();
}

void AArrestAndRescueGM::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if ((GetAARGS()->RedRespawnWaves == 0 && GetAARGS()->BlueRespawnWaves == 0) || DeadPlayers.Num() == 0)
	{
		if (GetReadyOrNotGameSession())
		{
			GetReadyOrNotGameSession()->ReinforcementTimer = 5;
		}
		GetAARGS()->Reinforcements_TimeRemaining = 5;
	}

	GetAARGS()->RedRespawnWaves = FMath::Max(GetAARGS()->RedRespawnWaves, 0);
	GetAARGS()->BlueRespawnWaves = FMath::Max(GetAARGS()->BlueRespawnWaves, 0);
	
}

void AArrestAndRescueGM::StartMatch()
{
	Super::StartMatch();
}

void AArrestAndRescueGM::RoundEnd()
{
	Super::RoundEnd();

	bSuddenDeath = false;
}

void AArrestAndRescueGM::MatchEnd()
{
	Super::MatchEnd();
}

void AArrestAndRescueGM::ResetLevel()
{
	Super::ResetLevel();
	GetAARGS()->BlueRespawnWaves = 1;
	GetAARGS()->RedRespawnWaves = 1;
}

void AArrestAndRescueGM::PlayerFreed(ACharacter* Freed, ACharacter* Freer)
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



void AArrestAndRescueGM::TimeLimitVictoryConditions_Implementation()
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
		// announce sudden death
		bSuddenDeath = true;

		// post status to chat - fixme, this needs to be localized
		FRChatMessage Message;

		Message.Color = FColor::White;
		Message.Message = "Sudden death!";
		gs->Multicast_BroadcastChatMessage(Message);
	}
}

void AArrestAndRescueGM::RespawnPlayer(APlayerController* Player, bool bForceSpectator /* = false */)
{
	AReadyOrNotPlayerState* ps = Cast<AReadyOrNotPlayerState>(Player->PlayerState);
	if (ps)
	{
		ps->TrySetPendingTeamAsTeam();

		// check this in case no body is actually on the team and spawned (allow them to  spawn in that case because otherwise the game can't continue)
		int32 Count = 0;
		for (TActorIterator<APlayerCharacter>It(GetWorld()); It; ++It)
		{
			APlayerCharacter* pc = *It;
			if (pc->GetTeam() == ps->Team)
			{
				Count++;
			}
		}

		if (ps->GetTeam() == ETeamType::TT_SERT_RED)
		{
			
			if (GetAARGS()->RedRespawnWaves == 0 && Count > 0)
				return;
		}
		else if (ps->GetTeam() == ETeamType::TT_SERT_BLUE)
		{
			if (GetAARGS()->BlueRespawnWaves == 0  && Count > 0)
				return;
		}
	}
	Super::RespawnPlayer(Player);
}

void AArrestAndRescueGM::RespawnAllPlayersOnTeam(ETeamType Team)
{
	Super::RespawnAllPlayersOnTeam(Team);
	GetAARGS()->RedRespawnWaves--;
	GetAARGS()->BlueRespawnWaves--;
}

void AArrestAndRescueGM::RespawnAllPlayers()
{
	Super::RespawnAllPlayers();
	GetAARGS()->RedRespawnWaves--;
	GetAARGS()->BlueRespawnWaves--;
}

void AArrestAndRescueGM::RespawnDeadPlayers()
{
	bool bHasDeadRedPlayer = false;
	bool bHasDeadBluePlayer = false;
	for (int32 i = 0; i < DeadPlayers.Num(); i++)
	{
		if (!DeadPlayers[i])
			continue;
		AReadyOrNotPlayerState* ps = Cast<AReadyOrNotPlayerState>(DeadPlayers[i]->PlayerState);
		if (ps)
		{
			if (ps->GetTeam() == ETeamType::TT_SERT_RED)
			{
				bHasDeadRedPlayer = true;
			}
			else if (ps->GetTeam() == ETeamType::TT_SERT_BLUE)
			{
				bHasDeadBluePlayer = true;
			}
		}
	}

	Super::RespawnDeadPlayers();
	if (bHasDeadRedPlayer) GetAARGS()->RedRespawnWaves--;
	if (bHasDeadBluePlayer) GetAARGS()->BlueRespawnWaves--;
}

void AArrestAndRescueGM::CheckVictoryConditions()
{
	AReadyOrNotGameState* gs = GetGameState<AReadyOrNotGameState>();
	if (!gs)
	{
		return;
	}

	if (GetMatchState() != EMatchState::MS_Playing)
	{
		V_LOGM(LogReadyOrNot, "Game ended, unable to check victory conditions!");
		return;
	}

	V_LOGM(LogReadyOrNot, "Checking Victory Conditions!");

	for (int32 i = 0; i < DeadPlayers.Num(); i++)
	{
		if (!DeadPlayers[i])
			continue;
			
		AReadyOrNotPlayerState* ps = Cast<AReadyOrNotPlayerState>(DeadPlayers[i]->PlayerState);
		if (ps)
		{
			// can't win while dead players exist and respwawn wave is valid...
			if (ps->GetTeam() == ETeamType::TT_SERT_BLUE && GetAARGS()->BlueRespawnWaves > 0)
			{
				V_LOGM(LogReadyOrNot, "Wave available on blue and pending respawn... no win!");
				return;
			}
			if (ps->GetTeam() == ETeamType::TT_SERT_RED && GetAARGS()->RedRespawnWaves > 0)
			{
				V_LOGM(LogReadyOrNot, "Wave available on red and pending respawn... no win!");
				return;
			}
		}
	}
	if (GetNumberOfFreePlayersOnTeam(ETeamType::TT_SERT_RED) == 0 && GetNumberOfFreePlayersOnTeam(ETeamType::TT_SERT_BLUE) == 0)
	{
	
		V_LOGM(LogReadyOrNot, "No one alive on either team... draw!");
		RoundWonTeam(ETeamType::TT_NONE);
	} else	if (GetNumberOfFreePlayersOnTeam(ETeamType::TT_SERT_BLUE) == 0)
	{
		V_LOGM(LogReadyOrNot, "No one alive on blue team... red team wins!");
		// all players on red arrested = blue victory
		RoundWonTeam(ETeamType::TT_SERT_RED);
	} else	if (GetNumberOfFreePlayersOnTeam(ETeamType::TT_SERT_RED) == 0)
	{
		V_LOGM(LogReadyOrNot, "No one alive on red team... blue team wins!");
		// all players on red arrested = blue victory
		RoundWonTeam(ETeamType::TT_SERT_BLUE);
	} else
	{
		V_LOGM(LogReadyOrNot, "No win conditions met");
		V_LOGM(LogReadyOrNot, "Free Players on Red: %s", *FString::FromInt(GetNumberOfFreePlayersOnTeam(ETeamType::TT_SERT_RED)));
		V_LOGM(LogReadyOrNot, "Free Players on Blue: %s", *FString::FromInt(GetNumberOfFreePlayersOnTeam(ETeamType::TT_SERT_BLUE)));
	}
}

void AArrestAndRescueGM::PlayerArrested(AReadyOrNotCharacter* ArrestedCharacter, AReadyOrNotCharacter* InstigatorCharacter)
{
	CheckVictoryConditions();
	AReadyOrNotPlayerState* ps = Cast<AReadyOrNotPlayerState>(ArrestedCharacter->GetPlayerState());
	if (ps)
	{
		ps->TimesArrested += 1;

		if (InstigatorCharacter)
		{
			ps = Cast<AReadyOrNotPlayerState>(InstigatorCharacter->GetPlayerState());
			if (ps)
			{
				if (ps->GetTeam() == ETeamType::TT_SERT_BLUE) 
				{
					GetAARGS()->BlueRespawnWaves++;
				}
				else if (ps->GetTeam() == ETeamType::TT_SERT_RED)
				{
					GetAARGS()->RedRespawnWaves++;
				}
				ps->ArrestsThisLife += 1;
				ps->Arrests += 1;
				ps->SetScore(ps->GetScore() + 25);
			}
		}

		// ArrestedCharacter->bArrestedButFreeable = true;
		//
		// // give godmode to the arrested character so they can't be killed
		// ArrestedCharacter->bGodMode = true;

		return;
	}
	
	if (VIPArrestedSound)
	{
		UFMODBlueprintStatics::PlayEvent2D(GetWorld(), VIPArrestedSound, true);
	}

	Super::PlayerArrested(ArrestedCharacter, InstigatorCharacter);

}

void AArrestAndRescueGM::PlayerKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter)
{
	Super::PlayerKilled(InstigatorCharacter, KilledCharacter);
	CheckVictoryConditions();
}

AActor* AArrestAndRescueGM::FindPlayerStart_Implementation(AController* Player, const FString& IncomingName)
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