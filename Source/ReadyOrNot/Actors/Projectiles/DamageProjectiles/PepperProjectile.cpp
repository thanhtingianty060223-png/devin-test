// Copyright Void Interactive, 2023


#include "PepperProjectile.h"
#include "Info/CSGasManager.h"

APepperProjectile::APepperProjectile()
{
	MovementComp->bShouldBounce = false;
}

void APepperProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	UCSGasManager* GasManager = UCSGasManager::Get(GetWorld());
	if (!GasManager)
	{
		Super::OnHit(HitComponent, OtherActor, OtherComp, NormalImpulse, Hit);
		return;
	}
	
	FVector TraceDirection = (Hit.TraceStart - Hit.ImpactPoint).GetSafeNormal();
	GasManager->AddPepperballLocation(this, Hit.Location, TraceDirection);

	Super::OnHit(HitComponent, OtherActor, OtherComp, NormalImpulse, Hit);
}
