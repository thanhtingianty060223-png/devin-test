// Copyright Void Interactive, 2022

#include "GrenadeProjectile.h"

#include "Actors/Environment/BreakableGlass.h"

AGrenadeProjectile::AGrenadeProjectile()
{
	DetonationRadialForce = CreateDefaultSubobject<URadialForceComponent>("RadialForceComponent");
	DetonationRadialForce->SetupAttachment(GetBulletMesh());
	DetonationRadialForce->bAutoActivate = false;
	DetonationRadialForce->Radius = 1000.0f;
	DetonationRadialForce->ImpulseStrength = 1000.0f;

	bAlwaysRelevant = true;
	Drag = 0.0f;
}

void AGrenadeProjectile::BeginPlay()
{
	Super::BeginPlay();
	
	GetWorldTimerManager().SetTimer(TH_RecordLocation, this, &AGrenadeProjectile::RecordLocation, LocationRecordingRate, true);
	PastLocations.Reset();
	PastLocations.Reserve(DetonationTime * LocationRecordingRate);

	// Add the throwers location as the first recorded location as a backup that'll almost certainly be on a navmesh
	if (!GetOwner())
		return;
	
	PastLocations.Emplace(GetOwner()->GetActorLocation());
}


void AGrenadeProjectile::OnMeshHit_Implementation(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// Projectile movement doesn't give a normal impulse as it's not actually a physics collision
	// For now just make the impulse a fraction of the current velocity
	NormalImpulse = GetVelocity() / 2;
	Super::OnMeshHit_Implementation(HitComponent, OtherActor, OtherComp, NormalImpulse, Hit);
	ImpactCount++;

	if (GetBulletMesh()->GetComponentVelocity().Size() > 800.0f)
	{
		BounceLocations.Emplace(Hit.Location);
	}
}

void AGrenadeProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	OnMeshHit(HitComponent, OtherActor, OtherComp, NormalImpulse, Hit);
}

void AGrenadeProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGrenadeProjectile, ProjectileLocation);
}

void AGrenadeProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!HasAuthority())
	{
		GetBulletMesh()->SetWorldLocation(ProjectileLocation, false, nullptr, ETeleportType::TeleportPhysics);
	}
	else
	{
		ProjectileLocation = GetBulletMesh()->GetComponentLocation();
	}

	// Apply glass damage, breakable glass doesn't respond well to physics collision. so do a trace
	if (GetBulletMesh()->GetComponentVelocity().Size() > 50.0f && DetonationCount < 1)
	{
		FCollisionObjectQueryParams CollisionObjectQueryParams;
		CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
		CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
		CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_Destructible);

		TArray<AActor*> ActorsToIgnore;
		ActorsToIgnore.Add(this);
		if (AReadyOrNotCharacter* OwnerCharacter = Cast<AReadyOrNotCharacter>(GetOwner()))
			ActorsToIgnore.Append(OwnerCharacter->GetCollisionIgnoredActors());
		else
			ActorsToIgnore.Add(GetOwner());

		FHitResult HitResult_Forward, HitResult_Backward;
		float Distance = 100.0f;
		FVector Start = GetBulletMesh()->GetComponentLocation();
		FVector End = GetBulletMesh()->GetComponentVelocity().GetSafeNormal();
		
		GetWorld()->LineTraceSingleByObjectType(HitResult_Forward, Start, Start + End * Distance, CollisionObjectQueryParams);
		GetWorld()->LineTraceSingleByObjectType(HitResult_Backward, Start, Start - End * Distance, CollisionObjectQueryParams);

		//DrawDebugLine(GetWorld(), HitResult_Forward.TraceStart, HitResult_Forward.TraceEnd, FColor::Red);
		//DrawDebugLine(GetWorld(), HitResult_Backward.TraceStart, HitResult_Backward.TraceEnd, FColor::Green);

		if (HitResult_Forward.IsValidBlockingHit())
		{
			if (ABreakableGlass* Glass = Cast<ABreakableGlass>(HitResult_Forward.GetActor()))
			{
				Glass->ConvertHitAndExecute(HitResult_Forward, 1000.0f);
			}
		}
		else if (HitResult_Backward.IsValidBlockingHit())
		{
			if (ABreakableGlass* Glass = Cast<ABreakableGlass>(HitResult_Backward.GetActor()))
			{
				Glass->ConvertHitAndExecute(HitResult_Backward, 1000.0f);
			}
		}
	}

	//ULog::Vector(GetBulletMesh()->GetComponentVelocity(), false, "velocity: ");

	ElapsedTime += DeltaTime;

	// Check to see if we need to detonate.
	if (DetonationCount < DetonationMax && ElapsedTime >= DetonationTime)
	{
		OnDetonate();

		DetonationTime += DetonationBetweenTime;
	}
	else if (DetonationCount >= DetonationMax)
	{
		OnDetonationComplete_Blueprint();
		SetReplicates(false);
		SetActorTickEnabled(false);
	}
}

void AGrenadeProjectile::OnDetonate()
{
	// If this is a local projectile and we aren't server (ie we're on the fired client), check for server validation before triggering
	// ##UE5UPGRADE##
	if (GetLocalRole() == ROLE_Authority && !GetWorld()->IsNetMode(NM_ListenServer) && !bServerValidated)
	{
		bDetonateOnValidation = true;
		return;
	}
	
	if (!bPlayDetonationSoundOnce || (bPlayDetonationSoundOnce && DetonationCount < 1))
		UFMODBlueprintStatics::PlayEventAtLocation(this, DetonationSound, GetBulletMesh()->GetComponentTransform(), true);

	if (DetonationCount < 1)
	{
		GetWorldTimerManager().ClearTimer(TH_RecordLocation);
	}

	DetonationCount++;

	// Damage everyone in the specified ranges
	for (int32 i = 0; i < GrenadeDamage.Num(); i++)
	{
		DetonationRadialForce->FireImpulse();

		// Apply damage only on server (Not from client-side projectiles)
		// ##UE5UPGRADE##
		if (!GetWorld()->IsNetMode(NM_ListenServer))
			continue;
		
		float DamageInnerRadius, DamageOuterRadius;

		if (bIncreaseRadiusOverTime && (DetonationMax - DetonationCount > 0))
		{
			DamageInnerRadius = GrenadeDamage[i].DamageInnerRadius / (float)(DetonationMax - DetonationCount);
			DamageOuterRadius = GrenadeDamage[i].DamageOuterRadius / (float)(DetonationMax - DetonationCount);
		}
		else
		{
			DamageInnerRadius = GrenadeDamage[i].DamageInnerRadius;
			DamageOuterRadius = GrenadeDamage[i].DamageOuterRadius;
		}

		TArray<AActor*> IgnoreActors;
		IgnoreActors.Add(this);
		IgnoreActors.Add(GetOwner<APawn>());

		AController* InstigatorController = GetOwner<APawn>() ? GetOwner<APawn>()->GetController() : nullptr;
		UGameplayStatics::ApplyRadialDamageWithFalloff(this, GrenadeDamage[i].MaxDamageOnDetonation, GrenadeDamage[i].MinDamageOnDetonation, GetBulletMesh()->GetComponentLocation(), DamageInnerRadius, DamageOuterRadius, 1.0f, GrenadeDamage[i].DamageType, IgnoreActors, this, InstigatorController, ECC_WorldStatic);
	}
	
	// Call the related blueprint effects
	OnDetonate_Blueprint();

	OnGrenadeDetonated.Broadcast(this);
}

void AGrenadeProjectile::OnProjectileValidated()
{
	Super::OnProjectileValidated();

	if (bDetonateOnValidation)
		OnDetonate();
}

void AGrenadeProjectile::RecordLocation()
{
	if (!PastLocations.Num() || (FVector::DistSquared(GetBulletMesh()->GetComponentLocation(), PastLocations.Last()) > FMath::Square(RecordDistanceThreshold)))
		PastLocations.Emplace(GetBulletMesh()->GetComponentLocation());
}