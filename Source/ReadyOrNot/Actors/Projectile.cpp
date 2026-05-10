// Copyright Void Interactive, 2022

#include "Projectile.h"

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	CollisionComp->InitSphereRadius(5.0f);
	CollisionComp->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);
	CollisionComp->bReturnMaterialOnMove = true;
	CollisionComp->SetCanEverAffectNavigation(false);
	CollisionComp->bNavigationRelevant = false;

	RootComponent = CollisionComp;
	
	MovementComp = CreateDefaultSubobject<UBulletProjectileMovementComponent>(TEXT("Movement"));
	MovementComp->UpdatedComponent = CollisionComp;
	MovementComp->bRotationFollowsVelocity = true;
	MovementComp->bShouldBounce = false;
	MovementComp->Bounciness = 0.09;
	MovementComp->Friction = 0.5;
	MovementComp->ProjectileGravityScale = 1.0;

	NetUpdateFrequency = 1.0f;
	MinNetUpdateFrequency = 0.1f;
	NetPriority = 1.0f;

	AActor::SetReplicateMovement(false);
}

// Called when the game starts or when spawned
void AProjectile::BeginPlay()
{
	Super::BeginPlay();
	StartPos = GetActorLocation();
	
}

// Called every frame
void AProjectile::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

}

void AProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AProjectile, ImpactEffectsClass);
}

void AProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
}

void AProjectile::Multicast_SpawnImpactEffects_Implementation(FHitResult Hit, TSubclassOf<AImpactEffect> EffectsClass, float DecalScale, bool bExitImpact, bool bArmorImpact)
{
	if (!bExitImpact)
	{
		if (bSpawnedImpactEffects)
			return;

		bSpawnedImpactEffects = true;
	}
	

	if (EffectsClass == nullptr || EffectsClass.Get() == nullptr)
	{
		EffectsClass = ImpactEffectsClass;
	}

	if ((StartPos - GetActorLocation()).Size() < 30)
	{
		return;
	}

	// this has already been simulated locally for the player.. don't multicast effects to them.
	if (bReplicates && GetLocalRole() < ROLE_Authority)
	{
		APlayerCharacter* owningChar = Cast<APlayerCharacter>(GetOwner());
		if (owningChar && owningChar->IsLocalPlayer())
		{
			return;
		}
	}

	AImpactEffect* ImpactEffects_Instance = GetWorld()->SpawnActorDeferred<AImpactEffect>(EffectsClass, FTransform(), GetOwner(), Cast<APawn>(GetOwner()), ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (ImpactEffects_Instance)
	{
		FTransform ImpactTransform;
		ImpactTransform.SetLocation(Hit.ImpactPoint);
		ImpactTransform.SetRotation(GetActorRotation().Quaternion());
		ImpactEffects_Instance->SurfaceHit = Hit;
		ImpactEffects_Instance->DecalScale = DecalScale;
		ImpactEffects_Instance->Tags.Add("SpawnedDecal");
		ImpactEffects_Instance->bBulletGoneThroughPlayer = bHasGoneThroughPlayer;
		ImpactEffects_Instance->bArmorImpact = bArmorImpact;
		ImpactEffects_Instance->bTraceComplex = true;
		
		ImpactEffects_Instance->TriggerImpactEffect(Hit);
		ImpactEffects_Instance->FinishSpawning(ImpactTransform);
	}

}
