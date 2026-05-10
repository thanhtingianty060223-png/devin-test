// ÂCopyright Void Interactive, 2017

#include "VIPEscortGS.h"
#include "VIPEscortGM.h"

#include "ReadyOrNot.h"

void AVIPEscortGS::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AVIPEscortGS, HoldVIP_TimeRemaining);
	DOREPLIFETIME(AVIPEscortGS, bCanKillVIP);
	DOREPLIFETIME(AVIPEscortGS, CurrentVIPTeam);
	DOREPLIFETIME(AVIPEscortGS, VIPCharacter);
	DOREPLIFETIME(AVIPEscortGS, LastWinningTeam);
	DOREPLIFETIME(AVIPEscortGS, VIPPlayer);
	DOREPLIFETIME(AVIPEscortGS, bVIPSelected);
	DOREPLIFETIME(AVIPEscortGS, VIPPlayerState);
	DOREPLIFETIME(AVIPEscortGS, RecentArrester);
	DOREPLIFETIME(AVIPEscortGS, RecentFreer);
	DOREPLIFETIME(AVIPEscortGS, RecentVIPKiller);

	DOREPLIFETIME_CONDITION_NOTIFY(AVIPEscortGS, bVIPArrested, COND_Custom, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(AVIPEscortGS, bVIPKilled, COND_Custom, REPNOTIFY_Always);
}

AVIPEscortGS::AVIPEscortGS()
{
	GameRulesIntroAnnouncerRowName = "VIPGameRulesIntroduction";
}

void AVIPEscortGS::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (HasAuthority())
	{
		AVIPEscortGM* gm = Cast<AVIPEscortGM>(GetWorld()->GetAuthGameMode());
		if (gm)
		{
			VIPPlayer = gm->VIPPlayer;
			
			if (!VIPPlayer)
			{
				VIPCharacter = nullptr;
			}

			VIPPlayer ? bVIPSelected = true : bVIPSelected = false;

			if (VIPPlayer)
			{
				VIPPlayerState = Cast<AReadyOrNotPlayerState>(VIPPlayer->PlayerState);
			}
			else if (GetLocalRole() >= ROLE_Authority)
			{
				VIPPlayerState = nullptr;
			}
		}
	}
}

void AVIPEscortGS::OnResetLevel()
{
	bVIPArrested = false;
	VIPCharacter = nullptr;
	VIPPlayer = nullptr;
}

void AVIPEscortGS::OnRep_VIPArrested()
{
	if (VIPCharacter)
	{
		if (bVIPArrested)
		{
			if (RecentArrester)
				ENQUEUE_INGAMELOG_MESSAGE_PVP({RecentArrester, VIPCharacter, EPVPEvent::VIPArrested});
		}
		else
		{
			if (RecentFreer)
				ENQUEUE_INGAMELOG_MESSAGE_PVP({RecentFreer, VIPCharacter, EPVPEvent::VIPFreed});
		}
	}
}

void AVIPEscortGS::OnRep_VIPKilled()
{
	if (VIPCharacter)
	{
		if (bVIPKilled)
		{
			ENQUEUE_INGAMELOG_MESSAGE_PVP({RecentVIPKiller, VIPCharacter, EPVPEvent::VIPKilled});
		}
	}
}
