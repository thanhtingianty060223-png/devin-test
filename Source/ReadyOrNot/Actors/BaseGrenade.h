// Copyright Void Interactive, 2022

#pragma once

#include "Actors/BaseItem.h"
#include "BaseGrenade.generated.h"

USTRUCT(BlueprintType)
struct FGrenadeDamage
{
	GENERATED_USTRUCT_BODY()
	
	// The type of damage this detonation will inflict
	UPROPERTY(EditDefaultsOnly, Category = "Grenade")
	TSubclassOf<UDamageType> DamageType;

	// The damage to apply at beginning of falloff distance
	UPROPERTY(EditDefaultsOnly, Category = "Grenade")
	float MaxDamageOnDetonation = 5.0f;

	// The minimum damage to apply with falloff
	UPROPERTY(EditDefaultsOnly, Category = "Grenade")
	float MinDamageOnDetonation = 0.1f;

	// Distance that the maximum amount of damage is applied within
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade")
	float DamageInnerRadius = 0.0f;

	// Maximum distance that grenade damage is applied
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade")
	float DamageOuterRadius = 100.0f;

	// Whether or not to attempt a second trace to victims starting some distance away from grenade origin
	UPROPERTY(EditDefaultsOnly, Category = "Grenade")
	bool bUseSecondTrace = false;

	// Distance that we begin our second trace from the origin of the grenade
	UPROPERTY(EditDefaultsOnly, Category = "Grenade")
	float SecondTraceStartDistance = 0.0f;

	// Second trace radius factor for reducing the damage radius on second traces
	UPROPERTY(EditDefaultsOnly, Category = "Grenade", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float SecondTraceRadiusFactor = 1.0f;

	FGrenadeDamage() {};
};

UENUM(BlueprintType)
enum class EGrenadeType : uint8
{
	None,
	Flashbang,
	Stinger,
	Gas,
	Smoke,
	Frag,
	Custom
};

UCLASS(Abstract)
class READYORNOT_API ABaseGrenade : public ABaseItem, public IReceiveAISenseUpdates
{
	GENERATED_BODY()

public:

	ABaseGrenade();

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
		class UPointLightComponent* DetonationLight;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
		class URadialForceComponent* DetonationRadialForce;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Grenade|Effects")
		class UFMODAudioComponent* FMODBounceSoundComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade")
		class UAIPerceptionStimuliSourceComponent* DetonationStimuliComp;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade")
		float GrenadeBounciness = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade|Cybernetics")
		bool bDetonationTriggersStimuli = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade|Cybernetics")
		float DetonationSoundMaxRange = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Grenade|Cybernetics")
		float DetonationLoudness = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade|Cybernetics")
		FName DetonationTag = "GrenadeExplosion";
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade|Cybernetics")
		FName ThrownTag = "GrenadeThrown";
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade|Cybernetics")
		FName BounceTag = "GrenadeBounce";

	bool bHasYelledGrenade = false;

	bool bDetonateAtEndofPath = false;

	virtual bool ShouldEnableWeaponFovShader() const override;
	virtual bool ShouldAttachToOwner() const override;

	virtual void SpawnThrownItemAtTransform(const FTransform& Transform, const FVector& ThrowDirection, const FVector& ThrowLocation = FVector::ZeroVector) override;

	UPROPERTY()
	class AThrownGrenade* Thrown = nullptr;
	void SetDetonateAtEndOfPath(bool bShouldDetonateAtEndOfPath);

	void AIThrow();
	
	UPROPERTY(BlueprintReadWrite)
	uint8 bUsedFixedThrowTrajectory : 1;

	UPROPERTY()
		TArray<FVector> FirstBouncePath;
	UPROPERTY()
		FHitResult FirstBounceHit;
	bool bAppliedFirstBounce = false;
	UPROPERTY()
		TArray<FVector> SecondBouncePath;
	UPROPERTY()
		FHitResult SecondBounceHit;
	bool bAppliedSecondBounce = false;
	UPROPERTY()
		TArray<FVector> ThirdBouncePath;
	UPROPERTY()
		FHitResult ThirdBounceHit;
	bool bAppliedThirdBounce = false;

	UPROPERTY(ReplicatedUsing = OnRep_GrenadePath)
	TArray<FVector_NetQuantize> CompletePath;

	FVector LastSetWorldLocation = FVector::ZeroVector;
	UFUNCTION()
	void OnRep_GrenadePath();
	UPROPERTY(Replicated)
		int32 BouncePt1;
	UPROPERTY(Replicated)
		int32 BouncePt2;
	UPROPERTY(Replicated)
		int32 BouncePt3;

	int32 PathIdx = 0;

	UPROPERTY(EditAnywhere, Category="Grenade")
	FString DeployGrenadeVoiceLine;

	UPROPERTY(EditAnywhere, Category = "Grenade")
	float GrenadeSpeed = 1500.0f;
	float MaxGrenadeSpeed = 2500.0f;

	UFUNCTION(BlueprintPure, BlueprintCallable)
		bool IsOutside();

	UFUNCTION(NetMulticast, Reliable)
		void Multicast_AddImpulse(FVector Impulse, FVector FromLocation);
	virtual void Multicast_AddImpulse_Implementation(FVector Impulse, FVector FromLocation);

	bool bPinPulled = false; // only on server!
	bool bDeadDropped = false; // only on server!
	bool bStartedDetonating = false; // only on server!

	UFUNCTION(NetMulticast, Reliable)
		void Multicast_OnDeadDropped();
	virtual void Multicast_OnDeadDropped_Implementation();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade|Effects")
		float BounceSoundMinImpulse = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade|Effects")
		TArray<USoundBase*> BounceSoundEffects;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Grenade|Activation", meta = (AllowPrivateAccess = "true"))
		class UParticleSystemComponent* ActivationEffect;
		
	//UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Activation, meta = (AllowPrivateAccess = "true"))
	//class UParticleSystemComponent* GrenadeTrailEffect;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade|Activation", meta = (AllowPrivateAccess = "true"))
	class UParticleSystem* GrenadeBounceEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade|Activation")
	UFMODEvent* ActivationSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade|Activation")
	float ActivationTime;

	UPROPERTY(BlueprintReadOnly, Category = "Grenade|Activation")
	float ActivationElapsedTime;

	UPROPERTY(BlueprintReadOnly, Category = "Grenade|Activation")
	bool bActivated;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade|Effects")
	bool bPlayDetonationEffectsExactlyOnce = false;

	UPROPERTY(BlueprintReadOnly, Category = "Detonation")
	bool bDetonationEffectsPlayed = false;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	
	virtual bool IsDepleted() const override { return bUsed; }
	virtual bool ShouldHideInPictureInPictureScopes() override { return !bUsed; }

	virtual void OnItemPrimaryUse() override;
	virtual void OnItemPrimaryUseEnd() override;
	virtual void OnItemSecondaryUsed() override;
	virtual void OnItemEndSecondaryUse() override;
	virtual void OnItemUseComplete() override;

	UFUNCTION(Server, Reliable, WithValidation)
			void Server_ThrowGrenade(bool bOverarmThrow, FVector ThrowDirection, FVector ThrowStart = FVector::ZeroVector);
	virtual void Server_ThrowGrenade_Implementation(bool bOverarmThrow, FVector ThrowDirection, FVector ThrowStart = FVector::ZeroVector);
	virtual bool Server_ThrowGrenade_Validate(bool bOverarmThrow, FVector ThrowDirection, FVector ThrowStart = FVector::ZeroVector) { return true; }

	UFUNCTION(NetMulticast, Reliable)
		void Multicast_GrenadeThrow(bool bOverarmThrow, FVector ThrowDirection, FVector ThrowStart = FVector::ZeroVector);
	virtual void Multicast_GrenadeThrow_Implementation(bool bOverarmThrow, FVector ThrowDirection, FVector ThrowStart = FVector::ZeroVector);

	virtual bool PlayDraw(bool bDrawFirst) override;
	
	virtual void OnThrownFromInventory(AReadyOrNotCharacter* Thrower, bool bMarkAsEvidence = true) override;

	// Actually do the throw (and make sure multicast is not called twice for the local grenade so the player has a smoove experience)
	UFUNCTION(BlueprintCallable)
	virtual void Throw(bool bLocalOnly, bool bOverarmThrow, const FVector& ThrowDirection, const FVector& ThrowStart = FVector::ZeroVector);

	UFUNCTION(Server, Unreliable, WithValidation)
	void Server_UpdateThrowPosition(FVector Position, FRotator Rotation, FVector Velocity);

	UPROPERTY(BlueprintReadOnly)
	float TimeSinceUsed = 0.0f;
	
	UPROPERTY(BlueprintReadOnly)
	float TimeSinceDetonate = 0.0f;

	bool bForceNoSimulateGrenadePathOnThrow = false;

	void FullySimulateGrenadePath(FVector ThrowDirection, FVector ForcedStartPoint = FVector::ZeroVector);

	UPROPERTY()
	TEnumAsByte<EDrawDebugTrace::Type> DrawDebugType = EDrawDebugTrace::ForDuration;

	UFUNCTION(Server, Reliable, WithValidation)
		void UpdateServerPath(const TArray<FVector_NetQuantize>& Path, int32 Bounce1, int32 Bounce2, int32 Bounce3);
	virtual void UpdateServerPath_Implementation(const TArray<FVector_NetQuantize>& Path, int32 Bounce1, int32 Bounce2, int32 Bounce3);
	virtual bool UpdateServerPath_Validate(const TArray<FVector_NetQuantize>& Path, int32 Bounce1, int32 Bounce2, int32 Bounce3) { return true; }

	UPROPERTY(Replicated, BlueprintReadOnly, Category = Grenade)
		bool bThrowAsQuickThrow;

	UPROPERTY(BlueprintReadOnly, Category = Grenade)
		bool bFastThrowOnceEquipped;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = Grenade)
		bool bFastThrowing;

	UFUNCTION(Server, Reliable, WithValidation)
		void Server_StartFastThrow();
	virtual void Server_StartFastThrow_Implementation();
	virtual bool Server_StartFastThrow_Validate() { return true; }

	UFUNCTION(BlueprintCallable)
		void DoThrowFast();

	UFUNCTION(Server, Reliable, WithValidation)
		void Server_SetThrowOverarm(bool bThrowOverarm, bool bQuickThrow);
	virtual void Server_SetThrowOverarm_Implementation(bool bThrowOverarm, bool bQuickThrow);
	virtual bool Server_SetThrowOverarm_Validate(bool bThrowOverarm, bool bQuickThrow) { return true; }

	UPROPERTY(Replicated)
	bool bGrenadeReleased = false;

	bool bThrowStarted = false;

	bool bFullyPrimed = false;
	float PrimeTime = 0.0f;

	// Impulse to apply when throwing this grenade
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Grenade")
	float ThrowImpulse = 220.0f;

	// Additional up impulse to apply when throwing this grenade
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Grenade")
	float UpImpulse = 52.5f;

	// Triggers action music when it detonates. (CS gas doesn't trigger a music change...)
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Grenade")
	bool bTriggersActionMusic = true;

	UPROPERTY(BlueprintReadWrite, Category="Grenade")
		bool bCanThrowGrenade = false;

	UFUNCTION(BlueprintCallable, Category="Grenade")
		void SetFullyPrimed() { bFullyPrimed = true; }

	void OnOverarmThrowFinished();

	uint8 bStartedThorwingGrenade : 1;

	virtual void StunTick_Implementation(EStunType StunType) override;

	bool IsFullyPrimed() { return bFullyPrimed; }

	void DoThrowUnderarm();

	void DoThrowOverarm();

	virtual bool IsBlockingAnimationPlaying(TArray<EBlockingAnimationExclusion> Exclusions = {}) const override;

	UPROPERTY(EditDefaultsOnly, Category="Grenade")
	float DetonationTime = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category="Grenade")
		bool bIncreaseDamageRadiusOverTime = false;

	float Bounces = 0;

	UPROPERTY(EditDefaultsOnly, Category="Grenade")
		float UnderarmForceScale = 1.0f;

	UPROPERTY(VisibleAnywhere, Category="Grenade")
		float Drag = 0.0f;

	UPROPERTY(EditAnywhere, Category="Grenade")
		float DragAppliedPerBounce = 100.0f;

	UPROPERTY(EditDefaultsOnly, Category="Grenade")
		float DetonationFlashIntensitiy = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category="Grenade")
		float DetonationFlashInterp = 10.0f;

	UPROPERTY(EditDefaultsOnly, Category="Grenade")
		float ReDetonationTime = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category="Grenade")
		bool bTriggerSFXOnRedetonate = false;

	UPROPERTY(EditDefaultsOnly, Category="Grenade")
		float ThrowDistance = 1300.0f;

	UPROPERTY(EditDefaultsOnly, Category="Grenade")
	float RedotonateCount = 0;
	
	UPROPERTY(EditDefaultsOnly, Category = Grenade)
	uint8 bNoMoraleDamage : 1;

	// How far away we can trigger scripted IListenForGrenade listeners (such as killhouse targets, etc)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grenade")
		float ListenerTriggerDistance = 300.0f;
	
	bool bHasEverDetonated = false;
	uint32 CurrentDetonations = 0;

	uint8 bCanPlayBounceSound : 1;

	UFUNCTION()
	void OnGrenadeBounceSoundStopped();

	UPROPERTY(EditAnywhere, Category="Grenade")
		FVector MaxRandomizedForceOnDetonation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category="Grenade")
		FVector FixedForceOnDetonation = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, Category="Grenade")
	TArray<UParticleSystem*> DetonationParticles;

	UPROPERTY()
		TArray<UParticleSystemComponent*> SpawnedParticles;

	UPROPERTY(EditDefaultsOnly, Category="Grenade")
		FRotator ParticleSpawnRotation = FRotator::ZeroRotator;

	UPROPERTY(EditDefaultsOnly, Category="Grenade")
	UFMODEvent* DetonationFMODEvent;

	UPROPERTY(EditDefaultsOnly, Category="Grenade")
	USoundCue* DetonationEvent;

	UPROPERTY(EditDefaultsOnly, Category="Grenade")
		bool bHideGrenadeOnDetonate = false;

	void StartDetonationTimer();

	UFUNCTION(NetMulticast, Reliable)
			void Multicast_DetonationEffects(FVector CalculatedForce);
	virtual void Multicast_DetonationEffects_Implementation(FVector CalculatedForce);

	UFUNCTION(BlueprintCallable, Category="Grenade")
	virtual void Detonate();
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Grenade")
	EGrenadeType Type = EGrenadeType::None;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDetonate, ABaseGrenade*, Grenade);
	UPROPERTY(BlueprintAssignable)
	FOnDetonate OnGrenadeDetonated;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGrenadeThrown, ABaseGrenade*, ThrownGrenade);
	UPROPERTY(BlueprintAssignable)
	FOnGrenadeThrown OnGrenadeThrown;

	UPROPERTY(EditDefaultsOnly, Category="Grenade|Gameplay")
		float RedrawDelayAfterThrow = 0.5f;

	UPROPERTY(BlueprintReadOnly, Replicated, /*ReplicatedUsing=OnRep_GrenadeUsed,*/ Category="Grenade")
	bool bUsed = false;
	
	UPROPERTY(BlueprintReadOnly, Replicated, /*ReplicatedUsing=OnRep_GrenadeUsed,*/ Category="Grenade")
	AReadyOrNotCharacter* ThrownBy = nullptr;

	bool bAIThrowComplete = false;

	virtual void SetItemVisibility(bool bNewVisibility) override;

	UFUNCTION()
	void OnRep_GrenadeUsed();

	FTimerHandle DetonationTime_Handle;

	UPROPERTY(EditDefaultsOnly, Category="Grenade")
	TArray<FGrenadeDamage> DetonationDamage;

	// Whether to use screen shaking
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Grenade|Shake")
		bool bUseScreenShake = false;

	// The screen shake to use
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Grenade|Shake")
		TSubclassOf<ULegacyCameraShake> ExplosionScreenShake;

	// How far away to affect people with shakes
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Grenade|Shake")
		float CameraShakeRadius = 500.0f;

	// Whether to make a decal
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Grenade|Decals")
		bool bUseDetonationDecal = false;

	// How far up off the ground this thing can make a decal at
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Grenade|Decals")
		float DetonationDecalTraceDistance = 20.0f;

	// The decal to use - this isn't a soft path because otherwise it might cause a hitch during gameplay
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Grenade|Decals")
		UMaterialInterface* DetonationDecal = nullptr;

	// How big the decal should be when we spawn it
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Grenade|Decals")
		FVector DetonationDecalSize;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Grenade|Bones")
	TArray<FName> HideBonesOnUsed;

	void PlayFMODBounceSound();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grenade")
		float BounceActivationRadius = 100.0f;

	UPROPERTY(EditAnywhere, Category="Grenade|Audio")
		UFMODEvent* FMODGrenadeBounce;
	int32 BounceCount = 0;

	uint8 bSimulatedGrenadePath : 1; 

	virtual bool CanEquip(AReadyOrNotCharacter* ToCharacter) const override;

	virtual void OnPhysicsImpact(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;

	//Relevancy Interface
	virtual float GetRelevancyDistance_Implementation() { return 250.0f; }

	virtual void OnRep_AttachmentRep() override;

	FORCEINLINE class UPointLightComponent* GetDetonationLight() const { return DetonationLight; }
	FORCEINLINE class URadialForceComponent* GetRadialForceComponent() const { return DetonationRadialForce; }

	TArray<AActor*> GetIgnoredActorsForThrow() const;

	// More sophisticated radial damage method using a second trace
	static bool ApplyRadialDamageWithSecondTrace(const UObject* WorldContextObject, float SecondTraceDistance, float SecondTraceRadiusFactor, float BaseDamage, float MinimumDamage, const FVector& Origin, float DamageInnerRadius, float DamageOuterRadius, float DamageFalloff, TSubclassOf<class UDamageType> DamageTypeClass, const TArray<AActor*>& IgnoreActors, AActor* DamageCauser, AController* InstigatedByController, ECollisionChannel DamagePreventionChannel);

	virtual void OnAIHearingSense_Implementation(ACyberneticController* InSenseController, FAIStimulus Stimulus, AActor*& OutOverrideSensedActor) override;
	virtual void OnAIPerceptionSense_Implementation(ACyberneticController* InSenseController, FAIStimulus Stimulus, AActor*& OutOverrideSensedActor) override;

	// Sound occlusion parameters
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade|Occlusion")
	float GrenadeOcclusionMultiplier = 1.0f;

	// Depth to fully occlude gunshots (in cm)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade|Occlusion")
	float GrenadeFullOcclusionDepth = 150.0f;

private:
	virtual void PostNetReceivePhysicState() override;

protected:
	// Max distance discrepancy between server and local vals before position is snapped back to server vals
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade")
	float MaxDistanceDifference = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade")
	float InterpStrength = 1.f;

	// Rate at which client sends position of the grenade to the server to replicate to other clients (Limited by netupdatefrequency anyway)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade")
	float ClientReplicationFrequency = 0.025;

	FVector TargetLocation;
	FRotator TargetRotation;

	FTimerHandle TH_SendPositionUpdatesToServer;

	void SendLocationToServer();

	void EnableGrenadePhysics();
};
