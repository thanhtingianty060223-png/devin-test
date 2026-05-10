// Copyright Void Interactive, 2021

#include "GrenadeLauncher.h"
#include "Actors/Projectiles/DamageProjectiles/GrenadeProjectile.h"
//#include "Components/BulletProjectileMovementComponent.h"

AGrenadeLauncher::AGrenadeLauncher()
{
	bNoLocalProjectile = false;
	bHasVisibleMags = false;

	ItemName = FText::FromString("Launcher");
}

void AGrenadeLauncher::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGrenadeLauncher, BouncePt1);
	DOREPLIFETIME(AGrenadeLauncher, BouncePt2);
	DOREPLIFETIME(AGrenadeLauncher, BouncePt3);
}

void AGrenadeLauncher::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	/*
	if (LastSpawnedProjectile && false)
	{
		AGrenadeProjectile* Projectile = Cast<AGrenadeProjectile>(LastSpawnedProjectile);
		if (Projectile)
		{
			if (Projectile != LastSimulatedGrenade && LastSimulatedGrenade)
			{
				pathIdx = 0;
				CompletePath.Empty();
				LastSimulatedGrenade->GetCollisionComp()->SetSimulatePhysics(true);
			}

			if (CompletePath.Num() > 0)
			{
				if (CompletePath.IsValidIndex(pathIdx))
				{
					Projectile->GetMovementComp()->SetUpdatedComponent(nullptr);
					Projectile->GetCollisionComp()->SetSimulatePhysics(false);
					GrenadeSpeed = 3000.0f;
					Projectile->SetActorLocation(UKismetMathLibrary::VInterpTo_Constant(Projectile->GetActorLocation(), CompletePath[pathIdx], DeltaSeconds, GrenadeSpeed));
					Projectile->AddActorWorldRotation(FRotator(GrenadeSpeed * 0.1f, GrenadeSpeed * 0.1f, GrenadeSpeed * 0.1f) * DeltaSeconds);
					if ((Projectile->GetActorLocation() - CompletePath[pathIdx]).Size() < 1.0f)
					{
						if (pathIdx == BouncePt1 || pathIdx == BouncePt2 || pathIdx == BouncePt3)
						{
							Projectile->AddActorWorldRotation(FRotator(GrenadeSpeed, GrenadeSpeed, GrenadeSpeed));
							GrenadeSpeed *= 0.8f;
							FVector Location = FirstBounceHit.ImpactPoint;
							FRotator Rotator = FirstBounceHit.ImpactNormal.Rotation() + FRotator(-90, 0, 0);
							if (pathIdx == BouncePt2)
							{
								Location = SecondBounceHit.ImpactPoint;
								Rotator = SecondBounceHit.ImpactNormal.Rotation() + FRotator(-90, 0, 0);

							}
							else if (pathIdx == BouncePt3)
							{
								Location = ThirdBounceHit.ImpactPoint;
								Rotator = ThirdBounceHit.ImpactNormal.Rotation() + FRotator(-90, 0, 0);
							}
							UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), BounceParticleEffect, FTransform(Rotator, Location));
							UFMODBlueprintStatics::PlayEventAtLocation(GetWorld(), BounceFMODEvent, FTransform(Rotator, Location), true);
						}
						//V_LOGM(LogReadyOrNot, "Grenade Hit pathIdx %s on [IsHost? %s]", *FString::FromInt(pathIdx), (GetLocalRole() == ROLE_Authority ? "true" : "false"));
						pathIdx++;
					}
				}
				else
				{
					pathIdx = 0;
					CompletePath.Empty();
					Projectile->GetCollisionComp()->SetSimulatePhysics(true);
					LastSpawnedProjectile = nullptr;
				}
			}
			else
			{
				FullySimulateGrenadePath(Projectile);
			}
		}
	}
	*/
}

void AGrenadeLauncher::UpdateServerPath_Implementation(const TArray<FVector_NetQuantize>& Path, int32 Bounce1, int32 Bounce2, int32 Bounce3)
{
	CompletePath = Path;
	BouncePt1 = Bounce1;
	BouncePt2 = Bounce2;
	BouncePt3 = Bounce3;
}

bool AGrenadeLauncher::UpdateServerPath_Validate(const TArray<FVector_NetQuantize>& Path, int32 Bounce1, int32 Bounce2, int32 Bounce3)
{
	return true;
}

void AGrenadeLauncher::FullySimulateGrenadePath(AGrenadeProjectile* Projectile)
{
	if (!IsLocallyControlled() && !Cast<ACyberneticCharacter>(GetOwner()))
		return;

	// fully simulate the grenade path as grenades are known to go through walls
	FVector OutLastTraceDestination;
	TArray<AActor*> IgnoredActors;
	IgnoredActors.Add(this);
	IgnoredActors.Add(GetOwner());
	IgnoredActors += (TArray<AActor*>)GetWorld()->GetGameState<AReadyOrNotGameState>()->AllItems;

	UGameplayStatics::Blueprint_PredictProjectilePath_ByTraceChannel(GetWorld(), FirstBounceHit, FirstBouncePath, OutLastTraceDestination, GetBulletSpawn()->GetComponentLocation(), (GetBulletSpawn()->GetComponentRotation().Vector())*LaunchDistance, true, 5.0f, ECollisionChannel::ECC_WorldDynamic, true, IgnoredActors, EDrawDebugTrace::None, 3.0f);
	FVector OutLastTraceDestinationBounce;
	FVector MirroredVector = UKismetMathLibrary::MirrorVectorByNormal(FirstBounceHit.TraceEnd - FirstBounceHit.TraceStart, FirstBounceHit.ImpactNormal);
	UGameplayStatics::Blueprint_PredictProjectilePath_ByTraceChannel(GetWorld(), SecondBounceHit, SecondBouncePath, OutLastTraceDestinationBounce, FirstBouncePath[FirstBouncePath.Num() - 1], (MirroredVector * GrenadeBounciness), true, 5.0f, ECollisionChannel::ECC_WorldDynamic, true, IgnoredActors, EDrawDebugTrace::None, 3.0f);

	FVector OutLastTraceDestinationBounce2;
	FVector MirroredVector2 = UKismetMathLibrary::MirrorVectorByNormal(SecondBounceHit.TraceEnd - SecondBounceHit.TraceStart, SecondBounceHit.ImpactNormal);
	UGameplayStatics::Blueprint_PredictProjectilePath_ByTraceChannel(GetWorld(), ThirdBounceHit, ThirdBouncePath, OutLastTraceDestinationBounce2, SecondBouncePath[SecondBouncePath.Num() - 1], (MirroredVector2 * GrenadeBounciness), true, 5.0f, ECollisionChannel::ECC_WorldDynamic, true, IgnoredActors, EDrawDebugTrace::None, 3.0f);
	CompletePath.Empty();
	CompletePath.Append(FirstBouncePath);
	CompletePath.Append(SecondBouncePath);
	CompletePath.Append(ThirdBouncePath);

	BouncePt1 = FirstBouncePath.Num() - 1;
	BouncePt2 = BouncePt1 + SecondBouncePath.Num() - 1;
	BouncePt3 = BouncePt3 + ThirdBouncePath.Num() - 1;

	UpdateServerPath(CompletePath, BouncePt1, BouncePt2, BouncePt3);
	LastSimulatedGrenade = Projectile;
	AppliedGrenadeProjectilePaths.Add(Projectile);
}
