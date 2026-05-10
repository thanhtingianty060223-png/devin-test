#include "TugOfWarGS.h"
#include "ReadyOrNot.h"

ATugOfWarGS::ATugOfWarGS()
{
	GameRulesIntroAnnouncerRowName = "TOWGameRulesIntro";
}

float ATugOfWarGS::GetWinningScore(bool& bUsesScoring)
{
	bUsesScoring = false;
	return 0.0f;
}

void ATugOfWarGS::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATugOfWarGS, Mover);
}