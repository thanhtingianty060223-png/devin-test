// Copyright Void Interactive, 2023

#pragma once

#include "PooledActor.h"
#include "ImpactEffect.generated.h"

USTRUCT(BlueprintType)
struct FImpactFx
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category = "Particle")
	UParticleSystem* ParticleFx;

	UPROPERTY(EditAnywhere, Category = "Particle")
	USoundCue* SoundFx;

	UPROPERTY(EditAnywhere, Category = "Decal")
	UMaterialInterface* Decal;

	UPROPERTY(EditAnywhere, Category = "Decal")
	bool bUseRandomFrame = false;

	UPROPERTY(EditAnywhere, Category = "Decal")
	int32 FrameMax = 3;

	UPROPERTY(EditAnywhere, Category = "Decal Mesh")
	TSubclassOf<AActor> DecalMesh;

	UPROPERTY(EditAnywhere, Category = "DoN's Mesh Painting")
	UTexture2D* PaintMaterialTexture;

};

UENUM(BlueprintType)
enum class EImpactEffectType : uint8
{
	Default,
	Rifle,
	Pistol,
	Shotgun,
	Ricochet,
	Beanbag,
	Pepperball,
	Flare
};

UCLASS(Config=Game)
class READYORNOT_API AImpactEffect : public APooledActor
{
	GENERATED_BODY()
	
public:	
	AImpactEffect();

	virtual void BeginPlay() override;

	void TriggerImpactEffect(const FHitResult& InSurfaceHit);
	void TriggerImpactEffect(const FHitResult& InSurfaceHit, FVector ParticleDirection);
	void TriggerRicochetEffect(const FHitResult& InSurfaceHit, FVector Direction);
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	USceneComponent* SceneComponent = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UParticleSystemComponent* ParticleSystemComponent = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UFMODAudioComponent* FMODAudioComponent = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UDecalComponent* DecalComponent = nullptr;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UDecalComponent* DecalRicochetComponent = nullptr;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UDecalComponent* DecalBloodComponent = nullptr;

	UPROPERTY()
	TArray<UParticleSystemComponent*> SpawnedParticles;

	FHitResult SurfaceHit;
	FHitResult SurfaceExit;

	UPROPERTY(EditAnywhere, Category = "Impact")
	EImpactEffectType Type = EImpactEffectType::Default;
	
	UPROPERTY(EditAnywhere, Category="Defaults")
	bool bPlayDefaultIfNull = false;

	UPROPERTY()
	float DecalScale = 1.0f;
	
	bool b2DSound = false;
	float Volume = 0.4f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Decal")
	float DecalMinSize = 2.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Decal")
	float DecalMaxSize = 2.2f;

	UPROPERTY(EditAnywhere, Category="Particle")
	float ParticleScale = 1.0f;

	UPROPERTY(EditAnywhere, Category="Particle")
	bool bReflectImpactEffectAcrossNormal;
	
	UPROPERTY(EditAnywhere, Category="Particle")
	UFMODEvent* FMODSoundFx;

	UPROPERTY(BlueprintReadWrite, Category="Particle")
	UFMODEvent* FMODHitmarker;
	
	UParticleSystem* GetEntryFX(EPhysicalSurface SurfaceType) const;

 	FImpactFx GetImpactFx(EPhysicalSurface Surface) const;

	UParticleSystem* GetExitFX(EPhysicalSurface SurfaceType) const;

	USoundCue* GetImpactSound(EPhysicalSurface SurfaceType) const;
	UMaterialInterface* GetEntryDecal(EPhysicalSurface SurfaceType) const;
	UMaterialInterface* GetExitDecal(EPhysicalSurface SurfaceType) const;

	UTexture2D* GetPaintMaterialTexture(EPhysicalSurface SurfaceType) const;

	UPROPERTY(Config)
	int32 MaxDecalMeshes = 250;
	
	UPROPERTY(EditAnywhere, Category = Decals)
	float DecalMeshScaleMultiplier = 0.1f;

	TSubclassOf<AActor> GetEntryDecalMesh(EPhysicalSurface SurfaceType) const;
	TSubclassOf<AActor> GetExitDecalMesh(EPhysicalSurface SurfaceType) const;
	bool ShouldUseRandomFrame(EPhysicalSurface SurfaceType) const;
	int32 GetRandomFrameMax(EPhysicalSurface SurfaceType) const;
	
	UPROPERTY()
	bool bBulletGoneThroughPlayer = false;

	UPROPERTY()
	bool bArmorImpact = false;

	UPROPERTY()
	bool bSpawnParticle = true;

	UPROPERTY()
	bool bTraceComplex = false;
	
	UPROPERTY(EditAnywhere, Category = Decals)
	TArray<UMaterialInterface*> BloodExitDecals;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FImpactFx DefaultImpactFx;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FImpactFx RON_Aluminium;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FImpactFx RON_Asphalt;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FImpactFx RON_Brick;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FImpactFx RON_CarbonFibre;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FImpactFx RON_Cardboard;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FImpactFx RON_Ceramic;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FImpactFx RON_ConcreteSoft;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FImpactFx RON_ConcreteStrong;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FImpactFx RON_Dirt;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FImpactFx RON_Drywall;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FImpactFx RON_Electrical;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FImpactFx RON_EnergyShield;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FImpactFx RON_Fabric_Carpet;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FImpactFx RON_Fabric_Stuffing;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FImpactFx RON_Fabric_Thin;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FImpactFx RON_Flesh;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FImpactFx RON_Galvanized;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FImpactFx RON_Glass_Plate;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FImpactFx RON_Glass_Windshield;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FImpactFx RON_Grass;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FImpactFx RON_Gravel;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FImpactFx RON_Ice;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FImpactFx RON_Lava;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FImpactFx RON_Lead;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FImpactFx RON_Leaves;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FImpactFx RON_Limestone;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FImpactFx RON_Mahogany;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FImpactFx RON_Marble_Coated;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FImpactFx RON_Marble_Thick;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FImpactFx RON_Mud;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FImpactFx RON_Oil;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FImpactFx RON_Paper;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FImpactFx RON_Pine;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FImpactFx RON_Plaster;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FImpactFx RON_Plastic;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FImpactFx RON_Plywood;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FImpactFx RON_Polystyrene;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FImpactFx RON_Powder;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FImpactFx RON_Rock;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FImpactFx RON_Rubber;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FImpactFx RON_Sand;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FImpactFx RON_Snow;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FImpactFx RON_Soil;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FImpactFx RON_Steel;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FImpactFx RON_Tin;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FImpactFx RON_Treewood;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FImpactFx RON_Wallpaper;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FImpactFx RON_Water;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FImpactFx RON_Vehicle;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FImpactFx RON_Bulletproof_Glass;
};
