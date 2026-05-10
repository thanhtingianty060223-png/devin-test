// Copyright Void Interactive, 2022

#pragma once

#include "Actors/PatrolPoint.h"
#include "GameFramework/Info.h"
#include "RosterScenarioSpawner.generated.h"

USTRUCT(BlueprintType)
struct FObjectiveSpawn
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FName Tag = NAME_None;

	UPROPERTY(EditAnywhere)
	FDataTableRowHandle Type;

	UPROPERTY(EditAnywhere)
	TArray<AAISpawn*> Spawners;
};

UCLASS(BlueprintType, NotBlueprintable, HideCategories = ("Rendering", "Replication", "Collision", "Input", "Actor", "LOD", "Cooking"))
class READYORNOT_API ARosterScenarioSpawner final : public AInfo
{
	GENERATED_BODY()

public:
	ARosterScenarioSpawner();

	UPROPERTY(EditAnywhere, Category = "Setup", meta = (TitleProperty = "Tag"))
	TArray<FObjectiveSpawn> Objectives;

	UPROPERTY()
	TArray<ADoor*> UsedTrapDoors;
	
	void DoRoster();

	//void DoHostageRescueRoster();

	TArray<FPatrolPoint> UsedPatrolPoints;

	void GetAllReachablePatrolPoints(const FVector& Location, TArray<FPatrolPoint>& OutPatrolPoints);

	bool IsPointOnUsedPatrolPoint(const FVector& Location) const;
	ADoor* GetClosestDoor(const FVector& Location) const;

	bool IsSpawnLocationValid(const FVector& Location) const;

	void DoSpawn(AAISpawn* Spawn);
};