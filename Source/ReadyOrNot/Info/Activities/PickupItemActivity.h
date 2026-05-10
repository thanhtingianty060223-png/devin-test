// Copyright Void Interactive, 2021

#pragma once

#include "BaseActivity.h"
#include "PickupItemActivity.generated.h"

UCLASS()
class READYORNOT_API UPickupItemActivity : public UBaseActivity
{
	GENERATED_BODY()

public:
	UPickupItemActivity();

	virtual void StartActivity(AAIController* Owner) override;
	virtual void FinishedActivity(bool bSuccess) override;
	virtual void PerformActivity(float DeltaTime) override;
	virtual bool CanFinishActivity() const override;
	virtual bool OverrideFocalPoint(FVector& FocalPoint) override;
	virtual float GetDestinationTolerance() const override;

	#if !UE_BUILD_SHIPPING
	virtual void GatherDebugString(FString& OutString) override;
	#endif

	FORCEINLINE bool IsPickingUpItem() const { return bHasStartedPickup; }
	
	ABaseItem* FindItemToPickup() const;

	class AWeaponCacheActor* GetWeaponCacheActor();

	UFUNCTION()
	void OnPickupItemComplete();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup Item Activity")
	float SearchRadius = 1000.0f;

	uint8 bHasStartedPickup : 1;
	uint8 bPickupItemAnimationComplete : 1;
	uint8 bHasEverSeenWeaponCacheActor : 1;

	UPROPERTY()
	class AWeaponCacheActor* WeaponCacheActor = nullptr;

	UPROPERTY()
	ABaseItem* PickupItem = nullptr;
};