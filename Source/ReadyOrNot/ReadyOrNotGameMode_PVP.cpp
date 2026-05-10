// Copyright Void Interactive, 2021

#include "ReadyOrNotGameMode_PVP.h"

#include "ReadyOrNotGameSession.h"

#include "HUD/Widgets/RoundEndWidget_PVP.h"

#include "lib/CompetitionHelperLib.h"
#include "lib/ReadyOrNotFunctionLibrary.h"

#if defined(WITH_STEAM)
#include "steam/steam_gameserver.h"
#endif

#if !UE_BUILD_SHIPPING
TAutoConsoleVariable<int32> CVarRonEndGameInstantly(TEXT("a.RonEndGameInstantly"), 0, TEXT("Ends the game instantly for debugging purposes"));
TAutoConsoleVariable<int32> CVarRonSuperLongRespawn(TEXT("a.RonSuperLongRespawn"), 0, TEXT("Sets a super long respawn time for debugging purposes"));
#endif

DECLARE_CYCLE_STAT(TEXT("RoN ~ GM PVP Tick"), STAT_RoNGameModePVPTick, STATGROUP_RONGMPVP);
DECLARE_CYCLE_STAT(TEXT("RoN ~ GM PVP Tick ~ Match State Playing"), STAT_RoNGameModePVPTick_MatchStatePlaying, STATGROUP_RONGMPVP);
DECLARE_CYCLE_STAT(TEXT("RoN ~ GM PVP Tick ~ Match State End Play"), STAT_RoNGameModePVPTick_MatchStateEndPlay, STATGROUP_RONGMPVP);

void AReadyOrNotGameMode_PVP::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	SCOPE_CYCLE_COUNTER(STAT_RoNGameModePVPTick);

	if (AReadyOrNotGameState* GS = Cast<AReadyOrNotGameState>(GetWorld()->GetGameState()))
	{
		switch (GetMatchState())
		{
			case EMatchState::MS_None:
			break;

			case EMatchState::MS_Warmup:
			break;

			case EMatchState::MS_Playing:
			{
				SCOPE_CYCLE_COUNTER(STAT_RoNGameModePVPTick_MatchStatePlaying);

				// give enough time for spawns to happen before calling this
				if (TimeSinceGameStart > 1.0f && !bCalledMatchStart)
				{
					bCalledMatchStart = true;
				}

				if (GetNumPlayers() == 0 && bHasEverHadControllerInGame)
				{
					RestartGame();
				}

				// reinforcements timer..
				if (RespawnMode == ERespawnMode::DelayedRespawn)
				{
					GS->bUseReinforcements = true;

					GS->Reinforcements_TimeRemaining = FMath::Clamp(GS->Reinforcements_TimeRemaining - DeltaSeconds, 0.0f, GS->Reinforcements_TimeRemaining);

					#if !UE_BUILD_SHIPPING
					if (CVarRonSuperLongRespawn.GetValueOnAnyThread() == 1)
					{
						GS->Reinforcements_TimeRemaining += DeltaSeconds * 2.0f;
					}
					#endif
					
					if (GS->Reinforcements_TimeRemaining <= 0)
					{
						RespawnDeadPlayers();

						if (AReadyOrNotGameSession* Session = Cast<AReadyOrNotGameSession>(GameSession))
							GS->Reinforcements_TimeRemaining = Session->ReinforcementTimer;
					}
				}
				else
				{
					if (RespawnMode == ERespawnMode::ImmediateRespawn)
					{
						for (TActorIterator<APlayerController>It(GetWorld()); It; ++It)
						{
							APlayerController* PlayerController = *It;
							AReadyOrNotPlayerState* PlayerState = PlayerController->GetPlayerState<AReadyOrNotPlayerState>();
							if (PlayerState && PlayerState->bReady && !PlayerState->bIsInGame)
							{
								PlayerState->Server_SetIsInGame_Implementation(true);
								RespawnPlayer(PlayerController, false);
							}
						}
					}
					GS->bUseReinforcements = false;
				}

				// check for timelimit
				if (bTimelimitUsedInMode && !GS->bUseTimelimit)
				{
					GS->bUseTimelimit = true;
					GS->RoundTimeRemaining = GetGameModeSettings().Timelimit;
				}
				else if (bTimelimitUsedInMode && GS->bUseTimelimit && GS->RoundTimeRemaining <= 0.0f)
				{
					// Round ended due to timelimit hit
					TimelimitReached();
				}
				else
				{
					if (bTimelimitUsedInMode && GS->bUseTimelimit && ShouldCountDownTimelimitNow())
					{
						GS->RoundTimeRemaining = FMath::Clamp(GS->RoundTimeRemaining - DeltaSeconds, 0.0f, GS->RoundTimeRemaining);
					}

					CheckRoundEnd(DeltaSeconds);
				}
				
				#if !UE_BUILD_SHIPPING
				if (CVarRonEndGameInstantly.GetValueOnAnyThread() == 1)
				{
					if (TimeSinceGameStart > 5.0f)
					{
						GS->RoundsPlayed = GS->RoundsToPlay;

						for (const TActorIterator<AReadyOrNotPlayerState> It(GetWorld()); It;)
						{
							AReadyOrNotPlayerState* PS = *It;
							RoundWonTeam(PS->GetTeam());
							break;
						}
						
					}
				}
				#endif
			}
			break;

			case EMatchState::MS_RoundEnded: case EMatchState::MS_MatchEnded:
			{
				SCOPE_CYCLE_COUNTER(STAT_RoNGameModePVPTick_MatchStateEndPlay);

				// Instantly restart the game if the only reason we are ending play is because no one is left in the game..
				if (GetNumPlayers() == 0)
				{
					NextGame();
				}
				else if (GS->EndPlayTimer <= 0.0f)
				{
					if (GS->GetCurrentSwatScore() < GS->RoundsToPlay && GS->GetCurrentSuspectScore() < GS->RoundsToPlay)
					{
						IncrementRoundsPlayed();
						NextRound();
					}
					else
					{
						GS->NextURLReplicated = GetNextURL();

						NextGame();
					}

					GS->ResetReplicatedTimers();
				}
				else
				{
					GS->EndPlayTimer = FMath::Clamp(GS->EndPlayTimer - DeltaSeconds, 0.0f, GS->EndPlayTimer);
				}
			}
			break;

			case EMatchState::MS_GoingToNextLevel:
			break;

			default:
			break;
		}
	}
}

void AReadyOrNotGameMode_PVP::ResetLevel()
{
	Super::ResetLevel();

	for (TActorIterator<AReadyOrNotPlayerController> It(GetWorld()); It; ++It)
	{
		if (AReadyOrNotPlayerController* PS = *It)
		{
			PS->Client_RemoveWidget(RoundEndWidgetClass);
		}
	}
}

void AReadyOrNotGameMode_PVP::StartMatch()
{
	Super::StartMatch();
	
	UReadyOrNotFunctionLibrary::StartTimerForCallback(CheckVictoryConditions_Handle, this, &AReadyOrNotGameMode_PVP::CheckVictoryConditions, 1.0f, true, false, 1.0f);

	OnRoundStarted();
	
	OnRoundStart.Broadcast();
}

void AReadyOrNotGameMode_PVP::PlayerKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter)
{
	Super::PlayerKilled(InstigatorCharacter, KilledCharacter);
	CheckVictoryConditions();
}

void AReadyOrNotGameMode_PVP::PlayerDowned(AReadyOrNotCharacter* DownedCharacter, AReadyOrNotCharacter* InstigatorCharacter)
{
	Super::PlayerDowned(DownedCharacter, InstigatorCharacter);
	CheckVictoryConditions();
}

void AReadyOrNotGameMode_PVP::PlayerArrested(AReadyOrNotCharacter* ArrestedCharacter, AReadyOrNotCharacter* InstigatorCharacter)
{
	Super::PlayerArrested(ArrestedCharacter, InstigatorCharacter);
	CheckVictoryConditions();
}

void AReadyOrNotGameMode_PVP::RoundEnd()
{
	V_LOGM(LogReadyOrNot, "Round Ended");

	UReadyOrNotFunctionLibrary::StopCallbackTimer(this, CheckVictoryConditions_Handle);

	if (AReadyOrNotGameState* GS = Cast<AReadyOrNotGameState>(GetWorld()->GetGameState()))
	{
		IncrementRoundsPlayed();
		
		// Spawn the round/match end screen for people and flush their keystrokes
		for (TActorIterator<AReadyOrNotPlayerController> It(GetWorld()); It; ++It)
		{
			if (AReadyOrNotPlayerController* PS = *It)
			{
				PS->Client_ClearHUDWidgets();
				PS->Client_CreateWidget("RoundEnd_PVP");
			}
		}

		if (GS->GetCurrentSwatScore() < GS->RoundsToPlay && GS->GetCurrentSuspectScore() < GS->RoundsToPlay)
		{
			GS->EndPlayTimer = 5.0f;

			if (GS->RoundWinningTeam != ETeamType::TT_NONE)
			{
				const ETeamType LosingTeam = GS->RoundWinningTeam == ETeamType::TT_SERT_BLUE ? ETeamType::TT_SERT_RED : ETeamType::TT_SERT_BLUE;
				if (AreAllPlayersOnTeamArrested(LosingTeam) && GetNumberOfArrestedPlayersOnTeam(LosingTeam) > 0)
				{
					GS->PlayAnnouncerForTeam("TeamArrested", GS->RoundWinningTeam);
				}
				else
				{
					GS->PlayAnnouncerForTeam("TeamRoundWin", GS->RoundWinningTeam);
				}
				
				GS->PlayAnnouncerForTeam("EnemyRoundWin", LosingTeam);
			}
			SetMatchState(EMatchState::MS_RoundEnded);
			UReadyOrNotFunctionLibrary::StartTimerForCallback(this, &AReadyOrNotGameMode_PVP::OnRoundEnded, 5.0f, false, true);
		}
		else
		{
			if (GS->BlueTeamWins > GS->RedTeamWins)
			{
				GS->MatchWinningTeam = ETeamType::TT_SERT_BLUE;
			}
			else if (GS->RedTeamWins > GS->BlueTeamWins)
			{
				GS->MatchWinningTeam = ETeamType::TT_SERT_RED;
			}
			else
			{
				GS->MatchWinningTeam = ETeamType::TT_NONE;
			}

			MatchEnd();

			UReadyOrNotFunctionLibrary::StartTimerForCallback(this, &AReadyOrNotGameMode_PVP::OnRoundEnded, 3.0f, false, true);
			SetMatchState(EMatchState::MS_MatchEnded);
		}
	}

	OnRoundEnd.Broadcast();	
}

void AReadyOrNotGameMode_PVP::TimeLimitVictoryConditions_Implementation()
{
}

void AReadyOrNotGameMode_PVP::OnRoundStarted_Implementation()
{
}

void AReadyOrNotGameMode_PVP::OnRoundEnded_Implementation()
{
	for (TActorIterator<AReadyOrNotPlayerController> It(GetWorld()); It; ++It)
	{
		if (AReadyOrNotPlayerController* PS = *It)
		{
			PS->Client_RemoveWidget(RoundEndWidgetClass);
        }
	}
}

void AReadyOrNotGameMode_PVP::Multicast_SetWinningTeam_Implementation(const ETeamType WinningTeam)
{
	if (AReadyOrNotGameState* GS = Cast<AReadyOrNotGameState>(GetWorld()->GetGameState()))
	{
		GS->RoundWinningTeam = WinningTeam;
	}
}

void AReadyOrNotGameMode_PVP::IncrementRoundsPlayed()
{
	if (!bIncrementedRoundCounterThisRound)
	{
		if (AReadyOrNotGameState* GS = Cast<AReadyOrNotGameState>(GetWorld()->GetGameState()))
		{
			GS->RoundsPlayed++;
		}
		
		bIncrementedRoundCounterThisRound = true;
	}
}

void AReadyOrNotGameMode_PVP::CheckRoundEnd_Implementation(float DeltaSeconds)
{
}

void AReadyOrNotGameMode_PVP::RoundWonTeam(ETeamType WinningTeam)
{
	// can't win a round if we're not in the playing state (stops double scoring on end if CheckVictoryConditions is called twice)
	if (GetMatchState() != EMatchState::MS_Playing)
		return;
	
	if (AReadyOrNotGameState* GS = GetGameState<AReadyOrNotGameState>())
	{
		GS->RoundWinningTeam = WinningTeam;
		Multicast_SetWinningTeam(WinningTeam);

		switch (WinningTeam)
		{
			case ETeamType::TT_SERT_BLUE:
				GS->BlueTeamWins++;
			break;

			case ETeamType::TT_SERT_RED:
				GS->RedTeamWins++;
			break;

			default:
			break;
		}

		RoundEnd();
	}
}

void AReadyOrNotGameMode_PVP::RoundWon(TArray<AReadyOrNotPlayerState*> WinningPlayers)
{
	if (AReadyOrNotGameState* GS = GetGameState<AReadyOrNotGameState>())
	{
		GS->RoundWinningTeam = ETeamType::TT_NONE;
		GS->RoundWinners = WinningPlayers;

		RoundEnd();
	}
}

void AReadyOrNotGameMode_PVP::TimelimitReached()
{
	// Reset the timelimit
	if (AReadyOrNotGameState* GS = GetGameState<AReadyOrNotGameState>())
	{
		GS->bUseTimelimit = false; // this will get reset to true on the next MS_Playing tick
	}

	// victory condition - determine a winner based on the current mode and call RoundEnd()
	TimeLimitVictoryConditions();
}

bool AReadyOrNotGameMode_PVP::AnyDeathsOnWinningTeam()
{
	if (AReadyOrNotGameState* GS = GetGameState<AReadyOrNotGameState>())
	{
		for (TActorIterator<AReadyOrNotPlayerState> It(GetWorld()); It; ++It)
		{
			AReadyOrNotPlayerState* PS = *It;
			
			if (PS->GetTeam() == GS->MatchWinningTeam)
			{
				if (PS->GetDeathCount() > 0)
				{
					return true;
				}
			}
		}
	}
	
	return false;
}

void AReadyOrNotGameMode_PVP::MatchEnd()
{
	#if WITH_EDITOR
	V_LOGM(LogReadyOrNot, "Match Ended!");
	#endif

	if (AReadyOrNotGameState* GS = GetGameState<AReadyOrNotGameState>())
	{
		GS->EndPlayTimer = 7.5f;

		//GS->Multicast_PlaySequence(nullptr);
		
		if (GS->MatchWinningTeam != ETeamType::TT_NONE)
		{
			const bool bAnyDeathsOnWinningTeam = AnyDeathsOnWinningTeam();

			GS->PlayAnnouncerForTeam(bAnyDeathsOnWinningTeam ? "TeamWin" : "TeamWinFlawless", GS->MatchWinningTeam);
			GS->PlayAnnouncerForTeam(bAnyDeathsOnWinningTeam ? "EnemyWin" : "EnemyWinFlawless", GS->MatchWinningTeam == ETeamType::TT_SERT_BLUE ? ETeamType::TT_SERT_RED : ETeamType::TT_SERT_BLUE);
		}
	}

	UpdatePlayerLeaderboardStats();
}

void AReadyOrNotGameMode_PVP::NextRound()
{
	if (AReadyOrNotGameState* GS = GetGameState<AReadyOrNotGameState>())
	{
		GS->bUseTimelimit = false;
		
		GS->ResetReplicatedTimers();
		GS->Multicast_StopSequence(nullptr);
		
		if (GetReadyOrNotGameSession())
		{
			GS->Reinforcements_TimeRemaining = GetGameModeSettings().ReinforcementTimer;
		}
	}
	
	SetMatchState(EMatchState::MS_Playing);
	
	ResetLevel();
	RespawnAllPlayers();
	ResetClientScores(true);
	
	bIncrementedRoundCounterThisRound = false;
	
	UReadyOrNotFunctionLibrary::StartTimerForCallback(CheckVictoryConditions_Handle, this, &AReadyOrNotGameMode_PVP::CheckVictoryConditions, 1.0f, true, false, 1.0f);

	OnRoundStarted();

	OnRoundStart.Broadcast();
}

void AReadyOrNotGameMode_PVP::CheckVictoryConditions()
{
	// Implement in game modes for victory conditions
}

void AReadyOrNotGameMode_PVP::UpdatePlayerLeaderboardStats()
{
	if (AReadyOrNotGameState* GS = GetGameState<AReadyOrNotGameState>())
	{
		for (TActorIterator<AReadyOrNotPlayerState> It(GetWorld()); It; ++It)
		{
			AReadyOrNotPlayerState* PS = *It;
			if (PS->GetTeam() == GS->MatchWinningTeam)
			{
				const FString SteamId = PS->GetUniqueId().ToString();
				const FString SteamName = PS->GetPlayerName();
						
				if (IOnlineSubsystem::Get())
				{
					if (IOnlineSessionPtr SessionInt = Online::GetSessionInterface())
					{
#if defined(WITH_STEAM)
						ISteamGameServer* SteamGameServerPtr = SteamGameServer();
						FNamedOnlineSession* Session = SessionInt->GetNamedSession(NAME_GameSession);
						if (Session && SteamGameServerPtr)
						{
							FString ServerName, ServerMode;
							Session->SessionSettings.Get<FString>("ServerName", ServerName);
							Session->SessionSettings.Get<FString>(SETTING_GAMEMODE, ServerMode);
									
							UCompetitionHelperLib::AddWin(EventID, SteamId, SteamName, ServerName, ServerMode);
						}
#endif
					}
				}
			}
		}
	}
}

