// Copyright Void Interactive, 2021

#include "AdminGameControls.h"
#include "ReadyOrNot.h"
#include "ReadyOrNotGameMode.h"

bool UAdminGameControls::IsAdmin()
{
	return UReadyOrNotStatics::GetReadyOrNotPlayerController()->IsAdmin();	
}

void UAdminGameControls::KickPlayer(APlayerState* KickingPlayerState, FText Reason)
{
	UReadyOrNotStatics::GetReadyOrNotPlayerController()->Server_AdminKickPlayer(KickingPlayerState, Reason);
}

void UAdminGameControls::GetKickablePlayers(TArray<APlayerState*>& KickablePlayers)
{
	if (IsAdmin())
	{
		for (TActorIterator<APlayerState> It(GetWorld()); It; ++It)
		{
			AReadyOrNotGameMode* gm = Cast<AReadyOrNotGameMode>(GetWorld()->GetAuthGameMode());
			if (gm)
			{
				for (TActorIterator<AReadyOrNotPlayerController> PcIt(GetWorld()); PcIt; ++PcIt)
				{
					// don't kick admins
					AReadyOrNotPlayerController* pc = *PcIt;
					if (pc->PlayerState == *It && (pc && !pc->IsAdmin()))
					{
						KickablePlayers.AddUnique(*It);
					}
				}
			}
		}
		
	}
}
