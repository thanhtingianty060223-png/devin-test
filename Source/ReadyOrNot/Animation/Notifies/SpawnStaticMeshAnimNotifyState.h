// Copyright Void Interactive, 2021

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "SpawnStaticMeshAnimNotifyState.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API USpawnStaticMeshAnimNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()


public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

	UPROPERTY(EditAnywhere)
	UStaticMesh* StaticMeshToSpawn;

	UPROPERTY(EditAnywhere)
	FName BoneToSpawnOn;

	FGuid SpawnedStaticMeshGUID;

	UPROPERTY(EditAnywhere)
	bool bCastShadow = true;
	
	UPROPERTY(EditAnywhere)
	bool bOnlyOwnerSee = false;
	
	UPROPERTY(EditAnywhere)
	bool bOwnerNoSee = false;

	UPROPERTY(EditAnywhere, meta = (EditCondition = "!bSimulatePhysicsAtEnd"))
	bool bDestroyAtEnd = true;

	UPROPERTY(EditAnywhere, meta = (EditCondition = "!bDestroyAtEnd"))
	bool bSimulatePhysicsAtEnd = false;

	UPROPERTY(EditAnywhere, meta = (EditCondition = "bSimulatePhysicsAtEnd"))
	FVector ForceVector = FVector::ZeroVector;
};
