// Copyright Void Interactive, 2022

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Sound/SoundSource.h"
#include "PropagationTestactor.generated.h"

UCLASS()
class READYORNOT_API APropagationTestactor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	APropagationTestactor();

	// Depth to fully occlude gunshots (in cm)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TickInterval = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UFMODEvent* Event;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bDebugMode = false;

	UPROPERTY(VisibleDefaultsOnly)
	UBillboardComponent* Billboard = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EOcclusionType OcclusionType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EPropagationType PropagationType;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
