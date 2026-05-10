#include "KingOfTheHillGS.h"
#include "ReadyOrNot.h"

AKingOfTheHillGS::AKingOfTheHillGS()
{
	GameRulesIntroAnnouncerRowName = "TOWGameRulesIntro";
	RoundsToPlay = 3;
}

float AKingOfTheHillGS::GetWinningScore(bool& bUsesScoring)
{
	bUsesScoring = false;
	return 0.0f;
}

void AKingOfTheHillGS::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AKingOfTheHillGS, Mover);
}