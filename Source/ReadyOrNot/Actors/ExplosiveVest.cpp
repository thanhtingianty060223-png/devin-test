// Copyright Void Interactive, 2022

#include "ExplosiveVest.h"

#include "AIController.h"
#include "Characters/CyberneticController.h"

static TAutoConsoleVariable<int32> CVarDrawExplosiveVestDebug(TEXT("a.RonDrawExplosiveVestDebug"), 0, TEXT("Visualize explosive vest inner and outer damage radii"));

AExplosiveVest::AExplosiveVest()
{
	PrimaryActorTick.bCanEverTick = true;
	bDisableTickWhenNotEquipped = false;
}

void AExplosiveVest::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// Clear our timers so we don't attempt a callback on a destroyed actor
	GetWorld()->GetTimerManager().ClearTimer(ExplosionStartTimer);
	GetWorld()->GetTimerManager().ClearTimer(ExplosionDamageTimer);
}

void AExplosiveVest::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!HasAuthority())
		return;

	if (!IsValid(GetOwnerCharacter()))
		return;

	// note(killo): we could hook into our owners onbeginkilled but this is probably safer
	if (!bVestExploded && bShouldExplodeOnDeath && GetOwnerCharacter()->IsDeadNotUnconscious())
		ExplodeVest();
}

bool AExplosiveVest::HandleDamage(float& Damage, FPointDamageEvent const& DamageEvent, AActor* DamageCauser)
{
	if (bShouldExplodeOnHit)
		ExplodeVest();

	return Super::HandleDamage(Damage, DamageEvent, DamageCauser);
}

void AExplosiveVest::ExplodeVest()
{
	if (!HasAuthority())
		return;

	if (!IsValid(GetOwnerCharacter()))
		return;

	if (bVestExploded || (!bShouldExplodeOnDeath && GetOwnerCharacter()->IsDeadOrUnconscious()))
		return;

	bVestExploded = true;

	Multicast_PlayPreExplosionEffects();

	const float Delay = FMath::RandRange(ExplosionEffectDelay - ExplosionEffectRandomDelay, ExplosionEffectDelay + ExplosionEffectRandomDelay);

	// Use a delay if it's greater than zero
	if (Delay > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(ExplosionStartTimer, this, &AExplosiveVest::StartExplosion, Delay);
	}
	else
	{
		// We use a timer to start an explosion next frame since it causes issues with animation BP I guess
		ExplosionStartTimer = GetWorld()->GetTimerManager().SetTimerForNextTick(this, &AExplosiveVest::StartExplosion);
	}
}

void AExplosiveVest::StartExplosion()
{
	if (!IsValid(GetOwnerCharacter()))
		return;

	Multicast_PlayExplosionEffects();

	AReadyOrNotCharacter* Character = GetOwnerCharacter();
	Character->Kill();
	Character->Multicast_OnExplosiveVestDetonation();

	if (ExplosionDamageDelay > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(ExplosionDamageTimer, this, &AExplosiveVest::ApplyExplosionDamage, ExplosionDamageDelay);
	}
	else
	{
		ApplyExplosionDamage();
	}
}

void AExplosiveVest::ApplyExplosionDamage()
{
	if (!IsValid(GetOwnerCharacter()))
		return;

	FVector Location = GetExplosiveVestTransform().GetLocation();

	TArray<AActor*> IgnoreActors;
	UGameplayStatics::ApplyRadialDamageWithFalloff(this, MaxDamageOnDetonation, MinDamageOnDetonation, Location, DamageInnerRadius, DamageOuterRadius, 1.0f, ExplosionDamageType, IgnoreActors, this, GetOwningAIController(), ECC_MAX);

#if !UE_BUILD_SHIPPING
	if (CVarDrawExplosiveVestDebug.GetValueOnGameThread() != 0)
	{
		DrawDebugSphere(GetWorld(), Location, DamageOuterRadius, 32, FColor::Yellow, false, 30.0f);
		DrawDebugSphere(GetWorld(), Location, DamageInnerRadius, 32, FColor::Blue, false, 30.0f);
	}
#endif
}

void AExplosiveVest::Multicast_PlayPreExplosionEffects_Implementation()
{
	FTransform Transform = GetExplosiveVestTransform();

	if (DetonationEvent)
	{
		UFMODBlueprintStatics::PlayEventAtLocation(GetWorld(), DetonationEvent, Transform, true);
	}
}

void AExplosiveVest::Multicast_PlayExplosionEffects_Implementation()
{
	FTransform Transform = GetExplosiveVestTransform();

	if (ExplosionScreenShake)
	{
		UGameplayStatics::PlayWorldCameraShake(GetWorld(), ExplosionScreenShake, GetActorLocation(), 0.0f, ExplosionScreenShakeRadius);
	}

	if (ExplosionParticle)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionParticle, Transform, true);
	}

	if (ExplosionEvent)
	{
		UFMODBlueprintStatics::PlayEventAtLocation(GetWorld(), ExplosionEvent, Transform, true);
	}
}

FTransform AExplosiveVest::GetExplosiveVestTransform()
{
	FTransform Transform = GetItemMesh()->GetComponentTransform();
	if (GetOwnerCharacter() && GetOwnerCharacter()->GetMesh())
		Transform = GetOwnerCharacter()->GetMesh()->GetSocketTransform(ExplosiveVestSocket);

	return Transform;
}