#include "DeployableSpawnComponent.h"
#include "ReadyOrNot.h"

bool UDeployableSpawnComponent::TrySpawnComponent(bool ShouldSpawn)
{
	if (ShouldSpawn)
	{
		FActorSpawnParameters ActorParams;
		ActorParams.Owner = nullptr;

		AActor* NewActor = GetWorld()->SpawnActor(ItemClass.Get(), &GetComponentTransform(), ActorParams);
		ABaseItem* NewItem = Cast<ABaseItem>(NewActor);
		if (NewItem)
		{
			NewItem->bDeployable = true;
		}

		MutateSpawnedDeployable(NewActor);
		return true;
	}
	else
	{
		return false;
	}
}
