#include "DeployableDepot.h"
#include "ReadyOrNot.h"
#include "GameModes/CoopGS.h"

void ADeployableDepot::OnGameStarted()
{
	ACoopGS* gs = Cast<ACoopGS>(GetWorld()->GetGameState());
	FLevelDataLookupTable LevelData = UBpGameplayHelperLib::GetLevelData(GetWorld());
	TArray<UDeployableSpawnComponent*> AllComponents;

	// If the gamestate says that we are not to spawn, then don't.
	if (gs->DepotLabel != DepotLabel)
	{
		V_LOGM(LogReadyOrNot, "Destroying Depot Labeled %s as it's not valid", *DepotLabel.ToString());
		Destroy();
		return;
	}

	if (!gs)
	{
		V_LOGM(LogReadyOrNot, "Gamestate not valid, destroying depot", *DepotLabel.ToString());
		Destroy();
		return;
	}

	// Spawn all of the deployables.
	GetComponents(AllComponents);
	for (int32 i = 0; i < AllComponents.Num(); i++)
	{
		UDeployableSpawnComponent* ThisComponent = Cast<UDeployableSpawnComponent>(AllComponents[i]);

		for (int32 j = 0; j < LevelData.Deployables.Num(); j++)
		{
			check(LevelData.Deployables[j].DeployableData);

			bool bShouldSpawn =  (gs->CurrentDeployables & (1 << j)) > 0;
			bShouldSpawn &= LevelData.Deployables[j].DeployableData->DeployableLabel == ThisComponent->DeployableLabel;

			if (ThisComponent->TrySpawnComponent(bShouldSpawn))
			{
				SpawnedDeployableComponents.Add(ThisComponent);
			}
		}
	}

	OnSuccessfulSpawn();
}
