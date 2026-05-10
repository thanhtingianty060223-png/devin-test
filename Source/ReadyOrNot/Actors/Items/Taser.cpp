// Copyright Void Interactive, 2023

#include "Taser.h"

#include "Characters/PlayerCharacter.h"
#include "Characters/CyberneticController.h"

#include "Actors/Environment/TaserReactionVolume.h"
#include "Actors/Projectiles/DamageProjectiles/BulletProjectile.h"

#include "Components/SpotLightComponent.h"
#include "CableComponent.h"
#include "Actors/Attachments/LaserAttachment.h"

#include "Audio/RoNSoundData.h"

ATaser::ATaser()
{
	TopCable = CreateDefaultSubobject<UCableComponent>(TEXT("LeftCable"));
	TopCable->SetupAttachment(GetItemMesh(), "TopCable");
	TopCable->SetVisibility(false);
	TopCable->bOwnerNoSee = false;
	TopCable->bOnlyOwnerSee = true;

	BottomCable = CreateDefaultSubobject<UCableComponent>(TEXT("RightCable"));
	BottomCable->SetupAttachment(GetItemMesh(), "BottomCable");
	BottomCable->SetVisibility(false);
	BottomCable->bOwnerNoSee = false;
	BottomCable->bOnlyOwnerSee = true;

	LeftDoor = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("LeftDoor"));
	LeftDoor->SetupAttachment(GetItemMesh(), "doorLeft");
	LeftDoor->SetOnlyOwnerSee(false);

	RightDoor = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("RightDoor"));
	RightDoor->SetupAttachment(GetItemMesh(), "doorRight");
	RightDoor->SetOnlyOwnerSee(false);

	CrackleSoundGenerator = CreateDefaultSubobject<UAudioComponent>(TEXT("CrackleSoundGenerator"));
	CrackleSoundGenerator->SetupAttachment(RootComponent);

	CrackleSoundGeneratorFMOD = CreateDefaultSubobject<UFMODAudioComponent>(TEXT("CrackleSoundGeneratorFMOD"));
	CrackleSoundGeneratorFMOD->SetupAttachment(RootComponent);

	LeftProjectile = nullptr;
	RightProjectile = nullptr;

	bCanReloadSameMagazine = false;
	
	bLoseMagOnReload = false;
	bClientPredictReload = false; // Otherwise replication never updates magazines array on reload -killo

	bHasVisibleMags = false;
}

void ATaser::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATaser, LeftCableAttachActor);
	DOREPLIFETIME(ATaser, RightCableAttachActor);
	DOREPLIFETIME(ATaser, LeftProjectile);
	DOREPLIFETIME(ATaser, RightProjectile);
	DOREPLIFETIME(ATaser, ProjectileHitResult);
	DOREPLIFETIME(ATaser, bFiredCartridge);
	DOREPLIFETIME(ATaser, bDetachedProbes);
	DOREPLIFETIME(ATaser, bStartedStun);
	DOREPLIFETIME(ATaser, StunDurationRemaining);
}

void ATaser::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority() && LaserAttachmentClass)
	{
		UWeaponAttachment* NewAttachment = NewObject<UWeaponAttachment>(this, LaserAttachmentClass);

		if (NewAttachment)
		{
			NewAttachment->RegisterComponent();
			NewAttachment->SetIsReplicated(true);
			NewAttachment->AttachToComponent(GetItemMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, NewAttachment->SocketAttachment);
			UnderbarrelAttachment = NewAttachment;
		}
	}

	LeftDoor->AttachToComponent(GetItemMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, "doorLeft");
	RightDoor->AttachToComponent(GetItemMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, "doorRight");
	TopCable->SetAttachEndTo(this, FName("ItemMesh"));
	BottomCable->SetAttachEndTo(this, FName("ItemMesh"));
	
	TopCable->SetCanEverAffectNavigation(false);
	BottomCable->SetCanEverAffectNavigation(false);
	LeftDoor->SetCanEverAffectNavigation(false);
	RightDoor->SetCanEverAffectNavigation(false);

	Multicast_ResetDoors_Implementation();

	int32 DefaultMagazineCount = StartingCartridges / CartridgesPerSlot;
	SetMagazineCount(DefaultMagazineCount, TArray<FName>());
	ReplenishAmmo();
}

void ATaser::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (PreviousParticleHitResult.ImpactPoint != ProjectileHitResult.ImpactPoint)
	{
		StartTaserParticleEffects();
	}

	if (StunDurationRemaining <= 0.0f)
	{
		DestroyTaserParticleEffects();
	}
	else
	{
		StunDurationRemaining -= DeltaSeconds;
		if (CrackleSoundGeneratorFMOD)
		{
			CrackleSoundGeneratorFMOD->SetParameter("StunDurationRemaining", StunDurationRemaining);
		}
	}

	LeftDoor->SetHiddenInGame(!GetItemMesh()->IsVisible() || !GetItemMesh()->SkeletalMesh);
	RightDoor->SetHiddenInGame(!GetItemMesh()->IsVisible() || !GetItemMesh()->SkeletalMesh);
	if (!LeftDoor->IsSimulatingPhysics() && !RightDoor->IsSimulatingPhysics())
	{
		LeftDoor->AttachToComponent(GetItemMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, "doorLeft");
		RightDoor->AttachToComponent(GetItemMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, "doorRight");
		LeftDoor->SetRelativeScale3D(FVector::OneVector);
		RightDoor->SetRelativeScale3D(FVector::OneVector);
		LeftDoor->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator.Quaternion());
		RightDoor->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator.Quaternion());
	}

	if (!GetOwner())
		return;

	if (LeftProjectile)
	{
		const float Distance = FVector::Dist(LeftProjectile->GetActorLocation(), GetItemMesh()->GetComponentLocation());
		if (Distance > ProbeMaxDistance)
		{
			V_LOGM(LogReadyOrNot, "Detaching probs due to %.2f exceeed %.2f", Distance * 0.001f, ProbeMaxDistance);
			DetachProbes();
			LeftProjectile = nullptr;
			RightProjectile = nullptr;
		}

		if (LeftProjectile && RightProjectile)
		{
			if (LeftProjectile->GetAttachParentActor() || RightProjectile->GetAttachParentActor())
			{
				OnRep_ProjectileReplicated();
				AReadyOrNotCharacter* pc = Cast<AReadyOrNotCharacter>(LeftProjectile->GetAttachParentActor());
				AReadyOrNotCharacter* pc2 = Cast<AReadyOrNotCharacter>(RightProjectile->GetAttachParentActor());
				if (pc == pc2 && pc)
				{
					if (pc->GetMesh()->IsSimulatingPhysics())
					{
						LeftProjectile->GetCollisionComp()->SetCollisionResponseToChannel(ECC_PhysicsBody, ECollisionResponse::ECR_Ignore);
						LeftProjectile->GetCollisionComp()->SetCollisionResponseToChannel(ECC_Pawn, ECollisionResponse::ECR_Ignore);
						LeftProjectile->GetBulletMesh()->SetCollisionResponseToChannel(ECC_PhysicsBody, ECollisionResponse::ECR_Ignore);
						LeftProjectile->GetBulletMesh()->SetCollisionResponseToChannel(ECC_Pawn, ECollisionResponse::ECR_Ignore);
						//LeftProjectile->GetBulletMeshSkele()->SetCollisionResponseToChannel(ECC_PhysicsBody, ECollisionResponse::ECR_Ignore);
						//LeftProjectile->GetBulletMeshSkele()->SetCollisionResponseToChannel(ECC_Pawn, ECollisionResponse::ECR_Ignore);

						RightProjectile->GetCollisionComp()->SetCollisionResponseToChannel(ECC_PhysicsBody, ECollisionResponse::ECR_Ignore);
						RightProjectile->GetCollisionComp()->SetCollisionResponseToChannel(ECC_Pawn, ECollisionResponse::ECR_Ignore);
						RightProjectile->GetBulletMesh()->SetCollisionResponseToChannel(ECC_PhysicsBody, ECollisionResponse::ECR_Ignore);
						RightProjectile->GetBulletMesh()->SetCollisionResponseToChannel(ECC_Pawn, ECollisionResponse::ECR_Ignore);
						//RightProjectile->GetBulletMeshSkele()->SetCollisionResponseToChannel(ECC_PhysicsBody, ECollisionResponse::ECR_Ignore);
						//RightProjectile->GetBulletMeshSkele()->SetCollisionResponseToChannel(ECC_Pawn, ECollisionResponse::ECR_Ignore);
					}
				}
			}
		}
	}
	else
	{
		TopCable->bEnableCollision = false;
		BottomCable->bEnableCollision = false;
	}

	BlinkTime -= DeltaSeconds;
	if (BlinkTime <= 0.0f)
	{
		bBlinkState = !bBlinkState;
		BlinkTime = 1.0f;
	}

	if (GetLocalRole() >= ROLE_Authority)
	{
		if (bFiredCartridge && !bDetachedProbes)
		{
			if (bHoldingTaser)
				TimeHoldingTaser += DeltaSeconds;
			
			if (!bStartedStun &&
				LeftProjectile && LeftProjectile->GetAttachParentActor() &&
				RightProjectile && RightProjectile->GetAttachParentActor())
			{
				// Sweep for taser reaction volumes
				TArray<FHitResult> TaserReactionHitResults;
				const FVector Start = LeftProjectile->GetAttachParentActor()->GetActorLocation();
				FVector End = Start;
				End.Z += 10.0f;

				FCollisionObjectQueryParams ObjectParams;
				const FCollisionShape Shape = FCollisionShape::MakeSphere(SweepForReactionVolumeSize);
				const FCollisionQueryParams CollisionParams;

				ObjectParams.AddObjectTypesToQuery(ECC_GameTraceChannel6);

				GetWorld()->SweepMultiByObjectType(TaserReactionHitResults, Start, End, FQuat(), ObjectParams, Shape, CollisionParams);
				for (int32 i = 0; i < TaserReactionHitResults.Num(); i++)
				{
					ATaserReactionVolume* Volume = Cast<ATaserReactionVolume>(TaserReactionHitResults[i].GetActor());
					if (!Volume)
					{
						continue;
					}

					Volume->OnTaserStunDelivered(Cast<AReadyOrNotCharacter>(LeftProjectile->GetAttachParentActor()), this);
				}

				Server_DeliverStunToAttachedTarget_Implementation();
			}
			else if(bCrackling && StunDurationRemaining <= 0.0f)
			{
				Multicast_StopCrackleSoundEffect();
			}
		}
		else
		{
			TimeHoldingTaser = false;
			bHoldingTaser = false;
		}
	}

	if (TaserLightDynamicMaterial && TaserLightDynamicMaterial->IsValidLowLevel())
	{
		if (!bFiredCartridge)
		{	// We haven't fired the cartridge = blink green
			if (bBlinkState)
			{
				TaserLightDynamicMaterial->SetScalarParameterValue("Emissive Power", 100.0f);
			}
			else
			{
				TaserLightDynamicMaterial->SetScalarParameterValue("Emissive Power", 0.0f);
			}

			TaserLightDynamicMaterial->SetVectorParameterValue("Taser Light Colour", FLinearColor::Green);
		}
		else if(!bDetachedProbes && StunDurationRemaining > 0.0f)
		{	// We have fired the cartridge and the probes aren't detached and we have stun duration remaining
			// = don't blink, lerp from green to red
			TaserLightDynamicMaterial->SetScalarParameterValue("Emissive Power", 100.0f);
			TaserLightDynamicMaterial->SetVectorParameterValue("Taser Light Colour",
				FLinearColor::LerpUsingHSV(FLinearColor::Red, FLinearColor::Green,
					UKismetMathLibrary::NormalizeToRange(StunDurationRemaining, 0.0f, PingStunDuration)));
		}
		else
		{	// We have fired the cartridge and the probes are detached OR we are out of stun = blink red
			if (bBlinkState)
			{
				TaserLightDynamicMaterial->SetScalarParameterValue("Emissive Power", 100.0f);
			}
			else
			{
				TaserLightDynamicMaterial->SetScalarParameterValue("Emissive Power", 0.0f);
			}

			TaserLightDynamicMaterial->SetVectorParameterValue("Taser Light Colour", FLinearColor::Red);
		}
	}
	else
	{
		if (FP_SkinMaterials.IsValidIndex(0))
		{
			TaserLightDynamicMaterial = FP_SkinMaterials[0];
		}
	}
}

void ATaser::Server_DeliverStunToAttachedTarget_Implementation()
{
	// Don't stun unless BOTH probes hit the SAME target.
	if (LeftProjectile && RightProjectile && (LeftProjectile->GetAttachParentActor() || RightProjectile->GetAttachParentActor()))
	{
		AReadyOrNotCharacter* pc = Cast<AReadyOrNotCharacter>(LeftProjectile->GetAttachParentActor());
		AReadyOrNotCharacter* pc2 = Cast<AReadyOrNotCharacter>(RightProjectile->GetAttachParentActor());
		if (pc == pc2 && pc)
		{
			StunDurationRemaining = PingStunDuration;
			bStartedStun = true;
			Multicast_StartCrackleSoundEffect();
		}
	}
}

void ATaser::TaserStunFailed()
{
	StunDurationRemaining = 0.0f;
	DestroyTaserParticleEffects();
}

void ATaser::Server_OnFire_Implementation(FRotator Direction, FVector SpawnLoc, int32 Seed)
{
	if (GetAmmo() <= 0.0f)
		return;
	
	bStartedStun = false;
	if (bRefireDelayTriggered || IsBlockingAnimationPlaying())
		return;

	AReadyOrNotCharacter* Character = Cast<AReadyOrNotCharacter>(GetOwner());
	if (!Character)
		return;

	bFiredCartridge = true;

	// Fire the Cartridge..
	if (GetAmmo() > 0.0f)
	{
		if (RefireDelay > 0.0f)
		{
			bRefireDelayTriggered = true;
			GetWorld()->GetTimerManager().SetTimer(RefireDelay_Handle, this, &ABaseWeapon::ResetRefireDelay, RefireDelay, false);
		}

		bool bTempFirstShot = IsFirstShot();

		// Apply recoil to consecutive shots
		TriggerFirstShot();

		if (GetLocalRole() >= ROLE_Authority)
			RemoveAmmo(1.0f);

		FHitResult Hit;

		// Use AI cached hitscan origin if available
		bool bAlreadyTraced = false;
		ACyberneticCharacter* CyberneticCharacter = Cast<ACyberneticCharacter>(GetOwner());
		if (CyberneticCharacter)
		{
			CyberneticCharacter->TimeSinceLastShot = 0.0f;
			if (!bAIFireAtBulletSpawn)
			{
				Hit = CyberneticCharacter->CachedHitScanResult;
				bAlreadyTraced = true;
			}
		} 

		if (!bAlreadyTraced)
		{
			FVector StartTrace = GetBulletSpawn()->GetComponentLocation();
			FVector EndTrace = StartTrace + GetBulletSpawn()->GetForwardVector() * ProbeMaxDistance;
			
			FCollisionQueryParams CollisionParams;
			CollisionParams.bTraceComplex = true;
			CollisionParams.AddIgnoredActor(GetOwner());
			CollisionParams.AddIgnoredActor(this);
			GetWorld()->LineTraceSingleByChannel(Hit, StartTrace, EndTrace, ECC_PROJECTILE, CollisionParams);
		}

		AReadyOrNotCharacter* HitCharacter = Cast<AReadyOrNotCharacter>(Hit.GetActor());
		if (HitCharacter)
		{
			if (Hit.GetComponent() == HitCharacter->GetMesh() || Hit.GetComponent() == HitCharacter->GetFaceMesh())
			{
				FTransform spawnTransform;
				spawnTransform.SetRotation(Direction.Quaternion());
				spawnTransform.SetLocation(Hit.ImpactPoint);

				LeftProjectile = SpawnProjectile(BulletProjectile, spawnTransform);
				LeftProjectile->GetMovementComp()->SetActive(false, true);
				LeftProjectile->GetCollisionComp()->SetCollisionResponseToChannel(ECC_PhysicsBody, ECollisionResponse::ECR_Ignore);
				LeftProjectile->GetCollisionComp()->SetCollisionResponseToChannel(ECC_Pawn, ECollisionResponse::ECR_Ignore);
				LeftProjectile->GetBulletMesh()->SetCollisionResponseToChannel(ECC_PhysicsBody, ECollisionResponse::ECR_Ignore);
				LeftProjectile->GetBulletMesh()->SetCollisionResponseToChannel(ECC_Pawn, ECollisionResponse::ECR_Ignore);
				//LeftProjectile->GetBulletMeshSkele()->SetCollisionResponseToChannel(ECC_PhysicsBody, ECollisionResponse::ECR_Ignore);
				//LeftProjectile->GetBulletMeshSkele()->SetCollisionResponseToChannel(ECC_Pawn, ECollisionResponse::ECR_Ignore);
				LeftProjectile->SetActorEnableCollision(false);

				LeftProjectile->SetActorLocation(Hit.ImpactPoint);
				LeftProjectile->AttachToActor(Hit.GetActor(), FAttachmentTransformRules::SnapToTargetIncludingScale);
				LeftProjectile->AttachToComponent(Hit.GetComponent(), FAttachmentTransformRules::KeepWorldTransform, Hit.BoneName);
				ProjectileHitResult = Hit;


				RightProjectile = SpawnProjectile(BulletProjectile, spawnTransform);
				RightProjectile->GetMovementComp()->SetActive(false, true);
				RightProjectile->GetCollisionComp()->SetCollisionResponseToChannel(ECC_PhysicsBody, ECollisionResponse::ECR_Ignore);
				RightProjectile->GetCollisionComp()->SetCollisionResponseToChannel(ECC_Pawn, ECollisionResponse::ECR_Ignore);
				RightProjectile->GetBulletMesh()->SetCollisionResponseToChannel(ECC_PhysicsBody, ECollisionResponse::ECR_Ignore);
				RightProjectile->GetBulletMesh()->SetCollisionResponseToChannel(ECC_Pawn, ECollisionResponse::ECR_Ignore);
				//RightProjectile->GetBulletMeshSkele()->SetCollisionResponseToChannel(ECC_PhysicsBody, ECollisionResponse::ECR_Ignore);
				//RightProjectile->GetBulletMeshSkele()->SetCollisionResponseToChannel(ECC_Pawn, ECollisionResponse::ECR_Ignore);
				RightProjectile->SetActorEnableCollision(false);
				RightProjectile->SetActorLocation(Hit.ImpactPoint);
				RightProjectile->AttachToActor(Hit.GetActor(), FAttachmentTransformRules::SnapToTargetIncludingScale);
				RightProjectile->AttachToComponent(Hit.GetComponent(), FAttachmentTransformRules::KeepWorldTransform, Hit.BoneName);
				TopCable->CableLength = MinCableLength;
				BottomCable->CableLength = MinCableLength;

				if (HasAuthority())
					UGameplayStatics::ApplyDamage(HitCharacter, 1.0f, GetOwningPlayerController(), this, DefaultDamageType);
			}
		}


		SimulateDoors();

		Multicast_PlayFireEffects(false);
		Multicast_PlayFireEffects_Implementation(false);

		OnWeaponFire.Broadcast(this, true);
		APlayerCharacter* pc = Cast<APlayerCharacter>(GetOwner()); // todo: decouple from APlayerCharacter
		if (pc)
		{
			/*APlayerController* controller = Cast<APlayerController>(pc->GetController());
			if (controller)
			{
				if (IsLocallyControlled())
				{
					controller->ClientStopCameraShake(FireCameraShake);
					controller->ClientStartCameraShake(FireCameraShake, 1.0f);
				}
			}*/
			float recoilVelocity = GetOwner()->GetVelocity().Size2D() * VelocityRecoilMultiplier;
			if (pc->bAiming)
			{
				PendingSpread += GetSpread() * ADSSpreadMultiplier  * (bTempFirstShot ? FirstShotSpread : 1.0f);
				pc->PendingRecoil += GetRecoil() * ADSRecoilMultiplier  * (bTempFirstShot ? FirstShotRecoil : 1.0f);
				pc->PendingRecoil.Pitch += FMath::RandRange(-recoilVelocity, recoilVelocity) * (bTempFirstShot ? FirstShotRecoil : 1.0f);
				pc->PendingRecoil.Yaw += FMath::RandRange(-recoilVelocity, recoilVelocity) * (bTempFirstShot ? FirstShotRecoil : 1.0f);
			}
			else
			{
				PendingSpread += GetSpread() * (bTempFirstShot ? FirstShotSpread : 1.0f);
				pc->PendingRecoil += GetRecoil() * (bTempFirstShot ? FirstShotRecoil : 1.0f);
				pc->PendingRecoil.Pitch += FMath::RandRange(-recoilVelocity, recoilVelocity) * (bTempFirstShot ? FirstShotRecoil : 1.0f);
				pc->PendingRecoil.Yaw += FMath::RandRange(-recoilVelocity, recoilVelocity) * (bTempFirstShot ? FirstShotRecoil : 1.0f);
			}
			pc->RecoilSpeed = RecoilInterpSpeed;
		}
	}
	else
	{
		Multicast_PlayFireEffects(true);
		Multicast_PlayFireEffects_Implementation(true);
		OnWeaponDryFire.Broadcast(this, true);
	}
}

void ATaser::OnFire(FRotator Direction, FVector SpawnLoc)
{
	if (GetAmmo() <= 0.0f)
		return;
	
	Server_SetHoldingTaser(true);

	if (GetLocalRole() < ROLE_Authority)
	{
		Server_OnFire_Implementation(Direction, SpawnLoc, FMath::Rand());
		Server_OnFire(Direction, SpawnLoc, FMath::Rand());
	}
	else
	{
		Server_OnFire_Implementation(Direction, SpawnLoc, FMath::Rand());
	}

	if (SoundData)
	{
		// Spawn the FMOD Audio as it doesn't exist yet
		FiringAudioComp = UFMODBlueprintStatics::PlayEventAttached(IsLocallyControlled() ? SoundData->FMODGunShot1P : SoundData->FMODGunShot3P, GetItemMesh(), MuzzleFlashParticleSocket, FVector::ZeroVector, EAttachLocation::SnapToTarget, true, true, true);
		// this is the first shot (or is going to be a single shot)
		if (FiringAudioComp)
		{
			FiringAudioComp->SetParameter("FireMode", 0.0f);
		}
	}
}

void ATaser::Server_SetHoldingTaser_Implementation(bool bNewHold)
{
	bHoldingTaser = bNewHold;
}

bool ATaser::Server_SetHoldingTaser_Validate(bool bNewHold)
{
	return true;
}

void ATaser::OnItemPrimaryUseEnd()
{
	Super::OnItemPrimaryUseEnd();
	Server_SetHoldingTaser(false);
}

void ATaser::OnWeaponTacticalReload()
{
	if (!bFiredCartridge)
	{
		return;
	}

	if (CanReload())
	{
		DetachProbes();
	}

	Super::OnWeaponTacticalReload();
}

void ATaser::OnWeaponReload(bool bForce)
{
	OnWeaponTacticalReload();
}

void ATaser::Client_OnItemPickedUp_Implementation(AActor* NewOwner, bool bEquipped)
{
	Super::Client_OnItemPickedUp_Implementation(NewOwner, bEquipped);
	
	Multicast_ResetDoors_Implementation();
}

void ATaser::Multicast_PlayFireEffects_Implementation(bool bDryFire)
{
	AReadyOrNotCharacter* Character = Cast<AReadyOrNotCharacter>(GetOwner());
	if (!Character || !AnimationData)
	{
		return;
	}

	if (bDryFire)
	{
		// play the firing animation
		if (Character->bIsCrouched)
		{
			Character->Play1PMontage(AnimationData->Crouch_Dryfire.Body_FP);
			Character->Play3PMontage(AnimationData->Crouch_Dryfire.Body_TP);
			PlayFPMontage(AnimationData->Crouch_Dryfire.Gun_FP);
			PlayTPMontage(AnimationData->Crouch_Dryfire.Gun_TP);
		}
		else
		{
			Character->Play1PMontage(AnimationData->DryFire.Body_FP);
			Character->Play3PMontage(AnimationData->DryFire.Body_TP);
			PlayFPMontage(AnimationData->DryFire.Gun_FP);
			PlayTPMontage(AnimationData->DryFire.Gun_TP);
		}
		
		if (SoundData)
		{
			PlayFMODAudio(SoundData->DryFire);
		}
	}
	else
	{
		if (Character->bIsCrouched)
		{
			Character->Play1PMontage(AnimationData->Crouch_FireSingle[0].Body_FP);
			Character->Play3PMontage(AnimationData->Crouch_FireSingle[0].Body_TP);
			PlayFPMontage(AnimationData->Crouch_FireSingle[0].Gun_FP);
			PlayTPMontage(AnimationData->Crouch_FireSingle[0].Gun_TP);
		}
		else
		{
			Character->Play1PMontage(AnimationData->FireSingle[0].Body_FP);
			Character->Play3PMontage(AnimationData->FireSingle[0].Body_TP);
			PlayFPMontage(AnimationData->FireSingle[0].Gun_FP);
			PlayTPMontage(AnimationData->FireSingle[0].Gun_TP);
		}

		GetMuzzleFlashParticleComp()->Activate(true);
		GetMuzzleSmokeParticleComp()->Activate(true);

		// Temp disable until taser fmod event's distance is reduced significantly
		/*if (SoundData)
		{
			// Spawn the FMOD Audio as it doesn't exist yet
			FiringAudioComp = UFMODBlueprintStatics::PlayEventAttached(IsLocallyControlled(true) ? SoundData->FMODGunShot1P : SoundData->FMODGunShot3P, GetItemMesh(), MuzzleFlashParticleSocket, FVector::ZeroVector, EAttachLocation::SnapToTarget, true, true, true);
			// this is the first shot (or is going to be a single shot)
			if (FiringAudioComp)
			{
				FiringAudioComp->SetParameter("FireMode", 0.0f);
			}
		}*/
	}
}

void ATaser::Server_NextMagazine_Implementation()
{
	// note(killo): see bLoseMagOnReloadComment in constructor
	bOverrideLoseMagOnReload = false;//true;

	Super::Server_NextMagazine_Implementation();
	
	bFiredCartridge = false;
	bDetachedProbes = false;

	Multicast_ResetDoors();
	Multicast_ResetDoors_Implementation();
}

void ATaser::Multicast_PlayTaserHitEffect_Implementation(FHitResult Hit)
{
	UFMODBlueprintStatics::PlayEventAttached(TaserHitEffectFMOD, Hit.GetComponent(), "spine_3", Hit.ImpactPoint, EAttachLocation::KeepWorldPosition, true, true, true);
}

void ATaser::SetItemVisibility(bool bNewVisibility)
{
	if (bNewVisibility != ItemMesh->GetVisibleFlag())
	{
		TArray<USceneComponent*> SceneComps;
		GetComponents(SceneComps);
		for (USceneComponent* Scene : SceneComps)
		{
			if (bNewVisibility && (Scene == TopCable || Scene == BottomCable))
			{
				continue;
			}
			Scene->SetVisibility(bNewVisibility);
		}
	}
}

void ATaser::StartTaserParticleEffects()
{
	DestroyTaserParticleEffects();
	if (LeftProjectile && RightProjectile)
	{
		if (ProjectileHitResult.GetActor() == LeftProjectile->GetAttachParentActor() && StunDurationRemaining > 0.0f)
		{
			ShowCables();
			PreviousParticleHitResult = ProjectileHitResult;
			bStartedParticleEffects = true;
			TaserImpactParticleComp_Start = UGameplayStatics::SpawnEmitterAttached(TaserImpactParticle_Start, ProjectileHitResult.GetComponent(), ProjectileHitResult.BoneName, ProjectileHitResult.ImpactPoint, ProjectileHitResult.ImpactNormal.Rotation(), EAttachLocation::KeepWorldPosition, false);
			//UGameplayStatics::SpawnEmitterAttached(TaserImpactParticle_Start, RightProjectile->GetRootComponent(), NAME_None, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget, false);
			TaserImpactParticleComp_LoopLeft = UGameplayStatics::SpawnEmitterAttached(TaserImpactParticle_Loop, ProjectileHitResult.GetComponent(), ProjectileHitResult.BoneName, ProjectileHitResult.ImpactPoint, ProjectileHitResult.ImpactNormal.Rotation(), EAttachLocation::KeepWorldPosition, false);
		
			//TaserImpactParticleComp_LoopLeft->AttachToComponent(ProjectileHitResult.GetComponent(), FAttachmentTransformRules::KeepWorldTransform, ProjectileHitResult.BoneName);
			TaserImpactParticleComp_LoopRight = UGameplayStatics::SpawnEmitterAttached(TaserImpactParticle_Loop, ProjectileHitResult.GetComponent(), ProjectileHitResult.BoneName, ProjectileHitResult.ImpactPoint, ProjectileHitResult.ImpactNormal.Rotation(), EAttachLocation::KeepWorldPosition, false);
			//APawn* pawn = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld())->GetPawn();
			if (TaserImpactParticleComp_LoopLeft)
			{
				TaserImpactParticleComp_LoopLeft->SetWorldScale3D(FVector(0.5f, 0.5f, 0.5f));
			}
			if (TaserImpactParticleComp_LoopRight)
			{
				TaserImpactParticleComp_LoopRight->SetWorldScale3D(FVector(0.5f, 0.5f, 0.5f));
			}
			//TaserImpactParticleComp_LoopRight->AttachToComponent(ProjectileHitResult.GetComponent(), FAttachmentTransformRules::KeepWorldTransform, ProjectileHitResult.BoneName);
		}
	}
}

void ATaser::DestroyTaserParticleEffects()
{
	if (TaserImpactParticleComp_LoopLeft)
	{
		TaserImpactParticleComp_LoopLeft->DestroyComponent();
		TaserImpactParticleComp_LoopLeft = nullptr;
	}

	if (TaserImpactParticleComp_LoopRight)
	{
		TaserImpactParticleComp_LoopRight->DestroyComponent();
		TaserImpactParticleComp_LoopRight = nullptr;
	}

	if (TaserImpactParticleComp_Start)
	{
		TaserImpactParticleComp_Start->DestroyComponent();
		TaserImpactParticleComp_Start = nullptr;
	}

	Multicast_HideCables_Implementation();
}

void ATaser::OnRep_ProjectileReplicated()
{
	TopCable->bEnableCollision = true;
	BottomCable->bEnableCollision = true;

	TopCable->SetAttachEndTo(LeftProjectile, FName());
	BottomCable->SetAttachEndTo(RightProjectile, FName());
}

void ATaser::ShowCables()
{
	TopCable->SetVisibility(true);
	BottomCable->SetVisibility(true);
}

void ATaser::Multicast_HideCables_Implementation()
{
	TopCable->SetAttachEndTo(nullptr, FName());
	BottomCable->SetAttachEndTo(nullptr, FName());
	TopCable->SetVisibility(false);
	BottomCable->SetVisibility(false);
}

void ATaser::Multicast_DestroyProjectiles_Implementation()
{
	if (LeftProjectile && GetLocalRole() >= ROLE_Authority)
		LeftProjectile->Destroy();
	if (RightProjectile && GetLocalRole() >= ROLE_Authority)
		RightProjectile->Destroy();

	LeftProjectile = nullptr;
	RightProjectile = nullptr;
}

void ATaser::Multicast_ResetDoors_Implementation()
{
	LeftDoor->SetSimulatePhysics(false);
	RightDoor->SetSimulatePhysics(false);

	LeftDoor->SetCollisionProfileName("NoCollision");
	RightDoor->SetCollisionProfileName("NoCollision");

	LeftDoor->AttachToComponent(GetItemMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, "doorLeft");
	RightDoor->AttachToComponent(GetItemMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, "doorRight");

	LeftDoor->SetRelativeScale3D(FVector::OneVector);
	RightDoor->SetRelativeScale3D(FVector::OneVector);

	LeftDoor->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator.Quaternion());
	RightDoor->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator.Quaternion());
}

void ATaser::SimulateDoors()
{
	//FTransform spawnTransform = BulletSpawn->GetComponentTransform();

	LeftDoor->SetSimulatePhysics(true);
	RightDoor->SetSimulatePhysics(true);

	LeftDoor->SetCollisionProfileName("PhysicsItem");
	RightDoor->SetCollisionProfileName("PhysicsItem");

	LeftDoor->SetWorldScale3D(FVector::OneVector);
	RightDoor->SetWorldScale3D(FVector::OneVector);

	FVector LeftDoorImpulse = BulletSpawn->GetForwardVector() * DoorBlowOutForce;
	FVector RightDoorImpulse = BulletSpawn->GetForwardVector() * DoorBlowOutForce;
	
	LeftDoorImpulse += (BulletSpawn->GetUpVector() * -DoorBlowOutForce / FMath::FRandRange(3.0, 5.0));
	RightDoorImpulse += (BulletSpawn->GetUpVector() * DoorBlowOutForce / FMath::FRandRange(3.0, 5.0));

	LeftDoor->AddImpulse(LeftDoorImpulse, NAME_None, true);
	RightDoor->AddImpulse(RightDoorImpulse, NAME_None, true);

	LeftDoorImpulse = (BulletSpawn->GetRightVector() * DoorBlowOutForce / 2);
	RightDoorImpulse = (BulletSpawn->GetRightVector() * DoorBlowOutForce / 2);
	//LeftDoorImpulse = (BulletSpawn->GetUpVector() * DoorBlowOutForce / FMath::FRandRange(10.0f, 100.0f));
	//RightDoorImpulse = (BulletSpawn->GetUpVector() * DoorBlowOutForce / FMath::FRandRange(10.0f, 100.0f));
	LeftDoor->AddAngularImpulseInRadians(LeftDoorImpulse, NAME_None, true);
	RightDoor->AddAngularImpulseInRadians(RightDoorImpulse, NAME_None, true);
}

void ATaser::SetMagazineCount(int32 Count, TArray<FName> AmmoTypes)
{
	Count *= CartridgesPerSlot;

	/* Tasers only have one ammo type but we do this just in case someone
	 * wants to mod the taser ammo types to shoot 120mm APFSDS rounds or something
	 */
	TArray<FName> FullAmmoTypes;
	for (const FName& AmmoTypeRowName : AmmoTypes)
	{
		for (int32 i = 0; i < CartridgesPerSlot; i++)
		{
			FullAmmoTypes.Add(AmmoTypeRowName);
		}
	}

	Super::SetMagazineCount(Count, FullAmmoTypes);
}

void ATaser::DetachProbes_Implementation()
{
 	Multicast_PlayDetachEffect();
	Multicast_HideCables();
	Multicast_HideCables_Implementation();

	if (LeftProjectile && RightProjectile)
	{
		Multicast_DestroyProjectiles();
		Multicast_DestroyProjectiles_Implementation();
	}
	
	bDetachedProbes = true;

	Multicast_ResetCableAttachments();
	Multicast_ResetCableAttachments_Implementation();
}

void ATaser::Multicast_ResetCableAttachments_Implementation()
{
	TopCable->SetAttachEndTo(this, FName("ItemMesh"));
	BottomCable->SetAttachEndTo(this, FName("ItemMesh"));
	TopCable->SetVisibility(false);
	BottomCable->SetVisibility(false);
}

void ATaser::Multicast_PlayDetachEffect_Implementation()
{
	CrackleSoundGenerator->Stop();
	CrackleSoundGenerator->SetSound(DetachSoundEffect);
	CrackleSoundGenerator->Play();
	CrackleSoundGeneratorFMOD->Stop();
	CrackleSoundGeneratorFMOD->SetEvent(DetachSoundEffectFMOD);
	CrackleSoundGeneratorFMOD->Play();
	GetMuzzleFlashParticleComp()->Activate();
}

void ATaser::Multicast_StartCrackleSoundEffect_Implementation()
{
	bCrackling = true;
	CrackleSoundGenerator->SetSound(CrackleSoundEffect);
	CrackleSoundGenerator->Play();
	CrackleSoundGeneratorFMOD->SetEvent(CrackleSoundEffectFMOD);
	CrackleSoundGeneratorFMOD->Play();
}

void ATaser::Multicast_StopCrackleSoundEffect_Implementation()
{
	bCrackling = false;
	CrackleSoundGenerator->Stop();
	CrackleSoundGeneratorFMOD->Stop();
}

float ATaser::GetWeight()
{
	return Super::GetWeight() + (GetMagazineCount() * CartridgeWeight);
}

float ATaser::GetAmmoWeight(int32 Count)
{
	return Count * CartridgeWeight;
}

bool ATaser::PlayDraw(bool bDrawFirst)
{
	return Super::PlayDraw(bDrawFirst);
}

bool ATaser::PlayHolster()
{
    
    if (bFiredCartridge && !bDetachedProbes)
    {
    	DetachProbes();
    }

	return Super::PlayHolster();
}

bool ATaser::HandleMelee(FHitResult Hit)
{
	if (!Hit.GetActor())
	{
		FVector StartTrace = GetBulletSpawn()->GetComponentLocation();
		FVector EndTrace = StartTrace + GetBulletSpawn()->GetForwardVector() * ProbeMaxDistance;
		FCollisionQueryParams CollisionParams;
		CollisionParams.AddIgnoredActor(GetOwner());
		CollisionParams.AddIgnoredActor(this);
		GetWorld()->LineTraceSingleByChannel(Hit, StartTrace, EndTrace, ECC_GameTraceChannel1, CollisionParams);
	}
	GetWorld()->GetTimerManager().SetTimer(HandleMeleeDefferred_Handle, FTimerDelegate::CreateUObject(this, &ATaser::HandleMeleeDeffered, Hit), 0.3f, false);
	return true;
}

void ATaser::HandleMeleeDeffered(FHitResult Hit)
{
	AReadyOrNotCharacter* Victim = Cast<AReadyOrNotCharacter>(Hit.GetActor());
	if (Victim)
	{
		Hit.Location = Victim->GetMesh() ? Victim->GetMesh()->GetSocketLocation("spine_3") : Victim->GetActorLocation();
		Hit.ImpactPoint = Hit.Location;
		Hit.BoneName = "spine_3";
		Hit.Component = Victim->GetMesh();
		
		DestroyTaserParticleEffects();

		bStartedParticleEffects = true;
		TaserImpactParticleComp_Start = UGameplayStatics::SpawnEmitterAttached(TaserImpactParticle_Start, Hit.GetComponent(), "spine_3", Hit.ImpactPoint, Hit.ImpactNormal.Rotation(), EAttachLocation::KeepWorldPosition, false);
		//UGameplayStatics::SpawnEmitterAttached(TaserImpactParticle_Start, RightProjectile->GetRootComponent(), NAME_None, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget, false);
		TaserImpactParticleComp_LoopLeft = UGameplayStatics::SpawnEmitterAttached(TaserImpactParticle_Loop, Hit.GetComponent(), "spine_3", Hit.ImpactPoint, Hit.ImpactNormal.Rotation(), EAttachLocation::KeepWorldPosition, false);

		//TaserImpactParticleComp_LoopLeft->AttachToComponent(ProjectileHitResult.GetComponent(), FAttachmentTransformRules::KeepWorldTransform, HitHitResult.BoneName);
		TaserImpactParticleComp_LoopRight = UGameplayStatics::SpawnEmitterAttached(TaserImpactParticle_Loop, Hit.GetComponent(), "spine_3", Hit.ImpactPoint, Hit.ImpactNormal.Rotation(), EAttachLocation::KeepWorldPosition, false);

		if (TaserImpactParticleComp_LoopLeft)
		{
			TaserImpactParticleComp_LoopLeft->SetWorldScale3D(FVector(0.5f, 0.5f, 0.5f));
		}
		
		if (TaserImpactParticleComp_LoopRight)
		{
			TaserImpactParticleComp_LoopRight->SetWorldScale3D(FVector(0.5f, 0.5f, 0.5f));
		}

		PreviousParticleHitResult = Hit;
		ProjectileHitResult = Hit;
		StunDurationRemaining = PingStunDuration;
		
		Multicast_PlayTaserHitEffect(Hit);
		Victim->StartBeingTasered(PingStunDuration,this);
	}
}
