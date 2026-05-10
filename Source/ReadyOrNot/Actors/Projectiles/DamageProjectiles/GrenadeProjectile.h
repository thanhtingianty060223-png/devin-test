// Copyright Void Interactive, 2022

#pragma once

#include "Actors/Projectiles/DamageProjectiles/BulletProjectile.h"
#include "Actors/BaseGrenade.h"
#include "GrenadeProjectile.generated.h"

UCLASS(Blueprintable, BlueprintType)
class READYORNOT_API AGrenadeProjectile : public ABulletProjectile
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	class URadialForceComponent* DetonationRadialForce;

	AGrenadeProjectile();

	virtual void BeginPlay() override;

	virtual void OnMeshHit_Implementation(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;
	virtual void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectileDetonated, AGrenadeProjectile*, Projectile);
	UPROPERTY(BlueprintAssignable)
	FOnProjectileDetonated OnGrenadeDetonated;

	int32 ImpactCount = 0;
	
	UPROPERTY(Replicated)
	FVector_NetQuantize ProjectileLocation;

	// How much time it takes for the grenade to detonate
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GrenadeProjectile)
	float DetonationTime;

	// How much time must elapse in between grenade detonations
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GrenadeProjectile)
	float DetonationBetweenTime;

	virtual void Tick(float DeltaSeconds) override;

	// How much time has elapsed
	UPROPERTY(BlueprintReadOnly, Category = GrenadeProjectile)
	float ElapsedTime;

	// How many times the grenade has detonated.
	UPROPERTY(BlueprintReadOnly, Category = GrenadeProjectile)
	int32 DetonationCount;

	// How many times the grenade is allowed to detonate.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GrenadeProjectile)
	int32 DetonationMax;

	// The detonation event
	UFUNCTION(BlueprintCallable, Category = GrenadeProjectile)
	void OnDetonate();
	
	UFUNCTION(BlueprintImplementableEvent, Category = GrenadeProjectile, DisplayName = "On Detonate")
	void OnDetonate_Blueprint();

	//TODO: Move launcher projectile base to code to have gas grenade projectile etc in code too
	UFUNCTION(BlueprintImplementableEvent, Category = GrenadeProjectile, DisplayName = "On Detonation Complete")
	void OnDetonationComplete_Blueprint();

	// Damage to do from the grenade explosion
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GrenadeProjectile)
	TArray<FGrenadeDamage> GrenadeDamage;

	// Whether to increase damage radius over time (used for CS gas grenades)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GrenadeProjectile)
	bool bIncreaseRadiusOverTime;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GrenadeProjectile)
	UFMODEvent* DetonationSound = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GrenadeProjectile)
	uint8 bPlayDetonationSoundOnce : 1;

	// Using this in same way as in BaseGasGrenade - Keep track of where we've bounced, so that if final location can't project to navmesh,
	// We can check whether it bounced off a navigable area. If so, use that location. In this base class as the Gas projectile inherits from another base blueprint
	// So can't set this up just for gas
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GrenadeProjectile)
	TArray<FVector> BounceLocations;

	UPROPERTY(BlueprintReadOnly)
	bool bDetonateOnValidation;

	virtual void OnProjectileValidated() override;

	// -------------------------------------------------------------
	// This section is all copied from BaseGasGrenade as it should do the same thing
	// TODO: Make all of this a component to just add to whatever
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float LocationRecordingRate = 0.1;

	// Distance the grenade needs to have moved since last check to record the new position
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RecordDistanceThreshold = 100;

	/** Record the location of the grenade throughout its throw at regular intervals. These will be used to find a valid navmesh location
	 *  if the final grenade location cannot be projected to one (Mainly for gas grenades atm. Most of the launcher projectile code is in BPs,
	 *  so need to put this here for all launcher grenades.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GrenadeProjectile)
	TArray<FVector> PastLocations;


private:
	FTimerHandle TH_RecordLocation;

	UFUNCTION()
	void RecordLocation();
	// -------------------------------------------------------------
};
