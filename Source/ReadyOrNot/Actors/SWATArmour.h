// Copyright Void Interactive, 2024

#pragma once

#include "Actors/BaseArmour.h"
#include "SWATArmour.generated.h"

/**
 *	Armour coverage, represents the amount of armour plate coverage in a plate carrier
 */
UENUM(BlueprintType)
enum class EArmourCoverage : uint8
{
	AC_None = 0,
	AC_FrontOnly = 1,
	AC_FrontBack = 2,
	AC_FrontBackSides = 3,
};

/**
 *	Armour material, contains all stats for a specific armour material
 */
UCLASS(BlueprintType)
class READYORNOT_API UArmourMaterial : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// Damage reduction percentage, either when durability is above zero or when durability is disabled
	UPROPERTY(EditAnywhere)
	float DamageReduction = 1.0f;

	// Minimum amount of damage reduction, when durability is zero
	UPROPERTY(EditAnywhere)
	float MinDamageReduction = 1.0f;

	// Whether or not armour durability should impact damage reduction
	UPROPERTY(EditAnywhere)
	bool bDurabilityEnabled = false;

	// Starting durability value for this armor material
	UPROPERTY(EditAnywhere)
	float Durability = 0.0f;

	// Armour level, if this value exceeds the penetration power of ammo it will apply full damage reduction
	UPROPERTY(EditAnywhere)
	uint32 ArmourLevel = 0;

	// Chance for this armour material to cause spalling when not penetrated
	UPROPERTY(EditAnywhere)
	float SpallingChance = 0.0f;
	
	// Armour max speed modifier percentage per armour plate (negative value)
	UPROPERTY(EditAnywhere)
	float MovementSpeedModifier = 0.00f;

	// Armour max acceleration modifier percentage per armour plate (negative value)
	UPROPERTY(EditAnywhere)
	float MovementAccelerationModifier = -0.05f;

	// Particle effect to play when this armour material is hit
	UPROPERTY(EditAnywhere)
	UParticleSystem* HitParticle;

	// Display name in the loadout menu
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

	// Priority for display in the loadout menu, higher numbers are less prioritized
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Priority = 0;
	
	// Description used in the loadout menu
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Description;

	// Icon used in the loadout menu
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> LoadoutIcon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<EGameVersionRestriction> LockedToDLC;
};

/**
 *	SWAT armour, manages armour plates as a plate carrier and handles incoming damage
 */
UCLASS(Abstract)
class READYORNOT_API ASWATArmour : public ABaseArmour
{
	GENERATED_BODY()

public:
	ASWATArmour();

private:
	void GetDurabilityAndMaterial(const FHitResult& HitResult, float** OutDurability, const UArmourMaterial** OutMaterial);

public:
	virtual bool HandleDamage(float& Damage, FPointDamageEvent const& DamageEvent, AActor* DamageCauser) override;
	virtual bool CheckPenetration(const FHitResult& HitResult, const FAmmoTypeData* AmmoType, float* OutSpallingChance) override;
	
	virtual bool HasRemainingProtection() const override;
	virtual float GetDurabilityPercentage() const override;

	void SetArmourCoverage(EArmourCoverage Coverage);
	void SetArmourMaterial(const UArmourMaterial* Material);

	float GetArmourSpeedMultiplier() const;
	float GetArmourAccelerationMultiplier() const;

	// Maximum armour coverage of this carrier, represents available slots for armour plates
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Armour")
	EArmourCoverage MaxArmourCoverage;

	// Material of this carrier, determines protection when a shot hits somewhere outside of armour coverage
	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Armour", meta = (DisplayThumbnail = "false"))
	const UArmourMaterial* CarrierMaterial = nullptr;

	// The maximum number of item slots this armour provides
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Armour")
	int32 TotalSlots = 10;

	// The default primary ammo slots that this armour provides
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Armour")
	int32 DefaultPrimaryAmmoSlots = 5;

	// The default secondary ammo slots that this armour provides
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Armour")
	int32 DefaultSecondaryAmmoSlots = 2;

	// The default grenade slots that this armour provides
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Armour")
	int32 DefaultGrenadeSlots = 2;

	// The default tactical device slots that this armour provides
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Armour")
	int32 DefaultTacticalDeviceSlots = 1;

private:
	// The set armour plate coverage, set up externally with SetArmourCoverage
	UPROPERTY(EditInstanceOnly, Replicated, Category = "Armour")
	EArmourCoverage ArmourCoverage;

	// The set armour material, set up externally with SetArmourMaterial
	UPROPERTY(EditInstanceOnly, Replicated, Category = "Armour")
	const UArmourMaterial* ArmourMaterial = nullptr;

	// The durability of each armour plate
	UPROPERTY(Replicated)
	TArray<float> Durabilities;

	static const int32 NumPlates = 3;
};
