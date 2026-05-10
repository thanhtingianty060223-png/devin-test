// Copyright Void Interactive, 2021

#pragma once

#include "GameFramework/Actor.h"
#include "WorldBuildingPlacementActor.generated.h"

UCLASS(BlueprintType, NotBlueprintable, HideCategories = ("Rendering", "Replication", "Collision", "Input", "Actor", "LOD", "Cooking"))
class READYORNOT_API AWorldBuildingPlacementActor final : public AActor
{
	GENERATED_BODY()
	
public:	
	AWorldBuildingPlacementActor();

	void GenerateActivities();
	
	bool GiveNextActivityForController(ACyberneticController* Controller, const FActivityRoute& Route);
	void GiveActivityForControllerWithoutRoute(ACyberneticController* Controller, float TimeDoingActivity);

	UPROPERTY(EditAnywhere, Category = "Setup")
	TSubclassOf<UWorldBuildingActivity> Activity;
	
	UPROPERTY(VisibleInstanceOnly, Category = "Status (Read Only)")
	ACyberneticController* InUseByController = nullptr;
	
	UPROPERTY()
	UWorldBuildingActivity* ActivityInstance = nullptr;
	
	UPROPERTY()
	class UMoveActivity* MoveToActivityInstance = nullptr;
	
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
	void OnWorldBuildingActivityFinished(class UBaseActivity* InActivity, ACyberneticController* CyberneticController);

	#if WITH_EDITOR
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }
	void EditorTick(float DeltaTime);
	#endif
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	USceneComponent* DefaultScene = nullptr;

	#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UBillboardComponent* BillboardComponent = nullptr;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UArrowComponent* ArrowComponent = nullptr;
	#endif
};
