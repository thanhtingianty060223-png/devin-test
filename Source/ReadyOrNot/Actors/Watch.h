// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Watch.generated.h"

UCLASS()
class READYORNOT_API AWatch : public AActor
{
	GENERATED_BODY()

public:
	AWatch();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = Mesh)
	UStaticMeshComponent* WatchMesh;

	UPROPERTY(EditDefaultsOnly, Category = Mesh)
	UStaticMeshComponent* HourHandMesh;

	UPROPERTY(EditDefaultsOnly, Category = Mesh)
	UStaticMeshComponent* MinuteHandMesh;

	UPROPERTY(EditDefaultsOnly, Category = Mesh)
	UStaticMeshComponent* SecondHandMesh;

	UPROPERTY(EditDefaultsOnly, Category = Mesh)
	UStaticMeshComponent* DateMesh;

private:
	virtual void Tick(float DeltaSeconds) override;

	float TimeElapsed = 0.0f;
};
