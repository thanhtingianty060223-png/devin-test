// Copyright Void Interactive, 2022

#pragma once

#include "FMODBlueprintStatics.h"
#include "Actors/BaseItem.h"
#include "BaseWeapon.generated.h"

/**
 * TODO(killo): probably going to convert this to enum even though its less flexible
 * Armour Level Reference
 * 0 - None
 * 1 - Below IIA
 * 2 - IIA
 * 3 - II
 * 4 - IIIA
 * 5 - III
 * 6 - IV
 */

/**
 *	Ammunition type data, holds relevant information for a specific type of ammunition
 */
USTRUCT(BlueprintType)
struct FAmmoTypeData : public FTableRowBase
{
	GENERATED_BODY()

	FAmmoTypeData()
	{
		bIgnoresArmour = false;
		bIsUsableByPlayer = true;
	}

	// The variety of ammo, like FMJ, JHP, Buckshot, etc.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText AmmoVariety = FText::AsCultureInvariant("FMJ");

	// The ammo caliber, for display in the UI
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText AmmoCaliber = FText::AsCultureInvariant("");

	// The ammo description, for display in the UI
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText AmmoDescription = FText();

	// Base damage value, used if no damage over range curve is present
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Damage = 30.0f;

	// Damage value at a given distance, overrides damage if there are any keys present
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FRuntimeFloatCurve DamageOverRangeCurve = {};

	// Projectile count, useful for some kinds of ammo like buckshot
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	uint8 ProjectileCount = 1;

	// Spread pattern, where X is the min/max yaw and Y is the min/max pitch
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector2D SpreadPattern = FVector2D::ZeroVector;

	// Whether or not this ammo will ignore any armour regardless of level
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	uint8 bIgnoresArmour : 1;
	
	// How much durability damage to armour this ammo type does per projectile
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float DurabilityDamage = 1.0f;

	// Penetration level, represents what level of armour this ammo can penetrate
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	uint8 PenetrationLevel = 0;

	// Penetration distance in millimeters
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float PenetrationDistance = 400.0f;

	// Max chance for a ricochet to occur at the most shallow angles
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float RicochetChance = 1.0f;

	// The damage done to characters inside the spalling damage area
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float SpallingDamage = 10.0f;

	// The radius of the spalling damage area when it occurs
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float SpallingRadius = 50.0f;

	// Dismemberment occurs when dismemberment damage for a given limb hits 1.0
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float DismembermentDamage = 0.0f;
	
	// Chance for this ammo type to trigger arterial bleedout when hitting an artery zone
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float ArteryHitChance = 0.0f;
	
	// Chance to play full hit animations, lengthier animations that stop a suspect temporarily
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float HitsChance = 0.2f;
	
	// Chance to play heavier, lengthier hit animation when shot by this ammo type while wearing armour, 0.0 - 1.0
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float ArmouredHitsChance = 0.0f;

	// Size of skinned decal wounds when a character is hit with this ammunition
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float WoundSize = 35.0f;
	
	// Whether or not this ammo should be selectable/usable by the player
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	uint8 bIsUsableByPlayer : 1;
	
	// Icon used in the loadout menu
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UTexture2D* LoadoutIcon = nullptr;

	// Small ammo icon used for smaller summary views
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UTexture2D* SmallIcon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float HeadDamageMultiplier = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float UpperBodyDamageMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float LowerBodyDamageMultiplier = 1.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float ArmDamageMultiplier = 0.3f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float HandDamageMultiplier = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float LegDamageMultiplier = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float FootDamageMultiplier = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ragdoll")
	float DefaultRagdollImpulseStrength = 3000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ragdoll")
	float HeadRagdollImpulseStrength = 3000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ragdoll")
	float ArmRagdollImpulseStrength = 3000.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ragdoll")
	float LegRagdollImpulseStrength = 3000.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ragdoll")
	float TorsoRagdollImpulseStrength = 3000.0f;
};

/**
 * 
 */
UCLASS(Abstract)
class READYORNOT_API ABaseWeapon : public ABaseItem
{
	GENERATED_BODY()

public:
	// if this weapon uses a pistol grip (used for customization menu)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon")
	bool bPistolGrip = true;
	
private:
	/** Which firing mode we were in prior to going into Safe Mode. */
	UPROPERTY()
	EFireMode FiremodeBeforeSafe = EFireMode::FM_Single;

	/** Play the animation for switching to the current firing mode */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void PlayFiringModeAnimation();

protected:
	// NOTE(killo): this is not replicated since there aren't any client sided effects that depend on ammo type
	// Current ammo type in use
	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Weapon")
	FAmmoTypeData CurrentAmmoType;

	// Current ammo type row name
	FName CurrentAmmoTypeRowName;

	// The data table to fetch ammo types for this weapon from
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon", meta=(RequiredAssetDataTags="RowStructure=AmmoTypeData"))
	UDataTable* AmmoDataTable;
	
	// Accepted ammunition types for this weapon
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	TArray<FName> AmmunitionTypes;

public:
	ABaseWeapon();

	UPROPERTY()
	class UReadyOrNotLoadoutManager* LoadoutFunctionLibrary;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	UArrowComponent* BulletSpawn;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	UArrowComponent* ShellSpawn;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	UParticleSystemComponent* ShellParticle;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	USpotLightComponent* Flashlight;

	UPROPERTY(ReplicatedUsing = OnRep_AttachmentRep, VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	UScopedWeaponAttachment* ScopeAttachment;

	UPROPERTY(ReplicatedUsing = OnRep_AttachmentRep, VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	UWeaponAttachment* MuzzleAttachment;

	UPROPERTY(ReplicatedUsing = OnRep_AttachmentRep, VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	UWeaponAttachment* UnderbarrelAttachment;

	UPROPERTY(ReplicatedUsing = OnRep_AttachmentRep, VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	UWeaponAttachment* OverbarrelAttachment;

	UPROPERTY(ReplicatedUsing = OnRep_AttachmentRep, VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	UWeaponAttachment* StockAttachment;

	UPROPERTY(ReplicatedUsing = OnRep_AttachmentRep, VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	UWeaponAttachment* GripAttachment;

	UPROPERTY(ReplicatedUsing = OnRep_AttachmentRep, VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	UWeaponAttachment* IlluminatorAttachment;

	UPROPERTY(ReplicatedUsing = OnRep_AttachmentRep, VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	UWeaponAttachment* AmmunitionAttachment;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Audio)
	UFMODAudioComponent* ADSAudioComponent = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Audio)
	UFMODAudioComponent* ADSEndAudioComponent = nullptr;

	UPROPERTY()
	UFMODAudioComponent* ADSAudioComponents[5];
	UPROPERTY()
	UFMODAudioComponent* ADSEndAudioComponents[5];
	
	uint8 ADSCompIndex = 0;
	uint8 ADSEndCompIndex = 0;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Attachments")
	int32 AttachmentPoints = 5;

	UPROPERTY(Replicated)
	bool bSupressed = false;

	//FRecoilSpring RecoilSpring;

	virtual void BeginPlay() override;

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintImplementableEvent, Category = Events)
		void OnWeaponFired(FRotator fireDirection);

	UFUNCTION(BlueprintImplementableEvent, Category = Events)
		void OnWeaponFiredEnd();

	UFUNCTION(BlueprintImplementableEvent, Category = Events)
		void OnWeaponReloadStarted();

	UFUNCTION(BlueprintCallable, Category = Events)
		virtual void OnWeaponReloadComplete();

	UFUNCTION(BlueprintCallable, Category = "Recoil")
	void ResetRecoilSettingsToDefault();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void EnableGlimmer();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
    void DisableGlimmer();
	
	virtual void OnOwnerPossessed_Implementation() override;

	UPROPERTY(BlueprintReadWrite)	
	FRotator AimAssistRotation = FRotator::ZeroRotator;

	virtual void OnFire(FRotator Direction, FVector SpawnLoc);

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void OnFireAtBulletSpawn();
	
	FFMODEventInstance ADSSoundInstance;

	void LoadSavedDefaultFireMode();
	void SaveCurrentFireModeToPlayerState();
	EFireMode LoadLastFireModeFromPlayerState() const;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponFireModeChanged, EFireMode, newFireMode);
	UPROPERTY(BlueprintAssignable, Category = Events)
	FOnWeaponFireModeChanged OnFireModeChanged;

	UPROPERTY(BlueprintReadWrite, Replicated, Category = "AWeapon")
	EFireMode CurrentFireMode = EFireMode::FM_Single;
	
	UPROPERTY(BlueprintReadWrite, Category = "AWeapon")
	EFireMode DefaultFireMode = EFireMode::FM_Single;

	/** The list of available firing modes on this weapon, not including Safe Mode. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TArray<EFireMode> AvailableFireModes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float BurstBulletCount = 3;

	/** Whether this weapon has a Safe Mode. Double tapping the fire select key will put the weapon on Safe. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	bool bHasSafeMode = true;

	virtual void NextFireMode() override;
	virtual void SafeModeToggle();

	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool IsLethalWeapon() const;

	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool IsLessLethalWeapon() const;

	virtual void OnWeaponReload(bool bForce = false);

	UPROPERTY(BlueprintReadOnly, Category = "Weapon|Attachments")
	float AddedMagazineCountFromAttachments = 0.0f;

	UFUNCTION(BlueprintCallable, Category = Attachments)
	virtual void AddMagazineCountFromAttachments(float AddAmount) { AddedMagazineCountFromAttachments = AddAmount; }

	UFUNCTION(BlueprintCallable, Category = Gameplay)
	virtual bool CanReload();

	UFUNCTION(BlueprintCallable, Category = Events)
		virtual void OnAimDownSights(bool bWasAiming);

	UFUNCTION(BlueprintCallable, Category = Events)
		virtual void OnEndAimDownSights(bool bWasAiming);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon|UI")
		FName Optics_UI_Socket = "UI_Optics";

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon|UI")
		FName Grip_UI_Socket = "UI_Grip";

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon|UI")
		FName Muzzle_UI_Socket = "UI_Muzzle";

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon|UI")
		FName Stock_UI_Socket = "UI_Stock";

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon|UI")
		FName Magazine_UI_Socket = "UI_Magazine";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|ADS")
		float ADSZoom = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|ADS")
		float ADSZoomInSpeed = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|ADS")
		float ADSZoomOutSpeed = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
		TSubclassOf<UDamageType> DefaultDamageType;

	UPROPERTY()
		TSubclassOf<UDamageType> DamageType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
		TSubclassOf<ULegacyCameraShake> FireCameraShake;
	
	UPROPERTY()
	UCameraShakeBase* FireCameraShakeInst = nullptr;

	UPROPERTY(EditAnywhere, Category = "Weapon|Balance")
	FRuntimeFloatCurve DamageOverRange{};
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Balance")
		float Damage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Balance")
		float DamageSeverityMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Balance")
		float DamageSeverityChance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Balance")
		float BleedoutDamageMultiplier = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Balance")
		float BleedoutDamageChance = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Weapon|Balance")
		float DefaultDamage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Balance")
		float FireRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Balance")
		float MinFireRateAI;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Balance")
	float MaxFireRateAI;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Balance")
	int32 BulletsFiredUntilFullyAccurate;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Weapon|Balance")
		float ProjectileMovementSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Balance")
		bool bArmorPiercing;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
		TSubclassOf<AImpactEffect> ImpactEffects;

	// Optional. Effect class to play when bullets ricochet. If not specified, will use ImpactEffects instead.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
		TSubclassOf<AImpactEffect> RicochetEffects;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon")
	UParticleSystem* RicochetParticleSystem;

	UPROPERTY(EditAnywhere, Category="Weapon")
	UFMODEvent* RicochetEvent;

	UPROPERTY(EditAnywhere, Category="Weapon")
	UMaterialInterface* SpallingDecal;
	
	UPROPERTY(EditAnywhere, Category="Weapon")
	UParticleSystem* SpallingParticleSystem;

	UPROPERTY(EditAnywhere, Category="Weapon")
	UFMODEvent* SpallingEvent;

	// Optional. Effect class to play when bullets penetrate out the other side. If not specified, will use ImpactEffects instead.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
		TSubclassOf<AImpactEffect> ExitEffects;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
		float Wobble;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
		float InitialWobbleDelay;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
		bool bAttachBulletOnHit = true;

	// Is multiplier by velocity of the bullet too
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
		float BulletPhysicsImpulseMultiplier = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
		bool bDestroyBulletOnHit = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
		USkeletalMesh* BulletProjectileMesh;
		
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
        UStaticMesh* FakeProjectileMeshStatic;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
		UStaticMesh* BulletProjectileMeshStatic;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
		FVector BulletProjectileScale = FVector(1);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
		TSubclassOf<class ABaseShell> ShellClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
		UStaticMesh* ShellMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
		TArray<FRotator> RecoilPattern;
	int32 RecoilIndex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
		float BulletDrag = 3000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
		float RecoilInterpSpeed = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
		float ADSRecoilMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
		float ADSSpreadMultiplier = 1.0f;

	// Higher is faster.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
		float RecoilReturnRate = 10.0f;

	//float RecoilCurveTime = 0.0f;

	// Whether or not to ignore the spread value introduced by ammo
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	bool bIgnoreAmmoTypeSpread = false;

	// The base spread pattern value for this weapon
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FRotator SpreadPattern;

	int32 SpreadIndex;

	// Higher is faster.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
		float SpreadReturnRate = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FRotator PendingSpread;

	// Unused
	/*UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AWeapon")
		bool bFireIfNoAmmo = false;*/

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Weapon")
		FRotator FireDirection;
		
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
		float GlimmerIntensity = 25.0f;

	//void AdvanceRecoilCurveTime(float DeltaTime);

	// Get the ammo type currently in use by this weapon
	FORCEINLINE const FAmmoTypeData* GetCurrentAmmoType() const { return !CurrentAmmoTypeRowName.IsNone() ? &CurrentAmmoType : nullptr; }
	FORCEINLINE FName GetCurrentAmmoTypeRowName() const { return CurrentAmmoTypeRowName; }

	UFUNCTION(BlueprintPure, Category = "AWeapon")
	FRotator GetRecoil();

	void SetAmmunitionTypes(const TArray<FName>& AmmoTypes);
	FORCEINLINE TArray<FName> GetAmmunitionTypes() const { return AmmunitionTypes; }

	FORCEINLINE UDataTable* GetAmmoDataTable() const { return AmmoDataTable; }

	void SetAmmunitionType(FName AmmoTypeRowName);
	void SetAmmunitionType(int32 AmmoTypeIndex);

	UFUNCTION(BlueprintCallable, Category = AWeapon)
	void AddAttachment(UClass* Class, bool bReplicateAttachment = true);

	UFUNCTION(BlueprintCallable, Category = AWeapon)
	void RemoveAttachment(bool bScopedAttachment = false, bool bMuzzleAttachment = false, bool bUnderbarrelAttachment = false, bool bOverbarrelAttachment = false, bool bStockAttachment = false, bool bGripAttachment = false, bool bIlluminatorAttachment = false, bool bAmmunitionAttachment = false);

	UFUNCTION(BlueprintCallable, Category = AWeapon)
	bool CanAddAttachment(UClass* AttachmentClass);

	UFUNCTION()
	void UpdateStoredAttachments(TSubclassOf<UWeaponAttachment> Attachment);

	UFUNCTION(BlueprintPure, Category = Attachment)
	class ULaserAttachment* GetLaserAttachment();
	
	UFUNCTION(BlueprintPure, Category = Attachment)
	class ULightAttachment* GetLightAttachment();

	virtual bool GetMeshspaceTransform(FTransform& Default, FTransform& Aiming, FTransform& Back) override;
	virtual float GetADSZoomMultiplier();
	virtual float GetADSZoomInSpeed();
	virtual float GetADSZoomOutSpeed();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	int32 SpawnProjectileCount = 1;

	//UPROPERTY(VisibleAnywhere, Category = "AWeapon")
	//uint8 bUseRecoilCurve : 1;
	//
	//UPROPERTY(VisibleAnywhere, Category = "AWeapon", meta = (EditCondition = "bUseRecoilCurve"))
	//FRuntimeCurveLinearColor RecoilCurve{};
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float FirstShotRecoil = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float FirstShotSpread = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float FirstShotResetTime = 0.2f;

	//UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "AWeapon")
	//FRotator LastShotRecoil = FRotator(-0.7f, 0.0f, 0.0f);
	//
	//UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "AWeapon")
	//float LastShotResetTime = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float VelocitySpreadMultiplier = 0.01f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float VelocityRecoilMultiplier = 0.005f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float RecoilMultiplierPitch = 0.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float RecoilMultiplierYaw = 0.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float RefireDelay = 0.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float RecoilReturnPercentage = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float RecoilReturnInterpSpeed = 2.5f;

	uint8 bReturnRecoil : 1;

	bool bRefireDelayTriggered = false;
	FTimerHandle RefireDelay_Handle;
	void ResetRefireDelay();

	bool bFirstShot = true;
	//bool bLastShot = false;
	FTimerHandle ResetFirstShot_Handle;
	//FTimerHandle ResetLastShot_Handle;
	void ResetFirstShot();
	//void ResetLastShot();
	bool IsFirstShot();
	//bool IsLastShot();
	void TriggerFirstShot();
	//void TriggerLastShot();

	FTimerHandle ResetShotsFired_Handle;
	uint16 RecentShotsFired = 0;
	void IncrementShotsFired();
	void ResetShotsFired();

	UFUNCTION(BlueprintPure, Category = "Weapon")
	FRotator GetSpread();

	virtual void OnRep_AttachmentRep() override;

	virtual void OnRep_AttachmentReplication() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Attachments")
	bool bAcceptsScopeAttachments = false;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Attachments")
	bool bAcceptsMuzzleAttachments = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Attachments")
	bool bAcceptsUnderbarrelAttachments = false;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Attachments")
	bool bAcceptsOverbarrelAttachments = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Attachments")
	bool bAcceptsStockAttachments = false;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Attachments")
	bool bAcceptsGripAttachments = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Attachments")
	bool bAcceptsIlluminatorAttachments = false;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Attachments")
	bool bAcceptsAmmunitionAttachments = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Attachments")
	TArray<TSubclassOf<UScopedWeaponAttachment>> AvailableScopeAttachments;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Attachments")
	TArray<TSubclassOf<UWeaponAttachment>> AvailableMuzzleAttachments;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Attachments")
	TArray<TSubclassOf<UWeaponAttachment>> AvailableUnderbarrelAttachments;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Attachments")
	TArray<TSubclassOf<UWeaponAttachment>> AvailableOverbarrelAttachments;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Attachments")
	TArray<TSubclassOf<UWeaponAttachment>> AvailableStockAttachments;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Attachments")
	TArray<TSubclassOf<UWeaponAttachment>> AvailableGripAttachments;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Attachments")
	TArray<TSubclassOf<UWeaponAttachment>> AvailableIlluminatorAttachments;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Attachments")
	TArray<TSubclassOf<UWeaponAttachment>> AvailableAmmunitionAttachments;

	FORCEINLINE class UArrowComponent* GetBulletSpawn() const { return BulletSpawn; }
	FORCEINLINE class UArrowComponent* GetShellSpawn() const { return ShellSpawn; }
	FORCEINLINE class UScopedWeaponAttachment* GetScopedAttachment() const { return ScopeAttachment;  }
	FORCEINLINE class UWeaponAttachment* GetUnderbarrelAttachment() const { return UnderbarrelAttachment; }
	FORCEINLINE class UWeaponAttachment* GetOverbarrelAttachment() const { return OverbarrelAttachment; }
	FORCEINLINE class UWeaponAttachment* GetMuzzleAttachment() const { return MuzzleAttachment; }
		
	// alex: procedural weapon animation addition

	/* if this is false, the calculations will not tick to compute the proc recoil */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|ProcRecoil")
	bool bCalculateProcRecoil;

	/* controls the strength of damp effect */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|ProcRecoil")
	float RecoilDampStrength;

	/* specifies for how long a recoil force should be applied */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|ProcRecoil")
	float RecoilFireTime;

	/* specifies how strong the recoil is */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|ProcRecoil")
	float RecoilFireStrength;

	/* does the same but is only applied for the first shot in a rapid fire of burst fire modes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|ProcRecoil")
	float RecoilFireStrengthFirst;

	/* will specify the maximum deviation angle the recoil can have */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|ProcRecoil")
	float RecoilAngleStrength;

	/* provides a more organic feeling to the recoil */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|ProcRecoil")
	float RecoilRandomness;

	/* specifies how ads influences the position recoil strength */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|ProcRecoil")
	float RecoilFireADSModifier;

	/* specifies how ads influences the angle recoil strength */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|ProcRecoil")
	float RecoilAngleADSModifier;

	/* will accummulate rotation over the firing duration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|ProcRecoil")
	FRotator RecoilRotationBuildup;

	/* will accummulate position over the firing duration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|ProcRecoil")
	FVector RecoilPositionBuildup;

	/* scale buildup when using sights */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|ProcRecoil")
	float RecoilBuildupADSModifier;

	/* apply buildup accummulation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|ProcRecoil")
	bool RecoilHasBuildup;

	/* controls the strength of damp effect */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|ProcRecoil")
	float RecoilBuildupDampStrength;

	virtual void TriggerProcRecoil(bool bIsFirstFire);
	virtual void ComputeProcRecoil(float DeltaTime);

	

	FVector ProcRecoil_TransDir;
	FRotator ProcRecoil_RotDir;
	float ProcRecoil_fireTime;
	bool ProcRecoil_firstFire;

	float ProcRecoil_PosADSModifier;
	float ProcRecoil_RotADSModifier;

	float ProcRecoil_BuildupADSModifier;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|ProcRecoil")
	FVector ProcRecoil_Trans;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|ProcRecoil")
	FRotator ProcRecoil_Rot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|ProcRecoil")
	FVector ProcRecoil_Trans_Buildup;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|ProcRecoil")
	FRotator ProcRecoil_Rot_Buildup;

	// High timer stuff for alex --eez
	UPROPERTY(BlueprintReadOnly, Category = "AWeapon", meta = (AllowPrivateAccess = "true"))
		float CurrentHighTimer = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|High Timer")
		float ReloadHighTimer = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|High Timer")
		float FireHighTimer = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|High Timer")
		float EquipHighTimer = 2.0f;

	virtual bool PlayDraw(bool bDrawFirst) override;
	virtual void Client_OnItemPickedUp_Implementation(AActor* NewOwner, bool bEquipped) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Scope Mask")
		bool bUseScopeMask = false;
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Weapon")
		virtual EWeaponUnderbarrelAnimationType GetUnderbarrelAnimationType() const;
};
