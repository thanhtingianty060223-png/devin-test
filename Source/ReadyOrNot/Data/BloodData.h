// Copyright Void Interactive, 2022

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Particles/ParticleSystem.h"
#include "BloodData.generated.h"

// TODO(killo): separate each category into structs for convenience? i.e. SplatterData, AnimatedSplatterData,

USTRUCT(BlueprintType)
struct READYORNOT_API FArteryData
{
	GENERATED_BODY()
	
	// Name of the bone/socket this artery is connected to
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName BoneName = NAME_None;

	// NOTE(killo): good idea if we have issues with artery hits not occuring on certain hits we want them to
	// Valid bones that this artery hit can occur from
	// UPROPERTY(EditAnywhere, BlueprintReadOnly)
	// TArray<FName> ValidBones;
	
	// Size of this arterial hit zone (in cm)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float ZoneSize = 10.0f;

	// Zone offset (in cm) relative to forward direction of the bone/socket
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float ZoneOffset = 0.0f;

	// The time it takes for death after hitting this artery with a normal hit
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float DeathTime = 5.0f;
};

USTRUCT(BlueprintType)
struct READYORNOT_API FGibData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	UStaticMesh* GibHead;
	
	UPROPERTY(EditAnywhere)
	UStaticMesh* GibArms;

	UPROPERTY(EditAnywhere)
	UStaticMesh* GibLegs;

	UPROPERTY(EditAnywhere)
	UStaticMesh* BoneHead;

	UPROPERTY(EditAnywhere)
	UStaticMesh* BoneArms;

	UPROPERTY(EditAnywhere)
	UStaticMesh* BoneLegs;
};

UCLASS(Blueprintable, BlueprintType)
class READYORNOT_API UBloodData : public UDataAsset
{
	GENERATED_BODY()

public:
	const float DecalFadeScreenSize = 0.02f;

	// Splatters
	
	// Regular splatters that spawn on far or horizontal surfaces (e.g. floors and ceilings)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Splatters")
	TArray<UMaterialInterface*> Splatters;

	// Upper distance limit for ALL blood splatters
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Splatters")
	float SplatterMaxTraceDistance = 1000.0f;

	// Upper distance limit for ALL blood splatters
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Splatters")
	FVector2D SplatterSizeRange = FVector2D(50.0f, 80.0f);

	
	// Animated splatters

	// Animated splatters for vertical surfaces (e.g. walls)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Splatters|Animated")
	TArray<UMaterialInterface*> AnimatedSplatters;
	
	// Animated decal class to use when animated decals are spawned
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Splatters|Animated")
	TSubclassOf<class AAnimatedDecal> AnimatedDecalClass;

	// Which bones can trigger animated splatters when hit
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Splatters|Animated")
	TArray<FName> AnimatedSplatterBones = { "spine_1", "spine_2", "pelvis", "root", "spine_3", "chest" };
	
	// Max distance that animated blood splatters can spawned, capped by the splatter max trace distance
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Splatters|Animated")
	float AnimatedSplatterMaxDistance = 250.0f;

	// The minimum and maximum size for animated splatter decals
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Splatters|Animated")
	FVector2D AnimatedSplatterSizeRange = FVector2D(50.0f, 80.0f);

	// Scalar for animation delta time when animating blood splatters 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Splatters|Animated")
	float AnimatedBloodTimescale = 0.25f;

	// Animation curve to use when animating blood splatters, animation progress over time
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Splatters|Animated")
	FRuntimeFloatCurve AnimatedBloodCurve;

	
	// Headshot splatters
	
	// Splatters for killing headshots only
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Splatters|Headshot")
	TArray<UMaterialInterface*> HeadshotSplatters;

	// The mesh to use for headshot mesh splatter
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Splatters|Headshot")
	UStaticMesh* HeadshotDecalMesh = nullptr;

	// Which bones can trigger headshot splatters when hit
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Splatters|Headshot")
	TArray<FName> HeadshotSplatterBones = { "head", "head_end", "head_equipment" };
	
	// Max distance headshot splatters can occur
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Splatters|Headshot")
	float HeadshotMaxSplatterDistance = 375.0f;

	// The minimum and maximum size for animated splatter decals
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Splatters|Headshot")
	FVector2D HeadshotSplatterSizeRange = FVector2D(50.0f, 80.0f);

	
	// Hit decals

	// Particle effects to play on entry wounds
	UPROPERTY(EditAnywhere, Category = "Hits")
	TArray<UParticleSystem*> HitEntryParticles;

	// Number of images in the skinned decal atlas (for randomization)
	UPROPERTY(EditAnywhere, Category = "Hits")
	uint32 SkinnedDecalImageCount = 9;

	UPROPERTY(EditAnywhere, Category = "Hits")
	FVector2D SkinnedDecalSizeRange = FVector2D(20.0f, 30.0f);

	// Arteries

	// Per-artery specific data including bone name
	UPROPERTY(EditAnywhere, Category = "Arteries")
	TArray<FArteryData> Arteries;

	// Spawn these particles from artery bleedout spots
	UPROPERTY(EditAnywhere, Category = "Arteries")
	TArray<UParticleSystem*> ArteryParticles;

	// Materials to use for decals that spawn during particle collision events
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arteries")
	TArray<UMaterialInterface*> ArteryParticleCollisionDecals;

	// Chance that a given particle collision will spawn a decal
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arteries")
	float ArteryParticleCollisionChance = 0.1f;

	// Particle collision decal random size range
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arteries")
	FVector2D ArteryParticleCollisionSizeRange = FVector2D(25.0f, 40.0f);
	
	// Dismemberment

	// Dismemberment particle effects to play on dismemberment points
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dismemberment")
	TArray<UParticleSystem*> DismembermentParticles;

	// Materials to use for decals that spawn during particle collision events
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dismemberment")
	TArray<UMaterialInterface*> DismembermentParticleCollisionDecals;

	// Chance that a given particle collision will spawn a decal
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dismemberment")
	float DismembermentParticleCollisionChance = 0.1f;

	// Particle collision decal random size range
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dismemberment")
	FVector2D DismembermentParticleCollisionSizeRange = FVector2D(25.0f, 40.0f);
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dismemberment", meta=(ShowOnlyInnerProperties))
	FGibData GibData;

	
	// Blood pools
	
	// The blood pool blueprint class to spawn after the set delay
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Blood Pools")
	TSubclassOf<class ABloodPool> BloodPoolClass;

	// How long to wait after death to spawn a blood pool
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Blood Pools")
	float BloodPoolSpawnDelay = 3.0f;

	// The bone we should spawn blood pools beneath by default
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Blood Pools")
	FName BloodPoolSpawnBone = NAME_None;

	
	// Explosive vest

	// Big splatter decals that spawn beneath exploding vests
	UPROPERTY(EditAnywhere, Category = "Explosive Vest")
	TArray<UMaterialInstance*> BigSplatterDecals;

	// The explosion gibs blueprint class to spawn after an explosion
	UPROPERTY(EditAnywhere, Category = "Explosive Vest")
	TSubclassOf<class AExplosionGibs> Gibs;

	// How far should we check for ground to spawn a splatter decal
	UPROPERTY(EditAnywhere, Category = "Explosive Vest")
	float BigSplatterTraceDistance = 100.0f;

	// The size of the splatter decal
	UPROPERTY(EditAnywhere, Category = "Explosive Vest")
	FVector BigSplatterDecalSize = FVector(8.0f, 96.0f, 96.0f);

	
	// Sounds

	// Sound that plays when bullet makes impact with flesh
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sounds")
	UFMODEvent* HitEvent;

	// Sound that plays when a dead body is hit by a bullet
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sounds")
	UFMODEvent* DeadHitEvent;

	// Sound that plays during dismemberment
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sounds")
	UFMODEvent* GoreEvent;

	// Sound that plays during headshots
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sounds")
	UFMODEvent* HeadshotEvent;
};
