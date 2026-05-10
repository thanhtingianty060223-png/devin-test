// Copyright Void Interactive, 2022

#pragma once

#include "GameFramework/Actor.h"
#include "SoundPropagator.generated.h"

UCLASS(BlueprintType, Blueprintable, HideCategories = ("Rendering", "Replication", "Collision", "Input", "Actor", "LOD", "Cooking"))
class READYORNOT_API ASoundPropagator : public AActor
{
	GENERATED_BODY()

public:
	ASoundPropagator();

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Setup")
	TMap<class APropagationPoint*, float> PropagationPoints;
	
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Features")
	uint8 bStopPropagationIfClosestToSound : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Features")
	float PathTracerRefreshRate = 0.8f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Features")
	float NavCheckRefreshRate = 4.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug")
	uint8 bEnableDebugSpheres : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug")
	uint8 bEnableDebugPathPoints : 1;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
	void OnPropagationEnterOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	void OnPropagationExitOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	USceneComponent* SceneComponent = nullptr;

	#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UBillboardComponent* BillboardComponent = nullptr;
	#endif
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UBoxComponent* PropagationSwitchEnter = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UBoxComponent* PropagationSwitchExit = nullptr;
};
