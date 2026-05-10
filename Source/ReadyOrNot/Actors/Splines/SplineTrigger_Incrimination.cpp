// Copyright Void Interactive, 2021

#include "SplineTrigger_Incrimination.h"

#include "GameModes/IncriminationGM.h"
#include "GameModes/IncriminationGS.h"

ASplineTrigger_Incrimination::ASplineTrigger_Incrimination()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	bReplicates = true;
	SetCanBeDamaged(false);

	bAlwaysRelevant = true;
}

void ASplineTrigger_Incrimination::BeginPlay()
{
	Super::BeginPlay();

	GAMEMODE_CHECK(AIncriminationGM, AIncriminationGS)
}
