// Copyright Void Interactive, 2024

#pragma once

#include "CoreMinimal.h"
#include "Actors/BaseItem.h"
#include "BaseArmour.generated.h"

/**
 *	Base armour class, provides common functionality for all armour
 */
UCLASS(Abstract)
class READYORNOT_API ABaseArmour : public ABaseItem
{
	GENERATED_BODY()

public:
	ABaseArmour();

	// Called when this piece of armour is hit, overridden by child classes for their unique damage logic
	virtual bool HandleDamage(float& Damage, FPointDamageEvent const& DamageEvent, AActor* DamageCauser) { return false; }

	// Returns true when a shot with the given hit result and ammo type will penetrate this armour, with optional output for spalling chance
	virtual bool CheckPenetration(const FHitResult& HitResult, const struct FAmmoTypeData* AmmoType, float* OutSpallingChance) { return true; }
	
	// Whether or not this armour provides any protection at all in its current state
	UFUNCTION(BlueprintPure)
	virtual bool HasRemainingProtection() const { return false; }

	// The remaining durability as a percentage value, 0.0 - 1.0
	UFUNCTION(BlueprintPure)
	virtual float GetDurabilityPercentage() const { return 0.0f; }

protected:
	float CalculateDurabilityDamageFactor(float Damage, const FAmmoTypeData* AmmoTypeData);
	
public:
	// Whether or not this armour is considered heavy, affects things like footsteps
	UPROPERTY(EditAnywhere, Category = "Armour")
	bool bIsHeavy = false;

	// TODO(killo): go into facewear
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Armour")
	float ScaleLensFlare = 1.0f;

	// When this is spawned in, we apply a few variations to the mesh
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Armour")
	TArray<USkeletalMesh*> Variations;
	
	// Particle effect to play whenever this armour is hit
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Armour")
	UParticleSystem* ArmourHitParticle = nullptr;

	// Sound to play whenever this armour is hit
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Armour")
	UFMODEvent* ArmourHitSound = nullptr;

	// Sound to play whenever this armour is hit while being worn, if applicable
	UPROPERTY(EditAnywhere, Category = "Armour")
	UFMODEvent* ArmourHitSoundFirstPerson = nullptr;

	// Paperdoll image for regular standing posture
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Armour|Paperdoll")
	UTexture2D* PaperdollTexture = nullptr;

	// Paperdoll image for regular crouched posture
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Armour|Paperdoll")
	UTexture2D* PaperdollTexture_Crouch = nullptr;
	
	// Paperdoll image for standing posture while carrying
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Armour|Paperdoll")
	UTexture2D* PaperdollTexture_Carry = nullptr;

	// Paperdoll image for crouched posture while carrying
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Armour|Paperdoll")
	UTexture2D* PaperdollTexture_Carry_Crouch = nullptr;
	
	// Screen shake used when the damage was fully intercepted, front side
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Armour|Screen Shake")
	TSubclassOf<ULegacyCameraShake> InterceptShakeFront;

	// Screen shake used when the damage was fully intercepted, back side
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Armour|Screen Shake")
	TSubclassOf<ULegacyCameraShake> InterceptShakeBack;

	// Screen shake used when the damage was fully intercepted, left side
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Armour|Screen Shake")
	TSubclassOf<ULegacyCameraShake> InterceptShakeLeft;

	// Screen shake used when the damage was fully intercepted, right side
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Armour|Screen Shake")
	TSubclassOf<ULegacyCameraShake> InterceptShakeRight;
};
