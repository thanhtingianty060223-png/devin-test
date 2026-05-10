// Void Interactive, 2020

#pragma once

#include "GameFramework/Actor.h"
#include "ActorSpawnPoint.generated.h"

UCLASS()
class READYORNOT_API AActorSpawnPoint : public AActor
{
	GENERATED_BODY()
	
public:	
	AActorSpawnPoint();

	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
	USceneComponent* SceneComponent = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
	UBillboardComponent* BillboardComponent = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Spawn Point|Data")
	uint8 bHasVisited : 1;
};
