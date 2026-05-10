// Copyright Void Interactive, 2024

#pragma once

#include "Actors/BaseWeapon.h"
#include "lib/DataSingleton.h"
#include "Interfaces/ReceiveAISenseUpdates.h"

#include "BaseMagazineWeapon.generated.h"

UENUM(BlueprintType)
enum class EReloadAnimEvent : uint8
{
	RAE_MagIn,
	RAE_MagOut,
	RAE_MagInQuick,
	RAE_MagOutQuick,
	RAE_BoltClosed,
	RAE_BoltClosedQuick,
	RAE_BoltOpened,
	RAE_BoltOpenedQuick,
};

/*
 *	Holds all information needed to represent a magazine locally/over the network
 */
USTRUCT(BlueprintType)
struct FMagazine
{
	GENERATED_BODY()
	
	FMagazine()
	{
		Ammo = 0;
		AmmoType = 0;
	}
	
	FMagazine(const uint16 Count, const uint16 Type)
	{
		Ammo = Count;
		AmmoType = Type;
	}

	// Amount of ammo remaining in this magazine
	UPROPERTY(EditAnywhere)
	uint16 Ammo = 0;

	// The ammo type index pointing into the parent weapons ammunition types array
	UPROPERTY(EditAnywhere)
	uint16 AmmoType = 0;
};

USTRUCT()
struct FHitscanShot
{
	GENERATED_BODY()
	
	UPROPERTY()
	FVector Location;

	UPROPERTY()
	FVector Direction;

	UPROPERTY()
	int32 Seed;
};

USTRUCT()
struct FRicochet
{
	GENERATED_BODY()

	FVector Location;
	FVector Normal;
	FVector Direction;

	int32 OutHitsIndex;
};

USTRUCT()
struct FHitscanCharacterHit
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<UPrimitiveComponent> Component = nullptr;

	FName BoneName = NAME_None;
};

USTRUCT(BlueprintType)
struct FSuppressionData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FVector Origin = FVector::ZeroVector;
	
	UPROPERTY(BlueprintReadOnly)
	FVector Direction = FVector::ZeroVector;
	
	UPROPERTY(BlueprintReadOnly)
	float Strength = 0.0f;
	
	UPROPERTY(BlueprintReadOnly)
	float Distance = 0.0f;
	
	UPROPERTY(BlueprintReadOnly)
	AReadyOrNotCharacter* Instigator = nullptr;
};

DECLARE_STATS_GROUP(TEXT("BaseMagazineWeapon"), STATGROUP_BaseMagazineWeapon, STATCAT_Advanced);

UCLASS(Abstract)
class READYORNOT_API ABaseMagazineWeapon : public ABaseWeapon, public IReceiveAISenseUpdates
{
	GENERATED_BODY()

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	class UParticleSystemComponent* MuzzleFlashParticleComponent;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	class UParticleSystemComponent* MuzzleSmokeParticleComponent;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	class UParticleSystemComponent* HeatSmokeParticleComponent;
	
	UPROPERTY(Transient, VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	class UParticleSystemComponent* RicochetParticleComponent;

	UPROPERTY(Transient)
	class UParticleSystemComponent* RicochetParticleComponents[5];

	uint8 RicochetCompIndex = 0;

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	virtual void Tick(float DeltaSeconds) override;

	virtual bool IsCurrentlyReloading() const override;
	virtual void CancelCurrentReloadAction_Implementation(bool bCancel) override;

	//virtual void OnItemDrawnComplete_Implementation(AActor* ItemOwner) override;

	FVector PreviousMag01Location;
	FVector CurrentMag01Velocity;

	ABaseMagazineWeapon();
	
	UPROPERTY()
	UFMODAudioComponent* FiringAudioComp = nullptr;

	/*
	 * will probably remove these texts, converted to just strings for now -killo
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Magazine Weapon|Manufacturer Info")
	FString CartridgeText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Magazine Weapon|Manufacturer Info")
	FString RPMText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Magazine Weapon|Manufacturer Info")
	FString BarrelLengthText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Magazine Weapon|Manufacturer Info")
	FString CapacityText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Magazine Weapon|Manufacturer Info")
	FString MuzzleVelocityText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Magazine Weapon")
		float SupressionStrength = 1.00f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Magazine Weapon")
		TSubclassOf<ULegacyCameraShake> SupressionCameraShake;
		
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Magazine Weapon")
	bool bSpawnNoTrail = false;

	UPROPERTY()
	TArray<	class ABulletTracer*> BulletTracers;

	UPROPERTY()
	TArray<ABaseShell*> SpawnedShells;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponFire, ABaseMagazineWeapon*, Weapon, bool, bServer);
	UPROPERTY(BlueprintAssignable, Category = Events)
	FOnWeaponFire OnWeaponFire;

	UPROPERTY(BlueprintAssignable, Category = Events)
	FOnWeaponFire OnWeaponDryFire;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponMagCheck, ABaseMagazineWeapon*, Weapon);
	FOnWeaponMagCheck OnWeaponMagCheck;
	
	DECLARE_DELEGATE(FOnWeaponFire_Native);
	FOnWeaponFire_Native Native_OnWeaponFire;

	UFUNCTION(BlueprintCallable, Category = Gameplay)
	virtual void OnFire(FRotator Direction, FVector SpawnLoc) override;

	float RefireDelayTimer = 0.0f;
	float TimeSinceLastShot = 0.0f;

	bool bNegativeRecoilSet = false;

	int32 LastSeed = -1;
	virtual void LocallySimulateFire(FRotator Direction, FVector SpawnLoc, int32 Seed);

	void PlayFireAnimation(bool bIsAiming, bool bIsCrouched, bool bOnlyLocal);
	void PlayDryFireAnimation(bool bIsAiming, bool bIsCrouched, bool bOnlyLocal);

	virtual FRotator CalculateRecoil();
	
	UFUNCTION(NetMulticast, Unreliable)
		void Multicast_SimulateFireForViewTargets(FVector_NetQuantize100 DirectionNet, FVector_NetQuantize100 SpawnLoc, int32 Seed);
	virtual void Multicast_SimulateFireForViewTargets_Implementation(FVector_NetQuantize100 DirectionNet, FVector_NetQuantize100 SpawnLoc, int32 Seed);

	UPROPERTY(Replicated, VisibleAnywhere, Category = "Magazine Weapon|Debug")
		bool bReloading = false;

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = Reload)
		void Server_SetCancelReloading(bool bNewValue);
	virtual void Server_SetCancelReloading_Implementation(bool bNewValue) { bCancelReloading = bNewValue; }
	virtual bool Server_SetCancelReloading_Validate(bool bNewValue) { return true; }

	UPROPERTY(Replicated, VisibleAnywhere, Category = "Magazine Weapon|Debug")
		bool bCancelReloading = false;

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_OnFire(FRotator Direction, FVector SpawnLoc, int32 Seed);
	virtual void Server_OnFire_Implementation(FRotator Direction, FVector SpawnLoc, int32 Seed);
	virtual bool Server_OnFire_Validate(FRotator Direction, FVector SpawnLoc, int32 Seed) { return true; }

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnFire(FVector_NetQuantize100 DirectionNet, FVector_NetQuantize100 SpawnLoc, int32 Seed);
	virtual void Multicast_OnFire_Implementation(FVector_NetQuantize100 DirectionNet, FVector_NetQuantize100 SpawnLoc, int32 Seed);

	virtual void OnItemPrimaryUseEnd() override;

	UFUNCTION(NetMulticast, BlueprintCallable, Unreliable, Category = Gameplay)
	void Multicast_SpawnShell(bool bOnlyLocallyControlled = false, bool bSkipLocallyControlled = false);
	virtual void Multicast_SpawnShell_Implementation(bool bOnlyLocallyControlled = false, bool bSkipLocallyControlled = false);

	virtual void OnWeaponReloadComplete() override;

	virtual FVector GetLocation_Implementation() const override;

	UPROPERTY(EditAnywhere, Category="Magazine Weapon")
	bool bHitScan = false;

	UPROPERTY(EditAnywhere, Category="Magazine Weapon")
	bool bSpawnTracer = false;

	UPROPERTY(EditAnywhere, Category="Magazine Weapon")
	bool bNoSpawnTracerForFiringPlayer = false;
	
	// if 2 then spawn tracer reset to 0
	int32 TracerCount = 0;

	uint32 BulletsFired = 0;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Magazine Weapon|FX")
		FName MuzzleFlashParticleSocket = "P_MuzzleFlash";


	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Magazine Weapon|FX")
		FName MuzzleSmokeParticleSocket = "P_MuzzleSmoke";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Magazine Weapon|FX")
	FTimerHandle GunTails_Handle;
	
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void PlaySound(USoundCue* Cue);

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Magazine Weapon|FX")
		bool bShowParticlesWhenFiring = true;

	// If applicable...
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Magazine Weapon")
		TSubclassOf<UDamageType> ArmorPiercingDamageType;
		
	UPROPERTY(EditAnywhere, Category = "Magazine Weapon|FX")
	UParticleSystem* ProjectileAttachedParticle;


	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Magazine Weapon|FX")
		bool bDrawBlood = true;

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_SpawnParticleEffects(bool bSkipAuthority = false, bool bSkipLocalOwner = false);
	virtual void Multicast_SpawnParticleEffects_Implementation(bool bSkipAuthority = false, bool bSkipLocalOwner = false);


	UFUNCTION(NetMulticast, Reliable)
	void Multicast_HandleSupression();
	virtual void Multicast_HandleSupression_Implementation();
	float TeamMateShotHeadDelay = 0.0f;

	UFUNCTION(BlueprintCallable, Category = Supression)
	void PlayBulletWhizz(float Pan);

	// The bullet projectile to use if the game has been pirated
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Magazine Weapon|Projectile")
    TSubclassOf<class ABulletProjectile> FakeBulletProjectile;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Magazine Weapon|Projectile")
	TSubclassOf<class ABulletProjectile> BulletProjectile;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Magazine Weapon|Projectile")
	TSubclassOf<class ABulletProjectile> ArmorPiercingBulletProjectile;

	// How far the bullets penetrate in CMs (if player selects AP bullet then this distance is doubled)
	UPROPERTY(EditAnywhere, Category = "Magazine Weapon|Ballistics")
	float PenetrationDistance = 20.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Magazine Weapon|Shell")
	UStaticMesh* BreachShell;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Magazine Weapon|Shell")
	UStaticMesh* BeanbagShell;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Magazine Weapon|Shell")
	UStaticMesh* BuckShotShell;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Magazine Weapon|Shell")
	UStaticMesh* SlugShell;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Magazine Weapon|Shell")
	FName MagazineSocket;

	UPROPERTY(Transient, EditDefaultsOnly, BlueprintReadWrite, Replicated, Category="Magazine Weapon|Magazine")
	TArray<FMagazine> Magazines;

	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Replicated, Category="Magazine Weapon|Magazine")
	int32 MagIndex;

	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Replicated, Category="Magazine Weapon|Magazine")
	int32 NextMagIndex;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Magazine Weapon|Magazine")
	FName HideBoneWhenNotReloading = NAME_None;

	// Ammo type that we prefer when reloading magazines
	UPROPERTY(Transient)
	int32 DesiredAmmoType;

	UPROPERTY(BlueprintReadWrite)
	uint8 bInfiniteAmmo : 1;

	UFUNCTION(Server, Reliable)
	void Server_SetDesiredAmmoTypeIndex(int32 Index);

	// Ammo type to change to once we fire the bullet in the chamber
	UPROPERTY()
	int32 QueuedAmmoType;

	UFUNCTION(BlueprintCallable, Category="Magazine Weapon|Magazine")
	void FindNextMagIndex();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Magazine Weapon|Magazine")
		bool bBulletInChamberOnReload = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Magazine Weapon|Magazine")
		bool bTacReloadWhenEmpty = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Magazine Weapon|Magazine")
		float AmmoMax;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Magazine Weapon|Breaching")
		float LockIntegrityMinDamage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Magazine Weapon|Breaching")
		float LockIntegrityMaxDamage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Magazine Weapon|Magazine")
		bool bLoseMagOnReload;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Magazine Weapon|Shell")
		bool bSpawnShell = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Magazine Weapon|Shell")
		float SpawnShellDelay = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Magazine Weapon|Shell")
		UFMODEvent* ShellBounceFMODAudio;

	/*
	// Get the display for the current magazine
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ammo")
		virtual ERemainingAmmoDisplay GetRemainingAmmoDisplay();

	// Get the display for the next magazine (only different from GetRemainingAmmoDisplay() if we are in the middle of a reload)
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ammo")
		virtual ERemainingAmmoDisplay GetNextRemainingAmmoDisplay();

	// Get the ammo display for a specific amount of ammo
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ammo")
		virtual ERemainingAmmoDisplay GetAmmoDisplayForAmount(float AmmoAmount);*/

	void SpawnShell(bool bOnlyLocal);
	
	FTimerHandle SpawnLocalShell_Handle;
	void SpawnLocalShell();

	FTimerHandle SpawnRemoteShells_Handle;
	void SpawnRemoteShells();

	virtual bool CanReload() override;

	bool bOverrideLoseMagOnReload;

	bool bClientPredictReload = true;

	UPROPERTY(EditAnywhere, Category="Magazine Weapon")
	float ImpactDecalScale = 1.0f;

	UPROPERTY()
	AImpactEffect* ImpactEffects_Instance = nullptr;

	UPROPERTY()
	TArray<AImpactEffect*> SpawnedImpactEffects;

	// Get ammo for a specific magazine
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Gameplay)
		virtual float GetAmmoInMagazine(int32 Index) const;

	// Get ammo for current magazine
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Gameplay)
		virtual float GetAmmo() const { return GetAmmoInMagazine(MagIndex); }

	// Get ammo for next magazine (only different from GetAmmo() if we are in the middle of a reload)
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Gameplay)
		virtual float GetNextAmmo() const { return GetAmmoInMagazine(NextMagIndex); }

	// Returns a copy of this weapon's currently loaded magazine
	UFUNCTION(BlueprintCallable, BlueprintPure)
		FMagazine GetCurrentMagazine() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "UI")
		virtual float GetMagazineAmmoPercentage(int32 MagazineIndex) const;

	// Gets the current ammo percentage for this weapon's current magazine
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "UI")
		virtual float GetCurrentAmmoPercentage() const { return GetMagazineAmmoPercentage(MagIndex); }

	// Gets the screen name for a specific magazine corresponding to its ammo type
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "UI")
		FText GetMagazineScreenName(const FMagazine& Magazine) const;

	// Has any ammo in any magazine?
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Gameplay)
		virtual bool HasAnyAmmo() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Gameplay)
		virtual bool HasAnyAmmoOfType(FName AmmoType) const;

	// Has any ammo in current magazine?
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Gameplay)
		virtual bool HasAmmo() const;

	// Removes X ammo, returns remainder
	UFUNCTION(BlueprintCallable, Category = Gameplay)
		virtual float RemoveAmmo(float Value);

	UFUNCTION(BlueprintCallable, Category = Gameplay)
		virtual void GivenAmmoFromAmmoBag() { Server_AddMagazine(FMagazine(AmmoMax, 0)); }

	UFUNCTION(BlueprintPure, Category = "Gameplay")
	bool AllMagsEmpty() const;

	virtual bool IncrementAmmoType();
	bool IncrementAmmoType(FName AmmoType);

	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category = Gameplay)
		void Server_NextMagazine();
	virtual void Server_NextMagazine_Implementation();
	virtual bool Server_NextMagazine_Validate() { return true; }

	UFUNCTION(Server, Reliable, WithValidation)
		void Server_SetTacticalReload(bool bIsTacticalReload);
	virtual void Server_SetTacticalReload_Implementation(bool bIsTacticalReload) { bTacticalReload = bIsTacticalReload; }
	virtual bool Server_SetTacticalReload_Validate(bool bIsTacticalReload) { return true; }

	UFUNCTION(Server, Reliable, WithValidation)
		void Server_SetReloading(bool bIsReloading);
	virtual void Server_SetReloading_Implementation(bool bIsReloading);
	virtual bool Server_SetReloading_Validate(bool bIsReloading) { return true; }

	UPROPERTY(EditAnywhere, Category="Magazine Weapon|Magazine")
		FName Magazine_HandSocket;

	// Whether or not ammo types should be shown on the HUD for this weapon
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Magazine Weapon")
	bool bShowAmmoTypesOnHUD = true;

	virtual void OnItemSecondaryUsed() override;
	virtual void OnItemEndSecondaryUse() override;
	virtual void OnWeaponReload(bool bForce = false) override;
	virtual void OnWeaponTacticalReload();
	
	virtual void OnThrownFromInventory(AReadyOrNotCharacter* Thrower, bool bMarkAsEvidence = true) override;

	UFUNCTION(Server, Reliable)
	void Server_AddMagazine(FMagazine Magazine);

	bool bTacticalReload = false;

	UPROPERTY(EditAnywhere, Category="Magazine Weapon|Morale")
	FString MoraleHighReloadTableOverride = "";

	UPROPERTY(EditAnywhere, Category="Magazine Weapon|Morale")
	FString MoraleMediumReloadTableOverride = "";
	
	UPROPERTY(EditAnywhere, Category="Magazine Weapon|Morale")
	FString MoraleLowReloadTableOverride = "";

	virtual void MagCheck();

	//void SpawnMagCheckUI();

	bool bNoLocalProjectile = false;
	bool bOnlyDetonateLocalProjectile = false;

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PerformHitscan(const FHitscanShot& HitscanShot, bool bLocalOnly, int32 Seed);

	UFUNCTION(Server, Reliable)
	void Server_HitscanHit(const FHitResult& HitResult, float Time, FVector TraceBegin, float Distance, float Penetration, int32 AmmoTypeIndex);
	
	UFUNCTION(Client, Reliable)
	void Client_HitscanDebug(bool bSuccess, FVector Center, FVector Extent, FVector ImpactPoint);

	void ApplyHitscanDamage(const FHitResult& HitResult, float TotalDistance, float Penetration, const FAmmoTypeData* ShotAmmoType);
	void CalculateSuppressionForAI(const FVector& Start, const FVector& End);

	void PlayHitscanImpactEffects(const FHitResult& HitResult, bool bIsEntry);
	void PlayHitscanRicochetEffects(const TArray<FRicochet>& Ricochets);
	
	bool bAIFireAtBulletSpawn = false;

	ABulletProjectile* SpawnProjectile(TSubclassOf<ABulletProjectile> Class, FTransform SpawnTransform, bool bLocalOnly = false, int32 ProjectileNumber = 0, int32 Seed = FMath::Rand());
	UPROPERTY(Replicated)
	ABulletProjectile* LastSpawnedProjectile;

	virtual bool IsBlockingAnimationPlaying(TArray<EBlockingAnimationExclusion> Exclusions = {}) const override;

	virtual void OnRep_AttachmentRep() override;

	UFUNCTION(BlueprintPure)
	bool IsPistolWithShield() const;

	FORCEINLINE class UParticleSystemComponent* GetMuzzleFlashParticleComp() const { return MuzzleFlashParticleComponent; }
	FORCEINLINE class UParticleSystemComponent* GetMuzzleSmokeParticleComp() const { return MuzzleSmokeParticleComponent; }
	FORCEINLINE class UParticleSystemComponent* GetHeatSmokeParticleComp() const { return HeatSmokeParticleComponent; }

	bool bLastAttached = false;
	// alex: static mag attachment addition
	float AttachStaticDelay = 0.0f;
	virtual void AttachStatic() override;
	virtual UStaticMesh* GetAppropriateMagazineMesh();
	virtual bool GetMagazineAttachmentSockets(FName& OutMag01, FName& OutMag02);

	virtual void DetachStatic() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Magazine Weapon|Magazine|UI")
	FName MagCheckOverrideSocket = NAME_None;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Magazine Weapon|Magazine")
	bool bHasVisibleMags = true;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Magazine Weapon|Magazine")
	FName Mag_01_Socket;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Magazine Weapon|Magazine")
	FName Mag_01_Bullets_Socket;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Magazine Weapon|Magazine")
	FName Mag_01_Extra_Socket;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category="Magazine Weapon|Magazine", meta = (AllowPrivateAccess = "true"))
	class UStaticMeshComponent* Mag_01_Comp;
	UPROPERTY()
	class UStaticMeshComponent* Mag_01_Comp_TPOnly;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Magazine Weapon|Magazine", meta = (AllowPrivateAccess = "true"))
		bool bShowBulletsWhenEmpty = false;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category="Magazine Weapon|Magazine", meta = (AllowPrivateAccess = "true"))
	class UStaticMeshComponent* Mag_01_Bullets_Comp;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category="Magazine Weapon|Magazine", meta = (AllowPrivateAccess = "true"))
	class UStaticMeshComponent* Mag_01_Extra_Comp;


	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Magazine Weapon|Magazine")
		UStaticMeshComponent* Mag_ReloadInterpFix_Comp;

	FTransform MagDefaultTransform;
	bool bHiddenMagInterpFix = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Magazine Weapon|Magazine")
	UStaticMesh* Mag_01_Static;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Magazine Weapon|Magazine")
	UStaticMesh* Mag_01_FMJ_Bullets_Static;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Magazine Weapon|Magazine")
	UStaticMesh* Mag_01_HP_Bullets_Static;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Magazine Weapon|Magazine")
	UStaticMesh* Mag_01_Extra_Static;

	// only use if second mag exists
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Magazine Weapon|Magazine")
	FName Mag_02_Socket;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Magazine Weapon|Magazine")
	FName Mag_02_Bullets_Socket;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Magazine Weapon|Magazine")
	FName Mag_02_Extra_Socket;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category="Magazine Weapon|Magazine", meta = (AllowPrivateAccess = "true"))
	class UStaticMeshComponent* Mag_02_Comp;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category="Magazine Weapon|Magazine", meta = (AllowPrivateAccess = "true"))
	class UStaticMeshComponent* Mag_02_Bullets_Comp;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category="Magazine Weapon|Magazine", meta = (AllowPrivateAccess = "true"))
	class UStaticMeshComponent* Mag_02_Extra_Comp;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Magazine Weapon|Magazine")
	UStaticMesh* Mag_02_Static;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Magazine Weapon|Magazine")
	UStaticMesh* Mag_02_FMJ_Bullets_Static;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Magazine Weapon|Magazine")
	UStaticMesh* Mag_02_HP_Bullets_Static;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Magazine Weapon|Magazine")
	UStaticMesh* Mag_02_Extra_Static;


	void MagCheckDone();

	/* determines if we should use fire loop anim assets */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Magazine Weapon|Animation")
	bool bUseFireLoopAnims;

	// Animations and icons and shizz
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Magazine Weapon|Magazine|UI")
	UTexture2D* MagCheckIcon_Empty;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Magazine Weapon|Magazine|UI")
	UTexture2D* MagCheckIcon_Full;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Magazine Weapon|Magazine")
	UFMODEvent* DroppedMagazineHitEvent;

	// Notifies + New Gun Audio system
	UFUNCTION(BlueprintCallable, Category = "Event Notifies")
	void OnReloadAnimEvent(EReloadAnimEvent Type);

	UFUNCTION(BlueprintCallable, Category = "Event Notifies")
	void OnNewFireModeAnimEvent(EFireMode NewFireMode);

	// Heat implementation
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Magazine Weapon|Heat")
		float HeatPerShot = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Magazine Weapon|Heat")
		float HeatThreshold = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Magazine Weapon|Heat")
		float HeatMax = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Magazine Weapon|Heat")
		float HeatDissipation = 0.2f;

	UPROPERTY(BlueprintReadWrite, Category="Magazine Weapon|Heat")
		float CurrentHeat = 0.0f;

	// Amount of time between firing and when the smoke effect plays.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Magazine Weapon|Heat")
		float HeatMinimumTime = 0.5f;

	UPROPERTY(BlueprintReadWrite, Category="Magazine Weapon|Heat")
		float HeatTime = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category="Magazine Weapon|Heat")
		bool bHeatEffectPlayed = false;

	// How much each magazine weighs (in kilograms) when fully loaded.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Magazine Weapon|Weight")
		float MagazineWeightFull;

	// How much each magazine weighs (in kilograms) when empty.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Magazine Weapon|Weight")
		float MagazineWeightEmpty;

	// How many magazines to start with, default.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Magazine Weapon|Weight")
		int32 MagazineCountDefault = 4;

	// How many magazines to start with, minimum.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Magazine Weapon|Weight")
		int32 MagazineCountMin = 1;

	// How many magazines to start with, maximum.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Magazine Weapon|Weight")
		int32 MagazineCountMax = 1;

	// Set the magazine count - this is meant to be called while spawning the loadout
	UFUNCTION(BlueprintCallable, Category="Magazine Weapon|Weight")
	virtual void SetMagazineCount(int32 Count, TArray<FName> AmmoTypes);
	
	UFUNCTION(BlueprintCallable, Category="Magazine Weapon|Weight")
	virtual void ReplenishAmmo();

	// Get the current magazine count - this is meant to be used in the weapon wheel thing
	UFUNCTION(BlueprintPure, Category = Magazine)
	virtual int32 GetMagazineCount();

	// Get the weight of the designated number of magazines
	UFUNCTION(BlueprintCallable, Category = Weight)
		virtual float GetAmmoWeight(int32 Count);

	virtual float GetWeight() override;

	UFUNCTION(BlueprintPure)
	bool InSafeMode() const;
	UFUNCTION(BlueprintPure)
	bool InSingleMode() const;
	UFUNCTION(BlueprintPure)
	bool InBurstMode() const;
	UFUNCTION(BlueprintPure)
	bool InFullAutoMode() const;

	// Weapons with matching MagazineLabels can use each others' dropped magazines
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Magazine Weapon")
	FName MagazineLabel;

	virtual float GetRelevancyDistance_Implementation() { return 3000.0f; }
	
	UPROPERTY(BlueprintReadWrite, VisibleInstanceOnly, Category="Magazine Weapon|Cleaning")
	bool bHasBeenDisassembled = false;
	
	// temp var to manage reload swapping
	//bool bSwappingReloads;

	//bool bIsPistolWShield;


	// Sound occlusion parameters
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Magazine Weapon|Occlusion")
	float GunshotOcclusionMultiplier = 1.0f;

	// Depth to fully occlude gunshots (in cm)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Magazine Weapon|Occlusion")
    float GunshotFullOcclusionDepth = 150.0f;
	
	protected:
	UPROPERTY(EditAnywhere, Category = "Magazine Weapon|Yell Shooting", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float ShootingYellBias = 0.70f;

public:
	float GetShootingYellBias() const { return ShootingYellBias; }

	/* if pistol should we use two handed data? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Magazine Weapon|Animation")
	bool bTwoHandedPistol;

	/* if rifle should we use heavy rifle data? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Magazine Weapon|Animation")
	bool bHeavyRifle;

	/* how much to apply force to when hitting a ragdoll */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Magazine Weapon|Ragdoll")
	float RagdollImpulseMultiplier = 5000.0f; // 50k is too much

protected:
	virtual void OnAIHearingSense_Implementation(ACyberneticController* InSenseController, FAIStimulus Stimulus, AActor*& OutOverrideSensedActor) override;
};
