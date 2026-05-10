// Copyright Void Interactive, 2024

#include "BaseMagazineWeapon.h"

#include "Door.h"
#include "ReadyOrNotGameMode.h"
#include "ReadyOrNotDebugSubsystem.h"

#include "Audio/RoNSoundData.h"

#include "Characters/PlayerCharacter.h"
#include "Characters/CyberneticController.h"
#include "Characters/CyberneticCharacter.h"

#include "Actors/BaseShell.h"
#include "Actors/Items/Shotgun.h"
#include "Actors/Items/BallisticsShield.h"

#include "Components/FMODAudioPropagationComponent.h"

#include "Projectiles/DamageProjectiles/BulletProjectile.h"
#include "Projectiles/BulletTracer.h"

#include "NavigationSystem.h"

#include "FMODBlueprintStatics.h"
#include "FMODWorldSubsystem.h"
#include "ObjectPoolBase.h"
#include "ObjectPoolFunctionLibrary.h"
#include "ReadyOrNotAIConfig.h"
#include "Attachments/MagazineAttachment.h"
#include "Characters/AI/SWATCharacter.h"
#include "Characters/AI/TrailerSWATCharacter.h"
#include "Components/InteractableComponent.h"

#include "Data/PenetrationData.h"

#include "DamageTypes/BulletDamageType.h"
#include "Environment/BreakableGlass.h"
#include "Info/SoundManager.h"
#include "Info/SWATManager.h"
#include "lib/HitRegistrationSettings.h"

#include "Perception/AISense_Hearing.h"
#include "Sound/SoundSource.h"

DECLARE_CYCLE_STAT(TEXT("RoN ~ Magazine Weapon ~ Tick"), STAT_BaseMagazineWeaponTick, STATGROUP_BaseMagazineWeapon);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Magazine Weapon ~ Non Local Mag Tick"), STAT_BaseMagazineWeapon_NonLocalMagTick, STATGROUP_BaseMagazineWeapon);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Magazine Weapon ~ Heat"), STAT_BaseMagazineWeapon_Heat, STATGROUP_BaseMagazineWeapon);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Magazine Weapon ~ Bullet Debug"), STAT_BaseMagazineWeapon_BulletDebug, STATGROUP_BaseMagazineWeapon);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Magazine Weapon ~ Penetration Debug"), STAT_BaseMagazineWeapon_PenetrationDebug, STATGROUP_BaseMagazineWeapon);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Magazine Weapon ~ FP Mag Bullet View"), STAT_BaseMagazineWeapon_FPMagBulletView, STATGROUP_BaseMagazineWeapon);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Magazine Weapon ~ Conditional Attach Static"), STAT_BaseMagazineWeapon_ConditionalAttachStatic, STATGROUP_BaseMagazineWeapon);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Magazine Weapon ~ Hit Scan"), STAT_DoHitScan, STATGROUP_BaseMagazineWeapon);

static TAutoConsoleVariable<int32> CVarRonDrawBulletDebug(TEXT("a.RonDrawAmmoDebug"), 0, TEXT("Turn on ammo debug drawing"));
static TAutoConsoleVariable<int32> CVarRonDrawBallisticsDebug(TEXT("a.RonDrawBallisticsDebug"), 0, TEXT("Turn on bullet ballistics drawing"));
static TAutoConsoleVariable<int32> CVarRonDrawPenetrationDebug(TEXT("a.RonDrawPenetrationDebug"), 0, TEXT("Turn on penetration debug drawing"));
static TAutoConsoleVariable<int32> CVarRonDrawBoneSuppression(TEXT("a.RonDrawBoneSuppression"), 0, TEXT("Turn on bone suppression drawing"));
static TAutoConsoleVariable<int32> CVarRonDrawHitRegistration(TEXT("a.RonDrawHitRegistration"), 0, TEXT("Draw hit registration as a client"));
static TAutoConsoleVariable<int32> CVarRonNoBullets(TEXT("a.RonNoBullets"), 0, TEXT("Don't Spawn Bullets"));
static TAutoConsoleVariable<int32> CVarRonServerNoAmmo(TEXT("a.RonServerNoProjectile"), 0, TEXT("Disable spawning projectiles on server"));
static TAutoConsoleVariable<int32> CVarRonDisableHitValidation(TEXT("a.RonForceDisableHitValidation"), 0, TEXT("Force disable hit validation on the server"));

void ABaseMagazineWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABaseMagazineWeapon, Magazines);
	DOREPLIFETIME(ABaseMagazineWeapon, MagIndex);
	DOREPLIFETIME(ABaseMagazineWeapon, NextMagIndex);
	DOREPLIFETIME(ABaseMagazineWeapon, bCancelReloading);
	DOREPLIFETIME(ABaseMagazineWeapon, LastSpawnedProjectile);
	DOREPLIFETIME_CONDITION(ABaseMagazineWeapon, bReloading, COND_SkipOwner);
} 

ABaseMagazineWeapon::ABaseMagazineWeapon()
{
	IFMODStudioModule::Get();

	if (RicochetParticleSystem == nullptr) 
	{
		ConstructorHelpers::FObjectFinder<UParticleSystem> RicochetParticlesObj(TEXT("ParticleSystem'/Game/ReadyOrNot/VFX/VFXImpacts/Edit/P_Bullet_Ricochet.P_Bullet_Ricochet'"));
		RicochetParticleSystem = RicochetParticlesObj.Object;
	}

	if (RicochetEvent == nullptr) 
	{
		ConstructorHelpers::FObjectFinder<UFMODEvent> RicochetEventObj(TEXT("FMODEvent'/Game/FMOD/Events/Surfaces/Impacts/Bullet_Impacts/Ricochets.Ricochets'"));
		RicochetEvent = RicochetEventObj.Object;
	}

	if (SpallingDecal == nullptr) 
	{
		ConstructorHelpers::FObjectFinder<UMaterialInterface> SpallingDecalObj(TEXT("Material'/Game/ReadyOrNot/VFX/VFXImpacts/Decal_Impact_Spall.Decal_Impact_Spall'"));
		SpallingDecal = SpallingDecalObj.Object;
	}
	
	if (SpallingParticleSystem == nullptr) 
	{
		ConstructorHelpers::FObjectFinder<UParticleSystem> SpallingParticlesObj(TEXT("ParticleSystem'/Game/ReadyOrNot/VFX/VFXImpacts/Edit/P_Bullet_Spall.P_Bullet_Spall'"));
		SpallingParticleSystem = SpallingParticlesObj.Object;
	}

	if (SpallingEvent == nullptr) 
	{
		ConstructorHelpers::FObjectFinder<UFMODEvent> SpallingEventObj(TEXT("FMODEvent'/Game/FMOD/Events/Surfaces/Impacts/Bullet_Impacts/Spalling.Spalling'"));
		SpallingEvent = SpallingEventObj.Object;
	}
	
	MuzzleFlashParticleComponent = CreateDefaultSubobject<UParticleSystemComponent>("MuzzleFlash");
	MuzzleFlashParticleComponent->SetupAttachment(ItemMesh, MuzzleFlashParticleSocket);
	MuzzleFlashParticleComponent->SetAutoActivate(false);

	MuzzleSmokeParticleComponent = CreateDefaultSubobject<UParticleSystemComponent>("MuzzleSmoke");
	MuzzleSmokeParticleComponent->SetupAttachment(ItemMesh, MuzzleSmokeParticleSocket);
	MuzzleSmokeParticleComponent->SetAutoActivate(false);

	HeatSmokeParticleComponent = CreateDefaultSubobject<UParticleSystemComponent>("HeatSmoke");
	HeatSmokeParticleComponent->SetupAttachment(ItemMesh, MuzzleSmokeParticleSocket);
	HeatSmokeParticleComponent->SetAutoActivate(false);

	for (uint8 i = 0; i < 5; i++)
	{
		FString CompName = "Ricochet Particle Component " + FString::FromInt(i);
		UParticleSystemComponent* Component = CreateDefaultSubobject<UParticleSystemComponent>(*CompName);
		Component->SetTemplate(RicochetParticleSystem);
		Component->bAutoDestroy = false;
		Component->bAllowAnyoneToDestroyMe = false;
		Component->SecondsBeforeInactive = 0.0f;
		Component->bAutoActivate = false;
		Component->bOverrideLODMethod = false;
		RicochetParticleComponents[i] = Component;

		// Ensure component is not saved
		Component->SetFlags(RF_Transient);
	}
	
	RicochetParticleComponent = RicochetParticleComponents[0];
	
	Mag_01_Comp = CreateDefaultSubobject<UStaticMeshComponent>("Mag_01_Comp");
	Mag_01_Comp->SetupAttachment(SceneComp);
	Mag_01_Comp->SetCastShadow(false);
	Mag_01_Comp->SetMobility(EComponentMobility::Movable);
	Mag_01_Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mag_01_Comp->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mag_01_Comp->SetIsReplicated(false);
	Mag_01_Comp->bNavigationRelevant = false;
	Mag_01_Comp->SetCanEverAffectNavigation(false);
	Mag_01_Comp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	Mag_01_Comp->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Ignore);
	Mag_01_Comp->SetCollisionResponseToChannel(ECC_SOUND, ECR_Ignore);
	
	Mag_01_Bullets_Comp = CreateDefaultSubobject<UStaticMeshComponent>("Mag_01_Bullets_Comp");
	Mag_01_Bullets_Comp->SetupAttachment(SceneComp);
	Mag_01_Bullets_Comp->SetCastShadow(false);
	Mag_01_Bullets_Comp->SetMobility(EComponentMobility::Movable);
	Mag_01_Bullets_Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mag_01_Bullets_Comp->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mag_01_Bullets_Comp->SetIsReplicated(false);
	Mag_01_Bullets_Comp->bNavigationRelevant = false;
	Mag_01_Bullets_Comp->SetCanEverAffectNavigation(false);
	Mag_01_Bullets_Comp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	Mag_01_Bullets_Comp->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Ignore);
	Mag_01_Bullets_Comp->SetCollisionResponseToChannel(ECC_SOUND, ECR_Ignore);
	
	Mag_01_Extra_Comp = CreateDefaultSubobject<UStaticMeshComponent>("Mag_01_Extra_Comp");
	Mag_01_Extra_Comp->SetupAttachment(SceneComp);
	Mag_01_Extra_Comp->SetCastShadow(false);
	Mag_01_Extra_Comp->SetMobility(EComponentMobility::Movable);
	Mag_01_Extra_Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mag_01_Extra_Comp->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mag_01_Extra_Comp->SetIsReplicated(false);
	Mag_01_Extra_Comp->bNavigationRelevant = false;
	Mag_01_Extra_Comp->SetCanEverAffectNavigation(false);
	Mag_01_Extra_Comp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	Mag_01_Extra_Comp->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Ignore);
	Mag_01_Extra_Comp->SetCollisionResponseToChannel(ECC_SOUND, ECR_Ignore);

	Mag_02_Comp = CreateDefaultSubobject<UStaticMeshComponent>("Mag_02_Comp");
	Mag_02_Comp->SetupAttachment(SceneComp);
	Mag_02_Comp->SetCastShadow(false);
	Mag_02_Comp->SetMobility(EComponentMobility::Movable);
	Mag_02_Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mag_02_Comp->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mag_02_Comp->SetIsReplicated(false);
	Mag_02_Comp->bNavigationRelevant = false;
	Mag_02_Comp->SetCanEverAffectNavigation(false);
	Mag_02_Comp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	Mag_02_Comp->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Ignore);
	Mag_02_Comp->SetCollisionResponseToChannel(ECC_SOUND, ECR_Ignore);
	//Mag_02_Comp->SetOnlyOwnerSee(true);
	
	Mag_02_Bullets_Comp = CreateDefaultSubobject<UStaticMeshComponent>("Mag_02_Bullets_Comp");
	Mag_02_Bullets_Comp->SetupAttachment(SceneComp);
	Mag_02_Bullets_Comp->SetCastShadow(false);
	Mag_02_Bullets_Comp->SetMobility(EComponentMobility::Movable);
	Mag_02_Bullets_Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mag_02_Bullets_Comp->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mag_02_Bullets_Comp->SetIsReplicated(false);
	//Mag_02_Bullets_Comp->SetOnlyOwnerSee(true);
	Mag_02_Bullets_Comp->bNavigationRelevant = false;
	Mag_02_Bullets_Comp->SetCanEverAffectNavigation(false);
	Mag_02_Bullets_Comp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	Mag_02_Bullets_Comp->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Ignore);
	Mag_02_Bullets_Comp->SetCollisionResponseToChannel(ECC_SOUND, ECR_Ignore);
	
	Mag_02_Extra_Comp = CreateDefaultSubobject<UStaticMeshComponent>("Mag_02_Extra_Comp");
	Mag_02_Extra_Comp->SetupAttachment(SceneComp);
	Mag_02_Extra_Comp->SetCastShadow(false);
	Mag_02_Extra_Comp->SetMobility(EComponentMobility::Movable);
	Mag_02_Extra_Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mag_02_Extra_Comp->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mag_02_Extra_Comp->SetIsReplicated(false);
	//Mag_02_Extra_Comp->SetOnlyOwnerSee(true);
	Mag_02_Extra_Comp->bNavigationRelevant = false;
	Mag_02_Extra_Comp->SetCanEverAffectNavigation(false);
	Mag_02_Extra_Comp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	Mag_02_Extra_Comp->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Ignore);
	Mag_02_Extra_Comp->SetCollisionResponseToChannel(ECC_SOUND, ECR_Ignore);

	Mag_ReloadInterpFix_Comp = CreateDefaultSubobject<UStaticMeshComponent>("Mag_ReloadInterpFix");
	Mag_ReloadInterpFix_Comp->SetupAttachment(SceneComp);
	Mag_ReloadInterpFix_Comp->SetMobility(EComponentMobility::Movable);
	Mag_ReloadInterpFix_Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mag_ReloadInterpFix_Comp->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mag_ReloadInterpFix_Comp->SetHiddenInGame(true);
	Mag_ReloadInterpFix_Comp->SetIsReplicated(false);
	Mag_ReloadInterpFix_Comp->bNavigationRelevant = false;
	Mag_ReloadInterpFix_Comp->SetCanEverAffectNavigation(false);
	Mag_ReloadInterpFix_Comp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	Mag_ReloadInterpFix_Comp->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Ignore);
	Mag_ReloadInterpFix_Comp->SetCollisionResponseToChannel(ECC_SOUND, ECR_Ignore);
	
	Mag_01_Comp->bVisibleInRayTracing = false;
	Mag_01_Bullets_Comp->bVisibleInRayTracing = false;
	Mag_01_Extra_Comp->bVisibleInRayTracing = false;
	Mag_02_Comp->bVisibleInRayTracing = false;
	Mag_02_Extra_Comp->bVisibleInRayTracing = false;
	Mag_02_Bullets_Comp->bVisibleInRayTracing = false;
	Mag_ReloadInterpFix_Comp->bVisibleInRayTracing = false;

	bDrawnBefore = false;	

	bUseFireLoopAnims = false;
	
	NetPriority = 2.0f;
	NetUpdateFrequency = 100.0f;
	MinNetUpdateFrequency = 2.0f;

	ConstructorHelpers::FObjectFinder<UStaticMesh> SM_FakeBulletMesh(TEXT("StaticMesh'/Game/ThirdParty/TrashSet/Meshes/Props/SM_trashbag_02_b.SM_trashbag_02_b'"));
	FakeProjectileMeshStatic = SM_FakeBulletMesh.Object;
}

void ABaseMagazineWeapon::BeginPlay()
{
	Super::BeginPlay();

	ReplenishAmmo();
}

void ABaseMagazineWeapon::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	//destroys spawned shells
	for (int i = 0; i < SpawnedShells.Num(); i++)
	{
		if (IsValid(SpawnedShells[i])) {
			SpawnedShells[i]->Destroy();
		}
	}	
}

void ABaseMagazineWeapon::SetMagazineCount(int32 Count, TArray<FName> AmmoTypes)
{
	ensureMsgf(Count >= 0, TEXT("Magazine count must be greater than or equal to zero"));
	if (Count < 0)
		return;

	// Keep magazines in a nice order
	// ##UE5UPGRADE##
	AmmoTypes.Sort([](const FName& a1, const FName& a2) {
		return a1.FastLess(a2);
	});

	Magazines.Empty();
	Magazines.InsertUninitialized(0, Count);

	for (int32 i = 0; i < Magazines.Num(); i++)
	{
		Magazines[i].Ammo = AmmoMax;
		Magazines[i].AmmoType = 0;

		// Use default ammo type if missing ammo type
		if (!AmmoTypes.IsValidIndex(i))
			continue;

		// If trying to set an invalid ammo type, ignore
		if (!AmmunitionTypes.Contains(AmmoTypes[i]))
			continue;

		Magazines[i].AmmoType = AmmunitionTypes.IndexOfByKey(AmmoTypes[i]);
	}

	MagIndex = 0;
	
	QueuedAmmoType = Magazines.IsValidIndex(MagIndex) ? Magazines[MagIndex].AmmoType : 0;
	SetAmmunitionType(QueuedAmmoType);
}

void ABaseMagazineWeapon::ReplenishAmmo()
{
	for (int32 i = 0; i < Magazines.Num(); i++)
	{
		// add the bullet in chamber
		if (i == 0 && bBulletInChamberOnReload)
			Magazines[i].Ammo = AmmoMax + 1;
		Magazines[i].Ammo = AmmoMax;
	}
}

int32 ABaseMagazineWeapon::GetMagazineCount()
{
	return Magazines.Num();
}

void ABaseMagazineWeapon::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	SCOPE_CYCLE_COUNTER(STAT_BaseMagazineWeaponTick);
	
#if !UE_BUILD_SHIPPING
	{
		SCOPE_CYCLE_COUNTER(STAT_BaseMagazineWeapon_BulletDebug);
		
		if (CVarRonDrawBulletDebug.GetValueOnGameThread() != 0 && IsEquipped())
		{
			FString DebugText = FString::Printf(TEXT("(%d/%d) %s"), static_cast<int32>(GetAmmo()), static_cast<int32>(AmmoMax), *GetCurrentAmmoTypeRowName().ToString());
			DrawDebugString(GetWorld(), GetActorLocation(), DebugText, nullptr, FColor::Green, 0.0f);
		}
	}
	{
		SCOPE_CYCLE_COUNTER(STAT_BaseMagazineWeapon_PenetrationDebug);

		if (CVarRonDrawPenetrationDebug.GetValueOnGameThread() != 0 && IsEquipped() && GetOwnerPlayerCharacter() && GetOwnerPlayerCharacter()->IsLocallyControlled())
		{
			FVector SpawnLocation = GetBulletSpawn()->GetComponentLocation();
			FVector SpawnDirection = GetBulletSpawn()->GetComponentRotation().Vector();

			FVector TraceStart = SpawnLocation;
			FVector TraceEnd = TraceStart + SpawnDirection * 100000.0f;
			
			FCollisionQueryParams CollisionQueryParams;
			CollisionQueryParams.bTraceComplex = true;
			CollisionQueryParams.bReturnPhysicalMaterial = true;
			CollisionQueryParams.AddIgnoredActor(this);
			CollisionQueryParams.AddIgnoredActor(GetOwner());
	
			if (GetOwnerCharacter())
			{
				CollisionQueryParams.AddIgnoredActors((TArray<AActor*>)GetOwnerCharacter()->GetInventoryComponent()->GetInventoryItems());
			}

			FHitResult HitResult;
			GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_PROJECTILE, CollisionQueryParams);

			UPenetrationData* PenetrationData = UBpGameplayHelperLib::GetPenetrationData();
			
			UPhysicalMaterial* HitPhysMat = HitResult.PhysMaterial.Get();
			const FAmmoTypeData* ShotAmmoType = GetCurrentAmmoType();
			
			if (HitPhysMat && ShotAmmoType)
			{
				EPhysicalSurface HitSurfaceType = UPhysicalMaterial::DetermineSurfaceType(HitPhysMat);
				FMaterialPenetration MaterialPenetration = PenetrationData ? PenetrationData->GetPenetrationData(HitSurfaceType) : FMaterialPenetration();

				FString MaterialName = HitPhysMat->GetName();

				float CurrentPenetrationDistance = ShotAmmoType->PenetrationDistance * 0.1f;
				float AdjustedDistance = CurrentPenetrationDistance / MaterialPenetration.PenetrationDensity;

				bool bPenetrates = CurrentAmmoType.PenetrationLevel >= MaterialPenetration.ArmourLevel;
				FColor Color = bPenetrates ? FColor::Yellow : FColor::Red;
				
				FString DebugString = FString::Printf(TEXT("%s\nDensity %.1fx / Armor Level %d\n%.1fcm/%.1fcm"),
					*MaterialName,
					MaterialPenetration.PenetrationDensity, MaterialPenetration.ArmourLevel,
					AdjustedDistance, CurrentPenetrationDistance);

				DrawDebugLine(GetWorld(), HitResult.TraceStart, HitResult.Location, FColor::Green, false);
				DrawDebugString(GetWorld(), HitResult.Location, DebugString, nullptr, Color, 0.0f, true);
			}
		}
	}
#endif
	
	TimeSinceLastShot += DeltaSeconds;

	TeamMateShotHeadDelay = FMath::Max(TeamMateShotHeadDelay - DeltaSeconds, 0.0f);
	RefireDelayTimer = FMath::Max(RefireDelayTimer - DeltaSeconds, 0.0f);
	AttachStaticDelay = FMath::Max(AttachStaticDelay - DeltaSeconds, 0.0f);
	
	if (InteractableComponent && InteractableComponent->GetAttachSocketName() != FName("J_Gun"))
	{
		InteractableComponent->AttachToComponent(ItemMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("J_Gun"));
		InteractableComponent->SetRelativeLocation(FVector::ZeroVector);
	}

	CurrentMag01Velocity = (PreviousMag01Location - (ItemMesh->GetComponentLocation() - ItemMesh->GetBoneLocation("tag_mag_01")));
	PreviousMag01Location = ItemMesh->GetComponentLocation() - ItemMesh->GetBoneLocation("tag_mag_01");
	//V_LOGM(LogReadyOrNot, "%s", *CurrentMag01Velocity.ToString());

	// magazine debug code
	//DrawDebugSphere(GetWorld(), GetItemMesh()->GetSocketLocation("Mag_01_Socket"), 10.0f, 10, FColor::Green, false, -1, 0, 1);
	//DrawDebugSphere(GetWorld(), Mag_01_Comp->GetComponentLocation(), 3.0f, 10, FColor::Yellow, false, -1, 0, 1);
	//DrawDebugSphere(GetWorld(), Mag_02_Comp->GetComponentLocation(), 4.0f, 10, FColor::Red, false, -1, 0, 1);
	//DrawDebugSphere(GetWorld(), Mag_ReloadInterpFix_Comp->GetComponentLocation(), 5.0f, 10, FColor::Purple, false, -1, 0, 1);

	if (!IsLocallyControlled() && bHasVisibleMags)
	{
		SCOPE_CYCLE_COUNTER(STAT_BaseMagazineWeapon_NonLocalMagTick);
		
		if (Mag_01_Comp && Mag_01_Bullets_Comp && Mag_01_Extra_Comp && Mag_02_Comp && Mag_02_Bullets_Comp && Mag_02_Extra_Comp && Mag_ReloadInterpFix_Comp)
		{
			if (ItemMesh->GetAnimInstance())
			{
				Mag_ReloadInterpFix_Comp->SetStaticMesh(GetAppropriateMagazineMesh());
			}
			
			Mag_02_Comp->SetOnlyOwnerSee(!bReloading);
			Mag_02_Extra_Comp->SetOnlyOwnerSee(!bReloading);
			Mag_02_Bullets_Comp->SetOnlyOwnerSee(!bReloading);
		}
	}

	const AReadyOrNotCharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter)
	{
		if (FiringAudioComp)
		{
			FiringAudioComp->SetParameter("FireMode", 0.0f);
			FiringAudioComp->Stop();
			FiringAudioComp = nullptr;
		}
		
		AttachStatic();
		return;
	}
	
	if (FiringAudioComp)
	{
		if (!FiringAudioComp->IsPlaying())
		{
			FiringAudioComp->Stop();
			FiringAudioComp = nullptr;
		}
	}
	
	// Heat implementation
	{
		SCOPE_CYCLE_COUNTER(STAT_BaseMagazineWeapon_Heat);
		
		if (OwnerCharacter->GetEquippedWeapon() != this && HeatSmokeParticleComponent->bHasBeenActivated)
		{
			HeatSmokeParticleComponent->Deactivate();
		}
		else
		{
			HeatTime += DeltaSeconds;
			CurrentHeat = FMath::Clamp(CurrentHeat - DeltaSeconds * HeatDissipation, 0.0f, CurrentHeat);

			if (!bHeatEffectPlayed && CurrentHeat >= HeatThreshold && HeatTime >= HeatMinimumTime)
			{
				HeatSmokeParticleComponent->Activate(true);
				bHeatEffectPlayed = true;
			}
			else if (CurrentHeat < HeatThreshold && HeatSmokeParticleComponent->bHasBeenActivated)
			{
				HeatSmokeParticleComponent->Deactivate();
			}
		}
	}

	// Show bullets when empty only in first person
	{
		SCOPE_CYCLE_COUNTER(STAT_BaseMagazineWeapon_FPMagBulletView);
		
		if (IsLocallyControlled() && Mag_01_Bullets_Comp)
		{
			// GetAmmo includes bullet in chamber, so plus one
			if (GetAmmo() <= 1 && !bShowBulletsWhenEmpty)
			{
				if (!Mag_01_Bullets_Comp->bHiddenInGame)
				{
					Mag_01_Bullets_Comp->SetHiddenInGame(true);
				}
			}
			else
			{
				if (Mag_01_Bullets_Comp->bHiddenInGame)
				{
					Mag_01_Bullets_Comp->SetHiddenInGame(false);
				}
			}
		}
	}

	{
		SCOPE_CYCLE_COUNTER(STAT_BaseMagazineWeapon_ConditionalAttachStatic);
		
		if (OwnerCharacter->IsLocalPlayer())
		{
			if (OwnerCharacter->GetEquippedWeapon() != this && !IsPistolWithShield())
			{
				DetachStatic();
			}
			else
			{
				if (ItemMesh->IsVisible())
				{
					AttachStatic();
				}
				else
				{
					DetachStatic();
				}
			}
		}
		else
		{
			if (bHasVisibleMags)
				AttachStatic();
		}
	}

	/* some ugly hacked logic to swap between ads/nonads reload sets*/
	/* it should ping pong between the two states depending if you are aiming and not */
	if (const APlayerCharacter* OwnerPlayerCharacter = GetOwnerPlayerCharacter())
	{
		//if (OwnerPlayerCharacter->GetMesh1P()) // Note(Ali): would this ever be null?
		if (IsCurrentlyReloading())
		{
			UAnimInstance* CurAnmInst = OwnerPlayerCharacter->GetMesh1P()->GetAnimInstance();

			if (CurAnmInst && AnimationData)
			{
				UAnimMontage* NewReloadMontage = nullptr;
				
				// PLAYING ADS RELOAD AND NOT SWAPPEDRELOADS AND NOT AIMING == Play Normal Reload
				if ((OwnerPlayerCharacter->Is1PMontagePlaying(AnimationData->Reload_FP_Ads) ||
					OwnerPlayerCharacter->Is1PMontagePlaying(AnimationData->ReloadEmpty_FP_Ads) ||
					OwnerPlayerCharacter->Is1PMontagePlaying(AnimationData->Tactical_Reload_FP_Ads) ||
					OwnerPlayerCharacter->Is1PMontagePlaying(AnimationData->Tactical_ReloadEmpty_FP_Ads)) && !OwnerPlayerCharacter->bAiming)
				{
					// we need to figure out the point at where we want to resume with the normal sequence
					const float CurTime = CurAnmInst->Montage_GetPosition(CurAnmInst->GetCurrentActiveMontage());
					//float CurPlayrate = CurAnmInst->Montage_GetPlayRate(CurAnmInst->GetCurrentActiveMontage());

					if (CurTime < (CurAnmInst->GetCurrentActiveMontage()->GetPlayLength() * 0.8f)) // tolerance 80%
					{
						if (OwnerPlayerCharacter->Is1PMontagePlaying(AnimationData->Reload_FP_Ads))
							NewReloadMontage = AnimationData->Reload.Body_FP;

						if (OwnerPlayerCharacter->Is1PMontagePlaying(AnimationData->ReloadEmpty_FP_Ads))
							NewReloadMontage = AnimationData->ReloadEmpty.Body_FP;

						if (OwnerPlayerCharacter->Is1PMontagePlaying(AnimationData->Tactical_Reload_FP_Ads))
							NewReloadMontage = AnimationData->Tactical_Reload.Body_FP;

						if (OwnerPlayerCharacter->Is1PMontagePlaying(AnimationData->Tactical_ReloadEmpty_FP_Ads))
							NewReloadMontage = AnimationData->Tactical_ReloadEmpty.Body_FP;

						if (NewReloadMontage)
						{
							#if !UE_BUILD_SHIPPING
							UE_LOG(LogTemp, Warning, TEXT("Swapping reload motion to non-ads!"));
							#endif
							
							CurAnmInst->Montage_Play(NewReloadMontage, 1.0f, EMontagePlayReturnType::Duration, CurTime, true);
							//bSwappingReloads = true;
						}
					}
				}

				// PLAYING NORMAL RELOAD AND SWAPPEDRELOADS AND AIMING == Play ADS Reload
				if ((OwnerPlayerCharacter->Is1PMontagePlaying(AnimationData->Reload.Body_FP) ||
					OwnerPlayerCharacter->Is1PMontagePlaying(AnimationData->ReloadEmpty.Body_FP) ||
					OwnerPlayerCharacter->Is1PMontagePlaying(AnimationData->Tactical_Reload.Body_FP) ||
					OwnerPlayerCharacter->Is1PMontagePlaying(AnimationData->Tactical_ReloadEmpty.Body_FP)) && OwnerPlayerCharacter->bAiming)
				{
					// we need to figure out the point at where we want to resume with the normal sequence
					const float CurTime = CurAnmInst->Montage_GetPosition(CurAnmInst->GetCurrentActiveMontage());
					//float CurPlayrate = CurAnmInst->Montage_GetPlayRate(CurAnmInst->GetCurrentActiveMontage());

					if (CurTime < (CurAnmInst->GetCurrentActiveMontage()->GetPlayLength() * 0.8f)) // tolerance 80%
					{
						if (OwnerPlayerCharacter->Is1PMontagePlaying(AnimationData->Reload.Body_FP))
							NewReloadMontage = AnimationData->Reload_FP_Ads;

						if (OwnerPlayerCharacter->Is1PMontagePlaying(AnimationData->ReloadEmpty.Body_FP))
							NewReloadMontage = AnimationData->ReloadEmpty_FP_Ads;

						if (OwnerPlayerCharacter->Is1PMontagePlaying(AnimationData->Tactical_Reload.Body_FP))
							NewReloadMontage = AnimationData->Tactical_Reload_FP_Ads;

						if (OwnerPlayerCharacter->Is1PMontagePlaying(AnimationData->Tactical_ReloadEmpty.Body_FP))
							NewReloadMontage = AnimationData->Tactical_ReloadEmpty_FP_Ads;

						if (NewReloadMontage)
						{
							#if !UE_BUILD_SHIPPING
							UE_LOG(LogTemp, Warning, TEXT("Swapping reload motion to ads!"));
							#endif
							
							CurAnmInst->Montage_Play(NewReloadMontage, 1.0f, EMontagePlayReturnType::Duration, CurTime, true);
							//bSwappingReloads = false;
						}
					}
				}
			}
		}
	}
}

bool ABaseMagazineWeapon::IsCurrentlyReloading() const
{
	// no anim = no reload
	if (!AnimationData)
		return false;
	
	return  IsItemAnimationPlaying(AnimationData->Reload) || IsItemAnimationPlaying(AnimationData->ReloadEmpty) ||
			IsItemAnimationPlaying(AnimationData->Tactical_Reload) || IsItemAnimationPlaying(AnimationData->Tactical_ReloadEmpty) ||
			IsItemAnimationPlaying(AnimationData->Crouch_Reload) || IsItemAnimationPlaying(AnimationData->Crouch_ReloadEmpty) ||
			IsItemAnimationPlaying(AnimationData->Tactical_Crouch_Reload) || IsItemAnimationPlaying(AnimationData->Tactical_Crouch_ReloadEmpty);
}
 
void ABaseMagazineWeapon::OnFire(FRotator Direction, FVector SpawnLoc)
{
    if (IsBlockingAnimationPlaying() || RefireDelayTimer > 0.0f)
    {
	    return;
    }
	
	if (GetLocalRole() < ROLE_Authority || IsLocallyControlled() || Cast<ACyberneticCharacter>(GetOwner()))
	{
		if (ScopeAttachment && ScopeAttachment->GetAllSocketNames().Contains("CenterPoint") && bIsAimingDownSights)
		{
			//DrawDebugLine(GetWorld(), SpawnLoc, SpawnLoc + Direction.Vector() * 250000.0f, FColor::White, false, 100.0f, 0, 1.0f);
			FVector ScopeLocation = ScopeAttachment->GetSocketLocation("CenterPoint");
			FRotator ScopeDirection = ScopeAttachment->GetSocketRotation("CenterPoint");
			//FRotator NewDirection = UKismetMathLibrary::FindLookAtRotation(ScopeLocation, SpawnLoc + ScopeDirection.Vector() * 2500.0f);
			//DrawDebugLine(GetWorld(), ScopeLocation, SpawnLoc + NewDirection.Vector() * 100000.0f, FColor::Green, false, 100.0f, 0, 1.0f);
			if(AimAssistRotation == FRotator::ZeroRotator)
			{
				Direction = UKismetMathLibrary::FindLookAtRotation(SpawnLoc, ScopeLocation + ScopeDirection.Vector() * 2000.0f);
			}
			else
			{
				Direction = AimAssistRotation;
			}
		}

		//DrawDebugLine(GetWorld(), SpawnLoc, SpawnLoc + Direction.Vector() * 250000.0f, FColor::Cyan, false, 100.0f, 0, 0.1f);

		// Prevent same seed from being used twice
		int32 Seed = FMath::Rand();
		// while (Seed == LastSeed)
		// {
		// 	Seed = FMath::Rand();
		// }

		int32 ProjectileIdentifier = 0;
		// Using seed to keep track of local projectile between server and client, so make sure it hasn't already been used
		UGameInstance* gi = GetGameInstance();
		AReadyOrNotPlayerController* pc = Cast<AReadyOrNotPlayerController>(gi->GetFirstLocalPlayerController(GetWorld()));
		if (pc && GetLocalRole() < ROLE_Authority)
		{
			while (pc->ActiveProjectiles.Find(Seed))
			{
				Seed = FMath::Rand();
			}
		}
		
		LocallySimulateFire(Direction, SpawnLoc, Seed);
		Server_OnFire(Direction, SpawnLoc, Seed);
	}
}

void ABaseMagazineWeapon::LocallySimulateFire(const FRotator Direction, const FVector SpawnLoc, int32 Seed)
{
	// Dis ugly af, todo: remove
	if (ACyberneticCharacter* AIOwner = Cast<ACyberneticCharacter>(GetOwner()))
	{
		if (GetAmmo() > 0.0f)
		{
			AIOwner->OnWeaponFire.Broadcast(AIOwner, this, Direction.Vector());
		}

		return;
	}
	
	if (GetAmmo() > 0.0f)
	{
		// Heat implementation
		CurrentHeat = FMath::Clamp(CurrentHeat + HeatPerShot, 0.0f, HeatMax);
		HeatSmokeParticleComponent->Deactivate();
		HeatTime = 0.0f;
		bHeatEffectPlayed = false;
		
		RefireDelayTimer = FireRate + RefireDelay;
	
		FTransform SpawnTransform;
		SpawnTransform.SetLocation(SpawnLoc);
		SpawnTransform.SetRotation(Direction.GetNormalized().Quaternion());

		FRandomStream RandomStream(Seed);
		for (int32 i = 0; i < SpawnProjectileCount; i++)
		{
			if (GetLocalRole() < ROLE_Authority)
			{
				int32 ProjectileSeed = RandomStream.GetUnsignedInt();
				ABulletProjectile* SpawnedBulletProjectile;
				
				if (UReadyOrNotGameInstance::bIsBuildPirated)
				{
					SpawnedBulletProjectile = SpawnProjectile(FakeBulletProjectile, SpawnTransform, true, i, ProjectileSeed);
				}
				else
				{
					SpawnedBulletProjectile = SpawnProjectile(bArmorPiercing && ArmorPiercingBulletProjectile ? ArmorPiercingBulletProjectile : BulletProjectile, SpawnTransform, true, i, ProjectileSeed);
				}
				
				if (AReadyOrNotPlayerController* Controller = Cast<AReadyOrNotPlayerController>(GetOwningPlayerController()))
				{
					Controller->OnLocallyFiredProjectile(SpawnedBulletProjectile, Seed);
				}
			}
		}

		SpawnShell(true);
		
		Multicast_SpawnParticleEffects_Implementation();

		if (GetOwnerPlayerCharacter())
		{
			GetOwnerPlayerCharacter()->OnEquippedWeaponFire(this, false);
		}
		
		OnWeaponFire.Broadcast(this, false);
		Native_OnWeaponFire.ExecuteIfBound();

		// Apply recoil to consecutive shots
		TriggerFirstShot();

		// trigger proc recoil if wanted
		if (bCalculateProcRecoil)
		{
			TriggerProcRecoil(bFirstShot);
		}

		IncrementShotsFired();
	}
	else
	{
		if (SoundData)
		{
			if (SoundData->DryFire)
			{
				PlayFMODAudio(SoundData->DryFire);
			}
		}

		if (GetOwnerPlayerCharacter())
		{
			GetOwnerPlayerCharacter()->OnEquippedWeaponDryFire(this, false);
		}
		
		OnWeaponDryFire.Broadcast(this, false);
	}
}

void ABaseMagazineWeapon::PlayFireAnimation(const bool bIsAiming, const bool bIsCrouched, const bool bOnlyLocal)
{
	if (!AnimationData)
		return;
	
	if (!IsPistolWithShield())
	{
		if (InSingleMode() || !bUseFireLoopAnims)
		{
			if (GetAmmo() <= 1)
			{
				if (bIsCrouched)
					PlayItemAnimation(bIsAiming ? AnimationData->Crouch_FireSingleSightsLast : AnimationData->Crouch_FireSingleLast, true, bOnlyLocal, !bOnlyLocal);
				else
					PlayItemAnimation(bIsAiming ? AnimationData->FireSingleSightsLast : AnimationData->FireSingleLast, true, bOnlyLocal, !bOnlyLocal);
			}
			else
			{
				TArray<FWeaponAnim> Anims;

				if (bIsCrouched)
					Anims = bIsAiming ? AnimationData->Crouch_FireSingleSights : AnimationData->Crouch_FireSingle;
				else
					Anims = bIsAiming ? AnimationData->FireSingleSights : AnimationData->FireSingle;
					
				if (Anims.Num() > 0)
				{
					const int32 Index = FMath::RandRange(0, Anims.Num()-1);
					PlayItemAnimation(Anims[Index], true, bOnlyLocal, !bOnlyLocal);
				}
			}
		}
		else if (bUseFireLoopAnims)
		{
			if (bIsCrouched)
				PlayItemAnimation(bIsAiming ? AnimationData->Crouch_FireLoopSights : AnimationData->Crouch_FireLoop, false, bOnlyLocal, !bOnlyLocal);
			else
				PlayItemAnimation(bIsAiming ? AnimationData->FireLoopSights : AnimationData->FireLoop, false, bOnlyLocal, !bOnlyLocal);
		}
	}
}

void ABaseMagazineWeapon::PlayDryFireAnimation(bool bIsAiming, const bool bIsCrouched, const bool bOnlyLocal)
{
	if (!IsPistolWithShield())
	{
		PlayItemAnimation(bIsCrouched ? AnimationData->Crouch_Dryfire : AnimationData->DryFire, true, bOnlyLocal, !bOnlyLocal);
	}
}

FRotator ABaseMagazineWeapon::CalculateRecoil()
{
	float AttachmentSpreadMultiplier = 1.0f;
	float AttachmentHorizontalRecoilMultiplier = 1.0f;
	float AttachmentVerticalRecoilMultiplier = 1.0f;

	TArray<UWeaponAttachment*> weaponAttachments;
	GetComponents(weaponAttachments);
	for (int32 i = 0; i < weaponAttachments.Num(); i++)
	{
		UWeaponAttachment* attachment = Cast<UWeaponAttachment>(weaponAttachments[i]);
		if (attachment)
		{
			AttachmentHorizontalRecoilMultiplier *= attachment->HorizontalRecoilMultiplier;
			AttachmentVerticalRecoilMultiplier *= attachment->VerticalRecoilMultiplier;
			AttachmentSpreadMultiplier *= attachment->SpreadMultiplier;
		}
	}

	PendingSpread += GetSpread() * (IsFirstShot() ? FirstShotSpread : 1.0f) * AttachmentSpreadMultiplier;
			
	bReturnRecoil = false;

	float RecoilOvertimeMultiplierPitch = 1.0f;
	float RecoilOvertimeMultiplierYaw = 1.0f;
			
	RecoilOvertimeMultiplierPitch += FMath::Clamp(float(RecentShotsFired), 0.0f, float(AmmoMax)) * RecoilMultiplierPitch;
	RecoilOvertimeMultiplierYaw += FMath::Clamp(float(RecentShotsFired), 0.0f, float(AmmoMax)) * RecoilMultiplierYaw;
			
	FRotator RecoilToApply = GetRecoil() * (IsFirstShot() ? FirstShotRecoil : 1.0f);

	RecoilToApply.Pitch *= AttachmentVerticalRecoilMultiplier * RecoilOvertimeMultiplierPitch;
	RecoilToApply.Yaw *= AttachmentHorizontalRecoilMultiplier * RecoilOvertimeMultiplierYaw;

	return RecoilToApply;
}

void ABaseMagazineWeapon::Multicast_SimulateFireForViewTargets_Implementation(FVector_NetQuantize100 DirectionNet, FVector_NetQuantize100 SpawnLoc, int32 Seed)
{
	const FRotator Direction = DirectionNet.Rotation();
	APlayerController* pc = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld());
	if (pc)
	{
		if (pc->GetViewTarget() == GetOwner() && !IsLocallyControlled() && GetLocalRole() < ROLE_Authority)
		{
			LocallySimulateFire(Direction, SpawnLoc, Seed);
		}
	}
}

void ABaseMagazineWeapon::Server_OnFire_Implementation(FRotator Direction, FVector SpawnLoc, int32 Seed)
{
#if !UE_BUILD_SHIPPING
	if (GetAmmo() > 0.0f && CVarRonServerNoAmmo.GetValueOnAnyThread() == 0)
#else
	if (GetAmmo() > 0.0f)
#endif
	{
		RefireDelayTimer = FireRate + RefireDelay;
		TimeSinceLastShot = 0.0f;

		#if !UE_BUILD_SHIPPING
		if (CVarRonNoBullets.GetValueOnAnyThread() == 0)
		{
		#endif
			FTransform SpawnTransform;
			SpawnTransform.SetRotation(Direction.GetNormalized().Quaternion());
			SpawnTransform.SetLocation(SpawnLoc);

			FRandomStream RandomStream(Seed);
			for (int32 i = 0; i < SpawnProjectileCount; i++)
			{
				int32 ProjectileSeed = RandomStream.GetUnsignedInt();
				if (UReadyOrNotGameInstance::bIsBuildPirated)
					SpawnProjectile(FakeBulletProjectile, SpawnTransform, false, i, ProjectileSeed);
				else
					SpawnProjectile(bArmorPiercing && ArmorPiercingBulletProjectile ? ArmorPiercingBulletProjectile : BulletProjectile, SpawnTransform, false, i, ProjectileSeed);
			}

			// We know the bullet is valid at this point, can send validation to firing client
			AReadyOrNotPlayerController* Controller = Cast<AReadyOrNotPlayerController>(GetOwningPlayerController());
			if (Controller && !Controller->IsLocalPlayerController())
				Controller->Client_OnProjectileValidation(Seed, EServerValidationState::Validated);

			// RemoveAmmo may modify the current ammo type, i.e. if magazine has a different ammo type loaded than the chambered ammo
			// Prematurely changing the ammo type will cause the resulting shot to use the incorrect ammo type
			if (GetLocalRole() >= ROLE_Authority)
			{
				BulletsFired++;
				RemoveAmmo(1);
			}

			Multicast_OnFire(Direction.Vector(), SpawnLoc, Seed);

			if (GetOwnerCharacter())
			{
				GetOwnerCharacter()->OnEquippedWeaponFire(this, true);
			}
		#if !UE_BUILD_SHIPPING
		}
		#endif

		ACyberneticCharacter* CyberneticCharacter = Cast<ACyberneticCharacter>(GetOwner());
        if (CyberneticCharacter && CyberneticCharacter->GetCyberneticsController())
        {
        	CyberneticCharacter->GetCyberneticsController()->TotalBulletsFired++;
        	CyberneticCharacter->GetCyberneticsController()->BulletsFiredTowardsAccuracyPenalty++;
        }
		
		OnWeaponFire.Broadcast(this, true);
		Native_OnWeaponFire.ExecuteIfBound();

		UAISense_Hearing::ReportNoiseEvent(GetWorld(), GetActorLocation(), 2.5f, this, bSupressed ? 1500.0f : 2000.0f, bSupressed ? "SuppressedGunshot" : "Gunshot");
	}
	else
	{
		if (GetOwnerPlayerCharacter())
		{
			GetOwnerPlayerCharacter()->OnEquippedWeaponDryFire(this, true);
		}
		
		OnWeaponDryFire.Broadcast(this, true);

		// Bullet invalid on client, ensure they know this
		AReadyOrNotPlayerController* Controller = Cast<AReadyOrNotPlayerController>(GetOwningPlayerController());
		if (Controller)
			Controller->Client_OnProjectileValidation(Seed, EServerValidationState::Invalid);
	}
}

void ABaseMagazineWeapon::Multicast_OnFire_Implementation(FVector_NetQuantize100 DirectionNet, FVector_NetQuantize100 SpawnLoc, int32 Seed)
{
	Multicast_SpawnParticleEffects_Implementation(false, true);
	Multicast_SimulateFireForViewTargets_Implementation(DirectionNet, SpawnLoc, Seed);
	Multicast_HandleSupression_Implementation();

	SpawnShell(false);
}

void ABaseMagazineWeapon::OnItemPrimaryUseEnd()
{
	Super::OnItemPrimaryUseEnd();
	
	if (FiringAudioComp)
	{
		// We've stopped holding down primary fire, set FireMode parameter so our shot tail can play if needed and unreference
		FiringAudioComp->SetParameter("FireMode", 0.0f);
		FiringAudioComp = nullptr;
	}
	
	APlayerCharacter* pc = Cast<APlayerCharacter>(GetOwner());
	if (pc)
	{
		if (bUseFireLoopAnims)
		{
			if (AnimationData)
			{
				if (InFullAutoMode() || InBurstMode())
				{
					if (!pc->bIsCrouched)
					{
						PlayItemAnimation(pc->bAiming ? AnimationData->FireLoopSightsEnd : AnimationData->FireLoopEnd);
					}
					else
					{
						PlayItemAnimation(pc->bAiming ? AnimationData->Crouch_FireLoopSightsEnd : AnimationData->Crouch_FireLoopEnd);
					}
				}
			}
		}
	}
}

void ABaseMagazineWeapon::Multicast_SpawnShell_Implementation(bool bOnlyLocallyControlled, bool bSkipLocallyControlled)
{
	if (!IsLocallyControlled() && bOnlyLocallyControlled)
		return;

	if (IsLocallyControlled() && bSkipLocallyControlled)
		return;

	if (ShellClass)	
	{
		/* 
		SpawnedShells.Remove(nullptr);
		
		//Checks if shell is destroyed but still in memory
		for (int i = 0; i < SpawnedShells.Num(); i++)
		{
			if (SpawnedShells[i]->IsPendingKill()) {
				SpawnedShells.RemoveAt(i);
			}
		}			
		*/
		
		/*
		if (SpawnedShells.Num() < 15)
		{
			Shell = GetWorld()->SpawnActorDeferred<ABaseShell>(ShellClass, ShellSpawn->GetComponentTransform());
			Shell->FinishSpawning(Shell->GetActorTransform());
			SpawnedShells.Add(Shell);
		}
		else
		{
			Shell = SpawnedShells[0];
			SpawnedShells.RemoveAt(0);
			SpawnedShells.Add(Shell);
		}
		*/

		FName ShellPoolName = NAME_None;
		switch (ShellClass->GetDefaultObject<ABaseShell>()->ShellType)
		{
			case EShellType::Bullet: ShellPoolName = "Bullet Shell Pool"; break;
			case EShellType::Shotgun: ShellPoolName = "Shotgun Shell Pool"; break;
		}
		
		if (UObjectPoolBase* ShellPool = UObjectPoolFunctionLibrary::GetObjectPool(this, ShellPoolName))
		{
			if (ABaseShell* Shell = ShellPool->GetActorFromPool<ABaseShell>())
			{
				Shell->PooledActor_BeginPlay();
				
				if (bOnlyLocallyControlled)
				{
					Shell->ShellMesh->SetOnlyOwnerSee(true);
				}
				
				Shell->SetReplicates(false);
				Shell->Tags.Add("Shell");
				Shell->SetOwner(GetOwner());
				Shell->PropelFromGun(this);

				for (UMaterialInstanceDynamic* Dyn : Shell->MID_ShellMesh)
				{
					Dyn->SetScalarParameterValue("DisableWeaponFOV", 0);
				}
			}
		}
	}
}

void ABaseMagazineWeapon::OnWeaponReloadComplete()
{
	bReloading = false;
	Server_SetReloading(false);
	Server_NextMagazine();
}

FVector ABaseMagazineWeapon::GetLocation_Implementation() const
{
	return ItemMesh->GetBoneLocation("J_Gun");
}

/*
ERemainingAmmoDisplay ABaseMagazineWeapon::GetRemainingAmmoDisplay()
{
	return GetAmmoDisplayForAmount(GetAmmo());
}

ERemainingAmmoDisplay ABaseMagazineWeapon::GetNextRemainingAmmoDisplay()
{
	return GetAmmoDisplayForAmount(GetNextAmmo());
}

ERemainingAmmoDisplay ABaseMagazineWeapon::GetAmmoDisplayForAmount(float Amount)
{
	if (Amount == AmmoMax)
	{
		return ERemainingAmmoDisplay::RAD_Full;
	}
	else if (Amount <= 0.0f)
	{
		return ERemainingAmmoDisplay::RAD_Empty;
	}
	else if (Amount >= AmmoMax * 0.70f)
	{
		return ERemainingAmmoDisplay::RAD_NearFull;
	}
	else if (Amount >= AmmoMax * 0.35f)
	{
		return ERemainingAmmoDisplay::RAD_Half;
	}
	else
	{
		return ERemainingAmmoDisplay::RAD_NearEmpty;
	}
	return ERemainingAmmoDisplay::RAD_None;
}
*/

void ABaseMagazineWeapon::SpawnShell(const bool bOnlyLocal)
{
	if (bSpawnShell && SpawnShellDelay == 0.0f)
	{
		if (bOnlyLocal)
			SpawnLocalShell();
		else
			SpawnRemoteShells();
	}
	else if (bSpawnShell && SpawnShellDelay > 0.0f)
	{
		if (bOnlyLocal)
			GetWorld()->GetTimerManager().SetTimer(SpawnLocalShell_Handle, this, &ABaseMagazineWeapon::SpawnLocalShell, SpawnShellDelay, false);
		else
			GetWorld()->GetTimerManager().SetTimer(SpawnRemoteShells_Handle, this, &ABaseMagazineWeapon::SpawnRemoteShells, SpawnShellDelay, false);
	}
}

void ABaseMagazineWeapon::SpawnLocalShell()
{
	Multicast_SpawnShell_Implementation(true, false);
}

void ABaseMagazineWeapon::SpawnRemoteShells()
{
	Multicast_SpawnShell_Implementation(false, true);
}

bool ABaseMagazineWeapon::CanReload()
{
	// require additional magazine to reload too....
	if (Magazines.Num() > 1)
		return Super::CanReload();
	return false;
}

float ABaseMagazineWeapon::GetAmmoInMagazine(int32 Index) const
{
	if (Magazines.IsValidIndex(Index))
		return Magazines[Index].Ammo;

	return 0.0f;
}

FMagazine ABaseMagazineWeapon::GetCurrentMagazine() const
{
	if (Magazines.IsValidIndex(MagIndex))
		return Magazines[MagIndex];

	return FMagazine(-1, -1);
}

float ABaseMagazineWeapon::GetMagazineAmmoPercentage(int32 MagazineIndex) const
{
	return FMath::Clamp(GetAmmoInMagazine(MagazineIndex) / AmmoMax, 0.0f, 1.0f);
}

FText ABaseMagazineWeapon::GetMagazineScreenName(const FMagazine& Magazine) const
{
	if (!AmmunitionTypes.IsValidIndex(Magazine.AmmoType))
		return FText();
	
	if (!AmmoDataTable)
		return FText();

	FAmmoTypeData* AmmoTypeData = AmmoDataTable->FindRow<FAmmoTypeData>(AmmunitionTypes[Magazine.AmmoType], "Ammo Type Screen Name");
	if (!AmmoTypeData)
		return FText();

	return AmmoTypeData->AmmoVariety;
}

bool ABaseMagazineWeapon::HasAnyAmmo() const
{
	for (int32 i = 0; i < Magazines.Num(); i++)
	{
		if (Magazines[i].Ammo > 0)
			return true;
	}
	return false;
}

bool ABaseMagazineWeapon::HasAnyAmmoOfType(FName AmmoType) const
{
	for (int32 i = 0; i < Magazines.Num(); i++)
	{
		if (AmmoType.IsEqual(AmmunitionTypes[Magazines[i].AmmoType]) && Magazines[i].Ammo > 0)
			return true;
	}
	return false;
}

bool ABaseMagazineWeapon::HasAmmo() const
{
	return GetAmmo() > 0;
}

float ABaseMagazineWeapon::RemoveAmmo(float Value)
{
	#if !UE_BUILD_SHIPPING
	if (CHECK_DEBUG_SUBSYSTEM)
	{
		if (DEBUG_SUBSYSTEM->bInfiniteAmmo)
			return AmmoMax;
	}
	#endif

	if (bInfiniteAmmo)
		return AmmoMax;
	
	if (Magazines.IsValidIndex(MagIndex))
	{
		Magazines[MagIndex].Ammo = FMath::Max(Magazines[MagIndex].Ammo - Value, 0.0f);
		SetAmmunitionType(QueuedAmmoType); // Chamber emptied, set next round

		return Magazines[MagIndex].Ammo;
	}
	return 0;
}

bool ABaseMagazineWeapon::AllMagsEmpty() const
{
	for (const FMagazine& Mag : Magazines)
	{
		if (Mag.Ammo > 0)
			return false;
	}
	
	return true;
}

bool ABaseMagazineWeapon::IncrementAmmoType()
{
	// Only cycle through available ammo types, otherwise we get stuck
	TArray<int32> AvailableAmmoTypes;
	for (FMagazine& Magazine : Magazines)
	{
		AvailableAmmoTypes.AddUnique(Magazine.AmmoType);
	}

	// Get the index of the current ammo type in the available ammo types array
	int32 NextAmmoType = -1;
	if (Magazines.IsValidIndex(MagIndex))
	{
		NextAmmoType = AvailableAmmoTypes.IndexOfByKey(Magazines[MagIndex].AmmoType);
	}

	NextAmmoType++;
	NextAmmoType = NextAmmoType > AvailableAmmoTypes.Num() - 1 ? 0 : NextAmmoType;

	if (!AvailableAmmoTypes.IsValidIndex(NextAmmoType))
		return false;
	
	// Set the desired ammo type index
	const int32 AmmoTypeIndex = AvailableAmmoTypes[NextAmmoType];

	if (!AmmunitionTypes.IsValidIndex(AmmoTypeIndex))
		return false;

	DesiredAmmoType = AmmoTypeIndex;
	Server_SetDesiredAmmoTypeIndex(DesiredAmmoType);

	return true;
}

bool ABaseMagazineWeapon::IncrementAmmoType(FName AmmoType)
{
	
	int i;
	for (i = 0; i < GetAmmunitionTypes().Num(); i++)
	{
		if (GetAmmunitionTypes()[i].IsEqual(AmmoType))
		{
			break;
		}
	}

	if (!AmmunitionTypes.IsValidIndex(i))
		return false;

	if (GetCurrentAmmoTypeRowName() == AmmoType)
	{
		DesiredAmmoType = GetAmmunitionTypes().IndexOfByKey(GetCurrentAmmoTypeRowName());
		Server_SetDesiredAmmoTypeIndex(DesiredAmmoType);
		return false;
	}

	DesiredAmmoType = i;
	Server_SetDesiredAmmoTypeIndex(DesiredAmmoType);
	SetAmmunitionType(AmmoType);
	return true;
}

void ABaseMagazineWeapon::Server_NextMagazine_Implementation()
{
	// Unlimited ammo for SWAT AI
	ACyberneticCharacter* ai = Cast<ACyberneticCharacter>(GetOwner());
	if (ai && ai->IsOnSWATTeam())
	{
		if (!Magazines.IsValidIndex(MagIndex))
			Magazines.Init(FMagazine(AmmoMax, 0), 1);
			
		if (Magazines.IsValidIndex(MagIndex))
			Magazines[MagIndex].Ammo = AmmoMax;

		bReloading = false;
		return;
	}
	
	if (GetLocalRole() >= ROLE_Authority && GetRemoteRole() != GetLocalRole())
	{
		FindNextMagIndex();
	}

	// Check if there is still a bullet in the chamber before modifying mags
	bool bBulletInChamber = false;
	if (Magazines.IsValidIndex(MagIndex))
	{
		if (Magazines[MagIndex].Ammo > 0)
			bBulletInChamber = true;
	}

	// Determine the ammo type that should be fired after the next shot
	if (Magazines.IsValidIndex(NextMagIndex))
	{
		QueuedAmmoType = Magazines[NextMagIndex].AmmoType;
	}

	// If chamber is empty, go ahead and switch ammo type
	if (!bBulletInChamber)
	{
		SetAmmunitionType(QueuedAmmoType);
	}

	if (bBulletInChamberOnReload)
	{
		if (Magazines.IsValidIndex(MagIndex) && Magazines.IsValidIndex(NextMagIndex))
		{
			// only add bullet if there is still a bullet in the gun
			if (Magazines[MagIndex].Ammo > 0)
			{
				// remove bullet from magazine as it's in the gun
				Magazines[MagIndex].Ammo--;
				// add bullet to next magazine as it's in the gun
				Magazines[NextMagIndex].Ammo++;
			}
		}
	}

	// Remove the current magazine if desired (and shift next magazine index to match)
	if ((bLoseMagOnReload || !bTacticalReload) && !bOverrideLoseMagOnReload)
	{
		if (Magazines.IsValidIndex(MagIndex))
		{
			Magazines.RemoveAt(MagIndex);
			if (NextMagIndex > MagIndex)
			{
				NextMagIndex--;
			}
		}
	}

	// Set the next magazine
	MagIndex = NextMagIndex;

	bOverrideLoseMagOnReload = false;
}

void ABaseMagazineWeapon::Server_SetReloading_Implementation(bool bIsReloading)
{
	bReloading = bIsReloading;
}

void ABaseMagazineWeapon::OnItemSecondaryUsed()
{
	Super::OnItemSecondaryUsed();
	
}

void ABaseMagazineWeapon::OnItemEndSecondaryUse()
{
	Super::OnItemEndSecondaryUse();
	
}

void ABaseMagazineWeapon::Server_SetDesiredAmmoTypeIndex_Implementation(int32 Index)
{
	if (AmmunitionTypes.IsValidIndex(Index))
	{
		DesiredAmmoType = Index;
	}
}

void ABaseMagazineWeapon::FindNextMagIndex()
{
	int32 FirstIndexWithHighestAmmo = MagIndex;
	float HighestAmmo = 0.0f;

	// First look for the magazine matching our current ammo type with the highest ammo
	for (int32 i = 0; i < Magazines.Num(); i++)
	{
		if (Magazines[i].AmmoType != DesiredAmmoType)
			continue;

		if (HighestAmmo < Magazines[i].Ammo && i != MagIndex)
		{
			FirstIndexWithHighestAmmo = i;
			HighestAmmo = Magazines[i].Ammo;
		}
	}

	// If we found one, then use that and exit out
	if (FirstIndexWithHighestAmmo != MagIndex)
	{
		NextMagIndex = FirstIndexWithHighestAmmo;
		return;
	}

	// Otherwise find any magazine with the highest amount of ammo available
	for (int32 i = 0; i < Magazines.Num(); i++)
	{
		if (HighestAmmo < Magazines[i].Ammo && i != MagIndex)
		{
			FirstIndexWithHighestAmmo = i;
			HighestAmmo = Magazines[i].Ammo;
		}
	}

	NextMagIndex = FirstIndexWithHighestAmmo;
}

void ABaseMagazineWeapon::CancelCurrentReloadAction_Implementation(bool bCancel)
{
	bCancelReloading = bCancel;
	Server_SetCancelReloading(bCancel);
}

void ABaseMagazineWeapon::OnWeaponReload(bool bForce)
{
	APlayerCharacter* pc = Cast<APlayerCharacter>(GetOwner());
	if (!pc)
		return;

	if (!bForce)
	{
		if (IsBlockingAnimationPlaying())
			return;
	}
	
	Server_SetTacticalReload(false);

	if (!CanReload())
		return;
	
	FindNextMagIndex();

	if (NextMagIndex == MagIndex && !bCanReloadSameMagazine)
	{
		return;
	}
	
	if (GetAmmo() <= 0 && HasAnyAmmo())
	{
		bReloading = true;
		Server_SetReloading(true);

		//if (bReloading && GetAmmo() == 0)
		//{
			//pc->Server_PlayPVPSpeech("ReloadingWeapon", pc->GetTeam());
		//}

		if (SoundData)
		{
			PlayFMODAudio(SoundData->QuickReloadEmpty_FullSeq);
		}
		
		if (AnimationData)
		{
			if (AnimationData->bEmptyReload)
			{
				EEmptyMagReloadType EmptyMagReload;
				UBpGameplayHelperLib::LoadEmptyMagReloadSettings(EmptyMagReload);

				//if (EmptyMagReload == EEmptyMagReloadType::FastReload)
				//{
				PlayTPMontage(AnimationData->ReloadEmpty.Gun_TP);
				pc->Play3PMontage(AnimationData->ReloadEmpty.Body_TP);
				PlayFPMontage(AnimationData->ReloadEmpty.Gun_FP);

				// Get out of ADS when empty reloading
				pc->Server_UpdateADS(false);
				pc->Server_UpdateADS_Implementation(false);
				OnEndAimDownSights(pc->bWasAiming);
				pc->Play1PMontage(AnimationData->ReloadEmpty.Body_FP);
				//}
				//else
				//{
				//	PlayTPMontage(AnimationData->Tactical_Reload.Gun_TP);
				//	pc->Play3PMontage(AnimationData->Tactical_Reload.Body_TP);
				//	PlayFPMontage(AnimationData->Tactical_Reload.Gun_FP);
				//
				//	// Get out of ADS when fast reloading
				//	pc->Server_UpdateADS(false);
				//	pc->Server_UpdateADS_Implementation(false);
				//	OnEndAimDownSights(pc->bWasAiming);
				//	pc->Play1PMontage(AnimationData->Tactical_Reload.Body_FP);
				//}
			}
			else
			{
				PlayTPMontage(AnimationData->Reload.Gun_TP);
				pc->Play3PMontage(AnimationData->Reload.Body_TP);
				PlayFPMontage(AnimationData->Reload.Gun_FP);

				if (!pc->bAiming) pc->Play1PMontage(AnimationData->Reload.Body_FP);
				else pc->Play1PMontage(AnimationData->Reload_FP_Ads);
			}
		}
	}
	else if (HasAnyAmmo())
	{
		bReloading = true;
		Server_SetReloading(true);

		if (SoundData)
		{
			PlayFMODAudio(SoundData->QuickReload_FullSeq);
		}
		
		if (AnimationData)
		{
			PlayTPMontage(AnimationData->Reload.Gun_TP);
			pc->Play3PMontage(AnimationData->Reload.Body_TP);
			PlayFPMontage(AnimationData->Reload.Gun_FP);

			// Get out of ADS when fast reloading
			pc->Server_UpdateADS(false);
			pc->Server_UpdateADS_Implementation(false);
			OnEndAimDownSights(pc->bWasAiming);
			pc->Play1PMontage(AnimationData->Reload.Body_FP);
		}
	}
}

void ABaseMagazineWeapon::OnWeaponTacticalReload()
{
	APlayerCharacter* pc = Cast<APlayerCharacter>(GetOwner());
	AReadyOrNotCharacter* rc = Cast<AReadyOrNotCharacter>(GetOwner());
	if (!rc)
		return;

	if (IsBlockingAnimationPlaying())
		return;

	Server_SetTacticalReload(true);

	if (!CanReload())
		return;

	FindNextMagIndex();
	
	constexpr float ReloadSpeed = 1.0f;
	if (GetAmmo() <= 0 && HasAnyAmmo())
	{
		bReloading = true;
		Server_SetReloading(true);

		if (SoundData)
		{
			PlayFMODAudio(SoundData->ReloadEmpty_FullSeq);
		}
		
	
		if (AnimationData)
		{
			if (AnimationData->bEmptyReload)
			{
				PlayTPMontage(AnimationData->Tactical_ReloadEmpty.Gun_TP, ReloadSpeed);
				rc->Play3PMontage(AnimationData->Tactical_ReloadEmpty.Body_TP, 0.0f, ReloadSpeed);
				PlayFPMontage(AnimationData->Tactical_ReloadEmpty.Gun_FP, ReloadSpeed);
				
				if (pc && !pc->bAiming) pc->Play1PMontage(AnimationData->Tactical_ReloadEmpty.Body_FP, ReloadSpeed);
				else rc->Play1PMontage(AnimationData->Tactical_ReloadEmpty_FP_Ads, ReloadSpeed);
			}
			else
			{
				PlayTPMontage(AnimationData->Tactical_Reload.Gun_TP, ReloadSpeed);
				rc->Play3PMontage(AnimationData->Tactical_Reload.Body_TP, 0.0f, ReloadSpeed);
				PlayFPMontage(AnimationData->Tactical_Reload.Gun_FP, ReloadSpeed);

				if (pc && !pc->bAiming) pc->Play1PMontage(AnimationData->Tactical_Reload.Body_FP, ReloadSpeed);
				else rc->Play1PMontage(AnimationData->Tactical_Reload_FP_Ads, ReloadSpeed);
			}

		}

	}
	else if (HasAnyAmmo())
	{
		bReloading = true;
		Server_SetReloading(true);

		if (SoundData)
		{
			PlayFMODAudio(SoundData->Reload_FullSeq);
		}

		if (AnimationData)
		{
			PlayTPMontage(AnimationData->Tactical_Reload.Gun_TP, ReloadSpeed);
			rc->Play3PMontage(AnimationData->Tactical_Reload.Body_TP, 0.0f, ReloadSpeed);
			PlayFPMontage(AnimationData->Tactical_Reload.Gun_FP, ReloadSpeed);
			
			if (pc && !pc->bAiming) pc->Play1PMontage(AnimationData->Tactical_Reload.Body_FP, ReloadSpeed);
			else rc->Play1PMontage(AnimationData->Tactical_Reload_FP_Ads, ReloadSpeed);
		}
	}
}

void ABaseMagazineWeapon::Server_AddMagazine_Implementation(FMagazine Magazine)
{
	if (!AmmunitionTypes.IsValidIndex(Magazine.AmmoType))
	{
		// TODO(killo): try to convert between weapon ammo types where it should be possible
		// i.e. if a gun takes the same 556 round, then try to convert ammo type index to match (see PickupMagazineActor)
		Magazine.AmmoType = 0;
	}

	Magazines.Add(Magazine);
}

void ABaseMagazineWeapon::MagCheck()
{
	APlayerCharacter* pc = Cast<APlayerCharacter>(GetOwner());
	if (!pc)
		return;

	if (IsBlockingAnimationPlaying())
		return;

	// Play mag check animation.
	if (AnimationData)
	{
		// check a magazine check anim exists before spawning the mag check..
		/*if (AnimationData->MagazineCheck.Body_FP)
		{
			SpawnMagCheckUI();
		}*/

		if (SoundData)
		{
			PlayFMODAudio(SoundData->MagCheck_FullSeq);
		}

		if (!pc->bIsCrouched)
		{
			if (!pc->bAiming)
			{
				PlayItemAnimation(AnimationData->MagazineCheck);
				
				/*
				PlayTPMontage(AnimationData->MagazineCheck.Gun_TP);
				pc->Play3PMontage(AnimationData->MagazineCheck.Body_TP);
				PlayFPMontage(AnimationData->MagazineCheck.Gun_FP);
				pc->Play1PMontage(AnimationData->MagazineCheck.Body_FP);*/
			}
			else
			{
				PlayItemAnimation(AnimationData->MagazineCheckSights);

				/*
				PlayTPMontage(AnimationData->MagazineCheck.Gun_TP);
				pc->Play3PMontage(AnimationData->MagazineCheckSights.Body_TP);
				PlayFPMontage(AnimationData->MagazineCheckSights.Gun_FP);
				pc->Play1PMontage(AnimationData->MagazineCheckSights.Body_FP);*/
			}
		}
		else
		{
			if (!pc->bAiming)
			{
				PlayItemAnimation(AnimationData->Crouch_MagazineCheck);

				/*
				PlayTPMontage(AnimationData->Crouch_MagazineCheck.Gun_TP);
				pc->Play3PMontage(AnimationData->Crouch_MagazineCheck.Body_TP);
				PlayFPMontage(AnimationData->Crouch_MagazineCheck.Gun_FP);
				pc->Play1PMontage(AnimationData->Crouch_MagazineCheck.Body_FP);*/
			}
			else
			{
				PlayItemAnimation(AnimationData->Crouch_MagazineCheckSights);

				/*
				PlayTPMontage(AnimationData->Crouch_MagazineCheckSights.Gun_TP);
				pc->Play3PMontage(AnimationData->Crouch_MagazineCheckSights.Body_TP);
				pc->Play1PMontage(AnimationData->Crouch_MagazineCheckSights.Body_FP);
				PlayFPMontage(AnimationData->Crouch_MagazineCheckSights.Gun_FP);*/
			}
		}

		if (AnimationData->MagazineCheck.Body_FP)
		{
			// need this to enable some IK stuff again in the graph when the mag check is done
			FTimerHandle TempHandle;
			GetWorld()->GetTimerManager().SetTimer(TempHandle, this, &ABaseMagazineWeapon::MagCheckDone, AnimationData->MagazineCheck.Body_FP->GetPlayLength(), false);
		}
	}

	if (GetOwnerPlayerCharacter())
	{
		GetOwnerPlayerCharacter()->OnEquippedWeaponMagCheck(this);
	}

	OnWeaponMagCheck.Broadcast(this);
}

void ABaseMagazineWeapon::MagCheckDone()
{
}

/*
void ABaseMagazineWeapon::SpawnMagCheckUI()
{
 	if (UBpGameplayHelperLib::GetWidgetData()->MagCheckUI)
 	{
 		APlayerCharacter* pc = Cast<APlayerCharacter>(GetOwner());
 		if (pc && Cast<APlayerController>(pc->GetController()))
 		{
 			if (UUserWidget* MagCheckUI = CreateWidget<UUserWidget>(GetWorld(), UBpGameplayHelperLib::GetWidgetData()->MagCheckUI))
 			{
 				MagCheckUI->AddToViewport(0);
 			}
 		}
 	}
}
*/

void ABaseMagazineWeapon::Multicast_PerformHitscan_Implementation(const FHitscanShot& HitscanShot, bool bLocalOnly, int32 Seed)
{
	SCOPE_CYCLE_COUNTER(STAT_DoHitScan);

	TRACE_BOOKMARK(TEXT("DoHitScan"));
	
	if (!bLocalOnly && IsLocallyControlled() && GetLocalRole() != ROLE_Authority)
		return;

	// Make it more difficult to abuse client trusted seed (though its just for ricochets)
	int32 FinalSeed = Seed;
	FinalSeed += HitscanShot.Location.X + HitscanShot.Location.Y + HitscanShot.Location.Z;
	FinalSeed += HitscanShot.Direction.X + HitscanShot.Direction.Y + HitscanShot.Direction.Z;
	FRandomStream RandomStream = FRandomStream(FinalSeed);
	
	TArray<FHitResult> OutHits;
	TArray<FHitResult> OutReverseHits;
	TArray<FHitResult> ReverseHitsReordered;

	FVector TraceStart = HitscanShot.Location;
	FVector TraceEnd = HitscanShot.Location + HitscanShot.Direction * 100000.0f;
	CalculateSuppressionForAI(TraceStart, TraceEnd);

	FCollisionQueryParams CollisionQueryParams;
	CollisionQueryParams.bTraceComplex = true;
	CollisionQueryParams.bReturnPhysicalMaterial = true;
	CollisionQueryParams.AddIgnoredActor(this);
	CollisionQueryParams.AddIgnoredActor(GetOwner());
	
	if (GetOwnerCharacter())
	{
		CollisionQueryParams.AddIgnoredActors((TArray<AActor*>)GetOwnerCharacter()->GetInventoryComponent()->GetInventoryItems());
	}
	
	FCollisionResponseParams CollisionResponseParams;
	CollisionResponseParams.CollisionResponse.SetAllChannels(ECR_Overlap);
	
	GetWorld()->LineTraceMultiByChannel(OutHits, TraceStart, TraceEnd, ECC_PROJECTILE, CollisionQueryParams, CollisionResponseParams);
	GetWorld()->LineTraceMultiByChannel(OutReverseHits, TraceEnd, TraceStart, ECC_PROJECTILE, CollisionQueryParams, CollisionResponseParams);
	
	const FAmmoTypeData* ShotAmmoType = GetCurrentAmmoType();
	UPenetrationData* PenetrationData = UBpGameplayHelperLib::GetPenetrationData();
	
	// Shot ammo type penetration distance in millimeters, Unreal units in centimeters
	const float MaxPenetrationDistance = ShotAmmoType ? (ShotAmmoType->PenetrationDistance * 0.1f) : (PenetrationDistance * 2.0f);
	float TotalPenetrationDistance = 0.0f;
	
	TArray<FRicochet> Ricochets;
	
	constexpr int32 MaxBounces = 3;
	int32 Bounces = 0;
	float BounceChance = ShotAmmoType ? ShotAmmoType->RicochetChance : 0.0f;
	int32 LastBounceIndex = 0;
	
	bool bLastHitPenetrated = false;
	float HitSurfaceSpallingChance = 0.0f;
	
	for (int32 i = 0; i < OutHits.Num(); i++)
	{
		FHitResult& HitResult = OutHits[i];
		HitResult.Distance = TotalPenetrationDistance; // HACK(killo): store the pen distance up to this point for later
		
		// Remove all remaining hits if we've run out of penetration power
		if (TotalPenetrationDistance > MaxPenetrationDistance)
		{
			OutHits.RemoveAt(i);
			if (ReverseHitsReordered.IsValidIndex(i))
			{
				ReverseHitsReordered.RemoveAt(i);
			}
			i--;
			continue;
		}

		// Get physical material and associated penetration data entry
		UPhysicalMaterial* HitPhysMat = HitResult.PhysMaterial.Get();
		EPhysicalSurface HitSurfaceType = UPhysicalMaterial::DetermineSurfaceType(HitPhysMat);
		FMaterialPenetration MaterialPenetration = PenetrationData ? PenetrationData->GetPenetrationData(HitSurfaceType) : FMaterialPenetration();

		#if !UE_BUILD_SHIPPING
		if (CVarRonDrawBallisticsDebug.GetValueOnAnyThread() != 0)
		{
			FVector LineStart = i > LastBounceIndex ? OutHits[i - 1].ImpactPoint : HitResult.TraceStart;
			FColor HitColor = FColor::MakeRedToGreenColorFromScalar(1.0f - (TotalPenetrationDistance / MaxPenetrationDistance));
			DrawDebugLine(GetWorld(), LineStart, HitResult.ImpactPoint, HitColor, false, 10.0f);
			DrawDebugString(GetWorld(), HitResult.ImpactPoint, FString::FromInt(i), nullptr, HitColor, 10.0f, false, 1.5);
		}
		#endif
		
		// Ricochet
		if (MaterialPenetration.bCanRicochet && Bounces < MaxBounces)
		{
			// TODO(killo): damage modifier after ricochet
			// TODO(killo): some damage from ricochet impacts themselves
			// TODO(killo): yaw dot product scaling
			
			FVector Direction = HitResult.TraceEnd - HitResult.TraceStart;
			Direction.Normalize();

			FVector Reflection = Direction - 2.0f * FVector::DotProduct(Direction, HitResult.Normal) * HitResult.Normal;
			Reflection.Normalize();
			
			// Energy modifier, lose more energy if we are deflecting back at higher angles
			// Then, modify our bounce chance based on that, slight angles mean ricochet more frequently
			float EnergyModifier = FMath::Clamp(1.0f - FVector::DotProduct(HitResult.Normal, Reflection), 0.0f, 1.0f);
			float FinalBounceChance = BounceChance * EnergyModifier * MaterialPenetration.RicochetChanceMultiplier;
			FinalBounceChance *= 1.0f - FMath::Clamp(TotalPenetrationDistance / MaxPenetrationDistance, 0.0f, 1.0f);
			
			if (UKismetMathLibrary::RandomBoolWithWeightFromStream(FinalBounceChance, RandomStream))
			{
				// If we bounced, reduce max pen distance, steeper angles heavily reducing our max penetration
				TotalPenetrationDistance += MaxPenetrationDistance * (1.0f - EnergyModifier);

				TArray<FHitResult> NewOutHits;
				TArray<FHitResult> NewOutReverseHits;
				
				// Calculate the angle of the reflection from the normal, as well as randomized pitch/yaw offsets
				// TODO(killo): may need to modify rand values, maybe based on hit material
				float ReflectionAngle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(Reflection, HitResult.Normal)));
				float PitchOffset = FMath::ClampAngle(ReflectionAngle + UKismetMathLibrary::RandomFloatInRangeFromStream(-5.0f, 20.0f, RandomStream), 35.0f, 85.0f); 
				float YawOffset = UKismetMathLibrary::RandomFloatInRangeFromStream(-20.0f, 20.0f, RandomStream);

				// Create our final outgoing ricochet vector
				FVector Right = FVector::CrossProduct(Reflection, HitResult.Normal);
				FVector Outgoing = Reflection.RotateAngleAxis(PitchOffset - ReflectionAngle, Right);
				Outgoing = Outgoing.RotateAngleAxis(YawOffset, HitResult.Normal);
			
				FVector NewStart = HitResult.Location + Outgoing * 1.0f;
				FVector NewEnd = HitResult.Location + Outgoing * 100000.0f;

				// Keep ricochet data for later, before we delete the hit result
				FRicochet Ricochet;
				Ricochet.Location = HitResult.Location;
				Ricochet.Normal = HitResult.ImpactNormal;
				Ricochet.Direction = Outgoing;
				Ricochet.OutHitsIndex = i;
				Ricochets.Add(Ricochet);
				
				// Ricochets don't count as full hits, remove them from the normal hits
				OutHits.RemoveAt(i, OutHits.Num() - i);
				OutReverseHits.RemoveAt(0, OutReverseHits.Num() - i);

				LastBounceIndex = i;
				i--;

				GetWorld()->LineTraceMultiByChannel(NewOutHits, NewStart, NewEnd, ECC_PROJECTILE, CollisionQueryParams, CollisionResponseParams);
				GetWorld()->LineTraceMultiByChannel(NewOutReverseHits, NewEnd, NewStart, ECC_PROJECTILE, CollisionQueryParams, CollisionResponseParams);

				// Ensure we correctly add our post-ricochet hits to the existing hits
				OutHits.Append(NewOutHits);
				NewOutReverseHits.Append(OutReverseHits);
				OutReverseHits = NewOutReverseHits;
		
				Bounces++;

				#if !UE_BUILD_SHIPPING
				if (CVarRonDrawBallisticsDebug.GetValueOnAnyThread() != 0)
				{
					DrawDebugLine(GetWorld(), NewStart, NewStart + HitResult.Normal * 50.0f, FColor::Red, false, 10.0f, 0);
					DrawDebugLine(GetWorld(), NewStart, NewStart + (HitResult.TraceStart - HitResult.Location).GetSafeNormal() * 50.0f, FColor::Magenta, false, 10.0f, 0, 1.0f);
					DrawDebugLine(GetWorld(), NewStart, NewStart + Reflection * 50.0f, FColor::Yellow, false, 10.0f, 0, 1.0f);
					DrawDebugLine(GetWorld(), NewStart, NewStart + Outgoing * 50.0f, FColor::Cyan, false, 10.0f, 0, 1.0f);
					DrawDebugBox(GetWorld(), HitResult.Location, FVector(10.0f), FColor::Cyan, false, 10.0f);
					DrawDebugString(GetWorld(), HitResult.Location, FString::Printf(TEXT("Bounce %d"), Bounces), nullptr, FColor::White, 10.0f);
				}
				#endif

				continue;
			}
		}
		
		// Add our matching reverse hit
		if (OutReverseHits.IsValidIndex(OutReverseHits.Num() - i - 1))
		{
			ReverseHitsReordered.Add(OutReverseHits[OutReverseHits.Num() - i - 1]);
		}

		// Check penetration possible
		AReadyOrNotCharacter* Character = Cast<AReadyOrNotCharacter>(HitResult.GetActor());
		if (Character)
		{
			ABaseArmour* Armour = Character->GetArmourForBone(HitResult.BoneName);
			bLastHitPenetrated = Armour ? Armour->CheckPenetration(HitResult, ShotAmmoType, &HitSurfaceSpallingChance) : true;
		}
		else
		{
			HitSurfaceSpallingChance = MaterialPenetration.SpallingChance;
			
			bLastHitPenetrated = MaterialPenetration.bIsPenetrable &&
				(ShotAmmoType ? ShotAmmoType->PenetrationLevel >= MaterialPenetration.ArmourLevel : true);
		}
		
		// Penetration
		if (ReverseHitsReordered.IsValidIndex(i))
		{
			const FHitResult& ReverseHit = ReverseHitsReordered[i];
			
			float EntryDistance = FVector::DistSquared(HitResult.TraceStart, HitResult.ImpactPoint);
			float ExitDistance = FVector::DistSquared(HitResult.TraceStart, ReverseHit.ImpactPoint);
			bool bValidPenetration = EntryDistance < ExitDistance; // The entry should be closer to the origin than the exit...
			
			if (bValidPenetration)
			{
				if (bLastHitPenetrated && ((HitResult.GetActor() && !HitResult.GetActor()->IsA(ABallisticsShield::StaticClass())) || !HitResult.GetActor()))
				{
					UPhysicalMaterial* ReversePhysMat = ReverseHit.PhysMaterial.Get();
					EPhysicalSurface ReverseSurfaceType = UPhysicalMaterial::DetermineSurfaceType(ReversePhysMat);
					FMaterialPenetration ReverseMaterial = PenetrationData ? PenetrationData->GetPenetrationData(ReverseSurfaceType) : FMaterialPenetration();
					
					float SurfaceDepth = (HitResult.ImpactPoint - ReverseHit.ImpactPoint).Size();

					float EntryPenetration = MaterialPenetration.PenetrationDensity * (SurfaceDepth / 2.0f);
					float ExitPenetration = ReverseMaterial.PenetrationDensity * (SurfaceDepth / 2.0f);
					
					float FinalDistance = EntryPenetration + ExitPenetration;
					
					TotalPenetrationDistance += FinalDistance;
				} 
				else
				{
					TotalPenetrationDistance = BIG_DIST;
				}
			}
			else
			{
				TotalPenetrationDistance = BIG_DIST;
			}
		} 
		else
		{
			// Remove subsequent hits (invalid penetration)
			if (OutHits.Num() > (i + 1))
			{
				OutHits.RemoveAt(i + 1, OutHits.Num() - (i + 1));
				break;
			}
		}
	}

	PlayHitscanRicochetEffects(Ricochets);
	
	TArray<FHitResult> CombinedHits;
	CombinedHits.Append(OutHits);
	CombinedHits.Append(ReverseHitsReordered);
	
	int32 FirstEntryIndex = 0;
	int32 LastEntryIndex = OutHits.Num() - 1;
	int32 FirstExitIndex = OutHits.Num();
	int32 LastExitIndex = CombinedHits.Num() - 1;
	
	int32 HitCount = -1;
	float TotalDistance = 0.0f;
	
	FVector LastEntryLocation = TraceStart;
	
	TMap<AReadyOrNotCharacter*, FHitscanCharacterHit> CharacterHitScanMemoryMap;
	for (int32 i = 0; i < CombinedHits.Num(); i++)
	{
		const FHitResult& HitResult = CombinedHits[i];
		bool bIsEntryHit = i < FirstExitIndex;
		
		if (!HitResult.GetActor())
			continue;
		
		HitCount++;

		AReadyOrNotCharacter* HitCharacter = Cast<AReadyOrNotCharacter>(HitResult.GetActor());
		
		// Check if we already applied damage to this character, on the same component and on a similar bone
		// This is to prevent multiple damage inflictions per frame (on the same component/bone)
		if (IsValid(HitCharacter))
		{
			bool bAlreadyAppliedDamage = false;
			for (auto It : CharacterHitScanMemoryMap)
			{
				if (It.Key == HitResult.GetActor() &&
					It.Value.Component == HitResult.Component &&
					HitCharacter->AreBonesInSameGroup(It.Value.BoneName, HitResult.BoneName))
				{
					bAlreadyAppliedDamage = true;
					break;
				}
			}

			// Continue to the next hit result
			if (bAlreadyAppliedDamage)
				continue;

			FHitscanCharacterHit Info;
			Info.Component = HitResult.Component;
			Info.BoneName = HitResult.BoneName;
			
			CharacterHitScanMemoryMap.Add(HitCharacter, Info);
		}
		
		if (HitResult.GetComponent())
		{
			HitResult.GetComponent()->OnComponentHit.Broadcast(HitResult.GetComponent(), nullptr, nullptr, HitResult.ImpactNormal, HitResult);
		}
		
		if (GetOwnerCharacter() && GetOwnerCharacter()->GetController() && bIsEntryHit)
		{
			// Make sure we account for ricochet in totaldistance
			for (const FRicochet& Ricochet : Ricochets)
			{
				if (Ricochet.OutHitsIndex == i)
				{
					float Distance = FVector::Distance(Ricochet.Location, LastEntryLocation);
					TotalDistance += Distance;
					
					LastEntryLocation = Ricochet.Location;
					break;
				}
			}
			
			float Distance = FVector::Distance(HitResult.Location, LastEntryLocation);
			TotalDistance += Distance;
			
			LastEntryLocation = HitResult.Location;
			
			UAISense_Hearing::ReportNoiseEvent(GetWorld(), HitResult.ImpactPoint, 1.5f, this, 0.0f, "BulletHitSurface");

			float CurrentPenetration = HitResult.Distance; // The amount this bullet has penned until now
			
			// Apply damage only on the server, if our client thinks we hit a character request damage
			// We only do client hit requests for characters, other hits follow the server authority
			if (HasAuthority() && (!HitCharacter || GetOwnerCharacter()->IsLocallyControlled()))
			{
				ApplyHitscanDamage(HitResult, TotalDistance, CurrentPenetration, ShotAmmoType);
			}
			else if (!HasAuthority() && IsValid(HitCharacter))
			{
				AReadyOrNotPlayerController* PlayerController = Cast<AReadyOrNotPlayerController>(GetOwningPlayerController());
				if (PlayerController)
				{
					int32 AmmoTypeIndex = AmmunitionTypes.IndexOfByKey(CurrentAmmoTypeRowName);
					
					float SynchronizedTime = PlayerController->GetSynchronizedWorldTime();
					Server_HitscanHit(HitResult, SynchronizedTime, TraceStart, TotalDistance, CurrentPenetration, AmmoTypeIndex);

					const FAmmoTypeData* AmmoTypeData = GetCurrentAmmoType();
					HitCharacter->PredictHitEffects(HitResult, AmmoTypeData ? AmmoTypeData->WoundSize : 35.0f);
				}
			}
		}
		
		// Apply ragdoll forces
		if (bIsEntryHit && HitResult.GetComponent() && HitResult.GetComponent()->Mobility == EComponentMobility::Movable && HitResult.GetComponent()->IsSimulatingPhysics())
		{
			HitResult.GetComponent()->AddImpulseAtLocation(-HitResult.ImpactNormal * RagdollImpulseMultiplier, HitResult.ImpactPoint, HitResult.BoneName);
		}

		// Spalling
		bool bSpalling = false;
		// ##UE5.3UPGRADE##
		// bool bShouldSpall = UKismetMathLibrary::RandomBoolWithWeightFromStream(HitSurfaceSpallingChance, (Seed + 64));
		bool bShouldSpall = false;
		// ##UE5.3UPGRADE##
		
		if (i == LastEntryIndex && !bLastHitPenetrated && ShotAmmoType && bShouldSpall)
		{
			bSpalling = true;

			FVector Location = HitResult.Location;
			FRotator DecalRotation = (-HitResult.ImpactNormal).Rotation() + FRotator(0.0f, 0.0f, FMath::RandRange(0.0f, 360.0f));

			if (HitResult.GetActor() && !HitResult.GetActor()->IsA(AReadyOrNotCharacter::StaticClass()))
				UGameplayStatics::SpawnDecalAttached(SpallingDecal, FVector(5.0f, 6.0f, 6.0f), HitResult.GetComponent(), HitResult.BoneName, Location, DecalRotation, EAttachLocation::KeepWorldPosition);
			
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), SpallingParticleSystem, Location, HitResult.ImpactNormal.Rotation());
			USoundSource* SoundSource = USoundSource::CreateThirdPersonSound(GetWorld(), SpallingEvent, FTransform(Location), {}, EOcclusionType::OT_Angular, EPropagationType::PT_Portal, false);
			if (SoundSource)
				SoundSource->Play();
			
			float Radius = ShotAmmoType->SpallingRadius;

			#if !UE_BUILD_SHIPPING
			if (CVarRonDrawBallisticsDebug.GetValueOnAnyThread() != 0)
				DrawDebugSphere(GetWorld(), HitResult.Location, Radius, 16, FColor::Yellow, false, 5.0f);
			#endif

			// Apply spalling damage only on the server
			if (HasAuthority() && Radius > 0.0f && GetOwnerCharacter())
			{
				TArray<AActor*> OutActors;
				UKismetSystemLibrary::SphereOverlapActors(GetWorld(), HitResult.Location, Radius, {}, AReadyOrNotCharacter::StaticClass(), { HitResult.GetActor() }, OutActors);
				
				for (AActor* Actor : OutActors)
				{
					AReadyOrNotCharacter* Character = Cast<AReadyOrNotCharacter>(Actor);
					if (!Character)
						continue;

					FVector SpallingDirection = (Actor->GetActorLocation() - HitResult.Location).GetSafeNormal();
					float SpallingDot = FVector::DotProduct(HitResult.ImpactNormal, SpallingDirection);

					// Prevent spalling mostly behind impact point
					if (SpallingDot < -0.25f)
						continue;
					
					float SpallingDamageToDeal = ShotAmmoType->SpallingDamage;
					float SpallingDamageTaken = UGameplayStatics::ApplyDamage(Character, SpallingDamageToDeal, GetOwnerCharacter()->GetController(), this, UDamageType::StaticClass());

					// Play bullet hit post process effects emulating a hit from where the spalling originated
					if (APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(Character))
					{
						FHitResult HitInfo;
						HitInfo.ImpactPoint = HitResult.Location;
						
						PlayerCharacter->Client_BulletHit(HitInfo);

						FSuppressionData Data;
						Data.Strength = 0.75f;
						Data.Origin = HitInfo.TraceStart;
						Data.Direction = (HitInfo.TraceEnd - HitInfo.TraceStart).GetSafeNormal();
						Data.Distance = FVector::Distance(HitInfo.TraceStart, PlayerCharacter->GetActorLocation());
						Data.Instigator = GetOwnerCharacter();
						
						PlayerCharacter->Multicast_InflictSuppression(Data, SupressionCameraShake, false);
					}

					#if !UE_BUILD_SHIPPING
					if (CVarRonDrawBallisticsDebug.GetValueOnAnyThread() != 0)
						DrawDebugString(GetWorld(), Character->GetActorLocation() + FVector::UpVector * 75.0f, FString::Printf(TEXT("Spalling (%.2f)"), SpallingDamageTaken), nullptr, FColor::Yellow, 5.0f);
					#endif
				}
			}
		}
		
		if (!HitCharacter && !bSpalling)
		{
			// First exit impact FX only when it isn't also the last exit impact
			bool bWasFirstExitExclusive = HitCount == FirstExitIndex && FirstExitIndex != LastExitIndex;

			// Check if this was the last (non-penetrating) exit
			const float LastShotPenetrationFXThreshold = 0.0f; // i.e. 10.0f for backface "spalling" impacts
			float PenetrationDifference = TotalPenetrationDistance - MaxPenetrationDistance;
			bool bLastExitImpactFX = HitCount == LastExitIndex && PenetrationDifference < LastShotPenetrationFXThreshold;
			
			// only a 25% chance to spawn impacts past the first entry and exit to reduce massive cost of impacts
			bool bWasFirstOrLastHit = HitCount == FirstEntryIndex || HitCount == LastExitIndex;
			bool bRandomImpactFX = !bWasFirstOrLastHit && UKismetMathLibrary::RandomBoolWithWeight(0.25f);
			
			if (HitCount == FirstEntryIndex || bWasFirstExitExclusive || bLastExitImpactFX || bRandomImpactFX)
			{
				PlayHitscanImpactEffects(HitResult, bIsEntryHit);
			}
		}
		
		// if hit is blocked by ballistic shield increment shield damage
		if (ABallisticsShield* Shield = Cast<ABallisticsShield>(HitResult.GetActor()))
		{
			Shield->DamageShieldGlass();
		}
	}
}

void ABaseMagazineWeapon::Server_HitscanHit_Implementation(const FHitResult& HitResult, float Time, FVector TraceBegin, float Distance, float Penetration, int32 AmmoTypeIndex)
{
	if (!ensure(HasAuthority()))
		return;

	// Attempt to resolve an ammo type
	// We don't really care if the client cheats this, it has to be one the server knows this weapon has anyway
	const FAmmoTypeData* AmmoTypeData = nullptr;
	if (AmmunitionTypes.IsValidIndex(AmmoTypeIndex))
	{
		FName AmmoTypeName = AmmunitionTypes[AmmoTypeIndex];
		AmmoTypeData = AmmoDataTable->FindRow<FAmmoTypeData>(AmmoTypeName, "Client Ammo Type Lookup");
	}
	else if (AmmunitionTypes.Num() > 0)
	{
		UE_LOG(LogReadyOrNotHitRegistration, Warning, TEXT("Shot dropped for %s, invalid ammo type"), *GetNameSafe(GetOwner()));
		return;
	}

	// Custom settings can turn off validation altogether
	const UHitRegistrationSettings* HitRegistrationSettings = GetDefault<UHitRegistrationSettings>();
	if (!HitRegistrationSettings->bEnableValidation || CVarRonDisableHitValidation.GetValueOnAnyThread() != 0)
	{
		ApplyHitscanDamage(HitResult, Distance, Penetration, AmmoTypeData);
		return;
	}

	float TraceDistanceForgiveness = FMath::Abs(HitRegistrationSettings->TraceDistanceForgiveness);
	float HitResultDistance = FVector::Distance(TraceBegin, HitResult.ImpactPoint) - TraceDistanceForgiveness;
	if (HitResultDistance > Distance)
	{
		UE_LOG(LogReadyOrNotHitRegistration, Warning, TEXT("Shot dropped for %s, invalid trace distance (%f > %f)"), *GetNameSafe(GetOwner()), HitResultDistance, Distance);
		return;
	}
	
	APlayerCharacter* PlayerCharacter = GetOwnerPlayerCharacter();
	if (!PlayerCharacter)
		return;

	AReadyOrNotCharacter* Character = Cast<AReadyOrNotCharacter>(HitResult.HitObjectHandle.FetchActor());
	if (!Character)
		return;
	
	// Check the player was roughly where they claim they were shooting from
	for (const FCharacterSnapshot& Snapshot : PlayerCharacter->GetSnapshots())
	{
		// TODO(killo): extrapolate up to n number of seconds into the future for the client
		if (Snapshot.Time > Time)
			continue;

		FBox ShooterBox = Snapshot.BoundingBox;
		FBox ExpandedBox = ShooterBox.ExpandBy(HitRegistrationSettings->ShooterForgiveness);

		if (!ExpandedBox.IsInsideOrOn(TraceBegin))
		{
			UE_LOG(LogReadyOrNotHitRegistration, Warning, TEXT("Shot dropped for %s, shooter position mismatch"), *GetNameSafe(GetOwner()));
			return;
		}
	}

	// Reconcile hit character between client and server, with interpolation
	const TArray<FCharacterSnapshot>& HitSnapshots = Character->GetSnapshots();
	for (int32 i = HitSnapshots.Num() - 2; i >= 0; i--)
	{
		const FCharacterSnapshot& CurrentSnapshot = HitSnapshots[i];

		if (CurrentSnapshot.Time > Time)
			continue;

		const FCharacterSnapshot& NextSnapshot = HitSnapshots[i + 1];

		float Interp = (Time - CurrentSnapshot.Time) / (NextSnapshot.Time - CurrentSnapshot.Time);
		if (!ensureMsgf(Interp >= 0.0f && Interp <= 1.0f, TEXT("Interp should never be out of range")))
			Interp = FMath::Clamp(Interp, 0.0f, 1.0f);

		FBox InterpolatedBox;
		InterpolatedBox.Min = FMath::Lerp(CurrentSnapshot.BoundingBox.Min, NextSnapshot.BoundingBox.Min, Interp);
		InterpolatedBox.Max = FMath::Lerp(CurrentSnapshot.BoundingBox.Max, NextSnapshot.BoundingBox.Max, Interp);

		bool bSuccess = InterpolatedBox.IsInsideOrOn(HitResult.ImpactPoint);
		if (bSuccess)
		{
			ApplyHitscanDamage(HitResult, Distance, Penetration, AmmoTypeData);
		}
		else
		{
			FVector ClosestPoint = InterpolatedBox.GetClosestPointTo(HitResult.ImpactPoint);
			float ErrorDistance = FVector::Distance(ClosestPoint, HitResult.ImpactPoint);
			UE_LOG(LogReadyOrNotHitRegistration, Warning, TEXT("Shot dropped for %s, shot was %.2f cm outside permissible area"), *GetNameSafe(GetOwner()), ErrorDistance);
		}

#if !UE_BUILD_SHIPPING
		Client_HitscanDebug(bSuccess, InterpolatedBox.GetCenter(), InterpolatedBox.GetExtent(), HitResult.ImpactPoint);
#endif
		
		return;
	}

	UE_LOG(LogReadyOrNotHitRegistration, Warning, TEXT("Shot dropped for %s, no matching snapshots found"), *GetNameSafe(GetOwner()));
}

void ABaseMagazineWeapon::Client_HitscanDebug_Implementation(bool bSuccess, FVector Center, FVector Extent, FVector ImpactPoint)
{
#if !UE_BUILD_SHIPPING
	if (CVarRonDrawHitRegistration.GetValueOnAnyThread() == 0)
		return;
	
	if (!GetWorld())
		return;
	
	FColor Color = bSuccess ? FColor::Green : FColor::Red;
	DrawDebugBox(GetWorld(), Center, Extent, Color, false, 10.0f, 0, 0.5f);

	DrawDebugSphere(GetWorld(), ImpactPoint, 10.0f, 12, Color, false, 10.0f, 0, 0.5f);
#endif
}

void ABaseMagazineWeapon::ApplyHitscanDamage(const FHitResult& HitResult, float TotalDistance, float Penetration, const FAmmoTypeData* ShotAmmoType)
{
	if (!ensure(HasAuthority()))
		return;
	
	AActor* HitActor = HitResult.GetActor();
	if (!IsValid(HitActor) || !HitActor->CanBeDamaged())
		return;

	float DamageToApply = Damage;
	if (FRichCurve* DamageCurve = DamageOverRange.GetRichCurve())
	{
		if (DamageCurve->GetNumKeys() > 0)
			DamageToApply = DamageCurve->Eval(TotalDistance);
	}
	
	if (ShotAmmoType)
	{
		DamageToApply = ShotAmmoType->Damage;
		
		if (const FRichCurve* DamageCurve = CurrentAmmoType.DamageOverRangeCurve.GetRichCurveConst())
		{
			if (DamageCurve->GetNumKeys() > 0)
				DamageToApply = DamageCurve->Eval(TotalDistance);
		}

		float MaxPenetration = FMath::Max(CurrentAmmoType.PenetrationDistance * 0.1f, 1.0f); // mm to cm
		float Energy = FMath::Clamp(MaxPenetration - Penetration, 0.0f, MaxPenetration) / MaxPenetration;
		
		if (Energy < 0.1f)
			Energy = 0.0f;

		AReadyOrNotCharacter* Character = Cast<AReadyOrNotCharacter>(HitActor);
		if (Character)
		{
			// Cut damage when suspect or owner-less weapon is over-penetrating through walls
			bool bShouldReduceDamage = !GetOwnerCharacter() || GetOwnerCharacter()->IsSuspect();
			if (bShouldReduceDamage && Energy <= 0.9f)
				DamageToApply = FMath::Min(DamageToApply, 5.0f);
		}
		
		DamageToApply *= Energy;
	}

	ABreakableGlass* Glass = Cast<ABreakableGlass>(HitActor);
	if (Glass)
	{
		Glass->ConvertHitAndExecute(HitResult, DamageToApply);
	}

	#if !UE_BUILD_SHIPPING
	if (CHECK_DEBUG_SUBSYSTEM)
	{
        if (DEBUG_SUBSYSTEM->bLogWeaponDamageValuesToConsole)
        {
            ULog::Number(DamageToApply, GetName() + " damage at " + FString::SanitizeFloat(TotalDistance / 100.0f) + "m: ");
        }
    
        DamageToApply *= DEBUG_SUBSYSTEM->bApplyGlobalDamageMultiplier_Weapons ? DEBUG_SUBSYSTEM->GlobalDamageMultiplier_Weapons : 1.0f;
	}
	#endif
	
	TSubclassOf<UDamageType> BulletDamageType = (bArmorPiercing && ArmorPiercingDamageType ? ArmorPiercingDamageType : (TSubclassOf<UDamageType>)UBulletDamageType::StaticClass());
	
	UGameplayStatics::ApplyPointDamage(HitActor, DamageToApply, HitResult.ImpactNormal, HitResult, GetOwnerCharacter()->GetController(), this, BulletDamageType);
}

void ABaseMagazineWeapon::CalculateSuppressionForAI(const FVector& Start, const FVector& End)
{
	#if !UE_BUILD_SHIPPING
	if (CVarRonDrawBoneSuppression.GetValueOnAnyThread() > 0)
	{
		DrawDebugLine(GetWorld(), Start, End, FColor::Green, false, 1.0f, 0, 1.25f);
	}
	#endif

	const FVector HitScanDirection = (End - Start).GetSafeNormal();

	const TArray<ACyberneticCharacter*>& AllAI = GetWorld()->GetGameState<AReadyOrNotGameState>()->AllAICharacters;
	for (ACyberneticCharacter* CyberneticCharacter : AllAI)
	{
		if (!CyberneticCharacter->GetCyberneticsController())
			continue;
		
		if (AReadyOrNotCharacter::IsOnSameTeam(CyberneticCharacter, GetOwnerCharacter()))
			continue;

		const FVector DirectionToHitScanStart = (CyberneticCharacter->GetActorLocation() - Start).GetSafeNormal2D();
		const float DotProduct = FVector::DotProduct(HitScanDirection, DirectionToHitScanStart);
		
		//LOG_NUMBER(DotProduct);
		if (DotProduct < 0.85f)
			continue;

		const FVector P = CyberneticCharacter->GetActorLocation();
		const FVector A = Start;
		const FVector B = End;
		const FVector AB = B - A;
		const FVector AP = P - A;
		
		const float Alpha = FVector::DotProduct(AP, AB)/FVector::DotProduct(AB, AB);
		const FVector ClosestPoint = A + (FMath::Clamp(Alpha, 0.0f, 1.0f) * AB);

		// Ignore if too far away from gun fire, only care if within 2m of us
		if (FVector::Distance(ClosestPoint, CyberneticCharacter->GetActorLocation()) > 100.0f)
			continue;
		
		if (CyberneticCharacter->GetCyberneticsController()->GetTrackedTarget() == GetOwner() ||
			UBpGameplayHelperLib::HasLineOfSight(CyberneticCharacter, GetOwner()))
		{
			constexpr uint8 NumBones = 22;
			TArray<FName, TInlineAllocator<NumBones>> Bones;
			Bones.Add("head");
			Bones.Add("neck_1");
			Bones.Add("spine_3");
			Bones.Add("spine_2");
			Bones.Add("spine_1");
			Bones.Add("pelvis");
			Bones.Add("upperarm_LE");
			Bones.Add("upperarm_RI");
			Bones.Add("lowerarm_LE");
			Bones.Add("lowerarm_RI");
			Bones.Add("hand_LE");
			Bones.Add("hand_RI");
			Bones.Add("thigh_LE");
			Bones.Add("thigh_RI");
			Bones.Add("calf_LE");
			Bones.Add("calf_RI");
			Bones.Add("foot_LE");
			Bones.Add("foot_RI");
			Bones.Add("heel_LE");
			Bones.Add("heel_RI");
			Bones.Add("ball_LE");
			Bones.Add("ball_RI");

			#if !UE_BUILD_SHIPPING
			if (CVarRonDrawBoneSuppression.GetValueOnAnyThread() > 0)
			{
				DrawDebugSphere(GetWorld(), ClosestPoint, 4.0f, 16, FColor::Red, false, 1.0f, 0);
			}
			#endif

			// Find closest suppressed bone
			FName ClosestSuppressedBone = NAME_None;
			float ClosestDistance = FLT_MAX;
			for (uint8 i = 0; i < Bones.Num(); ++i)
			{
				const float Distance = FVector::Distance(CyberneticCharacter->GetMesh()->GetBoneLocation(Bones[i]), ClosestPoint);
				if (Distance < ClosestDistance)
				{
					ClosestSuppressedBone = Bones[i];
					ClosestDistance = Distance;
				}
			}
			
			//const float Suppression = FMath::Clamp((DotProduct - 0.9f) * 10.0f, 0.0f, 1.0f);
			const float Suppression = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, 100.0f), FVector2D(1.0f, 0.01f), FVector::Distance(CyberneticCharacter->GetMesh()->GetBoneLocation(ClosestSuppressedBone), ClosestPoint));
			
			FSuppressionData SuppressionData;
			SuppressionData.Strength = Suppression;
			SuppressionData.Origin = Start;
			SuppressionData.Direction = HitScanDirection;
			SuppressionData.Distance = FVector::Distance(CyberneticCharacter->GetActorLocation(), ClosestPoint);
			SuppressionData.Instigator = GetOwnerCharacter();
			
			if (ClosestSuppressedBone != NAME_None)
			{
				SuppressionData.Distance = FVector::Distance(CyberneticCharacter->GetMesh()->GetBoneLocation(ClosestSuppressedBone), ClosestPoint);
				
				CyberneticCharacter->BoneSuppressionAmount.Add(ClosestSuppressedBone, SuppressionData);
				
				#if !UE_BUILD_SHIPPING
				if (CVarRonDrawBoneSuppression.GetValueOnAnyThread() > 0)
				{
					FString DebugMessage = FString::Printf(TEXT("Suppressed Bone: %s | Dist: %.3f | Strength: %.3f"), *ClosestSuppressedBone.ToString(), SuppressionData.Distance, SuppressionData.Strength);
					DrawDebugString(GetWorld(), ClosestPoint, DebugMessage, nullptr, FColor::White, 1.0f, true);
				}
				#endif
			}
			
			CyberneticCharacter->Multicast_InflictSuppression_Implementation(SuppressionData, nullptr, IsLessLethalWeapon());
		}
		else
		{
			FSuppressionData SuppressionData;
			SuppressionData.Strength = 0.0f;
			SuppressionData.Origin = Start;
			SuppressionData.Direction = HitScanDirection;
			SuppressionData.Distance = FVector::Distance(CyberneticCharacter->GetActorLocation(), ClosestPoint);
			SuppressionData.Instigator = GetOwnerCharacter();
			
			CyberneticCharacter->Multicast_InflictSuppression_NoLineOfSight_Implementation(SuppressionData, nullptr, IsLessLethalWeapon());
		}
	}
}

void ABaseMagazineWeapon::PlayHitscanImpactEffects(const FHitResult& HitResult, bool bIsEntry)
{
	FName ImpactPoolName = NAME_None;
	EImpactEffectType ImpactType = ImpactEffects->GetDefaultObject<AImpactEffect>()->Type;
	switch (ImpactType)
	{
		case EImpactEffectType::Default:		ImpactPoolName = "Rifle Impact Pool"; break;
		case EImpactEffectType::Rifle:			ImpactPoolName = "Rifle Impact Pool"; break;
		case EImpactEffectType::Pistol:			ImpactPoolName = "Pistol Impact Pool"; break;
		case EImpactEffectType::Shotgun:		ImpactPoolName = "Shotgun Impact Pool"; break;
		case EImpactEffectType::Ricochet:		ImpactPoolName = "Ricochet Impact Pool"; break;
		case EImpactEffectType::Beanbag:		ImpactPoolName = "Beanbag Impact Pool"; break;
		case EImpactEffectType::Pepperball:		ImpactPoolName = "Pepperball Impact Pool"; break;
		case EImpactEffectType::Flare:			ImpactPoolName = "Flare Impact Pool"; break;
		default:								ImpactPoolName = "All Impact Pool"; break;
	}
	
	if (UObjectPoolBase* ImpactPool = UObjectPoolFunctionLibrary::GetObjectPool(this, ImpactPoolName))
	{
		if (AImpactEffect* ImpactEffectInst = Cast<AImpactEffect>(ImpactPool->GetActorFromPool()))
		{
			ImpactEffectInst->MaxLifespan = 300.0f;
			ImpactEffectInst->PooledActor_BeginPlay();
			
			ImpactEffectInst->SetActorTransform(FTransform(FRotator::ZeroRotator, HitResult.Location));
			
			FVector Direction = HitResult.TraceEnd - HitResult.TraceStart;
			Direction.Normalize();
			
			FVector ParticleDirection = HitResult.ImpactNormal;
			if (bIsEntry)
			{
				// Particle direction matches the impacted surface normal
				ParticleDirection = HitResult.ImpactNormal;
			}
			else
			{
				// Exit effects should align with the trace direction
				ParticleDirection = -Direction;
			}
			
			ImpactEffectInst->Tags.Add("SpawnedDecal");
			ImpactEffectInst->DecalScale = 2.0f;
			ImpactEffectInst->bSpawnParticle = true;
			ImpactEffectInst->bTraceComplex = false;
			
			ImpactEffectInst->TriggerImpactEffect(HitResult, ParticleDirection);
		}
	}
}

void ABaseMagazineWeapon::PlayHitscanRicochetEffects(const TArray<FRicochet>& Ricochets)
{
	// Spawn effects where ricochets occurred
	for (const FRicochet& Ricochet : Ricochets)
	{
		// todo: object pool
		// Offset the ricochet particle up a few cm so that it doesn't collide on the surface it spawns
		//UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), RicochetParticleSystem, Ricochet.HitResult.Location + Ricochet.HitResult.ImpactNormal * 5.0f, Ricochet.Direction.Rotation());

		RicochetParticleComponent = RicochetParticleComponents[RicochetCompIndex];
		RicochetCompIndex++;
		if (RicochetCompIndex >= 4)
			RicochetCompIndex = 0;

		RicochetParticleComponent->SetTemplate(RicochetParticleSystem);
		RicochetParticleComponent->SetUsingAbsoluteLocation(true);
		RicochetParticleComponent->SetUsingAbsoluteRotation(true);
		RicochetParticleComponent->SetUsingAbsoluteScale(true);

		// Completely set the transform to overwrite any changes made to this instance
		FTransform ParticleTransform = FTransform(
			Ricochet.Direction.Rotation(),
			Ricochet.Location + Ricochet.Normal * 5.0f,
			FVector::OneVector);
		
		RicochetParticleComponent->SetRelativeTransform(ParticleTransform);
		RicochetParticleComponent->Activate(true);

		// Notify the texture streamer so that PSC gets managed as a dynamic component.
		IStreamingManager::Get().NotifyPrimitiveUpdated(RicochetParticleComponent);
		
		USoundSource* SoundSource = USoundSource::CreateThirdPersonSound(GetWorld(), RicochetEvent, FTransform(Ricochet.Location), {}, EOcclusionType::OT_Angular, EPropagationType::PT_Portal, false);
		if(SoundSource)
			SoundSource->Play();
	}	
}

ABulletProjectile* ABaseMagazineWeapon::SpawnProjectile(TSubclassOf<ABulletProjectile> Class, FTransform SpawnTransform, bool bLocalOnly, int32 ProjectileNumber, int32 Seed)
{
	FRandomStream RandomStream = FRandomStream(Seed);
	
	if (bLocalOnly && bNoLocalProjectile)
		return nullptr;

	float PendingSpreadYaw = FMath::Abs(PendingSpread.Yaw);
	float PendingSpreadPitch = FMath::Abs(PendingSpread.Pitch);

	FRotator AdditionalSpread;
	AdditionalSpread.Yaw = RandomStream.FRandRange(-PendingSpreadYaw + -SpreadPattern.Yaw, PendingSpreadYaw + SpreadPattern.Yaw);
	AdditionalSpread.Pitch = RandomStream.FRandRange(-PendingSpreadPitch + -SpreadPattern.Pitch, PendingSpreadPitch + SpreadPattern.Pitch);

	// Ammo type spread contribution
	const FAmmoTypeData* AmmoTypeData = GetCurrentAmmoType();
	if (!bIgnoreAmmoTypeSpread && AmmoTypeData)
	{
		float AmmoSpreadYaw = FMath::Abs(AmmoTypeData->SpreadPattern.X);
		float AmmoSpreadPitch = FMath::Abs(AmmoTypeData->SpreadPattern.Y);

		AdditionalSpread.Yaw += RandomStream.FRandRange(-AmmoSpreadYaw, AmmoSpreadYaw);
		AdditionalSpread.Pitch += RandomStream.FRandRange(-AmmoSpreadPitch, AmmoSpreadPitch);
	}

	TArray<UWeaponAttachment*> WeaponAttachments;
	GetComponents(WeaponAttachments);
	for (int32 i = 0; i < WeaponAttachments.Num(); i++)
	{
		UWeaponAttachment* Attachment = Cast<UWeaponAttachment>(WeaponAttachments[i]);
		if (Attachment)
		{
			AdditionalSpread *= Attachment->SpreadMultiplier;
		}
	}

	if (GetOwner())
	{
		float SpreadVelocity = GetOwner()->GetVelocity().Size2D() * VelocitySpreadMultiplier;
		AdditionalSpread.Yaw += RandomStream.FRandRange(-SpreadVelocity, SpreadVelocity);
		AdditionalSpread.Pitch += RandomStream.FRandRange(-SpreadVelocity, SpreadVelocity);
	}

	if (GetOwnerPlayerCharacter())
	{
		if (GetOwnerPlayerCharacter()->bAiming)
			AdditionalSpread *= ADSSpreadMultiplier;
	}

	// Use AI cached hitscan origin if available
	ACyberneticCharacter* CyberneticCharacter = Cast<ACyberneticCharacter>(GetOwner());
	if (CyberneticCharacter)
	{
		CyberneticCharacter->TimeSinceLastShot = 0.0f;
		if (!bAIFireAtBulletSpawn)
		{
			FHitResult CachedHit = CyberneticCharacter->CachedHitScanResult;
			SpawnTransform.SetLocation(CachedHit.TraceStart);
			SpawnTransform.SetRotation(UKismetMathLibrary::FindLookAtRotation(CachedHit.TraceStart, CachedHit.TraceEnd).Quaternion());
		}
	} 
	
	FRotator SpawnRotator = SpawnTransform.GetRotation().Rotator();
	SpawnRotator += (AdditionalSpread * (IsFirstShot() ? FirstShotSpread : 1.0f));
	SpawnRotator.Roll = 0;
	SpawnRotator.Normalize();

	SpawnTransform.SetRotation(SpawnRotator.Quaternion());

	if (bHitScan && !UReadyOrNotGameInstance::bIsBuildPirated)
	{
		FHitscanShot HitscanShot;
		HitscanShot.Location = SpawnTransform.GetLocation();
		HitscanShot.Direction = SpawnTransform.GetRotation().Vector();
		HitscanShot.Seed = Seed;
		
		if (bLocalOnly)
		{
			Multicast_PerformHitscan_Implementation(HitscanShot, true, Seed);
		}
		else
		{
			Multicast_PerformHitscan(HitscanShot, false, Seed);
		}
	}
	else
	{
		// clang does not accept ternary operations where the type is ambiguous
		TSubclassOf<ABulletProjectile> ProjectileClass;
		if (UReadyOrNotGameInstance::bIsBuildPirated) {
			ProjectileClass = ABulletProjectile::StaticClass();
		}
		else {
			ProjectileClass = Class;
		}
		//const TSubclassOf<ABulletProjectile> ProjectileClass = (UReadyOrNotGameInstance::bIsBuildPirated ? ABulletProjectile::StaticClass() : Class);
		if (ABulletProjectile* Bullet = GetWorld()->SpawnActorDeferred<ABulletProjectile>(ProjectileClass, SpawnTransform, GetOwner(), GetOwnerCharacter()))
		{
			Bullet->PhysicsImpulseMultiplier = Bullet->PhysicsImpulseMultiplier / SpawnProjectileCount;
			Bullet->ProjectileNumber = ProjectileNumber;
			Bullet->SetOwner(GetOwner());
			Bullet->FiredFromWeapon = this;
			Bullet->FiredFromPlayer = GetOwnerPlayerCharacter();
			Bullet->Damage = UReadyOrNotGameInstance::bIsBuildPirated ? 0.0f : Damage;
			Bullet->InitialLocation = GetBulletSpawn()->GetComponentLocation();
			Bullet->ArmorPiercing = UReadyOrNotGameInstance::bIsBuildPirated ? false : bArmorPiercing;
			Bullet->InitialSpeed = UReadyOrNotGameInstance::bIsBuildPirated ? 10000.0f : ProjectileMovementSpeed;
			Bullet->InitialDamageType = DamageType;
			Bullet->ImpactEffectsClass = ImpactEffects;
			Bullet->PenetrationDistance = bArmorPiercing ? PenetrationDistance * 2.0f : PenetrationDistance;
			Bullet->ExitEffects = ExitEffects != nullptr ? ExitEffects : ImpactEffects;
			Bullet->RicochetEffects = RicochetEffects;
			Bullet->Wobble = Wobble;
			Bullet->InitialWobbleDelay = InitialWobbleDelay;
			Bullet->OwningActor = this;
			Bullet->bAttachOnHit = UReadyOrNotGameInstance::bIsBuildPirated ? false : bAttachBulletOnHit;
			Bullet->bDestroyOnHit = UReadyOrNotGameInstance::bIsBuildPirated ? false : bDestroyBulletOnHit;
			//Bullet->GetBulletMeshSkele()->SetSkeletalMesh(BulletProjectileMesh);
			Bullet->GetBulletMesh()->SetStaticMesh(UReadyOrNotGameInstance::bIsBuildPirated ? FakeProjectileMeshStatic : BulletProjectileMeshStatic);
			Bullet->GetCollisionComp()->IgnoreActorWhenMoving(GetOwner(), true);
			Bullet->GetCollisionComp()->IgnoreActorWhenMoving(this, true);
			Bullet->GetBulletMesh()->IgnoreActorWhenMoving(GetOwner(), true);
			Bullet->GetBulletMesh()->IgnoreActorWhenMoving(this, true);			

			if (ProjectileAttachedParticle)
			{
				UGameplayStatics::SpawnEmitterAttached(ProjectileAttachedParticle, Bullet->GetRootComponent(), NAME_None, FVector::ZeroVector, FRotator::ZeroRotator, FVector(1), EAttachLocation::SnapToTarget, true, EPSCPoolMethod::AutoRelease);
			}

			Bullet->BulletProjectileScale = BulletProjectileScale;
			Bullet->PhysicsImpulseMultiplier = RagdollImpulseMultiplier;
			Bullet->DecalScale = ImpactDecalScale;
			Bullet->DamageType = DamageType;
			Bullet->Drag = BulletDrag;

			if (Bullet->GetCollisionComp())
			{
				Bullet->GetCollisionComp()->MoveIgnoreActors.Add(this);
				Bullet->GetCollisionComp()->MoveIgnoreActors.Add(GetOwnerCharacter());
				
				if (GetOwnerCharacter() && GetOwnerCharacter()->GetInventoryComponent())
					Bullet->GetCollisionComp()->MoveIgnoreActors.Append(GetOwnerCharacter()->GetInventoryComponent()->GetInventoryItems());

				if (Cast<ASWATCharacter>(GetOwnerCharacter()))
				{
					if (USWATManager* SwatManager = USWATManager::Get(this))
					{
						Bullet->GetCollisionComp()->MoveIgnoreActors.Append((TArray<AActor*>)SwatManager->SwatAI);
						Bullet->GetCollisionComp()->MoveIgnoreActors.Append((TArray<AActor*>)SwatManager->SwatTrailers);
					}
				}
			}

			Bullet->bDrawBlood = bDrawBlood;
			Bullet->LockIntegrityDamage = FMath::FRandRange(LockIntegrityMinDamage, LockIntegrityMaxDamage);
			Bullet->DamageLossMultiplierPerSurface = bArmorPiercing ? 0.75f : 0.5f;
			Bullet->SpeedLossMultiplierPerSurface = bArmorPiercing ? 0.75f : 0.5f;

			//Bullet->GetBulletMesh()->SetWorldRotation(FRotator(FMath::RandRange(0.0f, 360.0f)));
			//Bullet->GetBulletMesh()->SetSimulatePhysics(true);
			if (Bullet->GetMovementComp())
			{
				Bullet->GetMovementComp()->InitialSpeed = Bullet->InitialSpeed;
				Bullet->GetMovementComp()->MaxSpeed = Bullet->InitialSpeed;
			}
			
			/*ACyberneticCharacter* ai = Cast<ACyberneticCharacter>(GetOwner());
			if (ai)
			{
				SpawnRotator = ai->GetFireAtRotation(GetBulletSpawn()->GetComponentLocation(), GetBulletSpawn()->GetComponentRotation());
				if (SpawnRotator == FRotator::ZeroRotator)
				{
					SpawnRotator = GetBulletSpawn()->GetComponentRotation();
				}
				SpawnRotator.Roll = 0;
				SpawnRotator.Normalize();
				FQuat SpawnQuat = SpawnRotator.Quaternion();
				SpawnQuat.Normalize();
				SpawnTransform.SetRotation(SpawnQuat);
				bullet->FinishSpawning(SpawnTransform);
				LastSpawnedProjectile = bullet;
				return bullet;
			}*/
			
			Bullet->FinishSpawning(SpawnTransform);
			Bullet->SetReplicates(!bLocalOnly || bNoLocalProjectile);
			
			//Bullet->GetBulletMesh()->AddImpulse(GetBulletSpawn()->GetForwardVector() * ProjectileMovementSpeed);

			LastSpawnedProjectile = Bullet;
			return Bullet;
		}
	}

	return nullptr;
}

bool ABaseMagazineWeapon::IsBlockingAnimationPlaying(TArray<EBlockingAnimationExclusion> Exclusions) const
{
	APlayerCharacter* pc = Cast<APlayerCharacter>(GetOwner());
	if (!pc)
		return false;

	if (!AnimationData)
		return false;

	if (pc->Is1PMontagePlaying(AnimationData->Reload_Start.Body_FP) || pc->Is1PMontagePlaying(AnimationData->Reload_Loop.Body_FP) || pc->Is1PMontagePlaying(AnimationData->Reload_End.Body_FP))
	{
		//GEngine->AddOnScreenDebugMessage(9623, 5.0f, FColor::White, "Unable to Play Animation, Reload Animation is Playing on 1P Mesh");
		return true;
	}

	if (pc->Is1PMontagePlaying(AnimationData->Tactical_Reload.Body_FP) || pc->Is1PMontagePlaying(AnimationData->Reload.Body_FP))
	{
		//GEngine->AddOnScreenDebugMessage(9623, 5.0f, FColor::White, "Unable to Play Animation, Reload Animation is Playing on 1P Mesh");
		return true;
	} 

	if (pc->Is1PMontagePlaying(AnimationData->Tactical_ReloadEmpty.Body_FP) || pc->Is1PMontagePlaying(AnimationData->ReloadEmpty.Body_FP))
	{
		//GEngine->AddOnScreenDebugMessage(9623, 5.0f, FColor::White, "Unable to Play Animation, Reload Empty Animation is Playing on 1P Mesh");
		return true;
	}

	if (pc->Is1PMontagePlaying(AnimationData->Holster.Body_FP) || pc->Is1PMontagePlaying(AnimationData->Crouch_Holster.Body_FP))
	{
		if (!Exclusions.Contains(EBlockingAnimationExclusion::BAE_Holster))
		{
			//GEngine->AddOnScreenDebugMessage(9623, 5.0f, FColor::White, "Unable to Play Animation, Holster Animation is Playing on 1P Mesh");
			return true;
		}
	}


	if (pc->Is1PMontagePlaying(AnimationData->Draw.Body_FP) || pc->Is1PMontagePlaying(AnimationData->Crouch_Draw.Body_FP) || pc->Is1PMontagePlaying(AnimationData->DrawFirst.Body_FP) || pc->Is1PMontagePlaying(AnimationData->Crouch_DrawFirst.Body_FP))
	{
		if (!Exclusions.Contains(EBlockingAnimationExclusion::BAE_Draw))
		{
			//GEngine->AddOnScreenDebugMessage(9623, 5.0f, FColor::White, "Unable to Play Animation, Draw Animation is Playing on 1P Mesh");
			return true;
		}
	}

	if (pc->Is1PMontagePlaying(AnimationData->MagazineCheck.Body_FP) || pc->Is1PMontagePlaying(AnimationData->MagazineCheckSights.Body_FP) || pc->Is1PMontagePlaying(AnimationData->Crouch_MagazineCheck.Body_FP) || pc->Is1PMontagePlaying(AnimationData->Crouch_MagazineCheckSights.Body_FP))
	{
		if (!Exclusions.Contains(EBlockingAnimationExclusion::BAE_MagCheck))
		{
			//GEngine->AddOnScreenDebugMessage(9623, 5.0f, FColor::White, "Unable to Play Animation, Magazine Check is Playing on 1P Mesh");
			return true;
		}
	}
	
	if (pc->Is1PMontagePlaying(AnimationData->FireSelect_Semi.Body_FP) || pc->Is1PMontagePlaying(AnimationData->FireSelect_Burst.Body_FP) || pc->Is1PMontagePlaying(AnimationData->FireSelect_Auto.Body_FP))
	{
		if (!Exclusions.Contains(EBlockingAnimationExclusion::BAE_Holster))
		{
			//GEngine->AddOnScreenDebugMessage(9623, 5.0f, FColor::White, "Unable to Play Animation, Fire Select is Playing on 1P Mesh");
			return true;
		}
	}


	// ads variants also have to block!
	if (pc->Is1PMontagePlaying(AnimationData->Reload_FP_Ads) ||
		pc->Is1PMontagePlaying(AnimationData->ReloadEmpty_FP_Ads) ||
		pc->Is1PMontagePlaying(AnimationData->Tactical_Reload_FP_Ads) ||
		pc->Is1PMontagePlaying(AnimationData->Tactical_ReloadEmpty_FP_Ads))
	{
		//GEngine->AddOnScreenDebugMessage(9623, 5.0f, FColor::White, "Unable to Play Animation, Reload Animation is Playing on 1P Mesh");
		return true;
	}

	
	return Super::IsBlockingAnimationPlaying(Exclusions);
}

void ABaseMagazineWeapon::OnRep_AttachmentRep()
{
	MuzzleFlashParticleComponent->AttachToComponent(ItemMesh, FAttachmentTransformRules::KeepRelativeTransform, MuzzleFlashParticleComponent->GetAttachSocketName());
	MuzzleSmokeParticleComponent->AttachToComponent(ItemMesh, FAttachmentTransformRules::KeepRelativeTransform, MuzzleSmokeParticleComponent->GetAttachSocketName());
	if (GetOwnerCharacter())
	{
		DESTROY_COMPONENT(Mag_01_Comp);
		DESTROY_COMPONENT(Mag_01_Comp_TPOnly);
		DESTROY_COMPONENT(Mag_01_Bullets_Comp);
		DESTROY_COMPONENT(Mag_01_Extra_Comp);
		DESTROY_COMPONENT(Mag_ReloadInterpFix_Comp);
		DESTROY_COMPONENT(Mag_02_Comp);
		DESTROY_COMPONENT(Mag_02_Bullets_Comp);
		DESTROY_COMPONENT(Mag_02_Extra_Comp);
		// hack to try fix the desyncing magazines
		if (GetOwnerCharacter()->IsOnSWATTeam() || GetOwnerCharacter()->IsLocalPlayer())
		{
			Mag_01_Comp = NewObject<UStaticMeshComponent>(this, UStaticMeshComponent::StaticClass());
			Mag_01_Comp->RegisterComponent();
			Mag_01_Comp_TPOnly = NewObject<UStaticMeshComponent>(this, UStaticMeshComponent::StaticClass());
			Mag_01_Comp_TPOnly->RegisterComponent();
			Mag_01_Bullets_Comp = NewObject<UStaticMeshComponent>(this, UStaticMeshComponent::StaticClass());
			Mag_01_Bullets_Comp->RegisterComponent();
			Mag_01_Extra_Comp = NewObject<UStaticMeshComponent>(this, UStaticMeshComponent::StaticClass());
			Mag_01_Extra_Comp->RegisterComponent();
			Mag_ReloadInterpFix_Comp = NewObject<UStaticMeshComponent>(this, UStaticMeshComponent::StaticClass());
			Mag_ReloadInterpFix_Comp->SetHiddenInGame(true);
			Mag_ReloadInterpFix_Comp->RegisterComponent();
			Mag_02_Comp = NewObject<UStaticMeshComponent>(this, UStaticMeshComponent::StaticClass());
			Mag_02_Comp->RegisterComponent();
			Mag_02_Bullets_Comp = NewObject<UStaticMeshComponent>(this, UStaticMeshComponent::StaticClass());
			Mag_02_Bullets_Comp->RegisterComponent();
			Mag_02_Extra_Comp = NewObject<UStaticMeshComponent>(this, UStaticMeshComponent::StaticClass());
			Mag_02_Extra_Comp->RegisterComponent();
			bLastAttached = false;
			AttachStatic();

			// didn't seem to apply in begin play so I moved it here...
			TArray<UStaticMeshComponent*> StaticMeshComponents;
			GetComponents(StaticMeshComponents);
			for (UStaticMeshComponent* s : StaticMeshComponents)
			{
				s->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn,  ECR_Ignore);
				s->SetCollisionResponseToChannel(ECollisionChannel::ECC_PhysicsBody, ECR_Ignore);
				s->SetCollisionResponseToChannel(ECollisionChannel::ECC_ITEM, ECR_Ignore);
				s->SetCollisionResponseToChannel(ECC_SOUND, ECR_Ignore);
				s->bNavigationRelevant = false;
				s->SetCanEverAffectNavigation(false);
			}
			
			if (GetOwnerCharacter()->IsLocalPlayer())
			{
				EnableWeaponFovShader();
			}
		}
	}
	Super::OnRep_AttachmentRep();
}

bool ABaseMagazineWeapon::IsPistolWithShield() const
{
	if (!GetOwnerCharacter())
		return false;
	
	if (const ABallisticsShield* bs = GetOwnerCharacter()->GetEquippedItem<ABallisticsShield>())
	{
		return bs->PistolEquippedWithShield == this;
	}

	return false;
}

void ABaseMagazineWeapon::PlaySound(USoundCue* Cue)
{
	USoundData::SpawnSoundAttached(Cue, GetItemMesh(), true);
}

void ABaseMagazineWeapon::Multicast_SpawnParticleEffects_Implementation(bool bSkipAuthority, bool bSkipLocalOwner)
{
	if (GetLocalRole() >= ROLE_Authority && bSkipAuthority)
	{
		return;
	}
	if (IsLocallyControlled() && bSkipLocalOwner)
	{
		return;
	}

	bool bMuzzleFlash = FMath::FRandRange(0.0f, 1.0f) < 0.65f;

	bool bHideMuzzleFlash = false;
	bSupressed = false;
	TArray<UWeaponAttachment*> weaponAttachments;
	GetComponents(weaponAttachments);
	for (int32 i = 0; i < weaponAttachments.Num(); i++)
	{
		UWeaponAttachment* attachment = weaponAttachments[i];
		if (attachment)
		{
			if (attachment->bShouldSupressWeapon) bSupressed = true;
			if (attachment->bHidesMuzzleFlash) bHideMuzzleFlash = true;
			if (attachment->bOverrideMuzzleFlash)
			{
				MuzzleSmokeParticleComponent->SetTemplate(attachment->MuzzleSmokeParticle);
				MuzzleFlashParticleComponent->SetTemplate(attachment->MuzzleFlashParticle);
			}
		}
	}

	if (bMuzzleFlash && !bSupressed && !bHideMuzzleFlash)
	{
		MuzzleFlashParticleComponent->ActivateSystem();
	}

	MuzzleSmokeParticleComponent->ActivateSystem();
	
	if (!SoundData)
	{
		//GEngine->AddOnScreenDebugMessage(-1, 60.0f, FColor::Red, ItemName + " NO SOUND WILL PLAY | NO SOUND DATA!");
		return;
	}

	// Play sound effects
	AReadyOrNotCharacter* ReadyOrNotCharacter = Cast<AReadyOrNotCharacter>(GetOwner());
	APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(GetOwner());

	if (!ReadyOrNotCharacter)
		return;
	
	bool bIsOutside = ReadyOrNotCharacter->IsOutside();
	if (SoundData->bPlayFMODFiringAudio)
	{
		// Third Person
		if(GetWorld()->GetSubsystem<USoundManager>())
		{
			if (!IsLocallyControlled())
			{
				USoundSource* SoundSource = USoundSource::CreateThirdPersonSound(GetWorld(), SoundData->FMODGunShot3P, GetItemMesh()->GetComponentTransform(), {}, EOcclusionType::OT_Angular, EPropagationType::PT_Portal, false);
				SoundSource->Attach(GetItemMesh(), MuzzleFlashParticleSocket);
				SoundSource->SetParameter("FireMode", 0.0f);
				SoundSource->SetParameter("IsSupressed", bSupressed ? 1.0f : 0.0f);
				SoundSource->SetParameter("IsOutside", bIsOutside ? 1.0f : 0.0f);
				SoundSource->Play();
			}
			// First Person
			else
			{
				USoundSource* SoundSource = USoundSource::CreateFirstPersonSound(GetWorld(), SoundData->FMODGunShot1P, GetItemMesh()->GetComponentTransform(), {}, false);
				SoundSource->Attach(GetItemMesh(), MuzzleFlashParticleSocket);
				SoundSource->SetParameter("FireMode", 0.0f);
				SoundSource->SetParameter("IsSupressed", bSupressed ? 1.0f : 0.0f);
				SoundSource->SetParameter("IsOutside", bIsOutside ? 1.0f : 0.0f);
				SoundSource->Play();
			}
		}
		else
		{
			// Get the event description from FMOD so we can query the user properties
			FMOD::Studio::EventDescription* EventDescription = nullptr;
			if (FiringAudioComp)
				FiringAudioComp->StudioInstance->getDescription(&EventDescription);
			
			// Query the user property to see if we should retain the instance for loop firing
			bool bRetainInstance = false;
			if (EventDescription && GetAmmo() > 0.0f)
			{
				FMOD_STUDIO_USER_PROPERTY LoopFireProperty;
				if (EventDescription->getUserProperty("LoopFire", &LoopFireProperty) == FMOD_OK)
				{
					// Loop firing should only apply to firemodes other than single
					bRetainInstance = LoopFireProperty.type == FMOD_STUDIO_USER_PROPERTY_TYPE_FLOAT &&
						LoopFireProperty.floatvalue == 1.0f && CurrentFireMode != EFireMode::FM_Single;
				}
			}
			
			// Spawn the FMOD audio only if needed
			if ((bRetainInstance && !FiringAudioComp) || !bRetainInstance)
			{
				// Note(Ali): Optimization. Reuse audio components
				FiringAudioComp = UFMODWorldSubsystem::Get(this)->PlayAudioAttached(IsLocallyControlled() ? SoundData->FMODGunShot1P : SoundData->FMODGunShot3P, GetItemMesh(), MuzzleFlashParticleSocket, false);
			}
			
			// Set the parameters on this new frame in case they have updated
			if (FiringAudioComp)
			{
				//FiringAudioComp->SetParameter("Aim",  PlayerCharacter && PlayerCharacter->bAiming ? 1.0f : 0.0f);
				FiringAudioComp->SetProperty(EFMODEventProperty::ScheduleLookahead, 3.00f);
				
				// For third person sounds.
				if (!IsLocallyControlled())
				{
					// Get the local player, the listener.
					//APlayerCharacter* LocalPlayer = UBpGameplayHelperLib::GetLocalPlayerCharacter(GetWorld());
					//const APawn* LocalPawn = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld())->GetPawnOrSpectator();
					if (LOCAL_PLAYER)
					{
						AActor* WeaponOwner = GetOwner();
						TArray<AActor*> ActorsToIgnore = {WeaponOwner, PlayerCharacter, LocalPlayer};
						
						// Get the location of the listening pawn.
						FVector ListenLocation;
						if (LocalPlayer)
						{
							ListenLocation = LocalPlayer->GetFirstPersonCameraComponent()->GetComponentLocation();
							ActorsToIgnore.Append(LocalPlayer->GetCollisionIgnoredActors());
						}
			
						// Add shooter to ignored actors.
						if (ReadyOrNotCharacter)
						{
							ActorsToIgnore.Append(ReadyOrNotCharacter->GetCollisionIgnoredActors());
						}
						
						const float OcclusionAmount = ReadyOrNotCharacter->GetFMODPropagationComponent()->GetDepthMaterialOcclusionAmount(GetWorld(), ActorsToIgnore, GetItemMesh()->GetComponentLocation(), ListenLocation, GunshotFullOcclusionDepth, GunshotOcclusionMultiplier);
			
						// Check if nosound tag was triggered.
						if(OcclusionAmount == -1)
						{
							return;
						}
			
						// Set the occlusion parameter
						FiringAudioComp->SetParameter("Occlusion",  OcclusionAmount);
					}
				}
				// For first person sounds
				else
				{
					// Set appropriate parameters
					//FiringAudioComp->SetParameter("AmmoLeft", GetAmmo() / AmmoMax);
					
					//FMODParam OutsideParam;
					//OutsideParam.SetValues("IsOutside",  ReadyOrNotCharacter->IsOutside());
					//TArray<FMODParam> ParamsToSet = { OutsideParam };
					//ReadyOrNotCharacter->GetFMODPropagationComponent()->PlayEvent(SoundData->FMODGunShot3P, this->GetActorLocation(), ParamsToSet);
			
					if(GetAmmo() == 1 && SoundData->FireLast)
					{
						PlayFMODAudio(SoundData->FireLast);
						TArray<FMODParam> LastFireParams = {};
						ReadyOrNotCharacter->GetFMODPropagationComponent()->PlayEvent(SoundData->FireLast, this->GetActorLocation(), LastFireParams);
					}	
				}
				
				FiringAudioComp->SetParameter("FireMode", CurrentFireMode == EFireMode::FM_Single ? 0.0f : 1.0f);
				FiringAudioComp->SetParameter("IsSupressed", bSupressed ? 1.0f : 0.0f);
				FiringAudioComp->SetParameter("IsOutside", bIsOutside ? 1.0f : 0.0f);
			
				// Only play if this is a new instance or if somehow our loop has ended (it shouldn't)
				if (!FiringAudioComp->IsPlaying())
				{
					FiringAudioComp->Play();
				}
			}
		}
	}
	else
	{
		if (bSupressed)
		{
			//USoundData::SpawnSoundAttached(SoundData->GetFiringSound(bIsOutside).Gunshot_Supressed, GetFPItemMesh(), IsLocallyControlled(true) ? false : true, MuzzleFlashParticleSocket);
		}
		else
		{
			//USoundData::SpawnSoundAttached(SoundData->GetFiringSound(bIsOutside).Gunshot, GetFPItemMesh(), IsLocallyControlled(true) ? false : true, MuzzleFlashParticleSocket);

		}
		GetWorld()->GetTimerManager().SetTimer(GunTails_Handle, FTimerDelegate::CreateUObject(this, &ABaseMagazineWeapon::PlaySound, SoundData->GetFiringSound(bIsOutside).GunTail), 0.25f, false);
	}
}

void ABaseMagazineWeapon::Multicast_HandleSupression_Implementation()
{
	AReadyOrNotCharacter* OwnerPlayer = Cast<AReadyOrNotCharacter>(GetOwner());
	if (OwnerPlayer && SoundData)
	{
		if (!bSupressed)
		{
			APlayerCharacter* LocalPlayer =  Cast<APlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)); /*UBpGameplayHelperLib::GetLocalPlayerCharacter(GetWorld());*/
			if (!LocalPlayer)
				return;

			if (LocalPlayer == OwnerPlayer)
				return;

			const FVector BulletFireDirection = GetBulletSpawn()->GetForwardVector();
			const FVector DirectionToLocalPlayer = (LocalPlayer->GetActorLocation() - GetBulletSpawn()->GetComponentLocation()).GetSafeNormal();
			const float DotProduct = FVector::DotProduct(BulletFireDirection, DirectionToLocalPlayer);

			// intentionally only track world static
			FHitResult Hit;
			GetWorld()->LineTraceSingleByObjectType(Hit, LocalPlayer->GetActorLocation(), OwnerPlayer->GetActorLocation(), FCollisionObjectQueryParams(ECC_WorldStatic));
			ACyberneticController* CyberneticController = Cast<ACyberneticController>(OwnerPlayer->GetController());
			if ((CyberneticController && (CyberneticController->GetTrackedTarget() == LocalPlayer || CyberneticController->GetLastTrackedEnemy() == LocalPlayer)) || !Hit.bBlockingHit)
			{
				const float Distance = UKismetMathLibrary::NormalizeToRange(UBpGameplayHelperLib::GetDistanceBetweenActors(OwnerPlayer, LocalPlayer), 0.0f, 15000.0f);
				const float DotProductScaled = FMath::Clamp(Distance, 0.0f, 0.098f);

				//GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, "DotProductScaled: " + FString::SanitizeFloat(DotProductScaled));
				//GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, "DotProduct: " + FString::SanitizeFloat(DotProduct));

				if (DotProduct < 0.90f + DotProductScaled)
					return;

				if (DotProduct > 0.95f && APlayerCharacter::IsOnSameTeam(LocalPlayer, OwnerPlayer) && TeamMateShotHeadDelay <= 0.0f)
				{
					TeamMateShotHeadDelay = 20.0f;
					//LocalPlayer->Server_PlayPVPSpeech("TeammateShotNearHead", LocalPlayer->GetTeam());
				}

				FSuppressionData Data;
				Data.Strength = FMath::Clamp((DotProduct - 0.9f) * 10.0f, 0.0f, 1.0f);
				Data.Origin = GetBulletSpawn()->GetComponentLocation();
				Data.Distance = Distance;
				Data.Direction = DirectionToLocalPlayer;
				Data.Instigator = OwnerPlayer;
				
				LocalPlayer->Multicast_InflictSuppression_Implementation(Data, SupressionCameraShake, IsLessLethalWeapon());

				const FVector BulletRightVector = GetBulletSpawn()->GetRightVector();
				//const FVector LeftRightOffset = (LocalPlayer->GetActorLocation() - GetBulletSpawn()->GetComponentLocation()).GetSafeNormal();
				const float LeftRightDotProduct = FVector::DotProduct(BulletRightVector, DirectionToLocalPlayer);
				//GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, "LeftRightDotProduct: " + FString::SanitizeFloat(LeftRightDotProduct));

				// do sound effects
				if (SoundData->bPlayBulletWhizz)
				{
					PlayBulletWhizz(LeftRightDotProduct < 0.0f ? 0.0f : 1.0f);
				}
				/*
				if (SoundData->bPlayBulletCrack)
				{
					PlayBulletCrack(LeftRightDotProduct < 0.0f ? 0.0f : 1.0f);
				}
				*/
			}
		}
	}
}

void ABaseMagazineWeapon::PlayBulletWhizz(float Pan)
{
	const FFMODEventInstance EventInstance = UFMODBlueprintStatics::PlayEvent2D(GetWorld(), SoundData->BulletWhizzFar, true);
	if (EventInstance.Instance)
		EventInstance.Instance->setParameterByName("pan", Pan);
}

void ABaseMagazineWeapon::AttachStatic()
{
	if (!IsLocallyControlled() && !bHasVisibleMags)
	{
		DetachStatic();
		return;
	}
	
	if (bLastAttached)
		return;
	
	USceneComponent* CurMesh = ItemMesh;
	if (CurMesh == nullptr)
		return;

	// LOGIC FOR MAG ATTACHMENTS
	// TODO: switch to newer attach function

	FName Mag01Socket, Mag02Socket;
	GetMagazineAttachmentSockets(Mag01Socket, Mag02Socket);

	if (!Mag_01_Comp || !Mag_01_Bullets_Comp || !Mag_01_Extra_Comp || !Mag_02_Comp || !Mag_02_Bullets_Comp || !Mag_02_Extra_Comp)
	{
		return;
	}
	
	const bool bCastShadow = !IsLocallyControlled();

	if (Mag_01_Comp)
	{
		if (Mag_01_Comp->GetStaticMesh() != GetAppropriateMagazineMesh())
		{
			Mag_01_Comp->SetStaticMesh(GetAppropriateMagazineMesh());
			if (Mag_ReloadInterpFix_Comp)
			{
				Mag_ReloadInterpFix_Comp->SetStaticMesh(GetAppropriateMagazineMesh());
			}
		}

		Mag_01_Comp->SetVisibility(CurMesh->IsVisible());
		Mag_01_Comp->SetCastShadow(bCastShadow);

		Mag_01_Comp->AttachToComponent(CurMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, Mag01Socket);
	}
	
	if (Mag_01_Bullets_Comp && !Mag_01_Bullets_Comp->GetStaticMesh())
	{
		// ARMOR PIERCING === FMJ? Yes - RYAN
		if (bArmorPiercing)
		{
			Mag_01_Bullets_Comp->SetStaticMesh(Mag_01_FMJ_Bullets_Static);
		}
		else
		{
			Mag_01_Bullets_Comp->SetStaticMesh(Mag_01_HP_Bullets_Static);
		}
		Mag_01_Bullets_Comp->SetVisibility(CurMesh->IsVisible());
		Mag_01_Bullets_Comp->SetCastShadow(bCastShadow);
		Mag_01_Bullets_Comp->AttachToComponent(CurMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, Mag_01_Bullets_Socket);
	}
	if (Mag_01_Extra_Comp && !Mag_01_Extra_Comp->GetStaticMesh())
	{
		Mag_01_Extra_Comp->SetVisibility(CurMesh->IsVisible());
		Mag_01_Extra_Comp->SetCastShadow(bCastShadow);
	}
	
	if (Mag_02_Comp)
	{
		Mag_02_Comp->SetVisibility(CurMesh->IsVisible());
		Mag_02_Comp->SetCastShadow(bCastShadow);
		if (Mag_02_Comp->GetStaticMesh() != GetAppropriateMagazineMesh())
		{
			Mag_02_Comp->SetStaticMesh(GetAppropriateMagazineMesh());
		}
		Mag_02_Comp->AttachToComponent(CurMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, Mag02Socket);
	}
	if (Mag_02_Bullets_Comp && !Mag_02_Bullets_Comp->GetStaticMesh())
	{
		Mag_02_Bullets_Comp->SetVisibility(CurMesh->IsVisible());
		Mag_02_Bullets_Comp->SetCastShadow(bCastShadow);
		if (bArmorPiercing)
		{
			Mag_02_Bullets_Comp->SetStaticMesh(Mag_02_FMJ_Bullets_Static);
		}
		else
		{
			Mag_02_Bullets_Comp->SetStaticMesh(Mag_02_HP_Bullets_Static);
		}
		Mag_02_Bullets_Comp->AttachToComponent(CurMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, Mag_02_Bullets_Socket);
	}
	if (Mag_02_Extra_Comp && !Mag_02_Extra_Comp->GetStaticMesh())
	{
		Mag_02_Extra_Comp->SetVisibility(CurMesh->IsVisible());
		Mag_02_Extra_Comp->SetCastShadow(bCastShadow);
		if (Mag_02_Extra_Comp->GetStaticMesh() == nullptr)
		{
			if (Mag_02_Extra_Static)
				Mag_02_Extra_Comp->SetStaticMesh(Mag_02_Extra_Static);
		}
		Mag_02_Extra_Comp->AttachToComponent(CurMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, Mag_02_Extra_Socket);
	}
	if (Mag_01_Extra_Comp && !Mag_01_Extra_Comp->GetStaticMesh())
	{
		if (Mag_01_Extra_Comp->GetStaticMesh() == nullptr)
		{
			if (Mag_01_Extra_Static)
				Mag_01_Extra_Comp->SetStaticMesh(Mag_01_Extra_Static);
		}
	}
	
	if (Mag_01_Extra_Comp)
		Mag_01_Extra_Comp->AttachToComponent(CurMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, Mag_01_Extra_Socket);

	if (ItemMesh->IsSimulatingPhysics())
	{
		if (Mag_02_Comp) Mag_02_Comp->SetStaticMesh(nullptr);
		if (Mag_02_Extra_Comp) Mag_02_Extra_Comp->SetStaticMesh(nullptr);
		if (Mag_02_Bullets_Comp) Mag_02_Bullets_Comp->SetStaticMesh(nullptr);
		if (Mag_ReloadInterpFix_Comp) Mag_ReloadInterpFix_Comp->SetStaticMesh(nullptr);
	}

	if (!bLastAttached)
	{
		EnableWeaponFovShader();
	}

	bLastAttached = true;
}

UStaticMesh* ABaseMagazineWeapon::GetAppropriateMagazineMesh()
{
	TInlineComponentArray<USkinComponent*> SkinComps;
	GetComponents(SkinComps);
	
	for (USkinComponent* skin : SkinComps)
	{
		if (UStaticMesh** FoundMesh = skin->StaticMeshSkinMap.Find(Mag_01_Static))
		{
			return *FoundMesh;
		}
	}
	
	if (const UMagazineAttachment* MagazineAttachment = Cast<UMagazineAttachment>(AmmunitionAttachment))
	{
		return MagazineAttachment->MagazineStaticMesh;
	}
	
	return Mag_01_Static;
}

bool ABaseMagazineWeapon::GetMagazineAttachmentSockets(FName& OutMag01, FName& OutMag02)
{
	if (UMagazineAttachment* MagazineAttachment = Cast<UMagazineAttachment>(AmmunitionAttachment))
	{
		OutMag01 = MagazineAttachment->Socket_01;
		OutMag02 = MagazineAttachment->Socket_02;
		return true;
	}

	OutMag01 = Mag_01_Socket;
	OutMag02 = Mag_02_Socket;
	return false;
}

void ABaseMagazineWeapon::DetachStatic()
{
	if (!bLastAttached)
		return;
	
	bLastAttached = false;

	if (Mag_01_Comp)
	{
		Mag_01_Comp->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
		Mag_01_Comp->SetStaticMesh(nullptr);
	}

	if (Mag_01_Extra_Comp)
	{
		Mag_01_Extra_Comp->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
		Mag_01_Extra_Comp->SetStaticMesh(nullptr);
	}

	if (Mag_02_Comp)
	{
		Mag_02_Comp->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
		Mag_02_Comp->SetStaticMesh(nullptr);
	}

	if (Mag_02_Comp)
	{
		Mag_02_Extra_Comp->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
		Mag_02_Extra_Comp->SetStaticMesh(nullptr);
	}

	if (Mag_01_Bullets_Comp)
	{
		Mag_01_Bullets_Comp->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
		Mag_01_Bullets_Comp->SetStaticMesh(nullptr);
	}

	if (Mag_02_Bullets_Comp)
	{
		Mag_02_Bullets_Comp->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
		Mag_02_Bullets_Comp->SetStaticMesh(nullptr);
	}
	
	if (Mag_01_Comp_TPOnly)
	{
		Mag_01_Comp_TPOnly->SetStaticMesh(nullptr);
	}
	
	if (Mag_ReloadInterpFix_Comp)
	{
		Mag_ReloadInterpFix_Comp->SetStaticMesh(nullptr);
	}
}

void ABaseMagazineWeapon::OnReloadAnimEvent(EReloadAnimEvent Type)
{
	APlayerCharacter* pc = Cast<APlayerCharacter>(GetOwner());
	if (!pc)
	{
		return;
	}

	switch (Type)
	{
		case EReloadAnimEvent::RAE_MagIn:
			if (SoundData)
			{
				if (SoundData->MagIn)
				{
					PlayFMODAudio(SoundData->MagIn);
				}
				else
				{
					PlayFMODAudio(SoundData->MagInQuick);
				}
			}
			break;
		case EReloadAnimEvent::RAE_MagInQuick:
			if (SoundData)
			{
				if (SoundData->MagInQuick)
				{
					PlayFMODAudio(SoundData->MagInQuick);
				}
				else
				{
					PlayFMODAudio(SoundData->MagIn);
				}
			}
			break;
		case EReloadAnimEvent::RAE_MagOut:
			if (SoundData)
			{
				if (SoundData->MagOut)
				{
					PlayFMODAudio(SoundData->MagOut);
				}
				else
				{
					PlayFMODAudio(SoundData->MagOutQuick);
				}
			}
			break;
		case EReloadAnimEvent::RAE_MagOutQuick:
			if (SoundData)
			{
				if (SoundData->MagOutQuick)
				{
					PlayFMODAudio(SoundData->MagOutQuick);
				}
				else
				{
					PlayFMODAudio(SoundData->MagOut);
				}
			}
			break;
		case EReloadAnimEvent::RAE_BoltClosed:
			if (SoundData)
			{
				if (SoundData->BoltClose)
				{
					PlayFMODAudio(SoundData->BoltClose);
				}
				else
				{
					PlayFMODAudio(SoundData->BoltCloseQuick);
				}
			}
			break;
		case EReloadAnimEvent::RAE_BoltClosedQuick:
			if (SoundData)
			{
				if (SoundData->BoltCloseQuick)
				{
					PlayFMODAudio(SoundData->BoltCloseQuick);
				}
				else
				{
					PlayFMODAudio(SoundData->BoltClose);
				}
			}
			break;
		case EReloadAnimEvent::RAE_BoltOpened:
			if (SoundData)
			{
				if (SoundData->BoltOpen)
				{
					PlayFMODAudio(SoundData->BoltOpen);
				}
				else
				{
					PlayFMODAudio(SoundData->BoltOpenQuick);
				}
			}
			break;
		case EReloadAnimEvent::RAE_BoltOpenedQuick:
			if (SoundData)
			{
				if (SoundData->BoltOpenQuick)
				{
					PlayFMODAudio(SoundData->BoltOpenQuick);
				}
				else
				{
					PlayFMODAudio(SoundData->BoltOpen);
				}
			}
			break;
	}
}

void ABaseMagazineWeapon::OnNewFireModeAnimEvent(const EFireMode NewFireMode)
{
	if (!SoundData)
		return;

	switch (NewFireMode)
	{
		case EFireMode::FM_Single:		PlayFMODAudio(SoundData->SelectSemi); break;
		case EFireMode::FM_Auto:		PlayFMODAudio(SoundData->SelectAuto); break;
		case EFireMode::FM_Burst:		PlayFMODAudio(SoundData->SelectBurst); break;
		case EFireMode::FM_Continuous:	PlayFMODAudio(SoundData->SelectAuto); break;
		case EFireMode::FM_Safe:		PlayFMODAudio(SoundData->SelectSafe); break;
		default: break;
	}
}

float ABaseMagazineWeapon::GetWeight()
{
	float ReturnValue = Super::GetWeight();

	// Iterate over magazines
	for (int32 i = 0; i < Magazines.Num(); i++)
	{
		ReturnValue += (MagazineWeightFull - MagazineWeightEmpty) * (Magazines[i].Ammo / AmmoMax); // add the bullets
	}
	ReturnValue += Magazines.Num() * MagazineWeightEmpty; // add the empty magazines

	// Iterate over attachments
	if (ScopeAttachment && ScopeAttachment->WeaponAttachmentType != EWeaponAttachmentType::Null)
	{
		ReturnValue += ScopeAttachment->AttachmentWeight;
	}

	if (MuzzleAttachment && MuzzleAttachment->WeaponAttachmentType != EWeaponAttachmentType::Null)
	{
		ReturnValue += MuzzleAttachment->AttachmentWeight;
	}
	
	if (UnderbarrelAttachment && UnderbarrelAttachment->WeaponAttachmentType != EWeaponAttachmentType::Null)
	{
		ReturnValue += UnderbarrelAttachment->AttachmentWeight;
	}

	if (OverbarrelAttachment && OverbarrelAttachment->WeaponAttachmentType != EWeaponAttachmentType::Null)
	{
		ReturnValue += OverbarrelAttachment->AttachmentWeight;
	}

	return ReturnValue;
}

bool ABaseMagazineWeapon::InSafeMode() const
{
	//return CurrentFireMode == EFireMode::FM_Safe; // Safe mode deprecated
	return false;
}

bool ABaseMagazineWeapon::InSingleMode() const
{
	return CurrentFireMode == EFireMode::FM_Single;
}

bool ABaseMagazineWeapon::InBurstMode() const
{
	return CurrentFireMode == EFireMode::FM_Burst;
}

bool ABaseMagazineWeapon::InFullAutoMode() const
{
	return CurrentFireMode == EFireMode::FM_Auto;
}

void ABaseMagazineWeapon::OnAIHearingSense_Implementation(ACyberneticController* InSenseController, FAIStimulus Stimulus, AActor*& OutOverrideSensedActor)
{
	if (!GetOwnerCharacter())
		return;

	// Ignore gunshots from self
	if (InSenseController->GetCharacter() == GetOwnerCharacter())
		return;

	// Ignore gunshots from same team
	//if (InSenseController->GetTeam() == GetOwnerCharacter()->GetTeam())
	//	return;
	
	//if (!GetOwnerCharacter()->IsOnSWATTeam())
	//	return;
	
	const float DistanceToWeapon = FVector::Distance(Stimulus.ReceiverLocation, Stimulus.StimulusLocation);

	const float GunShotHearingRange = FMath::Clamp(AI_CONFIG_GET_FLOAT("GunShotHearingRange", 2000.0f), 100.0f, 10000.0f);
	const float GunShotReactionTime = FMath::Clamp(AI_CONFIG_GET_FLOAT("GunShotReactionTime", 0.25f), 0.0f, 60.0f);
	const float GunshotForgetTime = FMath::Clamp(AI_CONFIG_GET_FLOAT("GunShotForgetTime", 0.5f), 0.0f, 9999.0f);

	if (DistanceToWeapon <= GunShotHearingRange)
	{
		if (!InSenseController->IsSoundReactingToActor(this))
		{
			FActorSense WeaponSoundSense;
			WeaponSoundSense.Actor = this;
			WeaponSoundSense.Tag = Stimulus.Tag;
			WeaponSoundSense.Stimulus = Stimulus;
			WeaponSoundSense.SenseReactionTime = InSenseController->GetReactionTime(EActorSenseType::Sound) + GunShotReactionTime;
			WeaponSoundSense.SenseForgetTime = GunshotForgetTime;
			
			InSenseController->AddActorSoundSense(WeaponSoundSense);
		}
	}
}

float ABaseMagazineWeapon::GetAmmoWeight(int32 Count)
{
	return Count * MagazineWeightFull;
}

void ABaseMagazineWeapon::OnThrownFromInventory(AReadyOrNotCharacter* Thrower, bool bMarkAsEvidence)
{
	if (Mag_02_Comp)
	{
		Mag_02_Comp->SetHiddenInGame(true);
	}
	if (Mag_02_Extra_Comp)
	{
		Mag_02_Extra_Comp->SetHiddenInGame(true);
	}
	if (Mag_02_Bullets_Comp)
	{
		Mag_02_Bullets_Comp->SetHiddenInGame(true);
	}

	Super::OnThrownFromInventory(Thrower, bMarkAsEvidence);
}
