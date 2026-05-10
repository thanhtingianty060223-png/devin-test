// Copyright Void Interactive, 2024

#include "CustomizationData.h"

#include "SkinnedDecalSampler.h"
#include "Actors/Items/Headwear.h"

void FSavedCustomization::SetCustomizationItem(UCustomizationDataBase* Item)
{
	if (!Item)
		return;
	
	switch (Item->Type)
	{
	case ECustomizationType::Character: Character = Item; break;
	case ECustomizationType::Voice: Voice = Item; break;
	case ECustomizationType::Helmet: Helmet = Item; break;
	case ECustomizationType::Shirt: Shirt = Item; break;
	case ECustomizationType::Pants: Pants = Item; break;
	case ECustomizationType::Shoes: Shoes = Item; break;
	case ECustomizationType::Eyewear: Eyewear = Item; break;
	case ECustomizationType::Belt: Belt = Item; break;
	case ECustomizationType::Gloves: Gloves = Item; break;
	case ECustomizationType::Watch: Watch = Item; break;
	case ECustomizationType::Tattoo: Tattoo = Item; break;
	case ECustomizationType::PrimarySkin: PrimarySkin = Item; break;
	case ECustomizationType::SecondarySkin: SecondarySkin = Item; break;
	case ECustomizationType::ArmorSkin: ArmorSkin = Item; break;
	case ECustomizationType::HeadwearSkin: HeadwearSkin = Item; break;
	default: break;
	}
}

UCustomizationDataBase* FSavedCustomization::GetCustomizationItem(ECustomizationType Type)
{
	switch (Type)
	{
	case ECustomizationType::Character: return Character;
	case ECustomizationType::Voice: return Voice;
	case ECustomizationType::Helmet: return Helmet;
	case ECustomizationType::Shirt: return Shirt;
	case ECustomizationType::Pants: return Pants;
	case ECustomizationType::Shoes: return Shoes;
	case ECustomizationType::Eyewear: return Eyewear;
	case ECustomizationType::Belt: return Belt;
	case ECustomizationType::Gloves: return Gloves;
	case ECustomizationType::Watch: return Watch;
	case ECustomizationType::Tattoo: return Tattoo;
	case ECustomizationType::PrimarySkin: return PrimarySkin;
	case ECustomizationType::SecondarySkin: return SecondarySkin;
	case ECustomizationType::ArmorSkin: return ArmorSkin;
	case ECustomizationType::HeadwearSkin: return HeadwearSkin;
	default: return nullptr;
	}
}

USkeletalMeshComponent* SetupSkeletalMesh(AReadyOrNotCharacter* Target, UCustomizationSkeletalMesh* DataAsset)
{
	if (!Target || !DataAsset->SkeletalMesh.LoadSynchronous())
		return nullptr;

	FName ComponentName = FName(FString(TEXT("SkeletalMesh_")) + DataAsset->GetName());
	USkeletalMeshComponent* Component = NewObject<USkeletalMeshComponent>(Target, ComponentName, RF_Transient);
	if (!Component)
		return nullptr;
	
	Component->SetupAttachment(Target->GetMesh(), DataAsset->Socket);
	Component->SetSkeletalMesh(DataAsset->SkeletalMesh.LoadSynchronous());
	
	if (DataAsset->bUseMasterPose)
		Component->SetMasterPoseComponent(Target->GetMesh());

	for (FCustomizationMaterialSlot& Override : DataAsset->MaterialOverrides)
	{
		Component->SetMaterial(Override.Slot, Override.Material.LoadSynchronous());
	}
	
	Component->AddTickPrerequisiteComponent(Target->GetMesh());
	Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Component->SetReceivesDecals(false);
	Component->bOwnerNoSee = true;
	Component->bUseBoundsFromMasterPoseComponent = true;
	Component->bCastHiddenShadow = true;
	Component->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	Component->RegisterComponent();

	Target->CustomizationSkeletalMeshes.Add(Component);
	Target->SocketOverridesMap.Append(DataAsset->SocketOverridesMap);
	return Component;
}

UStaticMeshComponent* SetupStaticMesh(AReadyOrNotCharacter* Target, UCustomizationStaticMesh* DataAsset)
{
	if (!Target || !DataAsset->StaticMesh.LoadSynchronous())
		return nullptr;

	FName ComponentName = FName(FString(TEXT("StaticMesh_")) + DataAsset->GetName());
	UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(Target, ComponentName, RF_Transient);
	if (!Component)
		return nullptr;

	Component->SetupAttachment(Target->GetMesh(), DataAsset->Socket);
	Component->SetStaticMesh(DataAsset->StaticMesh.LoadSynchronous());
	Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Component->SetReceivesDecals(false);
	Component->AddTickPrerequisiteComponent(Target->GetMesh());
	Component->AlwaysLoadOnClient = true;
	Component->AlwaysLoadOnServer = true;
	Component->bOwnerNoSee = true;
	Component->bCastHiddenShadow = true;
	Component->RegisterComponent();

	Target->CustomizationStaticMeshes.Add(Component);
	return Component;
}

AActor* SetupBlueprintCustomization(AReadyOrNotCharacter* Target, UCustomizationBlueprint* DataAsset)
{
	if (!Target || !Target->GetWorld() || !DataAsset->BlueprintClass.LoadSynchronous())
		return nullptr;

	if (Target->IsLocalPlayer())
		return nullptr;
	
	UWorld* World = Target->GetWorld();
	
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = Target;
	
	AActor* Actor = World->SpawnActor<AActor>(DataAsset->BlueprintClass.LoadSynchronous(), SpawnParameters);
	if (!ensure(Actor))
		return nullptr;

	if (!DataAsset->bTickInThirdPerson)
		Actor->SetActorTickEnabled(false);
	
	Actor->AttachToComponent(Target->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, DataAsset->Socket);
	
	TArray<UMeshComponent*> MeshComponents;
	Actor->GetComponents<UMeshComponent>(MeshComponents);
	for (UMeshComponent* MeshComponent : MeshComponents)
	{
		MeshComponent->AddTickPrerequisiteComponent(Target->GetMesh());
		MeshComponent->SetReceivesDecals(false);
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MeshComponent->bOwnerNoSee = true;
		MeshComponent->bCastHiddenShadow = true;
		MeshComponent->bUseAttachParentBound = true;

		if (USkeletalMeshComponent* SkeletalMesh = Cast<USkeletalMeshComponent>(MeshComponent))
		{
			SkeletalMesh->bUseBoundsFromMasterPoseComponent = true;
			SkeletalMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
		}
	}

	Target->CustomizationActors.Add(Actor);
	return Actor;
}

AActor* SetupBlueprintCustomizationFirstPerson(APlayerCharacter* Target, UCustomizationBlueprint* DataAsset)
{
	if (!Target || !Target->GetWorld() || !DataAsset->BlueprintClass.LoadSynchronous())
		return nullptr;
	
	UWorld* World = Target->GetWorld();
	
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = Target;
	
	AActor* Actor = World->SpawnActor<AActor>(DataAsset->BlueprintClass.LoadSynchronous(), SpawnParameters);
	if (!ensure(Actor))
		return nullptr;
	
	Actor->AttachToComponent(Target->GetMesh1P(), FAttachmentTransformRules::SnapToTargetIncludingScale, DataAsset->Socket);

	TMap<UMaterialInterface*, UMaterialInstanceDynamic*> DynamicMaterialMap;
	
	TArray<UMeshComponent*> MeshComponents;
	Actor->GetComponents<UMeshComponent>(MeshComponents);
	for (UMeshComponent* MeshComponent : MeshComponents)
	{
		MeshComponent->SetOnlyOwnerSee(true);
		MeshComponent->SetReceivesDecals(false);
		MeshComponent->AddTickPrerequisiteComponent(Target->GetMesh());
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MeshComponent->CastShadow = true;
		MeshComponent->bSelfShadowOnly = true;
		MeshComponent->bUseAttachParentBound = true;
		
		TArray<UMaterialInterface*> Materials = MeshComponent->GetMaterials();
		for (int32 i = 0; i < Materials.Num(); i++)
		{
			UMaterialInterface* MaterialInterface = Materials[i];
			if (DynamicMaterialMap.Contains(MaterialInterface))
			{
				MeshComponent->SetMaterial(i, DynamicMaterialMap[MaterialInterface]);
			}
			else
			{
				UMaterialInstanceDynamic* MaterialInstanceDynamic = MeshComponent->CreateAndSetMaterialInstanceDynamic(i);
				DynamicMaterialMap.Add(MaterialInterface, MaterialInstanceDynamic);
			}
		}
		
		if (USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(MeshComponent))
		{
			SkeletalMeshComponent->SetMasterPoseComponent(Target->GetMesh1P());
			SkeletalMeshComponent->bUpdateOverlapsOnAnimationFinalize = false;
			SkeletalMeshComponent->bSimulationUpdatesChildTransforms = false;
			SkeletalMeshComponent->bUseAttachParentBound = true;
		}
	}

	TArray<UMaterialInstanceDynamic*> DynamicMaterials;
	DynamicMaterialMap.GenerateValueArray(DynamicMaterials);
	
	Target->CustomizationActors.Add(Actor);
	Target->CustomizationActorMaterials.Append(DynamicMaterials);
	return Actor;
}

void SetupGenericCustomization(AReadyOrNotCharacter* Target, UCustomizationDataBase* DataAsset, const TSet<ECustomizationType>& HiddenTypes, bool bAddToSkinnedDecal)
{
	if (!Target || !DataAsset)
		return;

	if (HiddenTypes.Contains(DataAsset->Type))
		return;
	
	if (UCustomizationStaticMesh* StaticMesh = Cast<UCustomizationStaticMesh>(DataAsset))
	{
		SetupStaticMesh(Target, StaticMesh);
	}
	else if (UCustomizationSkeletalMesh* SkeletalMesh = Cast<UCustomizationSkeletalMesh>(DataAsset))
	{
		USkeletalMeshComponent* SkeletalMeshComponent = SetupSkeletalMesh(Target, SkeletalMesh);
		if (SkeletalMeshComponent && bAddToSkinnedDecal)
		{
			Target->GetSkinnedDecalSampler()->SetMeshComponent(SkeletalMeshComponent, true);
		}
	}
	else if (UCustomizationBlueprint* Blueprint = Cast<UCustomizationBlueprint>(DataAsset))
	{
		SetupBlueprintCustomization(Target, Blueprint);
	}
}

USkeletalMeshComponent* SetupSkeletalMeshFirstPerson(APlayerCharacter* Target, UCustomizationSkeletalMesh* DataAsset, bool bBodyMesh)
{
	if (!Target || !DataAsset->SkeletalMesh.LoadSynchronous())
		return nullptr;

	FName ComponentName = FName(FString(TEXT("SkeletalMesh1P_")) + DataAsset->GetName());
	USkeletalMeshComponent* Component = NewObject<USkeletalMeshComponent>(Target, ComponentName, RF_Transient);
	if (!Component)
		return nullptr;
	
	Component->SetupAttachment(bBodyMesh ? Target->GetMeshBody1P() : Target->GetMesh1P(), DataAsset->Socket);
	Component->SetSkeletalMesh(DataAsset->SkeletalMesh.LoadSynchronous());
	
	if (DataAsset->bUseMasterPose)
		Component->SetMasterPoseComponent(Target->GetMesh1P());
	
	for (FCustomizationMaterialSlot& Override : DataAsset->MaterialOverrides)
	{
		Component->SetMaterial(Override.Slot, Override.Material.LoadSynchronous());
	}
	
	Component->AddTickPrerequisiteComponent(Target->GetMesh1P());
	Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Component->SetReceivesDecals(false);
	Component->SetOnlyOwnerSee(true);
	Component->CastShadow = true;
	Component->bSelfShadowOnly = true;
	Component->bCastContactShadow = false;
	Component->bUpdateOverlapsOnAnimationFinalize = false;
	Component->bSimulationUpdatesChildTransforms = false;
	Component->bUseAttachParentBound = true;
	Component->RegisterComponent();

	if (bBodyMesh)
	{
		Target->CustomizationFirstPersonBodyMeshes.Add(Component);
	}
	else
	{
		Target->CustomizationFirstPersonMeshes.Add(Component);
	}
	
	return Component;
}

UStaticMeshComponent* SetupStaticMeshFirstPerson(APlayerCharacter* Target, UCustomizationStaticMesh* DataAsset)
{
	if (!Target || !DataAsset->StaticMesh.LoadSynchronous())
		return nullptr;

	FName ComponentName = FName(FString(TEXT("StaticMesh_")) + DataAsset->GetName());
	UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(Target, ComponentName, RF_Transient);
	if (!Component)
		return nullptr;
	
	Component->SetupAttachment(Target->GetMesh1P(), DataAsset->Socket);
	Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Component->SetStaticMesh(DataAsset->StaticMesh.LoadSynchronous());
	Component->SetOnlyOwnerSee(true);
	Component->SetReceivesDecals(false);
	Component->AddTickPrerequisiteComponent(Target->GetMesh());
	Component->CastShadow = true;
	Component->bSelfShadowOnly = true;
	Component->bCastContactShadow = false;
	Component->RegisterComponent();
	
	Target->CustomizationFirstPersonMeshes.Add(Component);
	return Component;
}

void SetupGenericMeshFirstPerson(APlayerCharacter* Target, UCustomizationDataBase* DataAsset, const TSet<ECustomizationType>& HiddenTypes, bool bBodyMesh)
{
	if (!Target || !DataAsset)
		return;

	if (HiddenTypes.Contains(DataAsset->Type))
		return;

	if (UCustomizationStaticMesh* StaticMesh = Cast<UCustomizationStaticMesh>(DataAsset))
	{
		SetupStaticMeshFirstPerson(Target, StaticMesh);
	}
	else if (UCustomizationSkeletalMesh* SkeletalMesh = Cast<UCustomizationSkeletalMesh>(DataAsset))
	{
		SetupSkeletalMeshFirstPerson(Target, SkeletalMesh, bBodyMesh);
	}
	else if (UCustomizationBlueprint* Blueprint = Cast<UCustomizationBlueprint>(DataAsset))
	{
		SetupBlueprintCustomizationFirstPerson(Target, Blueprint);
	}
}

USkeletalMeshComponent* SetupCharacterFaceMesh(AReadyOrNotCharacter* Target, USkeletalMesh* SkeletalMesh)
{
	if (!Target || !SkeletalMesh)
		return nullptr;
	
	USkeletalMeshComponent* Component = NewObject<USkeletalMeshComponent>(Target, "CustomizationFaceMesh", RF_Transient);
	if (!Component)
		return nullptr;
	
	Component->SetupAttachment(Target->GetMesh());
	Component->SetSkeletalMesh(SkeletalMesh);
	Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Component->AddTickPrerequisiteComponent(Target->GetMesh());
	Component->SetReceivesDecals(false);
	Component->AlwaysLoadOnClient = true;
	Component->AlwaysLoadOnServer = true;
	Component->bOwnerNoSee = true;
	Component->bCastHiddenShadow = true;
	Component->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	Component->bEnableUpdateRateOptimizations = false;
	Component->bUseAttachParentBound = true;
	
	Component->RegisterComponent();

	Target->CustomizationSkeletalMeshes.Add(Component);
	return Component;
}

void ApplyCharacterCustomization(AReadyOrNotCharacter* Target, FSavedCustomization& SavedCustomization, const TSet<ECustomizationType>& HiddenTypes)
{
	UCustomizationCharacter* Character = Cast<UCustomizationCharacter>(SavedCustomization.Character);
	if (!Character)
		return;

	USkeletalMeshComponent* HeadMesh = SetupCharacterFaceMesh(Target, Character->HeadMesh.LoadSynchronous());
	if (!HeadMesh)
		return;
	
	HeadMesh->SetAnimInstanceClass(Target->GetFaceMesh()->GetAnimClass());
	HeadMesh->SetMasterPoseComponent(nullptr, true);
	
	Target->CustomizationFaceMesh = HeadMesh;
	Target->SetCurrentFaceROM(Character->FaceROM.LoadSynchronous());
	
	UInventoryComponent* InventoryComponent = Target->GetInventoryComponent();
	if (SavedCustomization.Helmet)
	{
		SetupGenericCustomization(Target, SavedCustomization.Helmet, HiddenTypes, false);
		HeadMesh->ShowMaterialSection(Character->HairMaterialIndex, 0, false, 0);
	}
}

void ApplyVoiceCustomization(AReadyOrNotCharacter* Target, FSavedCustomization& SavedCustomization)
{
	UCustomizationVoice* Voice = Cast<UCustomizationVoice>(SavedCustomization.Voice);
	if (!Voice)
		return;

	Target->SetSpeechCharacterName(Voice->VoiceHandle.ToString());
}

void ApplyArmsCustomization(AReadyOrNotCharacter* Target, FSavedCustomization& SavedCustomization)
{
	UCustomizationCharacter* Character = Cast<UCustomizationCharacter>(SavedCustomization.Character);
	if (!Character)
		return;
	
	UCustomizationSkeletalMesh* Arms = Cast<UCustomizationSkeletalMesh>(SavedCustomization.Gloves);
	if (!Arms)
		return;

	if (!Character->ArmsMaterial.LoadSynchronous())
		return;
	
	UCustomizationTattoo* Tattoo = Cast<UCustomizationTattoo>(SavedCustomization.Tattoo);
	USkeletalMeshComponent* ArmsMesh = SetupSkeletalMesh(Target, Arms);

	int32 HackSlot = ArmsMesh->GetNumMaterials() <= 1 ? 0 : 1; // no gloves has one slot, gloves have two (and put skin in 2nd slot)
	UMaterialInstanceDynamic* ArmsMaterialInstance =
		ArmsMesh->CreateAndSetMaterialInstanceDynamicFromMaterial(HackSlot, Character->ArmsMaterial.LoadSynchronous());
	
	if (Tattoo && ArmsMaterialInstance)
	{
		UTexture2D* TattooTexture = Tattoo->TattooTexture.LoadSynchronous();
		ArmsMaterialInstance->SetTextureParameterValue("TattooTexture", TattooTexture);
	}
}

void ApplyArmsCustomizationFirstPerson(APlayerCharacter* Target, FSavedCustomization& SavedCustomization)
{
	UCustomizationCharacter* Character = Cast<UCustomizationCharacter>(SavedCustomization.Character);
	if (!Character)
		return;
	
	UCustomizationSkeletalMesh* Arms = Cast<UCustomizationSkeletalMesh>(SavedCustomization.Gloves);
	if (!Arms)
		return;

	if (!Character->ArmsMaterial.LoadSynchronous())
		return;
	
	UCustomizationTattoo* Tattoo = Cast<UCustomizationTattoo>(SavedCustomization.Tattoo);
	USkeletalMeshComponent* ArmsMesh = SetupSkeletalMeshFirstPerson(Target, Arms, false);

	int32 HackSlot = ArmsMesh->GetNumMaterials() <= 1 ? 0 : 1; // no gloves has one slot, gloves have two (and put skin in 2nd slot)
	UMaterialInstanceDynamic* ArmsMaterialInstance =
		ArmsMesh->CreateAndSetMaterialInstanceDynamicFromMaterial(HackSlot, Character->ArmsMaterial.LoadSynchronous());
	
	if (Tattoo && ArmsMaterialInstance)
	{
		UTexture2D* TattooTexture = Tattoo->TattooTexture.LoadSynchronous();
		ArmsMaterialInstance->SetTextureParameterValue("TattooTexture", TattooTexture);
	}
}

void AddItemHiddenTypes(TSet<ECustomizationType>& Types, const UCustomizationDataBase* Item)
{
	if (!Item)
		return;

	Types.Append(Item->TypesToHide);
}

void AddAllHiddenItemTypes(TSet<ECustomizationType>& Types, const FSavedCustomization& Customization)
{
	AddItemHiddenTypes(Types, Customization.Character);
	AddItemHiddenTypes(Types, Customization.Voice);
	AddItemHiddenTypes(Types, Customization.Helmet);
	AddItemHiddenTypes(Types, Customization.Shirt);
	AddItemHiddenTypes(Types, Customization.Pants);
	AddItemHiddenTypes(Types, Customization.Shoes);
	AddItemHiddenTypes(Types, Customization.Eyewear);
	AddItemHiddenTypes(Types, Customization.Belt);
	AddItemHiddenTypes(Types, Customization.Gloves);
	AddItemHiddenTypes(Types, Customization.Watch);
	AddItemHiddenTypes(Types, Customization.Tattoo);
	AddItemHiddenTypes(Types, Customization.PrimarySkin);
	AddItemHiddenTypes(Types, Customization.SecondarySkin);
	AddItemHiddenTypes(Types, Customization.ArmorSkin);
	AddItemHiddenTypes(Types, Customization.HeadwearSkin);
}

void FSavedCustomization::ApplyCustomization(AReadyOrNotCharacter* Target)
{
	if (!IsValid(Target))
		return;

	Target->GetMesh()->SetVisibility(false, false);
	Target->GetMesh()->SetCastHiddenShadow(false);

	Target->GetFaceMesh()->SetVisibility(false, false);
	Target->GetFaceMesh()->SetCastHiddenShadow(false);
	Target->GetMeshGearSlot()->SetSkeletalMesh(nullptr);
	
	// Hack to get the indirect soft shadows going again
	// May need to create a second fake parent mesh so we can light all customization as a parent
	// That way the hidden parent mesh won't cast a direct shadow while still keeping light attach grouping
	Target->GetMesh()->SetCastHiddenShadow(true);
	Target->GetMesh()->SetLightingChannels(false, false, false);
	Target->GetMesh()->SetLightAttachmentsAsGroup(false);

	ClearCustomization(Target);
	
	TSet<ECustomizationType> HiddenTypes;
	AddAllHiddenItemTypes(HiddenTypes, *this);
	
	ApplyCharacterCustomization(Target, *this, HiddenTypes);
	ApplyVoiceCustomization(Target, *this);
	ApplyArmsCustomization(Target, *this);
	
	SetupGenericCustomization(Target, Shirt, HiddenTypes, true);
	SetupGenericCustomization(Target, Pants, HiddenTypes, true);
	SetupGenericCustomization(Target, Shoes, HiddenTypes, false);
	SetupGenericCustomization(Target, Eyewear, HiddenTypes, false);
	SetupGenericCustomization(Target, HeadwearSkin, HiddenTypes, false);
	SetupGenericCustomization(Target, Belt, HiddenTypes, false);
	SetupGenericCustomization(Target, Watch, HiddenTypes, false);

	// Ensure the meshes we've just added get set up and remove any new nulls
	Target->GetSkinnedDecalSampler()->SetupMaterials();
	Target->GetSkinnedDecalSampler()->RenderMeshes.Remove(nullptr);

	// Reattach all gear, forces sockets to update
	if (Target->GetInventoryComponent())
	{
		Target->GetInventoryComponent()->ReattachAllGear();
	}
	
	/* First Person Meshes*/
	AReadyOrNotPlayerController* PlayerController = Target->GetController<AReadyOrNotPlayerController>();
	if (!PlayerController || !PlayerController->IsLocalController())
		return;

	Target->GetMesh()->SetBoundsScale(1.5f); // Helps a lot with shadow clipping issues especially when leaning
	
	APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(Target);
	if (!PlayerCharacter)
		return;

	PlayerCharacter->GetMesh1P()->SetVisibility(false);
	PlayerCharacter->GetMeshBody1P()->SetVisibility(false);

	ApplyArmsCustomizationFirstPerson(PlayerCharacter, *this);
	
	SetupGenericMeshFirstPerson(PlayerCharacter, Shirt, HiddenTypes, false);
	SetupGenericMeshFirstPerson(PlayerCharacter, Watch, HiddenTypes, false);
	SetupGenericMeshFirstPerson(PlayerCharacter, Pants, HiddenTypes, true);
	SetupGenericMeshFirstPerson(PlayerCharacter, Shoes, HiddenTypes, true);
	
	// Set dirty so player can update fov shaders
	PlayerCharacter->bFirstPersonMeshesDirty = true;
}

void SetSpawnedGearSkin(ABaseItem* Target, UCustomizationDataBase* DataAsset)
{
	if (Target)
	{
		Target->Skin = Cast<UCustomizationSkin>(DataAsset);
		Target->OnRep_Skin();
	}
}

void FSavedCustomization::ApplyCustomizationSkins(AReadyOrNotCharacter* Target)
{
	if (!Target || !Target->GetInventoryComponent())
		return;

	FSpawnedGear& SpawnedGear = Target->GetInventoryComponent()->GetSpawnedGear();
	SetSpawnedGearSkin(SpawnedGear.Primary, PrimarySkin);
	SetSpawnedGearSkin(SpawnedGear.Secondary, SecondarySkin);
	SetSpawnedGearSkin(SpawnedGear.Armor, ArmorSkin);
	SetSpawnedGearSkin(SpawnedGear.Helmet, HeadwearSkin);
}

void FSavedCustomization::ApplyItemCustomization(ABaseItem* Target, UCustomizationSkin* Skin)
{
	if (!Target || !Skin)
		return;

	FName ItemTag = Target->CustomizationTag;
	if (!Skin->CompatibleItemTags.Contains(ItemTag))
	{
		UE_LOG(LogReadyOrNotLoadout, Warning, TEXT("Tried to set incompatible skin %s for item %s"), *Target->GetName(), *Skin->GetName());
		return;
	}

	USkeletalMesh* DefaultMesh = nullptr;
	ABaseItem* ItemCDO = Target->GetClass()->GetDefaultObject<ABaseItem>();
	if (ItemCDO && ItemCDO->ItemMesh)
		DefaultMesh = ItemCDO->ItemMesh->SkeletalMesh;
	
	USkeletalMeshComponent* SkeletalMeshComponent = Target->GetItemMesh();
	USkeletalMesh* MeshOverride = Skin ? Skin->MeshOverride.LoadSynchronous() : nullptr;
	
	if (SkeletalMeshComponent)
		SkeletalMeshComponent->SetSkeletalMesh(MeshOverride ? MeshOverride : DefaultMesh);
	
	if (!SkeletalMeshComponent || !SkeletalMeshComponent->SkeletalMesh)
		return;
	
	// Reset materials
	TArray<FSkeletalMaterial>& SkeletalMaterials = SkeletalMeshComponent->SkeletalMesh->GetMaterials();
	for (int32 i = 0; i < SkeletalMaterials.Num(); i++)
	{
		SkeletalMeshComponent->SetMaterial(i, SkeletalMaterials[i].MaterialInterface);
	}

	// Reset body socket
	Target->BodySocket = GetDefault<ABaseItem>(Target->GetClass())->BodySocket;
	
	if (Skin)
	{
		// Set skin materials
		for (FCustomizationMaterialSlot& CustomizationMaterialSlot : Skin->MaterialSlots)
		{
			SkeletalMeshComponent->SetMaterial(CustomizationMaterialSlot.Slot, CustomizationMaterialSlot.Material.LoadSynchronous());
		}

		// Set override socket
		if (Skin->bUseSocketOverride)
		{
			Target->BodySocket = Skin->SocketOverride;
			Target->HandsSocket = Skin->SocketOverride;
		}
	}

	if (Target->GetOwnerCharacter() && Target->GetOwnerCharacter()->GetInventoryComponent())
	{
		Target->GetOwnerCharacter()->GetInventoryComponent()->ReattachAllGear();
	}
}

void FSavedCustomization::ClearCustomization(AReadyOrNotCharacter* Target)
{
	/*
	 * Clear out any third person meshes
	 */
	if (!Target)
		return;
	
	for (USkeletalMeshComponent* SkeletalMesh : Target->CustomizationSkeletalMeshes)
	{
		if (IsValid(SkeletalMesh))
			SkeletalMesh->DestroyComponent();
	}
	Target->CustomizationSkeletalMeshes.Empty();

	for (UStaticMeshComponent* StaticMesh : Target->CustomizationStaticMeshes)
	{
		if (IsValid(StaticMesh))
			StaticMesh->DestroyComponent();
	}
	Target->CustomizationStaticMeshes.Empty();

	for (AActor* Actor : Target->CustomizationActors)
	{
		if (IsValid(Actor))
			Actor->Destroy();
	}

	Target->SocketOverridesMap.Empty();

	/*
	 * Clear out first person meshes if applicable
	 */
	APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(Target);
	if (!PlayerCharacter)
		return;
	
	for (UMeshComponent* MeshComponent : PlayerCharacter->CustomizationFirstPersonMeshes)
	{
		if (MeshComponent)
			MeshComponent->DestroyComponent();
	}
	PlayerCharacter->CustomizationFirstPersonMeshes.Empty();

	for (UMeshComponent* MeshComponent : PlayerCharacter->CustomizationFirstPersonBodyMeshes)
	{
		if (MeshComponent)
			MeshComponent->DestroyComponent();
	}
	PlayerCharacter->CustomizationFirstPersonBodyMeshes.Empty();
	
	PlayerCharacter->CustomizationActorMaterials.Empty();
}

void FSavedCustomization::Sanitize()
{
	UItemData* ItemData = UBpGameplayHelperLib::GetItemData();
	if (!ItemData)
		return;

	const FSavedCustomization& DefaultCustomization = ItemData->DefaultCustomization;
	
	if (!Character || Character->Type != ECustomizationType::Character)
		Character = DefaultCustomization.Character;

	if (!Voice || Voice->Type != ECustomizationType::Voice)
		Voice = DefaultCustomization.Voice;

	if (bHasHelmet && (!Helmet || Helmet->Type != ECustomizationType::Helmet))
		Helmet = DefaultCustomization.Helmet;
	
	if (!Shirt || Shirt->Type != ECustomizationType::Shirt)
		Shirt = DefaultCustomization.Shirt;
	
	if (!Pants || Pants->Type != ECustomizationType::Pants)
		Pants = DefaultCustomization.Pants;
	
	if (!Shoes || Shoes->Type != ECustomizationType::Shoes)
		Shoes = DefaultCustomization.Shoes;
	
	if (!Eyewear || Eyewear->Type != ECustomizationType::Eyewear)
		Eyewear = DefaultCustomization.Eyewear;
	
	if (!Belt || Belt->Type != ECustomizationType::Belt)
		Belt = DefaultCustomization.Belt;
	
	if (!Gloves || Gloves->Type != ECustomizationType::Gloves)
		Gloves = DefaultCustomization.Gloves;
	
	if (!Watch || Watch->Type != ECustomizationType::Watch)
		Watch = DefaultCustomization.Watch;
	
	if (!Tattoo || Tattoo->Type != ECustomizationType::Tattoo)
		Tattoo = DefaultCustomization.Tattoo;
}

void FSavedCustomization::SanitizeServer(AReadyOrNotCharacter* Target)
{
	Sanitize();

	if (!Target || !Target->GetInventoryComponent())
		return;

	FSpawnedGear& SpawnedGear = Target->GetInventoryComponent()->GetSpawnedGear();
	
	AHeadwear* Headwear = Cast<AHeadwear>(SpawnedGear.Helmet);
	if (Headwear && !Headwear->bHasHelmet)
	{
		Helmet = nullptr;
		bHasHelmet = false;
	}
}

void UCustomizationDataBase::GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const
{
	Super::GetAssetRegistryTags(OutTags);
		
	FString EnumString = UEnum::GetDisplayValueAsText(Type).ToString();
	OutTags.Add(FAssetRegistryTag("Customization Type", EnumString, FAssetRegistryTag::TT_Alphabetical));
}

#if WITH_EDITOR
bool UCustomizationDataBase::IsEditorOnly() const
{
#if UE_BUILD_SHIPPING
	return CookRule < ECustomizationAssetCookRule::AlwaysCook;
#elif UE_BUILD_DEVELOPMENT || UE_BUILD_DEBUG
	return CookRule < ECustomizationAssetCookRule::DevelopmentOnly;
#endif
}
#endif
