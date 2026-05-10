#include "ArrestAndRescueGS.h"
#include "ReadyOrNot.h"

AArrestAndRescueGS::AArrestAndRescueGS() : Super()
{
	bPvPMode = true;
	bCanReportToTOC = false;
	// must have 1 wave so they can spawn first time lol
	BlueRespawnWaves = 1;
	RedRespawnWaves = 1;
	RoundsToPlay = 5;
}

void AArrestAndRescueGS::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AArrestAndRescueGS, RedRespawnWaves);
	DOREPLIFETIME(AArrestAndRescueGS, BlueRespawnWaves);
}