// Copyright Void Interactive, 2017

#include "FreeMode.h"

AFreeMode::AFreeMode()
{
	bRunWarmup = false;
}

void AFreeMode::BeginPlay()
{
	Super::BeginPlay();
}

void AFreeMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void AFreeMode::PlayerKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter)
{
	if (KilledCharacter)
	{
		FTimerHandle TimerHandle;
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateUObject(this, &AFreeMode::RespawnPlayer, Cast<APlayerController>(KilledCharacter->GetController()), false), RespawnTime, false);
		KilledCharacter->SetLifeSpan(30.0f);
	}
	Super::PlayerKilled(InstigatorCharacter, KilledCharacter);

}

void AFreeMode::PlayerArrested(AReadyOrNotCharacter* ArrestedCharacter, AReadyOrNotCharacter* InstigatorCharacter)
{
	if (ArrestedCharacter)
	{
		FTimerHandle TimerHandle;
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateUObject(this, &AFreeMode::RespawnPlayer, Cast<APlayerController>(ArrestedCharacter->GetController()), false), RespawnTime, false);
		ArrestedCharacter->SetLifeSpan(30.0f);
	}

	Super::PlayerArrested(ArrestedCharacter, InstigatorCharacter);
}

void AFreeMode::RespawnPlayer(APlayerController* Player, bool bForceSpectator)
{
	APlayerController* pc = Cast<APlayerController>(Player);
	if (pc)
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
					APlayerCharacter* spawnedPC = nullptr;
					if (ps->GetTeam() == ETeamType::TT_SERT_BLUE)
					{
						spawnedPC = SpawnPlayerCharacter(pc, BlueCharacterClass.LoadSynchronous(), SpawnTransform);
					}
					else if (ps->GetTeam() == ETeamType::TT_SERT_RED)
					{
						spawnedPC = SpawnPlayerCharacter(pc, RedCharacterClass.LoadSynchronous(), SpawnTransform);

					}


					if (spawnedPC)
					{
						spawnedPC->SetActorTransform(startPoint->GetActorTransform());
					}

				}
			}
		}
		
	}
}

void AFreeMode::RespawnDeadPlayers()
{

}

// Moved to GunGameGM
/*void AFreeMode::PlayerWon(AController* Player)
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
	GetWorld()->GetTimerManager().SetTimer(tempHandle, this, &AFreeMode::RestartGame, 60.0f, false);
}*/

AActor* AFreeMode::FindPlayerStart_Implementation(AController* Player, const FString& IncomingName /* = TEXT("") */)
{
	return Super::FindPlayerStart_Implementation(Player, IncomingName);
}
