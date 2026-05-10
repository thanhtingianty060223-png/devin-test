// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GasNavLinkGenerator.generated.h"

UCLASS()
class READYORNOT_API AGasNavLinkGenerator : public AActor
{
	GENERATED_BODY()

public:
	AGasNavLinkGenerator();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(CallInEditor)
	void GenerateNavLinks();

	UPROPERTY()
	UBillboardComponent* BillboardComponent;
};
