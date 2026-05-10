// Copyright Void Interactive, 2023

#include "ImpactEffect.h"
#include "FMODBlueprintStatics.h"

AImpactEffect::AImpactEffect()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickInterval = 1.0f;

	bEverAllowTick = false;

	SceneComponent = CreateDefaultSubobject<USceneComponent>("Scene Component");
	SetRootComponent(SceneComponent);

	ParticleSystemComponent = CreateDefaultSubobject<UParticleSystemComponent>("Impact Particle Component");
	ParticleSystemComponent->bAutoDestroy = false;
	ParticleSystemComponent->bAllowAnyoneToDestroyMe = false;
	ParticleSystemComponent->SecondsBeforeInactive = 0.0f;
	ParticleSystemComponent->bAutoActivate = false;
	ParticleSystemComponent->bOverrideLODMethod = false;
	ParticleSystemComponent->SetupAttachment(GetRootComponent());
	
	FMODAudioComponent = CreateDefaultSubobject<UFMODAudioComponent>("FMOD Audio Component");
    FMODAudioComponent->bAutoActivate = false;
    FMODAudioComponent->bAutoDestroy = false;
    FMODAudioComponent->bStopWhenOwnerDestroyed = true;
	#if WITH_EDITORONLY_DATA
    FMODAudioComponent->bVisualizeComponent = false;
	#endif
    FMODAudioComponent->SetupAttachment(GetRootComponent());
	FMODAudioComponent->SetRelativeLocation(FVector::ZeroVector);
}

void AImpactEffect::BeginPlay()
{
	Super::BeginPlay();

	DecalComponent = UReadyOrNotFunctionLibrary::CreateDecalComponent(GetWorld(), nullptr, FVector::ZeroVector);
	DecalRicochetComponent = UReadyOrNotFunctionLibrary::CreateDecalComponent(GetWorld(), nullptr, FVector::ZeroVector);
	DecalBloodComponent = UReadyOrNotFunctionLibrary::CreateDecalComponent(GetWorld(), nullptr, FVector::ZeroVector);
}

void AImpactEffect::TriggerImpactEffect(const FHitResult& InSurfaceHit)
{
	FVector ParticleDirection = InSurfaceHit.Normal;
	if (bReflectImpactEffectAcrossNormal)
	{
		FVector Direction = InSurfaceHit.TraceEnd - InSurfaceHit.TraceStart;
		Direction.Normalize();

		ParticleDirection = FMath::GetReflectionVector(Direction, InSurfaceHit.Normal);
		ParticleDirection.Normalize();
	}
	
	TriggerImpactEffect(InSurfaceHit, ParticleDirection);
}

void AImpactEffect::TriggerImpactEffect(const FHitResult& InSurfaceHit, FVector ParticleDirection)
{
	if (!InSurfaceHit.GetComponent())
		return;
	
	SurfaceHit = InSurfaceHit;
	UPhysicalMaterial* HitPhysMat = SurfaceHit.PhysMaterial.Get();
	EPhysicalSurface HitSurfaceType = UPhysicalMaterial::DetermineSurfaceType(HitPhysMat);
	if (bArmorImpact)
	{
		HitSurfaceType = SurfaceType4;
	}

	if (bTraceComplex)
	{
		FHitResult ComplexHit;
		FCollisionQueryParams CollisionQueryParams;
		CollisionQueryParams.bTraceComplex = true;
		GetWorld()->LineTraceSingleByChannel(ComplexHit, InSurfaceHit.ImpactPoint, InSurfaceHit.TraceEnd, ECC_PROJECTILE, CollisionQueryParams);
		if (ComplexHit.GetActor() == SurfaceHit.GetActor() && !ComplexHit.bStartPenetrating)
		{
			SurfaceHit = ComplexHit;
		}
	}

	if (bSpawnParticle)
	{	
		ParticleSystemComponent->SetTemplate(GetEntryFX(HitSurfaceType));

		ParticleSystemComponent->SetUsingAbsoluteLocation(true);
		ParticleSystemComponent->SetUsingAbsoluteRotation(true);
		ParticleSystemComponent->SetUsingAbsoluteScale(true);

		// Completely set the transform to overwrite any changes made to this instance
		FTransform ParticleTransform = FTransform(
			ParticleDirection.Rotation() + FRotator(-90, 0, 0),
			SurfaceHit.Location,
			FVector(ParticleScale));
		
		ParticleSystemComponent->SetWorldTransform(ParticleTransform);
		ParticleSystemComponent->Activate(true);
		
		// Notify the texture streamer so that PSC gets managed as a dynamic component.
		IStreamingManager::Get().NotifyPrimitiveUpdated(ParticleSystemComponent);
		
		//SpawnedParticles.Add(UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), GetEntryFX(HitSurfaceType), SurfaceHit.Location, ParticleDirection.Rotation() + FRotator(-90, 0, 0), FVector(ParticleScale), true, EPSCPoolMethod::None));
	}

	if (b2DSound)
	{
		APlayerCharacter* pc = Cast<APlayerCharacter>(GetOwner());
		if (pc)
		{
			if (!pc->IsLocalPlayer())
			{
				b2DSound = false;
			}
		}
	}

	FVector AdjustedNormal = -SurfaceHit.Normal;

	// Hack fix for inverted decals on hit normals of (0, 0, +/- 1)
	if (SurfaceHit.Normal.GetAbs().Equals(FVector::UpVector, 0.01f))
		AdjustedNormal = SurfaceHit.Normal;

	const float DecalRandomSize = FMath::RandRange(DecalMinSize, DecalMaxSize) * DecalScale;
	FRotator DecalRandomRotation = AdjustedNormal.Rotation().GetDenormalized();
	DecalRandomRotation.Roll = FMath::FRandRange(0.0f, 360.0f);

	//bool bUseMeshPainting;
	//UBpGameplayHelperLib::GetUseMeshpainting(bUseMeshPainting);
	//if (bUseMeshPainting)
	//{
	//	if (GetPaintMaterialTexture(HitSurfaceType) && SurfaceHit.GetActor() && SurfaceHit.GetComponent())
	//	{
	//		UDonMeshPaintingHelper::PaintStroke(SurfaceHit, 10.0f, FLinearColor::White, GetPaintMaterialTexture(HitSurfaceType));
	//	}
	//}

	/*
	if (FMODSoundFx)
	{
		FMODAudioComponent->Event = FMODSoundFx;
		FMODAudioComponent->SetParameter("Surface", HitSurfaceType);
		FMODAudioComponent->Play();
	}
	*/

	if (FMODSoundFx)
	{
		USoundSource* SoundSource = USoundSource::CreateThirdPersonSound(GetWorld(), FMODSoundFx, GetActorTransform(), {}, EOcclusionType::OT_Angular, EPropagationType::PT_Portal, false);
		if(SoundSource)
		{
			SoundSource->SetParameter("Surface", HitSurfaceType);
			SoundSource->Play();
		}
	}
	
	if (bBulletGoneThroughPlayer && BloodExitDecals.Num() > 0 && !Cast<APlayerCharacter>(SurfaceHit.GetActor()))
	{
		float DecalSize = 70.0f * FMath::FRandRange(0.8f, 1.2f);
		// ##UE5UPGRADE## Math
		UDecalComponent* DecalComp = UGameplayStatics::SpawnDecalAttached(BloodExitDecals[FMath::FRandRange(0.0f, BloodExitDecals.Num() - 1)], FVector(0.1f, DecalSize, DecalSize), SurfaceHit.GetComponent(), SurfaceHit.BoneName, SurfaceHit.ImpactPoint, DecalRandomRotation, EAttachLocation::KeepWorldPosition);
		if (DecalComp)
		{
			DecalComp->FadeScreenSize = 0.01f;
			DecalComp->SetSortOrder(1000.0f);
			DecalComp->SetLifeSpan(300.0f);
		}
	}

	UReadyOrNotGameInstance* GameInstance = GetGameInstance<UReadyOrNotGameInstance>();
	TSubclassOf<AActor> DecalMeshClass = GetEntryDecalMesh(HitSurfaceType);
	
	AActor* DecalMesh = nullptr;
	if (GameInstance && DecalMeshClass)
	{
		// Quick and dirty
		TArray<AActor*>& DecalMeshActors = GameInstance->DecalMeshActors;
		if (DecalMeshActors.Num() != MaxDecalMeshes)
			DecalMeshActors.Init(nullptr, MaxDecalMeshes);

		if (DecalMeshActors.Num() > 0)
		{
			AActor* Bottom = DecalMeshActors[0];
			DecalMeshActors.RemoveAt(0, 1, false);

			if (IsValid(Bottom))
				Bottom->Destroy();
			
			FRotator DecalMeshRotator = InSurfaceHit.ImpactNormal.Rotation() + FRotator(-90, 0, 0);
			DecalMeshRotator = FRotator(FQuat(DecalMeshRotator) * FQuat(FRotator(0, FMath::RandRange(0.0f, 360.0f), 0)));
	
			DecalMesh = GetWorld()->SpawnActor<AActor>(DecalMeshClass, SurfaceHit.ImpactPoint, DecalMeshRotator);
			DecalMeshActors.Add(DecalMesh);
		}
	}
	
	if (DecalMesh)
	{
		DecalMesh->GetRootComponent()->SetWorldScale3D(FVector(DecalRandomSize * DecalMeshScaleMultiplier));
		DecalMesh->AttachToComponent(SurfaceHit.GetComponent(), FAttachmentTransformRules::KeepWorldTransform, SurfaceHit.BoneName);
		DecalMesh->SetLifeSpan(300.0f);
	}
	else
	{
		if (UMaterialInterface* Decal = GetEntryDecal(HitSurfaceType))
		{
			const float DecalLength = (SurfaceHit.GetComponent() && SurfaceHit.GetComponent()->IsA(USkeletalMeshComponent::StaticClass())) || SurfaceHit.bStartPenetrating > 0.0f ? 3.0f : 0.5f;

			if (DecalComponent)
			{
				DecalComponent->SetDecalMaterial(Decal);
				DecalComponent->DecalSize = FVector(DecalLength, DecalRandomSize, DecalRandomSize);
				DecalComponent->AttachToComponent(SurfaceHit.GetComponent(), FAttachmentTransformRules::KeepRelativeTransform, SurfaceHit.BoneName);
				DecalComponent->SetWorldLocationAndRotation(SurfaceHit.ImpactPoint, DecalRandomRotation);
				DecalComponent->SetVisibility(true);
				DecalComponent->SetHiddenInGame(false);
				DecalComponent->Activate();
				
				//UDecalComponent* DecalComp = UGameplayStatics::SpawnDecalAttached(GetEntryDecal(HitSurfaceType), FVector(DecalLength, DecalRandomSize, DecalRandomSize), SurfaceHit.GetComponent(), SurfaceHit.BoneName, SurfaceHit.ImpactPoint, DecalRandomRotation, EAttachLocation::KeepWorldPosition);
				//if (DecalComp)
				{
					if (ShouldUseRandomFrame(HitSurfaceType))
					{
						auto MaterialInstance = DecalComponent->CreateDynamicMaterialInstance();
						if (MaterialInstance)
						{
							MaterialInstance->SetScalarParameterValue(TEXT("Frame"), FMath::RandRange(0, GetRandomFrameMax(HitSurfaceType)));
						}
					}
					DecalComponent->FadeScreenSize = 0.01f;
					//DecalComponent->SetLifeSpan(300.0f);
					
					FRotator OrientedRotation = (-SurfaceHit.Normal).Rotation();
					OrientedRotation.Roll = FMath::FRandRange(0.0f, 360.0f);
					DecalComponent->SetWorldRotation(OrientedRotation);
				}
			}
		}
	}
}


void AImpactEffect::TriggerRicochetEffect(const FHitResult& InSurfaceHit, FVector Direction)
{
	// TODO(killo): code repetition ^
	if (!InSurfaceHit.GetComponent())
		return;
	
	SurfaceHit = InSurfaceHit;
	UPhysicalMaterial* HitPhysMat = SurfaceHit.PhysMaterial.Get();
	EPhysicalSurface HitSurfaceType = UPhysicalMaterial::DetermineSurfaceType(HitPhysMat);
	if (bArmorImpact)
	{
		HitSurfaceType = SurfaceType4;
	}

	if (bSpawnParticle)
		SpawnedParticles.Add(UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), GetEntryFX(HitSurfaceType), SurfaceHit.Location, Direction.Rotation() + FRotator(-90, 0, 0), FVector(ParticleScale), true, EPSCPoolMethod::None));

	FVector AdjustedNormal = -SurfaceHit.Normal;

	// Hack fix for inverted decals on hit normals of (0, 0, +/- 1)
	if (SurfaceHit.Normal.GetAbs().Equals(FVector::UpVector, 0.01f))
		AdjustedNormal = SurfaceHit.Normal;

	const float DecalRandomSize = FMath::RandRange(DecalMinSize, DecalMaxSize) * DecalScale;
	FRotator DecalRandomRotation = AdjustedNormal.Rotation().GetDenormalized();
	DecalRandomRotation.Roll = FMath::FRandRange(0.0f, 360.0f);

	if (FMODSoundFx)
	{
		USoundSource* SoundSource = USoundSource::CreateThirdPersonSound(GetWorld(), FMODSoundFx, FTransform(GetActorLocation()), {}, EOcclusionType::OT_Angular, EPropagationType::PT_Portal, false);
		if(SoundSource)
		{
			SoundSource->SetParameter("Surface", HitSurfaceType);
			SoundSource->Play();
		}
	}

	//UFMODAudioComponent* AudioComponent = UFMODBlueprintStatics::PlayEventAttached(FMODSoundFx, GetRootComponent(), NAME_None, FVector::ZeroVector, EAttachLocation::SnapToTarget, true, true, true);

	FRotator DecalMeshRotator = InSurfaceHit.ImpactNormal.Rotation() + FRotator(-90, 0, 0);
	DecalMeshRotator = FRotator(FQuat(DecalMeshRotator) * FQuat(FRotator(0, FMath::RandRange(0.0f, 360.0f), 0)));
	const float DecalLength = (SurfaceHit.GetComponent() && SurfaceHit.GetComponent()->IsA(USkeletalMeshComponent::StaticClass())) || SurfaceHit.bStartPenetrating > 0.0f ? 3.0f : 0.5f;
	UDecalComponent* DecalComp = UGameplayStatics::SpawnDecalAttached(GetEntryDecal(HitSurfaceType),FVector(DecalLength, DecalRandomSize, DecalRandomSize), SurfaceHit.GetComponent(), SurfaceHit.BoneName, SurfaceHit.ImpactPoint, DecalRandomRotation, EAttachLocation::KeepWorldPosition);
	if (DecalComp)
	{
		if (ShouldUseRandomFrame(HitSurfaceType))
		{
			auto MaterialInstance = DecalComp->CreateDynamicMaterialInstance();
			if (MaterialInstance)
			{
				MaterialInstance->SetScalarParameterValue(
					TEXT("Frame"), FMath::RandRange(0, GetRandomFrameMax(HitSurfaceType)));
			}
		}
		DecalComp->FadeScreenSize = 0.01f;
		DecalComp->SetLifeSpan(300.0f);

		FRotator OrientedRotation = (-SurfaceHit.Normal).Rotation();
		OrientedRotation.Roll = FMath::FRandRange(0.0f, 360.0f);
		DecalComp->SetWorldRotation(OrientedRotation);
	}
}

UParticleSystem* AImpactEffect::GetEntryFX(EPhysicalSurface SurfaceType) const
{
	if (UParticleSystem* Particle = GetImpactFx(SurfaceType).ParticleFx)
	{
		return Particle;
	}
	
	if (bPlayDefaultIfNull)
	{
		return DefaultImpactFx.ParticleFx;
	}
	
	return nullptr;
}

bool AImpactEffect::ShouldUseRandomFrame(EPhysicalSurface SurfaceType) const
{
	return GetImpactFx(SurfaceType).bUseRandomFrame;
}

int32 AImpactEffect::GetRandomFrameMax(EPhysicalSurface SurfaceType) const
{
	return GetImpactFx(SurfaceType).FrameMax;
}

FImpactFx AImpactEffect::GetImpactFx(EPhysicalSurface Surface) const
{
	switch (Surface)
	{
	case SurfaceType1:
		return RON_Aluminium;
	case SurfaceType2:
		return RON_Asphalt;
	case SurfaceType3:
		return RON_Brick;
	case SurfaceType4:
		return RON_CarbonFibre;
	case SurfaceType5:
		return RON_Cardboard;
	case SurfaceType6:
		return RON_Ceramic;
	case SurfaceType7:
		return RON_ConcreteSoft;
	case SurfaceType8:
		return RON_ConcreteStrong;
	case SurfaceType9:
		return RON_Dirt;
	case SurfaceType10:
		return RON_Drywall;
	case SurfaceType11:
		return RON_Electrical;
	case SurfaceType12:
		return RON_EnergyShield;
	case SurfaceType13:
		return RON_Fabric_Carpet;
	case SurfaceType14:
		return RON_Fabric_Stuffing;
	case SurfaceType15:
		return RON_Fabric_Thin;
	case SurfaceType16:
		return RON_Flesh;
	case SurfaceType17:
		return RON_Galvanized;
	case SurfaceType18:
		return RON_Glass_Plate;
	case SurfaceType19:
		return RON_Glass_Windshield;
	case SurfaceType20:
		return RON_Grass;
	case SurfaceType21:
		return RON_Gravel;
	case SurfaceType22:
		return RON_Ice;
	case SurfaceType23:
		return RON_Lava;
	case SurfaceType24:
		return RON_Lead;
	case SurfaceType25:
		return RON_Leaves;
	case SurfaceType26:
		return RON_Limestone;
	case SurfaceType27:
		return RON_Mahogany;
	case SurfaceType28:
		return RON_Marble_Coated;
	case SurfaceType29:
		return RON_Marble_Thick;
	case SurfaceType30:
		return RON_Mud;
	case SurfaceType31:
		return RON_Oil;
	case SurfaceType32:
		return RON_Paper;
	case SurfaceType33:
		return RON_Pine;
	case SurfaceType34:
		return RON_Plaster;
	case SurfaceType35:
		return RON_Plastic;
	case SurfaceType36:
		return RON_Plywood;
	case SurfaceType37:
		return RON_Polystyrene;
	case SurfaceType38:
		return RON_Powder;
	case SurfaceType39:
		return RON_Rock;
	case SurfaceType40:
		return RON_Rubber;
	case SurfaceType41:
		return RON_Sand;
	case SurfaceType42:
		return RON_Snow;
	case SurfaceType43:
		return RON_Soil;
	case SurfaceType44:
		return RON_Steel;
	case SurfaceType45:
		return RON_Tin;
	case SurfaceType46:
		return RON_Treewood;
	case SurfaceType47:
		return RON_Wallpaper;
	case SurfaceType48:
		return RON_Water;

	// Addons
	case SurfaceType49:
		return DefaultImpactFx; // RON_Shield
	case SurfaceType50:
		return DefaultImpactFx; // RON_Cloth
	case SurfaceType51:
		return RON_Vehicle;
	case SurfaceType52:
		return RON_Bulletproof_Glass;
	}
	return DefaultImpactFx;
}


UParticleSystem* AImpactEffect::GetExitFX(EPhysicalSurface SurfaceType) const
{
	if (GetImpactFx(SurfaceType).ParticleFx)
	{
		return GetImpactFx(SurfaceType).ParticleFx;
	}
	else if (bPlayDefaultIfNull)
	{
		return DefaultImpactFx.ParticleFx;
	}
	return nullptr;
}

USoundCue* AImpactEffect::GetImpactSound(EPhysicalSurface SurfaceType) const
{
	if (GetImpactFx(SurfaceType).SoundFx)
	{
		return GetImpactFx(SurfaceType).SoundFx;
	}
	else if (bPlayDefaultIfNull)
	{
		return DefaultImpactFx.SoundFx;
	}
	return nullptr;
}

UMaterialInterface* AImpactEffect::GetEntryDecal(EPhysicalSurface SurfaceType) const
{
	if (GetImpactFx(SurfaceType).Decal)
	{
		return GetImpactFx(SurfaceType).Decal;
	}
	else if (bPlayDefaultIfNull)
	{
		return DefaultImpactFx.Decal;
	}
	return nullptr;
}

UMaterialInterface* AImpactEffect::GetExitDecal(EPhysicalSurface SurfaceType) const
{
	return GetEntryDecal(SurfaceType);
}

UTexture2D* AImpactEffect::GetPaintMaterialTexture(EPhysicalSurface SurfaceType) const
{
	if (GetImpactFx(SurfaceType).PaintMaterialTexture)
	{
		return GetImpactFx(SurfaceType).PaintMaterialTexture;
	}
	return nullptr;
}

TSubclassOf<AActor> AImpactEffect::GetEntryDecalMesh(EPhysicalSurface SurfaceType) const
{
	if (GetImpactFx(SurfaceType).DecalMesh)
	{
		return GetImpactFx(SurfaceType).DecalMesh;
	}
	else if (bPlayDefaultIfNull)
	{
		return DefaultImpactFx.DecalMesh;
	}
	return nullptr;
}

TSubclassOf<AActor> AImpactEffect::GetExitDecalMesh(EPhysicalSurface SurfaceType) const
{
	return GetEntryDecalMesh(SurfaceType);
}

