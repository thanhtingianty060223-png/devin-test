// Copyright Void Interactive, 2024

#pragma once

#include "Actors/BaseArmour.h"
#include "Headwear.generated.h"

/**
 *	Headwear class, handles SWAT headwear armour, audio, effects and HUD overlays
 */
UCLASS(Abstract)
class READYORNOT_API AHeadwear : public ABaseArmour
{
	GENERATED_BODY()

public:
	AHeadwear();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	virtual bool HandleDamage(float& Damage, FPointDamageEvent const& DamageEvent, AActor* DamageCauser) override;
	virtual bool CheckPenetration(const FHitResult& HitResult, const FAmmoTypeData* AmmoType, float* OutSpallingChance) override;
	
	virtual bool HasRemainingProtection() const override { return Durability > 0.0f; }
	virtual float GetDurabilityPercentage() const override { return Durability > 0.0f ? 1.0f : 0.0f; }

	// Should this headwear come with a helmet?
	UPROPERTY(EditAnywhere, Category = "Headwear")
	bool bHasHelmet = true;
	
	// How many shots this headwear can take before damage is applied
	UPROPERTY(EditAnywhere, Replicated, Category = "Headwear")
	float Durability = 0.0f;

	// The amount of damage reduction that is applied with durability remaining
	UPROPERTY(EditAnywhere, Category = "Headwear")
	float DamageReduction = 1.0f;

	// Percent chance that this headwear will get lucky and ricochet a projectile
	UPROPERTY(EditAnywhere, Category = "Headwear")
	float RicochetChance = 0.0;

	// Sound event to play when a ricochet occurs off this headwear
	UPROPERTY(EditAnywhere, Category = "Headwear")
	UFMODEvent* RicochetEvent;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
    bool bProtectsAgainstInstantKnockout = false;

	void IsOverridingBreathingSounds(bool& bIsOverridingBreathingSounds, TArray<USoundCue*>& BreathingSounds, float BreathGap);
	bool IsUsingPostProcess();
	
	FPostProcessSettings GetMaskSupressionPostProcess();

	UFMODEvent* GetVoiceLineEventSpatalizedOverride() { return VoiceLineEventOverrideSpatalized; };
	UFMODEvent* GetVoiceLineEventLocalOverride() { return VoiceLineEventOverrideLocal; };

	UPROPERTY(EditAnywhere, Category = "Voice Lines")
	bool bUseMaskVoiceFilter = false;
	
protected:
	void Tick(float DeltaSeconds) override;
	float GetWeight() override;

	UPROPERTY(EditAnywhere, Category = "Voice Lines")
	UFMODEvent* VoiceLineEventOverrideSpatalized;

	UPROPERTY(EditAnywhere, Category = "Voice Lines")
	UFMODEvent* VoiceLineEventOverrideLocal;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD")
	TSubclassOf<UUserWidget> MaskOverlay;

	UPROPERTY(EditDefaultsOnly, Category = "Post Process")
	bool bEnablePostProcess = false;

	UPROPERTY(EditDefaultsOnly, Category = "Post Process")
	FPostProcessSettings MaskPostProcess;

	UPROPERTY(EditDefaultsOnly, Category = "Post Process")
	FPostProcessSettings MaskSupressionPostProcess;

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	bool bOverrideBreathingSound = false;

	UPROPERTY(EditDefaultsOnly, Category = "Audio", meta = (EditCondition = "bOverrideBreathingSound"))
	TArray<USoundCue*> OverriddenBreathingSounds;

	UPROPERTY(EditDefaultsOnly, Category = "Audio", meta = (EditCondition = "bOverrideBreathingSound"))
	float GapBetweenBreaths = 2.5f;

	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	bool bSpawnedOverlay = false;
	
private:
	void SpawnMaskOverlay(APlayerController* OwningController);
	void DestroyMaskOverlay();
	
	UPROPERTY()
	UUserWidget* SpawnedMaskOverlay;
};
