#include "SniperSpawn.h"
#include "ReadyOrNot.h"

ASniperSpawn::ASniperSpawn()
{
	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = RootSceneComponent;

	SpawnDirection = CreateDefaultSubobject<UArrowComponent>(TEXT("SpawnDirection"));
	SpawnDirection->SetupAttachment(RootSceneComponent);
}

void ASniperSpawn::SpawnPersonnelOfClass(TSubclassOf<ASniperCharacter> CharacterClass, int32 Designation)
{
	if (bHasSniperSpawned)
	{
		return;
	}

	// Spawn the thing.
	FVector SpawnLocation = SpawnDirection->GetComponentLocation();
	FRotator SpawnRotation = SpawnDirection->GetComponentRotation();
	AActor* TrySpawnSniper = GetWorld()->SpawnActor(CharacterClass.Get(), &SpawnLocation, &SpawnRotation);
	SpawnedSniper = Cast<ASniperCharacter>(TrySpawnSniper);

	// Set properties on the sniper
	if (SpawnedSniper != nullptr)
	{
		SpawnedSniper->Designation = Designation;
		SpawnedSniper->Server_BaseAimRotation = SpawnDirection->GetComponentRotation();
		SpawnedSniper->SetActorRotation(SpawnDirection->GetComponentRotation());
		bHasSniperSpawned = true;

		// Lock our movement (if the spawn point dictates it)
		if (bMovementLocked)
		{
			SpawnedSniper->LockMovement();
		}

		// If we start in ADS, put us there.
		if (bStartInADS)
		{
			SpawnedSniper->SecondaryUse();
		}

		// If the ADS is to be locked, lock it.
		if (bLockADS)
		{
			SpawnedSniper->bADSLocked = true;
		}
	}
}

void ASniperSpawn::SpawnSniperHere(int32 Designation)
{
	SpawnPersonnelOfClass(SniperClass, Designation);
}

void ASniperSpawn::SpawnMarksmanHere(int32 Designation)
{
	SpawnPersonnelOfClass(MarksmanClass, Designation);
}

void ASniperSpawn::SpawnSpotterHere(int32 Designation)
{
	SpawnPersonnelOfClass(SpotterClass, Designation);
}
