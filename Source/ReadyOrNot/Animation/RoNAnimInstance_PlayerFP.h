// Copyright Void Interactive, 2017
// Author: Alexander Mijalkovski

#pragma once

#include "Animation/ReadyOrNotAnimInstance.h"
#include "RoNAnimInstance_PlayerFP.generated.h"

UCLASS(transient, Blueprintable, hideCategories = AnimInstance, BlueprintType)
class URoNAnimInstance_PlayerFP : public UReadyOrNotAnimInstance
{
	GENERATED_UCLASS_BODY()

	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	// base functions used to find player animation sequences
	UFUNCTION(BlueprintPure, Category = Animation, meta = (BlueprintThreadSafe))
	UAnimSequenceBase * GetPlayerAnimation_FP(EBaseAnimType_FP::Entries AnimName) const;

	// ##UE5UPGRADE## Compatibility
	UFUNCTION(BlueprintPure, Category = Animation, meta = (BlueprintThreadSafe))
	UBlendSpace * GetPlayerBlendspace_FP(EBaseBlendspaces_FP::Entries BlendspaceName) const;


	UPROPERTY()
	class UReadyOrNotWeaponAnimData* LastWeaponAnimData = nullptr;
	
	// Preview entry for Anim Blueprint Editor
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UReadyOrNotWeaponAnimData* EditorWeaponAnimData = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Core")
	float LeanAngleY;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Core")
	float LeanAngleZ;

	UFUNCTION(BlueprintCallable, Category = "Animation Notifys")
		void OnHolsterComplete();

	UFUNCTION(BlueprintCallable, Category = "Animation Notifys")
		void OnReloadComplete();

	UFUNCTION(BlueprintCallable, Category = "Animation Notifys")
		void OnC2Detonation();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	FRotator MeshControlRotation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	FVector MeshPostureLeanOffset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	FVector MeshWeaponOffset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	FRotator MeshWeaponRotation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	FRotator MeshWeaponFreeAimRotation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	FVector MeshWeaponLeanOffset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Core")
	FRotator MeshWeaponLeanRotation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Core")
	USkeletalMeshComponent* TPMeshReference;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aim")
	float LazySpringStrength;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Core")
	float ADS_Movement_Weight;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Core")
	float InteractionSlotAlpha;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Core")
	float DefaultSlotAlpha;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Roll")
	float RollMoveInput;
};
