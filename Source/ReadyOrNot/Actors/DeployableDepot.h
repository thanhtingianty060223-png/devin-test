// Copyright Void Interactive, 2017

#pragma once

#include "GameFramework/Actor.h"
#include "Components/DeployableSpawnComponent.h"
#include "DeployableDepot.generated.h"

// A Deployable Depot is the place where deployables spawn. It can have up to 32 Deployable Spawn Components which are responsible for spawning the deployables.
UCLASS(Blueprintable, BlueprintType)
class READYORNOT_API ADeployableDepot : public AActor
{
	GENERATED_BODY()

public:
	// The spawned deployables.
	UPROPERTY(BlueprintReadOnly)
	TArray<class UDeployableSpawnComponent*> SpawnedDeployableComponents;

	// The label for this depot
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName DepotLabel = TEXT("Default");

	UFUNCTION()
	void OnGameStarted();

	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = "Deployable Depot")
		void Multicast_OnSuccessfulSpawn();
	virtual void Multicast_OnSuccessfulSpawn_Implementation() {OnSuccessfulSpawn();};

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
		void OnSuccessfulSpawn();
};
