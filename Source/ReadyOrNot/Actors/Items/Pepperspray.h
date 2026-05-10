// Copyright Void Interactive, 2021

#pragma once

#include "Actors/BaseItem.h"
#include "Pepperspray.generated.h"

UCLASS(Abstract)
class READYORNOT_API APepperspray : public ABaseItem
{
	GENERATED_BODY()

public:
	APepperspray();

	//FORCEINLINE TMap<ECyberneticsLevel, float> GetDurationModifier() const { return DurationModifier; }
	
	FORCEINLINE float GetPepperSprayAbuseDebounce() const { return PepperSprayAbuseDebounce; }
	
	FORCEINLINE float GetDuration_Torso() const { return DurationFrontTorso; }
	FORCEINLINE float GetDuration_FrontFace() const { return DurationFrontFace; }
	FORCEINLINE float GetDuration_BackFace() const { return DurationBackFace; }

	virtual void OnItemPrimaryUse() override;
	virtual void OnItemPrimaryUseEnd() override;

	virtual bool PlayHolster() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Damage")
	TSubclassOf<UDamageType> DamageType = nullptr;
	
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	float SprayTime = 0.0f;
	float SprayTimeOnTarget = 0.0f;

	// The component responsible for the spray effect
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UParticleSystemComponent* SprayParticleComponent = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UAmmoComponent* AmmoComponent = nullptr;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pepperspray|Particle")
	UParticleSystem* ParticleStart = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pepperspray|Particle")
	UParticleSystem* ParticleEnd = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pepperspray|Particle")
	UParticleSystem* ParticleImpact = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pepperspray|Particle")
    UParticleSystem* ParticleRunningOut = nullptr;

	// The particle effects emitted by the spray
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pepperspray|Particle")
	UParticleSystem* ParticleSprayLoop = nullptr;
	
	UPROPERTY()
	UParticleSystemComponent* ParticleSprayLoopComponent = nullptr;

	// The FMOD event for spraying
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pepperspray|Audio")
	UFMODEvent* FMODSprayEvent = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pepperspray|Audio")
	UFMODEvent* FMODSprayEmptyEvent = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pepperspray|Audio")
	UFMODEvent* FMODImpactEvent = nullptr;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pepperspray|Audio")
    UFMODEvent* FMODSprayLowAmmoEvent = nullptr;
	
	// The distance that the spray can travel (its maximum range)
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Pepperspray|Settings")
	float SprayDistance = 600.0f;

	// Whether we are currently spraying
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Pepperspray")
	uint8 bSpraying : 1;

	UFUNCTION()
	void OnRep_Spraying();

	// The duration of the spray when it hits the front of our torso
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Pepperspray|Settings")
	float DurationFrontTorso = 12.0f;

	// The duration of the spray when it hits the back of our face
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Pepperspray|Settings")
	float DurationBackFace = 8.0f;

	// The duration of the spray when it hits the front of our face (maximum impact!)
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Pepperspray|Settings")
	float DurationFrontFace = 16.0f;

	// How much each Cybernetic level affects the stun duration
	//UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Pepperspray|Cybernetics")
	//TMap<ECyberneticsLevel, float> DurationModifier;

	// If we are sprayed for more than this amount of time when we are in an abuse state, then we count it
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pepperspray|Settings")
	float PepperSprayAbuseDebounce = 2.0f;

	UFUNCTION(BlueprintCallable)
	void StartSpraying();
	
	UFUNCTION(BlueprintCallable)
	void StopSpraying();
	
	UFUNCTION(NetMulticast, BlueprintCallable, Reliable)
	void PlaySprayParticleEffect(bool bRunningOutEffect = false);
	
	UFUNCTION(NetMulticast, BlueprintCallable, Reliable)
	void StopSprayParticleEffect();
	
	UFUNCTION(NetMulticast, BlueprintCallable, Reliable)
	void PlaySpraySoundEffect(bool bRunningOutEffect = false);
	
	UFUNCTION(NetMulticast, BlueprintCallable, Reliable)
	void StopSpraySoundEffect();
	
	UFUNCTION(Server, Unreliable, WithValidation)
			void Server_StartSpraying();
	virtual void Server_StartSpraying_Implementation();
	virtual bool Server_StartSpraying_Validate() { return true; }

	UFUNCTION(Server, Reliable, WithValidation)
			void Server_StopSpraying();
	virtual void Server_StopSpraying_Implementation();
	virtual bool Server_StopSpraying_Validate() { return true; }

	UFUNCTION()
	void OnLowPeppersprayAmmo(float CurrentResource);
	
	UFUNCTION()
	void OnDepletedPeppersprayAmmo();

private:
	void PlaySprayAnimation();
	void StopSprayAnimation();
	
	void StartSpraying_Internal();
	void StopSpraying_Internal();
	
	void SetupPepperspray();
	void SetupAudio();
};
