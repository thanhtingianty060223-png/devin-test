// Copyright Void Interactive, 2017

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DynamicWorldActor.generated.h"

// A DynamicWorldActor is something which can spawn in the world, but only if it is permitted by a Dynamic World Roster.
// You can change what DynamicWorldActors will spawn in the world from the level data.
UCLASS(Blueprintable)
class READYORNOT_API ADynamicWorldActor : public AActor
{
	GENERATED_BODY()

public:
	ADynamicWorldActor();

	// The label that is used when spawning this object.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName DynamicLabel;

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable)
	virtual void CheckDynamicSpawn();

	UFUNCTION(BlueprintCallable)
	virtual void SetDynamicSpawn(bool bShouldSpawn);

	UFUNCTION(BlueprintImplementableEvent)
	void OnDynamicallySpawned();


	UPROPERTY(ReplicatedUsing=OnRep_ReplicateSpawn)
	bool bReplicateSpawn = false;

	UFUNCTION()
		void OnRep_ReplicateSpawn();
};
