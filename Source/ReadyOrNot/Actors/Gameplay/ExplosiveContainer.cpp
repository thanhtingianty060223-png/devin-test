// Copyright Void Interactive, 2022

#include "Actors/Gameplay/ExplosiveContainer.h"

#include "Characters/CyberneticCharacter.h"
#include "Characters/CyberneticController.h"
#include "Components/MoraleComponent.h"

AExplosiveContainer::AExplosiveContainer()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	SetCanBeDamaged(true);

	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	SetRootComponent(BaseMesh);
	FireEffectParticle = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("FireEffectParticle"));
	FireEffectParticle->SetupAttachment(BaseMesh);
	FireEffectParticle->bAutoActivate = false;
	ExplosionEffectParticle = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ExplosionEffectParticle"));
	ExplosionEffectParticle->SetupAttachment(BaseMesh);
	ExplosionEffectParticle->bAutoActivate = false;
	FMODFireAudioComponent = CreateDefaultSubobject<UFMODAudioComponent>(TEXT("FMODParticle"));
	FMODFireAudioComponent->SetupAttachment(BaseMesh);

	static ConstructorHelpers::FObjectFinder<UMaterial> ScorchDecalObj(TEXT("/Game/ThirdParty/BallisticsVFX/Decals/Decals_Scorch"));
	ScorchDecal = ScorchDecalObj.Object;
}

void AExplosiveContainer::BeginPlay()
{
	Super::BeginPlay();
}

void AExplosiveContainer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FireEffectParticle->IsActive() ? FMODFireAudioComponent->Play() : FMODFireAudioComponent->Stop();
	
	if (bTriggered && !bExploded)
	{
		if (!FireEffectParticle->IsActive())
		{
			FireEffectParticle->Activate();
		}

		if (GetLocalRole() >= ROLE_Authority)
		{
			if (TimerUntilExplosionOnceTriggered > 0.0f)
			{
				TimerUntilExplosionOnceTriggered -= DeltaTime;
			}
			else
			{
				Explode();
			}
		}
	}
}

void AExplosiveContainer::Explode()
{
	if (GetLocalRole() < ROLE_Authority)
		return;

	if (bExploded)
		return;

	if (!bTriggered)
	{
		bTriggered = true;
		return;
	}

	UGameplayStatics::ApplyRadialDamageWithFalloff(GetWorld(), 100.0f, 0.0f, GetActorLocation(), 100.0f, 300.0f, 1.0f, StunDamageType, { this }, this, ExplosionInstigator, ECC_PROJECTILE);

	ARadialForceActor* RadialForce = GetWorld()->SpawnActor<ARadialForceActor>(ARadialForceActor::StaticClass(), GetActorTransform());
	RadialForce->SetLifeSpan(5.0f);
	RadialForce->GetForceComponent()->DestructibleDamage = 100.0f;
	RadialForce->GetForceComponent()->Radius = 1000.0f;
	RadialForce->GetForceComponent()->ImpulseStrength = 10000.0f;
	RadialForce->GetForceComponent()->Falloff = ERadialImpulseFalloff::RIF_Linear;
	RadialForce->GetForceComponent()->AddCollisionChannelToAffect(ECC_WorldDynamic);
	RadialForce->GetForceComponent()->AddCollisionChannelToAffect(ECC_PhysicsBody);

	for (TActorIterator<ACyberneticController>It(GetWorld()); It; ++It)
	{
		if (!It->GetCharacter())
			continue;

		if (It->IsSWAT())
			continue;

		float Dist = (It->GetCharacter()->GetActorLocation() - GetActorLocation()).Size();
		if (Dist < 1000.0f)
		{
			FHitResult Hit;
			FVector StartTrace = It->GetCharacter()->GetActorLocation() + UKismetMathLibrary::FindLookAtRotation(It->GetCharacter()->GetActorLocation(), GetActorLocation()).Vector() * 100.0f;
			FVector EndTrace = GetActorLocation() + UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), It->GetCharacter()->GetActorLocation()).Vector() * 100.0f;
			GetWorld()->LineTraceSingleByObjectType(Hit, StartTrace, EndTrace, FCollisionObjectQueryParams(ECC_WorldStatic));
			if (!Hit.bBlockingHit)
			{
				It->GetCharacter()->StartStun(EStunType::ST_Stung, this);
			}
		}
	}

	FTimerHandle Tmp;
	GetWorld()->GetTimerManager().SetTimer(Tmp, RadialForce, &ARadialForceActor::FireImpulse, FMath::FRandRange(0.1f, 0.3f), false);

	Multicast_PlayExplosionEffects();
}

void AExplosiveContainer::Multicast_TriggerExplosive_Implementation()
{
	bTriggered = true;
}

void AExplosiveContainer::Multicast_PlayExplosionEffects_Implementation()
{
	if (bExploded)
		return;

	bExploded = true;

	ExplosionEffectParticle->Activate();
	FireEffectParticle->Deactivate();

	if (bHideMeshAfterDetonation)
	{
		BaseMesh->SetHiddenInGame(true);
		SetLifeSpan(5.0f);
	}

	if (FMODExplosionAudio)
	{
		UFMODBlueprintStatics::PlayEventAtLocation(GetWorld(), FMODExplosionAudio, GetActorTransform(), true);
	}

	UGameplayStatics::SpawnDecalAtLocation(GetWorld(), ScorchDecal, FVector(5.0f, 96.0f, 96.0f), GetActorLocation());
	UGameplayStatics::PlayWorldCameraShake(GetWorld(), ExplosionScreenShake, GetActorLocation(), 0.0f, 1500.0f);
}

float AExplosiveContainer::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (DamageAmount > MinDamageToTrigger)
	{
		ExplosionInstigator = EventInstigator;
		Explode();
	}
	return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

