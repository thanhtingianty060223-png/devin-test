// Copyright Void Interactive, 2022

#pragma once

#include "CoreMinimal.h"
#include "Actors/SuspectArmour.h"
#include "ExplosiveVest.generated.h"

/**
 *	Suspect explosive vest, handles explosive vest logic and effects
 */
UCLASS()
class READYORNOT_API AExplosiveVest : public ASuspectArmour
{
	GENERATED_BODY()

	AExplosiveVest();

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable)
	void ExplodeVest();

	virtual bool HandleDamage(float& Damage, FPointDamageEvent const& DamageEvent, AActor* DamageCauser) override;

private:
	void StartExplosion();
	void ApplyExplosionDamage();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayPreExplosionEffects();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayExplosionEffects();

	FTransform GetExplosiveVestTransform();

private:
	bool bVestExploded = false;

	FTimerHandle ExplosionStartTimer;
	FTimerHandle ExplosionDamageTimer;

public:
	// Whether or not this explosive vest should explode when shot
	UPROPERTY(EditAnywhere, Category = "Explosive Vest")
	bool bShouldExplodeOnHit = false;

	// Whether or not this explosive vest should explode on owner death
	UPROPERTY(EditAnywhere, Category = "Explosive Vest")
	bool bShouldExplodeOnDeath = false;

	// The type of damage the vest's explosion should cause
	UPROPERTY(EditAnywhere, Category = "Explosive Vest")
	TSubclassOf<UDamageType> ExplosionDamageType;

	// The Max damage the vest explosion should cause
	UPROPERTY(EditAnywhere, Category = "Explosive Vest")
	float MaxDamageOnDetonation = 200.0f;

	// The minimum damage the vest explosion should cause
	UPROPERTY(EditAnywhere, Category = "Explosive Vest")
	float MinDamageOnDetonation = 50.0f;

	// Inner radius of the vest explosion where no damage falloff should occur
	UPROPERTY(EditAnywhere, Category = "Explosive Vest")
	float DamageInnerRadius = 0.0f;

	// Outer radius of the vest explosion where falloff does occur
	UPROPERTY(EditAnywhere, Category = "Explosive Vest")
	float DamageOuterRadius = 1000.0f;

	// Delay that should occur before the explosion effect plays
	UPROPERTY(EditAnywhere, Category = "Explosive Vest")
	float ExplosionEffectDelay = 2.0f;

	// Offset the explosion effect delay randomly by up to this amount of time, give or take
	UPROPERTY(EditAnywhere, Category = "Explosive Vest")
	float ExplosionEffectRandomDelay = 0.1f;

	// Delay that should occur before explosion causes damage
	UPROPERTY(EditAnywhere, Category = "Explosive Vest")
	float ExplosionDamageDelay = 0.0f;

	// Animation montage to play when intentionally detonating the vest
	UPROPERTY(EditAnywhere, Category = "Explosive Vest")
	FString DetonationMontage = "tp_spct_detonatevest";

	// Optional bone to spawn all explosive vest related effects at
	UPROPERTY(EditAnywhere, Category = "Explosive Vest")
	FName ExplosiveVestSocket = "Bomb_Vest_Socket";

	// Particle effect to play on vest detonation
	UPROPERTY(EditAnywhere, Category = "Explosive Vest")
	UParticleSystem* ExplosionParticle;

	// Audio effect to play on vest detonation
	UPROPERTY(EditAnywhere, Category = "Explosive Vest")
	UFMODEvent* ExplosionEvent;

	// Audio effect to play right before detonation
	UPROPERTY(EditAnywhere, Category = "Explosive Vest")
	UFMODEvent* DetonationEvent;

	// The screen shake to be applied when this vest detonates
	UPROPERTY(EditAnywhere, Category = "Explosive Vest")
	TSubclassOf<ULegacyCameraShake> ExplosionScreenShake;

	// The radius to apply the screen shake when this vest detonates
	UPROPERTY(EditAnywhere, Category = "Explosive Vest")
	float ExplosionScreenShakeRadius = 1000.0f;
};
