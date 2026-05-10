// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "BaseGrenade.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "Interfaces/GasSource.h"
#include "BaseGasGrenade.generated.h"

UCLASS()
class READYORNOT_API ABaseGasGrenade : public ABaseGrenade, public IGasSource
{
	GENERATED_BODY()

	ABaseGasGrenade();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	virtual void Detonate() override;
	virtual void OnItemUseComplete() override;

	virtual void Throw(bool bLocalOnly, bool bOverarmThrow, const FVector& ThrowDirection, const FVector& ThrowStart = FVector::ZeroVector) override;

	virtual void OnPhysicsImpact(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;

public:
	bool IsReleasingGas_Implementation() const override;
	float GetGasRadius_Implementation() const override;
	int32 GetMaximumGasPoints_Implementation() const override;
	bool GetGasReleaseLocation_Implementation(FVector& OutLocation) const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gas, meta = (AllowPrivateAccess = "true"))
	int32 MaxGasPoints = 20;

	/** keep track of where the grenade has bounced so that if it can't be projected to nav, try a previous bounce point*/
	TArray<FVector> BounceLocations;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float LocationRecordingRate = 0.1;

	// Distance the grenade needs to have moved since last check to record the new position
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RecordDistanceThreshold = 40;

private:
	/** Record the location of the grenade throughout its throw at regular intervals. These will be used to find a valid navmesh location
	 *  if the final grenade location cannot be projected to one
	 */
	TArray<FVector> PastLocations;

	FTimerHandle TH_RecordLocation;

	UFUNCTION()
	void RecordLocation();
};
