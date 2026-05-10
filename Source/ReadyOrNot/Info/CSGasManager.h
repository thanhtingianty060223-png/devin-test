// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "Actors/Projectiles/DamageProjectiles/PepperProjectile.h"
#include "Data/CSGasData.h"
#include "NavigationSystem.h"
#include "CSGasManager.generated.h"

DECLARE_STATS_GROUP(TEXT("CSGAS"), STATGROUP_CSGAS, STATCAT_Advanced)

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="CS Gas Settings"))
class READYORNOT_API UReadyOrNotCSGasSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UReadyOrNotCSGasSettings() {}
	
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="CS Gas Settings")
	TSoftObjectPtr<UCSGasData> GasDataAsset = nullptr;
};

UCLASS()
class READYORNOT_API UCSGasManager : public UTickableWorldSubsystem
{
	GENERATED_BODY()

	UCSGasManager();
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

public:
	UFUNCTION(BlueprintPure)
	static UCSGasManager* Get(UWorld* World);
	
	UFUNCTION(BlueprintCallable, Category = "CS Gas")
	void AddGasSource(AActor* Source);
	UFUNCTION(BlueprintCallable, Category = "CS Gas")
	void RemoveGasSource(AActor* Source);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Navigation")
	UEnvQuery* GasGenerationQuery;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Navigation")
	int32 MaxGasPointsPerSource = 20;

	UFUNCTION()
	void GetGasSources(TArray<AActor*> &OutGasSources);
	UFUNCTION()
	void GetGasSafePoints(TArray<FVector> &OutGasSafePoints);

	UFUNCTION(BlueprintCallable)
	void AddPepperballLocation(APepperProjectile* Projectile, FVector Location, FVector Normal);

	UFUNCTION(BlueprintImplementableEvent, Category = Gas)
	void OnPepperCloudSpawned(FVector& Location);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gas)
	UCSGasData* GasData;

protected:
	virtual void Tick(float DeltaSeconds) override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual TStatId GetStatId() const override;


private:
	void UpdateGas();
	void OnGasGenerationQueryFinished(TSharedPtr<FEnvQueryResult> Result);

	UPROPERTY()
	AActor* NavVolumesContainer;
	void GenerateNavVolumes();
	void GenerateSphereNavVolume(FVector Location, float Radius);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gas, meta = (AllowPrivateAccess = "true"))
	TArray<AActor*> GasSources;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gas, meta = (AllowPrivateAccess = "true"))
	TArray<FVector> SafePoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gas, meta = (AllowPrivateAccess = "true"))
	TArray<FVector> GasPoints;

	UPROPERTY()
	TArray<AReadyOrNotCharacter*> CharactersInGasSourceRadius;

	UPROPERTY()
	TArray<AActor*> PepperbalLGasSources;

	void CalculatePepperballGasClouds(APepperProjectile* Projectile);
	void TickPepperballGasClouds();

	bool bIsRunningQuery = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gas, meta = (AllowPrivateAccess = "true"))
	FVector NavModifierBoxExtents = FVector(90, 90, 200);

	// Array of Tiles that need to be updated due to gas
	TArray<FIntPoint> AffectedNavTiles;

	float TickFrequency = 0.5;
	float ElapsedTimeSinceTick = 0;

	TMap<FVector, int32> PepperballLocations;

	bool ProjectPepperballToNavigation(APepperProjectile* Projectile, FVector Location, FVector Normal, FVector& OutLocation);

	bool ShouldApplyGasDamage(FVector Location, AActor* GasSource, UNavigationSystemV1* NavSys);

	FTimerHandle TH_PepperballLocationUpdate;

};
