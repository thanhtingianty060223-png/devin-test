// ÂCopyright Void Interactive, 2017

#include "KingOfTheHostageGM.h"
#include "ReadyOrNot.h"
#include "KingOfTheHostageGS.h"




void AKingOfTheHostageGM::BeginPlay()
{
	Super::BeginPlay();
	AKingOfTheHostageGS* gs = GetGameState<AKingOfTheHostageGS>();
	if (gs)
	{
		gs->RedTeam_RoundTimeRemaining = Start_RoundTime;
		gs->BlueTeam_RoundTimeRemaining = Start_RoundTime;
	}
}

void AKingOfTheHostageGM::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (GetMatchState() == EMatchState::MS_Playing)
	{
		AKingOfTheHostageGS* gs = GetGameState<AKingOfTheHostageGS>();
		if (gs)
		{
			if (bBlueTeamOnAttack)
				gs->BlueTeam_RoundTimeRemaining -= DeltaSeconds;
			else
				gs->RedTeam_RoundTimeRemaining -= DeltaSeconds;
		}

	}
}

APawn* AKingOfTheHostageGM::SpawnHostage(TSubclassOf<APawn> HostageClass, TArray<FVector> SpawnLocations)
{
	if (SpawnLocations.Num() > 0 && HostageClass)
	{
		APawn* hostage = GetWorld()->SpawnActor<APawn>(HostageClass, SpawnLocations[FMath::RandRange(0, SpawnLocations.Num() - 1)], FRotator(0, FMath::RandRange(0, 360), 0));
		SpawnedHostages.Add(hostage);
		return hostage;
	}
	return nullptr;
}
