// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/GasSource.h"
#include "PepperGasCloud.generated.h"

UCLASS()
class READYORNOT_API APepperGasCloud : public AActor, public IGasSource
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	APepperGasCloud();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Gas)
	UParticleSystemComponent* GasParticleSystem;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void Destroyed() override;;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	bool IsReleasingGas_Implementation() const override;
	float GetGasRadius_Implementation() const override;
	int32 GetMaximumGasPoints_Implementation() const override;
	bool GetGasReleaseLocation_Implementation(FVector& OutLocation) const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gas)
	float GasRadius = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gas)
	float Lifetime = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gas)
	float MaxGasPoints = 10;
};
