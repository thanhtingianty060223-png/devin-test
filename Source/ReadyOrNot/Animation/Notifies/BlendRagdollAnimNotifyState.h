// Copyright Void Interactive, 2021

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "BlendRagdollAnimNotifyState.generated.h"


/**
 * 
 */
UCLASS()
class READYORNOT_API UBlendRagdollAnimNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()

	UBlendRagdollAnimNotifyState();

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

#if WITH_EDITOR
	virtual void OnAnimNotifyCreatedInEditor(FAnimNotifyEvent& ContainingAnimNotifyEvent) override;
#endif

	UPROPERTY()
	float TotalBlendDuration;

	UPROPERTY()
	float CurrentBlendAmount;

	/* going from 0 - 1.0 when to wake up the main body/mass, usually done at half of the blend duration*/
	UPROPERTY(EditAnywhere, Category = "Ragdoll")
	float PelvisWakeUpTime;

	bool bWakedUpPelvis;

	UPROPERTY(EditAnywhere, Category = "Ragdoll")
	bool bUsePhysicalAnimComp;

	UPROPERTY(EditAnywhere, Category = "Ragdoll")
	FPhysicalAnimationData PhysicalAnimData;

	float ActiveAnimTime;
	float NotifyTotalDuration;

	bool bDiedWhileInTick;
	/** Was the character dead before we even began ticking? */
	bool bDeadOnBegin = false;
};
