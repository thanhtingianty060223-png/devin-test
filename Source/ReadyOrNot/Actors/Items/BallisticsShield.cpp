// Copyright Void Interactive, 2017

#include "BallisticsShield.h"

#include "Characters/PlayerCharacter.h"

#include "Info/ReadyOrNotSignificanceManager.h"

ABallisticsShield::ABallisticsShield()
{
	ItemMesh->OnComponentHit.AddDynamic(this, &ABallisticsShield::OnTPShieldHit);
}

bool ABallisticsShield::IsBlockingAnimationPlaying(TArray<EBlockingAnimationExclusion> Exclusions) const
{
	APlayerCharacter* pc = Cast<APlayerCharacter>(GetOwner());
	if (!pc)
	{
		return Super::IsBlockingAnimationPlaying(Exclusions);
	}

	if (PistolEquippedWithShield && AnimationData)
	{
		if (pc->Is1PMontagePlaying(AnimationData->Reload.Body_FP) || pc->Is1PMontagePlaying(AnimationData->Crouch_Reload.Body_FP) ||
			pc->Is1PMontagePlaying(AnimationData->ReloadEmpty.Body_FP) || pc->Is1PMontagePlaying(AnimationData->Crouch_ReloadEmpty.Body_FP) ||
			pc->Is3PMontagePlaying(AnimationData->Reload.Body_TP) || pc->Is3PMontagePlaying(AnimationData->Crouch_Reload.Body_TP) ||
			pc->Is3PMontagePlaying(AnimationData->ReloadEmpty.Body_TP) || pc->Is3PMontagePlaying(AnimationData->Crouch_ReloadEmpty.Body_TP))
		{
			return true;
		}
	}

	return Super::IsBlockingAnimationPlaying(Exclusions);
}

void ABallisticsShield::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABallisticsShield, PistolEquippedWithShield);
	DOREPLIFETIME(ABallisticsShield, bLowered);
	DOREPLIFETIME(ABallisticsShield, GlassPhaseParam);
}

void ABallisticsShield::BeginPlay()
{
	Super::BeginPlay();

	GlassPhaseParam = 0.0f;
	ItemMesh->SetScalarParameterValueOnMaterials("Phase", 0.0f);
}

void ABallisticsShield::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (RefireDelay > 0.0f)
	{
		RefireDelay -= DeltaSeconds;
	}

	GetItemMesh()->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECollisionResponse::ECR_Block);

	if (bTryingToAim)
	{
		OnItemSecondaryUsed();
	}
	if (bTryingToStopAiming)
	{
		OnItemEndSecondaryUse();
	}

	ItemMesh->SetScalarParameterValueOnMaterials("Phase", GlassPhaseParam);

	SetActorEnableCollision(true);
	GetItemMesh()->SetCollisionResponseToChannel(ECC_PROJECTILE, ECR_Block);

	// tick the weapon as well, very important...
	if (PistolEquippedWithShield)
	{
		PistolEquippedWithShield->SetItemVisibility(true);
		PistolEquippedWithShield->Tick(DeltaSeconds);	
		if (GetOwnerPlayerCharacter() && GetOwnerPlayerCharacter()->IsLocalPlayer())
		{
			PistolEquippedWithShield->GetItemMesh()->SetSkeletalMesh(PistolEquippedWithShield->GetAppropriateSkeletalMesh());
			if (PistolEquippedWithShield->ShouldEnableWeaponFovShader())
			{
			 	PistolEquippedWithShield->EnableWeaponFovShader();
			 }
			 PistolEquippedWithShield->TickWeaponFovShader(DeltaSeconds);
			PistolEquippedWithShield->AttachToComponent(GetOwnerPlayerCharacter()->GetMesh1P(), FAttachmentTransformRules::SnapToTargetIncludingScale, PistolEquippedWithShield->HandsSocket);
		} else if (GetOwnerCharacter())
		{
			PistolEquippedWithShield->AttachToComponent(GetOwnerCharacter()->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, PistolEquippedWithShield->HandsSocket);
		}
	}
}

void ABallisticsShield::SetPistol(ABaseItem* newPistol)
{
	bLowered = true;
	if (newPistol && newPistol->ItemCategories.Contains(EItemCategory::IC_UseableWithShield) && Cast<ABaseMagazineWeapon>(newPistol))
	{

		PistolEquippedWithShield = Cast<ABaseMagazineWeapon>(newPistol);
		UReadyOrNotSignificanceManager::UnregisterActorWithSignificanceManager(PistolEquippedWithShield);
		if (PistolEquippedWithShield && PistolEquippedWithShield->ShieldLoweredAnimationData)
		{
			DefaultAnimationData = PistolEquippedWithShield->ShieldLoweredAnimationData;
			AnimationData = DefaultAnimationData;

			SetOwner(PistolEquippedWithShield->GetOwner());

			AReadyOrNotCharacter* pc = Cast<AReadyOrNotCharacter>(GetOwner());
			if (!pc)
			{
				return;
			}

			pc->PlayLocal1PMontage(AnimationData->Draw.Body_FP);
			pc->PlayLocal3PMontage(AnimationData->Draw.Body_TP);

			PistolEquippedWithShield->PlayFPMontage(AnimationData->Draw.Gun_FP);
			PistolEquippedWithShield->PlayTPMontage(AnimationData->Draw.Gun_TP);

			if (GetOwnerPlayerCharacter() && GetOwnerPlayerCharacter()->IsLocalPlayer())
			{
				PistolEquippedWithShield->AttachToComponent(GetOwnerPlayerCharacter()->GetMesh1P(), FAttachmentTransformRules::SnapToTargetIncludingScale, PistolEquippedWithShield->HandsSocket);
			} else
			{
				PistolEquippedWithShield->AttachToComponent(GetOwnerCharacter()->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, PistolEquippedWithShield->HandsSocket);
			}
			PistolEquippedWithShield->SetItemVisibility(true);
			//pc->SetLastEquippedItem->GetInventoryItemOfType(EItemCategory::IC_Primary);
			Client_SetPistol(newPistol);
		}
		
	}
}

void ABallisticsShield::Client_SetPistol_Implementation(ABaseItem* newPistol)
{
	if (newPistol)
	{
		UReadyOrNotSignificanceManager::UnregisterActorWithSignificanceManager(newPistol);
		// newPistol->GetItemMesh()->EmptyOverrideMaterials();
		// newPistol->EnableWeaponFovShader();
		// newPistol->TickWeaponFovShader(0.0f);
	}
}

void ABallisticsShield::DamageShieldGlass()
{
	GlassPhaseParam += 0.01f;
}

void ABallisticsShield::OnItemPrimaryUse()
{
	if (!PistolEquippedWithShield)
		return;

	if (IsBlockingAnimationPlaying())
		return;

	APlayerCharacter* pc = Cast<APlayerCharacter>(GetOwner());
	if (!pc)
		return;

	if (!pc->bAiming)
		return;

	if (PistolEquippedWithShield)
	{
		if (pc->Is1PMontagePlaying(AnimationData->Reload.Body_FP) || pc->Is1PMontagePlaying(AnimationData->Crouch_Reload.Body_FP) ||
			pc->Is1PMontagePlaying(AnimationData->ReloadEmpty.Body_FP) || pc->Is1PMontagePlaying(AnimationData->Crouch_ReloadEmpty.Body_FP))
		{
			return;
		}
	}

	if (RefireDelay > 0.0f || PistolEquippedWithShield->RefireDelayTimer > 0.0f)
		return;

	RefireDelay = PistolEquippedWithShield->RefireDelay;

	// handle fire motions here for shield usage now
	if (PistolEquippedWithShield->GetAmmo() > 0)
	{
		if (pc->bAiming && PistolEquippedWithShield->GetAmmo() <= 1)
		{
			PistolEquippedWithShield->PlayFPMontage(AnimationData->FireSingleLast.Gun_FP);
			PistolEquippedWithShield->PlayTPMontage(AnimationData->FireSingleLast.Gun_TP);
			pc->Play1PMontage(AnimationData->FireSingleLast.Body_FP);
			pc->Play3PMontage(AnimationData->FireSingleLast.Body_TP);
		}
		else
		{
			PistolEquippedWithShield->PlayFPMontage(PistolEquippedWithShield->GetRandWeapAnimFromList(AnimationData->FireSingle, AT_Gun_FP));
			PistolEquippedWithShield->PlayTPMontage(PistolEquippedWithShield->GetRandWeapAnimFromList(AnimationData->FireSingle, AT_Gun_TP));
			pc->Play1PMontage(PistolEquippedWithShield->GetRandWeapAnimFromList(AnimationData->FireSingle, AT_Body_FP));
			pc->Play3PMontage(PistolEquippedWithShield->GetRandWeapAnimFromList(AnimationData->FireSingle, AT_Body_TP));
		}
	}

	if (pc->bAiming || !pc->IsLocalPlayer())
		PistolEquippedWithShield->OnFire(PistolEquippedWithShield->BulletSpawn->GetComponentRotation(), PistolEquippedWithShield->BulletSpawn->GetComponentLocation());
	else
		PistolEquippedWithShield->OnFire(pc->GetControlRotation(), PistolEquippedWithShield->BulletSpawn->GetComponentLocation());

	if (!pc->Is1PMontagePlaying(AnimationData->FireSingleLast.Body_FP))
	{
		if (PistolEquippedWithShield->GetAmmo() <= 0 && PistolEquippedWithShield->HasAnyAmmo())
		{
			pc->Play1PMontage(AnimationData->DryFire.Body_FP);
			PistolEquippedWithShield->PlayFPMontage(AnimationData->DryFire.Gun_FP);
		}
	}
	Super::OnItemPrimaryUse();
}

void ABallisticsShield::OnItemSecondaryUsed()
{
	AReadyOrNotCharacter* pc = Cast<AReadyOrNotCharacter>(GetOwner());
	if (!pc)
	{
		return;
	}

	if (IsBlockingAnimationPlaying())
	{
		bTryingToAim = true;
		bTryingToStopAiming = false;
		return;
	}

	bTryingToAim = false;

	UReadyOrNotSignificanceManager::UnregisterActorWithSignificanceManager(PistolEquippedWithShield);

	// Transition from lowered to raised
	if (AnimationData && PistolEquippedWithShield)
	{
		if (pc->IsCrouching())
		{
			pc->Play1PMontage(AnimationData->Crouch_ShieldDownToUp.Body_FP);
			pc->Play3PMontage(AnimationData->Crouch_ShieldDownToUp.Body_TP);
			PistolEquippedWithShield->PlayFPMontage(AnimationData->Crouch_ShieldDownToUp.Gun_FP);
			PistolEquippedWithShield->PlayTPMontage(AnimationData->Crouch_ShieldDownToUp.Gun_TP);
		}
		else
		{
			pc->Play1PMontage(AnimationData->ShieldDownToUp.Body_FP);
			pc->Play3PMontage(AnimationData->ShieldDownToUp.Body_TP);
			PistolEquippedWithShield->PlayFPMontage(AnimationData->ShieldDownToUp.Gun_FP);
			PistolEquippedWithShield->PlayTPMontage(AnimationData->ShieldDownToUp.Gun_TP);
		}

	}
	Super::OnItemSecondaryUsed();
	Server_SetLowered(false);
	Server_SetLowered_Implementation(false);
}

void ABallisticsShield::OnItemEndSecondaryUse()
{
	AReadyOrNotCharacter* pc = Cast<AReadyOrNotCharacter>(GetOwner());
	if (!pc)
	{
		return;
	}

	if (IsBlockingAnimationPlaying())
	{
		bTryingToAim = false;
		bTryingToStopAiming = true;
		return;
	}

	bTryingToStopAiming = false;

	// Transition from raised to lowered
	if (AnimationData && PistolEquippedWithShield)
	{
		if (pc->IsCrouching())
		{
			pc->Play1PMontage(AnimationData->Crouch_ShieldUpToDown.Body_FP);
			pc->Play3PMontage(AnimationData->Crouch_ShieldUpToDown.Body_TP);
			PistolEquippedWithShield->PlayFPMontage(AnimationData->Crouch_ShieldUpToDown.Gun_FP);
			PistolEquippedWithShield->PlayTPMontage(AnimationData->Crouch_ShieldUpToDown.Gun_TP);
		}
		else
		{
			pc->Play1PMontage(AnimationData->ShieldUpToDown.Body_FP);
			pc->Play3PMontage(AnimationData->ShieldUpToDown.Body_TP);
			PistolEquippedWithShield->PlayFPMontage(AnimationData->ShieldUpToDown.Gun_FP);
			PistolEquippedWithShield->PlayTPMontage(AnimationData->ShieldUpToDown.Gun_TP);
		}
	}
	
	Server_SetLowered(true);
	Server_SetLowered_Implementation(true);
}

void ABallisticsShield::Client_PlayShieldHitSound_Implementation()
{
	if (!GetOwnerPlayerCharacter())
		return;

	PlayFMODAudio(ShieldHitEvent);
	
	//UFMODBlueprintStatics::PlayEventAttached(ShieldHitEvent, GetOwnerPlayerCharacter()->GetFirstPersonCameraComponent(), NAME_None, FVector::ZeroVector, EAttachLocation::SnapToTarget, true, true, true);
}

void ABallisticsShield::Server_SetLowered_Implementation(bool bShouldLower)
{
	bLowered = bShouldLower;

	if (bLowered)
	{

		if (PistolEquippedWithShield) AnimationData = PistolEquippedWithShield->ShieldLoweredAnimationData;
		DefaultAnimationData = AnimationData;
	}
	else
	{
		if (PistolEquippedWithShield) AnimationData = PistolEquippedWithShield->ShieldRaisedAnimationData;
		DefaultAnimationData = AnimationData;
	}
}

bool ABallisticsShield::PlayDraw(bool bDrawFirst)
{
	AReadyOrNotCharacter* pc = Cast<AReadyOrNotCharacter>(GetOwner());
	if (!pc)
	{
		return false;
	}
	if (pc->GetInventoryComponent()->GetSpawnedGear().Secondary && pc->GetInventoryComponent()->GetSpawnedGear().Secondary->ItemCategories.Contains(EItemCategory::IC_UseableWithShield))
	{
		SetPistol(pc->GetInventoryComponent()->GetSpawnedGear().Secondary);
	} 
	else
	{
		pc->GetInventoryComponent()->RemoveInventoryItem(this);
	}
	
	return Super::PlayDraw(false);
}

bool ABallisticsShield::PlayHolster()
{
	AReadyOrNotCharacter* pc = Cast<AReadyOrNotCharacter>(GetOwner());
	if (pc)
	{
		if (PistolEquippedWithShield)
		{
			if (PistolEquippedWithShield)
			{
				PistolEquippedWithShield->SetItemVisibility(false);
				PistolEquippedWithShield->OnRep_AttachmentRep();
			}
			PistolEquippedWithShield = nullptr;
		}
	}
	
	return Super::PlayHolster();
}

void ABallisticsShield::OnItemReload()
{
	AReadyOrNotCharacter* pc = Cast<AReadyOrNotCharacter>(GetOwner());
	if (!pc)
	{
		return;
	}

	if (IsBlockingAnimationPlaying())
	{
		return;
	}

	if (!PistolEquippedWithShield)
	{
		return;
	}

	if (!PistolEquippedWithShield->CanReload())
	{
		return;
	}

	PistolEquippedWithShield->bTacticalReload = true; // slight hack to make the magazines not disappear --eez

	// Find the next mag index for the pistol
	PistolEquippedWithShield->FindNextMagIndex();

	// FIXME: play third person animations
	APlayerController* controller = Cast<APlayerController>(pc->GetController());
	if (controller)
	{
		if (PistolEquippedWithShield->GetAmmo() <= 0 && PistolEquippedWithShield->HasAnyAmmo())
		{
			if (pc->IsCrouching())
			{
				PistolEquippedWithShield->PlayFPMontage(AnimationData->Crouch_ReloadEmpty.Gun_FP);
				PistolEquippedWithShield->PlayTPMontage(AnimationData->Crouch_ReloadEmpty.Gun_TP);
				pc->Play1PMontage(AnimationData->Crouch_ReloadEmpty.Body_FP);
				pc->Play3PMontage(AnimationData->Crouch_ReloadEmpty.Body_TP);
			}
			else
			{
				PistolEquippedWithShield->PlayFPMontage(AnimationData->ReloadEmpty.Gun_FP);
				PistolEquippedWithShield->PlayTPMontage(AnimationData->ReloadEmpty.Gun_TP);
				pc->Play1PMontage(AnimationData->ReloadEmpty.Body_FP);
				pc->Play3PMontage(AnimationData->ReloadEmpty.Body_TP);
			}


			controller->ClientStartCameraShake(ReloadEmpty_CameraShake);
		}
		else if (PistolEquippedWithShield->HasAnyAmmo())
		{
			if (pc->IsCrouching())
			{
				PistolEquippedWithShield->PlayFPMontage(AnimationData->Crouch_Reload.Gun_FP);
				PistolEquippedWithShield->PlayTPMontage(AnimationData->Crouch_Reload.Gun_TP);
				pc->Play1PMontage(AnimationData->Crouch_Reload.Body_FP);
				pc->Play3PMontage(AnimationData->Crouch_Reload.Body_TP);
			}
			else
			{
				PistolEquippedWithShield->PlayFPMontage(AnimationData->Reload.Gun_FP);
				PistolEquippedWithShield->PlayTPMontage(AnimationData->Reload.Gun_TP);
				pc->Play1PMontage(AnimationData->Reload.Body_FP);
				pc->Play3PMontage(AnimationData->Reload.Body_TP);
			}



			controller->ClientStartCameraShake(PistolEquippedWithShield->Reload_CameraShake);
		}
	}
}

void ABallisticsShield::OnItemReloadComplete()
{
	if (PistolEquippedWithShield)
	{
		PistolEquippedWithShield->OnWeaponReloadComplete();
	}
}

void ABallisticsShield::OnRep_AttachmentRep()
{
	Super::OnRep_AttachmentRep();

	if (!PistolEquippedWithShield)
		return;
	
	GetItemMesh()->SetCollisionObjectType(ECC_Pawn);
}

void ABallisticsShield::OnTPShieldHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (AReadyOrNotCharacter* owner = GetOwnerCharacter())
	{
		if (owner->GetEquippedItem() == this && AnimationData)
		{
			PlayItemAnimation(AnimationData->ShieldHit);

			PlayFMODAudio(ShieldHitEvent);
			
			if (owner->GetRONPlayerController())
			{
				owner->GetRONPlayerController()->ClientStartCameraShake(ShieldHitCameraShake);

				Client_PlayShieldHitSound();
			}
		}
	}
}

void ABallisticsShield::OnThrownFromInventory(AReadyOrNotCharacter* Thrower, bool bMarkAsEvidence)
{
	if (PistolEquippedWithShield && Thrower->GetInventoryComponent())
	{
		Thrower->GetInventoryComponent()->ThrowSpecificItem(PistolEquippedWithShield);
	}
	
	Super::OnThrownFromInventory(Thrower, bMarkAsEvidence);
}
