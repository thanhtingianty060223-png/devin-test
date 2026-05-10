// Copyright Void Interactive, 2017

#pragma once

#include "Components/SceneComponent.h"
#include "Actors/BaseItem.h"
#include "DeployableSpawnComponent.generated.h"

UCLASS(meta = (BlueprintSpawnableComponent), Blueprintable)
class READYORNOT_API UDeployableSpawnComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	// The class of item that we are supposed to spawn.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Deployables)
	TSubclassOf<AActor> ItemClass;

	// The label associated with this deployable.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Deployables)
	FName DeployableLabel;

	// Try to spawn the item associated with the component.
	UFUNCTION(BlueprintCallable, Category = Deployables)
	bool TrySpawnComponent(bool ShouldSpawn);

	// Mutate the spawned item.
	UFUNCTION(BlueprintImplementableEvent, Category = Deployables)
	void MutateSpawnedDeployable(AActor* SpawnedDeployable);
};
