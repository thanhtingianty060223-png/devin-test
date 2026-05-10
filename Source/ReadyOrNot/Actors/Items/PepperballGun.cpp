// Copyright Void Interactive, 2022

#include "PepperballGun.h"

#include "ReadyOrNot.h"
#include "ReadyOrNotGameMode.h"

APepperballGun::APepperballGun()
{
	PrimaryActorTick.bCanEverTick = true;

	bShowParticlesWhenFiring = true;

	// Displacement trail
	ShootTrailComponent = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("Shot Trail Particle Component"));
	ShootTrailComponent->SetupAttachment(ItemMesh, "tag_muzzle");
}

void APepperballGun::BeginPlay()
{
	Super::BeginPlay();
	bSupressed = true;
}

void APepperballGun::SetMagazineCount(int32 Count, TArray<FName> AmmoTypes)
{
	BallsInHopper = MaxBallsInHopper;
}

void APepperballGun::Multicast_SpawnParticleEffects_Implementation(bool bSkipAuthority, bool bSkipLocalOwner)
{
	Super::Multicast_SpawnParticleEffects_Implementation(bSkipAuthority, bSkipLocalOwner);

	ShootTrailComponent->ActivateSystem();
}

void APepperballGun::Server_NextMagazine_Implementation()
{
	MagIndex += 1;
	if (Magazines.Num() >= 0 && !Magazines.IsValidIndex(MagIndex))
		MagIndex = 0;

	if (Magazines.Num() >= 0)
	{
		if (HasAnyAmmo() && Magazines[MagIndex].Ammo <= 0)
			Super::Server_NextMagazine_Implementation();
		else
		{
			int32 RequiredBalls = MaxBallsInHopper - BallsInHopper;
			BallsInHopper = FMath::Clamp(BallsInHopper + Magazines[MagIndex].Ammo, 0, MaxBallsInHopper);
			Magazines[MagIndex].Ammo = FMath::Clamp(Magazines[MagIndex].Ammo - RequiredBalls, 0, 100);
		}
	}
}

float APepperballGun::GetAmmo() const
{
	return BallsInHopper;
}

float APepperballGun::RemoveAmmo(float Value)
{
	BallsInHopper = (int32)FMath::Clamp(BallsInHopper - Value, 0.0f, (float)MaxBallsInHopper);
	return BallsInHopper;
}

void APepperballGun::IncrementStunShotCounter(class ACyberneticCharacter* StunnedPerson)
{
	CurrentStunShotCounter++;
	if (CurrentStunShotCounter % StunShotsUntilAbuse == 0)
	{
		AReadyOrNotGameMode::AddAbuse(Cast<APlayerCharacter>(GetOwner()), StunnedPerson);
	}
}

void APepperballGun::IncrementHeadshotCounter(class ACyberneticCharacter* HeadshottedPerson)
{
	CurrentHeadshotCounter++;
	if (CurrentHeadshotCounter % HeadshotsUntilAbuse == 0)
	{
		AReadyOrNotGameMode::AddAbuse(Cast<APlayerCharacter>(GetOwner()), HeadshottedPerson);
	}
}