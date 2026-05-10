// Copyright Void Interactive, 2017

#pragma once

#include "GameFramework/Actor.h"
#include "Gameplay/ImpactEffect.h"
#include "Components/BulletProjectileMovementComponent.h"
#include "Projectile.generated.h"

UCLASS()
class READYORNOT_API AProjectile : public AActor
{
	GENERATED_BODY()

public:	
	AProjectile();

	FORCEINLINE class USphereComponent* GetCollisionComp() const { return CollisionComp; }
	FORCEINLINE class UBulletProjectileMovementComponent* GetMovementComp() const { return MovementComp; }

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Components")
	UBulletProjectileMovementComponent* MovementComp;

protected:
	virtual void BeginPlay() override;
	virtual void Tick( float DeltaSeconds ) override;

	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	
	UFUNCTION(NetMulticast, Unreliable)
			void Multicast_SpawnImpactEffects(FHitResult Hit, TSubclassOf<AImpactEffect> EffectsClass = nullptr, float DecalScale = 1.0f, bool bExitImpact = false, bool bArmorImpact = false);
	virtual void Multicast_SpawnImpactEffects_Implementation(FHitResult Hit, TSubclassOf<AImpactEffect> EffectsClass = nullptr, float DecalScale = 1.0f, bool bExitImpact = false, bool bArmorImpact = false);
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class USphereComponent* CollisionComp = nullptr;

public:
	FVector StartPos = FVector::ZeroVector;
	bool bSpawnedImpactEffects = false;
	bool bHasGoneThroughPlayer = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	UFMODEvent* ProjectileHitSound = nullptr;

	FFMODEventInstance ProjectileHitInstance;
	
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
	TSubclassOf<AImpactEffect> ImpactEffectsClass;

	/*
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mesh)
		USkeletalMesh* Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mesh)
		UStaticMesh* StaticMesh;*/

	//AActor* HitActor;
};
