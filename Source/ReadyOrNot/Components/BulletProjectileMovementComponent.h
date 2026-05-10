// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BulletProjectileMovementComponent.generated.h"


UCLASS()
class READYORNOT_API UBulletProjectileMovementComponent : public UProjectileMovementComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UBulletProjectileMovementComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	virtual void HandleImpact(const FHitResult& Hit, float TimeSlice, const FVector& MoveDelta) override;

	/** If initial velocity is very high, can set friction to a very high value and use this to reduce it back to a sensible value after first bounce */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ProjectileBounces")
	bool bReduceFrictionAfterFirstBounce = false;

	/** Reduce the friction down to this value after the first bounce */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ProjectileBounces")
	float FrictionAfterFirstBounce;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
};
