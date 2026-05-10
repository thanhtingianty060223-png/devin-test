// Copyright Void Interactive, 2021

#pragma once

#include "Actors/BaseMagazineWeapon.h"
#include "GrenadeLauncher.generated.h"

UCLASS(Abstract)
class READYORNOT_API AGrenadeLauncher : public ABaseMagazineWeapon
{
	GENERATED_BODY()

	AGrenadeLauncher();
	
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY()
	TArray<class AGrenadeProjectile*> AppliedGrenadeProjectilePaths;

	UPROPERTY()
	class AGrenadeProjectile* LastSimulatedGrenade;
	UPROPERTY(EditAnywhere)
	float LaunchDistance = 2000.0f;

	UPROPERTY(EditAnywhere)
	float GrenadeBounciness = 1.0f;
	
	float GrenadeSpeed = 0.0f;

	UPROPERTY(EditAnywhere)
	UParticleSystem* BounceParticleEffect;

	UPROPERTY(EditAnywhere)
	UFMODEvent* BounceFMODEvent;

	UPROPERTY()
	TArray<FVector> FirstBouncePath;
	UPROPERTY()
	FHitResult FirstBounceHit;
	bool bAppliedFirstBounce = false;
	UPROPERTY()
	TArray<FVector> SecondBouncePath;
	UPROPERTY()
	FHitResult SecondBounceHit;
	bool bAppliedSecondBounce = false;
	UPROPERTY()
	TArray<FVector> ThirdBouncePath;
	UPROPERTY()
	FHitResult ThirdBounceHit;
	bool bAppliedThirdBounce = false;

	//UPROPERTY(ReplicatedUsing = OnRep_GrenadePath)
	TArray<FVector_NetQuantize> CompletePath;

	UPROPERTY(Replicated)
	int32 BouncePt1;
	UPROPERTY(Replicated)
	int32 BouncePt2;
	UPROPERTY(Replicated)
	int32 BouncePt3;

	int32 pathIdx = 2;

	UFUNCTION(Server, Reliable, WithValidation)
	void UpdateServerPath(const TArray<FVector_NetQuantize>& Path, int32 Bounce1, int32 Bounce2, int32 Bounce3);

	void FullySimulateGrenadePath(AGrenadeProjectile* Projectile);
};
