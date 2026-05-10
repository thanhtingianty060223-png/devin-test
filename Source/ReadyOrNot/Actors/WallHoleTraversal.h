// Copyright Void Interactive, 2022

#pragma once

#include "GameFramework/Actor.h"
#include "WallHoleTraversal.generated.h"

UCLASS(HideCategories = ("Rendering", "Replication", "Collision", "Input", "Actor", "LOD", "Cooking"))
class READYORNOT_API AWallHoleTraversal : public AActor
{
	GENERATED_BODY()
	
public:	
	AWallHoleTraversal();

	UFUNCTION(BlueprintCallable)
	void AddCooldownFor(AController* InController, float InCooldownTime);
	UFUNCTION(BlueprintPure)
	bool IsCooldownActiveFor(const AController* InController) const;
	
	FORCEINLINE FTransform GetEntryTransform() const { return EntryTransform; }
	FORCEINLINE FTransform GetExitTransform() const { return ExitTransform; }
	
	#if !UE_BUILD_SHIPPING
	void DrawDebug();
	#endif

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class USceneComponent* SceneComponent = nullptr;

	#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UBillboardComponent* BillboardComponent = nullptr;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UArrowComponent* ArrowComponent = nullptr;
	#endif
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup")
	uint8 bEnabled : 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup", meta = (EditCondition = "bEnabled"))
	FString Name = "None";
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Setup", meta = (EditCondition = "bEnabled"))
	TArray<TSoftObjectPtr<AStaticMeshActor>> IgnoredMeshActors;
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Setup", meta = (ClampMin = 0.0f, EditCondition = "bEnabled"))
	float CooldownAfterUse = 5.0f;
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Setup", meta = (ClampMin = 0.0f, EditCondition = "bEnabled"))
	float NavLinkProxyDistance = 50.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup|Triggers|Entry", meta = (EditCondition = "bEnabled"))
	FTransform EntryTriggerBoxTransform;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup|Triggers|Entry", meta = (EditCondition = "bEnabled"))
	FVector EntryTriggerBoxExtent = FVector(35.0f);
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup|Triggers|Exit", meta = (EditCondition = "bEnabled"))
	FTransform ExitTriggerBoxTransform;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup|Triggers|Exit", meta = (EditCondition = "bEnabled"))
	FVector ExitTriggerBoxExtent = FVector(35.0f);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation", meta = (EditCondition = "bEnabled"))
	UAnimMontage* EntryAnim = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation", meta = (EditCondition = "bEnabled"))
	UAnimMontage* LoopAnim = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation", meta = (EditCondition = "bEnabled"))
	UAnimMontage* ExitAnim = nullptr;
	
	// The current controller that is using this wall hole
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Data")
	AAIController* OccupiedByController = nullptr;
	
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;

	#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }
	#endif

	FTransform CalculateEntryTransform() const;
	FTransform CalculateExitTransform() const;

	UFUNCTION(CallInEditor, Category = "Tools")
	void TestForMeshes();
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Data")
	class ANavLinkProxy* NavLinkProxy = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Data", DisplayName = "Cooldowns")
	TMap<AController*, float> CooldownMap;
	
	FTransform EntryTransform;
	FTransform ExitTransform;
};
