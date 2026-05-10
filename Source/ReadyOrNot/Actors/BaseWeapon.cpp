// Copyright Void Interactive, 2022

#include "BaseWeapon.h"

#include "Components/InteractableComponent.h"

#include "Audio/RoNSoundData.h"

#include "Attachments/LaserAttachment.h"
#include "Attachments/LightAttachment.h"
#include "Commander/BaseProfile.h"
#include "Components/FMODAudioPropagationComponent.h"

#include "lib/ReadyOrNotFunctionLibrary.h"
#include "lib/ReadyOrNotLoadoutManager.h"

#include "Materials/MaterialInstanceDynamic.h"

void ABaseWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ABaseWeapon, FireDirection, COND_SkipOwner);
	DOREPLIFETIME(ABaseWeapon, bSupressed);
	DOREPLIFETIME(ABaseWeapon, ScopeAttachment);
	DOREPLIFETIME(ABaseWeapon, MuzzleAttachment);
	DOREPLIFETIME(ABaseWeapon, UnderbarrelAttachment);
	DOREPLIFETIME(ABaseWeapon, OverbarrelAttachment);
	DOREPLIFETIME(ABaseWeapon, StockAttachment);
	DOREPLIFETIME(ABaseWeapon, GripAttachment);
	DOREPLIFETIME(ABaseWeapon, IlluminatorAttachment);
	DOREPLIFETIME(ABaseWeapon, AmmunitionAttachment);
	DOREPLIFETIME(ABaseWeapon, ProjectileMovementSpeed);
	DOREPLIFETIME(ABaseWeapon, CurrentFireMode);
}

ABaseWeapon::ABaseWeapon()
{
	BulletSpawn = CreateDefaultSubobject<UArrowComponent>(TEXT("BulletSpawn_Code"));
	BulletSpawn->SetupAttachment(ItemMesh);
	BulletSpawn->SetArrowColor(FLinearColor::Green);
	BulletSpawn->ArrowSize = 0.3f;

	ShellSpawn = CreateDefaultSubobject<UArrowComponent>(TEXT("ShellSpawn_Code"));
	ShellSpawn->SetupAttachment(ItemMesh);
	ShellSpawn->SetArrowColor(FLinearColor::Yellow);
	ShellSpawn->ArrowSize = 0.3f;

	ShellParticle = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ShellParticle"));
	ShellParticle->SetupAttachment(ShellSpawn);
	ShellParticle->SetAutoActivate(false);
	
	for (uint8 i = 0; i < 5; i++)
	{
		FString CompName = "ADS Audio Component " + FString::FromInt(i);
		UFMODAudioComponent* AudioComponent = CreateDefaultSubobject<UFMODAudioComponent>(*CompName);
		AudioComponent->bAutoActivate = false;
		AudioComponent->bAutoDestroy = false;
		AudioComponent->bStopWhenOwnerDestroyed = true;
		#if WITH_EDITORONLY_DATA
		AudioComponent->bVisualizeComponent = false;
		#endif
		AudioComponent->SetupAttachment(ItemMesh);
		AudioComponent->SetRelativeLocation(FVector::ZeroVector);
		ADSAudioComponents[i] = AudioComponent;
	}
	
	ADSAudioComponent = ADSAudioComponents[0];
	
	for (uint8 i = 0; i < 5; i++)
	{
		FString CompName = "ADS End Audio Component " + FString::FromInt(i);
		UFMODAudioComponent* AudioComponent = CreateDefaultSubobject<UFMODAudioComponent>(*CompName);
		AudioComponent->bAutoActivate = false;
		AudioComponent->bAutoDestroy = false;
		AudioComponent->bStopWhenOwnerDestroyed = true;
		#if WITH_EDITORONLY_DATA
		AudioComponent->bVisualizeComponent = false;
		#endif
		AudioComponent->SetupAttachment(ItemMesh);
		AudioComponent->SetRelativeLocation(FVector::ZeroVector);
		ADSEndAudioComponents[i] = AudioComponent;
	}
	
	ADSEndAudioComponent = ADSEndAudioComponents[0];

	// alex: default properties for proc recoil
	bCalculateProcRecoil = false;
	RecoilDampStrength = 7.5f;
	RecoilBuildupDampStrength = 15.0f;
	RecoilFireTime = 0.05f; // should match RPM of gun
	RecoilFireStrengthFirst = 3.0f;
	RecoilFireStrength = 0.5f;
	RecoilAngleStrength = 0.4f;
	RecoilRandomness = 0.1f;

	ProcRecoil_Trans = FVector(0.0f, 0.0f, 0.0f);
	ProcRecoil_Rot = FRotator(0.0f, 0.0f, 0.0f);
	ProcRecoil_TransDir = FVector(0.0f, 0.0f, 0.0f);
	ProcRecoil_RotDir = FRotator(0.0f, 0.0f, 0.0f);
	ProcRecoil_fireTime = 0.0f;
	ProcRecoil_firstFire = false;
	RecoilFireADSModifier = 0.7f;
	RecoilAngleADSModifier = 0.3f;

	ProcRecoil_BuildupADSModifier = 1.0;

	ProcRecoil_Trans_Buildup = FVector(0.0f, 0.0f, 0.0f);
	ProcRecoil_Rot_Buildup = FRotator(0.0f, 0.0f, 0.0f);
	RecoilHasBuildup = false;
}

void ABaseWeapon::BeginPlay()
{
	Super::BeginPlay();

	DefaultFireMode = AvailableFireModes.Num() > 0 ? AvailableFireModes[0] : EFireMode::FM_Single;

	SetAmmunitionTypes(AmmunitionTypes);

	// Move to equip event?
	if (GetWorld()->GetGameState<AReadyOrNotGameState>()->bPvPMode)
		LoadSavedDefaultFireMode();

	CurrentFireMode = DefaultFireMode;

	SaveCurrentFireModeToPlayerState();

	DamageType = DefaultDamageType;
	DefaultDamage = Damage;

	if (FireCameraShake)
	{
		FireCameraShakeInst = NewObject<UCameraShakeBase>(this, FireCameraShake);
		FireCameraShakeInst->bSingleInstance = true;
	}
}

void ABaseWeapon::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bHasBeenCleared)
	{
		DisableGlimmer();	
	}

	if (!GetOwnerCharacter())
	{
		return;
	}

	PendingSpread = FMath::RInterpTo(PendingSpread, FRotator::ZeroRotator, DeltaSeconds, SpreadReturnRate);

	CurrentHighTimer -= DeltaSeconds;

	// tick proc recoil if wanted
	if (bCalculateProcRecoil)
		ComputeProcRecoil(DeltaSeconds);
	
	// alex moved this from fp blueprint to here
	// calculate ADS Weight Blend
	APlayerCharacter* pc = Cast<APlayerCharacter>(GetOwner());
	if (pc)
	{
		if (pc->bAiming)
		{
			CUR_FPS_ADS_Weight = FMath::FInterpConstantTo(CUR_FPS_ADS_Weight, FP_ADS_Motion_Weight, DeltaSeconds, 8.0f);
		}
		else
		{
			CUR_FPS_ADS_Weight = FMath::FInterpConstantTo(CUR_FPS_ADS_Weight, 1.0f, DeltaSeconds, 8.0f);
		}
	}
}


void ABaseWeapon::OnWeaponReloadComplete()
{
	
}

void ABaseWeapon::ResetRecoilSettingsToDefault()
{
	if (IsTemplate())
		return;

	ABaseWeapon* DefaultObject = GetClass()->GetDefaultObject<ABaseWeapon>();
	if (!ensure(DefaultObject)) // i don't think this can ever happen
		return;
	
	bCalculateProcRecoil = DefaultObject->bCalculateProcRecoil;
	RecoilDampStrength = DefaultObject->RecoilDampStrength;
	RecoilFireTime = DefaultObject->RecoilFireTime;
	RecoilFireStrength = DefaultObject->RecoilFireStrength;
	RecoilFireStrengthFirst = DefaultObject->RecoilFireStrengthFirst;
	RecoilAngleStrength = DefaultObject->RecoilAngleStrength;
	RecoilRandomness = DefaultObject->RecoilRandomness;
	RecoilFireADSModifier = DefaultObject->RecoilFireADSModifier;
	RecoilAngleADSModifier = DefaultObject->RecoilAngleADSModifier;
	RecoilRotationBuildup = DefaultObject->RecoilRotationBuildup;
	RecoilPositionBuildup = DefaultObject->RecoilPositionBuildup;
	RecoilBuildupADSModifier = DefaultObject->RecoilBuildupADSModifier;
	RecoilHasBuildup = DefaultObject->RecoilHasBuildup;
	RecoilBuildupDampStrength = DefaultObject->RecoilBuildupDampStrength;
}

void ABaseWeapon::Client_OnItemPickedUp_Implementation(AActor* NewOwner, bool bEquipped)
{
	Super::Client_OnItemPickedUp_Implementation(NewOwner, bEquipped);

	APlayerCharacter* pc = Cast<APlayerCharacter>(GetOwner());
	AReadyOrNotPlayerState* ps = pc ? Cast<AReadyOrNotPlayerState>(pc->GetPlayerState()) : nullptr;

	if (ps)
	{
		if (AvailableFireModes.Contains(ps->LastFireMode))
		{
			CurrentFireMode = ps->LastFireMode;
		}
	}

	DisableGlimmer();
}

void ABaseWeapon::OnOwnerPossessed_Implementation()
{
	DefaultFireMode = AvailableFireModes.Num() > 0 ? AvailableFireModes[0] : EFireMode::FM_Single;
	
	LoadSavedDefaultFireMode();

	EFireMode LastFireMode = LoadLastFireModeFromPlayerState();
	
	CurrentFireMode = LastFireMode;;
}

void ABaseWeapon::EnableGlimmer()
{
	AReadyOrNotGameState* GS = Cast<AReadyOrNotGameState>(GetWorld()->GetGameState());

	if (GS && GS->bPvPMode)
		return;
	
	USkeletalMeshComponent* TPWeaponMesh = GetItemMesh();
	if (TPWeaponMesh)
	{
		TArray<UMaterialInterface*> TPWeaponMaterials = TPWeaponMesh->GetMaterials();
		for (UMaterialInterface* TPWeaponMaterial : TPWeaponMaterials)
		{
			UMaterialInstanceDynamic* TPWeaponMID = Cast<UMaterialInstanceDynamic>(TPWeaponMaterial);
			
			if (TPWeaponMID)
			{
				TPWeaponMID->SetScalarParameterValue("GlimmerPower", GlimmerIntensity);
				TPWeaponMID->SetScalarParameterValue("ToggleGlimmer", 1.0f);
			}
		}
	}
}

void ABaseWeapon::DisableGlimmer()
{
	AReadyOrNotGameState* GS = Cast<AReadyOrNotGameState>(GetWorld()->GetGameState());

	if (GS && GS->bPvPMode)
		return;
	
	USkeletalMeshComponent* TPWeaponMesh = GetItemMesh();
	if (TPWeaponMesh)
	{
		TArray<UMaterialInterface*> TPWeaponMaterials = TPWeaponMesh->GetMaterials();
		for (UMaterialInterface* TPWeaponMaterial : TPWeaponMaterials)
		{
			UMaterialInstanceDynamic* TPWeaponMID = Cast<UMaterialInstanceDynamic>(TPWeaponMaterial);
			
			if (TPWeaponMID)
			{
				TPWeaponMID->SetScalarParameterValue("GlimmerPower", 0.0f);
				TPWeaponMID->SetScalarParameterValue("ToggleGlimmer", 0.0f);
			}
		}
	}
}

void ABaseWeapon::OnFire(FRotator Direction, FVector SpawnLoc)
{
	CurrentHighTimer = FireHighTimer;
}

void ABaseWeapon::OnFireAtBulletSpawn()
{
	if(AimAssistRotation == FRotator::ZeroRotator)
	{
		OnFire(GetBulletSpawn()->GetComponentRotation(), GetBulletSpawn()->GetComponentLocation()); 
	}
	else
	{
		OnFire(AimAssistRotation, GetBulletSpawn()->GetComponentLocation());
	}
}

void ABaseWeapon::LoadSavedDefaultFireMode()
{
	UBaseProfile* Profile = UBaseProfile::GetCurrentProfile();
	if (!Profile)
		return;

	EFireMode* FireMode = Profile->WeaponClassToDefaultFireModeMap.Find(GetClass());
	if (FireMode)
		DefaultFireMode = *FireMode;
}

void ABaseWeapon::SaveCurrentFireModeToPlayerState()
{
	if (APlayerCharacter* pc = Cast<APlayerCharacter>(GetOwner()))
	{
		if (AReadyOrNotPlayerState* ps = pc->GetPlayerState<AReadyOrNotPlayerState>())
		{
			ps->LastFireMode = CurrentFireMode;
		}
	}
}

EFireMode ABaseWeapon::LoadLastFireModeFromPlayerState() const
{
	if (APlayerCharacter* pc = Cast<APlayerCharacter>(GetOwner()))
	{
		if (AReadyOrNotPlayerState* ps = pc->GetPlayerState<AReadyOrNotPlayerState>())
		{
			return ps->LastFireMode;
		}
	}
	
	return DefaultFireMode;
}

void ABaseWeapon::PlayFiringModeAnimation()
{
	APlayerCharacter* pc = Cast<APlayerCharacter>(GetOwner());
	if (pc)
	{
		if (AnimationData)
		{
			if (!pc->IsCrouching())
			{
				if (CurrentFireMode == EFireMode::FM_Single)
				{
					pc->PlayLocal1PMontage(AnimationData->FireSelect_Semi.Body_FP);
					pc->PlayNonLocal3PMontage(AnimationData->FireSelect_Semi.Body_TP);
				}
				else if (CurrentFireMode == EFireMode::FM_Burst)
				{
					pc->PlayLocal1PMontage(AnimationData->FireSelect_Burst.Body_FP);
					pc->PlayNonLocal3PMontage(AnimationData->FireSelect_Burst.Body_TP);
				}
				else if (CurrentFireMode == EFireMode::FM_Auto)
				{
					pc->PlayLocal1PMontage(AnimationData->FireSelect_Auto.Body_FP);
					pc->PlayNonLocal3PMontage(AnimationData->FireSelect_Auto.Body_TP);
				}
				else if (CurrentFireMode == EFireMode::FM_Safe)
				{
					pc->PlayLocal1PMontage(AnimationData->FireSelect_Safe.Body_FP);
					pc->PlayNonLocal3PMontage(AnimationData->FireSelect_Safe.Body_TP);
				}
			}
			else
			{
				if (CurrentFireMode == EFireMode::FM_Single)
				{
					pc->PlayLocal1PMontage(AnimationData->Crouch_FireSelect_Semi.Body_FP);
					pc->PlayNonLocal3PMontage(AnimationData->Crouch_FireSelect_Semi.Body_TP);
				}
				else if (CurrentFireMode == EFireMode::FM_Burst)
				{
					pc->PlayLocal1PMontage(AnimationData->Crouch_FireSelect_Burst.Body_FP);
					pc->PlayNonLocal3PMontage(AnimationData->Crouch_FireSelect_Burst.Body_TP);
				}
				else if (CurrentFireMode == EFireMode::FM_Auto)
				{
					pc->PlayLocal1PMontage(AnimationData->Crouch_FireSelect_Auto.Body_FP);
					pc->PlayNonLocal3PMontage(AnimationData->Crouch_FireSelect_Auto.Body_TP);
				}
				else if (CurrentFireMode == EFireMode::FM_Safe)
				{
					pc->PlayLocal1PMontage(AnimationData->Crouch_FireSelect_Safe.Body_FP);
					pc->PlayNonLocal3PMontage(AnimationData->Crouch_FireSelect_Safe.Body_TP);
				}
			}

		}
	}
}

void ABaseWeapon::NextFireMode()
{
	const EFireMode FireModeBefore = CurrentFireMode;
	if (AvailableFireModes.Num() <= 0)
	{
		CurrentFireMode = EFireMode::FM_Single;
	}
	else if (FireModeBefore == EFireMode::FM_Safe)
	{
		CurrentFireMode = FiremodeBeforeSafe;
	}
	else if (AvailableFireModes.Num() > 0)
	{
		int32 FoundIndex;
		AvailableFireModes.Find(CurrentFireMode, FoundIndex);

		if (AvailableFireModes.IsValidIndex(FoundIndex + 1))
			CurrentFireMode = AvailableFireModes[FoundIndex + 1];
		else
			CurrentFireMode = AvailableFireModes[0];
	}

	// Is new fire mode selected?
	if (FireModeBefore != CurrentFireMode)
	{
		PlayFiringModeAnimation();

		SaveCurrentFireModeToPlayerState();
	}
}

void ABaseWeapon::SafeModeToggle()
{
	return; // safe mode deprecated

	/*
	// If we are currently on safe mode, bring us back to the previous firing mode.
	// If we are currently on a non-safe mode, bring us to safe mode.
	if (CurrentFireMode == EFireMode::FM_Safe)
	{
		CurrentFireMode = FiremodeBeforeSafe;
		PlayFiringModeAnimation();
	}
	else if (bHasSafeMode)
	{
		FiremodeBeforeSafe = CurrentFireMode;
		CurrentFireMode = EFireMode::FM_Safe;
		PlayFiringModeAnimation();
	}
	*/
}

bool ABaseWeapon::IsLethalWeapon() const
{
	return ItemClass == EItemClass::IC_Pistol || ItemClass == EItemClass::IC_Shotgun || ItemClass == EItemClass::IC_Sniper || ItemClass == EItemClass::IC_SMG || ItemClass == EItemClass::IC_LMG || ItemClass == EItemClass::IC_AssaultRifle;
}

bool ABaseWeapon::IsLessLethalWeapon() const
{
	return ItemClass == EItemClass::IC_LessLethal;
}

void ABaseWeapon::OnWeaponReload(bool bForce)
{
	CurrentHighTimer = ReloadHighTimer;
	OnWeaponReloadStarted();
}

bool ABaseWeapon::CanReload()
{
	return true;
}

void ABaseWeapon::OnAimDownSights(bool bWasAiming)
{
	if (!GetOwnerCharacter())
		return;
	
	bIsAimingDownSights = true;
	
	if (SoundData)
	{
		/*if (ADSSoundInstance.Instance)
		{
			ADSSoundInstance.Instance->stop(FMOD_STUDIO_STOP_IMMEDIATE);
		}*/

		/*
		if (GetOwnerCharacter()->GetFMODPropagationComponent()->IsPlaying())
		{
			GetOwnerCharacter()->GetFMODPropagationComponent()->Stop();
		}
		*/

		ADSAudioComponent = ADSAudioComponents[ADSCompIndex];
		ADSCompIndex++;
		if (ADSCompIndex >= 4)
			ADSCompIndex = 0;
		
		//GetOwnerCharacter()->GetFMODPropagationComponent()->PlayEventAttached(SoundData->OnADSSound, ItemMesh, NAME_None,{});
		
		USoundSource* SoundSource = USoundSource::CreateThirdPersonSound(GetWorld(), SoundData->OnADSSound, GetItemMesh()->GetComponentTransform(), {}, EOcclusionType::OT_Angular, EPropagationType::PT_Portal, false);
		if(SoundSource)
		{
			SoundSource->Attach(GetItemMesh(), "");
			SoundSource->Play();
		}
		
	}
}

void ABaseWeapon::OnEndAimDownSights(bool bWasAiming)
{
	if (!GetOwnerCharacter())
		return;
	
	bIsAimingDownSights = false;

	//if (pc && IsLocallyControlled())
	//{
	//	StopScopeMask();
	//}
	
	if (SoundData)
	{
	/*	if (ADSSoundInstance.Instance)
		{
			ADSSoundInstance.Instance->stop(FMOD_STUDIO_STOP_IMMEDIATE);
		}  */

		/*
		if (GetOwnerCharacter()->GetFMODPropagationComponent()->IsPlaying())
		{
			GetOwnerCharacter()->GetFMODPropagationComponent()->Stop();
		}
		*/
		
		ADSEndAudioComponent = ADSEndAudioComponents[ADSEndCompIndex];
		ADSEndCompIndex++;
		if (ADSEndCompIndex >= 4)
			ADSEndCompIndex = 0;
		
		//GetOwnerCharacter()->GetFMODPropagationComponent()->PlayEventAttached(SoundData->OnADSSound, ItemMesh, NAME_None,{},false);
		
		USoundSource* SoundSource = USoundSource::CreateThirdPersonSound(GetWorld(), SoundData->OnADSSound, GetItemMesh()->GetComponentTransform(), {}, EOcclusionType::OT_Angular, EPropagationType::PT_Portal, false);
		if(SoundSource)
		{
			SoundSource->Attach(GetItemMesh(), "");
			SoundSource->Play();
		}
		
		//ADSSoundInstance = UFMODBlueprintStatics::PlayEvent2D(GetWorld(), SoundData->OnEndADSSound, true);
	}	
}

//void ABaseWeapon::AdvanceRecoilCurveTime(const float DeltaTime)
//{
//	RecoilCurveTime += DeltaTime;
//
//	if (bUseRecoilCurve)
//	{
//		if (RecoilCurve.ExternalCurve)
//		{
//			if (RecoilCurveTime > RecoilCurve.ExternalCurve->FloatCurves->GetLastKey().Time)
//			{
//				RecoilCurveTime = 0.0f;
//			}
//		}
//		else
//		{
//			if (RecoilCurveTime > RecoilCurve.ColorCurves[UE_ARRAY_COUNT(RecoilCurve.ColorCurves)].GetLastKey().Time)
//			{
//				RecoilCurveTime = 0.0f;
//			}
//		}
//	}
//}

FRotator ABaseWeapon::GetRecoil()
{
	// No Recoil Pattern.
	if (RecoilPattern.Num() <= 0)
		return FRotator::ZeroRotator;
	
	FRotator ReturnRecoil;

	if (RecoilPattern.IsValidIndex(RecoilIndex))
	{
		// Update our Recoil Index.
		RecoilIndex += 1;

		// Get the last one seeing as we just updated it.
		ReturnRecoil = RecoilPattern[RecoilIndex-1];
	}
	else
	{
		// Recoil index isn't valid, we should set it to 0 again.
		RecoilIndex = 0;
		ReturnRecoil = RecoilPattern[RecoilIndex];
	}
	
	return (ItemClass == EItemClass::IC_AssaultRifle ? FRotator(ReturnRecoil.Pitch, 0.0f, ReturnRecoil.Roll) : ReturnRecoil);
	//return returnRecoil; //TODO: Revert back to this
}

void ABaseWeapon::SetAmmunitionTypes(const TArray<FName>& AmmoTypes)
{
	if (AmmoTypes.Num() <= 0)
		return;

	AmmunitionTypes = AmmoTypes;

	for (const FName& AmmoType : AmmoTypes)
	{
		if (AmmoType.IsNone())
			continue;

		SetAmmunitionType(AmmoType);
		return;
	}
	SetAmmunitionType(FName());
}

void ABaseWeapon::SetAmmunitionType(FName AmmoTypeRowName)
{
	if (CurrentAmmoTypeRowName == AmmoTypeRowName)
		return;

	CurrentAmmoType = FAmmoTypeData();
	CurrentAmmoTypeRowName = FName();

	if (AmmoTypeRowName.IsNone())
		return;
	
	if (!AmmoDataTable)
		return;

	FAmmoTypeData* AmmoType = AmmoDataTable->FindRow<FAmmoTypeData>(AmmoTypeRowName, "Ammo Type Lookup");
	if (!AmmoType)
		return;

	// Perform a copy of the data table ammo type
	CurrentAmmoType = *AmmoType;
	CurrentAmmoTypeRowName = AmmoTypeRowName;

	SpawnProjectileCount = CurrentAmmoType.ProjectileCount;
}

void ABaseWeapon::SetAmmunitionType(int32 AmmoTypeIndex)
{
	if (AmmunitionTypes.IsValidIndex(AmmoTypeIndex))
	{
		SetAmmunitionType(AmmunitionTypes[AmmoTypeIndex]);
	}
	else if (AmmunitionTypes.IsValidIndex(0))
	{
		SetAmmunitionType(AmmunitionTypes[0]);
	}
}

void ABaseWeapon::AddAttachment(UClass* Class, bool bReplicateAttachment)
{
	if (GetLocalRole() < ROLE_Authority && bReplicateAttachment)
		return;

	if (!Class)
		return;

	const TSubclassOf<UWeaponAttachment> AttachmentClass = (TSubclassOf<UWeaponAttachment>)(Class);
	if (AttachmentClass)
	{
		// already on here yo
		if (GetComponentByClass(AttachmentClass) != nullptr)
			return;

		if (!CanAddAttachment(AttachmentClass))
			return;

		const EWeaponAttachmentType AttachmentType = AttachmentClass->GetDefaultObject<UWeaponAttachment>()->WeaponAttachmentType;
		TArray<EWeaponAttachmentType> RemovesWeaponAttachmentTypes = AttachmentClass->GetDefaultObject<UWeaponAttachment>()->RemovesWeaponAttachmentTypes;
		if (AvailableScopeAttachments.Contains(AttachmentClass))
		{
			RemovesWeaponAttachmentTypes.Add(EWeaponAttachmentType::Optics);
		}

		if (AvailableMuzzleAttachments.Contains(AttachmentClass))
		{
			RemovesWeaponAttachmentTypes.Add(EWeaponAttachmentType::Muzzle);
		}

		if (AvailableUnderbarrelAttachments.Contains(AttachmentClass))
		{
			RemovesWeaponAttachmentTypes.Add(EWeaponAttachmentType::Underbarrel);
		}

		if (AvailableOverbarrelAttachments.Contains(AttachmentClass))
		{
			RemovesWeaponAttachmentTypes.Add(EWeaponAttachmentType::Overbarrel);
		}

		if (AvailableStockAttachments.Contains(AttachmentClass))
		{
			RemovesWeaponAttachmentTypes.Add(EWeaponAttachmentType::Stock);
		}

		if (AvailableGripAttachments.Contains(AttachmentClass))
		{
			RemovesWeaponAttachmentTypes.Add(EWeaponAttachmentType::Grip);
		}

		if (AvailableIlluminatorAttachments.Contains(AttachmentClass))
		{
			RemovesWeaponAttachmentTypes.Add(EWeaponAttachmentType::Illuminators);
		}
		
		if (AvailableAmmunitionAttachments.Contains(AttachmentClass))
		{
			RemovesWeaponAttachmentTypes.Add(EWeaponAttachmentType::Ammunition);
		}
		
		RemoveAttachment(RemovesWeaponAttachmentTypes.Contains(EWeaponAttachmentType::Optics), RemovesWeaponAttachmentTypes.Contains(EWeaponAttachmentType::Muzzle),
			RemovesWeaponAttachmentTypes.Contains(EWeaponAttachmentType::Underbarrel), RemovesWeaponAttachmentTypes.Contains(EWeaponAttachmentType::Overbarrel),
			RemovesWeaponAttachmentTypes.Contains(EWeaponAttachmentType::Stock), RemovesWeaponAttachmentTypes.Contains(EWeaponAttachmentType::Grip),
			RemovesWeaponAttachmentTypes.Contains(EWeaponAttachmentType::Illuminators), RemovesWeaponAttachmentTypes.Contains(EWeaponAttachmentType::Ammunition));

		// We destroyed the attachments now reutrn before creating anything
		if (AttachmentClass->GetDefaultObject<UWeaponAttachment>()->bNullAttachmentOnly)
		{
			return;
		}

		UWeaponAttachment* NewAttachment = NewObject<UWeaponAttachment>(this, AttachmentClass);

		if (NewAttachment && AttachmentType != EWeaponAttachmentType::Null)
		{
			NewAttachment->RegisterComponent();
			NewAttachment->SetIsReplicated(bReplicateAttachment);
			NewAttachment->SetOnlyOwnerSee(true);

			switch (AttachmentType)
			{
			case EWeaponAttachmentType::Null:
				NewAttachment->DestroyComponent();
				NewAttachment = nullptr;
				return;
			case EWeaponAttachmentType::Optics:
				ScopeAttachment = Cast<UScopedWeaponAttachment>(NewAttachment);
				if (ScopeAttachment)
				{
					if (ScopeAttachment->GetScopeMods(this).CustomWeaponMesh && ScopeAttachment->GetScopeMods(this).CustomWeaponMesh->IsValidLowLevel())
					{
						Rep_CustomItemMeshFromAttachment = ScopeAttachment->GetScopeMods(this).CustomWeaponMesh;
					}
				}
				break;
			case EWeaponAttachmentType::Muzzle:
				MuzzleAttachment = NewAttachment;
				break;
			case EWeaponAttachmentType::Underbarrel:
				UnderbarrelAttachment = NewAttachment;
				break;
			case EWeaponAttachmentType::Overbarrel:
				OverbarrelAttachment = NewAttachment;
				break;
			case EWeaponAttachmentType::Stock:
				StockAttachment = NewAttachment;
				break;
			case EWeaponAttachmentType::Grip:
				GripAttachment = NewAttachment;
				break;
			case EWeaponAttachmentType::Illuminators:
				IlluminatorAttachment = NewAttachment;
				break;
			case EWeaponAttachmentType::Ammunition:
				AmmunitionAttachment = NewAttachment;
				break;
			default:
				break;
			}
			if (GetItemMesh()->DoesSocketExist(NewAttachment->SocketAttachment))
			{
				NewAttachment->AttachToComponent(GetItemMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, NewAttachment->SocketAttachment);

			}
			else if (NewAttachment->bNeedNotAttach)
			{
				if (NewAttachment)
				{
					NewAttachment->SetVisibility(false);
				}
			}
		}

		// Update for server as RepGear doesn't replicate here :)
		OnRep_AttachmentRep();
	}
}

void ABaseWeapon::RemoveAttachment(bool bScopedAttachment /*= false*/, bool bMuzzleAttachment /*= false*/, bool bUnderbarrelAttachment /*= false*/, bool bOverbarrelAttachment, bool bStockAttachment, bool bGripAttachment, bool bIlluminatorAttachment, bool bAmmunitionAttachment)
{
	if (bScopedAttachment && ScopeAttachment)
	{
		ScopeAttachment->DestroyComponent();
		ScopeAttachment = nullptr;
	}

	if (bMuzzleAttachment && MuzzleAttachment)
	{
		MuzzleAttachment->DestroyComponent();
		MuzzleAttachment = nullptr;
	}

	if (bOverbarrelAttachment && OverbarrelAttachment)
	{
		OverbarrelAttachment->DestroyComponent();
		OverbarrelAttachment = nullptr;
	}

	if (bUnderbarrelAttachment && UnderbarrelAttachment)
	{
		UnderbarrelAttachment->DestroyComponent();
		UnderbarrelAttachment = nullptr;
	}

	if (bStockAttachment && StockAttachment)
	{
		StockAttachment->DestroyComponent();
		StockAttachment = nullptr;
	}

	if (bGripAttachment && GripAttachment)
	{
		GripAttachment->DestroyComponent();
		GripAttachment = nullptr;
	}

	if (bIlluminatorAttachment && IlluminatorAttachment)
	{
		IlluminatorAttachment->DestroyComponent();
		IlluminatorAttachment = nullptr;
	}

	if (bAmmunitionAttachment && AmmunitionAttachment)
	{
		AmmunitionAttachment->DestroyComponent();
		AmmunitionAttachment = nullptr;
	}
}

bool ABaseWeapon::CanAddAttachment(UClass* Class)
{
	const TSubclassOf<UWeaponAttachment> AttachmentClass = Class;
	
	if (!AvailableMuzzleAttachments.Contains(AttachmentClass) &&
		!AvailableScopeAttachments.Contains(AttachmentClass) &&
		!AvailableUnderbarrelAttachments.Contains(AttachmentClass) &&
		!AvailableOverbarrelAttachments.Contains(AttachmentClass) &&
		!AvailableStockAttachments.Contains(AttachmentClass) &&
		!AvailableGripAttachments.Contains(AttachmentClass) &&
		!AvailableIlluminatorAttachments.Contains(AttachmentClass) &&
		!AvailableAmmunitionAttachments.Contains(AttachmentClass))
	{
		return false;
	}

	if (AttachmentClass)
	{
		UWeaponAttachment* Temp_ScopeAttachment = AttachmentClass->GetDefaultObject<UWeaponAttachment>();
		if (Temp_ScopeAttachment)
		{
			if (GetClass()->GetDefaultObject<ABaseWeapon>()->ItemMesh->GetAllSocketNames().Contains(Temp_ScopeAttachment->SocketAttachment) || Temp_ScopeAttachment->SocketAttachment == NAME_None)
			{
				return true;
			}
		}
	}

	return false;
}

void ABaseWeapon::UpdateStoredAttachments(TSubclassOf<UWeaponAttachment> Attachment)
{
	//update map of stored attachments by weapon when new attachment is equipped
	UWeaponAttachment* DefaultObject = Attachment.GetDefaultObject();
	LoadoutFunctionLibrary = GetWorld()->GetGameState<AReadyOrNotGameState>()->LoadoutFunctionLibrary;
	FStoredWeaponAttachments TempStoredAttachments = FStoredWeaponAttachments();
	FStoredWeaponAttachments StoredWeaponAttachments = LoadoutFunctionLibrary->StoredAttachmentsByWeapon.FindRef(GetClass());
	if (DefaultObject == nullptr)
		return;
	//if there are attachments stored for this weapon, populate Temp with stored values
	if (!StoredWeaponAttachments.bIsEmpty)
		TempStoredAttachments = StoredWeaponAttachments;

	EWeaponAttachmentType DOAttachmentType = DefaultObject->WeaponAttachmentType;
	if ((DOAttachmentType == EWeaponAttachmentType::Null) && DefaultObject->RemovesWeaponAttachmentTypes.Num() != 0)
		DOAttachmentType = DefaultObject->RemovesWeaponAttachmentTypes[0];

	switch (DOAttachmentType)
	{
		case EWeaponAttachmentType::Optics:
			TempStoredAttachments.ScopeAttachment = Attachment;
			TempStoredAttachments.bIsEmpty = false;
			break;
		case EWeaponAttachmentType::Muzzle:
			TempStoredAttachments.MuzzleAttachment = Attachment;
			TempStoredAttachments.bIsEmpty = false;
			break;
		case EWeaponAttachmentType::Underbarrel:
			TempStoredAttachments.UnderbarrelAttachment = Attachment;
			TempStoredAttachments.bIsEmpty = false;
			break;
		case EWeaponAttachmentType::Overbarrel:
			TempStoredAttachments.OverbarrelAttachment = Attachment;
			TempStoredAttachments.bIsEmpty = false;
			break;
		case EWeaponAttachmentType::Stock:
			TempStoredAttachments.StockAttachment = Attachment;
			TempStoredAttachments.bIsEmpty = false;
			break;
		case EWeaponAttachmentType::Grip:
			TempStoredAttachments.GripAttachment = Attachment;
			TempStoredAttachments.bIsEmpty = false;
			break;
		case EWeaponAttachmentType::Illuminators:
			TempStoredAttachments.IlluminatorAttachment = Attachment;
			TempStoredAttachments.bIsEmpty = false;
			break;
		case EWeaponAttachmentType::Ammunition:
			TempStoredAttachments.AmmunitionAttachment = Attachment;
			TempStoredAttachments.bIsEmpty = false;
			break;
		default:
			break;
	}

	LoadoutFunctionLibrary->StoredAttachmentsByWeapon.Add(GetClass(), TempStoredAttachments);
}

ULaserAttachment* ABaseWeapon::GetLaserAttachment()
{
	if (Cast<ULaserAttachment>(UnderbarrelAttachment))
	{
		return Cast<ULaserAttachment>(UnderbarrelAttachment);
	}

	if (Cast<ULaserAttachment>(OverbarrelAttachment))
	{
		return Cast<ULaserAttachment>(OverbarrelAttachment);
	}

	if (Cast<ULaserAttachment>(IlluminatorAttachment))
	{
		return Cast<ULaserAttachment>(IlluminatorAttachment);
	}
	
	return nullptr;
}

ULightAttachment* ABaseWeapon::GetLightAttachment()
{
	if (Cast<ULightAttachment>(UnderbarrelAttachment))
	{
		return Cast<ULightAttachment>(UnderbarrelAttachment);
	}

	if (Cast<ULightAttachment>(OverbarrelAttachment))
	{
		return Cast<ULightAttachment>(OverbarrelAttachment);
	}
	
	if (Cast<ULightAttachment>(IlluminatorAttachment))
	{
		return Cast<ULightAttachment>(IlluminatorAttachment);
	}

	return nullptr;
}

bool ABaseWeapon::GetMeshspaceTransform(FTransform& Default, FTransform& Aiming, FTransform& Back)
{
	Default = MeshspaceTransform_Default;
	Aiming = MeshspaceTransform_Aiming;
	Back = MeshspaceTransform_Back;
	if (GetScopedAttachment())
	{
		FVector Offset = Aiming.GetLocation();
		Offset.Z +=	GetScopedAttachment()->GetMeshspaceOffsetVertical(this);
		Offset.Y += GetScopedAttachment()->GetMeshspaceOffsetHorizontal(this);
		Offset.X += GetScopedAttachment()->GetMeshspaceOffsetDistance(this);
		Aiming.SetLocation(Offset);
	}
	return true;
}

float ABaseWeapon::GetADSZoomMultiplier()
{
	if (GetScopedAttachment())
	{
		return GetScopedAttachment()->ZoomFOVAddition + ADSZoom;
	}
	return ADSZoom;
}

float ABaseWeapon::GetADSZoomInSpeed()
{
	if (GetScopedAttachment())
	{
		return GetScopedAttachment()->ZoomInSpeed;
	}
	return ADSZoomInSpeed;
}

float ABaseWeapon::GetADSZoomOutSpeed()
{
	if (GetScopedAttachment())
	{
		return GetScopedAttachment()->ZoomOutSpeed;
	}
	return ADSZoomOutSpeed;
}

void ABaseWeapon::ResetRefireDelay()
{
	bRefireDelayTriggered = false;
}

void ABaseWeapon::ResetFirstShot()
{
	bFirstShot = true;
}

/*void ABaseWeapon::ResetLastShot()
{
	bLastShot = false;
	
	#if WITH_EDITOR
	ULog::Info("Last Shot Reset...");
	#endif
}*/

bool ABaseWeapon::IsFirstShot()
{
	return bFirstShot;
}

/*
bool ABaseWeapon::IsLastShot()
{
	return bLastShot;
}
*/

void ABaseWeapon::TriggerFirstShot()
{
	bFirstShot = FirstShotResetTime > 0.0f ? true : false;
	
	GetWorld()->GetTimerManager().SetTimer(ResetFirstShot_Handle, this, &ABaseWeapon::ResetFirstShot, FirstShotResetTime, false);
}

/*void ABaseWeapon::TriggerLastShot()
{
	//bLastShot = LastShotResetTime > 0.0f ? true : false;
	//
	//UReadyOrNotFunctionLibrary::StartTimerForCallback(ResetLastShot_Handle, this, &ABaseWeapon::ResetLastShot, LastShotResetTime, false);

	#if WITH_EDITOR
	ULog::Info("Last Shot Triggered...");
	#endif
}*/

void ABaseWeapon::IncrementShotsFired()
{
	RecentShotsFired++;
	
	GetWorld()->GetTimerManager().ClearTimer(ResetShotsFired_Handle);
	UReadyOrNotFunctionLibrary::StartTimerForCallback(ResetShotsFired_Handle, this, &ABaseWeapon::ResetShotsFired, CurrentFireMode == EFireMode::FM_Single ? FireRate * 2 : FireRate + (FireRate*0.1f), false);
}

void ABaseWeapon::ResetShotsFired()
{
	if (RecentShotsFired > 0)
	{
		if (APlayerCharacter* pc = Cast<APlayerCharacter>(GetOwner()))
		{
			pc->PendingRecoil = (pc->AccumulatedPendingRecoil * -1.0f) * RecoilReturnPercentage;
			pc->AccumulatedPendingRecoil = FRotator::ZeroRotator;
		}

		bReturnRecoil = true;
	}
	
	RecentShotsFired = 0;
}

FRotator ABaseWeapon::GetSpread()
{
	FRotator returnSpread;

	returnSpread.Pitch = FMath::FRandRange(-SpreadPattern.Pitch, SpreadPattern.Pitch);
	returnSpread.Yaw = FMath::FRandRange(-SpreadPattern.Yaw, SpreadPattern.Yaw);
	returnSpread.Roll = FMath::FRandRange(-SpreadPattern.Roll, SpreadPattern.Roll);

	return returnSpread;
}

void ABaseWeapon::OnRep_AttachmentRep()
{
	if (Cast<ACyberneticCharacter>(GetOwner()))
	{
		BulletSpawn->AttachToComponent(ItemMesh, FAttachmentTransformRules::KeepRelativeTransform, BulletSpawn->GetAttachSocketName());
		ShellSpawn->AttachToComponent(ItemMesh, FAttachmentTransformRules::KeepRelativeTransform, ShellSpawn->GetAttachSocketName());

	}

	Super::OnRep_AttachmentRep();
	//USceneComponent* AttachMesh = nullptr;
	//if (IsLocallyControlled())
	//{
	//	AttachMesh = ItemMesh;
	//}
	//else
	//{
	//	AttachMesh = ItemMesh;
	//}

	const float PreviousMagazineAdd = AddedMagazineCountFromAttachments;
	float CurrentMagazineAdd = 0.0f;

	// attachments are not always valid...
	if (ScopeAttachment)
	{
		ScopeAttachment->AttachToComponent(ItemMesh, FAttachmentTransformRules::KeepRelativeTransform, ScopeAttachment->SocketAttachment);
		ScopeAttachment->SetVisibility(ItemMesh->IsVisible());
		CurrentMagazineAdd += ScopeAttachment->MagazineAmmoIncrease;
	}

	if (ScopeAttachment)
	{
		ScopeAttachment->AttachToComponent(ItemMesh, FAttachmentTransformRules::KeepRelativeTransform, ScopeAttachment->SocketAttachment);
		ScopeAttachment->SetVisibility(ItemMesh->IsVisible());
	}

	if (MuzzleAttachment)
	{
		MuzzleAttachment->AttachToComponent(ItemMesh, FAttachmentTransformRules::KeepRelativeTransform, MuzzleAttachment->SocketAttachment);
		MuzzleAttachment->SetVisibility(ItemMesh->IsVisible());
		CurrentMagazineAdd += MuzzleAttachment->MagazineAmmoIncrease;
	}

	if (UnderbarrelAttachment)
	{
		UnderbarrelAttachment->AttachToComponent(ItemMesh, FAttachmentTransformRules::KeepRelativeTransform, UnderbarrelAttachment->SocketAttachment);
		UnderbarrelAttachment->SetVisibility(ItemMesh->IsVisible());
		CurrentMagazineAdd += UnderbarrelAttachment->MagazineAmmoIncrease;
	}

	if (OverbarrelAttachment)
	{
		OverbarrelAttachment->AttachToComponent(ItemMesh, FAttachmentTransformRules::KeepRelativeTransform, OverbarrelAttachment->SocketAttachment);
		OverbarrelAttachment->SetVisibility(ItemMesh->IsVisible());
		CurrentMagazineAdd += OverbarrelAttachment->MagazineAmmoIncrease;
	}
	
	if (StockAttachment)
	{
		StockAttachment->AttachToComponent(ItemMesh, FAttachmentTransformRules::KeepRelativeTransform, StockAttachment->SocketAttachment);
		StockAttachment->SetVisibility(ItemMesh->IsVisible());
		CurrentMagazineAdd += StockAttachment->MagazineAmmoIncrease;
	}

	if (GripAttachment)
	{
		GripAttachment->AttachToComponent(ItemMesh, FAttachmentTransformRules::KeepRelativeTransform, GripAttachment->SocketAttachment);
		GripAttachment->SetVisibility(ItemMesh->IsVisible());
		CurrentMagazineAdd += GripAttachment->MagazineAmmoIncrease;
	}
	
	if (IlluminatorAttachment)
	{
		IlluminatorAttachment->AttachToComponent(ItemMesh, FAttachmentTransformRules::KeepRelativeTransform, IlluminatorAttachment->SocketAttachment);
		IlluminatorAttachment->SetVisibility(ItemMesh->IsVisible());
		CurrentMagazineAdd += IlluminatorAttachment->MagazineAmmoIncrease;
	}

	if (AmmunitionAttachment)
	{
		AmmunitionAttachment->AttachToComponent(ItemMesh, FAttachmentTransformRules::KeepRelativeTransform, AmmunitionAttachment->SocketAttachment);
		AmmunitionAttachment->SetVisibility(ItemMesh->IsVisible());
		CurrentMagazineAdd += AmmunitionAttachment->MagazineAmmoIncrease;
	}
	
	if (CurrentMagazineAdd != PreviousMagazineAdd)
	{
		AddMagazineCountFromAttachments(CurrentMagazineAdd);
	}
}

// Disable Attachment Replication.
void ABaseWeapon::OnRep_AttachmentReplication()
{
  Super::OnRep_AttachmentReplication();
}

// alex: proc recoil system start

inline void RecoilVecInterpolate(FVector& actual, const FVector& goal, float speed, float frameTime)
{
	const FVector delta = goal - actual;
	actual += delta * FMath::Min(frameTime * speed, 1.0f);
}

inline void RecoilRotInterpolate(FRotator& actual, const FRotator& goal, float speed, float frameTime)
{
	const FRotator delta = goal - actual;
	actual += delta * FMath::Min(frameTime * speed, 1.0f);
}

void ABaseWeapon::ComputeProcRecoil(float DeltaTime)
{
	RecoilVecInterpolate(ProcRecoil_Trans, FVector(0.0f, 0.0f, 0.0f), RecoilDampStrength, DeltaTime);
	RecoilRotInterpolate(ProcRecoil_Rot, FRotator(0.0f, 0.0f, 0.0f), RecoilDampStrength, DeltaTime);

	RecoilVecInterpolate(ProcRecoil_Trans_Buildup, FVector(0.0f, 0.0f, 0.0f), RecoilBuildupDampStrength, DeltaTime);
	RecoilRotInterpolate(ProcRecoil_Rot_Buildup, FRotator(0.0f, 0.0f, 0.0f), RecoilBuildupDampStrength, DeltaTime);

	// we need this for ADS check and applying modifier values
	APlayerCharacter* pc = Cast<APlayerCharacter>(GetOwner());
	if (pc)
	{
		if (pc->bAiming)
		{
			ProcRecoil_PosADSModifier = RecoilFireADSModifier;
			ProcRecoil_RotADSModifier = RecoilAngleADSModifier;
			ProcRecoil_BuildupADSModifier = RecoilBuildupADSModifier;
		}
		else
		{
			ProcRecoil_PosADSModifier = 1.0f;
			ProcRecoil_RotADSModifier = 1.0f;
			ProcRecoil_BuildupADSModifier = 1.0;
		}
	}

	if (ProcRecoil_fireTime > 0.0f)
	{
		ProcRecoil_fireTime -= DeltaTime;

		if (ProcRecoil_fireTime < 0.0f)
			ProcRecoil_fireTime = 0.0f;

		//float strength = ProcRecoil_firstFire ? RecoilFireStrengthFirst : RecoilFireStrength;

		ProcRecoil_Trans += ( ( ProcRecoil_TransDir * RecoilFireStrength ) * ProcRecoil_PosADSModifier) * DeltaTime;
		ProcRecoil_Rot += ( ( ProcRecoil_RotDir * RecoilAngleStrength ) * ProcRecoil_RotADSModifier) * DeltaTime;

		if (RecoilHasBuildup)
		{
			ProcRecoil_Trans_Buildup += (RecoilPositionBuildup * ProcRecoil_BuildupADSModifier) * DeltaTime;
			ProcRecoil_Rot_Buildup += (RecoilRotationBuildup * ProcRecoil_BuildupADSModifier) * DeltaTime;
		}
	}
}

void ABaseWeapon::TriggerProcRecoil(const bool bIsFirstFire)
{
	ProcRecoil_fireTime = RecoilFireTime;

	ProcRecoil_TransDir = FVector(0.0f, -1.0f, 0.0f);
	ProcRecoil_TransDir.X = FMath::FRandRange(-1.0f, 1.0f) * RecoilRandomness;
	ProcRecoil_TransDir.Z = FMath::FRandRange(-1.0f, 1.0f) * RecoilRandomness;

	FVector randAng;
	randAng.X = ProcRecoil_TransDir.X * 0.7f;
	randAng.Y = FMath::FRandRange(-1.0f, 1.0f) * RecoilRandomness * 0.5f;
	randAng.Z = -ProcRecoil_TransDir.Z * 0.2f; // swapped
	
	// normalize
	ProcRecoil_TransDir = ProcRecoil_TransDir.GetSafeNormal();
	randAng = randAng.GetSafeNormal();

	// should work i hope
	ProcRecoil_RotDir = randAng.Rotation();

	ProcRecoil_firstFire = bIsFirstFire;
}

bool ABaseWeapon::PlayDraw(bool bDrawFirst)
{
	CurrentHighTimer = EquipHighTimer;
	return Super::PlayDraw(bDrawFirst);
}

EWeaponUnderbarrelAnimationType ABaseWeapon::GetUnderbarrelAnimationType() const
{
	UWeaponAttachment* Attachment = GetUnderbarrelAttachment();
	
	if (!Attachment)
	{
		return EWeaponUnderbarrelAnimationType::WU_None;
	}

	return Attachment->UnderbarrelAnimationType;
}

/*
void FRecoilSpring::Init(const FVector& InLocation, const float InMass, const float InSpringiness)
{
	OriginalLocation = InLocation;
	TempLocation = InLocation;
	RestLocation = InLocation;
	Mass = InMass;
	Springiness = InSpringiness;

	Reset();
}

void FRecoilSpring::AddForce(const FVector& ForceToApply)
{
	//Force += ForceToApply.GetSafeNormal();
Reset();
	//TempLocation += ForceToApply.GetSafeNormal();
}

void FRecoilSpring::Update(const float DeltaTime)
{
	//Force += GetRestoreForce() * GetDampeningForce() * DeltaTime;

	//Force = -2.0f * (TempLocation - RestLocation);
	//Acceleration = Force/50.0f;
	//Velocity = 0.95f * (Velocity + Acceleration);
	//TempLocation += Velocity * DeltaTime;

	SpringForce = -2.0f*(TempLocation - RestLocation);
	DampingForce = 30.0f * Velocity;
	Force = SpringForce + 8.0f * 9.8f - DampingForce;
	Acceleration = Force/8.0f;
	Velocity += Acceleration * 0.18f;
	TempLocation += Velocity * 0.18f;
	
	//Force.X = -Springiness * (TempLocation.X - RestLocation.X);
	//Acceleration.X = Force.X/Mass;
	//Velocity.X = Damp * Velocity.X + Acceleration.X;
	//TempLocation.X += Velocity.X * DeltaTime;
	//
	//Force.Y = -Springiness * (TempLocation.Y - RestLocation.Y);
	//Acceleration.Y = Force.Y/Mass;
	//Velocity.Y = Damp * Velocity.Y + Acceleration.Y;
	//TempLocation.Y += Velocity.Y * DeltaTime;
}

void FRecoilSpring::Reset()
{
	Force = FVector::ZeroVector;
	Velocity = FVector(0.0f, 0.0f, -1000.0f);
	Acceleration = FVector::ZeroVector;
		
	TempLocation = OriginalLocation;
	RestLocation = OriginalLocation;
}
*/
