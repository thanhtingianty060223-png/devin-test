// Copyright Void Interactive, 2023

#include "GameModes/PartyModes/PropHuntGS.h"

void APropHuntGS::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APropHuntGS, AvailableProps);
}
