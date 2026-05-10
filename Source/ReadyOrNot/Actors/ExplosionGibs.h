// Copyright Void Interactive, 2022

#pragma once

#include "CoreMinimal.h"
#include "ExplosionGibs.generated.h"

UCLASS(Abstract, Blueprintable)
class READYORNOT_API AExplosionGibs : public AActor
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent)
	void SetupGibsForSkeletalMesh(const USkeletalMeshComponent* Mesh);

private:
	UPROPERTY(EditAnywhere)
	class UBloodData* BloodData;

	UFUNCTION(BlueprintCallable)
	void SpawnBloodDecal(const FHitResult& Hit);
};
