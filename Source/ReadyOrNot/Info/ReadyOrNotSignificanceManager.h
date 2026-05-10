// Copyright Void Interactive, 2021

#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "ReadyOrNotSignificanceManager.generated.h"

USTRUCT(BlueprintType)
struct FOptimizationAttachmentData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FName AttachedSocketName;

	UPROPERTY()
	USceneComponent* AttachedComponent;

	UPROPERTY()
	FTransform RelativeTransform;
};

USTRUCT(BlueprintType)
struct FRelevancyTracker
{
	GENERATED_USTRUCT_BODY()

    UPROPERTY()
	float TimeUntilRecheck;

};

DECLARE_STATS_GROUP(TEXT("ReadyOrNotSignificanceManager"), STATGROUP_SignificanceManager, STATCAT_Advanced);

UCLASS()
class READYORNOT_API UReadyOrNotSignificanceManager final : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static UReadyOrNotSignificanceManager* Get(const UObject* WorldContext);

protected:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	void GetRelevantViewpoints(TArray<FTransform>& OutViewpoints);
	bool IsRelevantForViewpoint(FTransform ViewPoint, AActor* TestActor);
	
	static bool IsViewpointUnique(TArray<FTransform>& Viewpoints, FTransform TestViewpoint);

	FTransform GetLocalViewPoint();
	bool bWasDemoPlaying = false;

	float TimeSinceLastTick = 0.0f;
	
	int32 ExpensiveTracesThisFrame = 0;
	int32 CheapTracesThisFrame = 0;
	int32 CurrentRelevancyIdx = 0;
	UPROPERTY()
	TArray<AActor*> ActorsMadeIrrelevantThisFrame;
	UPROPERTY()
	TArray<AActor*> ActorsMadeRelevantThisFrame;

	UPROPERTY()
	TArray<AActor*> ActorsMadeIrrelevant;

	// loop over all the relevent actors bit by it
	int32 HighPriorityRelevanceIndex;
	// Actors that are managed by siginificance
	UPROPERTY()
	TArray<AActor*> ActorsRelevantToSignificance;
	
	UPROPERTY()
	TArray<AReadyOrNotCharacter*> CharactersRelevantToSignificance;

	UPROPERTY()
	TMap<USkeletalMeshComponent*, USkeletalMesh*> SkeletalMeshLookupMap;

	UPROPERTY()
	TMap<USceneComponent*, FOptimizationAttachmentData> SceneCompAttachmentData;

	UPROPERTY()
	TMap<UStaticMeshComponent*, UStaticMesh*> StaticMeshLookupMap;

	UPROPERTY()
	TArray<AReadyOrNotCharacter*> IrrelevantPlayerCharacters;

	FCollisionQueryParams QueryParams;

	void PerformRelevancy(AActor* TestActor);

	void ActorRelevant(AActor* InActor, bool bUnregister);
	void ActorNotRelevant(AActor* InActor);

	void OptimizeSkeletalMesh(USkeletalMeshComponent* SkeletalMeshComp);
	void RecoverSkeletalMesh(USkeletalMeshComponent* SkeletalMeshComp);

	void OptimizeSceneComponent(USceneComponent* SceneComponent);
	void RecoverSceneComponent(USceneComponent* SceneComponent);

	// viewpoint of the player for determining significance
	UPROPERTY()
	TArray<FTransform> RelevantViewpoints;
	UPROPERTY()
	TArray<AActor*> PlayerViewTargets;

public:
	static void ForceActorRelevant(AActor* Actor);
	static void ForceActorNotRelevant(AActor* Actor);
	static bool IsActorRelevant(const AActor* Actor);
	static void RegisterActorWithSignificanceManager(AActor* Actor);
	static void UnregisterActorWithSignificanceManager(AActor* Actor);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnActorRelevancyChanged, AActor*, Actor, bool, bIsRelevant);
	FOnActorRelevancyChanged OnActorRelevancyChanged;

private:
	ENetMode GetNetMode() const;
};
