// Copyright Void Interactive, 2022

#pragma once

#include "Actors/BaseMagazineWeapon.h"
#include "PepperballGun.generated.h"

/**
 *
 */
UCLASS(Abstract)
class READYORNOT_API APepperballGun : public ABaseMagazineWeapon
{
	GENERATED_BODY()

public:
	APepperballGun();
	virtual void BeginPlay() override;
	
	virtual void SetMagazineCount(int32 Count, TArray<FName> AmmoTypes) override;

	void Multicast_SpawnParticleEffects_Implementation(bool bSkipAuthority, bool bSkipLocalOwner) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Cybernetics)
	UParticleSystemComponent* ShootTrailComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Cybernetics)
	UParticleSystem* ParticleShootTrail;
	
	// Number of headshots on civilians until you are docked for an abuse. Don't set to 0 or game will crash!
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Cybernetics)
		int32 HeadshotsUntilAbuse = 5;
	UPROPERTY(BlueprintReadOnly, Category = Cybernetics)
		int32 CurrentHeadshotCounter = 0;

	// Number of times you can hit stunned people until you are docked for abuse. Don't set to 0 or game will crash!
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Cybernetics)
		int32 StunShotsUntilAbuse = 10;
	UPROPERTY(BlueprintReadOnly, Category = Cybernetics)
		int32 CurrentStunShotCounter = 0;

	UFUNCTION(BlueprintCallable, Category = Cybernetics)
		void IncrementStunShotCounter(class ACyberneticCharacter* StunnedPerson);

	UFUNCTION(BlueprintCallable, Category = Cybernetics)
		void IncrementHeadshotCounter(class ACyberneticCharacter* HeadshottedPerson);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Ammo)
		int32 MaxBallsInHopper = 50;

	UPROPERTY(BlueprintReadOnly, Category = Ammo)
		int32 BallsInHopper = 50;

	virtual void Server_NextMagazine_Implementation() override;
	virtual float GetAmmo() const override;
	virtual float RemoveAmmo(float Value) override;
};
