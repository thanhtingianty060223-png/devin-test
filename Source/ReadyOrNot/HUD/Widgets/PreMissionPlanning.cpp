// Copyright Void Interactive, 2022

#include "PreMissionPlanning.h"

#include "ReadyOrNotGameMode.h"
#include "Characters/ReadyOrNotPlayerController.h"
#include "Data/ItemData.h"
#include "Actors/Items/Quadrotor.h"
#include "Actors/Attachments/WeaponAttachment.h"
#include "Actors/BaseWeapon.h"
#include "lib/BpGameplayHelperLib.h"
#include "Actors/BaseGrenade.h"
#include "Characters/AI/SWATCharacter.h"
#include "Commander/BaseProfile.h"
#include "Components/InteractableComponent.h"
#include "GameModes/CoopGM.h"
#include "GameModes/LobbyGS.h"
#include "GameModes/TrainingGS.h"
#include "GameModes/TutorialGS.h"
#include "GameModes/VIPEscortGS.h"
#include "lib/ReadyOrNotFunctionLibrary.h"

#define LOCTEXT_NAMESPACE "PreMissionPlanning"

TAutoConsoleVariable<float> CVarRonEditorPremissionEquipAllAsMe(TEXT("a.RonEditorPremissionEquipAllAsMe"), 0, TEXT("Equip all characters in the premission as myself"));

UPreMissionPlanning::UPreMissionPlanning()
{
	if (LoadoutMusic == nullptr) 
	{
		ConstructorHelpers::FObjectFinder<UFMODEvent> LoadoutEvent(TEXT("FMODEvent'/Game/FMOD/Events/Music/Menus/Mus_Loadout.Mus_Loadout'"));
		LoadoutMusic = LoadoutEvent.Object;
	}
}

void UPreMissionPlanning::NativePreConstruct()
{
	Super::NativePreConstruct();


	FSavedLoadout Loadout;
	UBpGameplayHelperLib::LoadLoadout(Loadout, "default");

	if (GetWorld()->GetGameState<ATrainingGS>())
	{
		UBpGameplayHelperLib::LoadLoadout(Loadout, "training");
	}

	Init(false, Loadout);
	
	if(AGameStateBase* BGS = UGameplayStatics::GetGameState(this))
	{
		if(ATutorialGS* TutorialGS = Cast<ATutorialGS>(BGS))
		{
			Init(true, TutorialGS->CurrentTutorialData.Loadout);
		}
	}

	
}

void UPreMissionPlanning::NativeConstruct()
{
	Super::NativeConstruct();

	ALobbyGS* LobbyGS = GetWorld()->GetGameState<ALobbyGS>();
	if (!LobbyGS)
	{
		PlayLoadoutMusic();
	}
	pc = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld());
	gs = GetWorld()->GetGameState<AReadyOrNotGameState>();
	ps = Cast<AReadyOrNotPlayerState>(pc->PlayerState);

}

void UPreMissionPlanning::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!gs)
		return;
	
	if (!pc)
		return;

	if (LoadoutEventInst.Instance)
	{
		bool bEveryoneReady = gs->TimeTillGameStartCountdown < 5.0f;
		LoadoutEventInst.Instance->setParameterByName("Loadout_StartGame", bEveryoneReady ? 1.0f : 0.0f);
	}

	if (!pc->PlayerCameraManager)
	{
		pc->SpawnPlayerCameraManager();
	}

	if (!ps)
		return;

	if (!EquippingPlayerState)
	{
		EquippingPlayerState = ps;
	}

	for (auto k : WorkBenchItemPtrMap)
	{
		if (k.Value)
		{
			FHitResult Hit;
			GetWorld()->LineTraceSingleByObjectType(Hit, k.Value->GetItemMesh()->GetComponentLocation(), k.Value->GetItemMesh()->GetComponentLocation() + FVector(0.0f, 0.0f, -3.0f), FCollisionObjectQueryParams(ECC_WorldStatic));
			if (Hit.bBlockingHit)
			{
				k.Value->GetItemMesh()->SetSimulatePhysics(false);
			}
		}
	}


	if (EquippingSwatMember != EEquippingSwat::ES_None)
	{
		if (UReadyOrNotStatics::GetReadyOrNotGameMode()
            && UReadyOrNotStatics::GetReadyOrNotGameMode()->GetNumPlayers() > 1)
		{
			EquippingSwatMember = EEquippingSwat::ES_None;
		}
	}

	PreMissionStreamedLevel = gs->PreMissionStreamedLevel;
	if (PreMissionStreamedLevel && !bIsWeaponCustomization)
	{
		if (PreMissionStreamedLevel->GetCurrentState() != ULevelStreaming::ECurrentState::LoadedVisible)
		{
			if (!bDestructing)
			{
				V_LOGM(LogReadyOrNot, "Found Streaming Level %s", *PreMissionStreamedLevel->GetWorldAssetPackageName());
				V_LOGM(LogReadyOrNot, "Showing %s", *PreMissionStreamedLevel->GetWorldAssetPackageName());
				PreMissionStreamedLevel->SetShouldBeVisible(true);
				PreMissionStreamedLevel->SetShouldBeLoaded(true);
			}
			return;
		}
		if (PreMissionStreamedLevel->GetLoadedLevel())
		{
			PreMissionStreamedLevel->GetLoadedLevel()->bClientOnlyVisible = true;
		}

		bool bCameraInStreamingLevel = false;
		if (PreMissionStreamedLevel->GetCurrentState() == ULevelStreaming::ECurrentState::LoadedVisible)
		{
			if (pc->GetViewTarget() && PreMissionStreamedLevel->GetLoadedLevel() && PreMissionStreamedLevel->GetLoadedLevel()->Actors.Contains(pc->GetViewTarget()))
			{
				bHasCameraEverBeenInStreamingLevel = true;
				bCameraInStreamingLevel = true;
			}
		}

		if (!bCameraInStreamingLevel && !bHasCameraEverBeenInStreamingLevel)
		{
			V_LOGM(LogReadyOrNot, "Camera not in stream level.. resetting camera! %s", *pc->GetName());
		}

		if (!bDestructing && (ActiveCameraTag == NAME_None || (!bCameraInStreamingLevel && !bHasCameraEverBeenInStreamingLevel)))
		{
			pc->bDisableCameraShakes = true;
			// TrySetPlayerCamera();
		}


		if (bCameraInStreamingLevel)
		{
		
			UpdateFriendlyLoadouts();
			if (!bSetInitialLoadout)
			{
				bSetInitialLoadout = true;
				UpdatePreviewCharacter(pc->GetRoNPlayerState(), "PreviewCharacter");
				HidePrimary(true);
				HideSecondary(true);

				if (GetDefaultPreviewCharacter() && GetDefaultPreviewCharacter()->GetInventoryComponent())
					GetDefaultPreviewCharacter()->GetInventoryComponent()->ClearEquippedItem();
			}
		}
	}
	
	TotalSeconds += InDeltaTime;

	ACameraActor* CameraViewTarget = Cast<ACameraActor>(pc->GetViewTarget());
	if (CameraViewTarget)
	{
		for (FName Tag : CameraViewTarget->Tags)
		{
			float* OriginalFov = CameraTagOriginalFovMap.Find(Tag);
			if (OriginalFov)
			{
				CameraViewTarget->GetCameraComponent()->FieldOfView = UReadyOrNotFunctionLibrary::GetInterfaceFovOffset(*OriginalFov);
			}
		}
	} else
	{
		bHasCameraEverBeenInStreamingLevel = false;
	}

	if (!bLoadedLoadout)
	{

		// load / santize / save all swat member loadouts just in case the data has changed
		
		FSavedLoadout SwatTeamLoadout;
		UBpGameplayHelperLib::LoadLoadout(SwatTeamLoadout, "defaultblueone");
		GetWorld()->GetTimerManager().SetTimer(TH_DoSaveLoadout, FTimerDelegate::CreateUObject(this, &UPreMissionPlanning::DoSaveLoadout, EEquippingSwat::ES_BlueOne, SwatTeamLoadout), 0.5f, false);
		UBpGameplayHelperLib::LoadLoadout(SwatTeamLoadout, "defaultbluetwo");
		GetWorld()->GetTimerManager().SetTimer(TH_DoSaveLoadout, FTimerDelegate::CreateUObject(this, &UPreMissionPlanning::DoSaveLoadout, EEquippingSwat::ES_BlueTwo, SwatTeamLoadout), 0.5f, false);
		UBpGameplayHelperLib::LoadLoadout(SwatTeamLoadout, "defaultredone");
		GetWorld()->GetTimerManager().SetTimer(TH_DoSaveLoadout, FTimerDelegate::CreateUObject(this, &UPreMissionPlanning::DoSaveLoadout, EEquippingSwat::ES_RedOne, SwatTeamLoadout), 0.5f, false);
		UBpGameplayHelperLib::LoadLoadout(SwatTeamLoadout, "defaultredtwo");
		GetWorld()->GetTimerManager().SetTimer(TH_DoSaveLoadout, FTimerDelegate::CreateUObject(this, &UPreMissionPlanning::DoSaveLoadout, EEquippingSwat::ES_RedTwo, SwatTeamLoadout), 0.5f, false);
		if(bCanUpdateWithUI)
			bLoadedLoadout = UBpGameplayHelperLib::LoadLoadout(ActiveLoadout, "default");
		else
			bLoadedLoadout = true;

		UBpGameplayHelperLib::SanitizeLoadout(ActiveLoadout);
		
		SaveActiveLoadout();
		
		OnLoadoutLoaded();
	}

	bool bUpdateVIP = false;
	
	AVIPEscortGS* vipgs = Cast<AVIPEscortGS>(GetWorld()->GetGameState());
	if (vipgs)
	{
		if (ps == vipgs->VIPPlayerState && !bWasVIP)
		{
			bWasVIP = true;
			bUpdateVIP = true;
		} else if (bWasVIP && ps != vipgs->VIPPlayerState)
		{
			bWasVIP = false;
			bUpdateVIP = true;
		}
	}

	bool bUpdateDifferentLoadoutWithServer = true;
	if (vipgs)
	{
		if (ps == vipgs->VIPPlayerState)
		{
			bUpdateDifferentLoadoutWithServer = false;
		}
	}	
	
	if (EquippingSwatMember == EEquippingSwat::ES_None
		&& bUpdateDifferentLoadoutWithServer
		&& ps->GetLoadout() != ActiveLoadout)
	{
		ActiveLoadout.CharacterType = "SWATJudge";
		SaveActiveLoadout();
	}
	
	if (OurSpawnedTeamType != ps->GetTeam() || bUpdateVIP)
	{
		
		OurSpawnedTeamType = ps->GetTeam();
		UBpGameplayHelperLib::SanitizeLoadout(ActiveLoadout);
		UpdateTeamVisuals(ps);
		if (bUpdateVIP && bWasVIP)
		{
			SetBodyArmour(nullptr);
		} else
		{
			SetBodyArmour(ActiveLoadout.Armor);
		}
		UReadyOrNotSaveGame* lg = UBpGameplayHelperLib::GetLoadGameInstance();
		if (lg)
		{
			if (lg->SkinSaveMap.Find(OurSpawnedTeamType))
			{
				SetPlayerSkin(*lg->SkinSaveMap.Find(OurSpawnedTeamType));
			}
		}
		bUpdateVIP = false;
		OnRequestRefresh.Broadcast();
	}

	AReadyOrNotCharacter* Quartermaster = GetQuartermaster();
	if (Quartermaster)
	{
		if (!Quartermaster->Is3PMontagePlaying(nullptr))
		{
			Quartermaster->PlayMontageFromTable("tp_quartermaster_idle");
		}
	}
}

void UPreMissionPlanning::NativeDestruct()
{
	Super::NativeDestruct();

	if(bDestructing)
		return;

	StopLoadoutMusic();

	LoadWeaponAttachments();
	
	if (bIsWeaponCustomization)
	{
		LoadWeaponPresets();
		
		
		FWeaponPreset Preset;
		Preset.PresetName = "Weapon Table";
		Preset.bSelected = false;
		Preset.bHasSavedData = true;
		Preset.AttachmentData.bHasSavedData = true;
		
		Preset.AttachmentData.ScopeAttachment = bIsCustomizingPrimary ? ActiveLoadout.PrimaryScope : ActiveLoadout.SecondaryScope;
		Preset.AttachmentData.MuzzleAttachment = bIsCustomizingPrimary ? ActiveLoadout.PrimaryMuzzle : ActiveLoadout.SecondaryMuzzle;
		Preset.AttachmentData.UnderbarrelAttachment = bIsCustomizingPrimary ? ActiveLoadout.PrimaryUnderbarrel : ActiveLoadout.SecondaryUnderbarrel;
		Preset.AttachmentData.OverbarrelAttachment = bIsCustomizingPrimary ? ActiveLoadout.PrimaryOverbarrel : ActiveLoadout.SecondaryOverbarrel;
		Preset.AttachmentData.StockAttachment = bIsCustomizingPrimary ? ActiveLoadout.PrimaryStock: ActiveLoadout.SecondaryStock;
		Preset.AttachmentData.GripAttachment = bIsCustomizingPrimary ? ActiveLoadout.PrimaryGrip : ActiveLoadout.SecondaryGrip;
		Preset.AttachmentData.IlluminatorAttachment = bIsCustomizingPrimary ? ActiveLoadout.PrimaryIlluminator : ActiveLoadout.SecondaryIlluminator;
		Preset.AttachmentData.AmmunitionAttachment = bIsCustomizingPrimary ? ActiveLoadout.PrimaryAmmunition : ActiveLoadout.SecondaryAmmunition;
		Preset.AttachmentData.Skin = bIsCustomizingPrimary ? ActiveLoadout.PrimarySkin : ActiveLoadout.SecondarySkin;

		UItemData* Id = UBpGameplayHelperLib::GetItemData(GetWorld());
		if (!Preset.AttachmentData.ScopeAttachment)
			Preset.AttachmentData.ScopeAttachment = Id->NullPrimaryScopeAttachment;

		if (!Preset.AttachmentData.MuzzleAttachment)
			Preset.AttachmentData.MuzzleAttachment = Id->NullMuzzleAttachment;

		if (!Preset.AttachmentData.UnderbarrelAttachment)
			Preset.AttachmentData.UnderbarrelAttachment = Id->NullUnderbarrelAttachment;

		if (!Preset.AttachmentData.OverbarrelAttachment)
			Preset.AttachmentData.OverbarrelAttachment = Id->NullOverbarrelAttachment;

		if (!Preset.AttachmentData.StockAttachment)
			Preset.AttachmentData.StockAttachment = Id->NullStockAttachment;

		if (!Preset.AttachmentData.GripAttachment)
			Preset.AttachmentData.GripAttachment = Id->NullGripAttachment;

		if (!Preset.AttachmentData.IlluminatorAttachment)
			Preset.AttachmentData.IlluminatorAttachment = Id->NullIlluminatorAttachment;
		
		if (!Preset.AttachmentData.AmmunitionAttachment)
			Preset.AttachmentData.AmmunitionAttachment = Id->NullAmmunitionAttachment;

		UpdateWeaponPreset(CustomizeItemClass, Preset, 4);

		for (auto k : WorkBenchItemPtrMap)
		{
			if (ABaseItem* DereferencedItem = k.Value)
			{
				DereferencedItem->Destroy();
				DereferencedItem = nullptr;
			}
		}
		WeaponToAttachmentsMap.Add(bIsCustomizingPrimary ? ActiveLoadout.Primary : ActiveLoadout.Secondary, Preset.AttachmentData);
		SaveWeaponAttachments();
		SaveWeaponPresets();
		SaveWeaponDefaultFireMode();
		
	}
	
	bDestructing = true;

#if UE_BUILD_SHIPPING
	if (IsInLobby())
#endif
	{
		DoSaveLoadout(EquippingSwatMember, ActiveLoadout);
		AttemptEquipLoadoutInGame();
	}

	// Try delete any items
	TArray<AActor*> OutActors;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), "CustomizationMenu", OutActors);
	for (AActor* a : OutActors)
	{
		V_LOGM(LogReadyOrNot, "Destroying %s", *a->GetName());
		a->Destroy();
	}

	for (auto k : WorkBenchItemPtrMap)
	{
		if (k.Value)
		{
			k.Value->Destroy();
		}
	}


	if (PreMissionStreamedLevel)
	{
		
		
		V_LOGM(LogReadyOrNot, "Hiding %s", *PreMissionStreamedLevel->GetWorldAssetPackageName());
		// destroy any items that we created during premission

		if(PreMissionStreamedLevel->GetLoadedLevel())
		{
			for (TActorIterator<ABaseItem>It(GetWorld()); It; ++It)
			{
				ABaseItem* Item = *It;
				for (int32 i = 0; i < PreMissionStreamedLevel->GetLoadedLevel()->Actors.Num(); i++)
				{
					if (Item->GetOwner() == PreMissionStreamedLevel->GetLoadedLevel()->Actors[i])
					{
						Item->Destroy();
					}
				}
			}
		}
		// Destroy any mission audio that is still playing
		//FLevelDataLookupTable LevelData = UBpGameplayHelperLib::GetLevelData();
		//for (TObjectIterator<UAudioComponent>It; It; ++It)
		//{
		//	UAudioComponent* AudioComponent = *It;
		//	if (!AudioComponent->Sound)
		//		continue;
		//	if (AudioComponent->Sound == LevelData.TocBriefingAudio.SoundFile)
		//	{
		//		AudioComponent->Stop();
		//		AudioComponent->GetOwner()->Destroy();
		//		continue;
		//	}
		//	for (int32 i = 0; i < LevelData.MissionAudio.Num(); i++)
		//	{
		//		if (AudioComponent->Sound == LevelData.MissionAudio[i].SoundFile)
		//		{
		//			AudioComponent->Stop();
		//			AudioComponent->GetOwner()->Destroy();
		//		}
		//	}
		//}
		
		PreMissionStreamedLevel->SetShouldBeVisible(false);
		PreMissionStreamedLevel->SetShouldBeLoaded(false);
		bDestructing = false;
		
		//GetWorld()->RemoveStreamingLevel(PreMissionStreamedLevel);
		//PreMissionStreamedLevel = nullptr;
		//UReadyOrNotStatics::GetReadyOrNotGameState()->PreMissionStreamedLevel = nullptr;
	}

	

	
	if (pc)
	{
		pc->SetViewTarget(pc->GetPawn());

		if (!bIsWeaponCustomization)
		{
			if (pc->PlayerCameraManager)
			{
				pc->PlayerCameraManager->StartCameraFade(3.0f, 0.0f, 1.0f, FLinearColor::Black, false, false); //changed bHoldWhenFinished to false for fix to bug 107540 
			} else
			{
				// NO Player Camera Managr???
				V_LOGM(LogReadyOrNot, "NO CAMERA MANAGER.. PREMISSION PLANNING WILL NOT WORK");
			}
		}
		
		pc->bDisableCameraShakes = false;
	}

	if (gs == GetWorld()->GetGameState<AReadyOrNotGameState>())
	{
		gs->bInPlanningMenu = false;
	}

}

void UPreMissionPlanning::Init(bool bReadOnly, FSavedLoadout PreviewLoadout)
{
	bCanUpdateWithUI = !bReadOnly;
	if (PreviewLoadout.IsValid())
	{
		ActiveLoadout = PreviewLoadout;
	}
}

bool UPreMissionPlanning::IsInLobby()
{
	if (!GetWorld())
		return false;
	
	FString LobbyLevel = GetGameInstance<UReadyOrNotGameInstance>()->LobbyLevel;
	return GetWorld()->GetMapName().Contains(LobbyLevel);
}

UPreMissionPlanning* UPreMissionPlanning::GetPremissionPlanning()
{
	for (TObjectIterator<UPreMissionPlanning> Itr; Itr; ++Itr)
	{
		UPreMissionPlanning* LiveWidget = *Itr;

		/* If the Widget has no World, Ignore it (It's probably in the Content Browser!) */
		if (!LiveWidget->GetWorld() || LiveWidget->GetWorld() != UBpGameplayHelperLib::GetWorldStatic() || !LiveWidget->IsInViewport())
		{
			continue;
		}
		

		return LiveWidget;
	}
	return nullptr;
}

void UPreMissionPlanning::PlayLoadoutMusic()
{
	LoadoutEventInst = UFMODBlueprintStatics::PlayEvent2D(GetWorld(), LoadoutMusic, true);
}

void UPreMissionPlanning::StopLoadoutMusic()
{
	UFMODBlueprintStatics::EventInstanceStop(LoadoutEventInst);
}

void UPreMissionPlanning::GetSubPremissionPlanningActors(TArray<AActor*>& OutActors)
{
	if (PreMissionStreamedLevel)
	{
		if (PreMissionStreamedLevel->GetLoadedLevel())
		{
			OutActors = PreMissionStreamedLevel->GetLoadedLevel()->Actors;
		}
	}
}

void UPreMissionPlanning::SetEquippingSwatMember(EEquippingSwat NewEquippingSwat, AReadyOrNotPlayerState* NewEquippingPlayerState)
{
	/*if (EquippingSwatMember == NewEquippingSwat)
		return;*/
	if (!pc)
		return;

	if (!ps)
		return;

	DoSaveLoadout(EquippingSwatMember, ActiveLoadout);
	EquippingSwatMember = NewEquippingSwat;
	if (NewEquippingPlayerState)
	{
		EquippingPlayerState = NewEquippingPlayerState;
	}
	else
	{
		EquippingPlayerState = ps;
	}
	switch (EquippingSwatMember)
	{
	case EEquippingSwat::ES_None:
		//Get remote player serve loadout for previewing other players in multiplayer
		if (ps != EquippingPlayerState)
		{
			ActiveLoadout = EquippingPlayerState->ServerSavedLoadout;
		}else
		{
			UBpGameplayHelperLib::LoadLoadout(ActiveLoadout, "default");
		}
		break;
	case EEquippingSwat::ES_BlueOne:
		UBpGameplayHelperLib::LoadLoadout(ActiveLoadout, "defaultblueone");
		ActiveLoadout.CharacterLookOverride = "SWAT_King";
		break;
	case EEquippingSwat::ES_BlueTwo:
		UBpGameplayHelperLib::LoadLoadout(ActiveLoadout, "defaultbluetwo");
		ActiveLoadout.CharacterLookOverride = "SWAT_Swan";
		break;
	case EEquippingSwat::ES_RedOne:
		UBpGameplayHelperLib::LoadLoadout(ActiveLoadout, "defaultredone");
		ActiveLoadout.CharacterLookOverride = "SWAT_Prescott";
		break;
	case EEquippingSwat::ES_RedTwo:
		UBpGameplayHelperLib::LoadLoadout(ActiveLoadout, "defaultredtwo");
		ActiveLoadout.CharacterLookOverride = "SWAT_Eli";
		break;
	default: ;
	}
	
	if (GetPreviewCharacter("PreviewCharacter"))
		GetPreviewCharacter("PreviewCharacter")->UpdateOverridesFromCharacterLookOverrideDataTable(ActiveLoadout.CharacterLookOverride);
	
	LastSavedLoadout.Add(NewEquippingSwat, ActiveLoadout);
	UBpGameplayHelperLib::SanitizeLoadout(ActiveLoadout);
	FSavedLoadout TmpLoadout = ActiveLoadout;

	if (GetPreviewCharacter("PreviewCharacter"))
	{
		GetPreviewCharacter("PreviewCharacter")->PlayMontageFromTable("tp_pregame_idle");
		if (GetPreviewCharacter("PreviewCharacter")->IsTableMontagePlaying("tp_pregame_idle"))
		{
			TmpLoadout.Primary = nullptr;
		}
	}

	FLoadoutEquipOptions LoadoutEquipOptions;
	LoadoutEquipOptions.bReplicates = false;
	LoadoutEquipOptions.bSanitizeLoadout = false;
	UBpGameplayHelperLib::EquipLoadoutOnPlayer(TmpLoadout, GetPreviewCharacter("PreviewCharacter"), LoadoutEquipOptions);
	OnLoadoutLoaded();
	OnSwatCharacterChanged();
}

void UPreMissionPlanning::SaveActiveLoadout()
{
	if ((EquippingSwatMember == EEquippingSwat::ES_None) && ps == EquippingPlayerState)
	{
		if (ps)
		{
			if (ps->LastLoadout != ActiveLoadout)
			{
				ps->Server_SetLoadout(ActiveLoadout);
				ps->Server_SetLoadout_Implementation(ActiveLoadout);
				OnLoadoutSaved();
			}
		}
	}
	else
	{
		if (const FSavedLoadout* FoundLoadout = LastSavedLoadout.Find(EquippingSwatMember))
		{
			if (!(*FoundLoadout == ActiveLoadout))
			{
				GetWorld()->GetTimerManager().SetTimer(TH_DoSaveLoadout, FTimerDelegate::CreateUObject(this, &UPreMissionPlanning::DoSaveLoadout, EquippingSwatMember, ActiveLoadout), 0.5f, false);
				OnLoadoutSaved();
			}
		}
	}


}

void UPreMissionPlanning::DoSaveLoadout(EEquippingSwat SwatMember, FSavedLoadout Loadout)
{
	UBpGameplayHelperLib::SanitizeLoadout(Loadout);
	if (ActiveLoadout.Primary && ActiveLoadout.Secondary)
	{
		V_LOGM(LogReadyOrNot, "Saving loadout Primary %s Secondary %s %d", *ActiveLoadout.Primary->GetName(), *ActiveLoadout.Secondary->GetName(), SwatMember)
	}
	switch (EquippingSwatMember)
	{
	case EEquippingSwat::ES_None:
		//Make sure not to save remote player loadout to self
		if (ps != EquippingPlayerState)
		{
			break;
		}
		else
		{
			UBpGameplayHelperLib::SaveLoadout(ActiveLoadout, "default");
		}
		break;
	case EEquippingSwat::ES_BlueOne:
			
		UBpGameplayHelperLib::SaveLoadout(ActiveLoadout, "defaultblueone");
		break;
	case EEquippingSwat::ES_BlueTwo:
		UBpGameplayHelperLib::SaveLoadout(ActiveLoadout, "defaultbluetwo");
		break;
	case EEquippingSwat::ES_RedOne:
		UBpGameplayHelperLib::SaveLoadout(ActiveLoadout, "defaultredone");
		break;
	case EEquippingSwat::ES_RedTwo:
		UBpGameplayHelperLib::SaveLoadout(ActiveLoadout, "defaultredtwo");
		break;
	default: ;
	}
}

void UPreMissionPlanning::TrySetPlayerCamera()
{
	if (bIsWeaponCustomization)
		return;
	
	if (pc)
	{
		if (ps)
		{
			if (pc)
			{
				for (TActorIterator<ACameraActor> It(GetWorld()); It; ++It)
				{
					ACameraActor* c = *It;
					if (c->Tags.Contains("CharacterViewAppearance"))
					{
						if (!CameraTagOriginalFovMap.Contains("CharacterViewAppearance"))
						{
							CameraTagOriginalFovMap.Add("CharacterViewAppearance", c->GetCameraComponent()->FieldOfView);
						}
						if (GetDefaultPreviewCharacter())
						{
							if (pc->GetViewTarget() != c)
							{
								HideWeapons(true, true);
								
								UpdatePreviewCharacter(Cast<AReadyOrNotPlayerState>(pc->PlayerState));
								HidePrimary(true);
								HideSecondary(true);
								GetDefaultPreviewCharacter()->GetInventoryComponent()->ClearEquippedItem();
							}
							if (!bPlayInitialCameraFade)
							{
								if (pc && pc->PlayerCameraManager)
								{
									bPlayInitialCameraFade = true;
								}
							}
							FViewTargetTransitionParams TransitionParams;
							TransitionParams.BlendTime = 0.0f;
							TransitionParams.bLockOutgoing = true;
							pc->SetViewTarget(c, TransitionParams);
						}
						//V_LOGM(LogReadyOrNot, "Camera Tag: %s", *c->GetName());

					}
				}
			}
			
		}
	}
}

class AReadyOrNotCharacter* UPreMissionPlanning::GetPreviewCharacter(FName Tag)
{
	for (TActorIterator<AReadyOrNotCharacter>It(GetWorld()); It; ++It)
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

void UPreMissionPlanning::UpdatePreviewCharacter(class AReadyOrNotPlayerState* InPreviewPlayerState, FName Tag)
{
	if (bIsWeaponCustomization)
		return;
	
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
		UBpGameplayHelperLib::SanitizeLoadout(ActiveLoadout);

		if (!InPreviewPlayerState->IsVipPlayerState())
		{
			FSavedLoadout Temp = ActiveLoadout;
			Temp.Primary = nullptr;
			Temp.Secondary = nullptr;
			
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
			}
			
		}
		GetPreviewCharacter(Tag)->PlayMontageFromTable("tp_pregame_idle");
	}
	else
	{
		FSavedLoadout LastLoadout = GetPreviewCharacter(Tag)->GetInventoryComponent()->GetLastEquippedLoadout();
		FSavedLoadout NewLoadout = InPreviewPlayerState->GetLoadout();
		PlayerStatePreviewMap.Add(GetPreviewCharacter(Tag), InPreviewPlayerState);
		FSavedLoadout Loadout = InPreviewPlayerState->GetLoadout();
		bool bUpdateLoadout = false;
		if (GetPreviewCharacter(Tag)->GetInventoryComponent()->GetLastEquippedLoadout() != InPreviewPlayerState->GetLoadout())
		{
			bUpdateLoadout = true;
		}

		AReadyOrNotCharacter* PreviewCharacter = GetPreviewCharacter(Tag);
		if (bUpdateLoadout && PreviewCharacter)
		{
			FLoadoutEquipOptions LoadoutEquipOptions;
			LoadoutEquipOptions.bReplicates = false; 
			LoadoutEquipOptions.EquipItemCategory = EItemCategory::IC_Primary;
			LoadoutEquipOptions.OverridePlayerState = InPreviewPlayerState;
			UBpGameplayHelperLib::EquipLoadoutOnPlayer(Loadout, PreviewCharacter, LoadoutEquipOptions);
		}
		
		if (PreviewCharacter && (PreviewCharacter->CustomizationSkeletalMeshes.Num() <= 0 || InPreviewPlayerState->bResetPremissionCustomization))
		{
			InPreviewPlayerState->bResetPremissionCustomization = false;
			
			FSavedCustomization Customization = InPreviewPlayerState->Customization;
			Customization.Sanitize();

			PreviewCharacter->Customization = Customization;
			Customization.ApplyCustomization(PreviewCharacter);
			Customization.ApplyCustomizationSkins(PreviewCharacter);
		}
	}
		
}

void UPreMissionPlanning::UpdateTeamVisuals(class AReadyOrNotPlayerState* InPreviewPlayerState, FName Tag /*= "PreviewCharacter"*/)
{
	AReadyOrNotCharacter* PreviewCharacter = GetPreviewCharacter(Tag);
	if (!PreviewCharacter)
		return;

	if (gs->bPvPMode)
	{
		AVIPEscortGS* vipgs = Cast<AVIPEscortGS>(gs);
		if (vipgs)
		{
			if (InPreviewPlayerState == vipgs->VIPPlayerState)
			{
				AReadyOrNotCharacter* VIPTemplate = VIPClass.LoadSynchronous()->GetDefaultObject<AReadyOrNotCharacter>();
				PreviewCharacter->GetMesh()->SetSkeletalMesh(VIPTemplate->GetMesh()->SkeletalMesh, false);
				PreviewCharacter->GetMesh()->EmptyOverrideMaterials();
				PreviewCharacter->GetFaceMesh()->SetSkeletalMesh(nullptr, false);
				if (ABaseItem* Secondary = PreviewCharacter->GetInventoryComponent()->GetSpawnedGear().Secondary)
				{
					Secondary->GetItemMesh()->AttachToComponent(PreviewCharacter->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, Secondary->HandsSocket);
				}
				
				return;
			}
		}

		AReadyOrNotCharacter* ClassTemplate = InPreviewPlayerState->GetTeam() == ETeamType::TT_SERT_RED ? RedTeamClass.LoadSynchronous()->GetDefaultObject<AReadyOrNotCharacter>() : BlueTeamClass.LoadSynchronous()->GetDefaultObject<AReadyOrNotCharacter>();
		FString MeshName = (ClassTemplate->GetMesh()->SkeletalMesh ? ClassTemplate->GetMesh()->SkeletalMesh->GetName() : "NULL REFERENCE");
		//GEngine->AddOnScreenDebugMessage(7781, 30.0f, FColor::White, "SkeletalMesh: " + MeshName);
		PreviewCharacter->GetMesh()->SetSkeletalMesh(ClassTemplate->GetMesh()->SkeletalMesh, false);
		PreviewCharacter->GetFaceMesh()->SetSkeletalMesh(ClassTemplate->GetFaceMesh()->SkeletalMesh, false);
		class TSubclassOf<UAnimInstance> TemplateAnimClass = ClassTemplate->GetFaceMesh()->AnimClass;
		PreviewCharacter->GetFaceMesh()->SetAnimInstanceClass(TemplateAnimClass);
		PreviewCharacter->GetMesh()->EmptyOverrideMaterials();
		PreviewCharacter->SetTPMeshOverrideMap(ClassTemplate->GetTPMeshOverrideMap());
		USkinComponent* skin = Cast<USkinComponent>(PreviewCharacter->GetComponentByClass(USkinComponent::StaticClass()));
		if (skin && skin->PreAppliedSkeletalMeshMap.Num() > 0)
		{
			skin->ResetSkin();
		}
		
	} 

}

void UPreMissionPlanning::UpdateFriendlyLoadouts()
{
	if (bIsWeaponCustomization)
		return;
	
	if (UReadyOrNotStatics::GetReadyOrNotGameMode() && UReadyOrNotStatics::GetReadyOrNotGameMode()->GetNumPlayers() == 1)
	{
		if (CVarRonEditorPremissionEquipAllAsMe.GetValueOnAnyThread() == 0)
		{
			UpdateSwatTeamLoadouts();
			return;
		}
		
	}
	RemoveDuplicates();
	
	for (TActorIterator<AReadyOrNotCharacter> It(GetWorld()); It; ++It)
	{
		AReadyOrNotCharacter* pchar = *It;
		if (pchar->Tags.Contains("FriendlyPreviewCharacter"))
		{
			if (!PlayerStatePreviewMap.Find(pchar))
			{
				PlayerStatePreviewMap.Add(pchar, nullptr);
			}
		}
	}

	for (auto& elem : PlayerStatePreviewMap)
	{
#if WITH_EDITOR
		if (CVarRonEditorPremissionEquipAllAsMe.GetValueOnAnyThread() == 1)
		{
			continue;
		}
#endif
		AReadyOrNotCharacter* pchar = Cast<AReadyOrNotCharacter>(elem.Key);
		AReadyOrNotPlayerState* pstate = Cast<AReadyOrNotPlayerState>(elem.Value);
		if (pstate && pstate->GetTeam() != OurSpawnedTeamType)
		{
			PlayerStatePreviewMap[pchar] = nullptr;
		}
		if (!elem.Value && pchar)
		{
			if (pchar)
			{
				pchar->SetActorHiddenInGame(true);
				TArray<USkeletalMeshComponent*> SkeletalMeshComponents;
				pchar->GetComponents(SkeletalMeshComponents);
				for (UActorComponent* a : SkeletalMeshComponents)
				{
					USkeletalMeshComponent* s = Cast<USkeletalMeshComponent>(a);
					s->SetCastShadow(false);
				}
				for (ABaseItem* b : pchar->GetInventoryComponent()->GetInventoryItems())
				{
					if (b)
					{
						if (!IsValid(b) || b->IsUnreachable())
						{
							b->Destroy();
						}
					}

				}
				pchar->GetInventoryComponent()->GetInventoryItems().Empty();
			}
		}
		else if (pchar)
		{
			pchar->SetActorHiddenInGame(false);
			TArray<USkeletalMeshComponent*> SkeletalMeshComponents;
			pchar->GetComponents(SkeletalMeshComponents);
			for (UActorComponent* a : SkeletalMeshComponents)
			{
				USkeletalMeshComponent* s = Cast<USkeletalMeshComponent>(a);
				s->SetCastShadow(true);
			}
		}
	}
	
	if (pc)
	{
		if (ps)
		{
			for (TActorIterator<AReadyOrNotCharacter>It(GetWorld()); It; ++It)
			{
				if (It->Tags.Contains("FriendlyPreviewCharacter") && !It->Tags.Contains("MyPlayerPreview"))
				{
					It->GetMesh()->bPauseAnims = !ps->bHasFinishedLoading;
				}
			}
			UpdatePreviewCharacter(ps, "MyPlayerPreview");
#if WITH_EDITOR
			if (CVarRonEditorPremissionEquipAllAsMe.GetValueOnAnyThread() == 1)
			{
				for (int32 i = 1; i < 5; i++)
				{
					FString TagAsString = "PlayerPreview" + FString::FromInt(i);
					FName Tag = FName(*TagAsString);
					UpdatePreviewCharacter(ps, Tag);
				}
				return;
			}
			
#endif
			int32 Count = 1;
			for (TActorIterator<AReadyOrNotPlayerState> It(GetWorld()); It; ++It)
			{
				AReadyOrNotPlayerState* pchar = *It;

				if (pchar->GetTeam() == ps->GetTeam() && pchar != ps)
				{
					FString TagAsString = "PlayerPreview" + FString::FromInt(Count);
					FName Tag = FName(*TagAsString);
					UpdatePreviewCharacter(pchar, Tag);
					Count++;
				}
			}
		}
	}
}

void UPreMissionPlanning::UpdateSwatTeamLoadouts()
{
	if (bIsWeaponCustomization)
		return;
	
	TArray<AReadyOrNotCharacter*> PreviewCharacters;
	for (TActorIterator<AReadyOrNotCharacter> It(GetWorld()); It; ++It)
	{
		AReadyOrNotCharacter* pchar = *It;
		if (pchar->Tags.Contains("FriendlyPreviewCharacter"))
		{
			PreviewCharacters.Add(pchar);
		}
	}
	for (int32 i = 0; i < PreviewCharacters.Num(); i++)
	{
		EEquippingSwat SwatEquipIdx = (EEquippingSwat)i;
		if (i == 0)
		{
			if (pc && ps)
			{
				UpdatePreviewCharacter(ps, "MyPlayerPreview");
			}
		}
		if (i > 0 && PreviewCharacters.IsValidIndex(i) && i <= (uint8)EEquippingSwat::ES_RedTwo)
		{
			bool bNeedsEquip = false;
			if (!LastEquippedPreviewLoadout.Find(SwatEquipIdx))
			{
				FSavedLoadout TmpLoadout;
				switch (SwatEquipIdx)
				{
				case EEquippingSwat::ES_None:
					UBpGameplayHelperLib::LoadLoadout(TmpLoadout, "default");
					break;
				case EEquippingSwat::ES_BlueOne:
					UBpGameplayHelperLib::LoadLoadout(TmpLoadout, "defaultblueone");
					break;
				case EEquippingSwat::ES_BlueTwo:
					UBpGameplayHelperLib::LoadLoadout(TmpLoadout, "defaultbluetwo");
					break;
				case EEquippingSwat::ES_RedOne:
					UBpGameplayHelperLib::LoadLoadout(TmpLoadout, "defaultredone");
					break;
				case EEquippingSwat::ES_RedTwo:
					UBpGameplayHelperLib::LoadLoadout(TmpLoadout, "defaultredtwo");
					break;
				default: ;
				}
				LastSavedLoadout.Add(SwatEquipIdx, TmpLoadout);
				LastEquippedPreviewLoadout.Add(SwatEquipIdx, FSavedLoadout());
				bNeedsEquip = true;
			}
			if (!bNeedsEquip)
			{
				bNeedsEquip = *LastEquippedPreviewLoadout.Find(SwatEquipIdx) != *LastSavedLoadout.Find(SwatEquipIdx);
			}

			if (bNeedsEquip)
			{
				PreviewCharacters[i]->SetActorHiddenInGame(false);
				TArray<USkeletalMeshComponent*> SkeletalMeshComponents;
				PreviewCharacters[i]->GetComponents(SkeletalMeshComponents);
				for (UActorComponent* a : SkeletalMeshComponents)
				{
					USkeletalMeshComponent* s = Cast<USkeletalMeshComponent>(a);
					s->SetCastShadow(true);
				}
				LastEquippedPreviewLoadout.Add(SwatEquipIdx, *LastSavedLoadout.Find(SwatEquipIdx));
				FLoadoutEquipOptions LoadoutEquipOptions;
				LoadoutEquipOptions.bReplicates = false;
				UBpGameplayHelperLib::EquipLoadoutOnPlayer(*LastEquippedPreviewLoadout.Find(SwatEquipIdx),
					PreviewCharacters[i], LoadoutEquipOptions);
				PreviewCharacters[i]->GetInventoryComponent()->ClearEquippedItem();
				
				FSavedCustomization Customization;
				{
					UBaseProfile* BaseProfile = UBaseProfile::GetCurrentProfile();
					if (BaseProfile)
					{
						FSavedCustomization* SavedCustomization = BaseProfile->Customizations.Find(SwatEquipIdx);
						if (SavedCustomization)
							Customization = *SavedCustomization;
					}

					UItemData* ItemData = UBpGameplayHelperLib::GetItemData();
					if (ItemData)
					{
						FDefaultCharacterCustomization* CharacterCustomization = ItemData->DefaultCharacters.Find(SwatEquipIdx);
						if (CharacterCustomization)
						{
							Customization.Character = CharacterCustomization->Character;
							Customization.Voice = CharacterCustomization->Voice;

							// Set a different default armor skin if we aren't using one
							if (!Customization.ArmorSkin)
								Customization.ArmorSkin = CharacterCustomization->ArmorSkin;
						}
					}
				}
				
				Customization.Sanitize();

				PreviewCharacters[i]->Customization = Customization;
				Customization.ApplyCustomization(PreviewCharacters[i]);
				Customization.ApplyCustomizationSkins(PreviewCharacters[i]);
			}
		}
		else if (i > 0)
		{
			PreviewCharacters[i]->SetActorHiddenInGame(true);
			TArray<USkeletalMeshComponent*> SkeletalMeshComponents;
			PreviewCharacters[i]->GetComponents(SkeletalMeshComponents);
			for (UActorComponent* a : SkeletalMeshComponents)
			{
				USkeletalMeshComponent* s = Cast<USkeletalMeshComponent>(a);
				s->SetCastShadow(false);
			}
			for (ABaseItem* b : PreviewCharacters[i]->GetInventoryComponent()->GetInventoryItems())
			{
				if (b)
				{
					if (!IsValid(b) || b->IsUnreachable())
					{
						b->Destroy();
					}
				}

			}
			PreviewCharacters[i]->GetInventoryComponent()->GetInventoryItems().Empty();
		}
	}
}

// find and remove any duplicates
void UPreMissionPlanning::RemoveDuplicates()
{
	TArray<AReadyOrNotPlayerState*> PlayerStatesInMap = {};
	for (auto& elem : PlayerStatePreviewMap)
	{
		if (PlayerStatesInMap.Contains(elem.Value) && elem.Key)
		{
			PlayerStatePreviewMap[elem.Key] = nullptr;
		}
		PlayerStatesInMap.AddUnique(elem.Value);
	}
}

AReadyOrNotCharacter* UPreMissionPlanning::GetQuartermaster()
{
	for (TActorIterator<AReadyOrNotCharacter> It(GetWorld()); It; ++It)
	{
		AReadyOrNotCharacter* pchar = *It;
		if (pchar->Tags.Contains("Quartermaster"))
		{
			return pchar;
		}
	}
	return nullptr;
}

void UPreMissionPlanning::SetLockInput(bool bShouldLockInput)
{
	bIsInputLocked = bShouldLockInput;
}

bool UPreMissionPlanning::GetInputLocked()
{
	return bIsInputLocked;
}

void UPreMissionPlanning::PlayAnimationOnQuartermaster(FString Animation)
{
	AReadyOrNotCharacter* qm = GetQuartermaster();
	if (qm)
	{
		qm->PlayMontageFromTable(Animation);
	}
}

void UPreMissionPlanning::PlayAnimationOnPreviewCharacter(FString Animation)
{
	if (!GetPreviewCharacter("PreviewCharacter"))
		return;

	FWeaponData wd;
	if (Animation == "tp_pregame_idle_primary")
	{
		wd.Blueprint = ActiveLoadout.Primary;
		//SetPrimaryWeapon(wd);
		UpdatePreviewCharacterPrimary();
		EquipPrimary();
		if (!bRifleDrawn)
		{
			HidePrimary(true);
			GetWorld()->GetTimerManager().SetTimer(HideRifleHandle, FTimerDelegate::CreateUObject(this, &UPreMissionPlanning::HidePrimary, false), 0.2f, false);
			bRifleDrawn = true;
		}
	}
	else if (Animation == "tp_pregame_idle_secondary" || Animation == "tp_pregame_idle_secondary_after_weapon")
	{
		// if (bPistolDrawn && bRifleDrawn)
		// {
		// 	Animation = "tp_pregame_idle_secondary_already_drawn_after_weapon";
		// }
		wd.Blueprint = ActiveLoadout.Secondary;
		//SetSecondaryWeapon(wd);
		UpdatePreviewCharacterSecondary();

		if (!bPistolDrawn)
		{
			GetWorld()->GetTimerManager().SetTimer(HidePistolHandle, FTimerDelegate::CreateUObject(this, &UPreMissionPlanning::EquipSecondary), 0.7f, false);
		}
		else
		{
			GetWorld()->GetTimerManager().ClearTimer(HidePistolHandle);
			EquipSecondary();
		}

		bPistolDrawn = true;
	}

	if (Animation == "tp_pregame_idle_secondary_drawn_only")
	{
		GetWorld()->GetTimerManager().SetTimer(HidePistolHandle, FTimerDelegate::CreateUObject(this, &UPreMissionPlanning::AttachSecondaryToSocket, FName("pistol_holster_socket")), 0.4f, false);
	}

	Animation = GetPistolNonPistolVariation(Animation);

	GetPreviewCharacter("PreviewCharacter")->PlayMontageFromTable(Animation);
}

FString UPreMissionPlanning::GetPistolNonPistolVariation(FString InAnimation)
{
	FString OutAnimation = InAnimation;

	if (!GetDefaultPreviewCharacter())
		return OutAnimation;

	if (!GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Primary)
		return OutAnimation;
	
	ABaseWeapon* BaseItem = Cast<ABaseWeapon>(GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Primary);
	if (BaseItem)
	{
		bool bIsPistolGrip = BaseItem->bPistolGrip;
		
		if (InAnimation == "tp_pregame_idle_primary")
		{
			OutAnimation = bIsPistolGrip ? "tp_pregame_idle_primary_pistol" : "tp_pregame_idle_primary_nonpistol";
		}
		if (InAnimation == "tp_pregame_idle_after_weapon")
		{
			OutAnimation = bIsPistolGrip ? "tp_pregame_idle_after_weapon_pistol" : "tp_pregame_idle_after_weapon_nonpistol";
		}
		if (InAnimation == "tp_pregame_idle_helmet_after_weapon")
		{
			OutAnimation = bIsPistolGrip ? "tp_pregame_idle_helmet_after_weapon_pistol" : "tp_pregame_idle_helmet_after_weapon_nonpistol";
		}
		if (InAnimation == "tp_pregame_idle_secondary_after_weapon")
		{
			OutAnimation = bIsPistolGrip ? "tp_pregame_idle_secondary_after_weapon_pistol" : "tp_pregame_idle_secondary_after_weapon_nonpistol";
		}
		if (InAnimation == "tp_pregame_idle_secondary_already_drawn")
		{
			OutAnimation = bIsPistolGrip ? "tp_pregame_idle_secondary_already_drawn_pistol" : "tp_pregame_idle_secondary_already_drawn_nonpistol";
		}
		if (InAnimation == "tp_pregame_idle_secondary_already_drawn_after_weapon")
		{
			OutAnimation = bIsPistolGrip ? "tp_pregame_idle_secondary_already_drawn_after_weapon_pistol" : "tp_pregame_idle_secondary_already_drawn_after_weapon_nonpistol";
		}
	}
	
	//GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Green, "In: " + InAnimation + " |Out: " + OutAnimation);
	return OutAnimation;
}

void UPreMissionPlanning::SetActiveCameraByTag(FName Tag, float BlendTime)
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
					GetWorld()->GetTimerManager().SetTimer(UnlockInput_Handle, FTimerDelegate::CreateUObject(this, &UPreMissionPlanning::SetLockInput, false), BlendTime * 0.1f, false);
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

void UPreMissionPlanning::SetLightColorByTag(FName Tag, FLinearColor Color)
{
	LightTag.Add(Tag, Color);
	for (TActorIterator<ALight> It(GetWorld()); It; ++It)
	{
		APointLight* pl = Cast<APointLight>(*It);
		if (pl && pl->Tags.Contains(Tag))
		{
			pl->SetLightColor(Color);
		}
		ASpotLight* sl = Cast<ASpotLight>(*It);
		if (sl && sl->Tags.Contains(Tag))
		{
			sl->GetLightComponent()->SetVisibility(Color.A != 0);
			sl->SetLightColor(Color);
		}
	}
}

void UPreMissionPlanning::EquipPrimary()
{
	if (!GetDefaultPreviewCharacter())
		return;
	ABaseItem* bi = Cast<ABaseItem>(GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Primary);
	if (bi)
	{
		bi->GetItemMesh()->AttachToComponent(GetDefaultPreviewCharacter()->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, "RightWeaponSocket");
		bi->GetItemMesh()->SetVisibility(true);
	}
}

void UPreMissionPlanning::EquipSecondary()
{
	if (!GetDefaultPreviewCharacter())
		return;

	ABaseItem* bi = Cast<ABaseItem>(GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Secondary);
	if (bi)
	{
		bi->bNoAttachmentRep = true;
		bi->HandsSocket = "LeftWeaponSocketWeaponPreview";
		bi->GetItemMesh()->AttachToComponent(GetDefaultPreviewCharacter()->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, "LeftWeaponSocketWeaponPreview");
		bi->GetItemMesh()->SetVisibility(true);
	}
}

void UPreMissionPlanning::HideWeapons(bool bHidePrimary, bool bHideSecondary, float Delay)
{
	if (Delay == 0.0f)
	{
		HidePrimary(bHidePrimary);
		HideSecondary(bHideSecondary);
	} else
	{
		GetWorld()->GetTimerManager().SetTimer(HidePrimary_Handle, FTimerDelegate::CreateUObject(this, &UPreMissionPlanning::HidePrimary, bHidePrimary), Delay, false);
		GetWorld()->GetTimerManager().SetTimer(HideSecondary_Handle, FTimerDelegate::CreateUObject(this, &UPreMissionPlanning::HideSecondary, bHideSecondary), Delay, false);
	}
	
}

void UPreMissionPlanning::HidePrimary(bool bIsHidden)
{
	if (!GetDefaultPreviewCharacter())
		return;
	if (GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Primary)
	{
		GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Primary->SetActorHiddenInGame(bIsHidden);
	}
}

void UPreMissionPlanning::HideSecondary(bool bIsHidden)
{
	if (!GetDefaultPreviewCharacter())
		return;
	
	if (GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Secondary)
	{
		GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Secondary->SetActorHiddenInGame(bIsHidden);
	}
}

void UPreMissionPlanning::AttachPrimaryToSocket(FName Socket)
{
	if (!GetDefaultPreviewCharacter())
		return;
	if (GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Primary)
	{
		GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Primary->GetItemMesh()->AttachToComponent(GetDefaultPreviewCharacter()->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, Socket);
	}
}

void UPreMissionPlanning::AttachSecondaryToSocket(FName Socket)
{
	if (!GetDefaultPreviewCharacter())
		return;

	if (Socket == "pistol_holster_socket")
	{
		bPistolDrawn = false;
	}

	
	if (GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Secondary)
	{
		GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Secondary->bNoAttachmentRep = true;
		GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Secondary->GetItemMesh()->AttachToComponent(GetDefaultPreviewCharacter()->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, Socket);
	}
}

bool UPreMissionPlanning::IsAnyWeaponVisible()
{
	if (!GetDefaultPreviewCharacter())
		return false;

	if (GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Primary)
	{
		if (!GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Primary->IsHidden())
			return true;
	}
	if (GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Secondary)
	{
		if (!GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Secondary->IsHidden())
			return true;
	}
	return false;
}

void UPreMissionPlanning::UpdatePrimaryWeaponAttachmentData()
{
	FSavedWeaponAttachmentData AttachmentData;
	AttachmentData.bHasSavedData = true;
	AttachmentData.ScopeAttachment = ActiveLoadout.PrimaryScope;
	AttachmentData.MuzzleAttachment = ActiveLoadout.PrimaryMuzzle;
	AttachmentData.OverbarrelAttachment = ActiveLoadout.PrimaryOverbarrel;
	AttachmentData.UnderbarrelAttachment = ActiveLoadout.PrimaryUnderbarrel;
	AttachmentData.StockAttachment = ActiveLoadout.PrimaryStock;
	AttachmentData.GripAttachment = ActiveLoadout.PrimaryGrip;
	AttachmentData.IlluminatorAttachment = ActiveLoadout.PrimaryIlluminator;
	AttachmentData.AmmunitionAttachment = ActiveLoadout.PrimaryAmmunition;
	AttachmentData.Skin = ActiveLoadout.PrimarySkin;

	UpdateItemAttachmentData(ActiveLoadout.Primary, AttachmentData);
}

void UPreMissionPlanning::UpdateSecondaryWeaponAttachmentData()
{
	FSavedWeaponAttachmentData AttachmentData;
	AttachmentData.bHasSavedData = true;
	AttachmentData.ScopeAttachment = ActiveLoadout.SecondaryScope;
	AttachmentData.MuzzleAttachment = ActiveLoadout.SecondaryMuzzle;
	AttachmentData.OverbarrelAttachment = ActiveLoadout.SecondaryOverbarrel;
	AttachmentData.UnderbarrelAttachment = ActiveLoadout.SecondaryUnderbarrel;
	AttachmentData.StockAttachment = ActiveLoadout.SecondaryStock;
	AttachmentData.GripAttachment = ActiveLoadout.SecondaryGrip;
	AttachmentData.IlluminatorAttachment = ActiveLoadout.SecondaryIlluminator;
	AttachmentData.AmmunitionAttachment = ActiveLoadout.SecondaryAmmunition;
	AttachmentData.Skin = ActiveLoadout.SecondarySkin;

	UpdateItemAttachmentData(ActiveLoadout.Secondary, AttachmentData);
}

void UPreMissionPlanning::UpdateItemAttachmentData(const TSubclassOf<ABaseItem> Item, const FSavedWeaponAttachmentData AttachmentData)
{
	WeaponToAttachmentsMap.Remove(Item);
	WeaponToAttachmentsMap.Add(Item, AttachmentData);
}

void UPreMissionPlanning::UpdateWeaponPresets(const TSubclassOf<ABaseItem> Weapon, const FSavedWeaponPreset Presets)
{
	WeaponToWeaponPresetsMap.Remove(Weapon);
	WeaponToWeaponPresetsMap.Add(Weapon, Presets);
}

void UPreMissionPlanning::UpdateWeaponPreset(const TSubclassOf<ABaseItem> Weapon, const FWeaponPreset PresetData, int32 Index)
{
	Index = FMath::Clamp(Index, 0, 4);
	
	FSavedWeaponPreset SavedPreset;
	if (FSavedWeaponPreset* FoundPreset = WeaponToWeaponPresetsMap.Find(Weapon))
	{
		SavedPreset = *FoundPreset;
	}
	
	if (SavedPreset.Presets.Num() > 4)
	{
		if (SavedPreset.Presets.IsValidIndex(Index))
		{
			SavedPreset.Presets[Index] = PresetData;
			
			WeaponToWeaponPresetsMap.Remove(Weapon);
			WeaponToWeaponPresetsMap.Add(Weapon, SavedPreset);
		}
	}
	else
	{
		SavedPreset.Presets.Empty(5);
		SavedPreset.Presets.Init(FWeaponPreset(), 5);
		
		if (SavedPreset.Presets.IsValidIndex(Index))
		{
			SavedPreset.Presets[Index] = PresetData;

			WeaponToWeaponPresetsMap.Remove(Weapon);
			WeaponToWeaponPresetsMap.Add(Weapon, SavedPreset);
		}
	}
}

void UPreMissionPlanning::SaveItemClassAsSlot(EItemType ItemType, TSubclassOf<ABaseItem> Class)
{
	UBaseProfile* Profile = UBaseProfile::GetCurrentProfile();
	if (!Profile)
		return;

	Profile->SavedWeaponClassOfTypeMap.Add(ItemType, Class);
	Profile->SaveProfile();
}

void UPreMissionPlanning::UpdateWeaponDefaultFireMode(const TSubclassOf<ABaseWeapon> Weapon, const EFireMode NewDefaultFireMode)
{
	WeaponClassToDefaultFireModeMap.Remove(Weapon);
	WeaponClassToDefaultFireModeMap.Add(Weapon, NewDefaultFireMode);
}

void UPreMissionPlanning::SaveWeaponDefaultFireMode()
{
	UBaseProfile* Profile = UBaseProfile::GetCurrentProfile();
	if (!Profile)
		return;

	Profile->WeaponClassToDefaultFireModeMap = WeaponClassToDefaultFireModeMap;
	Profile->SaveProfile();
}

void UPreMissionPlanning::LoadWeaponDefaultFireModes()
{
	UBaseProfile* Profile = UBaseProfile::GetCurrentProfile();
	if (!Profile)
		return;

	WeaponClassToDefaultFireModeMap = Profile->WeaponClassToDefaultFireModeMap;
	OnWeaponDefaultFireModesLoaded();
}

TSubclassOf<ABaseItem> UPreMissionPlanning::GetLastItemInSlot(EItemType ItemType)
{
	UBaseProfile* Profile = UBaseProfile::GetCurrentProfile();
	if (!Profile)
		return nullptr;

	TSubclassOf<ABaseItem>* Class = Profile->SavedWeaponClassOfTypeMap.Find(ItemType);
	if (Class)
		return *Class;

	return nullptr;
}

FSavedWeaponAttachmentData UPreMissionPlanning::GetItemAttachmentData(TSubclassOf<ABaseItem> Weapon)
{
	UBaseProfile* Profile = UBaseProfile::GetCurrentProfile();
	if (!Profile)
		return FSavedWeaponAttachmentData();

	FSavedWeaponAttachmentData* AttachmentData = Profile->AttachmentSaveMap.Find(Weapon);
	if (AttachmentData)
		return *AttachmentData;

	return FSavedWeaponAttachmentData();
}

FSavedWeaponPreset UPreMissionPlanning::GetWeaponPresetsData(const TSubclassOf<ABaseItem> Weapon)
{
	UBaseProfile* Profile = UBaseProfile::GetCurrentProfile();
	if (!Profile)
		return FSavedWeaponPreset();

	FSavedWeaponPreset* Preset = Profile->WeaponToWeaponPresetsMap.Find(Weapon);
	if (Preset)
		return *Preset;

	return FSavedWeaponPreset();
}

FWeaponPreset UPreMissionPlanning::GetWeaponPresetData(const TSubclassOf<ABaseItem> Weapon, const int32 Index)
{
	UBaseProfile* Profile = UBaseProfile::GetCurrentProfile();
	if (!Profile)
		return FWeaponPreset();

	FSavedWeaponPreset* SavedPreset = Profile->WeaponToWeaponPresetsMap.Find(Weapon);
	if (SavedPreset && SavedPreset->Presets.IsValidIndex(Index))
		return SavedPreset->Presets[Index];
	
	return FWeaponPreset();
}

void UPreMissionPlanning::UpdateOrbitalCameraLocation(FVector Location)
{
	return;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* a = *It;
		if (a->Tags.Contains("OrbitalCameraActor"))
		{
			a->SetActorLocation(Location);
		}
	}
}

void UPreMissionPlanning::SetWorkbenchItemClass(TSubclassOf<ABaseItem> Item, FName Tag)
{

	if (bIsWeaponCustomization)
	{
		Tag = "AttachmentWorkbench";
	}

	if (ABaseItem** DoubleItemPtr = WorkBenchItemPtrMap.Find(Tag))
	{
		if (ABaseItem* DereferencedItem = *DoubleItemPtr)
		{
			DereferencedItem->Destroy();
			DereferencedItem = nullptr;
		}
	}

	if (pc)
	{
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
		
		if (ps)
		{
			FSavedLoadout Loadout = ps->GetLoadout();
			WorkBenchItemPtrMap.Add(Tag, GetWorld()->SpawnActor<ABaseItem>(Item, PlacementActor->GetActorTransform()));
			ABaseWeapon* bw = Cast<ABaseWeapon>(WorkBenchItemPtrMap[Tag]);
			if (bw)
			{
				bw->Tags.Add("NoRelevancy");
				bw->SetReplicates(false);
				if (bw->ItemCategories.Contains(EItemCategory::IC_Primary))
				{
					bw->GetItemMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
					bw->GetItemMesh()->SetSimulatePhysics(true);
					bw->GetItemMesh()->SetLinearDamping(1.0f);
					bw->GetItemMesh()->SetAngularDamping(10000.0f);
					bw->AddAttachment(ActiveLoadout.PrimaryScope);
					bw->AddAttachment(ActiveLoadout.PrimaryMuzzle);
					bw->AddAttachment(ActiveLoadout.PrimaryOverbarrel);
					bw->AddAttachment(ActiveLoadout.PrimaryUnderbarrel);
					bw->AddAttachment(ActiveLoadout.PrimaryStock);
					bw->AddAttachment(ActiveLoadout.PrimaryGrip);
					bw->AddAttachment(ActiveLoadout.PrimaryIlluminator);
					bw->AddAttachment(ActiveLoadout.PrimaryAmmunition);

					if (ActiveLoadout.PrimarySkin)
					{
						USkinComponent* SkinComp = NewObject<USkinComponent>(bw, ActiveLoadout.PrimarySkin);
						if (SkinComp)
						{
							SkinComp->RegisterComponent();
						}
					}
					bw->AttachStatic();
					
				}
				else if (bw->ItemCategories.Contains(EItemCategory::IC_Secondary))
				{
					bw->GetItemMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
					bw->GetItemMesh()->SetLinearDamping(1.0f);
					bw->GetItemMesh()->SetAngularDamping(10000.0f);
					bw->GetItemMesh()->SetSimulatePhysics(true);

					bw->AddAttachment(ActiveLoadout.SecondaryScope);
					bw->AddAttachment(ActiveLoadout.SecondaryMuzzle);
					bw->AddAttachment(ActiveLoadout.SecondaryOverbarrel);
					bw->AddAttachment(ActiveLoadout.SecondaryUnderbarrel);
					bw->AddAttachment(ActiveLoadout.SecondaryStock);
					bw->AddAttachment(ActiveLoadout.SecondaryGrip);
					bw->AddAttachment(ActiveLoadout.SecondaryIlluminator);
					bw->AddAttachment(ActiveLoadout.SecondaryAmmunition);

					if (ActiveLoadout.SecondarySkin)
					{
						USkinComponent* SkinComp = NewObject<USkinComponent>(bw, ActiveLoadout.SecondarySkin);
						if (SkinComp)
						{
							SkinComp->RegisterComponent();
						}
					}
					bw->AttachStatic();
					
				}
			}
		}
	}
}

void UPreMissionPlanning::UpdateWorkbenchItemAttachments()
{
	FName PrimaryTag = "PrimaryPlacementAttachments";
	FName SecondaryTag = "SecondaryPlacementAttachments";
	if (bIsWeaponCustomization)
	{
		bIsCustomizingPrimary ? PrimaryTag = "AttachmentWorkbench" : SecondaryTag = "AttachmentWorkbench";
	}
	if (WorkBenchItemPtrMap.Find(PrimaryTag))
	{
		if (ABaseItem* PrimaryItem = *WorkBenchItemPtrMap.Find(PrimaryTag))
		{
			if (ABaseWeapon* PrimaryBW = Cast<ABaseWeapon>(PrimaryItem))
			{
				PrimaryBW->AddAttachment(ActiveLoadout.PrimaryScope);
				PrimaryBW->AddAttachment(ActiveLoadout.PrimaryMuzzle);
				PrimaryBW->AddAttachment(ActiveLoadout.PrimaryOverbarrel);
				PrimaryBW->AddAttachment(ActiveLoadout.PrimaryUnderbarrel);
				PrimaryBW->AddAttachment(ActiveLoadout.PrimaryStock);
				PrimaryBW->AddAttachment(ActiveLoadout.PrimaryGrip);
				PrimaryBW->AddAttachment(ActiveLoadout.PrimaryIlluminator);
				PrimaryBW->AddAttachment(ActiveLoadout.PrimaryAmmunition);

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
				
				if (ActiveLoadout.PrimarySkin)
				{
					PrimaryBW->GetComponents<USkinComponent>(SkinComps);
					
					if (SkinComps.Num() == 0)
					{
						USkinComponent* SkinComp = NewObject<USkinComponent>(PrimaryBW, ActiveLoadout.PrimarySkin);
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
				SecondaryBW->AddAttachment(ActiveLoadout.SecondaryScope);
				SecondaryBW->AddAttachment(ActiveLoadout.SecondaryMuzzle);
				SecondaryBW->AddAttachment(ActiveLoadout.SecondaryOverbarrel);
				SecondaryBW->AddAttachment(ActiveLoadout.SecondaryUnderbarrel);
				SecondaryBW->AddAttachment(ActiveLoadout.SecondaryStock);
				SecondaryBW->AddAttachment(ActiveLoadout.SecondaryGrip);
				SecondaryBW->AddAttachment(ActiveLoadout.SecondaryIlluminator);
				SecondaryBW->AddAttachment(ActiveLoadout.SecondaryAmmunition);

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

				if (ActiveLoadout.SecondarySkin)
				{
					SecondaryBW->GetComponents<USkinComponent>(SkinComps);
					
					if (SkinComps.Num() == 0)
					{
						USkinComponent* SkinComp = NewObject<USkinComponent>(SecondaryBW, ActiveLoadout.SecondarySkin);
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

void UPreMissionPlanning::SetPrimaryWeapon(FWeaponData WeaponData)
{
	if (!GetDefaultPreviewCharacter())
		return;

	if (pc)
	{
		if (ps)
		{
		
			ActiveLoadout.Primary = WeaponData.Blueprint;
							
			// update attachments from attachment save data
			FSavedWeaponAttachmentData AttachmentData = GetItemAttachmentData(ActiveLoadout.Primary);
			
			if (AttachmentData.bHasSavedData)
			{
				ActiveLoadout.PrimaryScope = AttachmentData.ScopeAttachment;
				ActiveLoadout.PrimaryMuzzle = AttachmentData.MuzzleAttachment;
				ActiveLoadout.PrimaryOverbarrel = AttachmentData.OverbarrelAttachment;
				ActiveLoadout.PrimaryUnderbarrel = AttachmentData.UnderbarrelAttachment;
				ActiveLoadout.PrimaryStock = AttachmentData.StockAttachment;
				ActiveLoadout.PrimaryGrip = AttachmentData.GripAttachment;
				ActiveLoadout.PrimaryIlluminator = AttachmentData.IlluminatorAttachment;
				ActiveLoadout.PrimaryAmmunition = AttachmentData.AmmunitionAttachment;
				ActiveLoadout.PrimarySkin = AttachmentData.Skin;
			}
			else
			{
				UItemData* ItemData = UBpGameplayHelperLib::GetItemData(GetWorld());
				if (ItemData)
				{
					ActiveLoadout.PrimaryScope = ItemData->NullPrimaryScopeAttachment;
					ActiveLoadout.PrimaryMuzzle = ItemData->NullMuzzleAttachment;
					ActiveLoadout.PrimaryOverbarrel = ItemData->NullOverbarrelAttachment;
					ActiveLoadout.PrimaryUnderbarrel = ItemData->NullUnderbarrelAttachment;
					ActiveLoadout.PrimaryStock = ItemData->NullStockAttachment;
					ActiveLoadout.PrimaryGrip = ItemData->NullGripAttachment;
					ActiveLoadout.PrimaryIlluminator = ItemData->NullIlluminatorAttachment;
					ActiveLoadout.PrimaryAmmunition = ItemData->NullAmmunitionAttachment;
					ActiveLoadout.PrimarySkin = ItemData->FactorySkin;
				}
			}
			SaveActiveLoadout();
			UpdatePreviewCharacterPrimary();
		}
	}
}

void UPreMissionPlanning::UpdatePreviewCharacterPrimary()
{
	if (!GetDefaultPreviewCharacter())
		return;

	if (pc)
	{
		if (ps)
		{
			ABaseItem* bi = GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Primary;
			if ((bi && bi->GetClass() != ActiveLoadout.Primary))
			{
				bi->Destroy();
				bi = nullptr;
				UpdatePreviewCharacter(ps, "MyPlayerPreview");
			}

			if (!bi)
			{
				if (!ActiveLoadout.Primary)
				{
					return;
				}
				bi = GetWorld()->SpawnActor<ABaseItem>(ActiveLoadout.Primary, FTransform());
				bi->Tags.Add("NoRelevancy");
				bi->Tags.Add("CustomizationMenu");
				bi->bNoAttachmentRep = true;
				bi->bDisableAnimInstanceWhenNotEquipped = false;
				bi->SetReplicates(false);

				GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Primary = bi;
				bi->AttachToComponent(GetDefaultPreviewCharacter()->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, bi->HandsSocket);

				EquipPrimary();

			}

			DoPrimaryWeaponPreviewBlend();

			UpdatePreviewWeaponAttachments(false, ActiveLoadout.PrimaryScope);
			UpdatePreviewWeaponAttachments(false, ActiveLoadout.PrimaryMuzzle);
			UpdatePreviewWeaponAttachments(false, ActiveLoadout.PrimaryUnderbarrel);
			UpdatePreviewWeaponAttachments(false, ActiveLoadout.PrimaryOverbarrel);
			UpdatePreviewWeaponAttachments(false, ActiveLoadout.PrimaryStock);
			UpdatePreviewWeaponAttachments(false, ActiveLoadout.PrimaryGrip);
			UpdatePreviewWeaponAttachments(false, ActiveLoadout.PrimaryIlluminator);
			UpdatePreviewWeaponAttachments(false, ActiveLoadout.PrimaryAmmunition);

			if (ActiveLoadout.PrimarySkin)
			{
				USkinComponent* SkinComp = NewObject<USkinComponent>(bi, ActiveLoadout.PrimarySkin);
				if (SkinComp)
				{
					SkinComp->RegisterComponent();
				}
			}
		}
	}
}

void UPreMissionPlanning::DoPrimaryWeaponPreviewBlend()
{
	if (!GetDefaultPreviewCharacter())
		return;

	ABaseWeapon* Weapon = Cast<ABaseWeapon>(GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Primary);
	if (!Weapon)
		return;
	
	// swap  animation if already playing (ie just swappign gun).. match current time`
	if (GetDefaultPreviewCharacter()->IsTableMontagePlaying("tp_pregame_idle_primary_nonpistol") && Weapon->bPistolGrip)
	{
		float CurrentMontageTime = GetDefaultPreviewCharacter()->GetMesh()->GetAnimInstance()->Montage_GetPosition(nullptr);
		UAnimMontage* NewMontage = GetDefaultPreviewCharacter()->GetMontageFromTable("tp_pregame_idle_primary_pistol");
		FAlphaBlend OldBlendIn = NewMontage->BlendIn;
		NewMontage->BlendIn = 0.0f;
		GetDefaultPreviewCharacter()->GetMesh()->GetAnimInstance()->Montage_Play(NewMontage, 1.0f, EMontagePlayReturnType::MontageLength, CurrentMontageTime);
		NewMontage->BlendIn = OldBlendIn;
	}
	else if (GetDefaultPreviewCharacter()->IsTableMontagePlaying("tp_pregame_idle_primary_pistol") && !Weapon->bPistolGrip)
	{
		float CurrentMontageTime = GetDefaultPreviewCharacter()->GetMesh()->GetAnimInstance()->Montage_GetPosition(nullptr);
		UAnimMontage* NewMontage = GetDefaultPreviewCharacter()->GetMontageFromTable("tp_pregame_idle_primary_nonpistol");
		FAlphaBlend OldBlendIn = NewMontage->BlendIn;
		NewMontage->BlendIn = 0.0f;
		GetDefaultPreviewCharacter()->GetMesh()->GetAnimInstance()->Montage_Play(NewMontage, 1.0f, EMontagePlayReturnType::MontageLength, CurrentMontageTime);
		NewMontage->BlendIn = OldBlendIn;
	}
	else if (GetDefaultPreviewCharacter()->IsTableMontagePlaying("tp_pregame_idle_helmet_after_weapon_nonpistol") && Weapon->bPistolGrip)
	{
		float CurrentMontageTime = GetDefaultPreviewCharacter()->GetMesh()->GetAnimInstance()->Montage_GetPosition(nullptr);
		UAnimMontage* NewMontage = GetDefaultPreviewCharacter()->GetMontageFromTable("tp_pregame_idle_helmet_after_weapon_pistol");
		FAlphaBlend OldBlendIn = NewMontage->BlendIn;
		NewMontage->BlendIn = 0.0f;
		GetDefaultPreviewCharacter()->GetMesh()->GetAnimInstance()->Montage_Play(NewMontage, 1.0f, EMontagePlayReturnType::MontageLength, CurrentMontageTime);
		NewMontage->BlendIn = OldBlendIn;
	}
	else if (GetDefaultPreviewCharacter()->IsTableMontagePlaying("tp_pregame_idle_helmet_after_weapon_pistol") && !Weapon->bPistolGrip)
	{
		float CurrentMontageTime = GetDefaultPreviewCharacter()->GetMesh()->GetAnimInstance()->Montage_GetPosition(nullptr);
		UAnimMontage* NewMontage = GetDefaultPreviewCharacter()->GetMontageFromTable("tp_pregame_idle_helmet_after_weapon_nonpistol");
		FAlphaBlend OldBlendIn = NewMontage->BlendIn;
		NewMontage->BlendIn = 0.0f;
		GetDefaultPreviewCharacter()->GetMesh()->GetAnimInstance()->Montage_Play(NewMontage, 1.0f, EMontagePlayReturnType::MontageLength, CurrentMontageTime);
		NewMontage->BlendIn = OldBlendIn;
	}
	else if (GetDefaultPreviewCharacter()->IsTableMontagePlaying("tp_pregame_after_weapon_nonpistol") && Weapon->bPistolGrip)
	{
		float CurrentMontageTime = GetDefaultPreviewCharacter()->GetMesh()->GetAnimInstance()->Montage_GetPosition(nullptr);
		UAnimMontage* NewMontage = GetDefaultPreviewCharacter()->GetMontageFromTable("tp_pregame_idle_after_weapon_pistol");
		FAlphaBlend OldBlendIn = NewMontage->BlendIn;
		NewMontage->BlendIn = 0.0f;
		GetDefaultPreviewCharacter()->GetMesh()->GetAnimInstance()->Montage_Play(NewMontage, 1.0f, EMontagePlayReturnType::MontageLength, CurrentMontageTime);
		NewMontage->BlendIn = OldBlendIn;
	}
	else if (GetDefaultPreviewCharacter()->IsTableMontagePlaying("ttp_pregame_idle_after_weapon_pistol") && !Weapon->bPistolGrip)
	{
		float CurrentMontageTime = GetDefaultPreviewCharacter()->GetMesh()->GetAnimInstance()->Montage_GetPosition(nullptr);
		UAnimMontage* NewMontage = GetDefaultPreviewCharacter()->GetMontageFromTable("tp_pregame_idle_after_weapon_nonpistol");
		FAlphaBlend OldBlendIn = NewMontage->BlendIn;
		NewMontage->BlendIn = 0.0f;
		GetDefaultPreviewCharacter()->GetMesh()->GetAnimInstance()->Montage_Play(NewMontage, 1.0f, EMontagePlayReturnType::MontageLength, CurrentMontageTime);
		NewMontage->BlendIn = OldBlendIn;
	}
}

void UPreMissionPlanning::SetSecondaryWeapon(FWeaponData WeaponData)
{
	if (!GetDefaultPreviewCharacter())
		return;
	if (pc)
	{
		if (ps)
		{
			ActiveLoadout.Secondary = WeaponData.Blueprint;
			// update attachments from attachment save data
			FSavedWeaponAttachmentData AttachmentData = GetItemAttachmentData(ActiveLoadout.Secondary);
			if (AttachmentData.bHasSavedData)
			{
				ActiveLoadout.SecondaryScope = AttachmentData.ScopeAttachment;
				ActiveLoadout.SecondaryMuzzle = AttachmentData.MuzzleAttachment;
				ActiveLoadout.SecondaryOverbarrel = AttachmentData.OverbarrelAttachment;
				ActiveLoadout.SecondaryUnderbarrel = AttachmentData.UnderbarrelAttachment;
				ActiveLoadout.SecondaryStock = AttachmentData.StockAttachment;
				ActiveLoadout.SecondaryGrip = AttachmentData.GripAttachment;
				ActiveLoadout.SecondaryIlluminator = AttachmentData.IlluminatorAttachment;
				ActiveLoadout.SecondaryAmmunition = AttachmentData.AmmunitionAttachment;
				ActiveLoadout.SecondarySkin = AttachmentData.Skin;
			}
			else
			{
				UItemData* ItemData = UBpGameplayHelperLib::GetItemData(GetWorld());
				if (ItemData)
				{
					ActiveLoadout.SecondaryScope = ItemData->NullPrimaryScopeAttachment;
					ActiveLoadout.SecondaryMuzzle = ItemData->NullMuzzleAttachment;
					ActiveLoadout.SecondaryOverbarrel = ItemData->NullOverbarrelAttachment;
					ActiveLoadout.SecondaryUnderbarrel = ItemData->NullUnderbarrelAttachment;
					ActiveLoadout.SecondaryStock = ItemData->NullStockAttachment;
					ActiveLoadout.SecondaryGrip = ItemData->NullGripAttachment;
					ActiveLoadout.SecondaryIlluminator = ItemData->NullIlluminatorAttachment;
					ActiveLoadout.SecondaryAmmunition = ItemData->NullAmmunitionAttachment;
					ActiveLoadout.SecondarySkin = ItemData->FactorySkin;

				}
			}
			SaveActiveLoadout();
			UpdatePreviewCharacterSecondary();
		}
	}
}

void UPreMissionPlanning::UpdatePreviewCharacterSecondary()
{
	if (!GetDefaultPreviewCharacter())
		return;
	if (pc)
	{
		if (ps)
		{
			ABaseItem* bi = GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Secondary;
			if ((bi && bi->GetClass() != ActiveLoadout.Secondary))
			{
				bi->Destroy();
				bi = nullptr;
			}

			if (!bi)
			{
				if (!ActiveLoadout.Secondary)
				{
					return;
				}
				bi = GetWorld()->SpawnActor<ABaseItem>(ActiveLoadout.Secondary);
				bi->Tags.Add("NoRelevancy");
				bi->Tags.Add("CustomizationMenu");
				bi->bNoAttachmentRep = true;
				bi->SetReplicates(false);
				GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Secondary = bi;
				bi->AttachToComponent(GetDefaultPreviewCharacter()->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, bi->HandsSocket);
			}

			if (!bPistolDrawn && !ps->IsVipPlayerState())
			{
				AttachSecondaryToSocket("Pistol_Holster_Socket");
			}
			else
			{
				EquipSecondary();
			}


			ABaseWeapon* bw = Cast<ABaseWeapon>(GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Secondary);
			if (bw)
			{
				bw->AddAttachment(ActiveLoadout.SecondaryScope, false);
				bw->AddAttachment(ActiveLoadout.SecondaryMuzzle, false);
				bw->AddAttachment(ActiveLoadout.SecondaryUnderbarrel, false);
				bw->AddAttachment(ActiveLoadout.SecondaryOverbarrel, false);
				bw->AddAttachment(ActiveLoadout.SecondaryStock, false);
				bw->AddAttachment(ActiveLoadout.SecondaryGrip, false);
				bw->AddAttachment(ActiveLoadout.SecondaryIlluminator, false);
				bw->AddAttachment(ActiveLoadout.SecondaryAmmunition, false);
			}

			if (ActiveLoadout.SecondarySkin)
			{
				USkinComponent* SkinComp = NewObject<USkinComponent>(bi, ActiveLoadout.SecondarySkin);
				if (SkinComp)
				{
					SkinComp->RegisterComponent();
				}
			}
		}
	}
}

void UPreMissionPlanning::SetHeadwear(TSubclassOf<ABaseItem> Headwear)
{
	if (!GetDefaultPreviewCharacter())
		return;

	ActiveLoadout.Helmet = Headwear;
	UpdatePreviewCharacterHeadwear();
}

void UPreMissionPlanning::UpdatePreviewCharacterHeadwear()
{
	if (!GetDefaultPreviewCharacter())
		return;

	if (ABaseItem* Helmet = GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Helmet)
	{
		Helmet->Destroy();
	}

	if (ABaseItem* bi = GetWorld()->SpawnActor<ABaseItem>(ActiveLoadout.Helmet))
	{
		GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Helmet = bi;
		bi->Tags.Add("NoRelevancy");
		bi->Tags.Add("CustomizationMenu");
		bi->bNoAttachmentRep = true;
		bi->SetReplicates(false);
		GetDefaultPreviewCharacter()->GetInventoryComponent()->AddInventoryItem(GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Helmet);
		bi->GetItemMesh()->SetVisibility(true);
		bi->AttachToComponent(GetDefaultPreviewCharacter()->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, bi->BodySocket);
	}
}

void UPreMissionPlanning::SetBodyArmour(TSubclassOf<ABaseItem> BodyArmour)
{
	if (!GetDefaultPreviewCharacter())
		return;

	if (BodyArmour)
	{
		ActiveLoadout.Armor = BodyArmour;
	}

	UpdatePreviewCharacterArmour();

}

void UPreMissionPlanning::UpdatePreviewCharacterArmour()
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
	if (ActiveLoadout.Armor)
	{
		ABaseItem* Armor = GetWorld()->SpawnActor<ABaseItem>(ActiveLoadout.Armor);
		GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Armor = Armor;
		if (Armor)
		{
			Armor->SetReplicates(false);
		}
		GetDefaultPreviewCharacter()->GetInventoryComponent()->AddInventoryItem(GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Armor);
	}
}

void UPreMissionPlanning::SetLongTactical(TSubclassOf<ABaseItem> LongTactical)
{
	if (!GetDefaultPreviewCharacter())
		return;
	
	ActiveLoadout.LongTactical = LongTactical;
	UpdatePreviewCharacterLongTactical();
}

void UPreMissionPlanning::UpdatePreviewCharacterLongTactical()
{
	if (!GetDefaultPreviewCharacter())
		return;

	if (GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().LongTactical)
	{
		GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().LongTactical->Destroy();
	}
	ABaseItem* LongTacticalInst = GetWorld()->SpawnActor<ABaseItem>(ActiveLoadout.LongTactical);;
	GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().LongTactical = LongTacticalInst;
	if (LongTacticalInst)
	{
		LongTacticalInst->SetReplicates(false);
	}
	GetDefaultPreviewCharacter()->GetInventoryComponent()->AddInventoryItem(GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().LongTactical);
}

void UPreMissionPlanning::SetItem(EItemType ItemType, TSubclassOf<ABaseItem> ItemClass)
{
	LastSetItemType = ItemType;
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
	default:
		break;

	}
}

void UPreMissionPlanning::SetItem_V2(const EItemClass ItemClass, const TSubclassOf<ABaseItem> ItemObjectClass)
{
	LastSetItemClass = ItemClass;
	LastSetItemObjectClass = ItemObjectClass;
	FWeaponData WeaponData;
	WeaponData.Blueprint = ItemObjectClass;
	switch (ItemClass)
	{
		case EItemClass::IC_AssaultRifle:	SetPrimaryWeapon(WeaponData); break;
		case EItemClass::IC_SMG:			SetPrimaryWeapon(WeaponData); break;
		case EItemClass::IC_LMG:			SetPrimaryWeapon(WeaponData); break;
		case EItemClass::IC_Pistol:			SetSecondaryWeapon(WeaponData); break;
		case EItemClass::IC_Sniper:			SetPrimaryWeapon(WeaponData);	break;
		case EItemClass::IC_LessLethal:		SetPrimaryWeapon(WeaponData); break;
		case EItemClass::IC_Shotgun:		SetPrimaryWeapon(WeaponData); break;
		case EItemClass::IC_Launcher:		SetPrimaryWeapon(WeaponData); break;
		case EItemClass::IC_Armor:			SetBodyArmour(ItemObjectClass); break;
		case EItemClass::IC_Headgear:		SetHeadwear(ItemObjectClass); break;
		case EItemClass::IC_LongTactical:	SetLongTactical(ItemObjectClass); break;
		default: break;
	}
}

void UPreMissionPlanning::SetPlayerSkin(TSubclassOf<USkinComponent> SkinCompClass)
{
	if (!GetDefaultPreviewCharacter())
		return;

	UItemData* ItemData = UBpGameplayHelperLib::GetItemData(GetWorld());
	if (!ItemData)
		return;

	if (!pc)
		return;

	if (!ps)
		return;

	UReadyOrNotSaveGame* lg = UBpGameplayHelperLib::GetLoadGameInstance();
	if (lg)
	{
		lg->SkinSaveMap.Add(ps->GetTeam(), SkinCompClass);
		UGameplayStatics::SaveGameToSlot(lg, lg->SaveSlotName, lg->UserIndex);
	}
	SetBodyArmour(ActiveLoadout.Armor);
	UpdateTeamVisuals(ps);
	USkinComponent* SkinComp = SkinCompClass && GetAvailablePlayerSkins().Contains(SkinCompClass) ? NewObject<USkinComponent>(GetDefaultPreviewCharacter(), SkinCompClass) : nullptr;
	if (SkinComp && SkinCompClass != ItemData->UniformSelection[0])
	{
		SkinComp->ApplySkin();
	}
	ActiveLoadout.PlayerSkin = SkinCompClass;
	SaveActiveLoadout();
}

TArray<TSubclassOf<USkinComponent>> UPreMissionPlanning::GetAvailablePlayerSkins()
{
	UItemData* ItemData = UBpGameplayHelperLib::GetItemData(GetWorld());
	if (!ItemData)
		return {};

	if (!pc)
		return {};

	if (!ps)
		return {};
	

	TArray<TSubclassOf<USkinComponent>> ReturnData;
	TArray<TSubclassOf<USkinComponent>> PlayerSkinComponents = ItemData->UniformSelection;
	for (TSubclassOf<USkinComponent> skin : PlayerSkinComponents)
	{
		if (!skin)
			continue;

		USkinComponent* co = skin->GetDefaultObject<USkinComponent>();
		if (co->LockedToTeam != ETeamType::TT_NONE && co->LockedToTeam != ps->GetTeam())
			continue;

		ReturnData.Add(skin);
	}

	return ReturnData;
}

void UPreMissionPlanning::SetPrimaryScopeAttachment(TSubclassOf<UWeaponAttachment> ScopeAttachment)
{
	if (!ScopeAttachment)
		return;

	if (!bIsWeaponCustomization)
		return;
	
	ActiveLoadout.PrimaryScope = ScopeAttachment;
	
	if (EquippingSwatMember == EEquippingSwat::ES_None)
	{
		UpdatePrimaryWeaponAttachmentData();
	} else
	{
		SaveActiveLoadout();
	}
	
	SaveWeaponAttachments();
	UpdatePreviewWeaponAttachments(false, ScopeAttachment);

}

void UPreMissionPlanning::SetPrimaryMuzzleAttachment(TSubclassOf<class UWeaponAttachment> MuzzleAttachment)
{
	if (!MuzzleAttachment)
		return;

	if (!bIsWeaponCustomization)
		return;
	
	
	ActiveLoadout.PrimaryMuzzle = MuzzleAttachment;
	if (EquippingSwatMember == EEquippingSwat::ES_None)
	{
		UpdatePrimaryWeaponAttachmentData();
	}
	else
	{
		SaveActiveLoadout();
	}
	
	SaveWeaponAttachments();
	UpdatePreviewWeaponAttachments(false, MuzzleAttachment);
}

void UPreMissionPlanning::SetPrimaryOverbarrelAttachment(TSubclassOf<class UWeaponAttachment> OverbarrelAttachment)
{
	// attachment should never be null (they pass in a null class specifier if we want to remove attachments)
	if (!OverbarrelAttachment)
		return;
	
	if (!bIsWeaponCustomization)
		return;
	
	ActiveLoadout.PrimaryOverbarrel = OverbarrelAttachment;

	if (EquippingSwatMember == EEquippingSwat::ES_None)
	{
		UpdatePrimaryWeaponAttachmentData();
	} else
	{
		SaveActiveLoadout();
	}
	
	SaveWeaponAttachments();
	UpdatePreviewWeaponAttachments(false, OverbarrelAttachment);
}

void UPreMissionPlanning::SetPrimaryUnderbarrelAttachment(TSubclassOf<class UWeaponAttachment> UnderbarrelAttachment)
{
	if (!UnderbarrelAttachment)
		return;

	if (!bIsWeaponCustomization)
		return;
	
	ActiveLoadout.PrimaryUnderbarrel = UnderbarrelAttachment;
	if (EquippingSwatMember == EEquippingSwat::ES_None)
	{
		UpdatePrimaryWeaponAttachmentData();
	} else
	{
		SaveActiveLoadout();
	}
	
	SaveWeaponAttachments();
	UpdatePreviewWeaponAttachments(false, UnderbarrelAttachment);
}

void UPreMissionPlanning::SetPrimaryStockAttachment(const TSubclassOf<UWeaponAttachment> StockAttachment)
{
	if (!StockAttachment)
		return;

	if (!bIsWeaponCustomization)
		return;
	
	ActiveLoadout.PrimaryStock = StockAttachment;

	if (EquippingSwatMember == EEquippingSwat::ES_None)
	{
		UpdatePrimaryWeaponAttachmentData();
	} else
	{
		SaveActiveLoadout();
	}
	
	SaveWeaponAttachments();
	UpdatePreviewWeaponAttachments(false, StockAttachment);
}

void UPreMissionPlanning::SetPrimaryGripAttachment(const TSubclassOf<UWeaponAttachment> GripAttachment)
{
	if (!GripAttachment)
		return;

	if (!bIsWeaponCustomization)
		return;
	
	ActiveLoadout.PrimaryGrip = GripAttachment;
	
	if (EquippingSwatMember == EEquippingSwat::ES_None)
	{
		UpdatePrimaryWeaponAttachmentData();
	} else
	{
		SaveActiveLoadout();
	}
	
	SaveWeaponAttachments();
	UpdatePreviewWeaponAttachments(false, GripAttachment);
}

void UPreMissionPlanning::SetPrimaryIlluminatorAttachment(const TSubclassOf<UWeaponAttachment> IlluminatorAttachment)
{
	if (!IlluminatorAttachment)
		return;

	if (!bIsWeaponCustomization)
		return;
	
	ActiveLoadout.PrimaryIlluminator = IlluminatorAttachment;

	if (EquippingSwatMember == EEquippingSwat::ES_None)
	{	
		UpdatePrimaryWeaponAttachmentData();
	} else
	{
		SaveActiveLoadout();
	}
	
	SaveWeaponAttachments();
	UpdatePreviewWeaponAttachments(false, IlluminatorAttachment);
}

void UPreMissionPlanning::SetPrimaryAmmunitionAttachment(const TSubclassOf<UWeaponAttachment> AmmunitionAttachment)
{
	if (!AmmunitionAttachment)
		return;

	if (!bIsWeaponCustomization)
		return;

	ActiveLoadout.PrimaryAmmunition = AmmunitionAttachment;

	if (EquippingSwatMember == EEquippingSwat::ES_None)
	{
		UpdatePrimaryWeaponAttachmentData();
	}

	SaveWeaponAttachments();
	UpdatePreviewWeaponAttachments(false, AmmunitionAttachment);
}

void UPreMissionPlanning::SetPrimarySkinAttachment(const TSubclassOf<class USkinComponent> SkinAttachment)
{
	if (!SkinAttachment)
		return;

	if (!bIsWeaponCustomization)
		return;

	ActiveLoadout.PrimarySkin = SkinAttachment;
	if (EquippingSwatMember == EEquippingSwat::ES_None)
	{
		UpdatePrimaryWeaponAttachmentData();
	}
	else
	{
		SaveActiveLoadout();
	}

	SaveWeaponAttachments();
	UpdatePreviewWeaponSkin(false, SkinAttachment);
}

void UPreMissionPlanning::UpdatePreviewWeaponAttachments(bool IsSecondary, TSubclassOf<UWeaponAttachment> Attachment)
{
	if (!GetDefaultPreviewCharacter())
		return;
	
	ABaseWeapon* bw = !IsSecondary ? Cast<ABaseWeapon>(GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Primary) : Cast<ABaseWeapon>(GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Secondary);

	if (bw)
	{
		bw->AddAttachment(Attachment, false);
	}
}

void UPreMissionPlanning::SetSecondaryStockAttachment(const TSubclassOf<UWeaponAttachment> StockAttachment)
{
	if (!StockAttachment)
		return;

	if (!bIsWeaponCustomization)
		return;
	
	ActiveLoadout.SecondaryStock = StockAttachment;

	if (EquippingSwatMember == EEquippingSwat::ES_None)
	{			
		UpdateSecondaryWeaponAttachmentData();
	} else
	{
		SaveActiveLoadout();
	}
	
	SaveWeaponAttachments();
	UpdatePreviewWeaponAttachments(true, StockAttachment);
}

void UPreMissionPlanning::SetSecondaryGripAttachment(const TSubclassOf<UWeaponAttachment> GripAttachment)
{
	if (!GripAttachment)
		return;
		
	if (!bIsWeaponCustomization)
		return;
	
	ActiveLoadout.SecondaryGrip = GripAttachment;

	if (EquippingSwatMember == EEquippingSwat::ES_None)
	{
		UpdateSecondaryWeaponAttachmentData();
	} else
	{
		SaveActiveLoadout();
	}
	
	SaveWeaponAttachments();
	UpdatePreviewWeaponAttachments(true, GripAttachment);
}

void UPreMissionPlanning::SetSecondaryIlluminatorAttachment(const TSubclassOf<UWeaponAttachment> IlluminatorAttachment)
{
	if (!IlluminatorAttachment)
		return;

	if (!bIsWeaponCustomization)
		return;
	
	ActiveLoadout.SecondaryIlluminator = IlluminatorAttachment;
	
	if (EquippingSwatMember == EEquippingSwat::ES_None)
	{	
		UpdateSecondaryWeaponAttachmentData();
	} else
	{
		SaveActiveLoadout();
	}
	
	SaveWeaponAttachments();
	UpdatePreviewWeaponAttachments(true, IlluminatorAttachment);
}

void UPreMissionPlanning::SetSecondaryAmmunitionAttachment(const TSubclassOf<UWeaponAttachment> AmmunitionAttachment)
{
	if (!AmmunitionAttachment)
		return;

	if (!bIsWeaponCustomization)
		return;

	ActiveLoadout.SecondaryAmmunition = AmmunitionAttachment;

	if (EquippingSwatMember == EEquippingSwat::ES_None)
	{
		UpdateSecondaryWeaponAttachmentData();
	}
	else
	{
		SaveActiveLoadout();

	}

	SaveWeaponAttachments();
	UpdatePreviewWeaponAttachments(true, AmmunitionAttachment);
}

void UPreMissionPlanning::CleanPrimaryGun()
{
	ActiveLoadout.PrimaryScope = UBpGameplayHelperLib::GetItemData()->NullPrimaryScopeAttachment;
	ActiveLoadout.PrimaryMuzzle =  UBpGameplayHelperLib::GetItemData()->NullMuzzleAttachment;
	ActiveLoadout.PrimaryOverbarrel =  UBpGameplayHelperLib::GetItemData()->NullOverbarrelAttachment;
	ActiveLoadout.PrimaryUnderbarrel =  UBpGameplayHelperLib::GetItemData()->NullUnderbarrelAttachment;
	ActiveLoadout.PrimaryIlluminator = UBpGameplayHelperLib::GetItemData()->NullIlluminatorAttachment;
	ActiveLoadout.PrimaryGrip = UBpGameplayHelperLib::GetItemData()->NullGripAttachment;
	ActiveLoadout.PrimaryStock = UBpGameplayHelperLib::GetItemData()->NullStockAttachment;
	ActiveLoadout.PrimaryAmmunition = UBpGameplayHelperLib::GetItemData()->NullAmmunitionAttachment;
	ActiveLoadout.PrimarySkin =  UBpGameplayHelperLib::GetItemData()->FactorySkin;
	
	if (EquippingSwatMember == EEquippingSwat::ES_None)
	{
		UpdateItemAttachmentData(ActiveLoadout.Primary, FSavedWeaponAttachmentData());
	} else
	{
		SaveActiveLoadout();
	}
	
	ClearPreviewWeaponSkin(false);
}

void UPreMissionPlanning::ClearPreviewWeaponSkin(bool IsSecondary)
{
	if (!GetDefaultPreviewCharacter())
		return;

	ABaseWeapon* bw;

	if (IsSecondary)
	{
		bw = Cast<ABaseWeapon>(GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Secondary);
	}
	else
	{
		bw = Cast<ABaseWeapon>(GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Primary);
	}
	
	if (bw)
	{
		TArray<USkinComponent*> SkinComps;
		bw->GetComponents(SkinComps);
		for (int32 i = 0; i < SkinComps.Num(); i++)
		{
			USkinComponent* SkinComp = Cast<USkinComponent>(SkinComps[i]);
			if (SkinComp)
			{
				SkinComp->ResetSkin();
				SkinComp->DestroyComponent();
			}
		}
	}
}
void UPreMissionPlanning::SetSecondaryScopeAttachment(TSubclassOf<class UWeaponAttachment> ScopeAttachment)
{
	if (!ScopeAttachment)
		return;

	if (!bIsWeaponCustomization)
		return;
	
	ActiveLoadout.SecondaryScope = ScopeAttachment;

	if (EquippingSwatMember == EEquippingSwat::ES_None)
	{	
		UpdateSecondaryWeaponAttachmentData();
	} else
	{
		SaveActiveLoadout();	
	}
	
	SaveWeaponAttachments();
	UpdatePreviewWeaponAttachments(true, ScopeAttachment);
}

void UPreMissionPlanning::SetSecondaryMuzzleAttachment(TSubclassOf<class UWeaponAttachment> MuzzleAttachment)
{
	if (!MuzzleAttachment)
		return;

	if (!bIsWeaponCustomization)
		return;
	
	ActiveLoadout.SecondaryMuzzle = MuzzleAttachment;
	
	if (EquippingSwatMember == EEquippingSwat::ES_None)
	{
		UpdateSecondaryWeaponAttachmentData();
	} else
	{
		SaveActiveLoadout();
	}
	
	SaveWeaponAttachments();
	UpdatePreviewWeaponAttachments(true, MuzzleAttachment);
	
}

void UPreMissionPlanning::SetSecondaryOverbarrelAttachment(TSubclassOf<class UWeaponAttachment> OverbarrelAttachment)
{
	if (!OverbarrelAttachment)
		return;

	if (!bIsWeaponCustomization)
		return;
	
	ActiveLoadout.SecondaryOverbarrel = OverbarrelAttachment;

	if (EquippingSwatMember == EEquippingSwat::ES_None)
	{	
		UpdateSecondaryWeaponAttachmentData();
	} else
	{
		SaveActiveLoadout();
	}
	
	SaveWeaponAttachments();
	UpdatePreviewWeaponAttachments(true, OverbarrelAttachment);

}

void UPreMissionPlanning::SetSecondaryUnderbarrelAttachment(TSubclassOf<class UWeaponAttachment> UnderbarrelAttachment)
{
	if (!UnderbarrelAttachment)
		return;

	if (!bIsWeaponCustomization)
		return;
	
	ActiveLoadout.SecondaryUnderbarrel = UnderbarrelAttachment;
	
	if (EquippingSwatMember == EEquippingSwat::ES_None)
	{
		UpdateSecondaryWeaponAttachmentData();
	} else
	{
		SaveActiveLoadout();
	}
	
	SaveWeaponAttachments();
	UpdatePreviewWeaponAttachments(true, UnderbarrelAttachment);
	
}

void UPreMissionPlanning::SetSecondarySkinAttachment(TSubclassOf<class USkinComponent> SkinAttachment)
{
	if (!SkinAttachment)
		return;

	if (!bIsWeaponCustomization)
		return;
	
	ActiveLoadout.SecondarySkin = SkinAttachment;

	if (EquippingSwatMember == EEquippingSwat::ES_None)
	{
		UpdateSecondaryWeaponAttachmentData();
	} else
	{
		SaveActiveLoadout();
	}
	
	SaveWeaponAttachments();
	UpdatePreviewWeaponSkin(true, SkinAttachment);
}

void UPreMissionPlanning::UpdatePreviewWeaponSkin(bool IsSecondary, TSubclassOf<class USkinComponent> SkinAttachment)
{
	if (!GetDefaultPreviewCharacter())
		return;

	ABaseWeapon* bw;

	if (IsSecondary)
	{
		bw = Cast<ABaseWeapon>(GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Secondary);
	}
	else
	{
		bw = Cast<ABaseWeapon>(GetDefaultPreviewCharacter()->GetInventoryComponent()->GetSpawnedGear().Primary);
	}
	
	if (bw)
	{
		if (SkinAttachment)
		{
			USkinComponent* SkinComp = NewObject<USkinComponent>(bw, SkinAttachment);
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

void UPreMissionPlanning::CleanSecondaryGun()
{
	ActiveLoadout.SecondaryScope = UBpGameplayHelperLib::GetItemData()->NullPrimaryScopeAttachment;
	ActiveLoadout.SecondaryMuzzle =  UBpGameplayHelperLib::GetItemData()->NullMuzzleAttachment;
	ActiveLoadout.SecondaryOverbarrel =  UBpGameplayHelperLib::GetItemData()->NullOverbarrelAttachment;
	ActiveLoadout.SecondaryUnderbarrel =  UBpGameplayHelperLib::GetItemData()->NullUnderbarrelAttachment;
	ActiveLoadout.SecondaryIlluminator = UBpGameplayHelperLib::GetItemData()->NullIlluminatorAttachment;
	ActiveLoadout.SecondaryGrip = UBpGameplayHelperLib::GetItemData()->NullGripAttachment;
	ActiveLoadout.SecondaryStock = UBpGameplayHelperLib::GetItemData()->NullStockAttachment;
	ActiveLoadout.SecondaryAmmunition = UBpGameplayHelperLib::GetItemData()->NullAmmunitionAttachment;
	ActiveLoadout.SecondarySkin =  UBpGameplayHelperLib::GetItemData()->FactorySkin;
	if (EquippingSwatMember == EEquippingSwat::ES_None)
	{
		UpdateItemAttachmentData(ActiveLoadout.Secondary, FSavedWeaponAttachmentData());
	
	} else
	{
		SaveActiveLoadout();
	}
	
	ClearPreviewWeaponSkin(true);
}

EItemType UPreMissionPlanning::ItemClassToItemType(const EItemClass InItemClass) const
{
	switch (InItemClass)
	{
		case EItemClass::IC_NoClass:
			return EItemType::IT_None;
		case EItemClass::IC_AssaultRifle:
			return EItemType::IT_Rifles;
		case EItemClass::IC_SMG:
			return EItemType::IT_SubmachineGun;
		case EItemClass::IC_LMG:
			return EItemType::IT_LightMachineGun;
		case EItemClass::IC_Pistol:
			return EItemType::IT_PistolsLethal;
		case EItemClass::IC_Sniper:
			return EItemType::IT_Sniper;
		case EItemClass::IC_Melee:
			return EItemType::IT_Melee;
		case EItemClass::IC_LessLethal:
			return EItemType::IT_LessLethal;
		case EItemClass::IC_Shotgun:
			return EItemType::IT_Shotgun;
		case EItemClass::IC_Launcher:
			return EItemType::IT_None;
		case EItemClass::IC_Grenade:
			return EItemType::IT_None;
		case EItemClass::IC_Shield:
			return EItemType::IT_None;
		case EItemClass::IC_Armor:
			return EItemType::IT_BodyArmor;
		case EItemClass::IC_TacticalDevice:
			return EItemType::IT_None;
		case EItemClass::IC_LongTactical:
			return EItemType::IT_LongTactical;
		default:
			return EItemType::IT_None;
	}
}

void UPreMissionPlanning::AttemptEquipLoadoutInGame()
{
	// Don't equip the player in Training
	if (GetWorld()->GetGameState<ATrainingGS>())
		return;

	if (LOCAL_PLAYER)
	{
		if (const AReadyOrNotPlayerState* PlayerState = LocalPlayer->GetPlayerState<AReadyOrNotPlayerState>())
		{
			UBpGameplayHelperLib::LoadLoadout(ActiveLoadout, "default");
			LocalPlayer->GetInventoryComponent()->Server_AttemptEquipNewLoadout(ActiveLoadout);
		}
	}
}

void UPreMissionPlanning::LoadLoadoutPresets()
{
	UBaseProfile* Profile = UBaseProfile::GetCurrentProfile();
	if (!Profile)
		return;

	//LoadoutPresetMap = Profile->LoadoutPresetMap;
	OnLoadoutPresetsLoaded();
}

void UPreMissionPlanning::SaveLoadoutPresets()
{
	UBaseProfile* Profile = UBaseProfile::GetCurrentProfile();
	if (!Profile)
		return;

	//Profile->LoadoutPresetMap = LoadoutPresetMap;
	Profile->SaveProfile();

	OnLoadoutPresetsSaved();
}

void UPreMissionPlanning::LoadWeaponPresets()
{
	UBaseProfile* Profile = UBaseProfile::GetCurrentProfile();
	if (!Profile)
		return;

	WeaponToWeaponPresetsMap = Profile->WeaponToWeaponPresetsMap;
	OnLoadoutItemPresetsLoaded();
}

void UPreMissionPlanning::SaveWeaponPresets()
{
	UBaseProfile* Profile = UBaseProfile::GetCurrentProfile();
	if (!Profile)
		return;

	Profile->WeaponToWeaponPresetsMap = WeaponToWeaponPresetsMap;
	Profile->SaveProfile();

	OnLoadoutItemPresetsSaved();
}

void UPreMissionPlanning::LoadWeaponAttachments()
{
	UBaseProfile* Profile = UBaseProfile::GetCurrentProfile();
	if (!Profile)
		return;

	WeaponToAttachmentsMap = Profile->AttachmentSaveMap;
	OnLoadoutItemAttachmentsLoaded();
}

void UPreMissionPlanning::SaveWeaponAttachments()
{
	UBaseProfile* Profile = UBaseProfile::GetCurrentProfile();
	if (!Profile)
		return;

	Profile->AttachmentSaveMap = WeaponToAttachmentsMap;
	Profile->SaveProfile();

	OnLoadoutItemAttachmentsSaved();
}

UReadyOrNotSaveGame* UPreMissionPlanning::LoadGame()
{
	return UBpGameplayHelperLib::GetLoadGameInstance();
}

void UPreMissionPlanning::SaveGame()
{
	if (UReadyOrNotSaveGame* lg = UBpGameplayHelperLib::GetLoadGameInstance())
	{
		UGameplayStatics::SaveGameToSlot(lg, lg->SaveSlotName, lg->UserIndex);
	}
}

#undef LOCTEXT_NAMESPACE
