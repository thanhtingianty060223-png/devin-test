// Copyright Void Interactive, 2023

#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "Octree/ThreatAwarenessOctree.h"
#include "ThreatAwarenessSubsystem.generated.h"

UCLASS(BlueprintType, NotBlueprintable)
class READYORNOT_API UThreatAwarenessSubsystem final : public UWorldSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	static UThreatAwarenessSubsystem* Get(const UObject* WorldContext);

	void OnWorldGenerated();

	void AddThreatPoint(const FThreatAwarenessData& InThreatPoint);
	void AddThreatPoints(const TArray<FThreatAwarenessData>& InThreatPoints);
	void RemoveThreatPoint(AThreatAwarenessActor* InThreatPoint);
	void RemoveThreatPoints(const TArray<AThreatAwarenessActor*>& InThreatPoints);
	void RemoveAllThreatPoints();

	bool FindThreatPoints(TArray<FThreatAwarenessDataOctreeElement>& OutThreatPoints, const FSphere& InQuerySphere);
	bool FindThreatPoints(TArray<FThreatAwarenessDataOctreeElement>& OutThreatPoints, const FBox& InQueryBox);

	UFUNCTION(BlueprintCallable)
	void GetThreatsForLocation(TArray<AThreatAwarenessActor*>& OutThreats, FVector Location, float MinDistance = 1000.0f, bool bRequireLOS = false, FName RoomName = NAME_None);
	
	AThreatAwarenessActor* GetNearestThreatForLocation(FVector Location, float MaxDistance = 10000.0f, float MaxZHeight = 10000.0f, bool bRequireLOS = false, TArray<EThreatLevel> ExcludeThreats = {}, TArray<AThreatAwarenessActor*> ExcludeThreatActors = {}, FName RoomName = NAME_None);
	AThreatAwarenessActor* GetFurthestThreatForLocation(FVector Location, float MaxDistance = 10000.0f, float MaxZHeight = 10000.0f, bool bRequireLOS = false, TArray<EThreatLevel> ExcludeThreats = {}, TArray<AThreatAwarenessActor*> ExcludeThreatActors = {}, FName RoomName = NAME_None);
	
	UFUNCTION(BlueprintPure)
	static AThreatAwarenessActor* GetNearestHighestThreat(const TArray<AThreatAwarenessActor*> InThreats, FVector Location);
	UFUNCTION(BlueprintPure)
	static AThreatAwarenessActor* GetFurthestHighestThreat(const TArray<AThreatAwarenessActor*> InThreats, FVector Location, float MinDistance = 0.0f);
	UFUNCTION(BlueprintPure)
	static TArray<AThreatAwarenessActor*> GetThreatsFromLocationBeyondRadius(const TArray<AThreatAwarenessActor*> InThreats, FVector Location, float MinDistance);
	
	static FBox CreateThreatSearchArea(const FVector& InThreatLocation, const FVector& InSearchBoxExtent);

	static AThreatAwarenessActor* FindClosestThreatPoint(const TArray<FThreatAwarenessDataOctreeElement>& InThreatPoints, const FVector& InTestLocation, ThreatSearchPredicate Predicate = [](const FThreatAwarenessDataOctreeElement&){ return true; });
	static AThreatAwarenessActor* FindFurthestThreatPoint(const TArray<FThreatAwarenessDataOctreeElement>& InThreatPoints, const FVector& InTestLocation, ThreatSearchPredicate Predicate = [](const FThreatAwarenessDataOctreeElement&){ return true; });
	static AThreatAwarenessActor* FindFurthestHighestThreatPoint(const TArray<FThreatAwarenessDataOctreeElement>& InThreatPoints, const FVector& InTestLocation, ThreatSearchPredicate Predicate = [](const FThreatAwarenessDataOctreeElement&){ return true; });

	UPROPERTY()
	TArray<class AThreatAwarenessActor*> AllThreatActors;

	UPROPERTY(BlueprintReadOnly)
	uint8 bDrawThreatAwarenessOctree : 1;
	UPROPERTY(BlueprintReadOnly)
	uint8 bDrawThreatPoints : 1;
	UPROPERTY(BlueprintReadOnly)
	uint8 bDrawThreatRoomNames : 1;

protected:
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;

	void InitializeThreatAwarenessSystem();
	void ShutdownThreatAwarenessSystem();

	void OnWorldInitialized(const UWorld::FActorsInitializedParams& Params);
	void OnPostWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);

	mutable FCriticalSection Mutex;

private:
	TSharedPtr<FThreatAwarenessOctree, ESPMode::ThreadSafe> ThreatAwarenessOctree = nullptr;
};
