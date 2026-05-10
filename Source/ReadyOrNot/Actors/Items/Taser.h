// Copyright Void Interactive, 2023

#pragma once

#include "Actors/BaseMagazineWeapon.h"
#include "Taser.generated.h"

/**
 * 
 */
UCLASS(Abstract)
class READYORNOT_API ATaser : public ABaseMagazineWeapon
{
	GENERATED_BODY()
	
public:
	ATaser();

	UPROPERTY()
	UMaterialInstanceDynamic* TaserLightDynamicMaterial;

	UPROPERTY(Replicated)
	FHitResult ProjectileHitResult;

	UPROPERTY(EditAnywhere)
	int32 CartridgesPerSlot = 1;

	UPROPERTY(EditDefaultsOnly, Category = Taser)
	int32 StartingCartridges = 6;

	UPROPERTY(EditAnywhere, Category = "Taser|Environment")
	float SweepForReactionVolumeSize = 800.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weight)
	float CartridgeWeight;

	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bFiredCartridge = false;

	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bDetachedProbes = false;

	UPROPERTY()
	float BlinkTime = 1.0f;

	UPROPERTY()
	bool bBlinkState = false;

	UPROPERTY(EditDefaultsOnly, Category = Taser)
	float MaxBatteryLevel = 100.0f;

	// Starting length of cable component
	UPROPERTY(EditDefaultsOnly, Category = Taser)
	float MinCableLength = 0.0f;

	// Max length of cable component
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Taser)
	float MaxCableLength = 70.0f;

	// How fast the cables gain in length
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Taser)
	float CableGainRate = 70.0f;

	// How far away we're allowed to get before the taser probes detach
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Taser)
	float ProbeMaxDistance = 2000.0f;

	virtual void SetMagazineCount(int32 Count, TArray<FName> AmmoTypes) override;

	// Detach the probes forcefully.
	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category = Taser)
			void DetachProbes();
	virtual void DetachProbes_Implementation();
	virtual bool DetachProbes_Validate() { return true; }

	// The sound that is made when the probes are detached.
	UPROPERTY(EditDefaultsOnly, Category = Taser)
	USoundBase* DetachSoundEffect;

	UPROPERTY(EditDefaultsOnly, Category = Taser)
	UFMODEvent* DetachSoundEffectFMOD;

	// How long the taser stun should last when we have hit the target
	UPROPERTY(EditDefaultsOnly, Category = Taser)
	float PingStunDuration = 8.0f;

	// How long is left in the stun
	UPROPERTY(BlueprintReadOnly, Replicated, Category = Taser)
	float StunDurationRemaining = 8.0f;

	// Whether we have started stunning the target or not
	UPROPERTY(BlueprintReadOnly, Replicated, Category = Taser)
	bool bStartedStun = false;

	// The sound effect to start playing while the probes are attached
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Taser)
	USoundBase* CrackleSoundEffect;

	// The sound effect to start playing while the probes are attached
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Taser)
	UFMODEvent* CrackleSoundEffectFMOD;

	// The sound effect to start playing while the probes are attached
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Taser)
	UFMODEvent* TaserHitEffectFMOD;

	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = Taser)
	void Multicast_PlayTaserHitEffect(FHitResult Hit);

	UPROPERTY(Replicated, BlueprintReadOnly)
	ABulletProjectile* LeftProjectile;
	UPROPERTY(Replicated, BlueprintReadOnly)
	ABulletProjectile* RightProjectile;

	bool bStartedParticleEffects = false;

	UPROPERTY(EditAnywhere)
	UParticleSystem* TaserImpactParticle_Start;

	UPROPERTY()
	UParticleSystemComponent* TaserImpactParticleComp_Start;

	UPROPERTY(EditAnywhere)
	UParticleSystem* TaserImpactParticle_Loop;

	UPROPERTY()
	UParticleSystemComponent* TaserImpactParticleComp_LoopLeft;

	UPROPERTY()
	UParticleSystemComponent* TaserImpactParticleComp_LoopRight;

	virtual void SetItemVisibility(bool bNewVisibility) override;

	FHitResult PreviousParticleHitResult;
	void StartTaserParticleEffects();
	void DestroyTaserParticleEffects();

	UFUNCTION()
	void OnRep_ProjectileReplicated();

	void ShowCables();

	bool bCrackling = false;

	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = Taser)
			void Multicast_StartCrackleSoundEffect();
	virtual void Multicast_StartCrackleSoundEffect_Implementation();

	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = Taser)
			void Multicast_StopCrackleSoundEffect();
	virtual void Multicast_StopCrackleSoundEffect_Implementation();

	UFUNCTION(Server, Reliable, WithValidation, Category = Taser)
			void Server_DeliverStunToAttachedTarget();
	virtual void Server_DeliverStunToAttachedTarget_Implementation();
	virtual bool Server_DeliverStunToAttachedTarget_Validate() { return true; }

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_HideCables();
	virtual void Multicast_HideCables_Implementation();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_DestroyProjectiles();
	virtual void Multicast_DestroyProjectiles_Implementation();

	UFUNCTION(NetMulticast, Reliable)
		void Multicast_ResetCableAttachments();
	virtual void Multicast_ResetCableAttachments_Implementation();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_ResetDoors();
	virtual void Multicast_ResetDoors_Implementation();

	UFUNCTION(NetMulticast, Reliable)
		void Multicast_PlayDetachEffect();
	virtual void Multicast_PlayDetachEffect_Implementation();

	UFUNCTION(NetMulticast, Reliable)
		void Multicast_PlayFireEffects(bool bDryFire);
	virtual void Multicast_PlayFireEffects_Implementation(bool bDryFire);
	
	UPROPERTY(EditDefaultsOnly, Category = Door)
	float DoorBlowOutForce = 200.0f;

	UPROPERTY(Replicated)
	AActor* LeftCableAttachActor;

	UPROPERTY(Replicated)
	AActor* RightCableAttachActor;

	UPROPERTY(EditDefaultsOnly, Category = Taser)
	UAnimMontage* TaserFireLoop1P;

	UPROPERTY(EditDefaultsOnly, Category = Taser)
	UAnimMontage* TaserFireLoop3P;

	UPROPERTY(EditDefaultsOnly, Category = Taser)
	TSubclassOf<ULegacyCameraShake> TaserFireLoopCameraShake;

	virtual void Server_OnFire_Implementation(FRotator Direction, FVector SpawnLoc, int32 Seed) override;
	virtual void OnFire(FRotator Direction, FVector SpawnLoc) override;
	bool bHoldingTaser = false;
	float TimeHoldingTaser = 0.0f;

	UFUNCTION(Server, Reliable, WithValidation)
			void Server_SetHoldingTaser(bool bNewHold);
	virtual void Server_SetHoldingTaser_Implementation(bool bNewHold);
	virtual bool Server_SetHoldingTaser_Validate(bool bNewHold);

	virtual void OnItemPrimaryUseEnd() override;
	virtual void OnWeaponTacticalReload() override;
	virtual void OnWeaponReload(bool bForce = false) override;
	
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable)
			void Server_DetachProbes();
	virtual void Server_DetachProbes_Implementation() { DetachProbes(); }
	virtual bool Server_DetachProbes_Validate() { return true; }

	void TaserStunFailed();

	virtual void Client_OnItemPickedUp_Implementation(AActor* NewOwner, bool bEquipped) override;

	virtual void Server_NextMagazine_Implementation() override;
	virtual float GetAmmoWeight(int32 Count) override;
	virtual bool PlayDraw(bool bDrawFirst) override;
	virtual bool PlayHolster() override;

	// Some stuff to fix the doors going bonkers
	virtual bool HandleMelee(FHitResult Hit) override;

	virtual float GetWeight() override;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh)
	class UCableComponent* TopCable;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh)
	class UCableComponent* BottomCable;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh)
	class USkeletalMeshComponent* LeftDoor;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh)
	class USkeletalMeshComponent* RightDoor;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh)
	class UAudioComponent* CrackleSoundGenerator;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh)
	class UFMODAudioComponent* CrackleSoundGeneratorFMOD;

	UPROPERTY(EditAnywhere, Category = Mesh)
	TSubclassOf<ULaserAttachment> LaserAttachmentClass;

private:
	// allow us to handle the melee when the impact effect actually occurs
	UFUNCTION()
	void HandleMeleeDeffered(FHitResult Hit);

	void SimulateDoors();
	
	FTimerHandle HandleMeleeDefferred_Handle;
	FTimerHandle ShowCables_Handle;
	
	FVector InitialLeftLocation;
	FVector InitialRightLocation;
	FVector InitialScale;
	FQuat InitialLeftQuat;
	FQuat InitialRightQuat;
};
