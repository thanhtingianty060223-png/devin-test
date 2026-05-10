// Void Interactive, 2020

#pragma once

#include "GameFramework/Actor.h"
#include "FlankingAvoidanceVolume.generated.h"

UCLASS()
class READYORNOT_API AFlankingAvoidanceVolume : public AActor
{
	GENERATED_BODY()
	
public:	
	AFlankingAvoidanceVolume();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	UBoxComponent* Bounds = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	class UNavModifierComponent* NavModifierComponent = nullptr;
};
