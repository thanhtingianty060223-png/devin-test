// ÂCopyright Void Interactive, 2017

#include "VIPEscortGM.h"
#include "VIPEscortGS.h"

#include "ReadyOrNot.h"
#include "ReadyOrNotGameSession.h"

#include "Actors/Gameplay/ReadyOrNotPlayerState.h"
#include "Actors/PlayerStart_VIP_Spawn.h"

#include "Characters/ReadyOrNotPlayerController.h"
#include "Info/LoadoutManager.h"

#include "lib/ReadyOrNotFunctionLibrary.h"

DECLARE_CYCLE_STAT(TEXT("RoN ~ VIP GM Tick"), STAT_RONVIPTick, STATGROUP_RONVIPGM);
DECLARE_CYCLE_STAT(TEXT("RoN ~ VIP GM Tick ~ Match State Playing"), STAT_RONVIPTick_MatchStatePlaying, STATGROUP_RONVIPGM);

void AVIPEscortGM::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	SCOPE_CYCLE_COUNTER(STAT_RONVIPTick);

	if (GetMatchState() == EMatchState::MS_MatchEnded || GetMatchState() == EMatchState::MS_RoundEnded || GetMatchState() == EMatchState::MS_GoingToNextLevel)
	{
		bVIPInitialized = false;
		
		return;
	}

	if (AVIPEscortGS* VIPGS = GetGameState<AVIPEscortGS>())
	{
		if (GetMatchState() == EMatchState::MS_Playing)
		{
			SCOPE_CYCLE_COUNTER(STAT_RONVIPTick_MatchStatePlaying);
			
			//if (!bVIPInitialized)
			//{
			//	InitializeVIPRound();
			//}

			if (IsVIPArrested())
			{
				VIPGS->HoldVIP_TimeRemaining = FMath::Clamp(VIPGS->HoldVIP_TimeRemaining - DeltaSeconds, 0.0f, VIPGS->HoldVIP_TimeRemaining);
				
				//if (VIPGS->UserHud1_Type == NAME_None)
				//{
				//	// we're transitioning to a state where the VIP has been detained - announce it!
				//	VIPGS->PlayAnnouncerForTeam("VIPTaken", ETeamType::TT_NONE);
				//	VIPGS->UserHud1_Type = TEXT("VIP_HoldTime");
				//}
			}
			else
			{
				VIPGS->HoldVIP_TimeRemaining = HostageHoldTime;
				//VIPGS->UserHud1_Type = NAME_None;
			}

			if (IsVIPAlive())
			{
#if WITH_EDITOR
				VIPGS->HoldVIP_TimeRemaining = FMath::Min(20.0f, VIPGS->HoldVIP_TimeRemaining);
#endif
				if (IsVIPArrested() && VIPGS->HoldVIP_TimeRemaining <= 0.0f)
				{
					if (!VIPGS->bCanKillVIP)
					{
						//VIPGS->VIPCharacter->Multicast_StartShowingObjectiveMarker(EPlayerObjectiveMarkerType::POMT_VipExecute, ETeamType::TT_SERT_RED);
					}
					
					VIPGS->bCanKillVIP = true;
				}
				else
				{
					VIPGS->bCanKillVIP = false;
				}
			}
		}
	}

	// VIP Shouldn't respawn.. make sure it doesn't enter the dead players list.
	RemoveDeadPlayer(VIPPlayer);
}

AActor* AVIPEscortGM::ChoosePlayerStart_Implementation(AController* Player)
{
	if (!Player)
	{
		return nullptr;
	}

	if (AReadyOrNotPlayerState* PS = Cast<AReadyOrNotPlayerState>(Player->PlayerState))
	{
		switch (PS->GetTeam())
		{
			case ETeamType::TT_SERT_RED:
				return FindPlayerStartWithTag(SWATRedStartTag);

			case ETeamType::TT_SERT_BLUE:
				return FindPlayerStartWithTag(SWATBlueStartTag);

			default:
				return FindPlayerStartWithTag(CurrentVIPTeam == ETeamType::TT_SERT_BLUE ? SWATBlueStartTag : SWATRedStartTag);
		}
	}

	return FindPlayerStartWithTag(CurrentVIPTeam == ETeamType::TT_SERT_BLUE ? SWATBlueStartTag : SWATRedStartTag);
}

void AVIPEscortGM::ResetLevel()
{
	Super::ResetLevel();

	if (AVIPEscortGS* VIPGS = GetGameState<AVIPEscortGS>())
	{
		VIPGS->OnResetLevel();

		VIPGS->HoldVIP_TimeRemaining = HostageHoldTime;
		VIPGS->VIPPlayer = nullptr;
		VIPGS->VIPPlayerState = nullptr;
		VIPGS->VIPCharacter = nullptr;

		VIPPlayer = nullptr;
		
		if (AReadyOrNotGameSession* Session = Cast<AReadyOrNotGameSession>(GameSession))
		{
			VIPGS->Reinforcements_TimeRemaining = Session->ReinforcementTimer;
		}
	}

	ChooseVIPSpawn();
}

void AVIPEscortGM::OnRoundStarted_Implementation()
{
	Super::OnRoundStarted_Implementation();

	if (AVIPEscortGS* VIPGS = GetGameState<AVIPEscortGS>())
	{
		if (VIPGS->RoundsPlayed > 0)
		{
			SwapSides();
		}
		else
		{
			// Choose a random team between SERT_RED and SERT_BLUE
			CurrentVIPTeam = static_cast<ETeamType>(static_cast<uint8>(FMath::RandRange(1, 2)));
		}
		
		VIPGS->CurrentVIPTeam = CurrentVIPTeam;
	}
	
	InitializeVIPRound();
}

void AVIPEscortGM::TimeLimitVictoryConditions_Implementation()
{
	RoundWonTeam(ETeamType::TT_SERT_RED); // suspects always win if timelimit runs out
}

bool AVIPEscortGM::ShouldCountDownTimelimitNow()
{
	// Don't remove timelimit when the VIP is in custody
	return !IsVIPArrested();
}

void AVIPEscortGM::RespawnPlayer(APlayerController* Player, const bool bForceSpectator)
{
	if (Player == VIPPlayer && GetMatchState() == EMatchState::MS_Playing)
	{
		if (AReadyOrNotPlayerState* PS = Cast<AReadyOrNotPlayerState>(Player->PlayerState))
		{
			FSavedLoadout VIPLoadout;
			VIPLoadout.Primary = PS->ServerSavedLoadout.Primary;
			VIPLoadout.Secondary = ULoadoutManager::GetItemByLookupIdx(GetWorld(), "M1911");
			VIPLoadout.SecondaryScope = PS->ServerSavedLoadout.SecondaryScope;
			VIPLoadout.SecondaryMuzzle = PS->ServerSavedLoadout.SecondaryMuzzle;
			VIPLoadout.SecondaryUnderbarrel = PS->ServerSavedLoadout.SecondaryUnderbarrel;
			VIPLoadout.SecondaryOverbarrel = PS->ServerSavedLoadout.SecondaryOverbarrel;
			VIPLoadout.SecondaryStock = PS->ServerSavedLoadout.SecondaryStock;
			VIPLoadout.SecondaryGrip = PS->ServerSavedLoadout.SecondaryGrip;
			VIPLoadout.SecondaryIlluminator = PS->ServerSavedLoadout.SecondaryIlluminator;
			VIPLoadout.SecondaryAmmoSlots = PS->ServerSavedLoadout.PrimaryAmmoSlots;
			VIPLoadout.SecondaryAmmoSlotsCount = PS->ServerSavedLoadout.PrimaryAmmoSlotsCount;
			VIPLoadout.SecondarySkin = PS->ServerSavedLoadout.SecondarySkin;
			
			PS->bSpawnLoadout = true;
			PS->Server_SetLoadout(VIPLoadout);
		}
		
		SpawnVIPPlayer();
	}
	else
	{
		if (AReadyOrNotPlayerState* RONPS = Cast<AReadyOrNotPlayerState>(Player->PlayerState))
		{
			RONPS->bSpawnLoadout = true;
			RONPS->Server_SetLoadout(RONPS->ServerSavedLoadout);
			
			if (RONPS->Deaths < 3)
				Super::RespawnPlayer(Player, bForceSpectator);
		}
	}
}

void AVIPEscortGM::InitializeVIPRound()
{
	ChooseNewVIPPlayer();
	SpawnVIPPlayer();

	if (AVIPEscortGS* VIPGS = GetGameState<AVIPEscortGS>())
	{
		VIPGS->RoundTimeRemaining = TimeToDeliverVIP;
		VIPGS->HoldVIP_TimeRemaining = HostageHoldTime;

		VIPGS->VIPPlayer = VIPPlayer;
		VIPGS->VIPCharacter = GetVIPCharacter();
	}
	
	bVIPInitialized = true;

	#if WITH_EDITOR
	ensureAlways(VIPPlayer && VIPPlayer->PlayerState);
	#endif
}

void AVIPEscortGM::UpdateVIPAndEnemyTransforms()
{
	//if (!ChosenVIPSpawn)
	//{
	//	if (!ChooseVIPSpawn())
	//		return;
	//}

	if (APlayerCharacter* VIPChar = GetVIPCharacter())
	{
		APlayerStart* PlayerStart = FindPlayerStartWithTag(CurrentVIPTeam == ETeamType::TT_SERT_BLUE ? SWATBlueStartTag : SWATRedStartTag);

		VIPChar->SetActorLocation(PlayerStart->GetActorLocation());
		VIPChar->SetActorRotation(PlayerStart->GetActorRotation());
	}

	//TArray<APlayerCharacter*> PlayerCharacters = GetAllPlayerCharactersInWorld();
	//for (APlayerCharacter* Character : PlayerCharacters)
	//{
	//	if (Character->IsOnSuspectTeam())
	//	{
	//		Character->SetActorLocation(ChosenVIPSpawn->GetRandomSpawnPoint());
	//		Character->SetActorRotation(ChosenVIPSpawn->GetSpawnDirection());
	//	}
	//}
}

void AVIPEscortGM::ChooseNewVIPPlayer()
{
	// Find all compatible players
	TArray<APlayerController*> CompatiblePlayers = GetAllCompatiblePlayersForVIP();
	const int32 NumOfCompatiblePlayers = CompatiblePlayers.Num();
	
	// Choose a random player who was never a vip
	if (NumOfCompatiblePlayers > 0)
	{
		if (AllPlayersWereVIP())
		{
			ResetVIPFlags();

			VIPPlayer = CompatiblePlayers[FMath::RandRange(0, NumOfCompatiblePlayers - 1)];
		}
		else
		{
			do
			{
				VIPPlayer = CompatiblePlayers[FMath::RandRange(0, NumOfCompatiblePlayers - 1)];
			}
			while (VIPPlayer && Cast<AReadyOrNotPlayerState>(VIPPlayer)->bWasVIP);
		}

		if (AReadyOrNotPlayerState* PS = Cast<AReadyOrNotPlayerState>(VIPPlayer))
		{
			PS->bIsVIP = true;
			PS->bWasVIP = true;
		}
	}
	// no compatible players?
	else
	{
		VIPPlayer = nullptr;
		
		if (AVIPEscortGS* VIPGS = GetGameState<AVIPEscortGS>())
		{
			VIPGS->VIPPlayer = nullptr;
			VIPGS->VIPPlayerState = nullptr;
			VIPGS->VIPCharacter = nullptr;
		}

		#if !UE_BUILD_SHIPPING
		ULog::Error("No compatible players for VIP");
		#endif
	}
}

void AVIPEscortGM::SpawnVIPPlayer()
{
	if (!VIPPlayer /*|| !ChosenVIPSpawn*/)
		return;

	DestroyVIPCharacter();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	SpawnParams.bNoFail = true;

	APlayerStart* PlayerStart = FindPlayerStartWithTag(CurrentVIPTeam == ETeamType::TT_SERT_BLUE ? SWATBlueStartTag : SWATRedStartTag);

	// Spawn the vip character
	if (APlayerCharacter* VIPChar = GetWorld()->SpawnActor<APlayerCharacter>(VIPCharacterClass, (PlayerStart ? PlayerStart->GetActorTransform() : ChoosePlayerStart_Implementation(VIPPlayer)->GetActorTransform()), SpawnParams))
	{
		// Equip its loadout
		if (AReadyOrNotPlayerState* PS = Cast<AReadyOrNotPlayerState>(VIPPlayer->PlayerState))
		{
			PS->bSpawnLoadout = true;
			
			if (AVIPEscortGS* VIPGS = GetGameState<AVIPEscortGS>())
				VIPGS->VIPPlayerState = PS;
		}

		VIPChar->bSpawnInventoryItemsOnPossess = true;

		VIPPlayer->Possess(VIPChar);
		
		VIPChar->CurrentRunSpeedPercent = VIPChar->MaxRunSpeedPercent;
		VIPChar->LastSetRunSpeed = VIPChar->MaxRunSpeedPercent;
		VIPChar->bDisableSprinting = true;
		VIPChar->SetWalkSpeed(VIPChar->GetRunSpeed() * VIPChar->MaxRunSpeedPercent, VIPChar->GetRunSpeed() * VIPChar->MaxRunSpeedPercent * VIPChar->SpeedModifier_Crouch);

		//VIPChar->bArrestComplete = true;
		//VIPChar->bArrestedButFreeable = true;
		//VIPChar->Multicast_CenterPrint(TEXT("YouAreTheVIP"), 3.0f, nullptr);

		// Bind crucial events
		VIPChar->OnCharacterKilled.AddDynamic(this, &AVIPEscortGM::VIPKilled);
		VIPChar->OnCharacterKilled.AddDynamic(this, &AReadyOrNotGameMode::PlayerKilled);
		VIPChar->OnPlayerArrested.AddDynamic(this, &AVIPEscortGM::PlayerArrested);
		VIPChar->OnPlayerFreed.AddDynamic(this, &AVIPEscortGM::VIPFreed);

		// VIPPlayer HUD setup 
		if (AReadyOrNotPlayerController* PC = Cast<AReadyOrNotPlayerController>(VIPPlayer))
		{
			PC->Client_SetControlRotation(VIPChar->GetActorRotation());
			PC->Client_DisableUIMouse();
			PC->Client_ClearHUDWidgets();
		}


		// Bind character events
		// Assign the vip character that was spawned
		if (AVIPEscortGS* VIPGS = GetGameState<AVIPEscortGS>())
		{
			VIPGS->Client_BindCharacterEvents(VIPChar);

			VIPGS->VIPCharacter = VIPChar;
		}
	}
}

void AVIPEscortGM::SwapSides()
{
	CurrentVIPTeam = (CurrentVIPTeam == ETeamType::TT_SERT_BLUE ? ETeamType::TT_SERT_RED : ETeamType::TT_SERT_BLUE);
}

APlayerCharacter* AVIPEscortGM::GetVIPCharacter() const
{
	return VIPPlayer ? Cast<APlayerCharacter>(VIPPlayer->GetPawn()) : nullptr;
}

TArray<APlayerStart_VIP_Spawn*> AVIPEscortGM::GetAllVIPSpawnsInWorld()
{
	return UReadyOrNotFunctionLibrary::GetActorsOfClass<APlayerStart_VIP_Spawn>(GetWorld());
}

bool AVIPEscortGM::ChooseVIPSpawn()
{
	TArray<APlayerStart_VIP_Spawn*> VIPSpawns = GetAllVIPSpawnsInWorld();
	
	if (VIPSpawns.Num() > 0)
	{
		if (HasVisitedAllVIPSpawns())
		{
			ResetAllVIPSpawnVisits();
		}

		// Choose a random unvisited spawn
		do
		{
			ChosenVIPSpawn = VIPSpawns[FMath::RandRange(0, VIPSpawns.Num() - 1)];
		}
		while (ChosenVIPSpawn->bHasVisited);

		ChosenVIPSpawn->bHasVisited = true;
	}

	return ChosenVIPSpawn != nullptr;
}

void AVIPEscortGM::DestroyVIPCharacter()
{
	if (VIPPlayer)
	{
		if (APawn* VIPChar = VIPPlayer->GetPawn())
		{
			VIPChar->Destroy();
			VIPPlayer->UnPossess();
		}
		
		if (AReadyOrNotPlayerState* VIPPS = VIPPlayer->GetPlayerState<AReadyOrNotPlayerState>())
			VIPPS->bIsVIP = false;
		
		if (AVIPEscortGS* VIPGS = GetGameState<AVIPEscortGS>())
			VIPGS->VIPPlayer = nullptr;
	}

	if (AVIPEscortGS* VIPGS = GetGameState<AVIPEscortGS>())
		VIPGS->VIPCharacter = nullptr;
}

bool AVIPEscortGM::AllPlayersWereVIP()
{
	TArray<APlayerController*> PlayerControllers = UReadyOrNotFunctionLibrary::GetActorsOfClass<APlayerController>(GetWorld());
	UReadyOrNotFunctionLibrary::RemoveAllNullElements(PlayerControllers);
	
	bool bAllWereVIP = true;
	for (APlayerController* Player : PlayerControllers)
	{
		if (AReadyOrNotPlayerState* PS = Cast<AReadyOrNotPlayerState>(Player))
		{
			if (!PS->bWasVIP)
			{
				bAllWereVIP = false;
				break;
			}
		}
	}

	return bAllWereVIP;
}

void AVIPEscortGM::ResetVIPFlags()
{
	TArray<APlayerController*> PlayerControllers = UReadyOrNotFunctionLibrary::GetActorsOfClass<APlayerController>(GetWorld());
	UReadyOrNotFunctionLibrary::RemoveAllNullElements(PlayerControllers);
	
	for (APlayerController* Player : PlayerControllers)
	{
		if (AReadyOrNotPlayerState* PS = Cast<AReadyOrNotPlayerState>(Player))
		{
			PS->bIsVIP = false;
			PS->bWasVIP = false;
		}
	}
}

void AVIPEscortGM::PlayerArrested(AReadyOrNotCharacter* ArrestedCharacter, AReadyOrNotCharacter* InstigatorCharacter)
{
	if (AVIPEscortGS* VIPGS = GetGameState<AVIPEscortGS>())
	{
		VIPGS->RecentArrester = InstigatorCharacter;
		VIPGS->bVIPArrested = true;
	}

	if (VIPPlayer && GetVIPCharacter() == ArrestedCharacter)
	{
		if (AReadyOrNotPlayerState* PS = Cast<AReadyOrNotPlayerState>(ArrestedCharacter->GetPlayerState()))
		{
			PS->TimesArrested += 1;
		}

		if (InstigatorCharacter)
		{
			if (AReadyOrNotPlayerState* InstigatorPS = Cast<AReadyOrNotPlayerState>(InstigatorCharacter->GetPlayerState()))
			{
				InstigatorPS->ArrestsThisLife += 1;
				InstigatorPS->Arrests += 1;
				InstigatorPS->IncreaseScore(25);
			}
		}
		//ArrestedCharacter->Multicast_StartShowingObjectiveMarker(EPlayerObjectiveMarkerType::POMT_VipRescue, ETeamType::TT_SERT_BLUE);

		ENQUEUE_INGAMELOG_MESSAGE_PVP({InstigatorCharacter, ArrestedCharacter, EPVPEvent::VIPArrested});

		if (AreAllPlayersOnTeamDead(GetVIPCharacter()))
		{
			RoundWonTeam(GetVIPCharacter()->GetTeam() == ETeamType::TT_SERT_BLUE ? ETeamType::TT_SERT_RED : ETeamType::TT_SERT_BLUE);
		}

		return;
	}

	Super::PlayerArrested(ArrestedCharacter, InstigatorCharacter);
}

void AVIPEscortGM::VIPFreed(ACharacter* Freed, ACharacter* Freer)
{
	if (AVIPEscortGS* VIPGS = GetGameState<AVIPEscortGS>())
	{
		VIPGS->PlayAnnouncerForTeam("VIPRescued", ETeamType::TT_NONE);

		VIPGS->RecentFreer = Cast<APlayerCharacter>(Freer);
		VIPGS->bVIPArrested = false;
	}

	// Add 25 points to the person who freed the VIP
	if (Freer)
	{
		if (AReadyOrNotPlayerState* PS = Cast<AReadyOrNotPlayerState>(Freer->GetPlayerState()))
		{
			PS->IncreaseScore(25);
		}
	}

	ENQUEUE_INGAMELOG_MESSAGE_PVP({Cast<APlayerCharacter>(Freer), Cast<APlayerCharacter>(Freed), EPVPEvent::VIPFreed});

	OnVIPFreed.Broadcast(Freed, Freer);
}

void AVIPEscortGM::VIPKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter)
{
	if (InstigatorCharacter)
	{
		if (AReadyOrNotPlayerState* PS = Cast<AReadyOrNotPlayerState>(InstigatorCharacter->GetPlayerState()))
		{
			if (AVIPEscortGS* VIPGS = GetGameState<AVIPEscortGS>())
			{
				if (VIPGS->bCanKillVIP && VIPGS->VIPPlayerState == KilledCharacter->GetPlayerState())
				{
					// VIP can be killed, no matter how it gets killed now the red team should win..
					VIPGS->PlayAnnouncerForTeam("VIPExecuted", ETeamType::TT_NONE);

					VIPGS->RecentVIPKiller = Cast<APlayerCharacter>(InstigatorCharacter);
					VIPGS->bVIPKilled = true;

					RoundWonTeam(ETeamType::TT_SERT_RED);
				}
				else
				{
					// Team that didn't kill the VIP wins...
					const ETeamType WinningTeam = PS->GetTeam() == ETeamType::TT_SERT_BLUE ? ETeamType::TT_SERT_RED : ETeamType::TT_SERT_BLUE;
					VIPGS->PlayAnnouncerForTeam("VIPKilledByEnemyEarly", WinningTeam);

					RoundWonTeam(WinningTeam);
				}
				
			}
		}
	}

	ENQUEUE_INGAMELOG_MESSAGE_PVP({Cast<APlayerCharacter>(InstigatorCharacter), Cast<APlayerCharacter>(KilledCharacter), EPVPEvent::VIPKilled});

	OnVIPKilled.Broadcast(InstigatorCharacter, KilledCharacter);
}

bool AVIPEscortGM::IsVIPAlive()
{
	if (APlayerCharacter* VIP = GetVIPCharacter())
	{
		return !VIP->IsDeadNotUnconscious();
	}

	return false;
}

bool AVIPEscortGM::IsVIPDead()
{
	if (APlayerCharacter* VIP = GetVIPCharacter())
	{
		return VIP->IsDeadNotUnconscious();
	}
	
	return false;
}

bool AVIPEscortGM::IsVIPArrested()
{
	if (APlayerCharacter* VIP = GetVIPCharacter())
	{
		return VIP->IsArrested();
	}
	
	return false;
}

void AVIPEscortGM::CheckVictoryConditions()
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

bool AVIPEscortGM::HasVisitedAllVIPSpawns()
{
	TArray<APlayerStart_VIP_Spawn*> VIPSpawns = GetAllVIPSpawnsInWorld();

	bool bHasVisitedAllSpawns = true;
	for (auto* VIPSpawn : VIPSpawns)
	{
		if (VIPSpawn && !VIPSpawn->bHasVisited)
		{
			bHasVisitedAllSpawns = false;
			break;
		}
	}

	return bHasVisitedAllSpawns;
}

void AVIPEscortGM::ResetAllVIPSpawnVisits()
{
	TArray<APlayerStart_VIP_Spawn*> VIPSpawns = GetAllVIPSpawnsInWorld();

	for (auto* VIPSpawn : VIPSpawns)
	{
		if (VIPSpawn)
		{
			VIPSpawn->bHasVisited = false;
		}
	}
}

TArray<APlayerController*> AVIPEscortGM::GetAllCompatiblePlayersForVIP()
{
	TArray<APlayerController*> PlayerControllers = UReadyOrNotFunctionLibrary::GetActorsOfClass<APlayerController>(GetWorld());
	UReadyOrNotFunctionLibrary::RemoveAllNullElements(PlayerControllers);

	TArray<APlayerController*> CompatiblePlayers;
	for (APlayerController* PlayerController : PlayerControllers)
	{
		if (AReadyOrNotPlayerState* PS = Cast<AReadyOrNotPlayerState>(PlayerController->PlayerState))
		{
			if (PS->GetTeam() == CurrentVIPTeam)
			{
				CompatiblePlayers.Add(PlayerController);
			}
		}
	}

	return CompatiblePlayers;
}
