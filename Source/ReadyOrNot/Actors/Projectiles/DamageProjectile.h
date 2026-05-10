// Copyright Void Interactive, 2023

#pragma once

#include "Actors/Projectile.h"
#include "DamageProjectile.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API ADamageProjectile : public AProjectile
{
	GENERATED_BODY()

protected:
	ADamageProjectile();

	virtual void BeginPlay() override;

	// The OnHit event calls ModifyProjectile and then ApplyDamage afterwards.
	virtual void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;
	virtual void ApplyDamage(AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit);

public:
	FVector SpawnLocation;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Gameplay)
	float Damage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	TSubclassOf<UDamageType> DamageType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	class UFMODEvent* HitMarker;
};
