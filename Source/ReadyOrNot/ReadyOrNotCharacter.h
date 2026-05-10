// Copyright Void Interactive, 2023

#pragma once

#include "Enums.h"
#include "Structs.h"

#include "Animation/ReadyOrNotCharacterAnimData.h"

#include "Components/CharacterHealthComponent.h"
#include "Components/InventoryComponent.h"

#include "Interfaces/CanUseMultitoolOn.h"
#include "GenericTeamAgentInterface.h"
#include "Actors/Items/Pepperspray.h"
#include "Components/GibComponent.h"
#include "Navigation/CrowdAgentInterface.h"
#include "Perception/AISightTargetInterface.h"
#include "Perception/AIPerceptionComponent.h"

#include "DamageTypes/StunDamage.h"
#include "Data/TOCData.h"
#include "Interfaces/ListenForGameEnd.h"

#include "Interfaces/Meleeable.h"
#include "Interfaces/Reportable.h"
#include "Navigation/CrowdManager.h"

#include "Actors/Sound/SoundSource.h"

#include "ReadyOrNotCharacter.generated.h"

USTRUCT()
struct FCharacterSnapshot
{
	GENERATED_BODY()

	UPROPERTY()
	float Time;

	UPROPERTY()
	FBox BoundingBox;
};

/**
 * FRonReplicatedAcceleration: Compressed representation of acceleration
 */
USTRUCT()
struct FRonReplicatedAcceleration
{
	GENERATED_BODY()

	UPROPERTY()
	uint8 AccelXYRadians = 0;	// Direction of XY accel component, quantized to represent [0, 2*pi]

	UPROPERTY()
	uint8 AccelXYMagnitude = 0;	//Accel rate of XY component, quantized to represent [0, MaxAcceleration]

	UPROPERTY()
	int8 AccelZ = 0;	// Raw Z accel rate component, quantized to represent [-MaxAcceleration, MaxAcceleration]
};

USTRUCT(BlueprintType)
struct FCharacterDamageEvent
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float RawDamage = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float FinalDamage = 0.0f;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FDamageEvent DamageEvent;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	AController* Instigator = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	AActor* Causer = nullptr;
};

UENUM(BlueprintType)
enum class ECharacterEmotion : uint8
{
	None,
	Angry,
	Wince,
	Sad,
	Afraid,
	Alert
};

DECLARE_STATS_GROUP(TEXT("ReadyOrNotCharacter"), STATGROUP_ReadyOrNotCharacter, STATCAT_Advanced);

UCLASS(config=Game, BlueprintType, Blueprintable)
class READYORNOT_API AReadyOrNotCharacter : public ACharacter, 
	public ICrowdAgentInterface, public IGenericTeamAgentInterface, public IAISightTargetInterface, 
	public IReportable, public ICanUseMultitoolOn, public IGetFriendlyName, public IUseabilityInterface,
	public IGatherDebugInterface, public IPingInterface, public IMeleeable, public IScoringInterface,
	public IListenForGameEnd, public IReceiveAISenseUpdates
{
	GENERATED_BODY()

public:
	explicit AReadyOrNotCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(Transient)
	USkeletalMeshComponent* CustomizationFaceMesh;
	
	UPROPERTY(Transient, VisibleInstanceOnly, Category="Customization")
	TArray<USkeletalMeshComponent*> CustomizationSkeletalMeshes;

	UPROPERTY(Transient, VisibleInstanceOnly, Category="Customization")
	TArray<UStaticMeshComponent*> CustomizationStaticMeshes;

	UPROPERTY(Transient)
	TArray<AActor*> CustomizationActors;

	UPROPERTY(ReplicatedUsing=OnRep_Customization)
	FSavedCustomization Customization;

	UFUNCTION()
	void OnRep_Customization();

	TMap<FName, FName> SocketOverridesMap;
	FName GetSocketOverride(FName Socket);
	
	FORCEINLINE class USceneComponent* GetTeamViewTarget() const { return CustomizationFaceMesh ? CustomizationFaceMesh : FaceMesh; }
	
protected:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class USkeletalMeshComponent* MeshGearSlot = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UInteractableComponent* InteractableComponent = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UObjectiveMarkerComponent* PlayerMarkerComponent = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class USkeletalMeshComponent* FaceMesh = nullptr;

	//UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	//class UMapActorComponent* MapActorComponent = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UFMODAudioPropagationComponent* FMODAudioPropagationComp = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UFMODAudioComponent* FMODVoiceAudioComp = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Replicated, Category = "Components")
	class UCharacterHealthComponent* CharacterHealth = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UInventoryComponent* InventoryComp = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UAIPerceptionStimuliSourceComponent* PerceptionStimuliComp = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UGibComponent* GibComponent = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Replicated, Category = "Components")
	class UPhysicalAnimationComponent* PhysicalAnimationComp = nullptr;

	// Enables character to receive skinned decals
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class USkinnedDecalSampler* SkinnedDecalSampler;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Replicated, Category = "Components")
	class URagdollComponent* RagdollComponent = nullptr;

	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void Tick_Authority(float DeltaSeconds);
	
	#if !UE_BUILD_SHIPPING
	virtual void Tick_Debug(float DeltaSeconds);
	#endif
	
public:
	FORCEINLINE class USkeletalMeshComponent* GetFaceMesh() const { return FaceMesh; }
	FORCEINLINE	class USkeletalMeshComponent* GetMeshGearSlot() const { return MeshGearSlot; }
	FORCEINLINE class UAIPerceptionStimuliSourceComponent* GetPerceptionStimuliSourceComp() const { return PerceptionStimuliComp; }
	FORCEINLINE class UFMODAudioPropagationComponent* GetFMODPropagationComponent() const { return FMODAudioPropagationComp; }
	FORCEINLINE class UInventoryComponent* GetInventoryComponent() const { return InventoryComp; }
	FORCEINLINE class UGibComponent* GetGibComponent() const { return GibComponent; }
	FORCEINLINE class UFMODAudioComponent* GetAudioComp() const { return FMODVoiceAudioComp; }
	//FORCEINLINE class UPhysicalAnimationComponent* GetPhysicalAnimationComp() const { return PhysicalAnimationComp; }
	FORCEINLINE class USkinnedDecalSampler* GetSkinnedDecalSampler() const { return SkinnedDecalSampler; }
	FORCEINLINE class UFMODAudioComponent* GetFMODVoiceAudioComp() const { return FMODVoiceAudioComp; }
	FORCEINLINE class URagdollComponent* GetRagdollComponent() const { return RagdollComponent; }
	
	UFUNCTION(BlueprintPure, Category = "Player | Health")
	FORCEINLINE class UCharacterHealthComponent* GetHealthComponent() const { return CharacterHealth; }

	virtual FVector GetTargetLocation(AActor* RequestedBy) const override;
	
	FRotator DeltaRotation;
	FRotator LastFrameRotation;

	FVector DeltaLocation;
	FVector LastFrameLocation;

	UFUNCTION(BlueprintPure, Category = "Player | Health")
    float GetCurrentHealth() const;
    
	UFUNCTION(BlueprintPure, Category = "Player | Health")
    float GetMaxHealth() const;
    
	UFUNCTION(BlueprintPure, Category = "Player | Health")
    float GetCurrentReviveHealth() const;

	UFUNCTION(BlueprintPure, Category = "Player | Health")
    float GetCurrentReviveTime() const;

	UFUNCTION(BlueprintPure, Category = "Player | Health")
    bool IsFullHealth() const;

	UFUNCTION(BlueprintPure, Category = "Player | Health")
	bool IsLowHealth() const;
	
	UFUNCTION(BlueprintPure, Category = "Player | Health")
	bool IsHalfHealth() const;

	UFUNCTION(BlueprintPure, Category = "Player | Health")
	bool IsHealthDepleted() const;
	
	UFUNCTION(BlueprintPure, Category = "Player | Health")
	bool IsReviveHealthDepleted() const;

	UFUNCTION(BlueprintPure, Category = "Player | Health")
	bool IsDowned() const;
	
	UFUNCTION(BlueprintPure, Category = "Player | Health")
	bool UsingReviveSystem() const;

	UFUNCTION(BlueprintCallable, Category = "Player | Health")
	void IncreaseHealth(float Amount);
	
	UFUNCTION(BlueprintCallable, Category = "Player | Health")
	void DecreaseHealth(float Amount);
	
	UFUNCTION(BlueprintCallable, Category = "Player | Health")
	void DepleteHealth();
	
	UFUNCTION(BlueprintCallable, Category = "Player | Health")
	void ResetHealth();

	UFUNCTION(BlueprintPure, Category = Health)
	bool AnyBodyPartHit() const;
	
	UFUNCTION(BlueprintPure, Category = Health)
	bool IsAnyLimbHit() const;
	
	UFUNCTION(BlueprintPure, Category = Health)
	bool IsLimbHit(ELimbType Limb) const;
	
	UFUNCTION(BlueprintPure, Category = Health)
	bool IsLimbBroken(ELimbType Limb) const;
	
	UFUNCTION(BlueprintPure, Category = Health)
	FLimbHealthData GetLimbHealth(ELimbType Limb) const;

	UFUNCTION(BlueprintPure, Category = Health)
	virtual bool IsActive() const;
	
	UFUNCTION(BlueprintPure, Category = Health)
	virtual bool IsActiveForMovement() const;
	
	bool IsActiveForVO() const;

	// Is this person dead or unconscious?
	UFUNCTION(BlueprintPure, Category = Health)
    bool IsDeadOrUnconscious() const;
	
	UFUNCTION(BlueprintPure, Category = Health)
    bool IsInjured() const;
	
	UFUNCTION(BlueprintPure, Category = Health)
    bool IsIncapacitated() const;

	UFUNCTION(BlueprintPure, Category = Health)
    EPlayerHealthStatus GetHealthStatus() const;

	// Is this person dead? (not unconscious)
	UFUNCTION(BlueprintPure, Category = Health)
    bool IsDeadNotUnconscious() const;
	
	// Is this person unconscious? (not dead)
	UFUNCTION(BlueprintPure, Category = Health)
    virtual bool IsUnconsciousNotDead() const;

	float TimeDeadOrUnconcious = 0.0f;

	float TimeIncapacitated = 0.0f;

	//DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBeginKill, class AReadyOrNotCharacter*, Killer);
	//FOnBeginKill OnBeginKilled;

	UFUNCTION(Exec, BlueprintCallable, Category = "Console Command")
    void Kill();
	
	UFUNCTION(Exec, BlueprintCallable, Category = "Console Command")
    void Incapacitate();

	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation)
			void Server_Incapacitate();
	virtual void Server_Incapacitate_Implementation();
	virtual bool Server_Incapacitate_Validate() { return true; }
	
	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation)
			void Server_Kill();
	virtual void Server_Kill_Implementation();
	virtual bool Server_Kill_Validate() { return true; }

	UFUNCTION()
	virtual void OnKilled(AReadyOrNotCharacter* InstigatorCharacter);
	
	UFUNCTION()
	virtual void OnIncapacitated(AReadyOrNotCharacter* InstigatorCharacter);
	
	// Set by OnExplosiveVestDetonation
	bool bIsInBitsAndPieces = false;

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnExplosiveVestDetonation();

	/* this characters controller rotation replicated as the players controller isn't valid for everyone */
	UPROPERTY(ReplicatedUsing = OnRep_ControlRotation, BlueprintReadOnly, Category = View)
	FRotator ReplicatedControlRotation;

	UFUNCTION()
	void OnRep_ControlRotation();

	virtual void PossessedBy(AController* NewController) override;

	UFUNCTION(BlueprintPure)
	bool IsPlayingRootMotionFromMontage() const;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCharacterKilled, AReadyOrNotCharacter*, InstigatorCharacter, AReadyOrNotCharacter*, KilledCharacter);
	UPROPERTY(BlueprintAssignable, Category = Events)
	FOnCharacterKilled OnCharacterKilled;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCharacterIncapacitated, AReadyOrNotCharacter*, IncapacitatedCharacter, AReadyOrNotCharacter*, InstigatorCharacter);
	UPROPERTY(BlueprintAssignable, Category = Events)
	FOnCharacterIncapacitated OnCharacterIncapacitated;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnWeaponFire, AReadyOrNotCharacter*, Character, ABaseMagazineWeapon*, Weapon, FVector, FireDirection);
	UPROPERTY(BlueprintAssignable, Category = Events)
	FOnWeaponFire OnWeaponFire;
	
	UPROPERTY(BlueprintAssignable, Category = Events)
	FOnWeaponFire OnWeaponDryFire;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDowned, AReadyOrNotCharacter*, DownedCharacter, AReadyOrNotCharacter*, InstigatorCharacter);
	UPROPERTY(BlueprintAssignable, Category = Events)
    FOnDowned OnPlayerDowned;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnPointDamage, float, Damage, AActor*, Causer, ACharacter*, InstigatorCharacter, ACharacter*, HitCharacter, class UBulletDamageType*, DamageEvent);
	UPROPERTY(BlueprintAssignable, Category = Events)
	FOnPointDamage OnPointDamageReceived;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnStunDamage, float, Damage, AActor*, Causer, ACharacter*, InstigatorCharacter, ACharacter*, HitCharacter, class UStunDamage*, DamageEvent);
	UPROPERTY(BlueprintAssignable, Category = Events)
	FOnStunDamage OnStunDamageReceived;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFreed, ACharacter*, Freed, ACharacter*, Freer);
	UPROPERTY(BlueprintAssignable, Category = Events)
	FOnFreed OnPlayerFreed;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerHitEvent, float, Damage, FName, HitBone);

	/* Called on the server when the player has been hit */
	UPROPERTY(BlueprintAssignable, Category = Gameplay)
	FOnPlayerHitEvent OnPlayerHit;

	/* Called on the server when the players armor has been hit*/
	UPROPERTY(BlueprintAssignable, Category = Gameplay)
	FOnPlayerHitEvent OnPlayerArmorHit;

	UPROPERTY(BlueprintReadOnly)
	ABaseItem* ThrownItem = nullptr;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGetupComplete);
	UPROPERTY(BlueprintAssignable)
	FOnGetupComplete OnGetupComplete;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemThrownNotifyEvent, ABaseItem*, InThrownItem);
	UPROPERTY(BlueprintAssignable)
	FOnItemThrownNotifyEvent OnItemThrown_FromAnimNotify;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDoorLockPickNotifyEvent);
	UPROPERTY(BlueprintAssignable)
	FOnDoorLockPickNotifyEvent OnDoorLockPickBegin_FromAnimNotify;
	UPROPERTY(BlueprintAssignable)
	FOnDoorLockPickNotifyEvent OnDoorLockPickEnd_FromAnimNotify;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDoorCheckedNotifyEvent);
	UPROPERTY(BlueprintAssignable)
	FOnDoorCheckedNotifyEvent OnDoorChecked_FromAnimNotify;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnC2NotifyEvent);
	UPROPERTY(BlueprintAssignable)
	FOnC2NotifyEvent OnC2Placed_FromAnimNotify;
	UPROPERTY(BlueprintAssignable)
	FOnC2NotifyEvent OnC2Detonate_FromAnimNotify;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDisarmTrapNotifyEvent);
	UPROPERTY(BlueprintAssignable)
	FOnDisarmTrapNotifyEvent OnTrapDisarmBegin_FromAnimNotify;
	UPROPERTY(BlueprintAssignable)
	FOnDisarmTrapNotifyEvent OnTrapDisarmEnd_FromAnimNotify;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMirrorDoorNotifyEvent);
	UPROPERTY(BlueprintAssignable)
	FOnDisarmTrapNotifyEvent OnMirrorDoorStarted_FromAnimNotify;
	UPROPERTY(BlueprintAssignable)
	FOnDisarmTrapNotifyEvent OnMirrorDoorFinished_FromAnimNotify;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDoorWedgePlacedNotifyEvent);
	UPROPERTY(BlueprintAssignable)
	FOnDoorWedgePlacedNotifyEvent OnStartDoorWedgePlacement_FromAnimNotify;
	UPROPERTY(BlueprintAssignable)
	FOnDoorWedgePlacedNotifyEvent OnEndDoorWedgePlacement_FromAnimNotify;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPickupItemNotifyEvent);
	FOnPickupItemNotifyEvent OnPickupItem_FromAnimNotify;
	
	virtual bool ShouldImprintIconOnHUD() const;

	UFUNCTION(NetMulticast, Reliable)
			void Multicast_TakeDamage(float Damage, FDamageEvent const& DamageEvent, AReadyOrNotCharacter* InstigatorCharacter, AActor* DamageCauser);
	virtual void Multicast_TakeDamage_Implementation(float Damage, FDamageEvent const& DamageEvent, AReadyOrNotCharacter* InstigatorCharacter, AActor* DamageCauser);

	virtual void RespondToBleedOutDamage();

	FPointDamageEvent* LastDeathPointDamageEvent = nullptr;
	FRadialDamageEvent* LastDeathRadialDamageEvent = nullptr;
	
	FName LastHitBoneName;
	bool bHasRunDeathLogic = false;
	bool bHasRunIncapLogic = false;
	bool bHasPlayedDownedLogic = false;
	bool bHasBroadcastIncapEvent = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Damage")
	FCharacterDamageEvent LastDamageEvent;

	bool bBleedoutDeath = false;
	bool bNonLethalDeath = false;
	
	UFUNCTION()
	void OnHealthDepleted();
	
	UFUNCTION(NetMulticast, Reliable)
			void Multicast_OnKilled(FName LastBoneHit, AActor* DamageCauser);
	virtual void Multicast_OnKilled_Implementation(FName LastBoneHit, AActor* DamageCauser);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnIncapacitated(FName LastBone);

	// If true, we should allow bullets to impact us on death
	UPROPERTY(BlueprintReadOnly, Category = "Physics")
	bool bBulletForceTransferred = true;

	// Minimum amount of force needed to trigger body splat sound
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Physics")
	float MinimumBodyFallImpulse = 750.0f;

	// Maximum amount of body splat sounds to play per body
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Physics")
	int32 MaxRagdollSounds = 3;

	UPROPERTY(BlueprintReadOnly, Category = "Physics")
	int32 RagdollSoundsPlayed = 0;

	// Sound effect that plays when we have a body fall sound
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Physics")
	UFMODEvent* BodyFallEvent;

	FFMODEventInstance BodyFallInstance;

	UPROPERTY()
	USoundSource* BodyFallSoundSource;

	UFUNCTION()
	virtual void OnDeadHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	
	UFUNCTION()
	virtual void OnMeshHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	void OnCapsuleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	//

	UPROPERTY(BlueprintReadOnly, Category = "Ragdoll")
	bool bCapsuleCollisionRagdolled = false;

	UPROPERTY(BlueprintReadOnly, Category = "Ragdoll")
	bool bCapsuleFloorAngleRagdolled = false;

	/* the amount of collision bumps allowed before triggering early ragdoll */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ragdoll")
	int CapsuleCollisionRagdollTriggerThreshold;

	/* the capsule floor angle threshold to trigger early ragdoll */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ragdoll")
	float CapsuleFloorAngleRagdollTriggerThreshold;

	/* the time incremented once floor angle is passed to trigger early ragdoll */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ragdoll")
	float CapsuleFloorAngleRagdollDelayThreshold;

	virtual void UpdateCapsuleFloorAngleRagdollTrigger(float DeltaSeconds);

	/* this is based on the notify code and produces the best anim to ragdoll transitions, better then EnableRagdoll func */
	UPROPERTY(BlueprintReadOnly, Category = "Ragdoll")
	bool bBlendingAnim2Ragdoll;

	float CurrentAnim2RagdollEndTime;
	float CurrentAnim2RagdollTime;
	float CurrentAnim2RagdollBlendValue;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ragdoll")
	float Anim2RagdollPelvisWakeUpTime;

	UFUNCTION(BlueprintCallable, Category = "Ragdoll")
	virtual void RequestAnim2RagdollBlend(float Duration);

	virtual void UpdateAnim2RagdollBlend(float DeltaSeconds);

	int CapsuleCollisionCounter;
	float CapsuleFloorAngleDelayCounter;

	//

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_ApplyPointDamage(AActor* DamagedActor, float BaseDamage, FVector const& HitFromDirection, FHitResult const& HitInfo, AController* EventInstigator, AActor* DamageCauser, TSubclassOf<UDamageType> DamageTypeClass);
	void Server_ApplyPointDamage_Implementation(AActor* DamagedActor, float BaseDamage, FVector const& HitFromDirection, FHitResult const& HitInfo, AController* EventInstigator, AActor* DamageCauser, TSubclassOf<UDamageType> DamageTypeClass);
	bool Server_ApplyPointDamage_Validate(AActor* DamagedActor, float BaseDamage, FVector const& HitFromDirection, FHitResult const& HitInfo, AController* EventInstigator, AActor* DamageCauser, TSubclassOf<UDamageType> DamageTypeClass) { return true; }

	bool bShouldSpawnBloodPool = true;

	UPROPERTY()
	USoundSource* VoiceSoundSource;

	FString LastVoiceLinePlayed = "";

protected:
	void InitCollisionPreset();
	
public:
	virtual bool IsComponentRelevantForNavigation(UActorComponent* Component) const override;
	
	void SetCollisionPreset(const FCharacterCollisionTemplate& InCollisionTemplate);
	void SetPreviousCollisionPreset();
	
	FCharacterCollisionTemplate DefaultCollision;
	FCharacterCollisionTemplate RagdollCollision;
	FCharacterCollisionTemplate PairedInteractionCollision;
	
	const FCharacterCollisionTemplate* PreviousCollisionTemplate = nullptr;

	UFUNCTION(BlueprintPure)
	bool IsInRagdoll() const;
	
	UFUNCTION(BlueprintPure)
	bool IsRagdollBlending() const;

	//UFUNCTION(BlueprintPure, Category = "Knockout")
	//virtual bool IsKnockedOut() const;

	UFUNCTION()
	void OnRagdollStart(URagdollComponent* InRagdollComponent);
	
	UFUNCTION()
	void OnRagdollBlendStop(URagdollComponent* InRagdollComponent);

	UFUNCTION()
	void OnRagdollPhysBodyHit(URagdollComponent* InRagdollComponent, FVector Impulse, const FHitResult& Hit);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_EnableRagdoll(float Duration = 0.0f);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_DisableRagdoll();
	void Multicast_DisableRagdoll_Implementation();

	// A duration of 0.0 = indefinite
	UFUNCTION(BlueprintCallable)
	void EnableRagdoll(float Duration = 0.0f);
	
	UFUNCTION(BlueprintCallable)
	void DisableRagdoll();

	UFUNCTION(BlueprintCallable)
	void ResetPhysicsAsset();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SavePoseSnapshot(const FName& SnapshotName);
	void Multicast_SavePoseSnapshot_Implementation(const FName& SnapshotName);
	
	virtual bool GetBackupAfterRagdoll();
	virtual bool GetBackupAfterRagdollArrest();
	
	virtual void OnRagdollDurationComplete();

	//void GetBackupAfterRagdoll_Internal(TFunction<void()> Callback);
	void GetBackupAfterRagdoll_Internal();

	UFUNCTION(BlueprintCallable, NetMulticast, Reliable)
			void Multicast_EnableRagdollBlendIn();
	virtual void Multicast_EnableRagdollBlendIn_Implementation();

	FTimerHandle TH_GetBackupAfterRagdoll;

	UPROPERTY(Replicated)
	bool bBlendInPhysics = false;
	UPROPERTY(Replicated)
	bool bStartBlendInIncapacitation = false;
	UPROPERTY(Replicated)
	bool bBlendInIncapacitation = false;
	UPROPERTY(Replicated)
	float IncapacitationBlendTime = 0.0f;
	UPROPERTY(Replicated)
	float IncapacitationBlendOutTime = 0.0f;
	UPROPERTY(Replicated, BlueprintReadOnly)
	UAnimSequence* IncapacitationLoopAnim = nullptr;

	void DisablePhysicalAnimation();
	
	UPROPERTY(Replicated)
	float BlendInterpAmount = 0.0f;
	
	// Set from anim notify
	float BlendInterpDuration = 1.0f;

	float FinalBlendIn = 0.0f;

	float TimeDead = 0.0f;

	//UPROPERTY(BlueprintReadOnly, Replicated, Category = "Gameplay")
	//float TimeDowned = 0.0f;

	UPROPERTY(EditAnywhere, Category = Physics)
	float FinalBlendInTime = 1.0f;

	float FadeOutDamping = 0.0f;

	UPROPERTY(EditAnywhere, Category = Physics)
	float BlendInAfterStartOfAnim = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Character")
	uint8 bIsPreviewCharacter : 1;

	uint8 bPreviousShowPlayerNames : 1;
	float TimeSinceLastPlayerMarkerUpdate = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category = "Gameplay")
	AReadyOrNotCharacter* ArrestedBy = nullptr;
	
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Gameplay")
	AReadyOrNotCharacter* KilledBy = nullptr;
	
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Gameplay")
	AReadyOrNotCharacter* IncapacitatedBy = nullptr;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Gameplay")
	ECharacterDeathReason DeathReason = ECharacterDeathReason::None;
	
	void DestroyAnimInstance();

	class AReadyOrNotPlayerController* GetRONPlayerController();
	class AReadyOrNotPlayerState* GetRONPlayerState();

	UFUNCTION()
	virtual void OnEquippedWeaponFire(ABaseMagazineWeapon* Weapon, bool bServer);
	
	UFUNCTION()
	virtual void OnEquippedWeaponDryFire(ABaseMagazineWeapon* Weapon, bool bServer);
	
	UFUNCTION()
	virtual void OnEquippedWeaponMagCheck(ABaseMagazineWeapon* Weapon);
	
	virtual void PlayWeaponFireAnimation(ABaseMagazineWeapon* Weapon, bool bIsAiming, bool bOnlyTP = false);
	virtual void PlayWeaponDryFireAnimation(ABaseMagazineWeapon* Weapon, bool bIsAiming, bool bOnlyTP = false);

	/*
	 * Delegates
	 */
	virtual void BindAllDelegates();
	virtual void UnbindAllDelegates();
	
	UFUNCTION()
	virtual void OnActorSpawned(AActor* Actor);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnCharacterTakeDamage, AReadyOrNotCharacter*, InstigatorCharacter, AReadyOrNotCharacter*, DamagedCharacter, AActor*, DamageCauser, float, Damage, float, HealthRemaining);
	UPROPERTY(BlueprintAssignable, Category = Events)
	FOnCharacterTakeDamage OnCharacterTakeDamage;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStunnedPlaySound, EStunType, StunType, bool, bIsImmune);
	UPROPERTY(BlueprintAssignable)
	FOnStunnedPlaySound OnAIStunnedPlaySound;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnStunned, AReadyOrNotCharacter*, StunnedCharacter, float, Duration, EStunType, StunType, AActor*, DamageCauser);
	UPROPERTY(BlueprintAssignable)
	FOnStunned OnStunnedEvent;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStunnedEnded, EStunType, StunType);
	UPROPERTY(BlueprintAssignable)
	FOnStunnedEnded OnStunnedEndedEvent;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMeleeHitTaken, AReadyOrNotCharacter*, InstigatorCharacter);
	UPROPERTY(BlueprintAssignable)
	FOnMeleeHitTaken OnMeleeHitTaken;
	
	/*
	* Bone Definition Code
	*/

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly ,Category = "Player | Details | Bones")
	TArray<FName> HeadBones;

	/* different bone strucutres that make up the parts of a body so we can know if a arm or leg has been hit without having to manually define each bone everywhere */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player | Details | Bones")
	TArray<FName> UpperBody;

	TArray<FName> Torso;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player | Details | Bones")
	TArray<FName> LowerBody;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player | Details | Bones")
	TArray<FName> R_Leg;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player | Details | Bones")
	TArray<FName> L_Leg;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player | Details | Bones")
	TArray<FName> L_Foot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player | Details | Bones")
	TArray<FName> R_Foot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player | Details | Bones")
	TArray<FName> L_Arm;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player | Details | Bones")
	TArray<FName> R_Arm;
		
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player | Details | Bones")
	TArray<FName> L_Hand;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player | Details | Bones")
	TArray<FName> R_Hand;

	/*
	 * Low Ready Related Code
	 */
public:
	FORCEINLINE bool IsLowReady() const { return bLowReadyPointUp || bLowReadyPointDown; }
	FORCEINLINE bool IsLowReadyPointDown() { return bLowReadyPointDown; }
	FORCEINLINE bool IsLowReadyPointUp() { return bLowReadyPointUp; }
	virtual void SetLowReady(bool bUp, bool bLowReady);
	virtual bool ShouldEnableDepthFade() { return false; }
	
protected:
	virtual void DoLowReadyTrace() {}
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Replicated, Category = LowReady)
	bool bLowReadyPointUp = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Replicated, Category = LowReady)
	bool bLowReadyPointDown = false;

	UPROPERTY(BlueprintReadOnly, Category = LowReady)
	float LowReadyDistance = BIG_NUMBER;

	FTimerHandle DoLowReadyTrace_Handle;
	
	UPROPERTY()
	TArray<UCapsuleComponent*> LowReadyIgnoredCapsules;

public:
	UFUNCTION(BlueprintPure, Category = Networking)
	virtual bool IsLocalPlayer() const;

	FORCEINLINE bool IsCrouching() const { return bIsCrouched || bIsCrouching; }

	// TODO: Delete once a crouch moveset for AI works
	// Hack for AI to stay crouched whilst in cover
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Movement")
	uint8 bIsCrouching : 1;

	uint8 bDisableTurnInPlace : 1;
	
	/*
	 * Team Related Code
	 */

	UFUNCTION(BlueprintPure, Category = Team)
	bool IsOnSWATTeam() const;

	/* trys to get our team from player state or returns default team */
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = Team)
	virtual ETeamType GetTeam() const;

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = Team)
	bool IsCivilian() const;
	
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = Team)
	bool IsSuspect() const;

	UFUNCTION(BlueprintPure, Category = "Teams")
	static bool IsOnSameTeam(AReadyOrNotCharacter* A, AReadyOrNotCharacter* B);

	void SetSquadPosition(ESquadPosition NewSquadPosition) { SquadPosition = NewSquadPosition;}
	ESquadPosition GetSquadPosition() const { return SquadPosition; }
	bool IsSquadPosition(TArray<ESquadPosition> InSquadPositions) { return InSquadPositions.Contains(SquadPosition); }

	void SetDefaultTeam(ETeamType InTeamType) { DefaultTeam = InTeamType; }

	// The amount the player is currently leaning
	UPROPERTY(BlueprintReadOnly, Replicated, Category = Gameplay)
	float QuickLeanAmount = 0.0f;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = Gameplay)
	float QuickLeanIntensity = 0.5f;
	
	UPROPERTY(BlueprintReadOnly, Replicated, Category = Gameplay)
	float QuickLeanInterpSpeed = 8.0f;

	// lean left and right
	UPROPERTY(BlueprintReadOnly, Replicated, Category = Gameplay)
	float FreeLeanX = 0.0f;
	
	// lean up and down
	UPROPERTY(BlueprintReadOnly, Replicated, Category = Gameplay)
	float FreeLeanZ = 0.0f;
	
	/* Toggles free lean on, movement controls should now adjust the lean */
	void DoFreeLean();

	void EndFreeLean();
	
	UFUNCTION(BlueprintCallable)
	void ToggleFreeLean();

	UPROPERTY(BlueprintReadOnly, Replicated, Category = Gameplay)
	bool bFreeLeaning;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = Gameplay)
	bool bIsLeaning;
	
	UPROPERTY(BlueprintReadOnly, Category = Lean)
	bool bLeaningLeft = false;

	UPROPERTY(BlueprintReadOnly, Category = Lean)
	bool bLeaningRight = false;

	UPROPERTY(BlueprintReadOnly, Category = Lean)
	bool bLeaningUp = false;

	UPROPERTY(BlueprintReadOnly, Category = Lean)
	bool bLeaningDown = false;
	
protected:
	/**
	* Called via input to turn at a given rate.
	* @param Val	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	*/
	UFUNCTION(BlueprintCallable, Category = Input)
	void Lean(float Val);

	/* leans up or down, called from input */
	UFUNCTION(BlueprintCallable, Category = Input)
	void LeanUp(float Val);

	/* leans left or right, called from input */
	UFUNCTION(BlueprintCallable, Category = Input)
	void LeanRight(float Val);

	void ToggleLean(float Val);

	void StopLean();
	
	// Toggle Lean Left
	UFUNCTION(BlueprintCallable, Category = Input)
	void ToggleLeanLeft(bool bADSIsActive = false);

	UPROPERTY(BlueprintReadOnly, Category = Input)
	bool bLeanLeftToggle = false;

	// Toggle Lean Right
	UFUNCTION(BlueprintCallable, Category = Input)
	void ToggleLeanRight(bool bADSIsActive = false);

	UPROPERTY(BlueprintReadOnly, Category = Input)
	bool bLeanRightToggle = false;
	
	UPROPERTY()
	UFMODAudioComponent* LeanAudioComponent;

	UPROPERTY(EditDefaultsOnly, Category = Lean)
	UFMODEvent* LeanAudioEvent;

	UPROPERTY(BlueprintReadOnly, Category = Lean)
	float LeanMovementValue;

	UFUNCTION()
	void CalculateLeanMovement(float DeltaTime);

	UFUNCTION(BlueprintImplementableEvent, Category = Lean)
	void OnLeanStart();

	UFUNCTION(BlueprintImplementableEvent, Category = Lean)
	void OnLeanEnd();

	/* get the amount that we can possibly lean in a given direction */
	UFUNCTION()
	float GetLeanAmount(FVector Component, float& InOutPendingVal, float MaxValue);

	UPROPERTY(BlueprintReadOnly, Category = Lean)
	FVector LeanPos_CurrentFrame;

	UPROPERTY(BlueprintReadOnly, Category = Lean)
	FVector LeanPos_LastFrame;

	UFUNCTION(Server, Unreliable, WithValidation)
			void Server_UpdateLean(float QuickLean, float newFreeLeanY, float NewFreeLeanZ);
	virtual void Server_UpdateLean_Implementation(float QuickLean, float newFreeLeanY, float NewFreeLeanZ);
	virtual bool Server_UpdateLean_Validate(float QuickLean, float newFreeLeanY, float NewFreeLeanZ) { return true; }
	
	UPROPERTY(BlueprintReadWrite, Category = Gameplay)
	ESquadPosition SquadPosition;

	UPROPERTY(Replicated)
	ETeamType DefaultTeam = ETeamType::TT_NONE;

	/*
	 * Melee Code (Do trace, get target, apply damage.)
	 * Works the same for both players & AI
	 */

public:
	virtual void StartMelee();

	UFUNCTION(BlueprintPure, Category = Damage)
	virtual bool CanMelee() const;
	
	// Called from notify
	UFUNCTION(Server, Reliable, WithValidation)
			void Server_DoMelee();
	virtual void Server_DoMelee_Implementation() { DoMelee(false); }
	virtual bool Server_DoMelee_Validate() { return true; }

	virtual void OnMeleeTrace(FHitResult HitResult, bool bLocal);
	
	virtual void DoMelee(bool bLocal = true);
	
	UFUNCTION(Client, Unreliable)
	void Client_PlayMeleeImpactEffects();
	void Client_PlayMeleeImpactEffects_Implementation();
	
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayMeleeImpactEffects(UFMODEvent* ImpactSound, UParticleSystem* ImpactParticle);
	void Multicast_PlayMeleeImpactEffects_Implementation(UFMODEvent* ImpactSound, UParticleSystem* ImpactParticle) { PlayMeleeImpactEffects(ImpactSound, ImpactParticle); }

	void PlayMeleeImpactEffects(UFMODEvent* ImpactSound, UParticleSystem* ImpactParticle);

	void SetMeleeDamage(float InNewMeleeDamage) { MeleeDamage = InNewMeleeDamage; }
	void SetMeleeRange(float InNewMeleeRange) { MeleeRange = InNewMeleeRange; }

protected:
	virtual void OnMelee_Implementation(AReadyOrNotCharacter* Attacker, FHitResult Hit) override;
	virtual UFMODEvent* GetMeleeImpactSound_Implementation() const override;
	virtual UParticleSystem* GetMeleeImpactParticle_Implementation() const override;
	virtual bool ShouldPlayMeleeEffectsLocally_Implementation() const override;

	// heard by the player that got hit
	UPROPERTY(EditAnywhere)
	UFMODEvent* FPMeleeImpactFMODEvent;

	// heard by all other players
	UPROPERTY(EditAnywhere)
	UFMODEvent* TPMeleeImpactFMODEvent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melee")
	UParticleSystem* MeleeImpactParticle = nullptr;

	// The camera shake that melee inflicts upon the other person
	UPROPERTY(EditAnywhere, Category = Melee)
	TSubclassOf<ULegacyCameraShake> MeleeCameraShake;
	
	// How far we can hit someone with melee
	UPROPERTY(EditAnywhere, Category = Melee)
	float MeleeRange = 85.0f;

	// How much damage melee does
	UPROPERTY(EditAnywhere, Category = Melee)
	float MeleeDamage = 5.0f;

	// What kind of damage melee does
	UPROPERTY(EditAnywhere, Category = Melee)
	TSoftClassPtr<UDamageType> MeleeDamageType;

	/*
	 * Screen Shake Code
	 */

public:
	UFUNCTION(BlueprintCallable, Reliable, Client, Category = Camera)
	virtual void Client_PlayScreenShake(TSubclassOf<ULegacyCameraShake> CameraShake);

	/* 
	 * Arrest / Surrender functions
	 */
	
	UFUNCTION()
	virtual void OnRep_Surrendered() {}

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnArrest, AReadyOrNotCharacter*, ArrestedCharacter, AReadyOrNotCharacter*, InstigatorCharacter);
	UPROPERTY(BlueprintAssignable, Category = Events)
	FOnArrest OnPlayerArrested;

	UPROPERTY(BlueprintAssignable, Category = Events)
	FOnArrest OnPlayerArrestStart;

	UPROPERTY(BlueprintAssignable, Category = Events)
	FOnArrest OnPlayerArrestedCanceled;

	UPROPERTY()
	AReadyOrNotCharacter* PendingAutoReport = nullptr;
	
	// returns if the person can be arrested by the current team (and is valid)
	UFUNCTION(BlueprintPure, Category = Arrest)
	virtual bool CanArrest() const;

	UFUNCTION(BlueprintPure, Category = Arrest)
	virtual bool IsArrested() const;

	UFUNCTION(BlueprintPure, Category = Arrest)
	virtual bool IsArrestedAndDead() const;
	
	UFUNCTION(BlueprintPure, Category = Arrest)
	virtual bool IsArrestedAndIncapacitated() const;

	UFUNCTION(BlueprintPure, Category = Arrest)
	virtual bool IsSurrendered() const { return bSurrendered; }
	
	UFUNCTION(BlueprintPure, Category = Arrest)
	FORCEINLINE bool IsSurrenderComplete() const { return bSurrenderComplete; }

	UFUNCTION(BlueprintPure)
	FORCEINLINE bool IsSurrenderedFor(const float Seconds) const { return SurrenderedTime >= Seconds; }
	
	UFUNCTION(BlueprintPure, Category = "Arrest | Surrendered")
	FORCEINLINE bool IsArrestedOrSurrendered() const { return IsSurrendered() || IsArrested(); }

	UFUNCTION(BlueprintPure)
	bool IsStartling() const;

	UFUNCTION(BlueprintPure)
	bool IsBeingCarried() const;
	
	UFUNCTION(BlueprintPure)
	bool IsBeingArrested() const;
	void SetIsBeingArrested(bool bNewIsBeingArrested) { bIsBeingArrested = bNewIsBeingArrested; }
	bool IsDoingArrest();
	
	void DoArrestWithZipcuffs(AReadyOrNotCharacter* Target);
	
	UFUNCTION(BlueprintCallable, Category = Damage)
	virtual void Arrest(AReadyOrNotCharacter* PlayerMakingArrest);

	UFUNCTION(BlueprintCallable, Category = Damage)
	void CancelArrest(AReadyOrNotCharacter* PlayerMakingArrest);

	UFUNCTION(BlueprintCallable, Category = Damage)
	virtual void ArrestComplete(AReadyOrNotCharacter* PlayerMakingArrest, class AZipcuffs* Zipcuffs);

	UPROPERTY(Replicated, BlueprintReadWrite)
	FCarryArrestedAnimState Rep_CarryArrestedAnimState;

	UFUNCTION(BlueprintPure)
	bool IsCarried() const;
	UFUNCTION(BlueprintPure)
	bool IsCarrying() const;
	UFUNCTION(BlueprintPure)
	bool IsPlayingCarryAnims() const;
	UFUNCTION(BlueprintPure)
	bool IsDropping() const;
	UFUNCTION(BlueprintPure)
	bool IsBeingThrown() const;
	
	UFUNCTION(BlueprintPure)
	virtual bool IsGettingUp() const;
	
	UPROPERTY(Replicated)
	bool bSurrendered = false;
	UPROPERTY(Replicated)
	bool bSurrenderComplete = false;
	
	bool bSurrenderingWithItem = false;
	
protected:
	float LastZVelocityInAir = 0.0f;

	float AirTime = 0.0f;
	float AirTimeBeforeTakingDamage = 0.5f;
	
	float SurrenderedTime = 0.0f;
	float TimeNotSurrendered = 0.0f;
	
	UPROPERTY(Replicated)
	uint8 bOrderedToRotateForArrest : 1;
	
	UPROPERTY()
	class APlacedZipcuffs* PlacedZipcuffs;

	// The zipcuffs that spawn on this character.
	UPROPERTY(EditAnywhere, Category = Zipcuffs)
    TSubclassOf<APlacedZipcuffs> SpawnedZipcuffsClass;

	UPROPERTY(EditAnywhere, Category = Zipcuffs)
	TSubclassOf<APlacedZipcuffs> SpawnedFPZipcuffsClass;

	// Which bone on the person to spawn the zipcuffs
	UPROPERTY(EditAnywhere, Category = Zipcuffs)
    FName ZipcuffBone;

	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bIsBeingCarried = false;
	
	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bCarryingDead = false;
	
	UPROPERTY(Replicated, BlueprintReadOnly)
	AReadyOrNotCharacter* CarriedByCharacter = nullptr; // the person who is carrying us
	
	UPROPERTY(BlueprintReadOnly)
	AReadyOrNotCharacter* ThrownByCharacter = nullptr; // the person who threw us
	
	UPROPERTY(Replicated, BlueprintReadOnly)
	AReadyOrNotCharacter* PendingCarryCharacter = nullptr; // the person we are confirming to carry
	
	UPROPERTY(ReplicatedUsing=OnRep_CurrentCarryCharacterChanged, BlueprintReadOnly)
	AReadyOrNotCharacter* CurrentCarryCharacter = nullptr; // the person we are currently carrying
	
	UPROPERTY()
	class ASkeletalMeshActor* FakeCarryCharacterMesh = nullptr;

	UFUNCTION()
	void OnRep_CurrentCarryCharacterChanged();
	
	UPROPERTY(Replicated, BlueprintReadOnly)
	float CurrentCarryConfirmTime = 0.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Carry Character")
	UAnimSequence* CarryMasterIdleLoop = nullptr;
	
	UPROPERTY(EditDefaultsOnly, Category = "Carry Character")
	UAnimSequence* CarrySlaveIdleLoop = nullptr;
	
	UPROPERTY(EditDefaultsOnly, Category = "Carry Character")
	float MaxCarryConfirmTime = 0.5f;

	virtual void ResetThrownByCharacter();
	virtual void ReactToCarryThrow();
	
	UPROPERTY(EditDefaultsOnly, Category = "Carry Character")
	float MaxRagdollArrestConfirmTime = 0.5f;

	UPROPERTY(Replicated, BlueprintReadOnly)
	float CurrentRagdollArrestConfirmTime = 0.0f;

	UPROPERTY(Replicated, BlueprintReadOnly)
	AReadyOrNotCharacter* PendingRagdollArrestCharacter = nullptr; // the person we are confirming to ragdoll arrest

	UPROPERTY(ReplicatedUsing=OnRep_CurrentRagdollArrestCharacterChanged, BlueprintReadOnly)
	AReadyOrNotCharacter* CurrentRagdollArrestCharacter = nullptr; // the person we are currently ragdoll arresting

	UFUNCTION()
	void OnRep_CurrentRagdollArrestCharacterChanged();
	
public:
	UPROPERTY()
	AReadyOrNotCharacter* LastCharacterMakingArrest = nullptr; // the person who is arresting us

	UPROPERTY()
	AReadyOrNotCharacter* CurrentlyArresting = nullptr; // the person we are currently arresting
	
	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bIsBeingArrested = false;
	
	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bArrestComplete = false;

	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bArrestedAsRagdoll = false;
	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bArrestedAsRagdoll_Flipped = false;
	
	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bIsPairedInteractionPlaying = false;

	UPROPERTY(BlueprintReadOnly)
	bool bNoTeamDamage = false;
	
	UPROPERTY(BlueprintReadOnly)
	bool bDisableInteraction = false;

	/* used to move the animgraph into a primed state when primary use is selected
	aka pull grenade pins and hold */
	UPROPERTY(BlueprintReadWrite, Replicated, Category = Gameplay)
	bool bPrimed = false;

	/* next grenade state is overarm throw if false, then next grenade state will be underarm throw */
	UPROPERTY(BlueprintReadWrite, Replicated, Category = Gameplay)
	bool bOverarmThrow;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnQuickthrowStart, AReadyOrNotCharacter*, Character, ABaseItem*, LastItemBeforeQuickThrow, ABaseItem*, QuickThrowGrenade);
	UPROPERTY(BlueprintAssignable, Category = "QuickThrow")
	FOnQuickthrowStart OnQuickThrowStart;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQuickthrowEnd, ABaseItem*, QuickThrowGrenade);
	UPROPERTY(BlueprintAssignable, Category = "QuickThrow")
	FOnQuickthrowEnd OnQuickThrowEnd;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCarryingChanged);
	UPROPERTY(BlueprintAssignable)
	FOnCarryingChanged OnCarryingChanged;

	UFUNCTION(BlueprintPure)
	virtual EAnimWeaponType GetCurrentWeaponAnimType() const;

	UFUNCTION(BlueprintCallable)
	virtual UAnimMontage* PlayMontageFromTable(const FString& Animation);
	UFUNCTION(BlueprintCallable)
	virtual UAnimMontage* PlayMontageFromTableWithIndex(const FString& Animation, int32 Index);

	bool bServerIsStrafing = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
	bool bIsStrafing;
	bool bWasWeaponRaised = false;
	

	UFUNCTION(BlueprintPure)
	virtual int32 GetMontageAnimCountFromTable(const FString& Animation) const;

	UFUNCTION(BlueprintPure, Category = Animation)
	virtual bool IsTableMontagePlaying(const FString& Animation) const;
	UFUNCTION(BlueprintPure)
	virtual bool IsAnyTableMontagePlaying(FString& OutMontage) const;
	virtual bool IsAnyTableMontagePlaying() const;
	UFUNCTION(BlueprintPure)
	virtual bool IsTableMontage(UAnimMontage* Montage) const;
	// use this as an optimization so we don't have to loop over the entire data table to find out if an animation has been played


	UFUNCTION(BlueprintCallable, Category = Animation)
    virtual bool IsTableMontagePlayingWithTimeRemaining(const FString& Animation, float& TimeRemaining) const;
	UFUNCTION(BlueprintCallable, Category = Animation)
    virtual bool IsMontagePlayingWithTimeRemaining(const UAnimMontage* Animation, float& TimeRemaining) const;
	UFUNCTION(BlueprintCallable, NetMulticast, Reliable)
	void StopTPMontageFromTable(const FString& Animation, float BlendoutTime = 0.25f);
	UFUNCTION(BlueprintCallable, Category = Animation)
	virtual void StopTPAnimMontage(class UAnimMontage* AnimMontage = nullptr);

	UFUNCTION(BlueprintPure)
	bool IsPlayingNonInterruptibleMontage(const FString& MontageNameTryingToBePlayed) const;
	UFUNCTION(BlueprintPure)
	virtual UAnimMontage* GetMontageFromTable(const FString& Animation) const;
	UFUNCTION(BlueprintPure)
	virtual UAnimMontage* GetMontageFromTableWithIndex(const FString& Animation, int32 Index) const;
	UFUNCTION(BlueprintPure)
	bool DoesMontageFromTableExist(const FString& Animation) const;
	// Networkable, if anim montage null will stop any playing montage
	UFUNCTION(BlueprintCallable)
	virtual void StopTPMontage(UAnimMontage* AnimMontage, float BlendoutTime = 0.25f);
	UFUNCTION(BlueprintPure, Category = Animation)
	FString GetLastTableMontagePlayed() const;
	UFUNCTION(BlueprintPure)
	TArray<FString> GetTableMontageQueue() const;

	UFUNCTION(BlueprintCallable)
	class APairedInteractionDriver* PlayPairedInteraction(class UInteractionsData* InteractionData, AActor* Driver, AActor* Slave, ABaseItem* OptionalItem = nullptr);

	UFUNCTION(BlueprintPure)
	FORCEINLINE bool IsLoadingTableMontage() const { return LastMontageTableStreamableHandle.IsValid() && LastMontageTableStreamableHandle->IsLoadingInProgress(); }

	UPROPERTY()
	TMap<FString, UAnimMontage*> PlayedTableMontageMap3P;
protected:
	UPROPERTY(BlueprintReadOnly)
	FString LastTableMontagePlayed;
	TSharedPtr<FStreamableHandle> LastMontageTableStreamableHandle;
	TArray<FString> TableMontageQueue;
	TMap<FString, float> TableMontageActiveCooldowns;
	FTimerHandle AnimationDelay_Handle;
	
public:

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PauseAllAnims(bool bPaused);
	virtual void Multicast_PauseAllAnims_Implementation(bool bPaused);
	
	UFUNCTION(BlueprintCallable, Category = Animation)
	virtual void Play3PMontage(UAnimMontage* NewMontage, float StartTime = 0.0f, float PlayRate = 1.0f);
	UFUNCTION(NetMulticast, Reliable)
	virtual void Play3PMontageDeferred(UAnimMontage* Montage, const FString& AnimationName);
	
	void PlayLocal3PMontage(UAnimMontage* NewMontage, float PlayRate = 1.0f);

	void PlayNonLocal3PMontage(UAnimMontage* NewMontage, float PlayRate = 1.0f);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayNonLocal3PMontage(UAnimMontage* NewMontage);


	UFUNCTION(Server, Reliable, WithValidation)
	void Server_PlayNonLocal3PMontage(UAnimMontage* NewMontage);

	UFUNCTION(Server, Unreliable, WithValidation, BlueprintCallable, Category = Animation)
	void Server_Play3PMontage(UAnimMontage* NewMontage, float StartTime = 0.0f, float PlayRate = 1.0f);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_Play3PMontage(UAnimMontage* NewMontage, float StartTime = 0.0f, float PlayRate = 1.0f);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_Stop3PMontage(UAnimMontage* Montage, float BlendoutTime);

	virtual bool CanPlayDeathAnimation() const;
	
	virtual bool PlayDeathAnimation();
	
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayDeathAnimation(UAnimMontage* Montage);
	virtual void Multicast_PlayDeathAnimation_Implementation(UAnimMontage* Montage);
	
	void StopDeathAnimation();

	virtual void OnBlendRagdollAnimFinished();
	
	virtual UReadyOrNotWeaponAnimData* GetCurrentWeaponAnimData() const;

	UFUNCTION(BlueprintPure, Category = Gameplay)
	bool Is3PMontagePlaying(const UAnimMontage* Montage) const;
	bool IsAny3PMontageActive() const;

	UFUNCTION(BlueprintPure)
	bool IsMontageSlotPlaying(FName SlotName) const;
	
	UFUNCTION(BlueprintPure)
	bool IsFullBodyMontagePlaying() const;
	UFUNCTION(BlueprintPure)
	bool IsUpperBodyMontagePlaying() const;

	/* 
	* 1p Animations (Overriden in PlayerCharacter)
	*/
	UFUNCTION(NetMulticast, Reliable)
	virtual void Play1PMontageDeferred(UAnimMontage* Montage, const FString& AnimationName);
	UFUNCTION(BlueprintCallable, Category = Animation)
	virtual void Play1PMontage(UAnimMontage* NewMontage, float PlayRate = 1.0f);
	virtual void Play1PMontage_NonClient(UAnimMontage* NewMontage, float PlayRate = 1.0f);
	UFUNCTION(NetMulticast, Reliable)
	virtual void Multicast_Stop1PMontage(UAnimMontage* Montage, float BlendoutTime);
	virtual void PlayLocal1PMontage(UAnimMontage* NewMontage, float PlayRate = 1.0f);
	UFUNCTION(Client, Reliable, BlueprintCallable, Category = Animation)
	virtual void Client_Play1PMontage(UAnimMontage* NewMontage, float PlayRate = 1.0f);
	/** Stop Animation Montage. If nullptr, it will stop what's currently active. The Blend Out Time is taken from the montage asset that is being stopped. **/
	UFUNCTION(BlueprintCallable, Category = Animation)
	virtual void StopFPAnimMontage(class UAnimMontage* AnimMontage = nullptr, float BlendoutTime = 0.0f);

	void SetCurrentFaceROM(class UPoseAsset* NewFaceRom) { CurrentFaceROM = NewFaceRom; }

	UFUNCTION()
	virtual void OnRep_MeshReplicated();
	
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_ChangeTPMesh(USkeletalMesh* Body, USkeletalMesh* Face);
	
	UFUNCTION(Exec)
	void UpdateOverridesFromCharacterLookOverrideDataTable(FString LookOverride);

	USkeletalMesh* GetTPMeshOverride(TSubclassOf<class ABaseArmour> InArmourClass, bool& bFound);
	void SetTPMeshOverrideMap(TMap<TSubclassOf<class ABaseArmour>, USkeletalMesh*> InArmorOverrideMapTP) { ArmorOverrideMapTP = InArmorOverrideMapTP; }
	TMap<TSubclassOf<class ABaseArmour>, USkeletalMesh*> GetTPMeshOverrideMap() { return ArmorOverrideMapTP; }

	class UPoseAsset* GetCurrentFaceROM() { return CurrentFaceROM; }
public:
	UPROPERTY(VisibleAnywhere)
	FCharacterLookOverride CharacterLookOverride;

	UFUNCTION(NetMulticast, Reliable)
	void ForceMeshUsingOverride(USkeletalMesh* InFPMesh, USkeletalMesh* InTPMesh, USkeletalMesh* InFaceMesh);


	bool HasCharacterLookOverrideStringSet() { return !Rep_CharacterLookOverride.IsEmpty(); }
protected:
	UPROPERTY(ReplicatedUsing=OnRep_CharacterLookOverride)
	FString Rep_CharacterLookOverride;
	
	UFUNCTION()
	void OnRep_CharacterLookOverride() { UpdateOverridesFromCharacterLookOverrideDataTable(Rep_CharacterLookOverride); }

	UPROPERTY(EditAnywhere, Category = "Armor Override")
	TMap<TSubclassOf<class ABaseArmour>, USkeletalMesh*> ArmorOverrideMapTP;
	
	protected:
	UPROPERTY(ReplicatedUsing = OnRep_MeshReplicated)
	USkeletalMesh* Rep_BodyMesh;

	UPROPERTY(ReplicatedUsing = OnRep_MeshReplicated)
	USkeletalMesh* Rep_FaceMesh;
	
	UPROPERTY(ReplicatedUsing = OnRep_MeshReplicated)
	USkeletalMesh* Rep_FPMesh;

	UPROPERTY()
	TMap<UAnimMontage*, float> AnimMontageCooldown;

	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Animation)
	UPoseAsset* CurrentFaceROM;

public:
	UFUNCTION(BlueprintPure)
	virtual bool IsAnimationBlocking() const { return false; }

	UPROPERTY(BlueprintReadOnly)
	float AnimationBlockingTime = 0.0f;

	/*
	 * Reportables
	 */

	UFUNCTION(BlueprintPure)
	FORCEINLINE bool HasBeenReported() const { return bHasBeenReported; }
	
	// IReportable implementation
	virtual bool CanReportNow_Implementation() override;
	virtual FString GetSpeechTypeForReport_Implementation() override;
	virtual void ReportToTOC_Implementation(class AReadyOrNotCharacter* Reporter, bool bPlayAnimation = true) override;
	
	void GetReportableFMODEvents(UFMODEvent*& OutDeadFMODEvent, UFMODEvent*& OutArrestedFMODEvent, UFMODEvent*& OutGeneralFMODEvent);
	void SetSpeakCoolDown(float NewSpeakCooldown) { SpeakCooldown = NewSpeakCooldown; }
	UFUNCTION(BlueprintPure)
	FORCEINLINE float GetSpeakCooldown() const { return SpeakCooldown; }
	void SetSpeechTimer(FTimerHandle TimerHandle) { TH_OnSpeechFinished = TimerHandle; }
	UFUNCTION(BlueprintPure)
	FORCEINLINE bool IsSpeechTimerActive() const { return GetWorld()->GetTimerManager().IsTimerActive(TH_OnSpeechFinished); }
	void ClearSpeechTimerHandle() { GetWorld()->GetTimerManager().ClearTimer(TH_OnSpeechFinished); }
	UFUNCTION(BlueprintPure)
	FString GetSpeechCharacterName() const;
	void SetSpeechCharacterName(FString NewSpeechCharacterName) { SpeechCharacterName = NewSpeechCharacterName; }

	bool IsInPositionForCarry(const AReadyOrNotCharacter* Carrier) const;
	bool IsInPositionForArrest(const AReadyOrNotCharacter* ArrestTarget) const;
	
	UFUNCTION(BlueprintPure)
	bool CanCarryCharacter(AReadyOrNotCharacter* CharacterToPickup) const;
	
	UFUNCTION(BlueprintPure)
	bool CanDropCharacter(AReadyOrNotCharacter* CharacterToDrop) const;

	UFUNCTION(BlueprintPure)
	bool CanBePickedUp() const;

	UFUNCTION()
	void OnCarryPickupComplete(AActor* Driver, AActor* Slave);
	UFUNCTION()
	void OnCarryPickupComplete_Driver(AActor* Driver);
	UFUNCTION()
	void OnCarryPickupComplete_Slave(AActor* Slave);
	
	UFUNCTION()
	void OnCarryDropComplete(AActor* Driver, AActor* Slave);
	UFUNCTION()
	void OnCarryDropComplete_Driver(AActor* Driver);
	UFUNCTION()
	void OnCarryDropComplete_Slave(AActor* Slave);
	
	UFUNCTION()
	void OnCarryThrowComplete(AActor* Driver, AActor* Slave);
	
	UFUNCTION()
	void OnCarryThrowComplete_Driver(AActor* Driver);
	UFUNCTION()
	void OnCarryThrowComplete_Slave(AActor* Slave);

	void CarryArrestedTarget(AReadyOrNotCharacter* ArrestedCharacter);
	void DropArrestedTarget(AReadyOrNotCharacter* ArrestedCharacter);
	void ThrowArrestedTarget(AReadyOrNotCharacter* ArrestedCharacter);

	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation)
			void Server_CarryArrestedTarget(AReadyOrNotCharacter* ArrestedCharacter);
	virtual void Server_CarryArrestedTarget_Implementation(AReadyOrNotCharacter* ArrestedCharacter);
	virtual bool Server_CarryArrestedTarget_Validate(AReadyOrNotCharacter* ArrestedCharacter) { return true; }
	
	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation)
			void Server_DropArrestedTarget(AReadyOrNotCharacter* ArrestedCharacter);
	virtual void Server_DropArrestedTarget_Implementation(AReadyOrNotCharacter* ArrestedCharacter);
	virtual bool Server_DropArrestedTarget_Validate(AReadyOrNotCharacter* ArrestedCharacter) { return true; }

	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation)
			void Server_ThrowArrestedTarget(AReadyOrNotCharacter* ArrestedCharacter);
	virtual void Server_ThrowArrestedTarget_Implementation(AReadyOrNotCharacter* ArrestedCharacter);
	virtual bool Server_ThrowArrestedTarget_Validate(AReadyOrNotCharacter* ArrestedCharacter) { return true; }

	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation)
	void Server_ReportTarget(AActor* Character);
	virtual void Server_ReportTarget_Implementation(AActor* Character);
	virtual bool Server_ReportTarget_Validate(AActor* Character) { return true; }

	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation)
			void Server_ReportToTOC(AActor* Actor, bool bPlayAnimation = true, bool bTocResponse = true);
	virtual void Server_ReportToTOC_Implementation(AActor* Actor, bool bPlayAnimation = true, bool bTocResponse = true);
	virtual bool Server_ReportToTOC_Validate(AActor* Actor, bool bPlayAnimation = true, bool bTocResponse = true) { return true; }

	UFUNCTION(BlueprintCallable, NetMulticast, Reliable)
			void Multicast_OnTargetReported();
	virtual void Multicast_OnTargetReported_Implementation();

	FString TOCResponseLine = "";
	
	FTimerHandle ReportToTOC_Handle;
	
	UFUNCTION(Server, Reliable, WithValidation)
			void Server_PlayTOCConversation();
	virtual void Server_PlayTOCConversation_Implementation();
	virtual bool Server_PlayTOCConversation_Validate() { return true; }

	UFUNCTION(BlueprintCallable)
	void PlayTOCResponse(FString Line, bool bIsNetworked = true, ETOCPriority Priority = ETOCPriority::ETP_MediumPriority, bool bCanPrefix = true, float Delay = 0.0f);
	
	UFUNCTION(BlueprintCallable)
	void PlayROEViolateTOCResponse();

	uint8 bPendingROEViolateResponseOnReport : 1;
	uint8 bPendingROEViolateResponse : 1;

	void ResetROEViolateResponseFlag();

	FTimerHandle& GetOnSpeechFinishedHandle() { return TH_OnSpeechFinished; }

	UFUNCTION(BlueprintCallable)
	void RagdollArrestTarget(AReadyOrNotCharacter* RagdollCharacter);
	
	UFUNCTION(BlueprintPure)
	virtual bool CanArrestRagdoll() const;
	
protected:

	UPROPERTY()
	class ATOCManager* TOCManager;

	UPROPERTY(EditAnywhere, Category = Camera)
	TSubclassOf<ULegacyCameraShake> ReportToTOC_PVP_CameraShake;
	
	/** The line that we spoke to TOC, so we can queue up the appropriate response. */
	UPROPERTY(BlueprintReadOnly, Category = TOC)
	FString TOCLine;

	UPROPERTY(EditInstanceOnly, Replicated)
	FString SpeechCharacterName;

	FTimerHandle TH_OnSpeechFinished;

public:
	UPROPERTY(Replicated)
	bool bHasBeenReported = false;

	/* The amount of time remaining before we can speak again. */
	UPROPERTY(BlueprintReadOnly)
	float SpeakCooldown = 0.0f;
	
	UPROPERTY(EditAnywhere, Category = "FMOD Audio", DisplayName = "Report Player Dead")
	UFMODEvent* ReportPlayerDeadFMODEvent;

	UPROPERTY(EditAnywhere, Category = "FMOD Audio", DisplayName = "Report Player Arrested")
	UFMODEvent* ReportPlayerArrestedFMODEvent;
	
	UPROPERTY(EditAnywhere, Category = "FMOD Audio", DisplayName = "Report Player General")
	UFMODEvent* ReportPlayerGeneralFMODEvent;

	UFUNCTION(Client, Reliable, BlueprintCallable)
	void Client_PlayFMODEvent2D(UFMODEvent* Event);

	UFUNCTION()
	virtual void OnBodyFallAudioStop();
	
	/*
	 * Speech Related Functions
	 */
	
	UFUNCTION()
	virtual void OnVoiceAudioStopped();

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FVoiceAudioEvent, AReadyOrNotCharacter*, Speaker);
	UPROPERTY(BlueprintAssignable)
	FVoiceAudioEvent OnVoiceAudioStoppedDelegate;

	UFUNCTION(BlueprintCallable, DisplayName = "Play VO")
	bool PlayRawVO(const FString& VoiceLine, const FString& OverrideSpeakerName = "", bool bIgnoreIfAlreadyPlaying = true);
	UFUNCTION(BlueprintCallable, DisplayName = "Play VO (Cooldown)")
	void PlayRawVOWithCooldown(FString VoiceLine, float Cooldown = 10.0f, FString OverrideSpeakerName = "");
	
	virtual FString MutateVoiceline(const FString& VO) { return VO; }

	UFUNCTION()
	void PlayReportSpeech(FString Voiceline, FString InTOCLine = "");

	UFUNCTION(BlueprintCallable)
	void PlayRadioSelectAnimation();

	virtual bool CanPlayVO(const FString& VoiceLine = "") const;
	void RemoveVocalChords();

	bool bCannotSpeak = false;
	
	UFUNCTION(NetMulticast, Reliable, Category = Speech)
	void Multicast_PlayRawVO(const FString& SpecificFileName, const FString& OverrideSpeakerName = "", bool bIgnoreIfAlreadyPlaying = true);
	
	void PlayVoiceOverSubtitles(USoundSource* SoundSource, const FString& SpeakerName, const FString& VoiceLine);

	bool HasSpecificSpeech(FString VoiceLine);
	
	virtual UFMODEvent* GetAppropriateVoiceLineEvent();
	void SetupVoiceLineParameters(const FString& FileName);
	
	UFUNCTION(Exec)
	void PlaySpecificDebugVoiceLine(FString FileName);
	UFUNCTION(Exec)
	void PlayRandomDebugVoiceLine(FString Line);
	UFUNCTION(Exec)
	void PlayRandomDebugConversation();

	float CivilianRespondCooldown = 0.0f;
	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation)
	void Server_Yell();

	// Execute the 'yell' and have targets surrender/yell
	FTimerHandle TH_OnYellExecute;
	UFUNCTION()
	void OnYellExecute();

	void PlayYellAnimation();

	UFUNCTION(BlueprintPure, Category = "Yell")
	virtual bool CanYell() const;

	// stored locally so if you have high ping spam the yell and send 20 yells at once
	float TimeUntilNextYell = 0.0f;
	float TimeSinceLastYell = 86400.0f;

	void Yell();

	
protected:
	UPROPERTY()
	TMap<FString, float> SpeechCooldownMap;
	UPROPERTY(EditAnywhere, Category = "FMOD Audio")
	UFMODEvent* FMODVoiceLine2D;
	UPROPERTY(EditAnywhere, Category = "FMOD Audio")
	UFMODEvent* FMODVoiceLineSpatalized;

	
	/*
	 * Stun/Damage Code
	 */
public:
	virtual float TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override final;

	virtual bool OnTakeDamage(float& Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser);

	virtual bool ShouldTakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) const override;
	
	float TimeSinceLastTakenDamage = FLT_MAX;
	float TimeSinceLastTakenStunDamage = FLT_MAX;

	UFUNCTION(BlueprintPure)
	FORCEINLINE bool HasRecentlyTakenDamage(const float Tolerance = 0.5f) const { return TimeSinceLastTakenDamage < Tolerance; }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE bool HasRecentlyTakenStunDamage(const float Tolerance = 0.5f) const { return TimeSinceLastTakenStunDamage < Tolerance; }

	virtual bool TryApplyStunDamage(UStunDamage* InStunDamage, float& Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser);
	virtual bool TryApplyBulletDamage(float& Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser);

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = Damage)
	bool DamageHitHead(const FPointDamageEvent& DamageEvent);

	UFUNCTION(BlueprintCallable, Category = Damage)
	void ApplyDamageToBone(float& Damage, const FName& HitBone, FPointDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser);
	
	virtual void ApplyHeadDamage(float& Damage, const FPointDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser);
	virtual void ApplyUpperBodyDamage(float& Damage, const FPointDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser);
	virtual void ApplyLowerBodyDamage(float& Damage, const FPointDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser);
	virtual void ApplyLeftArmDamage(float& Damage, const FPointDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser);
	virtual void ApplyRightArmDamage(float& Damage, const FPointDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser);
	virtual void ApplyLeftLegDamage(float& Damage, const FPointDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser);
	virtual void ApplyRightLegDamage(float& Damage, const FPointDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser);
	virtual void ApplyLeftFootDamage(float& Damage, const FPointDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser);
	virtual void ApplyRightFootDamage(float& Damage, const FPointDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser);

	virtual void ApplyBodyDamage(float& Damage, struct FPointDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser);

	bool AreBonesInSameGroup(const FName& BoneA, const FName& BoneB) const;
	
	bool IsHeadBone(const FName& Bone) const;
	bool IsBodyBone(const FName& Bone) const;
	bool IsArmBone(const FName& Bone) const;
	bool IsLegBone(const FName& Bone) const;
	bool IsHandBone(const FName& Bone) const;
	bool IsFootBone(const FName& Bone) const;
	
	UPROPERTY(BlueprintReadOnly)
	TMap<FName, FSuppressionData> BoneSuppressionAmount;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBoneDamaged, FName, BoneHit);
	UPROPERTY(BlueprintAssignable)
	FOnBoneDamaged OnBoneDamaged;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_EightParams(FOnBodyPartDamaged, bool, bHeadDamaged, bool, bBodyDamaged, bool, bLeftArmDamaged, bool, bRightArmDamaged, bool, bLeftLegDamaged, bool, bRightLegDamaged, bool, bLeftFootDamaged, bool, bRightFootDamaged);
	UPROPERTY(BlueprintAssignable)
	FOnBodyPartDamaged OnBodyPartDamaged;

	UFUNCTION(Client, Reliable)
	void Client_OnBoneDamaged(const FName& BoneHit);
	void Client_OnBoneDamaged_Implementation(const FName& BoneHit);

	UFUNCTION(Client, Reliable)
	void Client_OnBodyPartDamaged(bool bInHeadHit, bool bInBodyHit, bool bInLeftArmHit, bool bInRightArmHit, bool bInLeftLegHit, bool bInRightLegHit, bool bInLeftFootHit, bool bInRightFootHit);
	void Client_OnBodyPartDamaged_Implementation(bool bInHeadHit, bool bInBodyHit, bool bInLeftArmHit, bool bInRightArmHit, bool bInLeftLegHit, bool bInRightLegHit, bool bInLeftFootHit, bool bInRightFootHit);

	UPROPERTY(BlueprintReadOnly, Replicated, Category = Damage)
	uint8 bBodyHit : 1;
	
	UPROPERTY(BlueprintReadOnly, Replicated, Category = Damage)
	uint8 bRightFootHit : 1;
	
	UPROPERTY(BlueprintReadOnly, Replicated, Category = Damage)
	uint8 bLeftFootHit : 1;
	
	UPROPERTY(BlueprintReadOnly, Replicated, Category = Damage)
	uint8 bBlockedByBodyArmor : 1;
	
	UPROPERTY(BlueprintReadOnly, Replicated, Category = Damage)
	uint8 bBlockedByHeadArmor : 1;
	
	FORCEINLINE bool KilledByHeadshot() const { return HeadBones.Contains(LastHitBoneName); }

	// Prediction used by clients for their hits
	void PredictHitEffects(FHitResult Hit, float WoundSize);
	
	// Multicast version of spawn blood effects
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SpawnBloodEffects(FHitResult Hit, float WoundSize, AController* HitInstigator);

	// Spawns the relevant blood effects depending on which area has been hit
	void SpawnBloodEffects(FHitResult Hit, float WoundSize);
	
	// Spawns dismemberment effects when dismemberment occurs
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SpawnDismembermentEffects(EGibAreas GibArea, FHitResult HitResult);
	
	// Spawns arterial blood effects when an artery has been hit
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SpawnArterialBloodEffects(FHitResult HitResult, FName Artery);

	int32 SkinnedDecalsSpawned = 0;
	int32 DeadSurfaceBloodDecalsSpawned = 0;

	bool bIsArterialBleeding = false;
	FName LastArterialHitBone;

	UPROPERTY(EditInstanceOnly)
	TMap<EGibAreas, float> DismembermentDamageMap;

	UPROPERTY(EditInstanceOnly)
	TArray<FName> DamageExcludedBones;

	FTimerHandle TH_ArteryDeath;
	FTimerHandle TH_DeathRattle;
	
	void ApplyArteryDamage(FHitResult HitResult, AActor* DamageCauser);
	void ApplyDismembermentDamage(FHitResult HitResult, AActor* DamageCauser);
	
	// TODO(killo): move these into a helper library?
	void SpawnBloodSplatterEffects(FHitResult HitResult);
	void SpawnAnimatedBloodSplatterEffects(FHitResult HitResult);
	void SpawnHeadshotSplatterEffects(FHitResult HitResult);

	UFUNCTION()
	void OnArteryBleedParticleCollision(FName EventName, float EmitterTime, int32 ParticleTime, FVector Location, FVector Velocity, FVector Direction, FVector Normal, FName BoneName, UPhysicalMaterial* PhysMat);
	
	UFUNCTION()
	void OnDismembermentParticleCollision(FName EventName, float EmitterTime, int32 ParticleTime, FVector Location, FVector Velocity, FVector Direction, FVector Normal, FName BoneName, UPhysicalMaterial* PhysMat);

	void SpawnBloodTrailEffects(FName EventName, FVector Location, FVector Normal, TArray<UMaterialInterface*>& Materials, float SpawnChance, FVector2D SizeRange);
	
	UFUNCTION()
	void SpawnBloodPool();

	float GetWoundSize(AActor* DamageCauser);
	
	bool IsBoneArmored(FName BoneName) const;

	ABaseArmour* GetArmourForBone(FName BoneName);

	void PlayArmourHitEffects(ABaseArmour* Armour, FHitResult Hit);
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Damage)
	bool IsStunnedWith(EStunType StunType) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Damage)
	bool IsOnlyStunnedWithGas() const;
	
	UFUNCTION(BlueprintPure, Category = Damage)
	FORCEINLINE bool IsStunned() const { return IsStunnedWith(EStunType::ST_None); }
		
	UFUNCTION(BlueprintCallable, Category = Damage)
	virtual void StartStun(EStunType StunType = EStunType::ST_None, AActor* StunCauser = nullptr);
	
	UFUNCTION(BlueprintCallable, Category = Damage)
	virtual void EndStun(EStunType StunType);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Damage)
	FORCEINLINE bool IsCurrentlyTased() const { return IsStunnedWith(EStunType::ST_Tased); }

	// Once the player has gotten 1.0 gas damage acquired, they become stunned by the gas
	UPROPERTY(BlueprintReadOnly, Category = Damage)
	float GasDamageAccumulated = 0.0f;

	// How much gas damage is removed every second
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Damage)
	float GasDamageDecay = 0.02f;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Damage)
	FORCEINLINE bool IsCurrentlyGassed() const { return IsStunnedWith(EStunType::ST_Gassed); }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Damage)
	bool IsCurrentlyFlashed() { return IsStunnedWith(EStunType::ST_Flash); }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Damage)
	bool IsCurrentlyStung() { return IsStunnedWith(EStunType::ST_Stung); }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Damage)
	bool IsCurrentlySprayed() { return IsPepperSprayed(); }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Damage)
	bool IsPepperSprayed() const { return GetWorld()->GetTimerManager().IsTimerActive(TH_PepperSprayed); }

	FTimerHandle TH_PepperSprayed;

	UFUNCTION(BlueprintCallable, Category = Damage)
	virtual void StartPepperSprayed(APepperspray* PeppersprayUsed);

	UFUNCTION(BlueprintCallable, Category = Damage)
	bool IsPepperSprayedLocationValid(const FHitResult& Hit, APepperspray* Pepperspray);

	UFUNCTION(BlueprintCallable, Category = Damage)
	virtual void EndPepperSprayed();

	UFUNCTION(BlueprintCallable, Category = Damage)
	virtual void StartBeingTasered(float PingStunDuration, class ATaser* WeaponUsed);

	virtual bool CanBeTased();

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = Damage)
	virtual bool IsAffectedByDamageType(UDamageType* DamageType) const;

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = Damage)
	virtual bool IsAffectedByDamageTypeClass(TSubclassOf<UDamageType> DamageType) const;

	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = Damage)
			void Multicast_InflictSuppression(FSuppressionData SuppressionData, TSubclassOf<ULegacyCameraShake> CameraShake, bool bLessLethal);
	virtual void Multicast_InflictSuppression_Implementation(FSuppressionData SuppressionData, TSubclassOf<ULegacyCameraShake> CameraShake, bool bLessLethal);

	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = Damage)
			void Multicast_InflictSuppression_NoLineOfSight(FSuppressionData SuppressionData, TSubclassOf<ULegacyCameraShake> CameraShake, bool bLessLethal);
	virtual void Multicast_InflictSuppression_NoLineOfSight_Implementation(FSuppressionData SuppressionData, TSubclassOf<ULegacyCameraShake> CameraShake, bool bLessLethal);

	UPROPERTY(BlueprintReadOnly)
	uint8 bGodMode : 1;

	void SetGodMode(const bool bInGodMode) { bGodMode = bInGodMode; }
	FORCEINLINE bool HasGodMode() const { return bGodMode; }
	
	bool HasNoTarget() const { return bNoTarget; }
	bool HasBeenDamagedByCharacter(const AReadyOrNotCharacter* Character) const { return DamagedByCharacters.Contains(Character); }
	bool HasBeenDamagedByLethal() const;
	bool HasBeenDamagedByLessLethal() const;
	
protected:
	// Whether the notarget cheat is enabled
	UPROPERTY(BlueprintReadOnly)
	bool bNoTarget = false;
	
	UPROPERTY(EditDefaultsOnly, Category = "Armor Impact Effects")
	UParticleSystem* ArmorImpactEffect;

	// The blood data associated with this character.
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Blood and Gore")
	class UBloodData* Blood;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Damage")
	TArray<ABaseWeapon*> DamagedByWeapons;
	
	FTimerHandle TH_EndStunTimer;
	UPROPERTY(Replicated)
	bool bRepStunned = false;
	UPROPERTY(Replicated)
	EStunType RepStunnedWith;
	TMap<FTimerHandle, EStunType> StunMap;
	UPROPERTY(Replicated)
	uint8 bHasEverBeenStunned : 1;
	float TimeSinceLastStun = FLT_MAX;
	float TimeSinceLastBulletDamage = FLT_MAX;
	float CurrentStunDuration = 0.0f;
	float CurrentStunTime = 0.0f;

public:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	TArray<AReadyOrNotCharacter*> AITrackingMe;
	
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Damage")
	TArray<AReadyOrNotCharacter*> DamagedByCharacters;
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE bool HasEverBeenStunned() const { return bHasEverBeenStunned; }
	UFUNCTION(BlueprintPure)
	FORCEINLINE float GetTimeSinceLastStun() const { return TimeSinceLastStun; }
	UFUNCTION(BlueprintPure)
	FORCEINLINE float GetCurrentStunDuration() const { return CurrentStunDuration; }
	UFUNCTION(BlueprintPure)
	FORCEINLINE float GetCurrentStunTime() const { return CurrentStunTime; }
	UFUNCTION(BlueprintPure)
	FORCEINLINE float GetTimeSinceLastBulletDamage() const { return TimeSinceLastBulletDamage; }

	UPROPERTY(BlueprintReadOnly)
	FVector OriginalSpawnLocation = FVector::ZeroVector;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEvidenceCollected, AActor*, Evidence);
	UPROPERTY(BlueprintAssignable)
	FOnEvidenceCollected OnEvidenceCollected;

	/*
	 * Evidence Collection
	 */
	UFUNCTION(BlueprintCallable, Category = "Evidence")
	virtual void PickupEvidence(AActor* InEvidence);
	
	UFUNCTION(BlueprintCallable, Category = "Evidence")
	void CollectPendingEvidence();

	UFUNCTION(Category = "Collection")
	class ACollectedEvidenceActor* SpawnEvidenceCollectionBag(FTransform SpawnTransform);

	UFUNCTION(BlueprintCallable, Category = "Evidence Collection")
	void BeginEvidenceCollection_COOP(AActor* InEvidenceActor, class UInteractableComponent* CollectionInteractableComp, float CollectionTime);

	UFUNCTION(BlueprintCallable, Category = "Evidence Collection")
	void EndEvidenceCollection_COOP(class UInteractableComponent* CollectionInteractableComp);
	
	UFUNCTION(BlueprintCallable, Category = "Evidence Collection")
	void CompleteEvidenceCollection_COOP(AActor* InEvidenceActor);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_CollectEvidence(ABaseItem* Item);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_CollectEvidenceActor(class AEvidenceActor* InEvidenceActor);
	
	FTimerHandle TH_CompleteEvidenceCollection;

	float GetEvidenceCollectionTime() const;
	FORCEINLINE bool HasCollectionAnimTriggered() const { return bCollectionAnimHasTriggered; }
	void TriggerCollectionAnim();

	void StopEvidenceCollectingAnims();

protected:
	UPROPERTY()
	AActor* PendingEvidence = nullptr;

	UPROPERTY()
	bool bIsCollectingEvidence = false;

	UPROPERTY()
	bool bCollectionAnimHasTriggered = false;

	UPROPERTY(EditAnywhere, Category = "Collection")
	UAnimMontage* CollectingLoopAnim1P = nullptr;

	UPROPERTY(EditAnywhere, Category = "Collection")
	UAnimMontage* CollectingLoopAnim3P = nullptr;

	UPROPERTY(EditAnywhere, Category= "Collection")
	TSubclassOf<class ACollectedEvidenceActor> CollectedEvidenceClass;

	UPROPERTY(EditAnywhere, Category="Collection")
	UFMODEvent* Reward;

	// Facial animation: emotion
	// All of this should be handled from the client end.
	UPROPERTY(BlueprintReadOnly, Category = "Facial Animation")
	ECharacterEmotion CurrentEmotion;

	UPROPERTY(BlueprintReadOnly, Category = "Facial Animation")
	float FacialAnimationOverrideTime = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Facial Animation")
	float FacialAnimationBlend = 0.0f;
	
	UPROPERTY(BlueprintReadOnly, Category = "Facial Animation")
	float FacialAnimationBlendTarget = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Facial Animation")
	float FacialAnimationBlendDecay = 0.2f;

	UPROPERTY(BlueprintReadOnly, Category = "Facial Animation")
	int32 FacialAnimationPriority = 0;
	
	virtual void Multicast_ChangeFaceEmotion_Implementation(ECharacterEmotion NewEmotion, float OverrideTime, float Blend, float BlendDecay, int32 Priority);

public:
	UFUNCTION(BlueprintCallable, Reliable, NetMulticast, Category = "Facial Animation")
	void Multicast_ChangeFaceEmotion(ECharacterEmotion NewEmotion, float OverrideTime, float Blend, float BlendDecay, int32 Priority);
	
	UFUNCTION(BlueprintCallable)
	virtual void LockAllActions();
	UFUNCTION(BlueprintCallable)
	virtual void UnlockAllActions();
	UFUNCTION(BlueprintCallable)
	virtual void LockMovementAndActions();
	UFUNCTION(BlueprintCallable)
	virtual void UnlockMovementAndActions();
	UFUNCTION(BlueprintCallable)
	virtual void LockMovement();
	UFUNCTION(BlueprintCallable)
	virtual void UnlockMovement();
	UFUNCTION(BlueprintCallable)
	virtual void LockAim();
	UFUNCTION(BlueprintCallable)
	virtual void UnlockAim();
	UFUNCTION(BlueprintCallable)
	virtual void LockItemSelection();
	UFUNCTION(BlueprintCallable)
	virtual void UnlockItemSelection();
	UFUNCTION(BlueprintCallable)
	virtual void LockCommandMenu();
	UFUNCTION(BlueprintCallable)
	virtual void UnlockCommandMenu();
	UFUNCTION(BlueprintCallable)
	virtual void LockWeaponAttachments();
	UFUNCTION(BlueprintCallable)
	virtual void UnlockWeaponAttachments();
	UFUNCTION(BlueprintCallable)
	virtual void LockCantedSight();
	UFUNCTION(BlueprintCallable)
	virtual void UnlockCantedSight();

	FORCEINLINE bool IsMovementLocked() const { return bMovementLocked; }
	FORCEINLINE bool IsAimLocked() const { return bAimLocked; }
	FORCEINLINE bool IsActionsLocked() const { return bActionsLocked; }
	FORCEINLINE bool IsCommandMenuLocked() const { return bCommandMenuLocked; }
	
	/* is our character in the aiming state */
	UPROPERTY(BlueprintReadWrite, Replicated, Category = Gameplay)
	bool bAiming = false;
	
	UFUNCTION(BlueprintPure, Category = Animation)
	bool IsReloading() const;
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = Gameplay)
	bool bMovementLocked = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = Gameplay)
	bool bAimLocked = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = Gameplay)
	bool bActionsLocked = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	bool bItemSelectionLocked = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	bool bCommandMenuLocked = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	bool bWeaponAttachmentsLocked = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	bool bCantedSightLocked = false;
	
	/*
	 * Door Use Related Code
	 */
public:
	UFUNCTION(BlueprintCallable, Category = Use)
	virtual bool OpenDoor(class ADoor* Door, bool bOpenDoor);

	UFUNCTION(BlueprintCallable)
	void KickDoor(ADoor* Door);
	
	// keyframe - kick in the door!
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable)
	void Server_KickQueuedDoor();

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable)
			void Server_KickBreakQueuedDoor();
	virtual void Server_KickBreakQueuedDoor_Implementation();

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable)
	void Server_KickFailQueuedDoor();

	void SetPendingDoorForKick(class ADoor* Door);
	virtual bool IsOpeningDoor(ADoor* Door) const;
	virtual bool IsClosingDoor(ADoor* Door) const;
	
	virtual bool CanPushDoor(ADoor* Door) const;

	UPROPERTY()
	class ADoor* QueuedDoorToOpen = nullptr;
	
	UPROPERTY()
	class ADoor* QueuedDoorToClose = nullptr;

protected:
	
	// The last door that we attempted to kick
	UPROPERTY(Replicated)
	class ADoor* LastKickedDoor; // TODO: move to cyber char

	// Door kicking interaction ?
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interactions")
	class UInteractionsData* DoorKickInteractionFront;

	// Door kicking interaction ?
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interactions")
	class UInteractionsData* DoorKickInteractionBack;

	// Door kick failure interaction
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interactions")
	class UInteractionsData* DoorKickFailureInteractionFront;

	// Door kick failure interaction
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interactions")
	class UInteractionsData* DoorKickFailureInteractionBack;

	// Door kick break interaction
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interactions")
	class UInteractionsData* DoorKickBreakInteractionFront;

	// Door kick break interaction
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interactions")
	class UInteractionsData* DoorKickBreakInteractionBack;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interactions")
	class UInteractionsData* CarryArrestedInteractionData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interactions")
	class UInteractionsData* DropArrestedInteractionData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interactions")
	class UInteractionsData* ThrowArrestedInteractionData;

	/*
	 * Inventory Helper Functions
	 */

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemUse, AReadyOrNotCharacter*, ItemOwner, ABaseItem*, Item);
	UPROPERTY(BlueprintAssignable)
	FOnItemUse Event_OnItemPrimaryUse;
	
	UFUNCTION(BlueprintPure)
	class ABaseItem* GetEquippedItem() const;
	
	UFUNCTION(BlueprintPure)
	ABaseMagazineWeapon* GetEquippedWeapon() const;
	
	UFUNCTION(BlueprintPure)
	TArray<class ABaseItem*> GetRemovedItems() const;

	template<typename T>
	T* GetEquippedItem() const;

	UFUNCTION()
	virtual void OnItemEquipped(ABaseItem* NewEquippedItem);
	
	UFUNCTION()
	virtual void OnItemHolstered(ABaseItem* HolsteredItem);

	void ThrowEquippedItem();
	void ThrowAllWeapons();
	void ThrowAllItems();

	UFUNCTION(BlueprintCallable)
	void ToggleNightvisionGoggles();
	
	// Post process and camera effects
	UFUNCTION(BlueprintCallable)
	virtual void EnableNightVisionGoggles();

	UFUNCTION(Server, Reliable, WithValidation)
			void Server_RepNVGOn(bool bIsOn);
	virtual void Server_RepNVGOn_Implementation(bool bIsOn);
	virtual bool Server_RepNVGOn_Validate(bool bIsOn) { return true; }
	// replciated variable so clients can enable nvigss
	
	UPROPERTY(Replicated)
	bool bNVGOn = false;

	UFUNCTION(BlueprintCallable)
	virtual void DisableNightVisionGoggles();

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnNightVisionGogglesToggled, AReadyOrNotCharacter*, Character, bool, bOn);
	UPROPERTY(BlueprintAssignable, Category = "Headgear")
	FOnNightVisionGogglesToggled OnNightVisionGogglesToggled;

	UFUNCTION(BlueprintPure)
	float GetDeltaRotationToCharacter(class AReadyOrNotCharacter* Character);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_AddMoveIgnoreActor(AReadyOrNotCharacter* MoveIgnoreCharacter, bool bAdd);
	
	FCollisionQueryParams GetCollisionQueryParameters() const;

	UFUNCTION(BlueprintPure)
	TArray<AActor*> GetCollisionIgnoredActors() const;

	UFUNCTION(BlueprintPure)
	TArray<UPrimitiveComponent*> GetCollisionIgnoredComponents() const;

	UFUNCTION(Client, Reliable)
	void Client_SetControlRotation(FRotator NewRotation);

	UFUNCTION(BlueprintPure)
	bool HasLineOfSightTo(const FVector& Location) const;

	bool bHasBeenSpottedBySWAT = false;

protected:
	FTraceHandle IsOutsideTraceHandle;
	bool bCachedIsOutside = false;
public:
	UFUNCTION(BlueprintCallable, Category = Visibility)
	virtual bool IsOutside();

	virtual bool CanBeSeenFrom(const FVector& ObserverLocation, FVector& OutSeenLocation, int32& NumberOfLoSChecksPerformed, float& OutSightStrength, const AActor* IgnoreActor = nullptr, const bool* bWasVisible = nullptr, int32* UserData = nullptr) const override;
	
	/*
	 * Footstep related code
	 */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFootstep);
	FOnFootstep OnFootstep;

	UFUNCTION(BlueprintCallable, Category = "Player | Movement")
	void SpawnFootstepEffect();

	UFUNCTION(BlueprintCallable, Category = FMOD)
	virtual bool GetFMODFootstepParameters(int32& Stance, int32& Speed, int32& Surface);

	void StartFoley(bool bShouldPlayEveryStep, UFMODEvent* LocalFoleyEvent, UFMODEvent* RemoteFoleyEvent);
	void StopFoley();

protected:

	// Whether we should be playing the footstep foley component
	UPROPERTY(BlueprintReadWrite, Category = "Audio")
	bool bShouldPlayFootstepFoley;

	// Play foley every step
	UPROPERTY(BlueprintReadWrite, Category = "Audio")
	bool bPlayEveryStep;

	UPROPERTY(EditAnywhere, Category = "Player | Movement")
	TSubclassOf<AImpactEffect> FootstepImpactEffectFast;
	
	UPROPERTY(EditAnywhere, Category = "Player | Movement")
	TSubclassOf<AImpactEffect> FootstepImpactEffectSlow;

	UPROPERTY(BlueprintReadWrite, Category = "Player | Movement")
	UFMODEvent* CurrentFootstepFoleyEvent;

	UPROPERTY(BlueprintReadWrite, Category = "Player | Movement")
	UFMODEvent* CurrentFootstepFoleyEventRemote;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player | Movement")
	UFMODEvent* FootstepsLocal;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player | Movement")
	UFMODEvent* FootstepsRemote;

	// Used to set the foley event used when this character moves
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player | Movement")
	UFMODEvent* MovementFoley;
	
	// The socket to attach the movement foley event to on the character
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player | Movement")
	FName MovementFoleySocket = "spine_3";
	
	/*
	 * Physics
	 */
public:
	void SetPhysicsAsset(UPhysicsAsset* NewPhysicsAsset, bool bForce = false);
	
	void SetAppropriatePhysicsAsset(bool bForce = false);
	UPhysicsAsset* GetAppropriatePhysicsAsset();

protected:
	/* the ragdoll we use by default, globally set to every character */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ragdoll")
	UPhysicsAsset* DefaultRagdollPhysAsset;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ragdoll")
    UPhysicsAsset* DefaultAlivePhysAsset;

	/* cuffed ragdoll start: */

	/* the ragdoll we swap to if the character died cuffed */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ragdoll")
	UPhysicsAsset* CuffedRagdollPhysAsset;

	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing=OnRep_ActiveRagdollPhysAsset, Category = "Ragdoll")
	UPhysicsAsset* Rep_ActiveRagdollPhysAsset = nullptr;

	UFUNCTION()
	void OnRep_ActiveRagdollPhysAsset();

public:
	UPROPERTY(BlueprintReadOnly)
	bool bIsRelevant = true;

	bool HasRecentlyShot();

	UFUNCTION(Server, Reliable, WithValidation, Category = Use)
			void Server_Interact(UObject* Interactable, class UInteractableComponent* InInteractableComponent);
	virtual void Server_Interact_Implementation(UObject* Interactable, class UInteractableComponent* InInteractableComponent);
	virtual bool Server_Interact_Validate(UObject* Interactable, class UInteractableComponent* InInteractableComponent) { return true; }
	
	UFUNCTION(Server, Reliable, WithValidation, Category = Use)
			void Server_EndInteract(UObject* Interactable, class UInteractableComponent* InInteractableComponent);
	virtual void Server_EndInteract_Implementation(UObject* Interactable, class UInteractableComponent* InInteractableComponent);
	virtual bool Server_EndInteract_Validate(UObject* Interactable, class UInteractableComponent* InInteractableComponent) { return true; }
		
	UFUNCTION(Server, Reliable, WithValidation, Category = Use)
			void Server_DoubleTapInteract(UObject* Interactable, class UInteractableComponent* InInteractableComponent);
	virtual void Server_DoubleTapInteract_Implementation(UObject* Interactable, class UInteractableComponent* InInteractableComponent);
	virtual bool Server_DoubleTapInteract_Validate(UObject* Interactable, class UInteractableComponent* InInteractableComponent) { return true; }
	
	UFUNCTION(Server, Reliable, WithValidation, Category = Use)
			void Server_MeleeInteract(UObject* Interactable, class UInteractableComponent* InInteractableComponent);
	virtual void Server_MeleeInteract_Implementation(UObject* Interactable, class UInteractableComponent* InInteractableComponent);
	virtual bool Server_MeleeInteract_Validate(UObject* Interactable, class UInteractableComponent* InInteractableComponent) { return true; }

	UFUNCTION(Server, Reliable, WithValidation, Category = Fire)
			void Server_Interact_PrimaryUse(UObject* Interactable, class UInteractableComponent* InInteractableComponent);
	virtual void Server_Interact_PrimaryUse_Implementation(UObject* Interactable, class UInteractableComponent* InInteractableComponent);
	virtual bool Server_Interact_PrimaryUse_Validate(UObject* Interactable, class UInteractableComponent* InInteractableComponent) { return true; }
	
	UFUNCTION(Server, Reliable, WithValidation, Category = Fire)
            void Server_EndInteract_PrimaryUse(UObject* Interactable, class UInteractableComponent* InInteractableComponent);
	virtual void Server_EndInteract_PrimaryUse_Implementation(UObject* Interactable, class UInteractableComponent* InInteractableComponent);
	virtual bool Server_EndInteract_PrimaryUse_Validate(UObject* Interactable, class UInteractableComponent* InInteractableComponent) { return true; }

	// IUseabilityInterface
	virtual void Interact_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent) override;
	virtual void EndInteract_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent) override;
	virtual class UInteractableComponent* GetInteractableComponent_Implementation() const override;
	virtual EInputEvent DetermineInputEvent_Implementation() const override;
	virtual FName DetermineAnimatedIcon_Implementation() const override;
	virtual FText DetermineActionText_Implementation() const override;
	virtual float DetermineInteractionDistance_Implementation() const override;
	virtual float DetermineCurrentProgress_Implementation() const override;
	virtual bool CanInteract_Implementation() const override;
	// IUseabilityInterface

	virtual bool CanShowActionPrompt1() const;

	// IReceiveAISenseUpdates
	virtual void OnAIPerceptionSense_Implementation(ACyberneticController* InSenseController, FAIStimulus Stimulus, AActor*& OutOverrideSensedActor) override final;
	virtual void OnAIHearingSense_Implementation(ACyberneticController* InSenseController, FAIStimulus Stimulus, AActor*& OutOverrideSensedActor) override final;
	virtual void OnAIDamageSense_Implementation(ACyberneticController* InSenseController, FAIStimulus Stimulus, AActor*& OutOverrideSensedActor) override final;
	// IReceiveAISenseUpdates

private:
	// Objective Tag for special arrest
	UPROPERTY()
	class ANeutralizeSuspectByTag* NeutralizeSuspectTag = nullptr;

public:
	// Objective Tag for special arrest
	class ANeutralizeSuspectByTag* GetNeutralizeSuspectTag();

	// for pre-loading death animations from table
	UPROPERTY()
	TArray<UAnimMontage*> TorsoDeathAnims;
	UPROPERTY()
	TArray<UAnimMontage*> HeadDeathAnims;
	UPROPERTY()
	TArray<UAnimMontage*> LeftArmDeathAnims;
	UPROPERTY()
	TArray<UAnimMontage*> RightArmDeathAnims;
	UPROPERTY()
	TArray<UAnimMontage*> LeftLegDeathAnims;
	UPROPERTY()
	TArray<UAnimMontage*> RightLegDeathAnims;

	TArray<UAnimMontage*> GetMontagesFromTableRow(FString RowName);
	
	UPROPERTY(BlueprintReadOnly)
	UAnimMontage* CurrentDeathMontage = nullptr;
	
	UPROPERTY(BlueprintReadOnly)
	uint8 bPlayingDeathMontage : 1;
	
	UPROPERTY(BlueprintReadOnly)
	uint8 bStartedPlayingDeath : 1;
	
	// merged for replicated acceleration
	virtual void PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker) override;

	UPROPERTY(Transient, ReplicatedUsing = OnRep_ReplicatedAcceleration)
	FRonReplicatedAcceleration ReplicatedAcceleration;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = CustomMovement)
	float ReplicatedMaxSpeed;

	UFUNCTION()
	void OnRep_ReplicatedAcceleration();

	UPROPERTY()
	bool bIsBlendRagdollNotifyActive;

	UFUNCTION(Exec, BlueprintCallable, Category = "Console Command")
	void TestPhysicalAnimationComponent();
	
	// Sound occlusion parameters
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player | Occlusion")
	float FootstepOcclusionMultiplier = 1.0f;

	// Depth to fully occlude gunshots (in cm)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player | Occlusion")
	float FootstepFullOcclusionDepth = 150.0f;

	bool bRagdolledAndIncapacitated;

	void SetPhysicsAssetAngularMotorAnimInfluence(UPhysicsAsset* PhysicsAsset, float InSpring, float InDamping, float InForceLimit, bool bSkipCustomPhysicsType);

private:
	const int32 MaxSnapshots = 48;

	UPROPERTY()
	TArray<FCharacterSnapshot> Snapshots;
	
	void SaveCharacterSnapshot();

public:
	FORCEINLINE const TArray<FCharacterSnapshot>& GetSnapshots() { return Snapshots; }
};

template <typename T>
T* AReadyOrNotCharacter::GetEquippedItem() const
{
	return Cast<T>(GetEquippedItem());
};
