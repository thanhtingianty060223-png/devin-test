// Copyright Void Interactive, 2023

#pragma once

#include "Actors/Projectiles/DamageProjectile.h"
#include "BulletProjectile.generated.h"

UENUM(BlueprintType)
enum class EProjectileReaction : uint8
{
	PR_None,
	PR_Richochet,
	PR_Pierce
};

// Network validation state
UENUM(BlueprintType)
enum class EServerValidationState : uint8
{
	Unvalidated,
	Validated,
	Invalid
};

/**
 * 
 */
UCLASS()
class READYORNOT_API ABulletProjectile : public ADamageProjectile
{
	GENERATED_BODY()

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UStaticMeshComponent* BulletMesh;

	//UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	//class USkeletalMeshComponent* BulletMeshSkele;

	UFUNCTION(NetMulticast, Reliable)
			void Multicast_AttachToComponent(FVector NewLocation, USceneComponent* Component, FName BoneName);
	virtual void Multicast_AttachToComponent_Implementation(FVector NewLocation, USceneComponent* Component, FName BoneName);

	UFUNCTION(NetMulticast, Reliable)
			void Multicast_SimulatePhysics(bool bSimulate);
	virtual void Multicast_SimulatePhysics_Implementation(bool bSimulate);

	UFUNCTION(NetMulticast, Reliable)
			void Multicast_ApplyForceToHitObjects(const FHitResult& Hit, FVector Velocity);
	virtual void Multicast_ApplyForceToHitObjects_Implementation(const FHitResult& Hit, FVector Velocity);
	
	virtual void ApplyDamage(AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit) override;

protected:
	virtual void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;

public:
	ABulletProjectile();

	virtual void BeginPlay() override;

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintNativeEvent)
			void OnMeshHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	virtual void OnMeshHit_Implementation(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	FTimerHandle WobbleDelay_Handle;
	void ApplyWobble();

	int32 RespawnCount = 0;
	int32 ProjectileNumber = 0;

	float DistanceTraveled = 0.0f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = Gameplay)
		ABaseItem* FiredFromWeapon = nullptr;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = Gameplay)
		class APlayerCharacter* FiredFromPlayer = nullptr;

	// How much to rotate the bullet when it penetrates something (0.0 = don't rotate bullet, 1.0 = full range of motion)
	UPROPERTY(EditAnywhere, Category = Gameplay)
		float HitAngleMultiplier = 0.20f;

	// The angle at which deflection (ricochet) must occur, minimum. 
	UPROPERTY(EditAnywhere, Category = Gameplay)
		float RequiredAngleToDeflect = 60.0f;

	// The chance that deflection (ricochet) occurs.
	UPROPERTY(EditAnywhere, Category = Gameplay)
		float PercentageToDeflect = 0.5f;

	// How much to deflect the bullet by. (90.0 = perfect mirror)
	UPROPERTY(EditAnywhere, Category = Gameplay)
		float DeflectionAmount = 90.0f;

		float PenetrationDistance = 100.0f;
		float PenetratedDistance = 0.0f;

	// How much the bullet loses in speed (momentum) per deflection (ricochet)
	UPROPERTY(EditAnywhere, Category = Gameplay)
		float SpeedLossMultiplierPerSurface = 0.5f;

	// Not used.
	UPROPERTY(EditAnywhere, Category = Gameplay)
		float DamageLossMultiplierPerSurface = 0.5f;

	// Minimum velocity required to spawn a new bullet from penetration
	UPROPERTY(EditAnywhere, Category = Gameplay)
		float VelocityRequiredToRespawn = 5000.0f;

	// Whether this spawns blood effects.
	UPROPERTY(EditAnywhere, Replicated, Category = Gameplay)
		bool bDrawBlood = true;

	FVector PositionLastFrame;

	UPROPERTY()
	FVector BulletProjectileScale = FVector(1);

	UPROPERTY()
		float DecalScale = 1.0f;

	UPROPERTY(EditAnywhere, Category = Gameplay)
		bool bAffectedByGravity = true;


	UFUNCTION(BlueprintImplementableEvent, Category = Gameplay)
		void OnDeflect(FHitResult DeflectionHit);

	UFUNCTION(BlueprintCallable, Category = Gameplay)
		void OnRespawnProjectile(FVector RespawnLocation, FRotator RespawnRotation, float newSpeed, float newDamage, EProjectileReaction projectileReaction = EProjectileReaction::PR_None);

	UFUNCTION(NetMulticast, Unreliable, BlueprintCallable, Category = Gameplay)
		void Multicast_OnRespawnProjectile(FVector_NetQuantize100 RespawnLocation, FVector_NetQuantize100 RespawnRotation, float NewSpeed, float NewDamage, EProjectileReaction ProjectileReaction = EProjectileReaction::PR_None);
	virtual void Multicast_OnRespawnProjectile_Implementation(FVector_NetQuantize100 RespawnLocation, FVector_NetQuantize100 RespawnRotation, float NewSpeed, float NewDamage, EProjectileReaction ProjectileReaction = EProjectileReaction::PR_None);

	UPROPERTY(EditAnywhere, Category = Sounds)
	USoundCue* BulletWizzSound;

	UPROPERTY(EditAnywhere, Category = Sounds)
		float MinimumDistanceForWizz = 100.0f;

	UPROPERTY(EditAnywhere, Category = Sounds)
		float requiredSpeedForWizz = 3420.0f;

	UPROPERTY(EditAnywhere, Category = Debug)
	float DebugLineSize = 0.5;

	UPROPERTY(EditAnywhere, Category = Impacts)
		TSubclassOf<AImpactEffect> ExitEffects;

	UPROPERTY(EditAnywhere, Category = Impacts)
	TSubclassOf<AImpactEffect> RicochetEffects;

	UPROPERTY(EditAnywhere, Category = Impacts)
	UParticleSystem* RichochetParticle;

	UPROPERTY(EditAnywhere, Category = Gameplay, meta = (ClampMin = 0.0f, ClampMax = 1.0f, UIMin = 0.0f, UIMax = 1.0f))
	float ArmorPiercing;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = Gameplay)
		bool bDestroyOnHit = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = Gameplay)
		bool bAttachOnHit = true;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = Gameplay)
		float InitialSpeed = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = Gameplay)
		FVector InitialLocation;

	UPROPERTY(BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = Gameplay)
		AActor* OwningActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = Gameplay)
		float LockIntegrityDamage = 0.0f;

	UFUNCTION()
	void OnRep_UpdateMesh();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = Gameplay)
		TSubclassOf<UDamageType> InitialDamageType;

	// How long the bullet needs to last for.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = Gameplay)
		float LifeSpan = 0.0f;

	// How much to slow the bullet down for every second of travel.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = Gameplay)
		float Drag = -3000.0f;

	// How much a bullet will "wobble" while in the air.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = Gameplay)
		float Wobble = 30.0f;

	// How long it takes for a bullet to start wobbling.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = Gameplay)
		float InitialWobbleDelay = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = Gameplay)
	float PhysicsImpulseMultiplier = 5000.0f; // 0.01f


	FORCEINLINE class UStaticMeshComponent* GetBulletMesh() const { return BulletMesh; }
	//FORCEINLINE class USkeletalMeshComponent* GetBulletMeshSkele() const { return BulletMeshSkele; }
	//FORCEINLINE class UAudioComponent* GetWhizzAudioComp() const { return WhizzAudioComp; }

	bool bEmbedded = false; // whether or not the bullet embedded itself into a player. If it does, it doesn't spawn the exit wounds.

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay, Meta = (AllowPrivateAccess = "true"))
	bool bCanPenetrate = true;

	/** Used on client for whether this locally spawned projectile has been validated and can therefore interact with environment*/
	UPROPERTY()
	bool bServerValidated = false;

	UFUNCTION(BlueprintCallable, Category = Networking)
	virtual void OnProjectileValidated();
	
	virtual bool IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const override;

	virtual void PostNetInit() override;
};
