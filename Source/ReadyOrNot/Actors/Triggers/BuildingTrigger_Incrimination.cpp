// Void Interactive, 2020

#include "BuildingTrigger_Incrimination.h"

#include "GameModes/IncriminationGM.h"
#include "GameModes/IncriminationGS.h"

ABuildingTrigger_Incrimination::ABuildingTrigger_Incrimination()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	bReplicates = true;
	SetCanBeDamaged(false);
	
	bAlwaysRelevant = true;
}

void ABuildingTrigger_Incrimination::BeginPlay()
{
	Super::BeginPlay();

	GAMEMODE_CHECK(AIncriminationGM, AIncriminationGS)
}
