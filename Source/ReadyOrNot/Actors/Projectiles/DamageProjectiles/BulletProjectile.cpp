// Copyright Void Interactive, 2023

#include "BulletProjectile.h"
#include "Data/PenetrationData.h"
#include "Characters/PlayerCharacter.h"
#include "Actors/Items/BallisticsShield.h"
#include "Perception/AISense_Hearing.h"

ABulletProjectile::ABulletProjectile()
{
	BulletMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Bullet"));
	BulletMesh->SetupAttachment(GetCollisionComp());

	//BulletMeshSkele = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BulletSkele"));
	//BulletMeshSkele->SetupAttachment(GetCollisionComp());

	//WhizzAudioComp = CreateDefaultSubobject<UAudioComponent>(TEXT("WhizzAudioComp"));
	//WhizzAudioComp->SetupAttachment(GetCollisionComp());

	NetUpdateFrequency = 100.0f;
	MinNetUpdateFrequency = 2.0f;
	NetPriority = 2.0f;
	bReplicates = false;
}

void ABulletProjectile::PostNetInit()
{
	Super::PostNetInit();
	GetMovementComp()->InitialSpeed = InitialSpeed;
	GetMovementComp()->MaxSpeed = InitialSpeed;
	// Need to run some reinitialization code on clients as this function runs after component initialization
	
	if (GetMovementComp()->bInitialVelocityInLocalSpace)
	{
		GetMovementComp()->SetVelocityInLocalSpace(FVector(InitialSpeed, 0.f, 0.f));
	}
}

void ABulletProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//DOREPLIFETIME(ABulletProjectile, ProjectileMesh);
	//DOREPLIFETIME(ABulletProjectile, ProjectileMeshStatic);
	DOREPLIFETIME(ABulletProjectile, FiredFromWeapon);
	DOREPLIFETIME(ABulletProjectile, FiredFromPlayer);
	DOREPLIFETIME(ABulletProjectile, bDrawBlood);
	DOREPLIFETIME(ABulletProjectile, InitialSpeed);
}

void ABulletProjectile::BeginPlay()
{
	Super::BeginPlay();

	OnRep_UpdateMesh();
	GetWorld()->GetTimerManager().SetTimer(WobbleDelay_Handle, this, &ABulletProjectile::ApplyWobble, InitialWobbleDelay);
}

void ABulletProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (UBpGameplayHelperLib::GetRoNData()->bDrawBulletDebug && PositionLastFrame != FVector::ZeroVector)
	{
		DrawDebugLine(GetWorld(), PositionLastFrame, GetActorLocation(), FColor(GetMovementComp()->Velocity.Size()), false, 10.0f, 0, DebugLineSize);
	}

	if (PositionLastFrame != GetActorLocation())
	{
		PositionLastFrame = GetActorLocation();
	}
	
	
}

void ABulletProjectile::ApplyWobble()
{
	GetCollisionComp()->SetPhysicsLinearVelocity(FVector(FMath::Rand()), true);
}

void ABulletProjectile::ApplyDamage(AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit)
{
	AController* BulletInstigator = nullptr;
	ACharacter* OwningChar = Cast<ACharacter>(GetOwner());
	
	// you can't hit yourself
	if (OwningChar == OtherActor)
	{
		return;
	}

	if (OwningChar)
	{
		BulletInstigator = OwningChar->GetController();
	}

 	DistanceTraveled = FVector::Dist2D(InitialLocation, Hit.Location);
	float FinalDamage = Damage;
	if (FinalDamage < 0.0f)
	{
		FinalDamage = 0.0f;
	}
	
	float DamageApplied = UGameplayStatics::ApplyPointDamage(OtherActor, FinalDamage, NormalImpulse, Hit, BulletInstigator, this, DamageType);
	APlayerCharacter* Player = Cast<APlayerCharacter>(OtherActor);
	if (Player)
	{
		if ((DamageApplied > 0) || (Player && Player->IsDeadNotUnconscious()))
		{
			// Handle the Player Impact effects on Damage instead.
			if (!GetCollisionComp()->MoveIgnoreActors.Contains(OtherActor) && bDrawBlood)
			{
				Multicast_SpawnImpactEffects(Hit);
			}
		}
		else if (DamageApplied == 0)
		{
			Multicast_SpawnImpactEffects(Hit, nullptr, 1.0f, false, true);
			Destroy();
		}
	}
}

void ABulletProjectile::OnMeshHit_Implementation(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	const float NormalImpulseSize = NormalImpulse.Size();
	if (NormalImpulseSize > 100.0f)
	{
		if (!UFMODBlueprintStatics::EventInstanceIsValid(ProjectileHitInstance))
			ProjectileHitInstance = UFMODBlueprintStatics::PlayEventAtLocation(this, ProjectileHitSound, GetBulletMesh()->GetComponentTransform(), true);
	}
}

void ABulletProjectile::OnRespawnProjectile(FVector RespawnLocation, FRotator RespawnRotation, float NewSpeed, float NewDamage, EProjectileReaction ProjectileReaction)
{
	Multicast_OnRespawnProjectile(RespawnLocation, RespawnRotation.Vector(), NewSpeed, NewDamage, ProjectileReaction);
}

void ABulletProjectile::Multicast_OnRespawnProjectile_Implementation(FVector_NetQuantize100 RespawnLocation, FVector_NetQuantize100 RespawnRotation, float NewSpeed, float NewDamage, EProjectileReaction ProjectileReaction /*= EProjectileReaction::PR_None*/)
{
	FTransform SpawnTransform;
	SpawnTransform.SetLocation(RespawnLocation);
	SpawnTransform.SetRotation(RespawnRotation.Rotation().Quaternion());
	AReadyOrNotGameState* gs = Cast<AReadyOrNotGameState>(GetWorld()->GetGameState());

	if (!gs)
	{
		return;
	}

	ABulletProjectile* bullet = GetWorld()->SpawnActorDeferred<ABulletProjectile>(GetClass(), SpawnTransform, GetOwner(), nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (bullet)
	{
		bullet->RespawnCount = RespawnCount + 1;
		bullet->SetOwner(GetOwner());
		bullet->FiredFromPlayer = FiredFromPlayer;
		bullet->FiredFromWeapon = FiredFromWeapon;
		bullet->Damage = Damage;
		bullet->ArmorPiercing = ArmorPiercing;
		bullet->InitialSpeed = NewSpeed;
		bullet->InitialLocation = RespawnLocation;
		bullet->InitialDamageType = InitialDamageType;
		bullet->ImpactEffectsClass = ImpactEffectsClass;
		bullet->RicochetEffects = RicochetEffects;
		bullet->ExitEffects = ExitEffects;
		bullet->LockIntegrityDamage = LockIntegrityDamage;
		bullet->InitialWobbleDelay = InitialWobbleDelay;
		bullet->OwningActor = GetOwner();
		bullet->bAttachOnHit = bAttachOnHit;
		bullet->bDestroyOnHit = bDestroyOnHit;
		bullet->SetInstigator(Cast<APawn>(GetOwner()));
		//bullet->ProjectileMesh = ProjectileMesh;
		//bullet->ProjectileMeshStatic = ProjectileMeshStatic;
		bullet->DecalScale = DecalScale;
		bullet->FinishSpawning(bullet->GetActorTransform());
		bullet->bDrawBlood = bDrawBlood;
		bullet->bHasGoneThroughPlayer = bHasGoneThroughPlayer;
		bullet->PenetrationDistance = PenetrationDistance * 0.75f;
		bullet->PenetratedDistance = PenetratedDistance;

		ABaseMagazineWeapon* bw = Cast<ABaseMagazineWeapon>(FiredFromWeapon);
		if (bw && bw->LastSpawnedProjectile == this)
		{
			bw->LastSpawnedProjectile = bullet;
		}

		GetCollisionComp()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SetLifeSpan(3.0f);

	}
}

void ABulletProjectile::OnRep_UpdateMesh()
{
	/*
	BulletMesh->SetStaticMesh(ProjectileMeshStatic);
	BulletMeshSkele->SetSkeletalMesh(ProjectileMesh);
	*/

	BulletMesh->SetWorldScale3D(BulletProjectileScale);
	//BulletMeshSkele->SetWorldScale3D(BulletProjectileScale);
}

void ABulletProjectile::Multicast_AttachToComponent_Implementation(FVector NewLocation, USceneComponent* Component, FName BoneName)
{
	AttachToComponent(Component, FAttachmentTransformRules::KeepWorldTransform, BoneName);
	SetActorLocation(NewLocation);
}

void ABulletProjectile::Multicast_SimulatePhysics_Implementation(bool bSimulate)
{
	BulletMesh->SetCollisionProfileName("PhysicsItem");
	BulletMesh->SetCollisionResponseToChannel(ECC_PhysicsBody, ECollisionResponse::ECR_Ignore);
	BulletMesh->SetSimulatePhysics(true);

	//BulletMeshSkele->SetCollisionProfileName("PhysicsItem");
	//BulletMeshSkele->SetCollisionResponseToChannel(ECC_PhysicsBody, ECollisionResponse::ECR_Ignore);
	//BulletMeshSkele->SetSimulatePhysics(true);
	
	GetCollisionComp()->SetSimulatePhysics(true);
	GetCollisionComp()->SetCollisionResponseToChannel(ECC_PhysicsBody, ECollisionResponse::ECR_Ignore);
}

void ABulletProjectile::Multicast_ApplyForceToHitObjects_Implementation(const FHitResult& Hit, FVector Velocity)
{
	if (GetLocalRole() >= ROLE_Authority)
		return;

	/*if (WhizzAudioComp)
	{
		WhizzAudioComp->Stop();
	}
	*/

	// This should be where the grenade currently is as we need to trace to the eyes
	FVector StartTrace = Hit.TraceStart;
	/* End the trace so many units in front of the unit */
	FVector EndTrace = Hit.TraceEnd;
	FHitResult HitResult;

	static FName NAME_LineOfSight = FName(TEXT("LineOfSight"));
	FCollisionQueryParams CollisionParams(NAME_LineOfSight);
	CollisionParams.bReturnPhysicalMaterial = true;

	/* Do a line trace from center of screen to where we are looking, if it hits a weapon pick it up */
	//DrawDebugLine(GetWorld(), StartTrace, EndTrace, FColor::White, 1.0f, 0, 0.2f);
	GetWorld()->LineTraceSingleByChannel(HitResult, StartTrace, EndTrace, ECollisionChannel::ECC_GameTraceChannel1);
	
	if (ProjectileNumber == 0)
	{
		if (HitResult.GetComponent())
		{
			if (HitResult.GetComponent()->IsSimulatingPhysics())
			{
				HitResult.GetComponent()->AddImpulseAtLocation(/*Velocity*/-HitResult.ImpactNormal * PhysicsImpulseMultiplier, HitResult.ImpactPoint, HitResult.BoneName);
			}
		}
	}


	Multicast_SpawnImpactEffects_Implementation(Hit, nullptr, DecalScale);
	APlayerCharacter* pc = Cast<APlayerCharacter>(HitResult.GetActor());
	// If the thing we hit is a shield, then we need to run the shield hit effect
	ABallisticsShield* bs = Cast<ABallisticsShield>(Hit.GetActor());
	if (bs && bs->AnimationData)
	{
		pc = Cast<APlayerCharacter>(bs->GetOwner());
		if (pc)
		{
			pc->Play1PMontage(bs->AnimationData->ShieldHit.Body_FP);
			pc->Play3PMontage(bs->AnimationData->ShieldHit.Body_TP);
			bs->PlayFPMontage(bs->AnimationData->ShieldHit.Gun_FP);
			bs->PlayTPMontage(bs->AnimationData->ShieldHit.Gun_TP);
		}
	}
}

void ABulletProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (bEmbedded)
	{
		return;
	}

	// you can't hit yourself
	if (Hit.GetActor() == GetOwner())
		return;

	bEmbedded = true;

	if (GetLocalRole() >= ROLE_Authority)
	{   
		Multicast_ApplyForceToHitObjects(Hit, GetVelocity());
		if (OtherComp)
		{
			if (OtherComp->IsSimulatingPhysics())
			{
				OtherComp->AddImpulseAtLocation(/*GetVelocity()*/-Hit.ImpactNormal * PhysicsImpulseMultiplier, /*GetActorLocation()*/Hit.ImpactPoint, Hit.BoneName);
			}
		}
	}

	if (Cast<APlayerCharacter>(Hit.GetActor()))
	{
		bHasGoneThroughPlayer = true;
	}

	Super::OnHit(HitComponent, OtherActor, OtherComp, NormalImpulse, Hit);
	OnMeshHit(HitComponent, OtherActor, OtherComp, NormalImpulse, Hit);

	if (GetLocalRole() >= ROLE_Authority)
	{
		// ? shouldn't it be the person who is getting hit, who makes the sound?
		ACharacter* OwningChar = Cast<ACharacter>(GetOwner());
		if (OwningChar)
		{
			UAISense_Hearing::ReportNoiseEvent(GetWorld(), GetActorLocation(), 0.25f, OwningChar, 0.0f, "Gunshot");
		}

		if (bDestroyOnHit)
		{
			// We shouldn't destroy this immediately. Otherwise, BP events on other objects will have a dead bullet reference hanging in their function.
			// Instead the solution is to give this thing a small enough lifespan that it clears out in a frame or ten and don't let it hit twice. --eez
			//Destroy();
			this->SetLifeSpan(0.2f);
			SetActorHiddenInGame(true);
		}
		else if (bAttachOnHit)
		{
			FVector CurrentLocation = GetActorLocation();

			AttachToComponent(Hit.GetComponent(), FAttachmentTransformRules::KeepWorldTransform, Hit.BoneName);
			Multicast_AttachToComponent(CurrentLocation, Hit.GetComponent(), Hit.BoneName);
			Multicast_AttachToComponent_Implementation(CurrentLocation, Hit.GetComponent(), Hit.BoneName);
		}
		else
		{
			Multicast_SimulatePhysics(true);
			Multicast_SimulatePhysics_Implementation(true);
		}
	}

	UPhysicalMaterial* HitPhysMat = Hit.PhysMaterial.Get();
	EPhysicalSurface HitSurfaceType = UPhysicalMaterial::DetermineSurfaceType(HitPhysMat);
	float HitAngle = FMath::RadiansToDegrees(acosf(-FVector::DotProduct(Hit.Normal, GetActorForwardVector())));

	UPenetrationData* pd = UBpGameplayHelperLib::GetPenetrationData();
	// canot do any ricochet or piercing without the data
	if (!pd)
		return;

	FMaterialPenetration MaterialPenetration = pd->GetPenetrationData(HitSurfaceType);
	if (UBpGameplayHelperLib::GetRoNData()->bDrawBulletDebug)
	{
		DrawDebugLine(GetWorld(), StartPos, Hit.Location, FColor::Blue, false, 999999.0f, 0, DebugLineSize);
	}
	if (bDestroyOnHit && !bAttachOnHit && !Hit.bStartPenetrating)
	{
		FCollisionQueryParams CollisionQueryParams;
		CollisionQueryParams.bFindInitialOverlaps = true;
		CollisionQueryParams.bTraceComplex = false;

		FRotator newRotation = GetActorRotation();
		if (HitAngle > RequiredAngleToDeflect && FMath::RandRange(0, 1) < PercentageToDeflect && MaterialPenetration.bCanRicochet)
		{
			// Ricochet off of this surface.
			OnDeflect(Hit);

			Multicast_SpawnImpactEffects_Implementation(Hit, RicochetEffects, DecalScale, true);
			newRotation.Yaw += HitAngle * FMath::RandRange(-DeflectionAmount, DeflectionAmount);
			newRotation.Pitch += HitAngle * FMath::RandRange(-DeflectionAmount, DeflectionAmount) * 0.2f;

			int32 PenetrationCheckCount = PenetratedDistance;
			FHitResult LastHitResult;
			GetWorld()->LineTraceSingleByProfile(LastHitResult, Hit.ImpactPoint, Hit.ImpactPoint + newRotation.Vector() * 1.0f, GetCollisionComp()->GetCollisionProfileName(), CollisionQueryParams);

			while (PenetrationCheckCount < PenetrationDistance * MaterialPenetration.PenetrationMultiplier)
			{
				PenetrationCheckCount += 1;
				if (UBpGameplayHelperLib::GetRoNData()->bDrawBulletDebug)
				{
					DrawDebugLine(GetWorld(), LastHitResult.TraceStart, LastHitResult.TraceEnd, FColor::Orange, false, 999999.0f, 0, DebugLineSize);
				}

				GetWorld()->LineTraceSingleByChannel(LastHitResult, Hit.ImpactPoint, Hit.ImpactPoint + newRotation.Vector() * 1.0f, ECollisionChannel::ECC_WorldDynamic, CollisionQueryParams);
				if (!LastHitResult.bStartPenetrating)
				{
					bEmbedded = false;
					PenetratedDistance = PenetrationCheckCount;
					OnRespawnProjectile(LastHitResult.TraceEnd, newRotation, GetMovementComp()->InitialSpeed * SpeedLossMultiplierPerSurface * 0.5f, Damage * DamageLossMultiplierPerSurface, EProjectileReaction::PR_Richochet);
					break;
				}
			}
			
		}
		else if (MaterialPenetration.bIsPenetrable && bCanPenetrate)
		{
			// Find an exit penetration and spawn another projectile of the exact same type...
			int32 PenetrationCheckCount = PenetratedDistance;
			FHitResult LastHitResult;

			newRotation.Yaw += FMath::RandRange(-HitAngle * HitAngleMultiplier, HitAngle * HitAngleMultiplier);
			newRotation.Pitch += FMath::RandRange(-HitAngle * HitAngleMultiplier, HitAngle * HitAngleMultiplier) * 0.2f;

			GetWorld()->LineTraceSingleByProfile(LastHitResult, Hit.ImpactPoint, Hit.ImpactPoint + newRotation.Vector() * 1.0f, GetCollisionComp()->GetCollisionProfileName(), CollisionQueryParams);
			if (UBpGameplayHelperLib::GetRoNData()->bDrawBulletDebug)
			{
				DrawDebugLine(GetWorld(), LastHitResult.TraceStart, LastHitResult.TraceEnd, FColor::Orange, false, 999999.0f, 0, DebugLineSize);
			}
				
			while (PenetrationCheckCount < PenetrationDistance* MaterialPenetration.PenetrationMultiplier)
			{
				PenetrationCheckCount += 1;

				GetWorld()->LineTraceSingleByProfile(LastHitResult, LastHitResult.TraceEnd, LastHitResult.TraceEnd + newRotation.Vector() * 1.0f, GetCollisionComp()->GetCollisionProfileName(), CollisionQueryParams);

				if (UBpGameplayHelperLib::GetRoNData()->bDrawBulletDebug && LastHitResult.bStartPenetrating)
				{
					DrawDebugLine(GetWorld(), LastHitResult.TraceStart, LastHitResult.TraceEnd, FColor::Orange, false, 999999.0f, 0, DebugLineSize);
				}


				if (!LastHitResult.bStartPenetrating)
				{
					PenetratedDistance = PenetrationCheckCount;
					FHitResult ReversedHit = Hit;
					ReversedHit.ImpactNormal = -Hit.ImpactNormal;
					ReversedHit.ImpactPoint = LastHitResult.TraceStart;
					if (UBpGameplayHelperLib::GetRoNData()->bDrawBulletDebug)
					{
						DrawDebugLine(GetWorld(), LastHitResult.TraceStart, LastHitResult.TraceEnd, FColor::Yellow, false, 999999.0f, 0, DebugLineSize);
						/*DrawDebugLine(GetWorld(), LastHitResult.TraceStart, LastHitResult.TraceStart + Hit.ImpactNormal.BackwardVector * 100.0f, FColor::Yellow, false, 999999.0f, 0, DebugLineSize);*/
					}
					bEmbedded = false;
					// Hacky fix
					LastHitResult.ImpactPoint = LastHitResult.TraceEnd;
					Multicast_SpawnImpactEffects(ReversedHit, ExitEffects, DecalScale, true);
					OnRespawnProjectile(LastHitResult.TraceEnd, newRotation, GetMovementComp()->InitialSpeed * SpeedLossMultiplierPerSurface, Damage * DamageLossMultiplierPerSurface, EProjectileReaction::PR_Pierce);
					break;
				}

			}
		}
		else
		{
// 			if (UBpGameplayHelperLib::GetRoNData()->bDrawBulletDebug)
// 			{
// 				DrawDebugLine(GetWorld(), StartPos, Hit.Location, FColor::Yellow, false, 999999.0f, 0, DebugLineSize);
// 			}
		}
	} else
	{
		Multicast_SpawnImpactEffects(Hit);
	}
}

bool ABulletProjectile::IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const
{
	// If we're the client that fired, we don't want this projectile replicated to us as we have our own local version
	bool Relevant = Super::IsNetRelevantFor(RealViewer, ViewTarget, SrcLocation) && (RealViewer != GetInstigatorController());
	return Relevant;
}

void ABulletProjectile::OnProjectileValidated()
{
	bServerValidated = true;
}
