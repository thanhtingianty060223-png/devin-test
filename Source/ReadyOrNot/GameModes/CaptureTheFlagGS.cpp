// Void Interactive, 2020

#include "CaptureTheFlagGS.h"

#include "Actors/Gameplay/CTF_Flag.h"
#include "Subsystems/InGameLogSubsystem.h"

DECLARE_CYCLE_STAT(TEXT("RoN ~ CTF GS Tick"), STAT_CTFGSTick, STATGROUP_RONCTFGS);

ACaptureTheFlagGS::ACaptureTheFlagGS()
{
	GameRulesIntroAnnouncerRowName = "CTFGameRulesIntroduction";
}

void ACaptureTheFlagGS::BeginPlay()
{
	Super::BeginPlay();

	
}

void ACaptureTheFlagGS::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	SCOPE_CYCLE_COUNTER(STAT_CTFGSTick);

	
}

void ACaptureTheFlagGS::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACaptureTheFlagGS, FlagBearer);
	DOREPLIFETIME(ACaptureTheFlagGS, FlagBearerTeam);
	DOREPLIFETIME(ACaptureTheFlagGS, Flag);
	DOREPLIFETIME(ACaptureTheFlagGS, bGameWon);
	
	DOREPLIFETIME_CONDITION_NOTIFY(ACaptureTheFlagGS, bFlagCaptured, COND_Custom, REPNOTIFY_Always);
}

void ACaptureTheFlagGS::OnRep_FlagStatus()
{
	if (Flag)
	{
		if (bFlagCaptured)
		{
			if (FlagBearer == UGameplayStatics::GetPlayerCharacter(this, 0))
				Flag->SetActorHiddenInGame(true);
			else
				Flag->SetActorHiddenInGame(false);

			ENQUEUE_INGAMELOG_MESSAGE_PVP({FlagBearer, EPVPEvent::FlagCaptured});
		}
		else
		{
			Flag->SetActorHiddenInGame(false);
			Flag->ResetFlagTransforms();

			ENQUEUE_INGAMELOG_MESSAGE_PVP({FlagBearer, EPVPEvent::FlagDropped});
		}
	}
}
