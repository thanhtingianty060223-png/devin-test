// Void Interactive, 2020

#include "CaptureTheFlagGM.h"
#include "CaptureTheFlagGS.h"

#include "Actors/Gameplay/CTF_Flag.h"
#include "Actors/Environment/CTF_FlagSpawnPoint.h"

#include "Characters/PlayerCharacter.h"

#include "Subsystems/InGameLogSubsystem.h"

DECLARE_CYCLE_STAT(TEXT("RoN ~ CTF GM Tick"), STAT_CTFGMTick, STATGROUP_RONCTFGM);

void ACaptureTheFlagGM::BeginPlay()
{
	Super::BeginPlay();

}

void ACaptureTheFlagGM::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	SCOPE_CYCLE_COUNTER(STAT_CTFGMTick);

	
}

AActor* ACaptureTheFlagGM::ChoosePlayerStart_Implementation(AController* Player)
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

			case ETeamType::TT_SUSPECT:
				return FindPlayerStartWithTag(SWATRedStartTag);

			default:
				return FindPlayerStartWithTag(SWATBlueStartTag);
		}
	}

	return FindPlayerStartWithTag(SWATBlueStartTag);
}

void ACaptureTheFlagGM::TimeLimitVictoryConditions_Implementation()
{
	RoundWonTeam(ETeamType::TT_NONE);
}

bool ACaptureTheFlagGM::ShouldCountDownTimelimitNow()
{
	return true;
}

void ACaptureTheFlagGM::StartMatch()
{
	Super::StartMatch();

	SpawnFlag();
}

void ACaptureTheFlagGM::NextRound()
{
	if (ACaptureTheFlagGS* CTFGS = GetGameState<ACaptureTheFlagGS>())
	{
		CTFGS->bGameWon = false;
		CTFGS->bFlagCaptured = false;
	}

	SpawnFlag();
	
	Super::NextRound();
}

void ACaptureTheFlagGM::RoundEnd()
{
	if (ACaptureTheFlagGS* CTFGS = GetGameState<ACaptureTheFlagGS>())
	{
		CTFGS->bGameWon = true;
		
		CTFGS->Flag = nullptr;
		CTFGS->FlagBearer = nullptr;
		CTFGS->FlagBearerTeam = ETeamType::TT_NONE;
	}

	DestroyFlag();
	
	Super::RoundEnd();
}

void ACaptureTheFlagGM::RoundWonTeam(ETeamType WinningTeam)
{
	Super::RoundWonTeam(WinningTeam);

}

void ACaptureTheFlagGM::CaptureFlag(ACTF_Flag* CapturedFlag, APlayerCharacter* NewFlagBearer)
{
	if (!NewFlagBearer || !CapturedFlag)
		return;
	
	if (!NewFlagBearer->IsValidLowLevel() || !CapturedFlag->IsValidLowLevel())
		return;

	if (NewFlagBearer->IsDeadNotUnconscious())
		return;
	
	if (ACaptureTheFlagGS* CTFGS = GetGameState<ACaptureTheFlagGS>())
	{
		if (CTFGS->bFlagCaptured)
			return;

		CTFGS->FlagBearer = NewFlagBearer;
		CTFGS->FlagBearer->OnCharacterKilled.RemoveAll(this);
		CTFGS->FlagBearer->OnCharacterKilled.AddDynamic(this, &ACaptureTheFlagGM::OnFlagBearerKilled);
		CTFGS->FlagBearerTeam = NewFlagBearer->GetTeam();

		CTFGS->Flag = CapturedFlag;

		CapturedFlag->AttachToActor(NewFlagBearer, FAttachmentTransformRules::SnapToTargetIncludingScale, CapturedFlag->GetBoneToAttachName());
		CapturedFlag->SetActorLocation(NewFlagBearer->GetMesh()->GetSocketLocation(CapturedFlag->GetBoneToAttachName()));
		CapturedFlag->SetActorRelativeRotation(NewFlagBearer->GetMesh()->GetSocketRotation(CapturedFlag->GetBoneToAttachName()));
		
		CTFGS->bFlagCaptured = true;

		BroadcastFlagCaptured();

		ENQUEUE_INGAMELOG_MESSAGE_PVP({CTFGS->FlagBearer, EPVPEvent::FlagCaptured});
	}
}

void ACaptureTheFlagGM::DropFlag()
{
	if (ACaptureTheFlagGS* CTFGS = GetGameState<ACaptureTheFlagGS>())
	{
		// We can't drop the flag if no one has possession of it
		if (!CTFGS->FlagBearer)
			return;

		CTFGS->Flag->ResetFlagTransforms();

		CTFGS->bFlagCaptured = false;

		BroadcastFlagDropped();

		ENQUEUE_INGAMELOG_MESSAGE_PVP({CTFGS->FlagBearer, EPVPEvent::FlagDropped});
	}
}

ACTF_FlagSpawnPoint* ACaptureTheFlagGM::ChooseFlagSpawnPoint()
{
	TArray<ACTF_FlagSpawnPoint*> FlagSpawnPoints;

	for (TActorIterator<ACTF_FlagSpawnPoint> It(GetWorld()); It; ++It)
	{
		FlagSpawnPoints.Add(*It);
	}
	
	FlagSpawnPoints.RemoveAll([](ACTF_FlagSpawnPoint* FlagSpawnPoint)
	{
		return FlagSpawnPoint == nullptr;
	});
	
	if (FlagSpawnPoints.Num() == 0)
		return nullptr;

	bool bHasVisitedAllSpawnPoints = true;
	for (ACTF_FlagSpawnPoint* FlagSpawnPoint : FlagSpawnPoints)
	{
		if (!FlagSpawnPoint->bHasVisited)
		{
			bHasVisitedAllSpawnPoints = false;
			break;
		}
	}

	if (bHasVisitedAllSpawnPoints)
	{
		for (ACTF_FlagSpawnPoint* FlagSpawnPoint : FlagSpawnPoints)
		{
			FlagSpawnPoint->bHasVisited = false;
		}

		ACTF_FlagSpawnPoint* FlagSpawnPoint = FlagSpawnPoints[FMath::RandRange(0, FlagSpawnPoints.Num()-1)];
		FlagSpawnPoint->bHasVisited = true;
		
		return FlagSpawnPoint;
	}

	ACTF_FlagSpawnPoint* FlagSpawnPoint;
	
	do
	{
		FlagSpawnPoint = FlagSpawnPoints[FMath::RandRange(0, FlagSpawnPoints.Num()-1)];
	}
	while (FlagSpawnPoint->bHasVisited);

	FlagSpawnPoint->bHasVisited = true;

	return FlagSpawnPoint;
}

void ACaptureTheFlagGM::SpawnFlag()
{
	DestroyAllCTFFlags();
	
	ACTF_FlagSpawnPoint* FlagSpawnPoint = ChooseFlagSpawnPoint();

	if (FlagSpawnPoint)
		Flag = GetWorld()->SpawnActor<ACTF_Flag>(FlagClassToSpawn, FlagSpawnPoint->GetActorLocation(), FlagSpawnPoint->GetActorRotation());
}

void ACaptureTheFlagGM::OnFlagBearerKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter)
{
	DropFlag();
}

void ACaptureTheFlagGM::BroadcastFlagCaptured()
{
	if (ACaptureTheFlagGS* CTFGS = GetGameState<ACaptureTheFlagGS>())
	{
		if (!bHasOnFlagCapturedEventBroadcasted)
		{
			OnFlagCaptured.Broadcast(CTFGS->FlagBearer, CTFGS->FlagBearerTeam);

			bHasOnFlagCapturedEventBroadcasted = true;
			bHasOnFlagDroppedEventBroadcasted = false;
		}
	}
}

void ACaptureTheFlagGM::BroadcastFlagDropped()
{
	if (ACaptureTheFlagGS* CTFGS = GetGameState<ACaptureTheFlagGS>())
	{
		if (!bHasOnFlagDroppedEventBroadcasted)
		{
			OnFlagDropped.Broadcast(CTFGS->FlagBearer, CTFGS->FlagBearerTeam);
			
			bHasOnFlagDroppedEventBroadcasted = true;
			bHasOnFlagCapturedEventBroadcasted = false;
		}
	}
}

void ACaptureTheFlagGM::DestroyAllCTFFlags()
{
	DestroyFlag();
	
	for (TActorIterator<ACTF_Flag> It(GetWorld()); It; ++It)
	{
		(*It)->Destroy();
	}
}

void ACaptureTheFlagGM::DestroyFlag()
{
	if (Flag)
	{
		Flag->Destroy();
		Flag = nullptr;
	}
}
