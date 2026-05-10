// Copyright Void Interactive, 2017

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "RoNWeaponAnimInstance.generated.h"

/**
 * 
 */
UCLASS(transient, Blueprintable, hideCategories = AnimInstance, BlueprintType)
class READYORNOT_API URoNWeaponAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

	URoNWeaponAnimInstance();

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	// use for last tick values
	virtual void NativeLastTick(float DeltaSeconds);
	
public:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Core")
	float AmmoRemaining;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Optiwand")
	FRotator OptiwandBoneModify;

	UFUNCTION(BlueprintCallable, Category = "Animation Notifys")
	void OnSpeedReloadMagazineEjected();

	UFUNCTION(BlueprintCallable, Category = "Animation Notifys")
	void OnDisassembleMagazineEjected();

	int32 thrownMagazineCount = 0;

	UPROPERTY(BlueprintReadOnly)
	bool bIsEquipped = false;
};
