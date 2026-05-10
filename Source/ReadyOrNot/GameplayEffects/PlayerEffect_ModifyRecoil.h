// Copyright Void Interactive, 2021

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffects/BasePlayerEffect.h"
#include "Enums.h"
#include "PlayerEffect_ModifyRecoil.generated.h"

UENUM(BlueprintType)
enum class ERecoilModifierOption : uint8
{
	// Replaces the weapon's current recoil values with the new ones specified below
	RMO_ModifyRecoil	UMETA(DisplayName="Modify Recoil"),

	// Adds the new recoil values (specified below) to the weapon's current recoil values
	RMO_AddRecoil		UMETA(DisplayName="Add To Existing Recoil"),
	
	// Subtracts the new recoil values (specified below) from the weapon's current recoil values
	RMO_SubtractRecoil	UMETA(DisplayName="Subtract From Existing Recoil")
};

USTRUCT(BlueprintType)
struct FSpecificWeaponRecoilMod
{
	GENERATED_BODY()

	FSpecificWeaponRecoilMod()
	{
		RecoilFireStrength = 0.5f;
		RecoilFireStrengthFirst = 3.0f;
		RecoilDampStrength = 7.5f;
		RecoilAngleStrength = 0.4f;
		RecoilRandomness = 0.1f;
		RecoilFireADSModifier = 1.0f;
		RecoilBuildupADSModifier = 1.0f;
		RecoilAngleADSModifier = 1.0f;
		RecoilBuildupDampStrength = 15.0f;
	}
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	EItemClass WeaponClass = EItemClass::IC_AssaultRifle;

	// Specifies how strong the recoil is
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil Settings")
	float RecoilFireStrength = 0.5f;
	
	// Does the same but is only applied for the first shot in a rapid fire of burst fire modes
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil Settings")
	float RecoilFireStrengthFirst = 3.0f;

	// Controls the strength of damp effect
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Recoil Settings")
	float RecoilDampStrength = 7.5f;

	// Will specify the maximum deviation angle the recoil can have
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Recoil Settings")
	float RecoilAngleStrength = 0.4f;

	// Provides a more organic feeling to the recoil
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Recoil Settings")
	float RecoilRandomness = 0.1f;

	// Specifies how ADS influences the position recoil strength
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Recoil Settings")
	float RecoilFireADSModifier = 1.0f;

	// Scale buildup when using sights
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Recoil Settings")
	float RecoilBuildupADSModifier = 1.0f;

	// Specifies how ADS influences the angle recoil strength
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Recoil Settings")
	float RecoilAngleADSModifier = 1.0f;

	// Controls the strength of damp effect
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Recoil Settings")
	float RecoilBuildupDampStrength = 15.0f;
};

/**
 * Increases recoil to any gun the player has equipped
 */
UCLASS()
class READYORNOT_API UPlayerEffect_ModifyRecoil : public UBasePlayerEffect
{
	GENERATED_BODY()

public:
	UPlayerEffect_ModifyRecoil(const FObjectInitializer& ObjectInitializer);
	
protected:
	void Initialize_Implementation(AActor* InActor) override;

	void ApplyEffect_Implementation() override;

	void ResetEffect_Implementation() override;

	// Choose the behaviour of recoil modification
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil Settings")
	ERecoilModifierOption ModificationOption = ERecoilModifierOption::RMO_ModifyRecoil;

	// Only apply recoil to weapons of these classes
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil Settings", meta = (EditCondition = "!bApplySpecific"))
	TArray<EItemClass> WeaponFilter;
	
	// Specifies how strong the recoil is
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil Settings", meta = (EditCondition = "!bApplySpecific"))
	float RecoilFireStrength = 0.5f;
	
	// Does the same but is only applied for the first shot in a rapid fire of burst fire modes
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil Settings", meta = (EditCondition = "!bApplySpecific"))
	float RecoilFireStrengthFirst = 3.0f;

	// Controls the strength of damp effect
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Recoil Settings", meta = (EditCondition = "!bApplySpecific"))
	float RecoilDampStrength = 7.5f;

	// Will specify the maximum deviation angle the recoil can have
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Recoil Settings", meta = (EditCondition = "!bApplySpecific"))
	float RecoilAngleStrength = 0.4f;

	// Provides a more organic feeling to the recoil
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Recoil Settings", meta = (EditCondition = "!bApplySpecific"))
	float RecoilRandomness = 0.1f;

	// Specifies how ADS influences the position recoil strength
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Recoil Settings", meta = (EditCondition = "!bApplySpecific"))
	float RecoilFireADSModifier = 1.0f;

	// Scale buildup when using sights
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Recoil Settings", meta = (EditCondition = "!bApplySpecific"))
	float RecoilBuildupADSModifier = 1.0f;

	// Specifies how ADS influences the angle recoil strength
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Recoil Settings", meta = (EditCondition = "!bApplySpecific"))
	float RecoilAngleADSModifier = 1.0f;

	// Controls the strength of damp effect
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Recoil Settings", meta = (EditCondition = "!bApplySpecific"))
	float RecoilBuildupDampStrength = 15.0f;

	// Should we apply recoil to specific weapon classes in the player's inventory?
	// (True = Apply to the specific weapon classes specified. False = Use recoil values above to apply to all weapons in the player's inventory)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil Settings")
	uint8 bApplySpecific : 1;

	// The specific weapon classes to apply the new recoil values to
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil Settings", meta = (EditCondition = "bApplySpecific"))
	TArray<FSpecificWeaponRecoilMod> SpecificWeaponRecoilMods;
	
private:
	void ApplyRecoil(ABaseMagazineWeapon* AffectedWeapon);

	bool ShouldAffectWeapon(const class ABaseItem* Item);

	UPROPERTY()
	TMap<ABaseMagazineWeapon*, FSpecificWeaponRecoilMod> OriginalRecoilValues;

	UPROPERTY()
	TArray<class ABaseMagazineWeapon*> AffectedWeapons;
};
