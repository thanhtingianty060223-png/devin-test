// ÂCopyright Void Interactive, 2017

#include "KingOfTheHostageGS.h"
#include "ReadyOrNot.h"



void AKingOfTheHostageGS::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AKingOfTheHostageGS, BlueTeam_RoundTimeRemaining);
	DOREPLIFETIME(AKingOfTheHostageGS, RedTeam_RoundTimeRemaining);
}
