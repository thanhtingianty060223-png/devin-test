// Void Interactive, 2020

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "SpawnSkeletalMeshAnimNotifyState.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API USpawnSkeletalMeshAnimNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()

	public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

	UPROPERTY(EditAnywhere)
	USkeletalMesh* SkeletalMeshToSpawn;

	UPROPERTY(EditAnywhere)
	UAnimationAsset* PlayAnimation;
	
	UPROPERTY(EditAnywhere)
	FTransform MeshTransform;
	
	UPROPERTY(EditAnywhere)
	bool bEnableWeaponFOVShader = false;
	
	UPROPERTY(EditAnywhere)
	bool bOnlyOwnerSee = false;
	
	UPROPERTY(EditAnywhere)
	bool bOwnerNoSee = false;
	
	UPROPERTY(EditAnywhere)
	FName BoneToSpawnOn;

	FGuid SpawnedSkeletalMeshGUID;

	UPROPERTY(EditAnywhere, meta = (EditCondition = "!bSimulatePhysicsAtEnd"))
	bool bDestroyAtEnd = true;

	UPROPERTY(EditAnywhere, meta = (EditCondition = "!bDestroyAtEnd"))
	bool bSimulatePhysicsAtEnd = false;

	UPROPERTY(EditAnywhere, meta = (EditCondition = "bSimulatePhysicsAtEnd"))
	FVector ForceVector = FVector::ZeroVector;

	UPROPERTY()
	ASkeletalMeshActor* SkeletalMeshActor = nullptr;
};
