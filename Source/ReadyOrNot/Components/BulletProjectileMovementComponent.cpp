// Copyright Void Interactive, 2023


#include "BulletProjectileMovementComponent.h"


UBulletProjectileMovementComponent::UBulletProjectileMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bReduceFrictionAfterFirstBounce = false;
	FrictionAfterFirstBounce = 0.2;
}

void UBulletProjectileMovementComponent::BeginPlay()
{
	Super::BeginPlay();

}

void UBulletProjectileMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                                       FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UBulletProjectileMovementComponent::HandleImpact(const FHitResult& Hit, float TimeSlice, const FVector& MoveDelta)
{
	Super::HandleImpact(Hit, TimeSlice, MoveDelta);

	if (bReduceFrictionAfterFirstBounce)
	{
		Friction = FrictionAfterFirstBounce;
		bBounceAngleAffectsFriction = false;
	}
}


