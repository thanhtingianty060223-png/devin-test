// Copyright Void Interactive, 2023

#pragma once

#include "GameFramework/Actor.h"

#include "DamageTypes/StunDamage.h"

#include "Animation/ReadyOrNotWeaponAnimData.h"

#include "Attachments/ScopedWeaponAttachment.h"

#include "Gameplay/ThrownItem.h"

#include "Interfaces/GetFriendlyName.h"
#include "Interfaces/UseabilityInterface.h"
#include "Interfaces/PingInterface.h"
#include "Interfaces/GatherDebugText.h"
#include "Interfaces/ScoringInterface.h"
#include "Interfaces/CanIssueCommandOn.h"

#include "Enums.h"
#include "FMODBlueprintStatics.h"
#include "Interfaces/Securable.h"

#include "BaseItem.generated.h"

DECLARE_STATS_GROUP(TEXT("BaseItem"), STATGROUP_BaseItem, STATCAT_Advanced);

UENUM(BlueprintType)
enum class EBlockingAnimationExclusion : uint8
{
	BAE_None,
	BAE_Holster,
	BAE_Draw,
	BAE_FireSelect,
	BAE_MagCheck,
	BAE_PullPin,
	BAE_Throw
};

USTRUCT(BlueprintType)
struct FRepGearAttach
{
	GENERATED_USTRUCT_BODY()

		UPROPERTY(BlueprintReadWrite, Category = Attach)
		bool bEquipped = false;

		UPROPERTY(BlueprintReadWrite, Category = Attach)
		bool bMeshVisibleTo1P = true;

	UPROPERTY(BlueprintReadWrite, Category = Attach)
		bool bMeshVisibleTo3P = true;

		UPROPERTY(BlueprintReadWrite, Category = Attach)
		USceneComponent* Attach1P;

	UPROPERTY(BlueprintReadWrite, Category = Attach)
		FName Socket1P;

	UPROPERTY(BlueprintReadWrite, Category = Attach)
		USceneComponent* Attach3P;

	UPROPERTY(BlueprintReadWrite, Category = Attach)
		FName Socket3P;

	UPROPERTY(BlueprintReadWrite, Category = Attach)
		USceneComponent* ScopeAttach;

	UPROPERTY(BlueprintReadWrite, Category = Attach)
		FName ScopeSocket;
};

USTRUCT(BlueprintType)
struct FWeightStunMultiplier
{
	GENERATED_USTRUCT_BODY()

		UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float MinimumWeight = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float MaximumWeight = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float MinimumWeightMultiplier = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float MaximumWeightMultiplier = 0.0f;
};

USTRUCT(BlueprintType)
struct FLoadout
{
	GENERATED_USTRUCT_BODY()

		UPROPERTY(EditAnywhere, Category = Equip)
		FString Name;

		UPROPERTY(EditAnywhere, Category = Equip)
		TArray<TSubclassOf<class ABaseItem>> Items;
};

// Immutable visual data which we need to expose through class defaults --eez
USTRUCT(BlueprintType)
struct FItemVisualData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Item)
	UTexture2D* ItemIcon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Item)
	UTexture2D* PremissionPlanningItemIcon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Item)
	UTexture2D* RadialItemIcon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Item)
	USkeletalMesh* ItemMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Item)
	TArray<UTexture2D*> AmmoIcons;
};

/** Structure to store the lookup of GameObjects for use in a UDataTable */
USTRUCT(BlueprintType)
struct FItemLookupTable : public FTableRowBase
{
	GENERATED_BODY()

	/*
	*	Item stuff
	*/

	// Item Name
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Info")
	FText ItemName;

	// Item Description
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Info")
	FText ItemDescription;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Info")
	TArray<EGameVersionRestriction> LockedToDLC;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Info")
	TArray<EItemCategory> ItemCategories;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Info")
	EItemClass ItemClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Info")
	EItemType ItemType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Info")
	TSoftClassPtr<class ABaseItem> BlueprintClass;

	// if this weapon uses a pistol grip (used for customization menu)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Info")
	bool bPistolGrip = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Info")
	UTexture2D* ItemIcon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Weight")
	float ItemWeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Balance")
	float HolsterPlayRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Balance")
	float DrawPlayRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Low Ready")
	bool bUseLowReady;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Low Ready")
	float PushbackRange;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Low Ready")
	float LowReadyRange;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Balance")
	float LowReadyRangeSightsModifier;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Balance")
	float LowReadyPitchThreshold;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Balance")
	float MovementSpeedMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item WeaponFOV")
	float LeanOffset = 0.0f;


	/*
	*	Weapon Stuff
	*/

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon ADS")
	float ADSZoom;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon ADS")
	float ADSZoomInSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon ADS")
	float ADSZoomOutSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Impact")
	TSubclassOf<class AImpactEffect> ImpactEffects;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Impact")
	TSubclassOf<class AImpactEffect> RicochetEffects;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Impact")
	class UParticleSystem* RicochetParticleSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Impact")
	TSubclassOf<class AImpactEffect> ExitEffects;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Weight")
	float MagazineWeightFull;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Weight")
	float MagazineWeightEmpty;

	// AI interactions
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	bool bADSCountsAsAbuse;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (ClampMin = 0.0f))
	float HesitationBoostMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Fire Mode")
	TArray<EFireMode> FireModes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Fire Mode")
	float BurstBulletCount = 3;

	// Accepted ammunition types, corresponds to rows in ammo data table, first in list as default
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Projectile")
	TArray<FName> AmmunitionTypes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Balance")
	FRuntimeFloatCurve DamageOverRange {};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Balance")
	float Damage;

	// the increase in damage that can occur from a 'more serious' shot. 1.2f = Damage *= 1.0f to 1.2f.
	// Damage of 10 with a Damage Severity Multiplier of 1.2f would cause the Min Damage to be 10 and the max damage 12
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Balance")
	float DamageSeverityMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Balance")
	float DamageSeverityChance;

	// AI ONLY
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Balance vs AI")
	float BleedoutDamageMultiplier = 0.0f;

	// AI ONLY
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Balance vs AI")
	float BleedoutDamageChance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Balance")
	TSubclassOf<UDamageType> DefaultDamageType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Balance")
	TSubclassOf<UDamageType> ArmorPiercingDamageType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Balance")
	float MagazineCountDefault;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Balance")
	float MagazineCountMin;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Balance")
	float MagazineCountMax;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Magazine Sharing")
	FName MagazineLabel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Projectile")
	float ProjectileMovementSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Projectile")
	float PenetrationDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Projectile")
	TSubclassOf<class ABulletProjectile> FakeBulletProjectile;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Projectile")
	TSubclassOf<class ABulletProjectile> BulletProjectile;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Projectile")
	TSubclassOf<class ABulletProjectile> ArmorPiercingBulletProjectile;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Balance")
	int32 MagazineSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Balance")
	float FireRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Balance", meta = (ClampMin = 0))
	float Range;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Balance", meta = (ClampMin = 0, ClampMax = 100))
	float Accuracy;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Balance AI")
	float MinFireRateAI;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Balance AI")
	float MaxFireRateAI;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Balance AI")
	int32 BulletsFiredUntilFullyAccurate;

	// Whether or not to ignore the spread value introduced by ammo
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Balance")
	bool bIgnoreAmmoTypeSpread;

	// The base spread pattern value for this weapon
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Balance")
	FRotator SpreadPattern;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Balance")
	float SpreadReturnRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Balance")
	float ADSSpreadMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Balance")
	TArray<FRotator> RecoilPattern;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Balance")
	float RecoilReturnRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Balance")
	float RecoilInterpSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Balance")
	float ADSRecoilMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Breaching")
	float LockIntegrityMinDamage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Breaching")
	float LockIntegrityMaxDamage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Projectile")
	float BulletDrag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Projectile")
	bool bHitScan = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Projectile")
	bool bSpawnTracer = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Projectile")
	bool bNoSpawnTracerForFiringPlayer = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Projectile")
	int32 SpawnProjectileCount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Projectile")
	float Wobble;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Projectile")
	float InitialWobbleDelay;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Recoil")
	TSubclassOf<ULegacyCameraShake> FireCameraShake;

	//UPROPERTY(EditAnywhere, Category = "Weapon Recoil")
	//uint8 bUseRecoilCurve : 1;
	//
	//UPROPERTY(EditAnywhere, Category = "Weapon Recoil", meta = (EditCondition = "bUseRecoilCurve"))
	//FRuntimeCurveLinearColor RecoilCurve{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Recoil")
	float FirstShotRecoil;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Recoil")
	float FirstShotSpread;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Recoil")
	float FirstShotResetTime;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Recoil")
	//FRotator LastShotRecoil;
	//
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Recoil")
	//float LastShotResetTime;

	// The speed of the recoil return when firing stops
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Recoil", meta = (ClampMin = 0.0f))
	float RecoilReturnInterpSpeed = 2.5f;

	// The amount of recoil to return by the total accumulated recoil (0.0 to 1.0)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Recoil", meta = (ClampMin = 0.0f))
	float RecoilReturnPercentage = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Recoil")
	float VelocitySpreadMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Recoil")
	float VelocityRecoilMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Recoil", DisplayName = "Recoil Multiplier Pitch (Additive)")
	float RecoilMultiplierPitch;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Recoil", DisplayName = "Recoil Multiplier Yaw (Additive)")
	float RecoilMultiplierYaw;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Balance")
	float RefireDelay;

	// Weapon wheel stuff
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Wheel")
	TArray<TSubclassOf<class UWeaponWheel_ItemStat_Base>> ItemStats;


	/*
	*	Attachment Stuff
	*/

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Attachments")
	int32 AttachmentPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Attachments")
	bool bAcceptsScopeAttachments;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Attachments", meta = (EditCondition = "bAcceptsScopeAttachments"))
	TArray<TSubclassOf<class UScopedWeaponAttachment>> AvailableScopeAttachments;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Attachments")
	bool bAcceptsMuzzleAttachments;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Attachments", meta = (EditCondition = "bAcceptsMuzzleAttachments"))
	TArray<TSubclassOf<class UWeaponAttachment>> AvailableMuzzleAttachments;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Attachments")
	bool bAcceptsUnderbarrelAttachments;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Attachments", meta = (EditCondition = "bAcceptsUnderbarrelAttachments"))
	TArray<TSubclassOf<class UWeaponAttachment>> AvailableUnderbarrelAttachments;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Attachments")
	bool bAcceptsOverbarrelAttachments;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Attachments", meta = (EditCondition = "bAcceptsOverbarrelAttachments"))
	TArray<TSubclassOf<class UWeaponAttachment>> AvailableOverbarrelAttachments;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Attachments")
	bool bAcceptsStockAttachments;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Attachments", meta = (EditCondition = "bAcceptsStockAttachments"))
	TArray<TSubclassOf<class UWeaponAttachment>> AvailableStockAttachments;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Attachments")
	bool bAcceptsGripAttachments;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Attachments", meta = (EditCondition = "bAcceptsGripAttachments"))
	TArray<TSubclassOf<class UWeaponAttachment>> AvailableGripAttachments;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Attachments")
	bool bAcceptsIlluminatorAttachments;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Attachments", meta = (EditCondition = "bAcceptsIlluminatorAttachments"))
	TArray<TSubclassOf<class UWeaponAttachment>> AvailableIlluminatorAttachments;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Attachments")
	bool bAcceptsAmmunitionAttachments;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Attachments", meta = (EditCondition = "bAcceptsAmmunitionAttachments"))
	TArray<TSubclassOf<class UWeaponAttachment>> AvailableAmmunitionAttachments;

	/*
	*	Recoil Stuff
	*/

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Procedural Recoil")
	bool bCalculateProcRecoil;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Procedural Recoil")
	float RecoilDampStrength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Procedural Recoil")
	float RecoilFireTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Procedural Recoil")
	float RecoilFireStrength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Procedural Recoil")
	float RecoilFireStrengthFirst;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Procedural Recoil")
	float RecoilAngleStrength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Procedural Recoil")
	float RecoilRandomness;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Procedural Recoil")
	float RecoilADSModfier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Procedural Recoil")
	float RecoilAngleADSModifier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Procedural Recoil")
	FRotator RecoilRotationBuildup;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Procedural Recoil")
	FVector RecoilPositionBuildup;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Procedural Recoil")
	float RecoilBuildupADSMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Procedural Recoil")
	bool RecoilHasBuildup;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Procedural Recoil")
	float RecoilBuildupDampStrength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Procedural Aim")
	float FreeAimLimit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Procedural Aim")
	float FreeAimLimitADS;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Procedural Aim")
	float LazySpringStrength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Procedural Aim")
	float LazySpringStrengthADS;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Procedural Aim")
	float FPADSMotionWeight;

	// added for more control:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Procedural Aim")
	float FreeAimInterpSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Procedural Aim")
	float FreeAimInterpADSModifier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Procedural Aim")
	float FreeAimInterpHipModifier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Procedural Aim")
	float FreeAimSlowMoveModifier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Procedural Aim")
	float FreeAimSlowMoveTolerance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Meshspace")
	bool bDisableMeshspaceMovement;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Meshspace")
	FTransform MeshspaceTransform_Default;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Meshspace")
	FTransform MeshspaceTransform_Aiming;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Meshspace")
	FTransform MeshspaceTransform_Back;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Meshspace")
	FVector MovementSpeedScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Meshspace")
	FRotator MovementSpeedRotationScalePitchYawRoll;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Meshspace")
	float MeshSpaceAimInterp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Meshspace")
	FTransform MeshSpaceTransform_OnDraw;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Meshspace")
	float OnDrawMeshspaceInterp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Meshspace")
	FTransform MeshspaceTransform_OnHolster;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Meshspace")
	float OnHolsterMeshspaceInterp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Meshspace")
	float InertiaDragAimRotation = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Meshspace")
	float InertiaDragAimLocation = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Meshspace")
	float InertiaDragStrafeRotation = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Meshspace")
	float InertiaDragStrafeLocation = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Sockets")
	FName BodySocket;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Sockets")
	FName HandsSocket;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Bob")
	float CameraBobScaleH;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Bob")
	float CameraBobScaleV;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Bob")
	float CameraBobSpeedScaleH;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Bob")
	float CameraBobSpeedScaleV;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Bob")
	float CameraBobAmplitudeBaseSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Bob")
	float CameraBobIntensitySprintScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Bob")
	float CameraBobAmplitudeWalkScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Bob")
	float CameraBobAmplitudeSprintScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Bob")
	float WeaponBobScaleH;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Bob")
	float WeaponBobScaleV;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Bob")
	float WeaponBobScaleInjured;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Bob")
	float WeaponBobSpeedScaleH;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Bob")
	float WeaponBobSpeedScaleV;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Bob")
	float WeaponBobSpeedScaleInjured;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Bob")
	float WeaponBobCrouchModifier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Bob")
	float WeaponBobADSModifier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Bob Rotation")
	float WeaponBobRotPitchScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Bob Rotation")
	float WeaponBobRotRollScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Bob Rotation")
	float WeaponBobRotPitchSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Bob Rotation")
	float WeaponBobRotRollSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Bob Rotation")
	float WeaponBobRotCrouchModifier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Bob Rotation")
	float WeaponBobRotADSModifier;

	FItemLookupTable()
	{
		ItemWeight = 0;
		ItemIcon = nullptr;
		HolsterPlayRate = 1.0f;
		DrawPlayRate = 1.0f;
		bUseLowReady = false;
		PushbackRange = 0;
		LowReadyRange = 0;
		LowReadyRangeSightsModifier = 0;
		LowReadyPitchThreshold = 0;
		MovementSpeedMultiplier = 1.0f;
		ADSZoom = 0;
		ADSZoomInSpeed = 0;
		ADSZoomOutSpeed = 0;
		ImpactEffects = nullptr;
		RicochetEffects = nullptr;
		ExitEffects = nullptr;
		BurstBulletCount = 3;
		Damage = 0.0f;
		DamageSeverityMultiplier = 1.0f;
		ProjectileMovementSpeed = 0.0f;
		PenetrationDistance = 0.0f;
		BulletProjectile = nullptr;
		ArmorPiercingBulletProjectile = nullptr;
		MagazineSize = 0;
		FireRate = 0.0f;
		bIgnoreAmmoTypeSpread = false;
		SpreadPattern = FRotator::ZeroRotator;
		SpreadReturnRate = 0;
		ADSSpreadMultiplier = 0;
		RecoilPattern = TArray<FRotator>();
		RecoilReturnRate = 0;
		ADSRecoilMultiplier = 0;
		SpawnProjectileCount = 0;
		FirstShotRecoil = 0;
		FirstShotSpread = 0;
		FirstShotResetTime = 0;
		VelocitySpreadMultiplier = 0;
		VelocityRecoilMultiplier = 0;

		ItemClass = EItemClass::IC_NoClass;
		bADSCountsAsAbuse = false;
		DamageSeverityChance = 0;
		Range = 0;
		Accuracy = 0;
		MinFireRateAI = 0;
		MaxFireRateAI = 0;
		BulletsFiredUntilFullyAccurate = 10;
		RecoilInterpSpeed = 0;
		Wobble = 0;
		InitialWobbleDelay = 0;

		RefireDelay = 0;
		bAcceptsScopeAttachments = false;
		AvailableScopeAttachments = TArray<TSubclassOf<class UScopedWeaponAttachment>>();
		bAcceptsMuzzleAttachments = false;
		AvailableMuzzleAttachments = TArray<TSubclassOf<class UWeaponAttachment>>();
		bAcceptsUnderbarrelAttachments = false;
		AvailableUnderbarrelAttachments = TArray<TSubclassOf<class UWeaponAttachment>>();
		bAcceptsOverbarrelAttachments = false;
		AvailableOverbarrelAttachments = TArray<TSubclassOf<class UWeaponAttachment>>();
		bAcceptsStockAttachments = false;
		AvailableStockAttachments = TArray<TSubclassOf<class UWeaponAttachment>>();
		bAcceptsGripAttachments = false;
		AvailableGripAttachments = TArray<TSubclassOf<class UWeaponAttachment>>();
		bAcceptsIlluminatorAttachments = false;
		AvailableIlluminatorAttachments = TArray<TSubclassOf<class UWeaponAttachment>>();
		bAcceptsAmmunitionAttachments = false;
		AvailableAmmunitionAttachments = TArray<TSubclassOf<class UWeaponAttachment>>();
		bCalculateProcRecoil = false;
		RecoilDampStrength = 0;
		RecoilFireTime = 0;
		RecoilFireStrength = 0;
		RecoilFireStrengthFirst = 0;
		RecoilAngleStrength = 0;
		RecoilRandomness = 0;
		RecoilADSModfier = 0;
		RecoilAngleADSModifier = 0;
		RecoilRotationBuildup = FRotator::ZeroRotator;
		RecoilPositionBuildup = FVector::ZeroVector;
		RecoilBuildupADSMultiplier = 0;
		RecoilHasBuildup = false;
		RecoilBuildupDampStrength = 0;

		RecoilInterpSpeed = 0;
		BulletDrag = 0;
		DefaultDamageType = 0;
		FireCameraShake = nullptr;
		LockIntegrityMinDamage = 0;
		LockIntegrityMaxDamage = 0;
		AttachmentPoints = 0;
		MagazineWeightFull = 0;
		MagazineWeightEmpty = 0;
		MagazineCountDefault = 0;
		MagazineCountMax = 0;
		MagazineCountMin = 0;
		MagazineLabel = "";
		FreeAimLimit = 0;
		FreeAimLimitADS = 0;
		LazySpringStrength = 0;
		LazySpringStrengthADS = 0;
		FPADSMotionWeight = 0;

		//
		FreeAimInterpSpeed = 20.0f;
		FreeAimInterpADSModifier = 1.0f;
		FreeAimInterpHipModifier = 1.0f;
		FreeAimSlowMoveModifier = 0.3f;
		FreeAimSlowMoveTolerance = 360.f;
		//

		bDisableMeshspaceMovement = false;
		MeshspaceTransform_Default = FTransform();
		MeshspaceTransform_Aiming = FTransform();
		MeshspaceTransform_Back = FTransform();
		MeshSpaceAimInterp = 0;
		MeshSpaceTransform_OnDraw = FTransform();
		OnDrawMeshspaceInterp = 0;
		MeshspaceTransform_OnHolster = FTransform();
		OnHolsterMeshspaceInterp = 0;
		BodySocket = "";
		HandsSocket = "";

		CameraBobScaleH = 2.3f;
		CameraBobScaleV = 5.0f;
		CameraBobSpeedScaleH = 0.7f;
		CameraBobSpeedScaleV = 1.4f;

		CameraBobAmplitudeBaseSpeed = 0.2f;
		CameraBobIntensitySprintScale = 1.6f;
		CameraBobAmplitudeWalkScale = 0.4f;
		CameraBobAmplitudeSprintScale = 1.2f;

		/* weapon bob related */
		WeaponBobScaleH = 0.25f;
		WeaponBobScaleV = 0.5f;
		WeaponBobScaleInjured = 1.0f;
		WeaponBobSpeedScaleH = 0.7f;
		WeaponBobSpeedScaleV = 1.4f;
		WeaponBobSpeedScaleInjured = 1.0f;

		WeaponBobCrouchModifier = 1.0f;
		WeaponBobADSModifier = 1.0f;

		WeaponBobRotPitchScale = 1.5f;
		WeaponBobRotRollScale = 1.5f;
		WeaponBobRotPitchSpeed = 1.4f;
		WeaponBobRotRollSpeed = 0.7f;
		WeaponBobRotCrouchModifier = 1.0f;
		WeaponBobRotADSModifier = 1.0f;
	}
};

USTRUCT()
struct FMeshFOVMaterials
{
	GENERATED_BODY()
	
	UPROPERTY()
	TArray<UMaterialInstanceDynamic*> FovMats;
};

UENUM(meta=(Bitflags, UseEnumValuesAsMaskValuesInEditor="true"))
enum class ELoadoutCategory : uint8
{
	None = 0 UMETA(Hidden),
	Primary = 1,
	Secondary = 2,
	LongTactical = 8,
	TacticalDevice = 16
};

UCLASS(Abstract)
class READYORNOT_API ABaseItem : public AActor,
	public IGetFriendlyName, public ICanIssueCommandOn, public IUseabilityInterface,
	public IPingInterface, public IGatherDebugInterface, public IScoringInterface,
	public ISecurable
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item")
	FText ItemName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item")
	TSoftObjectPtr<UTexture2D> ItemIcon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item", meta=(MultiLine=true))
	FText ItemDescription;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Config, Category = "Item")
	EItemClass ItemClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	EItemType ItemType;

	UPROPERTY(EditAnywhere, Category="Item", meta=(Bitmask, BitmaskEnum="ELoadoutCategory"))
	uint32 CategoryFlags;

	UPROPERTY(EditAnywhere, Category="Item")
	int32 LoadoutPriority = 1;

	UPROPERTY(EditAnywhere, Category="Item")
	bool bShowInLoadout = true;

	UPROPERTY(EditAnywhere, Category="Item")
	FName CustomizationTag;
	
	// Number of items to give per slot, only applies to devices
	UPROPERTY(EditAnywhere, Category="Item")
	int32 ItemsPerSlot = 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FName LookupTableIdx = "Default";
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Balance", meta=(Units="Kilograms"))
	float ItemWeight;

	UPROPERTY(EditAnywhere, Category = "Item|Balance")
	float MovementSpeedMultiplier = 1.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Balance")
	float HolsterPlayRate = 1.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Balance")
	float DrawPlayRate = 1.0f;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Item|Balance")
	float LeanOffset = 0.0f;
	
	// Whether we should allow kicking while this item is equipped
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Item|Balance")
	bool bDisallowKicking = false;

	// Disable to disallow freelooking with this item equipped
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Item|Balance")
	bool bFreelookEnabled = true;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Item|Low Ready")
	bool bUseLowReady = true;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Item|Low Ready")
	float PushbackRange = 100.0f;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Item|Low Ready")
	float LowReadyRange = 80.0f;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Item|Low Ready")
	float LowReadyRangeSightsModifier = 1.15f;

	UPROPERTY(EditAnywhere, Category = "Item|Tick")
	bool bDisableTickWhenNotEquipped;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TArray<EGameVersionRestriction> LockedToDLC;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|UI")
	FItemVisualData Visuals;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item|UI")
	UTexture2D* HudOutline;
	
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Item|Camera Bob")
	float CameraBobScaleH;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Item|Camera Bob")
	float CameraBobScaleV;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Item|Camera Bob")
	float CameraBobSpeedScaleH;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Item|Camera Bob")
	float CameraBobSpeedScaleV;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Item|Camera Bob")
	float CameraBobAmplitudeBaseSpeed;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Item|Camera Bob")
	float CameraBobIntensitySprintScale;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Item|Camera Bob")
	float CameraBobAmplitudeWalkScale;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Item|Camera Bob")
	float CameraBobAmplitudeSprintScale;
	
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Item|Weapon Bob")
	float WeaponBobScaleH;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Item|Weapon Bob")
	float WeaponBobScaleV;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Item|Weapon Bob")
	float WeaponBobScaleInjured;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Item|Weapon Bob")
	float WeaponBobSpeedScaleH;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Item|Weapon Bob")
	float WeaponBobSpeedScaleV;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Item|Weapon Bob")
	float WeaponBobSpeedScaleInjured;
	
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Item|Weapon Bob")
	float WeaponBobCrouchModifier;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Item|Weapon Bob")
	float WeaponBobADSModifier;
	
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Item|Weapon Bob|Rotation")
	float WeaponBobRotPitchScale;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Item|Weapon Bob|Rotation")
	float WeaponBobRotRollScale;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Item|Weapon Bob|Rotation")
	float WeaponBobRotPitchSpeed;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Item|Weapon Bob|Rotation")
	float WeaponBobRotRollSpeed;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Item|Weapon Bob|Rotation")
	float WeaponBobRotCrouchModifier;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Item|Weapon Bob|Rotation")
	float WeaponBobRotADSModifier;
	
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Root, meta = (AllowPrivateAccess = "true"))
	class USceneComponent* SceneComp;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	class USkeletalMeshComponent* ItemMesh;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	class UBoxComponent* InteractionBox;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Audio, meta = (AllowPrivateAccess = "true"))
	class UFMODAudioComponent* FMODAudioComp;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Interaction, meta = (AllowPrivateAccess = "true"))
	class UInteractableComponent* InteractableComponent;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Scoring, meta = (AllowPrivateAccess = "true"))
	class UScoringComponent* ScoringComponent;
	
	UFUNCTION(BlueprintCallable, Category = "Audio")
		void PlayFMODAudio(UFMODEvent* Event);

	void SetMeshUpdateRateParameters(FAnimUpdateRateParameters* AnimUpdateRateParams);

	FVector GetItemLocation() const;
	FVector GetItemRelativeLocation() const;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	
	ABaseItem();
	
	UFUNCTION(BlueprintCallable)
	virtual void SpawnThrownItemAtTransform(const FTransform& Transform, const FVector& ThrowDirection, const FVector& ThrowLocation = FVector::ZeroVector);

	UPROPERTY(EditAnywhere)
	UFMODEvent* PhysicsImpact;

	int32 PhysicsImpactAudioCount = 0;
	
	UFUNCTION()
	virtual void OnPhysicsImpact(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	void PlayHitImpactSound(const FHitResult& Hit, float Impulse);
	
	int32 HitImpactCount = 0;
	FFMODEventInstance HitEventInstance;
	
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_ApplyPointDamage(AActor* DamagedActor, float BaseDamage, FVector const& HitFromDirection, FHitResult const& HitInfo, AController* EventInstigator, AActor* DamageCauser, TSubclassOf<UDamageType> DamageTypeClass);
	void Server_ApplyPointDamage_Implementation(AActor* DamagedActor, float BaseDamage, FVector const& HitFromDirection, FHitResult const& HitInfo, AController* EventInstigator, AActor* DamageCauser, TSubclassOf<UDamageType> DamageTypeClass);
	bool Server_ApplyPointDamage_Validate(AActor* DamagedActor, float BaseDamage, FVector const& HitFromDirection, FHitResult const& HitInfo, AController* EventInstigator, AActor* DamageCauser, TSubclassOf<UDamageType> DamageTypeClass) { return true; }
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Thrown Item")
	TSubclassOf<AThrownItem> ThrownItemClass = nullptr;

	// hide a frame of gun being in middle of screen for clients
	bool bHasSetOriginalTransform = false;
	FTransform OriginalTransform;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void FellOutOfWorld(const UDamageType& dmgType) override;

	UFUNCTION(BlueprintImplementableEvent, Category = Events)
	void SetupBaseEvents();
	
	virtual void Tick(float DeltaTime) override;

	void SetOriginalTransform();

	UFUNCTION(Category = "Equip")
	virtual bool IsCollidesWhileNotEquipped() const { return false; } 
	
	UFUNCTION(BlueprintPure, Category = "Equip")
	virtual bool CanEquip(AReadyOrNotCharacter* ToCharacter) const;

	UFUNCTION(BlueprintNativeEvent, Category = "HUD")
			bool CanShowActionSlot1(class AReadyOrNotCharacter* PC);
	virtual bool CanShowActionSlot1_Implementation(class AReadyOrNotCharacter* PC);

	// Called on both client and server when stunned
	UFUNCTION(BlueprintNativeEvent, Category = "Equip")
		void StunnedWhileEquipped();
	virtual void StunnedWhileEquipped_Implementation() {}

	// Called on both client and server when stun has ended
	UFUNCTION(BlueprintNativeEvent, Category = "Equip")
		void EndStunWhileEquipped();
	virtual void EndStunWhileEquipped_Implementation() {}

	// Called on the first tick when we are stunned (and the person holding this is the local player)
	UFUNCTION(BlueprintNativeEvent, Category = "Equip")
		void StunTick(EStunType StunType);
	virtual void StunTick_Implementation(EStunType StunType);

	/* for playing the stun ending motion */
	UFUNCTION(BlueprintNativeEvent, Category = "Equip")
		void LastStunTick(EStunType StunType);
	virtual void LastStunTick_Implementation(EStunType StunType);

	// Whether we are playing a stun start animation
	UFUNCTION(BlueprintPure, Category = "Equip")
		bool IsPlayingStunnedAnimation();

	// Whether we are playing a stun end animation
	UFUNCTION(BlueprintPure, Category = "Equip")
		bool IsPlayingStunnedEndAnimation();

	void StunnedAnimationEnd(UAnimMontage* animMontage, bool bInterrupted);
		
	//GetFriendlyName interface
	virtual FString GetFriendlyName_Implementation() override;
	virtual UTexture2D* GetFriendlyIcon_Implementation() override;

#ifdef WITH_EDITOR
	virtual bool IsSelectable() const { return bAllowSelectable; };
	bool bAllowSelectable = true;

	virtual void ClearCrossLevelReferences() {
		if (bDontSave) { Destroy(); }
	};
	bool bDontSave = false;

#endif

	// Call this on an object instance instead of getting ItemWeight for the weight of the item + all ammunition + all attachments
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Weight)
	virtual float GetWeight();

	bool bDrawnBefore;
	bool bNoAttachmentRep = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Draw")
	bool bAttachOnDrawComplete = false;

	UPROPERTY(Replicated)
	USkeletalMesh* Rep_CustomItemMeshFromAttachment = nullptr;
	USkeletalMesh* GetAppropriateSkeletalMesh();

	UFUNCTION(BlueprintCallable, Category = "Attachments")
	virtual void AttachStatic() {};

	UFUNCTION(BlueprintCallable, Category = "Attachments")
	virtual void DetachStatic() {};

	UPROPERTY(EditAnywhere, Category="Item")
	TSoftObjectPtr<class UCustomizationSkin> DefaultSkin;
	
	UPROPERTY(ReplicatedUsing=OnRep_Skin)
	class UCustomizationSkin* Skin;
	
	UFUNCTION()
	void OnRep_Skin();
	
	UFUNCTION(BlueprintNativeEvent)
	void OnOwnerPossessed();
	virtual void OnOwnerPossessed_Implementation() {}

	bool ItemPreviousHidden = false;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Item")
	bool bInInventory = false;


	UPROPERTY(EditAnywhere)
	bool bShouldTickAnimBPWhenNotEquipped = false;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Picture in Picture")
	virtual bool ShouldHideInPictureInPictureScopes() { return true; }

	UPROPERTY(Replicated)
	FVector Server_ReplicatedPhysicsLocation;

	UPROPERTY(BlueprintReadWrite, Replicated, Category = "ABaseItem")
	FVector TargetWorldScale;

	UPROPERTY(BlueprintReadWrite, Replicated, Category = "ABaseItem")
	float TargetWorldScaleInterpSpeed;
	
	bool bInterpToTargetScale;
	
	UFUNCTION(BlueprintCallable, Category = "ABaseItem")
	void InterpToTargetScale(FVector NewScale, float InterpSpeed);

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Replicated, Category = "Item|Evidence")
		bool bStartAsEvidence = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated,  Category = "Item|Evidence")
	bool bIsEvidence;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated,  Category = "Item|Evidence")
	bool bMarkAsEvidenceWhenNoOwner = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category = "Weapon Clearing")
	bool bIsClearable = false;

	UPROPERTY(BlueprintReadOnly, Replicated,  Category = "Weapon Clearing")
	bool bHasBeenCleared = false;

	bool IsEvidence() const;

	float TimeSinceLastNavMeshCheck = 0.0f;

	// Block ANY damage from these damage types
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Armour)
		TArray<TSubclassOf<class UDamageType>> BlockAnyDamageFrom;

	// Block direct hits 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Armour)
		TArray<TSubclassOf<class UDamageType>> BlockDirectHitsFrom;

	// If this is enabled, taser hits are completed negated
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Armour)
		bool bTaserDamageBlocked = false;

	// Use as a multiplier for stun damage - anywhere we get hit from it
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Armour)
		TMap<TSubclassOf<class UDamageType>, float> MultiplyStunDamageFrom;

	// How much the weight of this thing affects our stun damage multiplier
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Armour)
		TMap<TSubclassOf<class UDamageType>, FWeightStunMultiplier> MultiplyStunDamageByWeight;
	

	UPROPERTY()
	TArray<UMaterialInstanceDynamic*> Dynamic1PMaterialInstances;

	float LastWeaponFOVAspectRatio = 0.0f;
	bool bEnabledWeaponFovShader = false;
	bool bDisableWeaponFOV_FromNotify = false;
	virtual bool ShouldEnableWeaponFovShader() const;
	virtual bool ShouldAttachToOwner() const;
	void EnableWeaponFovShader();
	void TickWeaponFovShader(float DeltaTime);
	void DisableWeaponFovShader();
	void BlendOutWeaponFovShader();
	void BlendInWeaponFovShader();
	UPROPERTY()
	TArray<UMaterialInstanceDynamic*> DynamicWeaponFovMats;
	UPROPERTY()
	TMap<USkeletalMesh*, FMeshFOVMaterials> SkeletalMeshToFOVMats;
	UPROPERTY()
	TMap<UStaticMesh*, FMeshFOVMaterials> StaticMeshToFOVMats;

	void InitializeFOVMaterials();
	
	float DesiredDynamicWeaponFoVBlendEffectAmount = 1.0f;
	float CurrentDynamicWeaponFoVBlendEffectAmount = 1.0f;
	
	//UPROPERTY()
	//USkeletalMesh* ItemSkeletalMeshNonComp;

	UPROPERTY(BlueprintReadOnly, Category = Mesh)
	TArray<UMaterialInstanceDynamic*> FP_SkinMaterials;

	UPROPERTY(BlueprintReadOnly, Category = Mesh)
	TArray<UMaterialInstanceDynamic*> TP_SkinMaterials;

	UFUNCTION(Server, Reliable, WithValidation, Category = Mesh)
	void Server_SetMasterPoseComponent(class USkeletalMeshComponent* Mesh);
	virtual void Server_SetMasterPoseComponent_Implementation(class USkeletalMeshComponent* Mesh);
	virtual bool Server_SetMasterPoseComponent_Validate(class USkeletalMeshComponent* Mesh) { return true; }

	UFUNCTION()
	void OnRep_MasterPoseComponent();

	UPROPERTY(ReplicatedUsing = OnRep_MasterPoseComponent)
		USkeletalMeshComponent* MasterPoseRep;

	UPROPERTY(EditAnywhere, Category="Item|Other")
	bool bShowStaticMeshOnBody = false;

	UPROPERTY(BlueprintReadWrite, Category = "Weapon Wheel")
	FName WeaponWheelCategoryName = "None";

	virtual bool ConsumeMouseMovement(FRotator RotateVector);

	virtual bool ConsumeIncrementalUse(float UseAmount);

	virtual bool ConsumeLeanInput(float leanAmount);

	virtual bool ConsumeMovementForward(float Val);

	virtual bool ConsumeMovementRight(float Val);

	virtual bool ConsumeSprintInput();

	virtual bool ConsumeCrouchInput();

	virtual bool ConsumeJumpInput();

	// If something is depleted, it will show up with an X over it in the devices menu (this only really concerns a few things)
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = Item)
		virtual bool IsDepleted() const { return false; }

	UFUNCTION(BlueprintImplementableEvent, Category = Events)
		void OnItemUsed();

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemUseCompleted, ABaseItem*, Item);
	UPROPERTY(BlueprintAssignable, Category = "Multitool")
	FOnItemUseCompleted OnItemUseCompleted;

	UFUNCTION(BlueprintCallable, Category = Events)
		virtual void OnItemUseComplete();

	virtual void SetItemVisibility(bool bNewVisibility);
	void SetItemHiddenInSceneCapture(bool bNewHiddenInSceneCapture);
	
	//UPROPERTY(Replicated, BlueprintReadWrite, Category = Visibility)
		//bool bForceInvisible = false;

	//UPROPERTY(BlueprintReadWrite, Category = Visibility)
		//bool bForcedInvisible = false;

	UFUNCTION(BlueprintCallable, Category = Networking)
		virtual void OnItemPrimaryUse();

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemPrimaryUse, ABaseItem*, Item);
	FOnItemPrimaryUse OnItemPrimaryUseStart;

	UFUNCTION(BlueprintImplementableEvent, Category = Events)
		void OnItemEndUse();

	UFUNCTION(BlueprintCallable, Category = Networking)
		virtual void OnItemPrimaryUseEnd();

		virtual void OnItemSecondaryUsed();

		virtual void OnItemEndSecondaryUse();

	UFUNCTION(BlueprintCallable, Category = Networking)
		virtual void NextFireMode() {}

	FTimerHandle EquipNextWeapon_Handle;

	// returns true if the item is currently being reloaded, ie we should call the CancelReload() function
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = Reload)
		virtual bool IsCurrentlyReloading() const { return false; }

	// the event that occurs when we are trying to reload while IsCurrentlyReloading() returns true
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = Reload)
		void CancelCurrentReloadAction(bool bCancel);
	virtual void CancelCurrentReloadAction_Implementation(bool bCancel) {}

	// if true, we can reload the weapon if the mag index is not the same. Use for special weapons, like the taser
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Reload")
		bool bCanReloadSameMagazine = false;

	UPROPERTY(BlueprintReadWrite, Category = Pouches)
		UAnimMontage* LastReloadAnim_3P;


	UPROPERTY(EditAnywhere, Category = Balance)
		float QuickLeanMultiplier = 300.0f;
	UPROPERTY(EditAnywhere, Category = Balance)
		float FreeLeanMultiplier = 150.0f;
	
	UFUNCTION(Client, Reliable, BlueprintCallable, Category = Networking)
		void Client_OnItemPickedUp(AActor* NewOwner, bool bEquipped);
	virtual void Client_OnItemPickedUp_Implementation(AActor* NewOwner, bool bEquipped);
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEvidenceCollected);
	UPROPERTY(BlueprintAssignable)
	FOnEvidenceCollected OnEvidenceCollected;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemAttachmentsChanged, EItemAttachment, AttachmentChanged);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item|Other")
		EWeaponType WeaponType = EWeaponType::WT_Special;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ABaseItem")
	bool bDisableMeshspaceMovement;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ABaseItem")
		FTransform MeshspaceTransform_Default;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ABaseItem")
		FTransform MeshspaceTransform_Aiming;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ABaseItem")
		FTransform MeshspaceTransform_Back;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Meshspace")
	float InertiaDragAimRotation = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Meshspace")
	float InertiaDragAimLocation = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Meshspace")
	float InertiaDragStrafeRotation = 0.5f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Meshspace")
	float InertiaDragStrafeLocation = 0.5f;

	virtual void GetMovementSpeedScale(FVector& OutMovementSpeedScale, FRotator& OutMovementSpeedRotationScalePitchYawRoll);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Meshspace")
	FVector MovementSpeedScale;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Meshspace")
    FRotator MovementSpeedRotationScalePitchYawRoll;

	virtual bool GetMeshspaceTransform(FTransform& Default, FTransform& Aiming, FTransform& Back);

	UPROPERTY(EditAnywhere, BlueprintReadwrite, Category = "ABaseItem")
		float MeshSpaceAimInterp = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ABaseItem")
		FTransform MeshspaceTransform_OnDraw;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ABaseItem")
		float OnDrawMeshspaceInterp = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ABaseItem")
		FTransform MeshspaceTransform_OnHolster;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ABaseItem")
		float OnHolsterMeshspaceInterp = 10.0f;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "ABaseItem")
	FName BodySocket;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "ABaseItem")
	FName HandsSocket;

	UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadWrite, Category = "ABaseItem")
	int32 AnimationIndex1P;
	int32 DefaultAnimationIndex1P;

	void ResetAnimationIndex();

	UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadWrite, Category = "ABaseItem")
	int32 AnimationIndex3P;
	int32 DefaultAnimationIndex3P;

	// Will be dropped if the item is holstered.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ABaseItem")
		bool bDeployable = false; 

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ABaseItem")
		bool bShouldEquipToHands = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ABaseItem")
		TArray<EItemCategory> ItemCategories;

	UFUNCTION(BlueprintCallable, Category = "ABaseItem")
		bool ContainsItemCategory(EItemCategory TestCategory) const;

	UFUNCTION(BlueprintCallable, Category = Gameplay)
	FName GetEquipSocket();

	UFUNCTION(BlueprintCallable, Category = "Anim")
	bool IsMontagePlaying(UAnimMontage* Montage, bool bIncludeFP = true, bool bIncludeTP = false);

	UFUNCTION()
	void PlayFPMontage(UAnimMontage* NewMontage, float PlayRate = 1.0f);

	UFUNCTION()
	void StopFPMontage(class UAnimMontage* AnimMontage = nullptr);

	void PlayLocalFPMontage(UAnimMontage* NewMontage, float PlayRate = 1.0f);

	/** Return current playing Montage **/
	UFUNCTION(BlueprintCallable, Category = Animation)
	class UAnimMontage* GetCurrentFPMontage();

	/** Return current playing Montage **/
	UFUNCTION(BlueprintCallable, Category = Animation)
	class UAnimMontage* GetCurrentTPMontage();

	UFUNCTION(Client, Reliable, BlueprintCallable, Category = Animation)
		void Client_PlayFPMontage(UAnimMontage* NewMontage, float PlayRate = 1.0f);
	virtual void Client_PlayFPMontage_Implementation(UAnimMontage* NewMontage, float PlayRate = 1.0f);

	UFUNCTION(BlueprintPure)
	bool IsPlayingHolster() const;
	
	UFUNCTION(BlueprintPure)
	bool IsPlayingDraw() const;
	
	UFUNCTION(BlueprintCallable)
	virtual bool PlayDraw(bool bDrawFirst);

	UFUNCTION(Client, Reliable)
	virtual void ClientPlayDraw(bool bDrawFirst);
	
	FTimerHandle TH_HolsterComplete;
	virtual void OnHolsterComplete();

	FTimerHandle TH_DrawComplete;
	virtual void OnDrawComplete();

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemDrawComplete, ABaseItem*, Item);
	UPROPERTY(BlueprintAssignable)
	FOnItemDrawComplete OnItemDrawComplete;

	virtual bool PlayHolster();
	float UpdateAttachmentDelayOnDraw = 0.0f;

	UFUNCTION()
	void PlayTPMontage(UAnimMontage* NewMontage, float PlayRate = 1.0f);

	void PlayLocalTPMontage(UAnimMontage* NewMontage, float PlayRate = 1.0f);

	UFUNCTION(Client, Reliable)
	void Client_PlayItemAnimation(const FWeaponAnim& InWeaponAnim, bool bRestartIfAlreadyPlaying = true, bool bOnlyLocal = false, bool bOnlyTP = false);
	void Client_PlayItemAnimation_Implementation(const FWeaponAnim& InWeaponAnim, bool bRestartIfAlreadyPlaying = true, bool bOnlyLocal = false, bool bOnlyTP = false) { PlayItemAnimation(InWeaponAnim, bRestartIfAlreadyPlaying, bOnlyLocal, bOnlyTP); }
	
	UFUNCTION(Client, Reliable)
	void Client_StopItemAnimation(const FWeaponAnim& InWeaponAnim, bool bOnlyTP = false);
	void Client_StopItemAnimation_Implementation(const FWeaponAnim& InWeaponAnim, bool bOnlyTP = false) { StopItemAnimation(InWeaponAnim, bOnlyTP); }
	
	void PlayItemAnimation(const FWeaponAnim& InWeaponAnim, bool bRestartIfAlreadyPlaying = true, bool bOnlyLocal = false, bool bOnlyTP = false);
	void StopItemAnimation(const FWeaponAnim& InWeaponAnim, bool bOnlyTP = false);

	bool IsItemAnimationPlaying(const FWeaponAnim& InWeaponAnim, bool bOnlyFP = false, bool bOnlyBody = false) const;

	bool IsFPMontagePlaying(const UAnimMontage* InMontage) const;
	bool IsTPMontagePlaying(const UAnimMontage* InMontage) const;

	UFUNCTION()
	void StopTPMontage(class UAnimMontage* AnimMontage = nullptr);

	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = Animation)
		void Multicast_PlayTPMontage(UAnimMontage* NewMontage, float PlayRate = 1.0f);
	virtual void Multicast_PlayTPMontage_Implementation(UAnimMontage* NewMontage, float PlayRate = 1.0f);

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = Animation)
		void Server_PlayTPMontage(UAnimMontage* NewMontage, float PlayRate = 1.0f);
	virtual void Server_PlayTPMontage_Implementation(UAnimMontage* NewMontage, float PlayRate = 1.0f);
	virtual bool Server_PlayTPMontage_Validate(UAnimMontage* NewMontage, float PlayRate = 1.0f) { return true; }
	
	AReadyOrNotCharacter* GetOwnerCharacter() const;
	class APlayerCharacter* GetOwnerPlayerCharacter() const;

	APlayerController* GetOwningPlayerController() const;
	class ACyberneticController* GetOwningAIController() const;
	class ACyberneticCharacter* GetOwningAICharacter() const;

	UFUNCTION(BlueprintPure)
	bool IsEquipped() const;

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
		TSubclassOf<ULegacyCameraShake> Reload_CameraShake;

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
		TSubclassOf<ULegacyCameraShake> ReloadEmpty_CameraShake;

	UPROPERTY(EditAnywhere, Category = "AItem")
		TSubclassOf<ULegacyCameraShake> DrawCameraShake;

	UPROPERTY(EditAnywhere, Category = "AItem")
		TSubclassOf<ULegacyCameraShake> HolsterCameraShake;

	UFUNCTION(BlueprintPure, Category = "AItem")
	virtual bool IsBlockingAnimationPlaying(TArray<EBlockingAnimationExclusion> Exclusions) const;

	UPROPERTY()
	AReadyOrNotCharacter* PreviousOwner = nullptr;

	FPoseSnapshot FPPoseSnapshot;
	UPROPERTY()
	UClass* LastFPAnimInstanceClass;
	FPoseSnapshot TPPoseSnapshot;
	UPROPERTY()
	UClass* LastTPAnimInstanceClass;

	UFUNCTION(BlueprintPure, Category = Gameplay)
	bool IsLocallyControlled();

	UFUNCTION(BlueprintCallable, Category = Evidence)
	void MarkAsEvidence(bool bMarkAsEvidence);

	//UPROPERTY(Replicated)
	//class AThrownEvidenceActor* ThrownEvidenceActor;

	//FVector FinalDropLocation;
	
	//FVector ProjectDropLocationToNavmesh();

	// COOP Evidence Collection variables 
	UPROPERTY(VisibleAnywhere, Replicated ,Category = "Collection")
	bool bIsBeingCollected;

	UPROPERTY(VisibleAnywhere, Replicated ,Category = "Collection")
	float CurrentCollectionTime = 0.0f;

	UPROPERTY(VisibleAnywhere, Replicated, Category = "Collection")
	float MaxCollectionTime = 2.0f;

	UPROPERTY(VisibleAnywhere, Replicated, Category = "Collection")
	class AReadyOrNotCharacter* CollectingCharacter;

	void StartEvidenceCollection_COOP(class AReadyOrNotCharacter* Collector);
	
	void StopEvidenceCollection_COOP();
	
	void CompleteEvidenceCollection_COOP(class AReadyOrNotCharacter* Collector);

	UPROPERTY(ReplicatedUsing = OnRep_IsDropping)
	bool bDropping = false;

	UFUNCTION()
	void OnRep_IsDropping();

	bool bHasRunInitialRep = false;

	UFUNCTION(BlueprintCallable, Category = Networking)
	virtual void OnRep_AttachmentRep();

	FTimerHandle DisableOrEnableAnimation_Handle;
	UFUNCTION()
	void DisableOrEnableAnimation();

	UPROPERTY(EditAnywhere, Category = Optimizations)
		bool bDisableAnimInstanceWhenNotEquipped = true;

	FTimerHandle OnRep_AttachmentRepTick_Handle;


	virtual void OnRep_AttachmentReplication() override;

	UFUNCTION(BlueprintImplementableEvent, Category = Networking)
		void BP_AttachmentRep();

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE class USkeletalMeshComponent* GetItemMesh() const { return ItemMesh; }

	//UFUNCTION(BlueprintCallable, BlueprintPure)
	//FORCEINLINE class UAudioComponent* GetAudioComp() const { return AudioComponent; }


	// additions for new anim system
	
	/* Animation Data for current Item */
	UPROPERTY(EditAnywhere, Replicated, BlueprintReadOnly, Category = "AnimationData")
	UReadyOrNotWeaponAnimData* AnimationData;

	/* Animation Data for equipped grip of current Item */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AnimationData")
		UReadyOrNotWeaponAnimData* DefaultAnimationData;

	/* Animation Data for equipped grip of current Item */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AnimationData")
	UReadyOrNotWeaponAnimData* GripAnimationData;

	/* Animation Data for when this item is equipped with a shield (raised mode) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AnimationData")
	UReadyOrNotWeaponAnimData* ShieldRaisedAnimationData;

	/* Animation Data for when this item is equipped with a shield (lowered mode) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AnimationData")
	UReadyOrNotWeaponAnimData* ShieldLoweredAnimationData;

	UAnimMontage* GetRandWeapAnimFromList(TArray<FWeaponAnim> AnimList, EAnimationType PartToPlay);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Aim")
	float FreeAimLimit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Aim")
	float FreeAimLimitADS;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Aim")
	float LazySpringStrength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Aim")
	float LazySpringStrengthADS;

	//
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Aim")
	float FreeAimInterpSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Aim")
	float FreeAimInterpADSModifier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Aim")
	float FreeAimInterpHipModifier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Aim")
	float FreeAimSlowMoveModifier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Aim")
	float FreeAimSlowMoveTolerance;
	//

	float TimeStuck = 0.0f;


	float GetFreeAimLimit();
	float GetFreeAimLimitADS();

	float GetLazySpringStrength();
	float GetLazySpringStrengthADS();

	// The camera shake that melee gives to the person who is causing it
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Melee)
	TSubclassOf<ULegacyCameraShake> MeleeUserCameraShake;

	// Stuff for AI
	UPROPERTY(BlueprintReadOnly, Category = "AI")
		bool bADSCountsAsAbuse = false;
	
	UPROPERTY(BlueprintReadOnly, Category = "AI")
	float HesitationBoostMultiplier = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Aim")
		bool bIsAimingDownSights = false;

	//void TraceADS(float DeltaSeconds);

	virtual void UpdateFOVShader(float DeltaTime);

public:
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = LowReady)
	virtual float GetLowReadyRange();

	// Low ready system, what pitch we will switch between going down/up
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = LowReady)
	float LowReadyPitchThreshold = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Audio")
	class UWeaponSound* SoundData;

	// Whether we should override the breathing audio effect
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Item|Audio")
		bool bOverrideBreathingEvent;

	// What to override the breathing audio effect with
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Item|Audio")
		UFMODEvent* BreathingAudioOverride;

	// Whether to use easy pickup collision
	UPROPERTY(BlueprintReadOnly, Replicated, EditAnywhere, Category = ABaseItem)
	bool bEasyPickup;

	// Whether this item has its pickups disabled (thrown grenades, etc)
	UPROPERTY(BlueprintReadOnly, Replicated, EditAnywhere, Category = ABaseItem)
	bool bNoPickup;

	UPROPERTY(BlueprintReadWrite, Category = ABaseItem)
		bool bScriptedFPHidden = false;

	UFUNCTION(Client, Reliable, BlueprintCallable, Category = ABaseItem)
		void Client_SetFPModelVisibility(bool bVisibility);
	virtual void Client_SetFPModelVisibility_Implementation(bool bVisibility);

	UFUNCTION(BlueprintCallable)
	void PlayWeaponCleaning();

public:
	// additon for motion block
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Gameplay")
	EMotionBlockType ActiveMotionBlock = EMotionBlockType::MB_Rifle;

	// addition for anim graph
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Gameplay")
	bool bIsOneHandedItem;

	// true if the item has a doorpush animation
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Animation")
		bool HasDoorPushAnimation() const;
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Animation")
		bool IsDoorPushAnimationPlaying() const;

	// true if the item has a button push animation
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Animation")
		bool HasButtonPushAnimation() const;
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Animation")
		bool IsButtonPushAnimationPlaying() const;

	// play the doorpush animation
	UFUNCTION(BlueprintCallable, Category = "Animation")
		void PlayDoorPushAnimation();

	// play the button push animation
	UFUNCTION(BlueprintCallable, Category = "Animation")
		void PlayButtonPushAnimation();

	/* Controls Weight/Intensity of FP ADS Movement Motions */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim")
	float FP_ADS_Motion_Weight = 0.15f;

	virtual bool HandleMelee(FHitResult Hit) { return false; }

	// Physics event for FP/TP weapon
	UFUNCTION()
		void OnMeshComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	/* temporary to blend the ads value */
	float CUR_FPS_ADS_Weight;
	
	UFUNCTION(BlueprintCallable, Category = "Accessibility")
	void DrawOutline();
	UFUNCTION(BlueprintCallable, Category = "Accessibility")
	void DisableOutline();

	UFUNCTION()
	virtual void OnThrownFromInventory(AReadyOrNotCharacter* Thrower, bool bMarkAsEvidence = true);

	// ICanIssueCommandOn
	///////////////////////////////////
	virtual bool CanIssueCommand_Implementation() const override;
	virtual AActor* GetCommandActor_Implementation() const override;
	///////////////////////////////////

	// IUseabilityInterface
	///////////////////////////////////
	virtual void Interact_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent) override;
	virtual void EndInteract_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent) override;
	virtual void OnFocusLost_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent) override;
	virtual class UInteractableComponent* GetInteractableComponent_Implementation() const override;
	virtual FName DetermineAnimatedIcon_Implementation() const override;
	virtual FText DetermineActionText_Implementation() const override;
	virtual EInputEvent DetermineInputEvent_Implementation() const override;
	virtual bool CanInteractThroughHitActors_Implementation(const FHitResult& Hit) const override;
	///////////////////////////////////

	// IPingInterface
	///////////////////////////////////
	virtual FSlateBrush GetPingIcon_Implementation() override;
	virtual FText GetPingText_Implementation() override;
	virtual FVector GetPingLocation_Implementation() override;
	virtual float GetPingDuration_Implementation() override;
	virtual bool CanPing_Implementation() override;
	///////////////////////////////////
	
	// IScoringInterface
	///////////////////////////////////
	virtual class UScoringComponent* GetScoringComponent_Implementation() const override;
	///////////////////////////////////

	virtual void GatherDebugData_Implementation(TArray<FDebugData>& OutDebugData) override;

	virtual void Secure_Implementation(AReadyOrNotCharacter* InInstigator) override;
	virtual bool IsSecured_Implementation() const override;
	virtual FVector GetLocation_Implementation() const override;
	virtual bool CanBeSecured_Implementation() const override;
	virtual bool CanBeSecuredByTrailers_Implementation() const override;
};
