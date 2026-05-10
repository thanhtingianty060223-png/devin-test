// Copyright Void Interactive, 2023

#include "DamageProjectile.h"

#include "Actors/DestructibleVehicle.h"
#include "Actors/Gameplay/GlassActor.h"

#include "Kismet/GameplayStatics.h"

ADamageProjectile::ADamageProjectile()
{
	GetCollisionComp()->bTraceComplexOnMove = true;
}

void ADamageProjectile::BeginPlay()
{
	Super::BeginPlay();

	SpawnLocation = GetActorLocation();
}

void ADamageProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	Super::OnHit(HitComponent, OtherActor, OtherComp, NormalImpulse, Hit);

	if (!HasAuthority() || !bReplicates)
		return;

	if (Cast<ADestructibleVehicle>(OtherActor) || Cast<AGlassActor>(OtherActor))
	{
		Multicast_SpawnImpactEffects(Hit);
		ApplyDamage(OtherActor, NormalImpulse, Hit);
	}
	else
	{
		if (!Cast<APlayerCharacter>(OtherActor))
		{
			// Handle the Player Impact effects on Damage instead.
			if (!GetCollisionComp()->MoveIgnoreActors.Contains(OtherActor))
			{
				Multicast_SpawnImpactEffects(Hit);
			}
		}

		ApplyDamage(OtherActor, NormalImpulse, Hit);
	}
}

void ADamageProjectile::ApplyDamage(AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit)
{
	AController* BulletInstigator = nullptr;
	ACharacter* OwningChar = Cast<ACharacter>(GetOwner());
	if (OwningChar)
	{
		BulletInstigator = OwningChar->GetController();
	}
	
	if (UGameplayStatics::ApplyPointDamage(OtherActor, Damage, NormalImpulse, Hit, BulletInstigator, GetOwner(), DamageType) > 0)
	{
		// Handle the Player Impact effects on Damage instead.
		if (!GetCollisionComp()->MoveIgnoreActors.Contains(OtherActor))
		{
			Multicast_SpawnImpactEffects(Hit);
			Multicast_SpawnImpactEffects_Implementation(Hit);
		}
	}
}

