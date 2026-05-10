// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrainingTarget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHit, ATrainingTarget*, Target);

UCLASS()
class READYORNOT_API ATrainingTarget : public AActor
{
	GENERATED_BODY()

public:
	ATrainingTarget();

private:
	UPROPERTY(EditDefaultsOnly, Category = Mesh)
	UStaticMeshComponent* TargetMesh;

	UPROPERTY(EditDefaultsOnly, Category = Collision)
	UBoxComponent* SuccessBox;

	UPROPERTY(EditDefaultsOnly, Category = Collision)
	UBoxComponent* FailureBox;

public:
	/** Event triggered when a success box is shot. */
	UPROPERTY(BlueprintAssignable)
	FOnHit OnSuccessfulShot;

	/** Event triggered when a target it hit by a grenade. */
	UPROPERTY(BlueprintAssignable)
	FOnHit OnGrenadeHit;

protected:
	virtual void BeginPlay() override;

	/** Delegate for point damage events, such as bullet hits. */
	UFUNCTION()
	void OnPointDamage(AActor* DamagedActor, float Damage, class AController* InstigatedBy, FVector HitLocation, class UPrimitiveComponent* FHitComponent, FName BoneName, FVector ShotFromDirection, const class UDamageType* DamageType, AActor* DamageCauser);

	/** Delegate for radial damage events, such as grenade hits. */
	UFUNCTION()
	void OnRadialDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, FVector Origin, FHitResult HitInfo, class AController* InstigatedBy, AActor* DamageCauser);

private:
	/** Check if a bullet hit is inside a specified box. */
	static bool HitAnyBox(const UBoxComponent& BaseBox, FVector HitLocation);
};
