// Copyright Void Interactive, 2021

#include "SearchAndDestroyGS.h"
#include "ReadyOrNot.h"
#include "SearchAndDestroyGM.h"

void ASearchAndDestroyGS::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASearchAndDestroyGS, bBombPlanted);
	DOREPLIFETIME(ASearchAndDestroyGS, bBombDetonate);
	DOREPLIFETIME(ASearchAndDestroyGS, bBombDefused);
	DOREPLIFETIME(ASearchAndDestroyGS, bHasBomb);
	DOREPLIFETIME(ASearchAndDestroyGS, BombCountActive);
}
