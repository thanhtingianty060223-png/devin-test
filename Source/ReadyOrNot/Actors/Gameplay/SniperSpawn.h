// Copyright Void Interactive, 2017

#pragma once
#include "GameFramework/Actor.h"
#include "Components/ArrowComponent.h"
#include "Characters/SniperCharacter.h"
#include "SniperSpawn.generated.h"

/*
 *	Sniper spawn points are meant to be locations that sniper characters spawn from, when a personnel calls for them.
 *	FIXME: Maybe we should consider grouping this with AISpawn somehow, polymorphically?
 *	@author	Nick Whitlock <eezstreet>
 */
UCLASS(Blueprintable, BlueprintType)
class READYORNOT_API ASniperSpawn : public AActor
{
	GENERATED_BODY()

protected:
	UFUNCTION()
	void SpawnPersonnelOfClass(TSubclassOf<ASniperCharacter> CharacterClass, int32 Designation);

public:
	ASniperSpawn();

	// The name of this spawn point, as referenced by the personnel data.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sniper)
	FName SpawnLabel;

	// Whether the sniper that is spawned should have its movement locked.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sniper)
	bool bMovementLocked = false;

	// Whether the sniper that is spawned should start out in ADS
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sniper)
	bool bStartInADS = false;

	// Whether the sniper that is spawned should be locked in ADS
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sniper)
	bool bLockADS = false;

	// The class of sniper to spawn
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sniper)
	TSubclassOf<ASniperCharacter> SniperClass;

	// The class of spotter to spawn
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sniper)
	TSubclassOf<ASniperCharacter> SpotterClass;

	// The class of marksman to spawn
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sniper)
	TSubclassOf<ASniperCharacter> MarksmanClass;

	// The direction that this sniper will start in
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Sniper)
	UArrowComponent* SpawnDirection;

	// The root scene component (required)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Sniper)
	USceneComponent* RootSceneComponent;

	// True if the sniper has spawned. False otherwise.
	UPROPERTY(BlueprintReadOnly, Category = Sniper)
	bool bHasSniperSpawned = false;

	// If this is valid, this will point to the sniper that has spawned.
	UPROPERTY(BlueprintReadOnly, Category = Sniper)
	ASniperCharacter* SpawnedSniper = nullptr;

	// Spawn the sniper, if it hasn't already.
	UFUNCTION(BlueprintCallable, Category = Sniper)
	void SpawnSniperHere(int32 Designation);

	// Spawn the marksman, if it hasn't already spawned something.
	UFUNCTION(BlueprintCallable, Category = Sniper)
	void SpawnMarksmanHere(int32 Designation);

	// Spawn the spotter, if it hasn't already spawned something.
	UFUNCTION(BlueprintCallable, Category = Sniper)
	void SpawnSpotterHere(int32 Designation);
};
