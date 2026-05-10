// Copyright Void Interactive, 2023

#include "LaserAttachment.h"

#include "Actors/Gameplay/LensFlare.h"
#include "Actors/Items/BallisticsShield.h"

#include "Components/InventoryComponent.h"

ULaserAttachment::ULaserAttachment()
{
	SetIsReplicatedByDefault(true);
}

void ULaserAttachment::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ULaserAttachment, bRepOn);
}

void ULaserAttachment::OnRep_On()
{
	ToggleLaser(bRepOn);
}

void ULaserAttachment::BeginPlay()
{
	Super::BeginPlay();
	
}

void ULaserAttachment::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	float BeamDistance = 0.0f;
	FVector BeamEndHitLocation = FVector::ZeroVector;
	FVector BeamEndHitNormal = FVector::ZeroVector;
	
	if (IsLaserOn())
	{
		// calculate beam distance
		{
			if (const ABaseWeapon* Weapon = GetWeapon())
			{
				FVector StartTrace = GetComponentLocation() + Weapon->GetBulletSpawn()->GetForwardVector() * -100.0f;
				FVector EndTrace = StartTrace + Weapon->GetBulletSpawn()->GetForwardVector() * 200000.0f;

				FHitResult HitResult;
				
				FCollisionQueryParams CollisionParams;
				CollisionParams.bTraceComplex = true;
				CollisionParams.AddIgnoredComponent(this);
				CollisionParams.AddIgnoredActor(Weapon);
				CollisionParams.AddIgnoredActor(Weapon->GetOwner());
				
				if (AReadyOrNotCharacter* Character = Weapon->GetOwnerCharacter())
				{
					CollisionParams.AddIgnoredActors(Character->GetCollisionIgnoredActors());
				}
				
				GetWorld()->LineTraceSingleByChannel(HitResult, StartTrace, EndTrace, ECC_PROJECTILE, CollisionParams);
				
				BeamEndHitNormal = HitResult.ImpactNormal;
				if (BeamEndHitNormal == FVector::ZeroVector)
				{
					BeamEndHitNormal = EndTrace;
				}
				
				BeamEndHitLocation = HitResult.ImpactPoint;
				if (BeamEndHitLocation == FVector::ZeroVector)
				{
					BeamEndHitLocation = EndTrace;
				}
				
				BeamDistance = HitResult.Distance == 0.0f ? 20000.0f : HitResult.Distance;
			}
		}
	}

	if (const ABaseWeapon* Weapon = GetWeapon())
	{
		const AReadyOrNotCharacter* OwnerChar = Weapon->GetOwnerCharacter();
		
		if (!OwnerChar || (OwnerChar && (OwnerChar->IsDeadOrUnconscious() || OwnerChar->IsIncapacitated())))
		{
			DetachLaser();
			return;
		}
	}
	else
	{
		DetachLaser();
	}

	if (LaserParticleComponent && LaserBeamEndComponent)
	{
		LaserParticleComponent->SetFloatParameter("Distance", BeamDistance);
		
		if (bRequireNVG)
		{
			if (APlayerController* pc = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld()))
			{
				if (APlayerCharacter* viewTarget = Cast<APlayerCharacter>(pc->GetViewTarget()))
				{
					// only visible to people with NVGS on...
					LaserParticleComponent->SetHiddenInGame(!viewTarget->bNVGOn);
					LaserBeamEndComponent->SetHiddenInGame(!viewTarget->bNVGOn);
					LaserParticleComponent->MarkRenderStateDirty();
					LaserBeamEndComponent->MarkRenderStateDirty();
				}
			}
		}
	}

	if (LaserParticleComponent && GetWeapon())
	{
		FRotator laserRotation = UKismetMathLibrary::FindLookAtRotation(GetComponentLocation() + GetWeapon()->GetBulletSpawn()->GetForwardVector() * -100.0f, BeamEndHitLocation);
		LaserParticleComponent->SetWorldRotation(laserRotation);
		if (SpawnedLensFlare)
		{ 
			SpawnedLensFlare->SetActorRotation(laserRotation);
		}
	}
	
	if (LaserBeamEndComponent)
	{
		LaserBeamEndComponent->SetWorldLocationAndRotation(BeamEndHitLocation, BeamEndHitNormal.Rotation());
	}
}

void ULaserAttachment::DestroyComponent(const bool bPromoteChildren)
{
	Super::DestroyComponent(bPromoteChildren);

	DetachLaser();

	DESTROY_COMPONENT(LaserParticleComponent);
	DESTROY_COMPONENT(LaserBeamEndComponent);
	
	if (SpawnedLensFlare)
	{
		SpawnedLensFlare->Destroy();
		SpawnedLensFlare = nullptr;
	}
}

void ULaserAttachment::ToggleLaser(bool bOn)
{
	if (bOn != bRepOn) // new value
	{
		bRepOn = bOn;
		
		if (bOn)
		{
			AttachLaser();
		}
		else
		{
			DetachLaser();
		}

		PlayToggleSound();
	}
}

bool ULaserAttachment::IsLaserOn() const
{
	return LaserParticleComponent ? LaserParticleComponent->IsVisible() : false;
}

void ULaserAttachment::AttachLaser()
{
	ABaseWeapon* Weapon = GetWeapon();
	if (!Weapon)
		return;

	if (Weapon->GetOwnerCharacter() && Weapon->GetOwnerCharacter()->GetEquippedItem() != Weapon)
	{
		bool bEquippedWithShield = false;
		if (const ABallisticsShield* bShield = Cast<ABallisticsShield>(Weapon->GetOwnerCharacter()->GetEquippedItem()))
		{
			bEquippedWithShield = bShield->PistolEquippedWithShield == Weapon;
		}
		
		if (!bEquippedWithShield)
		{
			DetachLaser();
			return;
		}
	}

	// initalize, if not already
	{
		if (!SpawnedLensFlare && LensFlareClass)
		{
			SpawnedLensFlare = GetWorld()->SpawnActor<ALensFlare>(LensFlareClass);
		}
		
		if (!LaserParticleComponent)
		{
			LaserParticleComponent = NewObject<UParticleSystemComponent>(Weapon, UParticleSystemComponent::StaticClass());
			LaserParticleComponent->RegisterComponent();
			LaserParticleComponent->SetTemplate(LaserParticle);
			LaserParticleComponent->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetIncludingScale, "laser");
			LaserParticleComponent->SetVisibility(false);
			LaserParticleComponent->SetHiddenInGame(true);
			LaserParticleComponent->SetActive(false);
			LaserParticleComponent->SetAutoActivate(false);
			
			for (int32 i = 0; i < LaserParticleComponent->GetNumMaterials(); i++)
			{
				SkinMaterials.Add(LaserParticleComponent->CreateAndSetMaterialInstanceDynamic(i));
			}
		}

		if (!LaserBeamEndComponent)
		{
			LaserBeamEndComponent = NewObject<UParticleSystemComponent>(Weapon, UParticleSystemComponent::StaticClass());
			LaserBeamEndComponent->RegisterComponent();
			LaserBeamEndComponent->SetTemplate(LaserBeamEnd);
			LaserBeamEndComponent->SetVisibility(false);
			LaserBeamEndComponent->SetHiddenInGame(true);
			LaserBeamEndComponent->SetActive(false);
			LaserBeamEndComponent->SetAutoActivate(false);
		}
		
		if (SpawnedLensFlare)
		{
			SpawnedLensFlare->AttachToComponent(LaserParticleComponent, FAttachmentTransformRules::SnapToTargetIncludingScale, NAME_None);
			SpawnedLensFlare->SetOwningCharacter(Weapon->GetOwnerCharacter());
			SpawnedLensFlare->SetActorHiddenInGame(true);
		}
	}

	if (LaserParticleComponent)
	{
		LaserParticleComponent->SetActive(true, true);
		LaserParticleComponent->SetHiddenInGame(false);
		LaserParticleComponent->SetVisibility(true);
	}

	if (LaserBeamEndComponent)
	{
		LaserBeamEndComponent->SetActive(true, true);
		LaserBeamEndComponent->SetHiddenInGame(false);
		LaserBeamEndComponent->SetVisibility(true);
	}
	
	if (SpawnedLensFlare)
		SpawnedLensFlare->SetActorHiddenInGame(false);
}

void ULaserAttachment::DetachLaser()
{
	if (LaserParticleComponent)
	{
		LaserParticleComponent->SetActive(false);
		LaserParticleComponent->SetHiddenInGame(true);
		LaserParticleComponent->SetVisibility(false);
	}

	if (LaserBeamEndComponent)
	{
		LaserBeamEndComponent->SetActive(false);
		LaserBeamEndComponent->SetHiddenInGame(true);
		LaserBeamEndComponent->SetVisibility(false);
	}

	if (SpawnedLensFlare)
	{
		SpawnedLensFlare->SetActorHiddenInGame(true);
	}
}
