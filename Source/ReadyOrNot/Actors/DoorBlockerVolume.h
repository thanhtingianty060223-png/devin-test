// Copyright Void Interactive, 2023

#pragma once

#include "GameFramework/Actor.h"
#include "DoorBlockerVolume.generated.h"

UCLASS()
class READYORNOT_API ADoorBlockerVolume : public AActor
{
	GENERATED_BODY()
	
public:	
	ADoorBlockerVolume();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	class UBoxComponent* Bounds = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	class UNavModifierComponent* NavModifierComponent = nullptr;
};
