// Copyright Void Interactive, 2023 

#include "HUD/Widgets/Loadout/V2/Loadout_V2.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Commander/BaseProfile.h"
#include "Components/InteractableComponent.h"
#include "GameModes/VIPEscortGS.h"
#include "HUD/Widgets/HumanCharacterHUD_V2.h"
#include "HUD/Widgets/PreMissionPlanning.h"
#include "lib/ReadyOrNotLoadoutManager.h"

// void ULoadout_V2::NativeOnActivated()
// {
// 	Super::NativeOnActivated();
// }

PRAGMA_DISABLE_OPTIMIZATION

void ULoadout_V2::NativePreConstruct()
{
	Super::NativePreConstruct();
}

void ULoadout_V2::NativeConstruct()
{
	Super::NativeConstruct();

	pc = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld());
	gs = GetWorld()->GetGameState<AReadyOrNotGameState>();
	ps = Cast<AReadyOrNotPlayerState>(pc->PlayerState);

	// gs->Loadout_V2 = this;
	LoadoutFunctionLibrary = gs->LoadoutFunctionLibrary;
	pc->HideHUD();

	if (!EquippingPlayerState)
	{
		EquippingPlayerState = ps;
	}
	LoadStoredWeaponAttachments();

	SetDefaultCamera();
	UpdateDefaultPreviewCharacter();
}

void ULoadout_V2::NativeDestruct()
{
	Super::NativeDestruct();
	ExitLoadout();
}

void ULoadout_V2::SetDefaultCamera(float BlendTime)
{
	for (TActorIterator<ACameraActor> It(GetWorld()); It; ++It)
	{
		ACameraActor* c = *It;
		if (c->Tags.Contains("CharacterViewAppearance"))
		{
			// AReadyOrNotPlayerController* pc = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld());
			FViewTargetTransitionParams TransitionParams;
			TransitionParams.BlendTime = BlendTime;
			TransitionParams.bLockOutgoing = true;
			pc->SetViewTarget(c, TransitionParams);
		}
	}
}

bool ULoadout_V2::IsInLobby()
{
	return GetWorld() && GetWorld()->GetMapName().Contains("Lobby_V2");
}

void ULoadout_V2::AttemptEquipLoadoutInGame()
{
	if (LOCAL_PLAYER)
	{
		if (AReadyOrNotPlayerState* PlayerState = LocalPlayer->GetPlayerState<AReadyOrNotPlayerState>())
		{
			// FSavedLoadout ToLoad;
			// UBpGameplayHelperLib::LoadLoadout(LoadoutFunctionLibrary->GetActiveLoadout(), "default");
			PlayerState->Server_SetLoadout(LoadoutFunctionLibrary->GetActiveLoadout());
			LocalPlayer->GetInventoryComponent()->Server_AttemptEquipNewLoadout(
				LoadoutFunctionLibrary->GetActiveLoadout());
		}
	}
}

void ULoadout_V2::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	PlayAnimationReverse(FadeOut);
}

bool ULoadout_V2::NativeOnHandleBackAction()
{
	const bool Handled = Super::NativeOnHandleBackAction();
	//ExitLoadout();
	return false;
}

// void ULoadout_V2::OpenItemList(EItemCategory LoadoutSlot, TArray<LoadoutCategory>* GearCategoryClasses)
// {
// 	
// }

// bool ULoadout_V2::NativeOnHandleBackAction()
// {
// 	// AReadyOrNotGameState* gs = Cast<AReadyOrNotGameState>(GetWorld()->GetGameState());
// 	gs->PreMissionStreamedLevel->SetShouldBeVisible(false); 
// 	return Super::NativeOnHandleBackAction();
// }

void ULoadout_V2::ApplyLoadoutPreset(FLoadoutPreset LoadoutPreset)
{
	// TODO
	// ActiveLoadout = LoadoutPreset.Loadout;
	// SetEquippingSwatMember(EEquippingSwat::ES_None, ps);
}

AReadyOrNotCharacter* ULoadout_V2::GetPreviewCharacter(FName Tag)
{
	for (TActorIterator<AReadyOrNotCharacter> It(GetWorld()); It; ++It)
	{
		AReadyOrNotCharacter* Character = *It;
		if (Character)
		{
			if (Character->Tags.Contains(Tag))
				return Character;
		}
	}
	return nullptr;
}

void ULoadout_V2::PlayAnimationOnPreviewCharacter(FString Animation)
{
	if (!GetPreviewCharacter("PreviewCharacter"))
		return;
	
	if (Animation == "tp_pregame_idle")
	{
		UpdateDefaultPreviewCharacter();
		
		UInventoryComponent* InventoryComponent = GetDefaultPreviewCharacter()->GetInventoryComponent();
		InventoryComponent->Holster(InventoryComponent->GetSpawnedGear().Primary, true);
		InventoryComponent->Holster(InventoryComponent->GetSpawnedGear().Secondary, true);
	}
	else if (Animation == "tp_pregame_idle_primary")
	{
		// SetPrimaryWeapon(wd);
		UpdatePreviewCharacterPrimary();
		EquipPrimary();
		// if (!bRifleDrawn)
		// {
		// 	HidePrimary(true);
		// 	GetWorld()->GetTimerManager().SetTimer(HideRifleHandle,
		// 	                                       FTimerDelegate::CreateUObject(
		// 		                                       this, &ULoadout_V2::HidePrimary, false), 0.2f, false);
		// 	bRifleDrawn = true;
		// }
	}
	else if (Animation == "tp_pregame_idle_secondary" || Animation == "tp_pregame_idle_secondary_after_weapon")
	{
		// if (bPistolDrawn && bRifleDrawn)
		// {
		// 	Animation = "tp_pregame_idle_secondary_already_drawn_after_weapon";
		// }

		//SetSecondaryWeapon(wd);
		UpdatePreviewCharacterSecondary();

		EquipSecondary();
		// if (!bPistolDrawn)
		// {
		// 	GetWorld()->GetTimerManager().SetTimer(HidePistolHandle,
		// 	                                       FTimerDelegate::CreateUObject(this, &ULoadout_V2::EquipSecondary),
		// 	                                       0.7f, false);
		// }
		// else
		// {
		// 	GetWorld()->GetTimerManager().ClearTimer(HidePistolHandle);
		// 	EquipSecondary();
		// }

		bPistolDrawn = true;
	}

	if (Animation == "tp_pregame_idle_secondary_drawn_only")
	{
		GetWorld()->GetTimerManager().SetTimer(HidePistolHandle,
		                                       FTimerDelegate::CreateUObject(
			                                       this, &ULoadout_V2::AttachSecondaryToSocket,
			                                       FName("pistol_holster_socket")), 0.4f, false);
	}

	Animation = GetPistolNonPistolVariation(Animation);

	GetPreviewCharacter("PreviewCharacter")->PlayMontageFromTable(Animation);
}

void ULoadout_V2::UpdatePreviewCharacterPrimary()
{
	if (!GetDefaultPreviewCharacter())
		return;

	if (pc)
	{
		if (ps)
		{
			ABaseItem* bi = GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Primary;
			if ((bi && bi->GetClass() != LoadoutFunctionLibrary->GetActiveLoadout().Primary))
			{
				bi->Destroy();
				bi = nullptr;
				UpdatePreviewCharacter(ps, "MyPlayerPreview");
			}

			if (!bi)
			{
				if (!LoadoutFunctionLibrary->GetActiveLoadout().Primary)
				{
					return;
				}
				bi = GetWorld()->SpawnActor<
					ABaseItem>(LoadoutFunctionLibrary->GetActiveLoadout().Primary, FTransform());
				bi->Tags.Add("NoRelevancy");
				bi->Tags.Add("CustomizationMenu");
				bi->bNoAttachmentRep = true;
				bi->bDisableAnimInstanceWhenNotEquipped = false;
				bi->SetReplicates(false);

				GetDefaultPreviewCharacter()->GetInventoryComponent()->AddInventoryItem(bi);
				GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Primary = bi;
				// bi->AttachToComponent(GetDefaultPreviewCharacter()->GetMesh(),
				//                       FAttachmentTransformRules::SnapToTargetIncludingScale, bi->HandsSocket);

				EquipPrimary();
			}

			DoPrimaryWeaponPreviewBlend();

			UpdatePreviewWeaponAttachments(false, LoadoutFunctionLibrary->GetActiveLoadout().PrimaryScope);
			UpdatePreviewWeaponAttachments(false, LoadoutFunctionLibrary->GetActiveLoadout().PrimaryMuzzle);
			UpdatePreviewWeaponAttachments(false, LoadoutFunctionLibrary->GetActiveLoadout().PrimaryUnderbarrel);
			UpdatePreviewWeaponAttachments(false, LoadoutFunctionLibrary->GetActiveLoadout().PrimaryOverbarrel);
			UpdatePreviewWeaponAttachments(false, LoadoutFunctionLibrary->GetActiveLoadout().PrimaryStock);
			UpdatePreviewWeaponAttachments(false, LoadoutFunctionLibrary->GetActiveLoadout().PrimaryGrip);
			UpdatePreviewWeaponAttachments(false, LoadoutFunctionLibrary->GetActiveLoadout().PrimaryIlluminator);
			UpdatePreviewWeaponAttachments(false, LoadoutFunctionLibrary->GetActiveLoadout().PrimaryAmmunition);

			if (LoadoutFunctionLibrary->GetActiveLoadout().PrimarySkin)
			{
				USkinComponent* SkinComp = NewObject<USkinComponent>(
					bi, LoadoutFunctionLibrary->GetActiveLoadout().PrimarySkin);
				if (SkinComp)
				{
					SkinComp->RegisterComponent();
				}
			}
		}
	}
}

void ULoadout_V2::UpdatePreviewCharacterSecondary()
{
	if (!GetDefaultPreviewCharacter())
		return;
	if (pc)
	{
		if (ps)
		{
			ABaseItem* bi = GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Secondary;
			if ((bi && bi->GetClass() != LoadoutFunctionLibrary->GetActiveLoadout().Secondary))
			{
				bi->Destroy();
				bi = nullptr;
			}

			if (!bi)
			{
				if (!LoadoutFunctionLibrary->GetActiveLoadout().Secondary)
				{
					return;
				}
				bi = GetWorld()->SpawnActor<ABaseItem>(LoadoutFunctionLibrary->GetActiveLoadout().Secondary);
				bi->Tags.Add("NoRelevancy");
				bi->Tags.Add("CustomizationMenu");
				bi->bNoAttachmentRep = true;
				bi->SetReplicates(false);
				GetDefaultPreviewCharacter()->GetInventoryComponent()->AddInventoryItem(bi);
				GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Secondary = bi;
				// bi->AttachToComponent(GetDefaultPreviewCharacter()->GetMesh(),
				//                       FAttachmentTransformRules::SnapToTargetIncludingScale, bi->HandsSocket);
			}

			if (!bPistolDrawn && !ps->IsVipPlayerState())
			{
				AttachSecondaryToSocket("Pistol_Holster_Socket");
			}
			else
			{
				EquipSecondary();
			}


			ABaseWeapon* bw = Cast<ABaseWeapon>(
				GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Secondary);
			if (bw)
			{
				LoadAddAttachment(bw, LoadoutFunctionLibrary->GetActiveLoadout().SecondaryScope, false);
				LoadAddAttachment(bw, LoadoutFunctionLibrary->GetActiveLoadout().SecondaryMuzzle, false);
				LoadAddAttachment(bw, LoadoutFunctionLibrary->GetActiveLoadout().SecondaryUnderbarrel, false);
				LoadAddAttachment(bw, LoadoutFunctionLibrary->GetActiveLoadout().SecondaryOverbarrel, false);
				LoadAddAttachment(bw, LoadoutFunctionLibrary->GetActiveLoadout().SecondaryStock, false);
				LoadAddAttachment(bw, LoadoutFunctionLibrary->GetActiveLoadout().SecondaryGrip, false);
				LoadAddAttachment(bw, LoadoutFunctionLibrary->GetActiveLoadout().SecondaryIlluminator, false);
				LoadAddAttachment(bw, LoadoutFunctionLibrary->GetActiveLoadout().SecondaryAmmunition, false);
			}

			if (LoadoutFunctionLibrary->GetActiveLoadout().SecondarySkin)
			{
				USkinComponent* SkinComp = NewObject<USkinComponent>(
					bi, LoadoutFunctionLibrary->GetActiveLoadout().SecondarySkin);
				if (SkinComp)
				{
					SkinComp->RegisterComponent();
				}
			}
		}
	}
}

void ULoadout_V2::HidePrimaryAndSecondary()
{
	FSavedLoadout Temp = LoadoutFunctionLibrary->GetActiveLoadout();
	Temp.Primary = nullptr;
	Temp.Secondary = nullptr;

	if (GetDefaultPreviewCharacter()->GetInventoryComponent()->GetLastEquippedLoadout() != Temp)
	{
		FLoadoutEquipOptions LoadoutEquipOptions;
		LoadoutEquipOptions.bReplicates = false;
		LoadoutEquipOptions.EquipItemCategory = EItemCategory::IC_None;
		LoadoutEquipOptions.bSanitizeLoadout = false;
		UBpGameplayHelperLib::EquipLoadoutOnPlayer(Temp, GetDefaultPreviewCharacter(), LoadoutEquipOptions);
		for (ABaseItem* bi : GetDefaultPreviewCharacter()->GetInventoryComponent()->GetInventoryItems())
		{
			if (bi)
			{
				DESTROY_COMPONENT(bi->InteractableComponent);

				bi->Tags.Add("CustomizationMenu");
			}
		}
	}
}

void ULoadout_V2::UpdateDefaultPreviewCharacter()
{
	UpdatePreviewCharacter(ps, "PreviewCharacter");
}

void ULoadout_V2::UpdatePreviewCharacter(class AReadyOrNotPlayerState* InPreviewPlayerState, FName Tag)
{
	// TODO
	// if (bIsWeaponCustomization)
	// 	return; 

	if (!InPreviewPlayerState)
		return;

	if (!GetPreviewCharacter(Tag))
		return;

	GetPreviewCharacter(Tag)->GetInventoryComponent()->SetIsReplicated(false);
	GetPreviewCharacter(Tag)->SetActorHiddenInGame(false);

	AVIPEscortGS* vipgs = Cast<AVIPEscortGS>(GetWorld()->GetGameState());
	if (vipgs)
	{
		if (InPreviewPlayerState == vipgs->VIPPlayerState)
		{
			GetPreviewCharacter(Tag)->GetMeshGearSlot()->SetSkeletalMesh(nullptr);
		}
	}

	UpdateTeamVisuals(InPreviewPlayerState, Tag);
	if (Tag == "PreviewCharacter")
	{
		OurSpawnedTeamType = InPreviewPlayerState->GetTeam();
		PreviewPlayerState = InPreviewPlayerState;
		// UBpGameplayHelperLib::SanitizeLoadout(ActiveLoadout);

		if (!InPreviewPlayerState->IsVipPlayerState())
		{
			FSavedLoadout Temp = LoadoutFunctionLibrary->GetActiveLoadout();
			//Temp.Primary = nullptr;
			//Temp.Secondary = nullptr;

			if (GetPreviewCharacter(Tag)->GetInventoryComponent()->GetLastEquippedLoadout() != Temp)
			{
				FLoadoutEquipOptions LoadoutEquipOptions;
				LoadoutEquipOptions.bReplicates = false;
				LoadoutEquipOptions.OverridePlayerState = InPreviewPlayerState;
				LoadoutEquipOptions.EquipItemCategory = EItemCategory::IC_None;
				LoadoutEquipOptions.bSanitizeLoadout = false;
				UBpGameplayHelperLib::EquipLoadoutOnPlayer(Temp, GetPreviewCharacter(Tag), LoadoutEquipOptions);
				for (ABaseItem* bi : GetPreviewCharacter(Tag)->GetInventoryComponent()->GetInventoryItems())
				{
					if (bi)
					{
						DESTROY_COMPONENT(bi->InteractableComponent);

						bi->Tags.Add("CustomizationMenu");
					}
				}

				// UpdatePreviewCharacterPrimary();
				// UpdatePreviewCharacterSecondary();
			}
		}
		// GetPreviewCharacter(Tag)->PlayMontageFromTable("tp_pregame_idle");
	}
	else
	{
		FSavedLoadout LastLoadout = GetPreviewCharacter(Tag)->GetInventoryComponent()->GetLastEquippedLoadout();
		FSavedLoadout NewLoadout = InPreviewPlayerState->GetLoadout();
		PlayerStatePreviewMap.Add(GetPreviewCharacter(Tag), InPreviewPlayerState);
		FSavedLoadout Loadout = InPreviewPlayerState->GetLoadout();
		bool bUpdateLoadout = false;
		if (GetPreviewCharacter(Tag)->GetInventoryComponent()->GetLastEquippedLoadout() != InPreviewPlayerState->
			GetLoadout())
		{
			bUpdateLoadout = true;
		}

		if (bUpdateLoadout)
		{
			FLoadoutEquipOptions LoadoutEquipOptions;
			LoadoutEquipOptions.bReplicates = false;
			LoadoutEquipOptions.EquipItemCategory = EItemCategory::IC_Primary;
			LoadoutEquipOptions.OverridePlayerState = InPreviewPlayerState;
			//UBpGameplayHelperLib::EquipLoadoutOnPlayer(Loadout, GetPreviewCharacter(Tag), LoadoutEquipOptions);
		}
	}
}

void ULoadout_V2::AttachSecondaryToSocket(FName Socket)
{
	return;
	
	if (!GetDefaultPreviewCharacter())
		return;

	if (Socket == "pistol_holster_socket")
	{
		bPistolDrawn = false;
	}


	if (GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Secondary)
	{
		GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Secondary->bNoAttachmentRep = true;
		GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Secondary->GetItemMesh()->
		                              AttachToComponent(GetDefaultPreviewCharacter()->GetMesh(),
		                                                FAttachmentTransformRules::SnapToTargetIncludingScale, Socket);
	}
}

void ULoadout_V2::EquipPrimary()
{
	if (!GetDefaultPreviewCharacter())
		return;
	
	ABaseItem* bi = Cast<ABaseItem>(GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Primary);
	if (bi)
	{
		// FName Socket = bi->HandsSocket;
		// bi->GetItemMesh()->AttachToComponent(GetDefaultPreviewCharacter()->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, Socket);
		// bi->GetItemMesh()->SetVisibility(true);

		GetDefaultPreviewCharacter()->GetInventoryComponent()->PutItemInHands(bi, true, true);
	}

	GetPreviewCharacter("PreviewCharacter")->PlayMontageFromTable("tp_pregame_idle_primary");
}

void ULoadout_V2::ExitLoadout()
{
	if (!IsExiting)
	{
		IsExiting = true;

		if (LoadoutFunctionLibrary)
		{
			LoadoutFunctionLibrary->DoSaveActiveLoadout();
			SaveStoredWeaponAttachments();
			LoadoutFunctionLibrary->SetActiveLoadoutByName("default");
			if (ps)
				ps->LastLoadout = LoadoutFunctionLibrary->GetActiveLoadout();
			AttemptEquipLoadoutInGame();
		}

		if (gs)
		{
			gs->Loadout_V2 = nullptr;
			gs->LoadoutFunctionLibrary = nullptr;
		}

		if (pc && pc->PlayerCameraManager)
		{
			pc->SetViewTarget(pc->GetPawn());
			pc->PlayerCameraManager->SetManualCameraFade(1.0f, FLinearColor::Black, false);

			pc->SetShouldShowMouseCursor(false);
		}

		if (gs && gs->PreMissionStreamedLevel)
		{
			gs->PreMissionStreamedLevel->SetShouldBeVisible(false);
		}

		PlayAnimationForward(FadeOut);
	}
}

void ULoadout_V2::UpdateTeamVisuals(class AReadyOrNotPlayerState* InPreviewPlayerState,
                                    FName Tag /*= "PreviewCharacter"*/)
{
	AReadyOrNotCharacter* PreviewCharacter = GetPreviewCharacter(Tag);
	if (!PreviewCharacter)
		return;

	if (gs->bPvPMode)
	{
		AReadyOrNotCharacter* LocalPlayer = Cast<AReadyOrNotCharacter>( UReadyOrNotStatics::GetReadyOrNotPlayerController()->GetPawn());
		if (LocalPlayer)
		{
			//GEngine->AddOnScreenDebugMessage(7781, 30.0f, FColor::White, "SkeletalMesh: " + MeshName);
			PreviewCharacter->GetMesh()->SetSkeletalMesh(LocalPlayer->GetMesh()->SkeletalMesh, false);
			PreviewCharacter->GetFaceMesh()->SetSkeletalMesh(LocalPlayer->GetFaceMesh()->SkeletalMesh, false);
			PreviewCharacter->GetMesh()->EmptyOverrideMaterials();
			PreviewCharacter->SetTPMeshOverrideMap(LocalPlayer->GetTPMeshOverrideMap());
		}
		
	}
}

void ULoadout_V2::DoPrimaryWeaponPreviewBlend()
{
	if (!GetDefaultPreviewCharacter())
		return;

	ABaseItem* BaseItem = GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Primary;
	ABaseWeapon* BaseWeapon = Cast<ABaseWeapon>(BaseItem);
	bool bPistolGrip = BaseWeapon ? BaseWeapon->bPistolGrip : false;
	
	// swap  animation if already playing (ie just swappign gun).. match current time`
	if (GetDefaultPreviewCharacter()->IsTableMontagePlaying("tp_pregame_idle_primary_nonpistol") && bPistolGrip)
	{
		float CurrentMontageTime = GetDefaultPreviewCharacter()->GetMesh()->GetAnimInstance()->
		                                                         Montage_GetPosition(nullptr);
		UAnimMontage* NewMontage = GetDefaultPreviewCharacter()->GetMontageFromTable(
			"tp_pregame_idle_primary_pistol");
		FAlphaBlend OldBlendIn = NewMontage->BlendIn;
		NewMontage->BlendIn = 0.0f;
		GetDefaultPreviewCharacter()->GetMesh()->GetAnimInstance()->Montage_Play(
			NewMontage, 1.0f, EMontagePlayReturnType::MontageLength, CurrentMontageTime);
		NewMontage->BlendIn = OldBlendIn;
	}
	else if (GetDefaultPreviewCharacter()->IsTableMontagePlaying("tp_pregame_idle_primary_pistol") && !bPistolGrip)
	{
		float CurrentMontageTime = GetDefaultPreviewCharacter()->GetMesh()->GetAnimInstance()->
		                                                         Montage_GetPosition(nullptr);
		UAnimMontage* NewMontage = GetDefaultPreviewCharacter()->GetMontageFromTable(
			"tp_pregame_idle_primary_nonpistol");
		FAlphaBlend OldBlendIn = NewMontage->BlendIn;
		NewMontage->BlendIn = 0.0f;
		GetDefaultPreviewCharacter()->GetMesh()->GetAnimInstance()->Montage_Play(
			NewMontage, 1.0f, EMontagePlayReturnType::MontageLength, CurrentMontageTime);
		NewMontage->BlendIn = OldBlendIn;
	}
	else if (GetDefaultPreviewCharacter()->IsTableMontagePlaying("tp_pregame_idle_helmet_after_weapon_nonpistol") && bPistolGrip)
	{
		float CurrentMontageTime = GetDefaultPreviewCharacter()->GetMesh()->GetAnimInstance()->
		                                                         Montage_GetPosition(nullptr);
		UAnimMontage* NewMontage = GetDefaultPreviewCharacter()->GetMontageFromTable(
			"tp_pregame_idle_helmet_after_weapon_pistol");
		FAlphaBlend OldBlendIn = NewMontage->BlendIn;
		NewMontage->BlendIn = 0.0f;
		GetDefaultPreviewCharacter()->GetMesh()->GetAnimInstance()->Montage_Play(
			NewMontage, 1.0f, EMontagePlayReturnType::MontageLength, CurrentMontageTime);
		NewMontage->BlendIn = OldBlendIn;
	}
	else if (GetDefaultPreviewCharacter()->IsTableMontagePlaying("tp_pregame_idle_helmet_after_weapon_pistol") && !bPistolGrip)
	{
		float CurrentMontageTime = GetDefaultPreviewCharacter()->GetMesh()->GetAnimInstance()->
		                                                         Montage_GetPosition(nullptr);
		UAnimMontage* NewMontage = GetDefaultPreviewCharacter()->GetMontageFromTable(
			"tp_pregame_idle_helmet_after_weapon_nonpistol");
		FAlphaBlend OldBlendIn = NewMontage->BlendIn;
		NewMontage->BlendIn = 0.0f;
		GetDefaultPreviewCharacter()->GetMesh()->GetAnimInstance()->Montage_Play(
			NewMontage, 1.0f, EMontagePlayReturnType::MontageLength, CurrentMontageTime);
		NewMontage->BlendIn = OldBlendIn;
	}
	else if (GetDefaultPreviewCharacter()->IsTableMontagePlaying("tp_pregame_after_weapon_nonpistol") && bPistolGrip)
	{
		float CurrentMontageTime = GetDefaultPreviewCharacter()->GetMesh()->GetAnimInstance()->
		                                                         Montage_GetPosition(nullptr);
		UAnimMontage* NewMontage = GetDefaultPreviewCharacter()->GetMontageFromTable(
			"tp_pregame_idle_after_weapon_pistol");
		FAlphaBlend OldBlendIn = NewMontage->BlendIn;
		NewMontage->BlendIn = 0.0f;
		GetDefaultPreviewCharacter()->GetMesh()->GetAnimInstance()->Montage_Play(
			NewMontage, 1.0f, EMontagePlayReturnType::MontageLength, CurrentMontageTime);
		NewMontage->BlendIn = OldBlendIn;
	}
	else if (GetDefaultPreviewCharacter()->IsTableMontagePlaying("ttp_pregame_idle_after_weapon_pistol") && !bPistolGrip)
	{
		float CurrentMontageTime = GetDefaultPreviewCharacter()->GetMesh()->GetAnimInstance()->
		                                                         Montage_GetPosition(nullptr);
		UAnimMontage* NewMontage = GetDefaultPreviewCharacter()->GetMontageFromTable(
			"tp_pregame_idle_after_weapon_nonpistol");
		FAlphaBlend OldBlendIn = NewMontage->BlendIn;
		NewMontage->BlendIn = 0.0f;
		GetDefaultPreviewCharacter()->GetMesh()->GetAnimInstance()->Montage_Play(
			NewMontage, 1.0f, EMontagePlayReturnType::MontageLength, CurrentMontageTime);
		NewMontage->BlendIn = OldBlendIn;
	}
}

void ULoadout_V2::SetPrimaryWeapon(FWeaponData WeaponData)
{
	if (!GetDefaultPreviewCharacter())
		return;

	if (pc)
	{
		if (ps)
		{
			LoadoutFunctionLibrary->SetActivePrimary(WeaponData.Blueprint);

			// update attachments from attachment save data
			FSavedWeaponAttachmentData AttachmentData = GetItemAttachmentData(
				LoadoutFunctionLibrary->GetActivePrimary());

			if (AttachmentData.bHasSavedData)
			{
				LoadoutFunctionLibrary->SetPrimaryAttachment(AttachmentData.ScopeAttachment,
				                                             EWeaponAttachmentType::Optics);
				LoadoutFunctionLibrary->SetPrimaryAttachment(AttachmentData.MuzzleAttachment,
				                                             EWeaponAttachmentType::Muzzle);
				LoadoutFunctionLibrary->SetPrimaryAttachment(AttachmentData.OverbarrelAttachment,
				                                             EWeaponAttachmentType::Overbarrel);
				LoadoutFunctionLibrary->SetPrimaryAttachment(AttachmentData.UnderbarrelAttachment,
				                                             EWeaponAttachmentType::Underbarrel);
				LoadoutFunctionLibrary->SetPrimaryAttachment(AttachmentData.StockAttachment,
				                                             EWeaponAttachmentType::Stock);
				LoadoutFunctionLibrary->
					SetPrimaryAttachment(AttachmentData.GripAttachment, EWeaponAttachmentType::Grip);
				LoadoutFunctionLibrary->SetPrimaryAttachment(AttachmentData.IlluminatorAttachment,
				                                             EWeaponAttachmentType::Illuminators);
				LoadoutFunctionLibrary->SetPrimaryAttachment(AttachmentData.AmmunitionAttachment,
				                                             EWeaponAttachmentType::Ammunition);
				LoadoutFunctionLibrary->SetActivePrimarySkin(AttachmentData.Skin);
			}
			else
			{
				UItemData* ItemData = UBpGameplayHelperLib::GetItemData(GetWorld());
				TSubclassOf<ABaseWeapon> PrimaryWeapon = LoadoutFunctionLibrary->GetActivePrimary();
				FStoredWeaponAttachments StoredPrimaryAttachments = LoadoutFunctionLibrary->StoredAttachmentsByWeapon.
					FindRef(PrimaryWeapon);

				LoadoutFunctionLibrary->SetPrimaryAttachment(
					StoredPrimaryAttachments.ScopeAttachment != nullptr
						? StoredPrimaryAttachments.ScopeAttachment
						: ItemData->NullPrimaryScopeAttachment,
					EWeaponAttachmentType::Optics);
				LoadoutFunctionLibrary->SetPrimaryAttachment(
					StoredPrimaryAttachments.MuzzleAttachment != nullptr
						? StoredPrimaryAttachments.MuzzleAttachment
						: ItemData->NullMuzzleAttachment,
					EWeaponAttachmentType::Muzzle);
				LoadoutFunctionLibrary->SetPrimaryAttachment(
					StoredPrimaryAttachments.OverbarrelAttachment != nullptr
						? StoredPrimaryAttachments.OverbarrelAttachment
						: ItemData->NullOverbarrelAttachment,
					EWeaponAttachmentType::Overbarrel);
				LoadoutFunctionLibrary->SetPrimaryAttachment(
					StoredPrimaryAttachments.UnderbarrelAttachment != nullptr
						? StoredPrimaryAttachments.UnderbarrelAttachment
						: ItemData->NullUnderbarrelAttachment,
					EWeaponAttachmentType::Underbarrel);
				LoadoutFunctionLibrary->SetPrimaryAttachment(
					StoredPrimaryAttachments.StockAttachment != nullptr
						? StoredPrimaryAttachments.StockAttachment
						: ItemData->NullStockAttachment,
					EWeaponAttachmentType::Stock);
				LoadoutFunctionLibrary->SetPrimaryAttachment(
					StoredPrimaryAttachments.GripAttachment != nullptr
						? StoredPrimaryAttachments.GripAttachment
						: ItemData->NullGripAttachment,
					EWeaponAttachmentType::Grip);
				LoadoutFunctionLibrary->SetPrimaryAttachment(
					StoredPrimaryAttachments.IlluminatorAttachment != nullptr
						? StoredPrimaryAttachments.IlluminatorAttachment
						: ItemData->NullIlluminatorAttachment,
					EWeaponAttachmentType::Illuminators);
				LoadoutFunctionLibrary->SetPrimaryAttachment(
					StoredPrimaryAttachments.AmmunitionAttachment != nullptr
						? StoredPrimaryAttachments.AmmunitionAttachment
						: ItemData->NullAmmunitionAttachment,
					EWeaponAttachmentType::Ammunition);
				LoadoutFunctionLibrary->SetActivePrimarySkin(ItemData->FactorySkin);

				/*UItemData* ItemData = UBpGameplayHelperLib::GetItemData(GetWorld());
				if (ItemData)
				{
					LoadoutFunctionLibrary->SetPrimaryAttachment(ItemData->NullPrimaryScopeAttachment,
					                                             EWeaponAttachmentType::Optics);
					LoadoutFunctionLibrary->SetPrimaryAttachment(ItemData->NullMuzzleAttachment,
					                                             EWeaponAttachmentType::Muzzle);
					LoadoutFunctionLibrary->SetPrimaryAttachment(ItemData->NullOverbarrelAttachment,
					                                             EWeaponAttachmentType::Overbarrel);
					LoadoutFunctionLibrary->SetPrimaryAttachment(ItemData->NullUnderbarrelAttachment,
					                                             EWeaponAttachmentType::Underbarrel);
					LoadoutFunctionLibrary->SetPrimaryAttachment(ItemData->NullStockAttachment,
					                                             EWeaponAttachmentType::Stock);
					LoadoutFunctionLibrary->SetPrimaryAttachment(ItemData->NullGripAttachment,
					                                             EWeaponAttachmentType::Grip);
					LoadoutFunctionLibrary->SetPrimaryAttachment(ItemData->NullIlluminatorAttachment,
					                                             EWeaponAttachmentType::Illuminators);
					LoadoutFunctionLibrary->SetPrimaryAttachment(ItemData->NullAmmunitionAttachment,
					                                             EWeaponAttachmentType::Ammunition);
					LoadoutFunctionLibrary->SetActivePrimarySkin(ItemData->FactorySkin);
				}*/
			}
			// SaveActiveLoadout();
			UpdatePreviewCharacterPrimary();
		}
	}
}

void ULoadout_V2::SetSecondaryWeapon(FWeaponData WeaponData)
{
	if (!GetDefaultPreviewCharacter())
		return;
	if (pc)
	{
		if (ps)
		{
			LoadoutFunctionLibrary->SetActiveSecondary(WeaponData.Blueprint);
			// ActiveLoadout.Secondary = ;
			// update attachments from attachment save data
			FSavedWeaponAttachmentData AttachmentData = GetItemAttachmentData(
				LoadoutFunctionLibrary->GetActiveSecondary());
			if (AttachmentData.bHasSavedData)
			{
				LoadoutFunctionLibrary->SetSecondaryAttachment(AttachmentData.ScopeAttachment,
				                                               EWeaponAttachmentType::Optics);
				LoadoutFunctionLibrary->SetSecondaryAttachment(AttachmentData.MuzzleAttachment,
				                                               EWeaponAttachmentType::Muzzle);
				LoadoutFunctionLibrary->SetSecondaryAttachment(AttachmentData.OverbarrelAttachment,
				                                               EWeaponAttachmentType::Overbarrel);
				LoadoutFunctionLibrary->SetSecondaryAttachment(AttachmentData.UnderbarrelAttachment,
				                                               EWeaponAttachmentType::Underbarrel);
				LoadoutFunctionLibrary->SetSecondaryAttachment(AttachmentData.StockAttachment,
				                                               EWeaponAttachmentType::Stock);
				LoadoutFunctionLibrary->SetSecondaryAttachment(AttachmentData.GripAttachment,
				                                               EWeaponAttachmentType::Grip);
				LoadoutFunctionLibrary->SetSecondaryAttachment(AttachmentData.IlluminatorAttachment,
				                                               EWeaponAttachmentType::Illuminators);
				LoadoutFunctionLibrary->SetSecondaryAttachment(AttachmentData.AmmunitionAttachment,
				                                               EWeaponAttachmentType::Ammunition);
				LoadoutFunctionLibrary->SetActiveSecondarySkin(AttachmentData.Skin);
			}
			else
			{
				UItemData* ItemData = UBpGameplayHelperLib::GetItemData(GetWorld());
				TSubclassOf<ABaseWeapon> SecondaryWeapon = LoadoutFunctionLibrary->GetActiveSecondary();
				FStoredWeaponAttachments StoredSecondaryAttachments = LoadoutFunctionLibrary->StoredAttachmentsByWeapon.
					FindRef(SecondaryWeapon);

				LoadoutFunctionLibrary->SetSecondaryAttachment(
					StoredSecondaryAttachments.ScopeAttachment != nullptr
						? StoredSecondaryAttachments.ScopeAttachment
						: ItemData->NullPrimaryScopeAttachment,
					EWeaponAttachmentType::Optics);
				LoadoutFunctionLibrary->SetSecondaryAttachment(
					StoredSecondaryAttachments.MuzzleAttachment != nullptr
						? StoredSecondaryAttachments.MuzzleAttachment
						: ItemData->NullMuzzleAttachment,
					EWeaponAttachmentType::Muzzle);
				LoadoutFunctionLibrary->SetSecondaryAttachment(
					StoredSecondaryAttachments.OverbarrelAttachment != nullptr
						? StoredSecondaryAttachments.OverbarrelAttachment
						: ItemData->NullOverbarrelAttachment,
					EWeaponAttachmentType::Overbarrel);
				LoadoutFunctionLibrary->SetSecondaryAttachment(
					StoredSecondaryAttachments.UnderbarrelAttachment != nullptr
						? StoredSecondaryAttachments.UnderbarrelAttachment
						: ItemData->NullUnderbarrelAttachment,
					EWeaponAttachmentType::Underbarrel);
				LoadoutFunctionLibrary->SetSecondaryAttachment(
					StoredSecondaryAttachments.StockAttachment != nullptr
						? StoredSecondaryAttachments.StockAttachment
						: ItemData->NullStockAttachment,
					EWeaponAttachmentType::Stock);
				LoadoutFunctionLibrary->SetSecondaryAttachment(
					StoredSecondaryAttachments.GripAttachment != nullptr
						? StoredSecondaryAttachments.GripAttachment
						: ItemData->NullGripAttachment,
					EWeaponAttachmentType::Grip);
				LoadoutFunctionLibrary->SetSecondaryAttachment(
					StoredSecondaryAttachments.IlluminatorAttachment != nullptr
						? StoredSecondaryAttachments.IlluminatorAttachment
						: ItemData->NullIlluminatorAttachment,
					EWeaponAttachmentType::Illuminators);
				LoadoutFunctionLibrary->SetSecondaryAttachment(
					StoredSecondaryAttachments.AmmunitionAttachment != nullptr
						? StoredSecondaryAttachments.AmmunitionAttachment
						: ItemData->NullAmmunitionAttachment,
					EWeaponAttachmentType::Ammunition);
				LoadoutFunctionLibrary->SetActiveSecondarySkin(ItemData->FactorySkin);

				/*if (ItemData)
				{
					LoadoutFunctionLibrary->SetSecondaryAttachment(ItemData->NullPrimaryScopeAttachment,
					                                               EWeaponAttachmentType::Optics);
					LoadoutFunctionLibrary->SetSecondaryAttachment(ItemData->NullMuzzleAttachment,
					                                               EWeaponAttachmentType::Muzzle);
					LoadoutFunctionLibrary->SetSecondaryAttachment(ItemData->NullOverbarrelAttachment,
					                                               EWeaponAttachmentType::Overbarrel);
					LoadoutFunctionLibrary->SetSecondaryAttachment(ItemData->NullUnderbarrelAttachment,
					                                               EWeaponAttachmentType::Underbarrel);
					LoadoutFunctionLibrary->SetSecondaryAttachment(ItemData->NullStockAttachment,
					                                               EWeaponAttachmentType::Stock);
					LoadoutFunctionLibrary->SetSecondaryAttachment(ItemData->NullGripAttachment,
					                                               EWeaponAttachmentType::Grip);
					LoadoutFunctionLibrary->SetSecondaryAttachment(ItemData->NullIlluminatorAttachment,
					                                               EWeaponAttachmentType::Illuminators);
					LoadoutFunctionLibrary->SetSecondaryAttachment(ItemData->NullAmmunitionAttachment,
					                                               EWeaponAttachmentType::Ammunition);
					LoadoutFunctionLibrary->SetActiveSecondarySkin(ItemData->FactorySkin);
				}*/
			}
			// SaveActiveLoadout();
			UpdatePreviewCharacterSecondary();
		}
	}
}

FSavedWeaponAttachmentData ULoadout_V2::GetItemAttachmentData(TSubclassOf<ABaseItem> Weapon)
{
	UBaseProfile* Profile = UBaseProfile::GetCurrentProfile();
	if (!Profile)
		return FSavedWeaponAttachmentData();

	FSavedWeaponAttachmentData* AttachmentData = Profile->AttachmentSaveMap.Find(Weapon);
	if (AttachmentData)
		return *AttachmentData;

	return FSavedWeaponAttachmentData();
}

FString ULoadout_V2::GetPistolNonPistolVariation(FString InAnimation)
{
	FString OutAnimation = InAnimation;

	AReadyOrNotCharacter* Character = GetDefaultPreviewCharacter();
	if (!Character || !Character->GetInventoryComponent())
		return OutAnimation;

	ABaseWeapon* PrimaryWeapon = Cast<ABaseWeapon>(Character->GetInventoryComponent()->GetSpawnedGear().Primary);
	if (!IsValid(PrimaryWeapon))
		return OutAnimation;
	
	bool bIsPistolGrip = PrimaryWeapon->bPistolGrip;
	
	if (InAnimation == "tp_pregame_idle_primary")
	{
		OutAnimation = bIsPistolGrip ? "tp_pregame_idle_primary_pistol" : "tp_pregame_idle_primary_nonpistol";
	}
	if (InAnimation == "tp_pregame_idle_after_weapon")
	{
		OutAnimation = bIsPistolGrip
			               ? "tp_pregame_idle_after_weapon_pistol"
			               : "tp_pregame_idle_after_weapon_nonpistol";
	}
	if (InAnimation == "tp_pregame_idle_helmet_after_weapon")
	{
		OutAnimation = bIsPistolGrip
			               ? "tp_pregame_idle_helmet_after_weapon_pistol"
			               : "tp_pregame_idle_helmet_after_weapon_nonpistol";
	}
	if (InAnimation == "tp_pregame_idle_secondary_after_weapon")
	{
		OutAnimation = bIsPistolGrip
			               ? "tp_pregame_idle_secondary_after_weapon_pistol"
			               : "tp_pregame_idle_secondary_after_weapon_nonpistol";
	}
	if (InAnimation == "tp_pregame_idle_secondary_already_drawn")
	{
		OutAnimation = bIsPistolGrip
			               ? "tp_pregame_idle_secondary_already_drawn_pistol"
			               : "tp_pregame_idle_secondary_already_drawn_nonpistol";
	}
	if (InAnimation == "tp_pregame_idle_secondary_already_drawn_after_weapon")
	{
		OutAnimation = bIsPistolGrip
			               ? "tp_pregame_idle_secondary_already_drawn_after_weapon_pistol"
			               : "tp_pregame_idle_secondary_already_drawn_after_weapon_nonpistol";
	}
	
	return OutAnimation;
}

void ULoadout_V2::HidePrimary(bool bIsHidden)
{
	if (!GetDefaultPreviewCharacter())
		return;
	if (GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Primary)
	{
		GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Primary->
		                              SetActorHiddenInGame(bIsHidden);
	}
}

void ULoadout_V2::UpdatePreviewWeaponAttachments(bool IsSecondary, TSubclassOf<UWeaponAttachment> Attachment)
{
	if (!GetDefaultPreviewCharacter())
		return;

	ABaseWeapon* bw = !IsSecondary
		                  ? Cast<ABaseWeapon>(
			                  GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Primary)
		                  : Cast<ABaseWeapon>(
			                  GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Secondary);

	if (bw)
	{
		LoadAddAttachment(bw, Attachment, false);
	}
}

void ULoadout_V2::LoadAddAttachment(ABaseWeapon* BaseWeapon, TSubclassOf<UWeaponAttachment> Attachment,
                                    bool bReplicateAttachment)
{
	BaseWeapon->AddAttachment(Attachment, bReplicateAttachment);
	BaseWeapon->UpdateStoredAttachments(Attachment);
}

void ULoadout_V2::LoadWeaponPresets()
{
	UBaseProfile* Profile = UBaseProfile::GetCurrentProfile();
	if (!Profile)
		return;

	WeaponToWeaponPresetsMap = Profile->WeaponToWeaponPresetsMap;
	OnLoadoutItemPresetsLoaded();
}

// void ULoadout_V2::SetEquippingSwatMember(EEquippingSwat NewEquippingSwat,
//                                          AReadyOrNotPlayerState* NewEquippingPlayerState)
// {
// 	if (!pc)
// 		return;
// 
// 	if (!ps)
// 		return;
// 
// 	LoadoutFunctionLibrary->DoSaveActiveLoadout();
// 	LoadoutFunctionLibrary->SetActiveSwatMember(NewEquippingSwat);
// 	// EquippingSwatMember = NewEquippingSwat;
// 	if (NewEquippingPlayerState)
// 	{
// 		EquippingPlayerState = NewEquippingPlayerState;
// 	}
// 	else
// 	{
// 		EquippingPlayerState = ps;
// 	}
// 	switch (LoadoutFunctionLibrary->GetActiveSwatMember())
// 	{
// 	case EEquippingSwat::ES_None:
// 		//Get remote player serve loadout for previewing other players in multiplayer
// 		if (ps != EquippingPlayerState)
// 		{
// 			// ActiveLoadout = EquippingPlayerState->ServerSavedLoadout;
// 		}
// 		else
// 		{
// 			UBpGameplayHelperLib::LoadLoadout(ActiveLoadout, "default");
// 		}
// 		break;
// 	case EEquippingSwat::ES_BlueOne:
// 		UBpGameplayHelperLib::LoadLoadout(ActiveLoadout, "defaultblueone");
// 		ActiveLoadout.CharacterLookOverride = "SWAT_King";
// 		break;
// 	case EEquippingSwat::ES_BlueTwo:
// 		UBpGameplayHelperLib::LoadLoadout(ActiveLoadout, "defaultbluetwo");
// 		ActiveLoadout.CharacterLookOverride = "SWAT_Swan";
// 		break;
// 	case EEquippingSwat::ES_RedOne:
// 		UBpGameplayHelperLib::LoadLoadout(ActiveLoadout, "defaultredone");
// 		ActiveLoadout.CharacterLookOverride = "SWAT_Prescott";
// 		break;
// 	case EEquippingSwat::ES_RedTwo:
// 		UBpGameplayHelperLib::LoadLoadout(ActiveLoadout, "defaultredtwo");
// 		ActiveLoadout.CharacterLookOverride = "SWAT_Eli";
// 		break;
// 	default: ;
// 	}
// 
// 	if (GetPreviewCharacter("PreviewCharacter"))
// 		GetPreviewCharacter("PreviewCharacter")->UpdateOverridesFromCharacterLookOverrideDataTable(
// 			ActiveLoadout.CharacterLookOverride);
// 
// 	LastSavedLoadout.Add(NewEquippingSwat, ActiveLoadout);
// 	UBpGameplayHelperLib::SanitizeLoadout(ActiveLoadout);
// 	FSavedLoadout TmpLoadout = ActiveLoadout;
// 
// 	if (GetPreviewCharacter("PreviewCharacter"))
// 	{
// 		GetPreviewCharacter("PreviewCharacter")->PlayMontageFromTable("tp_pregame_idle");
// 		if (GetPreviewCharacter("PreviewCharacter")->IsTableMontagePlaying("tp_pregame_idle"))
// 		{
// 			TmpLoadout.Primary = nullptr;
// 		}
// 	}
// 
// 	FLoadoutEquipOptions LoadoutEquipOptions;
// 	LoadoutEquipOptions.bReplicates = false;
// 	LoadoutEquipOptions.bSanitizeLoadout = false;
// 	UBpGameplayHelperLib::EquipLoadoutOnPlayer(TmpLoadout, GetPreviewCharacter("PreviewCharacter"),
// 	                                           LoadoutEquipOptions);
// 	OnLoadoutLoaded();
// 	OnSwatCharacterChanged();
// }

void ULoadout_V2::EquipSecondary()
{
	if (!GetDefaultPreviewCharacter())
		return;

	ABaseItem* bi = Cast<ABaseItem>(GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Secondary);
	if (bi)
	{
		// FName Socket = bi->HandsSocket;
		// bi->GetItemMesh()->AttachToComponent(GetDefaultPreviewCharacter()->GetMesh(),
		// 	FAttachmentTransformRules::SnapToTargetIncludingScale, Socket);
		// bi->GetItemMesh()->SetVisibility(true);

		GetDefaultPreviewCharacter()->GetInventoryComponent()->PutItemInHands(bi, true, true);
	}

	GetPreviewCharacter("PreviewCharacter")->PlayMontageFromTable("tp_pregame_idle_secondary");
}

void ULoadout_V2::SetActiveCameraByTagWithFade(FName Tag, float BlendTime, float FadeTime)
{
	const float Duration = 1.0f;
	pc->PlayerCameraManager->StartCameraFade(0.0f, 2.0f, Duration, FLinearColor::Black, false, false);
	FTimerHandle Handle;
	FTimerDelegate TimerDelegate = FTimerDelegate::CreateUObject(this, &ULoadout_V2::SetActiveCameraByTag, Tag,
	                                                             BlendTime);
	GetWorld()->GetTimerManager().SetTimer(Handle, TimerDelegate, Duration, false);
}

void ULoadout_V2::SetActiveCameraByTag(FName Tag, float BlendTime)
{
	if (bDestructing)
		return;

	if (pc)
	{
		FViewTargetTransitionParams TransitionParams;
		TransitionParams.BlendTime = BlendTime;
		TransitionParams.BlendExp = BlendTime * 0.25f;
		TransitionParams.bLockOutgoing = true;
		for (TActorIterator<ACameraActor> It(GetWorld()); It; ++It)
		{
			ACameraActor* c = *It;
			if (c->Tags.Contains(Tag))
			{
				// stored and update the camera fov based on the scaler
				if (!CameraTagOriginalFovMap.Contains(Tag))
				{
					CameraTagOriginalFovMap.Add(Tag, c->GetCameraComponent()->FieldOfView);
				}

				if (BlendTime > 0.0f)
				{
					SetLockInput(true);
					GetWorld()->GetTimerManager().SetTimer(UnlockInput_Handle,
					                                       FTimerDelegate::CreateUObject(
						                                       this, &ULoadout_V2::SetLockInput, false),
					                                       BlendTime * 0.1f, false);
				}

				if (pc->GetViewTarget())
				{
					float Distance = (pc->GetViewTarget()->GetActorLocation() - c->GetActorLocation()).Size();
					if (Distance > 800.0f)
					{
						TransitionParams.BlendTime = 0.0f;
					}
				}
				pc->SetViewTarget(c, TransitionParams);
				ActiveCameraTag = Tag;
			}
		}
	}
}

void ULoadout_V2::SetLockInput(bool bShouldLockInput)
{
	bIsInputLocked = bShouldLockInput;
}

bool ULoadout_V2::GetInputLocked()
{
	return bIsInputLocked;
}

// void ULoadout_V2::SaveActiveLoadout()
// {
// 	// TODO
// 	if ((LoadoutFunctionLibrary->GetActiveSwatMember() == EEquippingSwat::ES_None) && ps == EquippingPlayerState)
// 	{
// 		if (ps)
// 		{
// 			if (ps->LastLoadout != LoadoutFunctionLibrary->GetActiveLoadout())
// 			{
// 				ps->Server_SetLoadout(LoadoutFunctionLibrary->GetActiveLoadout());
// 				ps->Server_SetLoadout_Implementation(LoadoutFunctionLibrary->GetActiveLoadout());
// 				OnLoadoutSaved();
// 			}
// 		}
// 	}
// 	else
// 	{
// 		if (const FSavedLoadout* FoundLoadout = LastSavedLoadout.Find(LoadoutFunctionLibrary->GetActiveSwatMember()))
// 		{
// 			if (!(*FoundLoadout == LoadoutFunctionLibrary->GetActiveLoadout()))
// 			{
// 				GetWorld()->GetTimerManager().SetTimer(TH_DoSaveLoadout,
// 				                                       FTimerDelegate::CreateUObject(
// 					                                       LoadoutFunctionLibrary,
// 					                                       &UReadyOrNotLoadoutFunctionLibrary::DoSaveActiveLoadout),
// 				                                       0.5f, false);
// 				OnLoadoutSaved();
// 			}
// 		}
// 	}
// }

void ULoadout_V2::SetItem(EItemType ItemType, TSubclassOf<ABaseItem> ItemClass)
{
	// LastSetItemType = ItemType;
	FWeaponData WeaponData;
	switch (ItemType)
	{
	case EItemType::IT_None:
		break;
	case EItemType::IT_Rifles:
		WeaponData.Blueprint = ItemClass;
		SetPrimaryWeapon(WeaponData);
		break;
	case EItemType::IT_SubmachineGun:
		WeaponData.Blueprint = ItemClass;
		SetPrimaryWeapon(WeaponData);
		break;
	case EItemType::IT_Shotgun:
		WeaponData.Blueprint = ItemClass;
		SetPrimaryWeapon(WeaponData);
		break;
	case EItemType::IT_PistolsLethal:
		WeaponData.Blueprint = ItemClass;
		SetSecondaryWeapon(WeaponData);
		break;
	case EItemType::IT_PistolsNonLethal:
		WeaponData.Blueprint = ItemClass;
		SetSecondaryWeapon(WeaponData);
		break;
	case EItemType::IT_PrimaryNonLethal:
		WeaponData.Blueprint = ItemClass;
		SetPrimaryWeapon(WeaponData);
		break;
	case EItemType::IT_Headwear:
		SetHeadwear(ItemClass);
		break;
	case EItemType::IT_BodyArmor:
		SetBodyArmour(ItemClass);
		break;
	case EItemType::IT_LongTactical:
		SetLongTactical(ItemClass);
		break;
	case EItemType::IT_Launcher:
		WeaponData.Blueprint = ItemClass;
		SetPrimaryWeapon(WeaponData);
		break;
	default:
		break;
	}
}

void ULoadout_V2::SetLongTactical(TSubclassOf<ABaseItem> LongTactical)
{
	if (!GetDefaultPreviewCharacter())
		return;

	LoadoutFunctionLibrary->SetActiveLongTactical(LongTactical);
	UpdatePreviewCharacterLongTactical();
}

void ULoadout_V2::UpdatePreviewCharacterLongTactical()
{
	// if (!GetDefaultPreviewCharacter())
	// 	return;

	// if (GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().LongTactical)
	// {
	// 	GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().LongTactical->Destroy();
	// }
	// ABaseItem* LongTacticalInst = GetWorld()->SpawnActor<ABaseItem>(ActiveLoadout.LongTactical);;
	// GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().LongTactical = LongTacticalInst;
	// if (LongTacticalInst)
	// {
	// 	LongTacticalInst->SetReplicates(false);
	// }
	// GetDefaultPreviewCharacter()->GetInventoryComponent()->AddInventoryItem(
	// 	GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().LongTactical);
}

void ULoadout_V2::SetHeadwear(TSubclassOf<ABaseItem> Headwear)
{
	if (!GetDefaultPreviewCharacter())
		return;
	LoadoutFunctionLibrary->SetActiveHeadwear(Headwear);
	// ActiveLoadout.Helmet = Headwear;
	UpdatePreviewCharacterHeadwear();
}

void ULoadout_V2::UpdatePreviewCharacterHeadwear()
{
	if (!GetDefaultPreviewCharacter())
		return;

	if (ABaseItem* Helmet = GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Helmet)
	{
		Helmet->Destroy();
	}

	if (ABaseItem* bi = GetWorld()->SpawnActor<ABaseItem>(LoadoutFunctionLibrary->GetActiveHeadwear()))
	{
		GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Helmet = bi;
		bi->Tags.Add("NoRelevancy");
		bi->Tags.Add("CustomizationMenu");
		bi->bNoAttachmentRep = true;
		bi->SetReplicates(false);
		GetDefaultPreviewCharacter()->GetInventoryComponent()->AddInventoryItem(
			GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Helmet);
		bi->GetItemMesh()->SetVisibility(true);
		bi->AttachToComponent(GetDefaultPreviewCharacter()->GetMesh(),
		                      FAttachmentTransformRules::SnapToTargetIncludingScale, bi->BodySocket);
	}
}

void ULoadout_V2::SetBodyArmour(TSubclassOf<ABaseItem> BodyArmour)
{
	if (!GetDefaultPreviewCharacter())
		return;

	if (BodyArmour)
	{
		LoadoutFunctionLibrary->SetActiveBodyArmor(BodyArmour);
		// ActiveLoadout.Armor = BodyArmour;
	}

	UpdatePreviewCharacterArmour();
}

void ULoadout_V2::UpdatePreviewCharacterArmour()
{
	if (!GetDefaultPreviewCharacter())
		return;


	if (GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Armor)
	{
		if (GetDefaultPreviewCharacter()->GetMeshGearSlot())
		{
			GetDefaultPreviewCharacter()->GetMeshGearSlot()->SetSkeletalMesh(nullptr);
		}
		GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Armor->Destroy();
	}
	if (LoadoutFunctionLibrary->GetActiveBodyArmor())
	{
		ABaseItem* Armor = GetWorld()->SpawnActor<ABaseItem>(LoadoutFunctionLibrary->GetActiveBodyArmor());
		GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Armor = Armor;
		if (Armor)
		{
			Armor->SetReplicates(false);
		}
		GetDefaultPreviewCharacter()->GetInventoryComponent()->AddInventoryItem(
			GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Armor);
	}
}

// void ULoadout_V2::SetWorkbenchItemClass(TSubclassOf<ABaseItem> Item, FName Tag)
// {
// 	// TODO get rid of troublesome global state
// 	// if (bIsWeaponCustomization)
// 	// {
// 	// 	Tag = "AttachmentWorkbench";
// 	// }
// 
// 	if (ABaseItem** DoubleItemPtr = WorkBenchItemPtrMap.Find(Tag))
// 	{
// 		if (ABaseItem* DereferencedItem = *DoubleItemPtr)
// 		{
// 			DereferencedItem->Destroy();
// 			DereferencedItem = nullptr;
// 		}
// 	}
// 
// 	if (pc)
// 	{
// 		AActor* PlacementActor = nullptr;
// 		V_LOGM(LogReadyOrNot, "Searching for bench placement actor of tag %s", *Tag.ToString());
// 		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
// 		{
// 			AActor* a = *It;
// 			if (a->Tags.Contains(Tag))
// 			{
// 				/*float Dist = (a->GetActorLocation() - pc->GetPawn()->GetActorLocation()).Size();
// 				if (Dist < 500.0f)
// 				{
// 					PlacementActor = a;
// 					break;
// 				}*/
// 				PlacementActor = a;
// 				break;
// 			}
// 		}
// 
// 		//must have a bench placement actor otherwise this equipment menu won't work correctly
// 		ensure(PlacementActor);
// 		if (!PlacementActor)
// 			return;
// 
// 		if (ps)
// 		{
// 			FSavedLoadout Loadout = ps->GetLoadout();
// 			WorkBenchItemPtrMap.Add(Tag, GetWorld()->SpawnActor<ABaseItem>(Item, PlacementActor->GetActorTransform()));
// 			ABaseWeapon* bw = Cast<ABaseWeapon>(WorkBenchItemPtrMap[Tag]);
// 			if (bw)
// 			{
// 				bw->Tags.Add("NoRelevancy");
// 				bw->SetReplicates(false);
// 				if (bw->ItemCategories.Contains(EItemCategory::IC_Primary))
// 				{
// 					bw->GetItemMesh()->VisibilityBasedAnimTickOption =
// 						EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
// 					bw->GetItemMesh()->SetSimulatePhysics(true);
// 					bw->GetItemMesh()->SetLinearDamping(1.0f);
// 					bw->GetItemMesh()->SetAngularDamping(10000.0f);
// 					bw->AddAttachment(ActiveLoadout.PrimaryScope);
// 					bw->AddAttachment(ActiveLoadout.PrimaryMuzzle);
// 					bw->AddAttachment(ActiveLoadout.PrimaryOverbarrel);
// 					bw->AddAttachment(ActiveLoadout.PrimaryUnderbarrel);
// 					bw->AddAttachment(ActiveLoadout.PrimaryStock);
// 					bw->AddAttachment(ActiveLoadout.PrimaryGrip);
// 					bw->AddAttachment(ActiveLoadout.PrimaryIlluminator);
// 					bw->AddAttachment(ActiveLoadout.PrimaryAmmunition);
// 
// 					if (ActiveLoadout.PrimarySkin)
// 					{
// 						USkinComponent* SkinComp = NewObject<USkinComponent>(bw, ActiveLoadout.PrimarySkin);
// 						if (SkinComp)
// 						{
// 							SkinComp->RegisterComponent();
// 						}
// 					}
// 					bw->AttachStatic();
// 				}
// 				else if (bw->ItemCategories.Contains(EItemCategory::IC_Secondary))
// 				{
// 					bw->GetItemMesh()->VisibilityBasedAnimTickOption =
// 						EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
// 					bw->GetItemMesh()->SetLinearDamping(1.0f);
// 					bw->GetItemMesh()->SetAngularDamping(10000.0f);
// 					bw->GetItemMesh()->SetSimulatePhysics(true);
// 
// 					bw->AddAttachment(ActiveLoadout.SecondaryScope);
// 					bw->AddAttachment(ActiveLoadout.SecondaryMuzzle);
// 					bw->AddAttachment(ActiveLoadout.SecondaryOverbarrel);
// 					bw->AddAttachment(ActiveLoadout.SecondaryUnderbarrel);
// 					bw->AddAttachment(ActiveLoadout.SecondaryStock);
// 					bw->AddAttachment(ActiveLoadout.SecondaryGrip);
// 					bw->AddAttachment(ActiveLoadout.SecondaryIlluminator);
// 					bw->AddAttachment(ActiveLoadout.SecondaryAmmunition);
// 
// 					if (ActiveLoadout.SecondarySkin)
// 					{
// 						USkinComponent* SkinComp = NewObject<USkinComponent>(bw, ActiveLoadout.SecondarySkin);
// 						if (SkinComp)
// 						{
// 							SkinComp->RegisterComponent();
// 						}
// 					}
// 					bw->AttachStatic();
// 				}
// 			}
// 		}
// 	}
// }

// void ULoadout_V2::UpdateWorkbenchItemAttachments()
// {
// 	FName PrimaryTag = "PrimaryPlacementAttachments";
// 	FName SecondaryTag = "SecondaryPlacementAttachments";
// 	// TODO urgh
// 	// if (bIsWeaponCustomization)
// 	// {
// 	// bIsCustomizingPrimary ? PrimaryTag = "AttachmentWorkbench" : SecondaryTag = "AttachmentWorkbench";
// 	// }
// 	PrimaryTag = "AttachmentWorkbench";
// 	if (WorkBenchItemPtrMap.Find(PrimaryTag))
// 	{
// 		if (ABaseItem* PrimaryItem = *WorkBenchItemPtrMap.Find(PrimaryTag))
// 		{
// 			if (ABaseWeapon* PrimaryBW = Cast<ABaseWeapon>(PrimaryItem))
// 			{
// 				PrimaryBW->AddAttachment(LoadoutFunctionLibrary->GetActiveLoadout().PrimaryScope);
// 				PrimaryBW->AddAttachment(LoadoutFunctionLibrary->GetActiveLoadout().PrimaryMuzzle);
// 				PrimaryBW->AddAttachment(LoadoutFunctionLibrary->GetActiveLoadout().PrimaryOverbarrel);
// 				PrimaryBW->AddAttachment(LoadoutFunctionLibrary->GetActiveLoadout().PrimaryUnderbarrel);
// 				PrimaryBW->AddAttachment(LoadoutFunctionLibrary->GetActiveLoadout().PrimaryStock);
// 				PrimaryBW->AddAttachment(LoadoutFunctionLibrary->GetActiveLoadout().PrimaryGrip);
// 				PrimaryBW->AddAttachment(LoadoutFunctionLibrary->GetActiveLoadout().PrimaryIlluminator);
// 				PrimaryBW->AddAttachment(LoadoutFunctionLibrary->GetActiveLoadout().PrimaryAmmunition);
// 
// 				TArray<USkinComponent*> SkinComps;
// 				PrimaryBW->GetComponents<USkinComponent>(SkinComps);
// 
// 				for (int32 i = 0; i < SkinComps.Num(); i++)
// 				{
// 					USkinComponent* SkinComp = SkinComps[i];
// 					if (SkinComp/* && SkinComp->GetClass() != ActiveLoadout.PrimarySkin*/)
// 					{
// 						SkinComp->ResetSkin();
// 						SkinComp->DestroyComponent();
// 					}
// 				}
// 
// 				if (LoadoutFunctionLibrary->GetActiveLoadout().PrimarySkin)
// 				{
// 					PrimaryBW->GetComponents<USkinComponent>(SkinComps);
// 
// 					if (SkinComps.Num() == 0)
// 					{
// 						USkinComponent* SkinComp = NewObject<USkinComponent>(PrimaryBW, LoadoutFunctionLibrary->GetActiveLoadout().PrimarySkin);
// 						if (SkinComp)
// 						{
// 							SkinComp->RegisterComponent();
// 
// 							if (SkinComp->bResetsToFactorySkin)
// 							{
// 								SkinComp->ResetSkin();
// 							}
// 							else
// 							{
// 								SkinComp->ApplySkin();
// 							}
// 						}
// 					}
// 				}
// 			}
// 		}
// 	}
// 
// 	if (WorkBenchItemPtrMap.Find(SecondaryTag))
// 	{
// 		if (ABaseItem* SecondaryItem = *WorkBenchItemPtrMap.Find(SecondaryTag))
// 		{
// 			if (ABaseWeapon* SecondaryBW = Cast<ABaseWeapon>(SecondaryItem))
// 			{
// 				SecondaryBW->AddAttachment(ActiveLoadout.SecondaryScope);
// 				SecondaryBW->AddAttachment(ActiveLoadout.SecondaryMuzzle);
// 				SecondaryBW->AddAttachment(ActiveLoadout.SecondaryOverbarrel);
// 				SecondaryBW->AddAttachment(ActiveLoadout.SecondaryUnderbarrel);
// 				SecondaryBW->AddAttachment(ActiveLoadout.SecondaryStock);
// 				SecondaryBW->AddAttachment(ActiveLoadout.SecondaryGrip);
// 				SecondaryBW->AddAttachment(ActiveLoadout.SecondaryIlluminator);
// 				SecondaryBW->AddAttachment(ActiveLoadout.SecondaryAmmunition);
// 
// 				TArray<USkinComponent*> SkinComps;
// 				SecondaryBW->GetComponents<USkinComponent>(SkinComps);
// 
// 				for (int32 i = 0; i < SkinComps.Num(); i++)
// 				{
// 					USkinComponent* SkinComp = Cast<USkinComponent>(SkinComps[i]);
// 					if (SkinComp/* && SkinComp->GetClass() != ActiveLoadout.SecondarySkin*/)
// 					{
// 						SkinComp->ResetSkin();
// 						SkinComp->DestroyComponent();
// 					}
// 				}
// 
// 				if (ActiveLoadout.SecondarySkin)
// 				{
// 					SecondaryBW->GetComponents<USkinComponent>(SkinComps);
// 
// 					if (SkinComps.Num() == 0)
// 					{
// 						USkinComponent* SkinComp = NewObject<USkinComponent>(SecondaryBW, ActiveLoadout.SecondarySkin);
// 						if (SkinComp)
// 						{
// 							SkinComp->RegisterComponent();
// 
// 							if (SkinComp->bResetsToFactorySkin)
// 								SkinComp->ResetSkin();
// 							else
// 								SkinComp->ApplySkin();
// 						}
// 					}
// 				}
// 			}
// 		}
// 	}
// }

void ULoadout_V2::UpdateWorkbenchItemAttachments(FSavedLoadout Loadout, bool IsSecondary)
{
	FName PrimaryTag = "PrimaryPlacementAttachments";
	FName SecondaryTag = "SecondaryPlacementAttachments";
	// TODO urgh
	// if (bIsWeaponCustomization)
	// {
	// bIsCustomizingPrimary ? PrimaryTag = "AttachmentWorkbench" : SecondaryTag = "AttachmentWorkbench";
	// }
	if (IsSecondary)
	{
		SecondaryTag = "AttachmentWorkbench";
	}
	else
	{
		PrimaryTag = "AttachmentWorkbench";
	}
	if (WorkBenchItemPtrMap.Find(PrimaryTag))
	{
		if (ABaseItem* PrimaryItem = *WorkBenchItemPtrMap.Find(PrimaryTag))
		{
			if (ABaseWeapon* PrimaryBW = Cast<ABaseWeapon>(PrimaryItem))
			{
				LoadAddAttachment(PrimaryBW, Loadout.PrimaryScope);
				LoadAddAttachment(PrimaryBW, Loadout.PrimaryMuzzle);
				LoadAddAttachment(PrimaryBW, Loadout.PrimaryOverbarrel);
				LoadAddAttachment(PrimaryBW, Loadout.PrimaryUnderbarrel);
				LoadAddAttachment(PrimaryBW, Loadout.PrimaryStock);
				LoadAddAttachment(PrimaryBW, Loadout.PrimaryGrip);
				LoadAddAttachment(PrimaryBW, Loadout.PrimaryIlluminator);
				LoadAddAttachment(PrimaryBW, Loadout.PrimaryAmmunition);

				TArray<USkinComponent*> SkinComps;
				PrimaryBW->GetComponents<USkinComponent>(SkinComps);

				for (int32 i = 0; i < SkinComps.Num(); i++)
				{
					USkinComponent* SkinComp = SkinComps[i];
					if (SkinComp/* && SkinComp->GetClass() != ActiveLoadout.PrimarySkin*/)
					{
						SkinComp->ResetSkin();
						SkinComp->DestroyComponent();
					}
				}

				if (LoadoutFunctionLibrary->GetActiveLoadout().PrimarySkin)
				{
					PrimaryBW->GetComponents<USkinComponent>(SkinComps);

					if (SkinComps.Num() == 0)
					{
						USkinComponent* SkinComp = NewObject<USkinComponent>(PrimaryBW, Loadout.PrimarySkin);
						if (SkinComp)
						{
							SkinComp->RegisterComponent();

							if (SkinComp->bResetsToFactorySkin)
							{
								SkinComp->ResetSkin();
							}
							else
							{
								SkinComp->ApplySkin();
							}
						}
					}
				}
			}
		}
	}

	if (WorkBenchItemPtrMap.Find(SecondaryTag))
	{
		if (ABaseItem* SecondaryItem = *WorkBenchItemPtrMap.Find(SecondaryTag))
		{
			if (ABaseWeapon* SecondaryBW = Cast<ABaseWeapon>(SecondaryItem))
			{
				LoadAddAttachment(SecondaryBW, Loadout.SecondaryScope);
				LoadAddAttachment(SecondaryBW, Loadout.SecondaryMuzzle);
				LoadAddAttachment(SecondaryBW, Loadout.SecondaryOverbarrel);
				LoadAddAttachment(SecondaryBW, Loadout.SecondaryUnderbarrel);
				LoadAddAttachment(SecondaryBW, Loadout.SecondaryStock);
				LoadAddAttachment(SecondaryBW, Loadout.SecondaryGrip);
				LoadAddAttachment(SecondaryBW, Loadout.SecondaryIlluminator);
				LoadAddAttachment(SecondaryBW, Loadout.SecondaryAmmunition);

				TArray<USkinComponent*> SkinComps;
				SecondaryBW->GetComponents<USkinComponent>(SkinComps);

				for (int32 i = 0; i < SkinComps.Num(); i++)
				{
					USkinComponent* SkinComp = Cast<USkinComponent>(SkinComps[i]);
					if (SkinComp/* && SkinComp->GetClass() != ActiveLoadout.SecondarySkin*/)
					{
						SkinComp->ResetSkin();
						SkinComp->DestroyComponent();
					}
				}

				if (Loadout.SecondarySkin)
				{
					SecondaryBW->GetComponents<USkinComponent>(SkinComps);

					if (SkinComps.Num() == 0)
					{
						USkinComponent* SkinComp = NewObject<USkinComponent>(SecondaryBW, Loadout.SecondarySkin);
						if (SkinComp)
						{
							SkinComp->RegisterComponent();

							if (SkinComp->bResetsToFactorySkin)
								SkinComp->ResetSkin();
							else
								SkinComp->ApplySkin();
						}
					}
				}
			}
		}
	}
}

void ULoadout_V2::SetWorkbenchItemClass(TSubclassOf<ABaseItem> Item, FName Tag, FSavedLoadout Loadout)
{
	// TODO get rid of troublesome global state
	// if (bIsWeaponCustomization)
	// {
	// 	Tag = "AttachmentWorkbench";
	// }

	if (ABaseItem** DoubleItemPtr = WorkBenchItemPtrMap.Find(Tag))
	{
		if (ABaseItem* DereferencedItem = *DoubleItemPtr)
		{
			DereferencedItem->Destroy();
			DereferencedItem = nullptr;
		}
	}

	// if (pc)
	// {
	AActor* PlacementActor = nullptr;
	V_LOGM(LogReadyOrNot, "Searching for bench placement actor of tag %s", *Tag.ToString());
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* a = *It;
		if (a->Tags.Contains(Tag))
		{
			/*float Dist = (a->GetActorLocation() - pc->GetPawn()->GetActorLocation()).Size();
			if (Dist < 500.0f)
			{
				PlacementActor = a;
				break;
			}*/
			PlacementActor = a;
			break;
		}
	}

	//must have a bench placement actor otherwise this equipment menu won't work correctly
	ensure(PlacementActor);
	if (!PlacementActor)
		return;

	// if (ps)
	// {
	// FSavedLoadout Loadout = ps->GetLoadout();
	WorkBenchItemPtrMap.Add(Tag, GetWorld()->SpawnActor<ABaseItem>(Item, PlacementActor->GetActorTransform()));
	ABaseWeapon* bw = Cast<ABaseWeapon>(WorkBenchItemPtrMap[Tag]);
	if (bw)
	{
		bw->Tags.Add("NoRelevancy");
		bw->SetReplicates(false);
		if (bw->ItemCategories.Contains(EItemCategory::IC_Primary))
		{
			bw->GetItemMesh()->VisibilityBasedAnimTickOption =
				EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
			bw->GetItemMesh()->SetSimulatePhysics(true);
			bw->GetItemMesh()->SetLinearDamping(1.0f);
			bw->GetItemMesh()->SetAngularDamping(10000.0f);
			bw->GetItemMesh()->SetConstraintMode(EDOFMode::XZPlane);
			LoadAddAttachment(bw, Loadout.PrimaryScope);
			LoadAddAttachment(bw, Loadout.PrimaryMuzzle);
			LoadAddAttachment(bw, Loadout.PrimaryOverbarrel);
			LoadAddAttachment(bw, Loadout.PrimaryUnderbarrel);
			LoadAddAttachment(bw, Loadout.PrimaryStock);
			LoadAddAttachment(bw, Loadout.PrimaryGrip);
			LoadAddAttachment(bw, Loadout.PrimaryIlluminator);
			LoadAddAttachment(bw, Loadout.PrimaryAmmunition);

			if (Loadout.PrimarySkin)
			{
				USkinComponent* SkinComp = NewObject<USkinComponent>(bw, Loadout.PrimarySkin);
				if (SkinComp)
				{
					SkinComp->RegisterComponent();
				}
			}
			bw->AttachStatic();
		}
		else if (bw->ItemCategories.Contains(EItemCategory::IC_Secondary))
		{
			bw->GetItemMesh()->VisibilityBasedAnimTickOption =
				EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
			bw->GetItemMesh()->SetLinearDamping(1.0f);
			bw->GetItemMesh()->SetAngularDamping(10000.0f);
			bw->GetItemMesh()->SetSimulatePhysics(true);
			bw->GetItemMesh()->SetConstraintMode(EDOFMode::XZPlane);
			LoadAddAttachment(bw, Loadout.SecondaryScope);
			LoadAddAttachment(bw, Loadout.SecondaryMuzzle);
			LoadAddAttachment(bw, Loadout.SecondaryOverbarrel);
			LoadAddAttachment(bw, Loadout.SecondaryUnderbarrel);
			LoadAddAttachment(bw, Loadout.SecondaryStock);
			LoadAddAttachment(bw, Loadout.SecondaryGrip);
			LoadAddAttachment(bw, Loadout.SecondaryIlluminator);
			LoadAddAttachment(bw, Loadout.SecondaryAmmunition);

			if (Loadout.SecondarySkin)
			{
				USkinComponent* SkinComp = NewObject<USkinComponent>(bw, Loadout.SecondarySkin);
				if (SkinComp)
				{
					SkinComp->RegisterComponent();
				}
			}
			bw->AttachStatic();
		}
	}
	// }
	// }
}

void ULoadout_V2::SaveStoredWeaponAttachments()
{
	UBaseProfile* Profile = UBaseProfile::GetCurrentProfile();
	if (!Profile)
		return;

	Profile->LoadoutAttachmentSaveMap = LoadoutFunctionLibrary->StoredAttachmentsByWeapon;
	Profile->SaveProfile();
}

void ULoadout_V2::LoadStoredWeaponAttachments()
{
	UBaseProfile* Profile = UBaseProfile::GetCurrentProfile();
	if (!Profile)
		return;

	LoadoutFunctionLibrary->StoredAttachmentsByWeapon = Profile->LoadoutAttachmentSaveMap;
}

PRAGMA_ENABLE_OPTIMIZATION
