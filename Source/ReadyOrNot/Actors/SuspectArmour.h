// Copyright Void Interactive, 2022

#pragma once

#include "Actors/BaseArmour.h"
#include "Engine/DataTable.h"
#include "SuspectArmour.generated.h"

/**
 *	Suspect Armour Data, holds all data table customization for suspect body armour and helmets
 */
USTRUCT(BlueprintType)
struct READYORNOT_API FSuspectArmourData : public FTableRowBase
{
	GENERATED_BODY()

	FSuspectArmourData();

	// Blueprint class to use for this armour
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftClassPtr<class ASuspectArmour> BlueprintClass;

	// Optional skeletal mesh for this armour
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	USkeletalMesh* Mesh;

	// Whether or not this armour is a helmet
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bIsHelmet = false;

	// Armour level, higher values tend to block more powerful rounds
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	uint8 ArmourLevel = 0;

	// Durability value, when zeroed armour offers no protection
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Durability = 0.0f;

	// Damage multiplier when this armour blocks a shot, 0.0 is no damage
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float DamageMultiplier = 0.25f;

	// Chance for spalling to occur whenever this armour blocks a shot
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float SpallingChance = 0.0f;
	
	// Movement speed multiplier, stacks with other armour. 1.0 for normal movespeed
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float MovementSpeedMultiplier = 1.0f;

	// Movement acceleration multiplier, stacks with other armour. 1.0 for normal acceleration
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float MovementAccelerationMultiplier = 1.0f;

	// Particle effect to play when this armour is hit
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UParticleSystem* HitParticleEffect;

	// Sound effect to play when this armour blocks a hit
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UFMODEvent* BlockedSoundEvent;

	// Sound effect to play when ths armour fails to block a hit
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UFMODEvent* PenetratedSoundEvent;

	// If set, this will override the footsteps set by the AI data table when equipped. Only applies to vests
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UFMODEvent* Footsteps;
};


/**
 *	Suspect armour
 */
UCLASS()
class READYORNOT_API ASuspectArmour : public ABaseArmour
{
	GENERATED_BODY()

protected:
	ASuspectArmour();

public:
	void SetArmourData(const FSuspectArmourData& Data);
	
	virtual bool HandleDamage(float& Damage, FPointDamageEvent const& DamageEvent, AActor* DamageCauser) override;
	virtual bool CheckPenetration(const FHitResult& HitResult, const FAmmoTypeData* AmmoType, float* OutSpallingChance) override;
	
private:
	bool ShotBlocked(const FAmmoTypeData* AmmoType) const;

	UFUNCTION()
	void OnRep_ArmourData();

private:
	UPROPERTY(ReplicatedUsing = OnRep_ArmourData)
	FSuspectArmourData ArmourData;

public:
	// Durability value, when zeroed armour offers no protection
	UPROPERTY(EditAnywhere, Category = "Suspect Armour")
	float Durability = 0.0f;

	FORCEINLINE bool IsHelmet() const { return ArmourData.bIsHelmet; }
	FORCEINLINE int32 GetArmourLevel() const { return ArmourData.ArmourLevel; }
	FORCEINLINE float GetDamageMultiplier() const { return ArmourData.DamageMultiplier; }
	FORCEINLINE float GetSpallingChance() const { return ArmourData.SpallingChance; }
	FORCEINLINE float GetArmourSpeedMultiplier() const { return ArmourData.MovementSpeedMultiplier; }
	FORCEINLINE float GetArmourAccelerationMultiplier() const { return ArmourData.MovementAccelerationMultiplier; }

	// Sound effect to play when this armour blocks a hit
	UPROPERTY(EditAnywhere, Category = "Suspect Armour")
	UFMODEvent* BlockedSoundEvent = nullptr;

	// Sound effect to play when this armour fails to block a hit
	UPROPERTY(EditAnywhere, Category = "Suspect Armour")
	UFMODEvent* PenetratedSoundEvent = nullptr;
};
