// Void Interactive, 2020

#pragma once

#include "GameFramework/Actor.h"
#include "CTF_Flag.generated.h"

UCLASS()
class READYORNOT_API ACTF_Flag : public AActor
{
	GENERATED_BODY()
	
public:	
	ACTF_Flag();

	UFUNCTION(BlueprintCallable, Category = "CTF Flag")
	void ResetFlagTransforms();

	UFUNCTION(BlueprintPure, Category = "CTF Flag")
	FORCEINLINE FName GetBoneToAttachName() const { return BoneToAttach; }
	
protected:
	void BeginPlay() override;
	void Tick(float DeltaTime) override;

	UFUNCTION()
	void OnFlagBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
	USceneComponent* SceneComponent = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
	UStaticMeshComponent* FlagMeshComponent = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
	UBoxComponent* CaptureBoxComponent = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
	class UObjectiveMarkerComponent* ObjectiveMarkerComponent = nullptr;

	UPROPERTY(EditAnywhere, Category = "Settings")
	FName BoneToAttach = "Flag_Socket";

private:
	float FindNearestFloor();
};
