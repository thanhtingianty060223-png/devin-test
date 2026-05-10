// Copyright Void Interactive, 2023

#include "Shotgun.h"
#include "ReadyOrNotDebugSubsystem.h"

void AShotgun::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AShotgun, Shells);
}

AShotgun::AShotgun()
{
	bHasVisibleMags = false;
}

void AShotgun::Tick(const float DeltaSeconds)
{
	ShellMesh = CurrentShellMesh;
	Mag_01_Static = ShellMesh;
	
	Super::Tick(DeltaSeconds);
}

void AShotgun::BeginPlay()
{
	Super::BeginPlay();

	// Setup default mags/tube for shotgun
	int32 DefaultSlots = FMath::Max(MaxShells / ShellsPerSlot, 0);
	SetMagazineCount(DefaultSlots, TArray<FName>());
	UpdateShellMesh();

	// Only need to worry about this for shell rack shotguns.
	//if (!bUseShellRack)
	//{
	//	return;
	//}

	//// Add shell components to the list so we can manipulate them later
	//TArray<UShellRackShellComponent*> ActorComponents;
	//GetComponents(ActorComponents);
	//ShellMeshComponents.Empty();

	//for (int32 i = 0; i < ActorComponents.Num(); i++)
	//{
	//	UShellRackShellComponent* ShellComponent = ActorComponents[i];
	//	ShellMeshComponents.Insert(ShellComponent, ShellComponent->ShellNumber);
	//	ShellComponent->SetVisibility(false, true);
	//}
}

void AShotgun::Server_NextMagazine_Implementation()
{
	ACyberneticCharacter* ai = Cast<ACyberneticCharacter>(GetOwner());
	if (ai && ai->IsOnSWATTeam())
	{
		uint16 AmmoTypeIndex = 0;
		if (Magazines.IsValidIndex(MagIndex))
		{
			AmmoTypeIndex = Magazines[MagIndex].AmmoType;
		}

		Shells.Init(AmmoTypeIndex, MaxShellsInWeapon);

		return;
	}

	// Already full?
	if (Shells.Num() >= MaxShellsInWeapon)
		return;
	
	FindNextMagIndex();

	// Use the next mag index if we're using an invalid magazine or it doesn't match our player's desired type
	// Otherwise reload from the current magazine (ammo pool)
	if (!Magazines.IsValidIndex(MagIndex) || Magazines[MagIndex].AmmoType != DesiredAmmoType)
	{
		MagIndex = NextMagIndex;
		UpdateShellMesh();
	}
	
	if (!Magazines.IsValidIndex(MagIndex))
	{
		return;
	}

	if (Magazines[MagIndex].Ammo <= 0)
	{
		Magazines.RemoveAt(MagIndex);
		return;
	}

	Magazines[MagIndex].Ammo--;

	// The top of the shell array represents the shell in the chamber
	if (Shells.Num() == 0)
	{
		// If starting with an empty shell array, add directly to the top of the shells array and set ammo type
		Shells.Add(Magazines[MagIndex].AmmoType);
		SetAmmunitionType(Magazines[MagIndex].AmmoType);
	}
	else
	{
		// Otherwise load directly beneath the top of the shells array
		Shells.Insert(Magazines[MagIndex].AmmoType, Shells.Num() - 1);
	}
	
	// Remove mags after we loaded them into Shells
	Magazines.RemoveAll([](const FMagazine& Magazine)
	{
		return Magazine.Ammo <= 0;
	});
}

void AShotgun::LocallySimulateFire(FRotator Direction, FVector SpawnLoc, int32 Seed)
{
	Super::LocallySimulateFire(Direction, SpawnLoc, Seed);

	if (GetLocalRole() < ROLE_Authority)
	{
		// Locally remove some ammo, once shells get replicated the ammo type is set properly in case of desync
		RemoveAmmo(1.0f);
	}
}

bool AShotgun::HasAnyAmmo() const
{
	for (const FMagazine& Magazine : Magazines)
	{
		if (Magazine.Ammo > 0)
			return true;
	}
	return false;
}

float AShotgun::GetAmmo() const
{
	return Shells.Num();
}

float AShotgun::RemoveAmmo(float Value)
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
	
	// Some logic will try to remove negative shells or more than one
	// Ignore this for shotguns
	if (Value != 1.0f)
		return Shells.Num();

	if (Shells.Num() > 0)
	{
		Shells.Pop(false);
	}

	if (Shells.Num() > 0)
	{
		int32 NewAmmoType = Shells.Top();
		SetAmmunitionType(NewAmmoType);
	}

	return Shells.Num();
}

void AShotgun::ReplenishAmmo()
{
	for (int32 i = 0; i < Magazines.Num(); i++)
	{
		Magazines[i].Ammo = OriginalShellCounts.IsValidIndex(i) ? OriginalShellCounts[i] : 0;
	}

	Shells.Empty();
	LoadShellsFromMagazine();
}

bool AShotgun::CanReload()
{
	// AI can always reload
	if (Cast<ACyberneticCharacter>(GetOwnerCharacter()))
	{
		return true;
	}
	
	if (Shells.Num() < MaxShellsInWeapon && HasAnyAmmo())
	{
		return true;
	}
	//else if (bUseShellRack && ShellsInRack < MaxShellsInRack && TotalShells > 0)
	//{
	//	return true;
	//}

	return false;
}

bool AShotgun::IsBlockingAnimationPlaying(TArray<EBlockingAnimationExclusion> Exclusions) const
{
	APlayerCharacter* pc = Cast<APlayerCharacter>(GetOwner());
	if (!pc)
		return false;

	if (!AnimationData)
		return false;

	if (bReloading)
		return true;

	if (bBlockingFireAnimation)
	{
		if (pc->Is1PMontagePlaying(AnimationData->FireLoop.Body_FP) || pc->Is1PMontagePlaying(AnimationData->FireLoopEnd.Body_FP) ||
			pc->Is1PMontagePlaying(AnimationData->FireLoopSights.Body_FP) || pc->Is1PMontagePlaying(AnimationData->FireLoopSightsEnd.Body_FP) ||
			pc->Is1PMontagePlaying(AnimationData->FireSingleLast.Body_FP) || pc->Is1PMontagePlaying(AnimationData->FireSingleSightsLast.Body_FP) || 
			pc->Is1PMontagePlaying(AnimationData->Grip_VFG_Body_FP_Fire_Last) || pc->Is1PMontagePlaying(AnimationData->Grip_VFG_Body_FP_Fire_Aim_Last)) // added grip overrides
		{
			return true;
		}


		// updated for grip override
		if ((GetUnderbarrelAnimationType() != EWeaponUnderbarrelAnimationType::WU_None) && AnimationData->bOverrideFireAnimForGrip)
		{
			// to be safe
			if (pc->Is1PMontagePlaying(AnimationData->Grip_VFG_Body_FP_Fire) || 
				pc->Is1PMontagePlaying(AnimationData->Grip_AFG_Body_FP_Fire) || 
				pc->Is1PMontagePlaying(AnimationData->Grip_AFG_Body_FP_Fire_Aim) || 
				pc->Is1PMontagePlaying(AnimationData->Grip_VFG_Body_FP_Fire_Aim))
			{
				return true;
			}
		}
		else
		{
			// added sight path, was missing?
			if (pc->bAiming)
			{
				for (int32 i = 0; i < AnimationData->FireSingleSights.Num(); i++)
				{
					if (pc->Is1PMontagePlaying(AnimationData->FireSingleSights[i].Body_FP))
					{
						return true;
					}
				}
			}
			else
			{
				for (int32 i = 0; i < AnimationData->FireSingle.Num(); i++)
				{
					if (pc->Is1PMontagePlaying(AnimationData->FireSingle[i].Body_FP))
					{
						return true;
					}
				}
			}
		}
	}

	return Super::IsBlockingAnimationPlaying();
}

void AShotgun::OnWeaponReload(bool bForce)
{
	OnWeaponTacticalReload();
}

void AShotgun::CheckReloadSettings()
{
	if (!bReloading)
	{
		// Load the settings, put them into bTapReload
		EShotgunReloadType ReloadType;
		UBpGameplayHelperLib::LoadShotgunSettings(ReloadType);

		bTapReload = ReloadType == EShotgunReloadType::SRT_SingleLoad;
	}
}

void AShotgun::OnWeaponTacticalReload()
{
	CheckReloadSettings();

	// Immediately switch our mag index since shotguns reload from magazines (ammo pools) into the gun directly
	if (Magazines.IsValidIndex(MagIndex) && Magazines[MagIndex].AmmoType != DesiredAmmoType)
	{
		FindNextMagIndex();
		MagIndex = NextMagIndex;
		
		UpdateShellMesh();
	}

	APlayerCharacter* pc = Cast<APlayerCharacter>(GetOwner());
	if (!pc)
		return;

	if (IsBlockingAnimationPlaying())
		return;

	if (!CanReload())
		return;
	
	if (!AnimationData)
	{
		return;	// don't bother with this, the animations literally drive everything
	}

	//APlayerController* controller = Cast<APlayerController>(pc->GetController());
	if (GetAmmo() <= 0 && HasAnyAmmo())
	{	// Case 1: no ammo in gun, and we are reloading from empty.
		//if (bUseShellRack && ShellsInRack > 0)
		//{	// Use shells from rack instead of slow reload
		//	if (AnimationData->ShellRack_ReloadEmpty.Num() >= ShellsInRack)
		//	{
		//		pc->Play1PMontage(AnimationData->ShellRack_ReloadEmpty[ShellsInRack-1].Body_FP);
		//		PlayFPMontage(AnimationData->ShellRack_ReloadEmpty[ShellsInRack-1].Gun_FP);

		//		pc->Play3PMontage(AnimationData->ShellRack_ReloadEmpty[ShellsInRack - 1].Body_TP);
		//		PlayTPMontage(AnimationData->ShellRack_ReloadEmpty[ShellsInRack - 1].Gun_TP);

		//		bTacticalReload = true;
		//		bReloading = true;
		//	}
		//}
		//if (bUseShellRack)
		//{
		//	pc->Play1PMontage(AnimationData->Tactical_ReloadEmpty.Body_FP);
		//	PlayFPMontage(AnimationData->Tactical_ReloadEmpty.Gun_FP);

		//	pc->Play3PMontage(AnimationData->Tactical_ReloadEmpty.Body_TP);
		//	PlayTPMontage(AnimationData->Tactical_ReloadEmpty.Gun_TP);

		//	Server_NextMagazine();
		//	bTacticalReload = true;
		//	bReloading = true;
		//}
		{
			pc->Play1PMontage(AnimationData->Reload_Start_Empty.Body_FP);
			PlayFPMontage(AnimationData->Reload_Start_Empty.Gun_FP);

			pc->Play3PMontage(AnimationData->Reload_Start_Empty.Body_TP);
			PlayTPMontage(AnimationData->Reload_Start_Empty.Gun_TP);

			Server_NextMagazine();
			bTacticalReload = true;
			bReloading = true;
		}
	}
	else if(GetAmmo() < MaxShellsInWeapon)
	{	// Case 2: some ammo in gun (not reloading from empty)
		//if (bUseShellRack && ShellsInRack > 0)
		//{	// Use shells from rack instead of slow reload
		//	if (AnimationData->ShellRack_Reload.Num() >= ShellsInRack)
		//	{
		//		pc->Play1PMontage(AnimationData->ShellRack_Reload[ShellsInRack-1].Body_FP);
		//		PlayFPMontage(AnimationData->ShellRack_Reload[ShellsInRack-1].Gun_FP);

		//		pc->Play3PMontage(AnimationData->ShellRack_Reload[ShellsInRack - 1].Body_TP);
		//		PlayTPMontage(AnimationData->ShellRack_Reload[ShellsInRack - 1].Gun_TP);

		//		bTacticalReload = true;
		//		bReloading = true;
		//	}
		//}
		//if (bUseShellRack)
		//{
		//	pc->Play1PMontage(AnimationData->Tactical_Reload.Body_FP);
		//	PlayFPMontage(AnimationData->Tactical_Reload.Gun_FP);

		//	pc->Play3PMontage(AnimationData->Tactical_Reload.Body_TP);
		//	PlayTPMontage(AnimationData->Tactical_Reload.Gun_TP);

		//	bTacticalReload = true;
		//	bReloading = true;
		//}
		{
			pc->Play1PMontage(AnimationData->Reload_Start.Body_FP);
			PlayFPMontage(AnimationData->Reload_Start.Gun_FP);

			pc->Play3PMontage(AnimationData->Reload_Start.Body_TP);
			PlayTPMontage(AnimationData->Reload_Start.Gun_TP);

			bTacticalReload = true;
			bReloading = true;
		}
	}
	//else if (bUseShellRack && bReloadRack && ShellsInRack < MaxShellsInRack)
	//{	// Case 3: the gun is loaded but the shell rack isn't
	//	if (bVelcroRack)
	//	{	// If it's a velcro rack, just play the tactical reload animation
	//		pc->Play1PMontage(AnimationData->Tactical_Reload.Body_FP);
	//		PlayFPMontage(AnimationData->Tactical_Reload.Gun_FP);

	//		pc->Play3PMontage(AnimationData->Tactical_Reload.Body_TP);
	//		PlayTPMontage(AnimationData->Tactical_Reload.Gun_TP);

	//		bTacticalReload = true;
	//		bReloading = true;
	//	}
	//	else if (AnimationData->ShellRack_ReloadRack.Num() >= ShellsInRack)
	//	{
	//		pc->Play1PMontage(AnimationData->ShellRack_ReloadRack[ShellsInRack-1].Body_FP);
	//		PlayFPMontage(AnimationData->ShellRack_ReloadRack[ShellsInRack-1].Gun_FP);

	//		pc->Play3PMontage(AnimationData->ShellRack_ReloadRack[ShellsInRack - 1].Body_TP);
	//		PlayTPMontage(AnimationData->ShellRack_ReloadRack[ShellsInRack - 1].Gun_TP);

	//		bTacticalReload = true;
	//		bReloading = true;
	//	}
	//}
}

void AShotgun::PlayReloadLoop()
{
	APlayerCharacter* pc = Cast<APlayerCharacter>(GetOwner());
	if (!pc)
		return;

	int32 MagazineShells = 0;
	for (const FMagazine& Magazine : Magazines)
	{
		MagazineShells += Magazine.Ammo;
	}
	
	if (AnimationData)
	{
		// Shotgun Reload_End montages load a shell, so play the end if we have 1 shell left int total
		if (Shells.Num() == (MaxShellsInWeapon - 1) || bCancelReloading || MagazineShells <= 1 || bTapReload)
		{
			//if (bReloading)
			{
				bReloading = false;
				pc->Play1PMontage(AnimationData->Reload_End.Body_FP);
				PlayFPMontage(AnimationData->Reload_End.Gun_FP);

				pc->Play3PMontage(AnimationData->Reload_End.Body_TP);
				PlayTPMontage(AnimationData->Reload_End.Gun_TP);

			}
		}
		else
		{
			Server_NextMagazine();
			pc->Play1PMontage(AnimationData->Reload_Loop.Body_FP);
			PlayFPMontage(AnimationData->Reload_Loop.Gun_FP);

			pc->Play3PMontage(AnimationData->Reload_Loop.Body_TP);
			PlayTPMontage(AnimationData->Reload_Loop.Gun_TP);
		}
	}
}

void AShotgun::OnRep_AttachmentRep()
{
	if (bNoAttachmentRep)
		return;

	Super::OnRep_AttachmentRep();

	//if (!bReloading)
	//{
	//	for (int32 i = 0; i < ShellsInRack; i++)
	//	{
	//		if (ShellMeshComponents.IsValidIndex(i))
	//		{
	//			ShellMeshComponents[i]->SetIsReplicated(false);
	//			ShellMeshComponents[i]->AttachToComponent(ItemMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, ShellRackSockets[i]);
	//			ShellMeshComponents[i]->SetVisibility(IsEquipped(), true);
	//			ShellMeshComponents[i]->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	//		}
	//	}
	//}
}

float AShotgun::GetWeight()
{
	float Result = Super::GetWeight();

	for (const FMagazine& Magazine : Magazines)
	{
		Result += ShellWeight * Magazine.Ammo;
	}
	Result += (ShellWeight * Shells.Num());

	return Result;
}

void AShotgun::SetMagazineCount(int32 Count, TArray<FName> AmmoTypes)
{
	#if WITH_EDITOR
	ensureMsgf(Count >= 0, TEXT("Magazine count must be greater than or equal to zero"));
	#endif
	
	if (Count < 0)
		return;

	// For each unique ammo type we use, create a magazine
	Magazines.Empty();
	for (int32 i = 0; i < AmmunitionTypes.Num(); i++)
	{
		Magazines.Add(FMagazine(0, i));
	}

	// For each ammo slot, find the appropriate magazine and increase its ammo count
	FName DefaultAmmoType = AmmunitionTypes.Num() > 0 ? AmmunitionTypes[0] : FName();
	for (int32 i = 0; i < Count; i++)
	{
		FName Type = AmmoTypes.IsValidIndex(i) && !AmmoTypes[i].IsNone() ? AmmoTypes[i] : DefaultAmmoType;

		int32 AmmoTypeIndex = AmmunitionTypes.IndexOfByKey(Type);
		if (AmmoTypeIndex == INDEX_NONE)
			continue;

		for (FMagazine& Magazine : Magazines)
		{
			if (Magazine.AmmoType == AmmoTypeIndex)
			{
				Magazine.Ammo += ShellsPerSlot;
			}
		}
	}

	// Remove any empty magazines
	Magazines.RemoveAll([](const FMagazine& Magazine)
	{
		return Magazine.Ammo <= 0;
	});

	// Set our original magazine shell counts for UI / ammo replenishment
	OriginalShellCounts.Empty();
	for (const FMagazine& Magazine : Magazines)
	{
		OriginalShellCounts.Add(Magazine.Ammo);
	}

	// Loads shells from magazines into the tube and sets up the ammo type
	Shells.Empty();
	LoadShellsFromMagazine();

	// TODO(killo): needs reimpl
	// Special logic for the shell rack
	//if (bUseShellRack)
	//{
	//	if (TotalShells < MaxShellsInRack)
	//	{
	//		ShellsInRack = TotalShells;
	//		TotalShells = 0;
	//	}
	//	else
	//	{
	//		ShellsInRack = MaxShellsInRack;
	//		TotalShells -= ShellsInRack;
	//	}

	//	Multicast_UpdateMagazineCount(ShellsInRack, TotalShells);
	//	Multicast_UpdateMagazineCount_Implementation(ShellsInRack, TotalShells);
	//}
}

float AShotgun::GetMagazineAmmoPercentage(int32 MagazineIndex) const
{
	float CurrentShells = GetAmmoInMagazine(MagazineIndex);
	float MaximumShells = OriginalShellCounts.IsValidIndex(MagazineIndex) ? OriginalShellCounts[MagazineIndex] : 0.0f;

	// HACK: since we take shells from the first magazine to initially load the shotgun
	// If we modified the original shell counts array, rearming would break
	if (MagazineIndex == 0)
		MaximumShells -= MaxShellsInWeapon;

	return FMath::Clamp(CurrentShells / MaximumShells, 0.0f, 1.0f);
}

float AShotgun::GetCurrentAmmoPercentage() const
{
	float RemainingShells = Shells.Num();
	return FMath::Clamp(RemainingShells / MaxShellsInWeapon, 0.0f, 1.0f);
}

void AShotgun::LoadShellsFromMagazine()
{
	#if WITH_EDITOR
	ensureMsgf(Shells.Num() <= MaxShellsInWeapon, TEXT("Shells should not exceed max shells allowed in weapon"));
	#endif
	
	uint8 CurrentMagazine = 0;
	while (Shells.Num() < MaxShellsInWeapon)
	{
		if (!Magazines.IsValidIndex(CurrentMagazine))
			break;

		if (Magazines[CurrentMagazine].Ammo <= 0)
		{
			CurrentMagazine++;
			continue;
		}

		Shells.Add(Magazines[CurrentMagazine].AmmoType);
		Magazines[CurrentMagazine].Ammo--;
	}

	// Set the ammo type to match whats in the chamber
	if (Shells.Num() > 0)
	{
		SetAmmunitionType(Shells.Top());
	}

	// Remove mags after we loaded them into Shells
	Magazines.RemoveAll([](const FMagazine& Magazine)
	{
		return Magazine.Ammo <= 0;
	});
}

void AShotgun::UpdateShellMesh()
{
	bLastAttached = false;
	
	if (!AmmunitionTypes.IsValidIndex(MagIndex))
		return;
	
	if (!AmmoDataTable)
		return;
	
	FName AmmoTypeRowName = AmmunitionTypes[MagIndex];
	FAmmoTypeData* AmmoTypeData = AmmoDataTable->FindRow<FAmmoTypeData>(AmmoTypeRowName, "Ammo Type Lookup");
	if (!AmmoTypeData)
		return;

	// TODO(killo): handle with ammo type name to shell mesh map
	const FText& AmmoVariety = AmmoTypeData->AmmoVariety;
	if (AmmoVariety.IsEmpty())
	{
		CurrentShellMesh = nullptr;
	}
	else if (AmmoVariety.EqualToCaseIgnored(FText::FromString("Slug")))
	{
		CurrentShellMesh = SlugShell;
	}
	else if (AmmoVariety.EqualToCaseIgnored(FText::FromString("Buckshot")))
	{
		CurrentShellMesh = BuckShotShell;
	}
	else if (AmmoVariety.EqualToCaseIgnored(FText::FromString("Beanbag")))
	{
		CurrentShellMesh = BeanbagShell;
	}
	else if (AmmoVariety.EqualToCaseIgnored(FText::FromString("Breach")))
	{
		CurrentShellMesh = BreachShell;
	}

	ShellMesh = CurrentShellMesh;
	Mag_01_Static = ShellMesh;
}

void AShotgun::OnRep_ShellsReplicated()
{
	if (Shells.Num() > 0)
		SetAmmunitionType(Shells.Top());
}

float AShotgun::GetAmmoWeight(int32 Count)
{
	return ShellWeight * Count;
}

void AShotgun::AttachStatic()
{	
	Super::AttachStatic();
	
	if (IsCurrentlyReloading())
	{
		EnableWeaponFovShader();
	}
}

void AShotgun::DetachStatic()
{
	Super::DetachStatic();
}

void AShotgun::OnDrawComplete()
{
	Super::OnDrawComplete();
	EnableWeaponFovShader();
}

// TODO(killo): needs reimpl
#if 0
void AShotgun::AddMagazineCountFromAttachments(float AddAmount)
{
	// TODO(killo): needs reimpl
	float PreviousAmount = AddedMagazineCountFromAttachments;
	AddedMagazineCountFromAttachments = AddAmount;

	float Delta = AddAmount - PreviousAmount;
	if (AmmoMax + Delta < 0.0f)
	{
		Delta = -AmmoMax;
	}

	for (int32 i = 0; i < Magazines.Num(); i++)
	{
		Magazines[i].Ammo += Delta;
	}
	AmmoMax += Delta;
	ShellsInWeapon += Delta;
	MaxShellsInWeapon += Delta;
}

void AShotgun::Multicast_UpdateMagazineCount_Implementation(int32 NewShellRackCount, int32 NewTotalShellCount)
{
	if (bUseShellRack)
	{
		ShellsInRack = NewShellRackCount;
		for (int32 i = NewShellRackCount; i < MaxShellsInRack; i++)
		{
			ShellMeshComponents[i]->SetHiddenInGame(true, true);
		}
	}
}


void AShotgun::Server_ReloadShellInRack_Implementation()
{

	ShellsInRack--;
	ShellsInWeapon = (int32)FMath::Clamp(ShellsInWeapon + 1.0f, 0.0f, (float)MaxShellsInWeapon);
}

void AShotgun::FinishedLoadingShellFromRack()
{
	if (ShellMeshComponents.IsValidIndex(ShellsInRack - 1))
	{
		ShellMeshComponents[ShellsInRack - 1]->SetHiddenInGame(true, true);
	}

	if (GetLocalRole() < ROLE_Authority)
	{
		Server_ReloadShellInRack();
	}
	Server_ReloadShellInRack_Implementation();

	bReloading = false;
}

void AShotgun::LoadNextShellInRack()
{
	if (!bCancelReloading && !bTapReload && Shells.Num() != MaxShellsInWeapon)
	{
		OnWeaponTacticalReload();
	}
}

void AShotgun::Server_ReloadShellRack_Implementation()
{
	int32 NumShellsInRack;

	if (TotalShells + ShellsInRack <= MaxShellsInRack)
	{	// Removing all of the shells from reserve
		NumShellsInRack = TotalShells + ShellsInRack;
		TotalShells = 0;
	}
	else
	{	// Removing as many as we can
		TotalShells -= (MaxShellsInRack - ShellsInRack);
		NumShellsInRack = MaxShellsInRack;
	}

	ShellsInRack = NumShellsInRack;
}

void AShotgun::RefreshEntireShellRack()
{
	if (GetLocalRole() < ROLE_Authority)
	{
		Server_ReloadShellRack();
	}
	Server_ReloadShellRack_Implementation();


	for (int32 i = 0; i < ShellsInRack; i++)
	{
		if (ShellMeshComponents.IsValidIndex(i))
		{
			ShellMeshComponents[i]->AttachToComponent(ItemMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, ShellRackSockets[i]);
			ShellMeshComponents[i]->SetVisibility(true, true);
			ShellMeshComponents[i]->SetHiddenInGame(false, true);
			ShellMeshComponents[i]->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}

	bRefreshingShellRack = true;
}
#endif