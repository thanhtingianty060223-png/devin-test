// Copyright Void Interactive, 2017

#include "GunGameGM.h"
#include "GunGameGS.h"

#include <NavigationSystem.h>

AGunGameGM::AGunGameGM()
{
	bRunWarmup = true;
}

void AGunGameGM::BeginPlay()
{
	Super::BeginPlay();
}

void AGunGameGM::RoundEnd()
{
	Super::RoundEnd();

	bSuddenDeath = false;
}

void AGunGameGM::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
}

void AGunGameGM::PlayerWon(AController* Player)
{
	if (!Player)
		return;

	AReadyOrNotPlayerState* ps = Cast<AReadyOrNotPlayerState>(Player->PlayerState);
	if (ps)
	{
		FRChatMessage Message;
		AReadyOrNotGameState* gs = Cast<AReadyOrNotGameState>(GetWorld()->GetGameState());
		Message.Color = FColor::Green;
		Message.Message = ps->GetPlayerName() + " has won! Restarting server in 60 seconds.";
		gs->Multicast_BroadcastChatMessage(Message);
	}
	
	FTimerHandle tempHandle;
	GetWorld()->GetTimerManager().SetTimer(tempHandle, this, &AGunGameGM::RestartGame, 60.0f, false);
}

ABaseItem* AGunGameGM::EquipNextGun(APlayerCharacter* Player, bool bAdvanceGunIdx)
{
	AReadyOrNotGameState* gs = Cast<AReadyOrNotGameState>(GetWorld()->GetGameState());
	if (Player)
	{
		int32 GunIdx = INDEX_NONE;

		AReadyOrNotPlayerState* ps = Cast<AReadyOrNotPlayerState>(Player->GetPlayerState());
		if (ps)
		{
			if (bAdvanceGunIdx)
			{
				ps->GunGameIdx++;
				

				int32 killsRemaining = ((Itemlist.Num() - ps->GunGameIdx) * KillsToProgress) - ps->KillsSinceUpgrade;
				if (killsRemaining < 5)
				{
					FRChatMessage Message;
					Message.Color = FColor::Red;
					Message.Message = ps->GetPlayerName() + " only has " + FString::FromInt(killsRemaining) + " kills remaining!";
					gs->Multicast_BroadcastChatMessage(Message);
				}

			}
			GunIdx = ps->GunGameIdx;
		}

		if (Itemlist.IsValidIndex(GunIdx - 1))
		{
			ABaseItem* item = Player->GetInventoryComponent()->GetInventoryItemOfClass(Itemlist[GunIdx - 1].LoadSynchronous());
			Player->GetInventoryComponent()->DestroyInventoryItem(item);
		}

		if (Player->GetEquippedItem() && Itemlist.Contains(Player->GetEquippedItem()->GetClass()))
		{
			//  clear it from the player
			Player->GetEquippedItem()->Destroy();
			Player->GetInventoryComponent()->ClearEquippedItem();
		}

		if (Itemlist.IsValidIndex(GunIdx))
		{
			ABaseItem* item = GetWorld()->SpawnActor<ABaseItem>(Itemlist[GunIdx].LoadSynchronous());
			if (item)
			{
			
				// equip the next item
				Player->GetInventoryComponent()->AddInventoryItem(item);
				Player->GetInventoryComponent()->PutItemInHands(item);
				return item;
			}
		}
		else
		{
			// no guns or this player has won
			PlayerWon(Player->GetController());
		}

	}
	return nullptr;
}

void AGunGameGM::PlayerKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter)
{
	TArray<AReadyOrNotPlayerState*> HighestKillPlayers = FindTopKillers();
	bool bSuddenDeathVictory = false;
	if (bSuddenDeath && InstigatorCharacter && InstigatorCharacter != KilledCharacter && HighestKillPlayers.Contains(InstigatorCharacter->GetPlayerState<AReadyOrNotPlayerState>()))
	{
		// this person may win the sudden death game
		bSuddenDeathVictory = true;
	}

	Super::PlayerKilled(InstigatorCharacter, KilledCharacter);

	if (bSuddenDeathVictory)
	{	// sudden death win
		AReadyOrNotPlayerState* PlayerState = InstigatorCharacter->GetPlayerState<AReadyOrNotPlayerState>();
		TArray<AReadyOrNotPlayerState*> PlayerStates;

		PlayerStates.Add(PlayerState);
		RoundWon(PlayerStates);
		return;
	}
	
	APlayerCharacter* player = Cast<APlayerCharacter>(InstigatorCharacter);
	if (player)
	{
		AReadyOrNotPlayerState* ps = Cast<AReadyOrNotPlayerState>(InstigatorCharacter->GetPlayerState());
		if (ps)
		{
			AReadyOrNotGameState* gs = Cast<AReadyOrNotGameState>(GetWorld()->GetGameState());
			ps->KillsSinceUpgrade++;
			if (ps->KillsSinceUpgrade >= KillsToProgress)
			{
				ps->KillsSinceUpgrade = 0;
		
				ABaseItem* item = EquipNextGun(Cast<APlayerCharacter>(InstigatorCharacter), true);
				if (ps && item && KilledCharacter && KilledCharacter->GetPlayerState())
				{
					// post status to chat - fixme, this needs to be localized
					FRChatMessage Message;

					Message.Color = FColor::White;
					Message.Message = ps->GetPlayerName() + " has been given a " + item->ItemName.ToString() + " for killing " + KilledCharacter->GetPlayerState()->GetPlayerName() + "!";
					gs->Multicast_BroadcastChatMessage(Message);
				}
			}
		}

	}

	

}

void AGunGameGM::PlayerArrested(AReadyOrNotCharacter* ArrestedCharacter, AReadyOrNotCharacter* InstigatorCharacter)
{
	Super::PlayerArrested(ArrestedCharacter, InstigatorCharacter);

	APlayerCharacter* player = Cast<APlayerCharacter>(InstigatorCharacter);
	if (player)
	{
		AReadyOrNotPlayerState* ps = Cast<AReadyOrNotPlayerState>(InstigatorCharacter->GetPlayerState());
		if (ps)
		{
			ps->KillsSinceUpgrade++;
			if (ps->KillsSinceUpgrade > KillsToProgress)
			{

				EquipNextGun(Cast<APlayerCharacter>(InstigatorCharacter), true);
			}
		}

	}
}

void AGunGameGM::RespawnPlayer(APlayerController* Player, bool bForceSpectator)
{
	APlayerController* pc = Cast<APlayerController>(Player);
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
					ps->bIsInGame = true;
					APlayerCharacter* character = SpawnPlayerCharacter(pc, BlueCharacterClass.LoadSynchronous(), SpawnTransform);
					UBpGameplayHelperLib::EquipLoadoutOnPlayer(DefaultItems, character, FLoadoutEquipOptions());
					EquipNextGun(character, false);

				}
			}
		}

	}
}

AActor* AGunGameGM::FindPlayerStart_Implementation(AController* Player, const FString& IncomingName)
{
	return GetRandomSafeStart();
}

void AGunGameGM::RespawnAllPlayers()
{
	V_LOGM(LogReadyOrNot, "Respawning ALL players!");
	for (TActorIterator<APlayerController> It(GetWorld()); It; ++It)
	{
		APlayerController* controller = *It;
		AReadyOrNotPlayerState* ps = Cast<AReadyOrNotPlayerState>(controller->PlayerState);
		if (ps)
		{
			ps->bIsInGame = true;
			RespawnPlayer(controller);
		}

		APlayerCharacter* pc = Cast<APlayerCharacter>(controller->GetPawn());
		if (pc)
		{
			pc->Multicast_ShowThirdPerson();
			pc->Multicast_ShowThirdPerson_Implementation();
		}
	}
}

void AGunGameGM::TimeLimitVictoryConditions_Implementation()
{
	Super::TimeLimitVictoryConditions_Implementation();

	TArray<AReadyOrNotPlayerState*> HighestKillPlayers = FindTopKillers();

	// if we have a tie, institute Sudden Death
	if (HighestKillPlayers.Num() > 1)
	{
		bSuddenDeath = true;

		// post status to chat - fixme, this needs to be localized
		FRChatMessage Message;
		AGunGameGS* gs = Cast<AGunGameGS>(GetWorld()->GetGameState());

		if (!gs)
		{
			return;
		}

		Message.Color = FColor::White;
		Message.Message = "Sudden death!";
		gs->Multicast_BroadcastChatMessage(Message);
	}
	else
	{
		RoundWon(HighestKillPlayers);
	}
}

TArray<AReadyOrNotPlayerState*> AGunGameGM::FindTopKillers()
{
	int32 HighestKillCount = 0;
	TArray<AReadyOrNotPlayerState*> HighestKillPlayers;

	for (TActorIterator<AReadyOrNotPlayerState> It(GetWorld()); It; ++It)
	{
		if (It->Kills == HighestKillCount)
		{
			HighestKillPlayers.AddUnique(*It);
		}
		else if (It->Kills > HighestKillCount)
		{
			HighestKillPlayers.Empty();
			HighestKillCount = It->Kills;
			HighestKillPlayers.AddUnique(*It);
		}
	}

	return HighestKillPlayers;
}