// Copyright Void Interactive, 2022

#pragma once

#include "GameFramework/Actor.h"
#include "ReplaySplineActor.generated.h"

UCLASS()
class READYORNOT_API AReplaySplineActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AReplaySplineActor();
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	class USplineComponent* SplineComponent = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FRotator> SplinePointRotations = TArray<FRotator>();
};
