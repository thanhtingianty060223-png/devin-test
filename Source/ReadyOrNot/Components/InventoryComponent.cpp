// Copyright Void Interactive, 2023

#include "InventoryComponent.h"

#include "InteractableComponent.h"

#include "ReadyOrNotGameMode.h"

#include "Actors/BaseGrenade.h"
#include "Actors/ExplosiveVest.h"
#include "Actors/SuspectArmour.h"
#include "Actors/Items/Chemlight.h"
#include "Actors/Items/BallisticsShield.h"
#include "Actors/Items/Headwear.h"

#include "Audio/RoNSoundData.h"

#include "Characters/AI/SWATCharacter.h"

#include "HUD/Widgets/HumanCharacterHUD_V2.h"

#include "Info/ReadyOrNotSignificanceManager.h"

#include "ReadyOrNotDebugSubsystem.h"
#include "SkinnedDecalSampler.h"
#include "Actors/PairedInteractionDriver.h"

static TAutoConsoleVariable<int32> CVarDrawInventoryDebug(TEXT("a.RonDrawInventoryChangeDebug"), 0, TEXT("Show debug info for inventory item change requests"));

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UInventoryComponent, SpawnedGear);
	DOREPLIFETIME(UInventoryComponent, InventoryItems);
	DOREPLIFETIME(UInventoryComponent, SelectedDevice);
	DOREPLIFETIME(UInventoryComponent, LastEquippedLoadout);
	DOREPLIFETIME(UInventoryComponent, RemovedInventoryItems);
	DOREPLIFETIME(UInventoryComponent, LatestItemChangeRequest);
}

AReadyOrNotCharacter* UInventoryComponent::GetOwningCharacter() const
{
	return Cast<AReadyOrNotCharacter>(GetOwner());
}

APlayerCharacter* UInventoryComponent::GetOwningPlayerCharacter() const
{
	return Cast<APlayerCharacter>(GetOwner());
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();	
}

void UInventoryComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	Super::OnComponentDestroyed(bDestroyingHierarchy);
	for (int32 i = 0; i < InventoryItems.Num(); i++)
	{
		if (InventoryItems[i])
		{
			if (GetWorld())
			{
				GetWorld()->DestroyActor(InventoryItems[i]);
			}
		}
	}
}

void UInventoryComponent::OnActorRelevancyChanged(AActor* Actor, bool bIsRelevant)
{
	if (!GetOwningCharacter() || GetOwningCharacter()->IsLocalPlayer())
		return;

	if (bIsRelevant)
	{
		// handle when this itemcomes back into relevancy
		ABaseItem* Item = Cast<ABaseItem>(Actor);
		if (Item)
		{
			if (Item->GetOwner() == GetOwner())
			{
				const FName Socket = (Item == GetEquippedItem() || IsEquippedWithShield(Item)) ? Item->HandsSocket : Item->BodySocket;
				Item->AttachToComponent(GetOwningCharacter()->GetMesh(), FAttachmentTransformRules::KeepWorldTransform, Socket);
			}
		}
	}
}

void UInventoryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (TimeToTickItems > 0.0f)
		TimeToTickItems -= DeltaTime;

	if (!GetOwningCharacter())
		return;
	
	AReadyOrNotPlayerController* ReplayController = UBpGameplayHelperLib::GetLocalRoNPlayerController(GetWorld());
	if (ReplayController && ReplayController->bIsReplaySpectator)
	{
		for (ABaseItem* Item : InventoryItems)
		{
			if (Item)
			{
				bool bShouldDisplay = false;
				for (EItemCategory ItemCategory : DisplayedBodyItemCategories)
				{
					if (Item->ContainsItemCategory(ItemCategory))
					{
						bShouldDisplay = true;
						break;
					}
				}
				for (TSubclassOf<ABaseItem> ItemClass : DisplayedBodyItemClass)
				{
					if (Item->IsA(ItemClass))
					{
						bShouldDisplay = true;
						break;
					}
				}
				for (TSubclassOf<ABaseItem> ItemClass : HiddenBodyItemClass)
				{
					if (Item->IsA(ItemClass))
					{
						bShouldDisplay = false;
						break;
					}
				}
				
				Item->SetActorTickEnabled(Item == GetEquippedItem() || !Item->bDisableTickWhenNotEquipped);
				
				if (bShouldDisplay)
				{
					Item->SetActorEnableCollision(true);
					Item->GetItemMesh()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
					Item->GetItemMesh()->SetSimulatePhysics(false);
					Item->AttachToComponent(GetOwningCharacter()->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, GetEquippedItem() == Item ? Item->HandsSocket : Item->BodySocket);
					Item->GetItemMesh()->AttachToComponent(GetOwningCharacter()->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, GetEquippedItem() == Item ? Item->HandsSocket : Item->BodySocket);
					Item->GetItemMesh()->SetSkeletalMesh(Item->GetAppropriateSkeletalMesh());
					Item->SetItemVisibility(true);
					Item->SetActorHiddenInGame(false);
					Item->GetItemMesh()->SetHiddenInGame(false);
				}
				else
				{
					Item->SetActorEnableCollision(false);
					Item->GetItemMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
					Item->SetActorHiddenInGame(true);
				}
			}
		}
	}
	
	if (bPendingInventoryItemsChanged)
	{
		if (!InventoryItems.Contains(nullptr))
		{
			bPendingInventoryItemsChanged = false;
			if (GetOwningPlayerCharacter()->HumanCharacterWidget_V2)
			{
				GetOwningPlayerCharacter()->HumanCharacterWidget_V2->OnInventoryItemsChanged();
			}
		}
	}

	if (bPendingInventorySpawnReplication)
	{
		Client_NotifyInventorySpawned_Implementation();
	}
	
	if (QueuedItemSwap)
	{
		if (!GetOwningCharacter()->IsAnimationBlocking())
		{
			PutItemInHands(QueuedItemSwap, false);
			QueuedItemSwap = nullptr;
		}
	}

	if (UReadyOrNotSignificanceManager::IsActorRelevant(GetOwningCharacter()))
	{
		for (ABaseItem* Item : RemovedInventoryItems)
		{
			if (IsValid(Item))
				Item->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		}
		
		for (ABaseItem* Item : InventoryItems)
		{
			// just tick the equipped item anim bp
			if (IsValid(Item))
			{
				TInlineComponentArray<UActorComponent*> Components;
				Item->GetComponents(Components);
				for (UActorComponent* Comp : Components)
				{
					Comp->SetCanEverAffectNavigation(false);
				}
				
				if (GetEquippedItem() == Item || Item->IsCollidesWhileNotEquipped())
				{
					Item->GetItemMesh()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
				}
				else if(!Item->IsCollidesWhileNotEquipped())
				{
					Item->GetItemMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				}
				
				if (GetOwningCharacter()->IsDeadOrUnconscious())
				{
					Item->SetActorEnableCollision(true);
					Item->GetItemMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				}

				Item->GetItemMesh()->bNoSkeletonUpdate = (Item != GetEquippedItem()) && Item->GetGameTimeSinceCreation() > 1.0f && !IsEquippedWithShield(Item) && TimeToTickItems < 0.0f && !Item->bShouldTickAnimBPWhenNotEquipped;
				bool bShouldTick = Item->GetGameTimeSinceCreation() < 1.0f || Item == LatestItemChangeRequest.FromItem || Item == LatestItemChangeRequest.ToItem || !Item->bDisableTickWhenNotEquipped || IsEquippedWithShield(Item) || TimeToTickItems > 0.0f;
				Item->SetActorTickEnabled(bShouldTick);
				if (Item->InteractableComponent)
				{
					Item->InteractableComponent->bEnabled = false;
				}
				
				if (GetOwningPlayerCharacter() && GetOwningPlayerCharacter()->IsLocalPlayer())
				{
					// Fixes bug (in combination with attaching it on their back in on local holster complete) where the shield would not have collision on the hosts back
					if (!Item->IsA(ABallisticsShield::StaticClass()))
					{
						if (!GetOwningCharacter()->IsCarried() && !GetOwningCharacter()->IsCarrying() &&
							!UInteractionsData::IsPairedInteractionPlayingOn(GetOwningCharacter()))
						{
							if (Item->ShouldAttachToOwner() && !Item->GetItemMesh()->IsAttachedTo(GetOwningPlayerCharacter()->GetMesh1P()))
							{
								Item->AttachToComponent(GetOwningPlayerCharacter()->GetMesh1P(), FAttachmentTransformRules::SnapToTargetIncludingScale, Item->HandsSocket);
							}
						}
						
						Item->SetItemVisibility(GetEquippedItem() == Item || IsEquippedWithShield(Item));
						Item->SetItemHiddenInSceneCapture(true);
						Item->GetItemMesh()->SetBoundsScale(5.0f); // Helps with WPO messing with shading when using depth fade
					}
				}

				if (GetOwningCharacter()->IsCarrying() || GetOwningCharacter()->IsCarried())
				{
					if (Item == GetEquippedItem())
						Item->AttachToComponent(GetOwningPlayerCharacter()->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, Item->BodySocket);
				}

				// if (Item == SpawnedGear.Armor)
				// {
				// 	if (GetOwningCharacter()->GetMeshGearSlot()->SkeletalMesh != Item->GetAppropriateSkeletalMesh())
				// 	{
				// 		GetOwningCharacter()->GetMeshGearSlot()->SetSkeletalMesh(Item->GetAppropriateSkeletalMesh());
				// 		GetOwningCharacter()->GetMeshGearSlot()->EmptyOverrideMaterials();
				// 		
				// 		GetOwningCharacter()->GetSkinnedDecalSampler()->SetupMaterials();
				// 	}
				// 	Item->GetItemMesh()->SetSkeletalMesh(nullptr);
				// }
			}
		}
	} else
	{
		for (ABaseItem* Item : InventoryItems)
		{
			// just tick the equipped item anim bp
			if (Item)
			{
				Item->GetItemMesh()->bNoSkeletonUpdate = true;
			}
		}
	}

	#if !UE_BUILD_SHIPPING
	if (GetOwningPlayerCharacter() && CVarDrawInventoryDebug.GetValueOnGameThread() != 0)
	{
		FString DebugStr = FString::Format(TEXT("ItemChangeRequest: {0} IsComplete? {1} From: {2} To: {3}"), {LatestItemChangeRequest.ChangeId.ToString(),
			LatestItemChangeRequest.bIsComplete, LatestItemChangeRequest.FromItem ? LatestItemChangeRequest.FromItem->GetName() : "None", LatestItemChangeRequest.ToItem ? LatestItemChangeRequest.ToItem->GetName() : "None"});
		GEngine->AddOnScreenDebugMessage(-1, DeltaTime + 0.01f, FColor::White, DebugStr);
	}
	#endif

	UReadyOrNotSignificanceManager* SignificanceManager = UReadyOrNotSignificanceManager::Get(this);
	if (!bHasBoundToSignificanceManager && SignificanceManager)
	{
		SignificanceManager->OnActorRelevancyChanged.RemoveAll(this);
		SignificanceManager->OnActorRelevancyChanged.AddDynamic(this, &UInventoryComponent::OnActorRelevancyChanged);
		bHasBoundToSignificanceManager = true;
	}

	// usually you pull your weapon out to equip other things so this is probs legit
	if (Cast<ABaseMagazineWeapon>(LatestItemChangeRequest.ToItem))
	{
		LastEquippedItem = LatestItemChangeRequest.ToItem;
		// SetLastEquippedItemWheel(LastEquippedItem);
		
		LastEquippedWeapon = Cast<ABaseMagazineWeapon>(LatestItemChangeRequest.ToItem);
	}

	if (GetOwningPlayerCharacter() && GetOwningPlayerCharacter()->IsLocalPlayer())
	{
		if (GetEquippedItem())
		{
			if (GetEquippedItem()->ShouldEnableWeaponFovShader())
			{
				GetEquippedItem()->EnableWeaponFovShader();
			}
			
			GetEquippedItem()->TickWeaponFovShader(DeltaTime);
		}
	}

	if (GetOwningCharacter() && GetOwningCharacter()->IsDeadOrUnconscious())
	{
		ThrowEquippedItem();
	}
}

void UInventoryComponent::Client_NotifyInventorySpawned_Implementation()
{
	// If we still have null items, we must wait or continue to wait for inventory to replicate
	if (InventoryItems.Contains(nullptr))
	{
		bPendingInventorySpawnReplication = true;
		return;
	}

	bPendingInventorySpawnReplication = false;

	RefreshItemCategories();

	if (GetOwningPlayerCharacter() && GetOwningPlayerCharacter()->HumanCharacterWidget_V2)
	{
		GetOwningPlayerCharacter()->HumanCharacterWidget_V2->OnInventoryItemsChanged();
	}
}

void UInventoryComponent::OnRep_SpawnedGear()
{
	V_LOGM(LogReadyOrNot, "Received Spawned Gear: %s", *SpawnedGear.Guid.ToString());
	OnRep_InventoryItemsChanged();
}

ABaseItem* UInventoryComponent::GetHolsteredItem() const
{
	for (ABaseItem* Item : InventoryItems)
	{
		if (IsValid(Item) && Item->GetItemMesh() && GetOwningCharacter())
		{
			if (Cast<ABaseArmour>(Item))
				continue;
			
			const USkeletalMeshComponent* SKM = GetOwningPlayerCharacter() && GetOwningPlayerCharacter()->IsLocalPlayer() ? GetOwningPlayerCharacter()->GetMesh1P() : GetOwningCharacter()->GetMesh();

			if (Item->GetAttachParentSocketName() == Item->BodySocket && Item->GetItemMesh()->IsAttachedTo(SKM))
			{
				return Item;
			}
		}
	}

	// If we haven't found a holstered weapon
	// This will happen if we're looking for a holstered weapon on FP mesh since weapons don't attach to FP mesh on holster
	// Likely could just only check TP mesh above
	if (IsValid(LastEquippedItem))
	{
		return LastEquippedItem;
	}

	return nullptr;
}

bool UInventoryComponent::IsAnyItemAttachedToHands() const
{	
	for (const ABaseItem* Item : InventoryItems)
	{
		if (IsValid(Item) && Item->GetItemMesh() && GetOwningCharacter())
		{
			const USkeletalMeshComponent* SKM = GetOwningPlayerCharacter() && GetOwningPlayerCharacter()->IsLocalPlayer() ? GetOwningPlayerCharacter()->GetMesh1P() : GetOwningCharacter()->GetMesh();
			
			if (Item->GetAttachParentSocketName() == Item->HandsSocket && Item->GetItemMesh()->IsAttachedTo(SKM))
				return true;
		}
	}
	
	return false;
}

bool UInventoryComponent::IsAnyItemAttachedToBody() const
{
	for (const ABaseItem* Item : InventoryItems)
	{
		if (IsValid(Item) && Item->GetItemMesh() && GetOwningCharacter())
		{
			const USkeletalMeshComponent* SKM = GetOwningPlayerCharacter() && GetOwningPlayerCharacter()->IsLocalPlayer() ? GetOwningPlayerCharacter()->GetMesh1P() : GetOwningCharacter()->GetMesh();
			
			if (Item->GetAttachParentSocketName() == Item->BodySocket && Item->GetItemMesh()->IsAttachedTo(SKM))
				return true;
		}
	}
	
	return false;
}

void UInventoryComponent::Multicast_NotifyInventoryItemsChanged_Implementation()
{
	OnRep_InventoryItemsChanged();
}

void UInventoryComponent::Client_NotifyInventoryItemsDestroyed_Implementation()
{
}

void UInventoryComponent::Client_NotifyInventoryItemsChanged_Implementation()
{
	OnRep_InventoryItemsChanged();
	
}
void UInventoryComponent::PlayTPHolster(FItemChangeRequest ItemChangeRequest)
{
	if (ItemChangeRequest.FromItem && ItemChangeRequest.FromItem->AnimationData && !ItemChangeRequest.bInstant)
	{
		if (GetOwningCharacter() && !GetOwningCharacter()->IsLocalPlayer())
		{			
			ItemChangeRequest.FromItem->PlayHolster();
			
			if (GetOwningCharacter()->IsCrouching())
			{
				if (ItemChangeRequest.FromItem->AnimationData->Crouch_Holster.Body_TP)
				{
					GetWorld()->GetTimerManager().SetTimer(TH_OnHolsterComplete, this, &UInventoryComponent::OnTPHolsterComplete, ItemChangeRequest.FromItem->AnimationData->Crouch_Holster.Body_TP->GetPlayLength() - 0.1f);
				}
				else
				{
					OnTPHolsterComplete();
				}
			}
			else
			{
				if (ItemChangeRequest.FromItem->AnimationData->Holster.Body_TP)
				{
					V_LOGM(LogReadyOrNot, "Playing TP Holster in %f", ItemChangeRequest.FromItem->AnimationData->Holster.Body_TP->GetPlayLength());
					GetWorld()->GetTimerManager().SetTimer(TH_OnHolsterComplete, this, &UInventoryComponent::OnTPHolsterComplete, ItemChangeRequest.FromItem->AnimationData->Holster.Body_TP->GetPlayLength() - 0.1f);
				}
				else
				{
					OnTPHolsterComplete();
				}
			}
		}
	}
	else
	{
		OnTPHolsterComplete();
	}
}

void UInventoryComponent::Server_PlayTPHolster_Implementation(FItemChangeRequest ItemChangeRequest)
{
	LatestItemChangeRequest = ItemChangeRequest;
	OnRep_ItemChangeRequest();
}

bool UInventoryComponent::Server_PlayTPHolster_Validate(FItemChangeRequest ItemChangeRequest)
{
	return true;
}

void UInventoryComponent::PlayTPDraw(FItemChangeRequest ItemChangeRequest)
{
	if (ItemChangeRequest.ToItem && !ItemChangeRequest.bInstant)
	{
		ItemChangeRequest.ToItem->PlayDraw(!ItemChangeRequest.ToItem->bDrawnBefore);
		
		if (ItemChangeRequest.ToItem->bAttachOnDrawComplete)
		{
			ItemChangeRequest.ToItem->OnItemDrawComplete.RemoveAll(this);
			ItemChangeRequest.ToItem->OnItemDrawComplete.AddDynamic(this, &UInventoryComponent::OnNewItemChangeDrawComplete);
		}
	}
}

void UInventoryComponent::OnNewItemChangeDrawComplete(ABaseItem* Item)
{
	if (Item && Item->GetItemMesh())
	{
		Item->OnItemDrawComplete.RemoveAll(this);
		
		#if !UE_BUILD_SHIPPING
		const FString ServerClientStr = GetNetMode() == NM_Client ? "[Client]" : "[Server]";
		V_LOGM(LogReadyOrNot, "%s[%s][%d][%s] Changing TP: %s to socket %s", *ServerClientStr, *GetName(), __LINE__, *FString(__FUNCTION__), *Item->GetName(), *Item->HandsSocket.ToString());
		#endif
		
		Item->AttachToComponent(GetOwningCharacter()->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, Item->HandsSocket);
		Item->GetItemMesh()->SetRelativeLocation(FVector::ZeroVector);
		Item->GetItemMesh()->SetRelativeRotation(FRotator::ZeroRotator);
		Item->GetItemMesh()->AttachToComponent(Item->GetRootComponent(), FAttachmentTransformRules::SnapToTargetIncludingScale);
		Item->GetItemMesh()->SetSkeletalMesh(Item->GetAppropriateSkeletalMesh());
		Item->SetActorTickEnabled(true);
		Item->SetItemVisibility(true);
		Item->MarkAsEvidence(false);
	}
}

void UInventoryComponent::PlayLocalHolster_Implementation(FItemChangeRequest ItemChangeRequest)
{
	// 'Client' event may still play for AI char... just ignore
	if (!GetOwningCharacter()->IsLocalPlayer())
	{
		return;	
	}
	
	if (ItemChangeRequest.FromItem && ItemChangeRequest.FromItem->AnimationData && !ItemChangeRequest.bInstant)
	{
		if (!ItemChangeRequest.bNoDraw)
		{
			if (ItemChangeRequest.FromItem->AnimationData->Holster.Body_FP)
				GetWorld()->GetTimerManager().SetTimer(TH_OnHolsterComplete, this, &UInventoryComponent::OnLocalHolsterComplete, ItemChangeRequest.FromItem->AnimationData->Holster.Body_FP->GetPlayLength() - 0.1f);
		}
		
	
		ItemChangeRequest.FromItem->PlayHolster();		
	}
	else
	{
		if (!ItemChangeRequest.bNoDraw)
		{
			OnLocalHolsterComplete();
		}
	}
}

void UInventoryComponent::PlayLocalDraw_Implementation(FItemChangeRequest ItemChangeRequest)
{
	if (GetOwningPlayerCharacter())
	{
		GetOwningPlayerCharacter()->bUserLowReady = false;
		if (!GetOwningPlayerCharacter()->bStartedFadeIn)
			return;
	}
	
	if (ItemChangeRequest.ToItem)
	{
		
		ItemChangeRequest.ToItem->PlayDraw(!ItemChangeRequest.ToItem->bDrawnBefore);
	}
}

void UInventoryComponent::OnLocalHolsterComplete()
{
	if (!GetWorld())
		return;
	
	GetWorld()->GetTimerManager().ClearTimer(TH_OnHolsterComplete);
	if (GetOwningPlayerCharacter() && GetOwningPlayerCharacter()->IsLocalPlayer())
	{
		if (LatestItemChangeRequest.ToItem && LatestItemChangeRequest.ToItem->GetOwner() == GetOwningCharacter() && LatestItemChangeRequest.ToItem->GetItemMesh())
		{
			PlayLocalDraw(LatestItemChangeRequest);
			if (LatestItemChangeRequest.ToItem)
			{
				LatestItemChangeRequest.ToItem->GetItemMesh()->SetHiddenInGame(false);
				LatestItemChangeRequest.ToItem->GetItemMesh()->SetSkeletalMesh(LatestItemChangeRequest.ToItem->GetAppropriateSkeletalMesh());
				LatestItemChangeRequest.ToItem->EnableWeaponFovShader();
				LatestItemChangeRequest.ToItem->SetItemVisibility(true); 
				LatestItemChangeRequest.ToItem->SetActorTickEnabled(true);
				LatestItemChangeRequest.ToItem->Client_SetFPModelVisibility(true);
				
				//GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UInventoryComponent::LocalAttachEquippedItemToHands);
				const FTimerDelegate Delegate = FTimerDelegate::CreateUObject(this, &UInventoryComponent::LocalAttachEquippedItemToHands, GetOwningPlayerCharacter(), LatestItemChangeRequest.ToItem);
				GetWorld()->GetTimerManager().SetTimer(TH_AttachEquippedItemDelay, Delegate, 0.0001f, false);
			}
		}

		if (LatestItemChangeRequest.FromItem)
		{
			// We need to keep this on their back for collision purposes
			if (Cast<ABallisticsShield>(LatestItemChangeRequest.FromItem))
			{
				LatestItemChangeRequest.FromItem->AttachToComponent(GetOwningCharacter()->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, LatestItemChangeRequest.FromItem->BodySocket);
				LatestItemChangeRequest.FromItem->SetActorEnableCollision(true);
				LatestItemChangeRequest.FromItem->SetItemVisibility(true); 
				LatestItemChangeRequest.FromItem->Client_SetFPModelVisibility(false);
			}
			else
			{
				LatestItemChangeRequest.FromItem->SetItemVisibility(false);
			}
			
			LatestItemChangeRequest.FromItem->SetActorTickEnabled(!LatestItemChangeRequest.FromItem->bDisableTickWhenNotEquipped);
		}
		LatestItemChangeRequest.bIsComplete = true;
		
		OnItemEquipped.Broadcast(LatestItemChangeRequest.ToItem);
		OnItemHolstered.Broadcast(LatestItemChangeRequest.FromItem);
		SetLastEquippedItemWheel(LatestItemChangeRequest.FromItem);
	}

	GetWorld()->GetTimerManager().ClearTimer(TH_UpdateItemVisualizations);
	TH_UpdateItemVisualizations = GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UInventoryComponent::UpdateItemVisualizations));
}

// Set the currently equipped item to visible. Currently exists to just play 1 frame after holster complete to account for 1 frame montage play delay
void UInventoryComponent::LocalAttachEquippedItemToHands(APlayerCharacter* LocalPlayer, ABaseItem* Item)
{
	GetWorld()->GetTimerManager().ClearTimer(TH_AttachEquippedItemDelay);

	if (LocalPlayer && Item) // just in case they get destroyed between frames somehow...
	{
		Item->AttachToComponent(LocalPlayer->GetMesh1P(), FAttachmentTransformRules::SnapToTargetIncludingScale, Item->HandsSocket);
	}
}

void UInventoryComponent::UpdateItemVisualizations()
{
	if (GetOwningPlayerCharacter())
	{
		TInlineComponentArray<UItemVisualizationComponent*> VisualizationComponents;
		GetOwningPlayerCharacter()->GetComponents(VisualizationComponents);
		
		for (UItemVisualizationComponent* V : VisualizationComponents)
		{
			V->UpdateItemVisualizationComponent();
		}
	}
}

void UInventoryComponent::OnTPHolsterComplete()
{
	if (!GetOwningCharacter())
		return;
	
	if (GetOwningCharacter()->IsLocalPlayer())
		return;

	if (!GetWorld())
		return;
	
	GetWorld()->GetTimerManager().ClearTimer(TH_OnHolsterComplete);

	#if !UE_BUILD_SHIPPING
	V_LOGM(LogReadyOrNot, "OnTpHolsterComplete");
	#endif

	if (LatestItemChangeRequest.ToItem && LatestItemChangeRequest.ToItem->GetItemMesh())
	{
		if (!LatestItemChangeRequest.ToItem->bAttachOnDrawComplete)
		{
			#if !UE_BUILD_SHIPPING
			const FString ServerClientStr = GetNetMode() == NM_Client ? "[Client]" : "[Server]";
			V_LOGM(LogReadyOrNot, "%s[%s][%d][%s] Changing TP: %s to socket %s", *ServerClientStr, *GetName(), __LINE__, *FString(__FUNCTION__), *LatestItemChangeRequest.ToItem->GetName(), *LatestItemChangeRequest.ToItem->HandsSocket.ToString());
			#endif
			
			LatestItemChangeRequest.ToItem->AttachToComponent(GetOwningCharacter()->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, LatestItemChangeRequest.ToItem->HandsSocket);
			LatestItemChangeRequest.ToItem->GetItemMesh()->SetRelativeLocation(FVector::ZeroVector);
			LatestItemChangeRequest.ToItem->GetItemMesh()->SetRelativeRotation(FRotator::ZeroRotator);
			LatestItemChangeRequest.ToItem->GetItemMesh()->AttachToComponent(LatestItemChangeRequest.ToItem->GetRootComponent(), FAttachmentTransformRules::SnapToTargetIncludingScale);
			LatestItemChangeRequest.ToItem->GetItemMesh()->SetSkeletalMesh(LatestItemChangeRequest.ToItem->GetAppropriateSkeletalMesh());
			LatestItemChangeRequest.ToItem->SetActorTickEnabled(true);
			LatestItemChangeRequest.ToItem->SetItemVisibility(true);
			LatestItemChangeRequest.ToItem->MarkAsEvidence(false);
		}
	}
	
	PlayTPDraw(LatestItemChangeRequest);

	LatestItemChangeRequest.bIsComplete = true;

	ReattachAllGear();

	GetWorld()->GetTimerManager().ClearTimer(TH_UpdateItemVisualizations);
	TH_UpdateItemVisualizations = GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UInventoryComponent::UpdateItemVisualizations));
	
	if (LatestItemChangeRequest.ToItem)
	{
		OnItemEquipped.Broadcast(LatestItemChangeRequest.ToItem);
	}
	
	if (LatestItemChangeRequest.FromItem)
	{
		OnItemHolstered.Broadcast(LatestItemChangeRequest.FromItem);
		SetLastEquippedItemWheel(LatestItemChangeRequest.FromItem); 
	}

	// Force relevant for one frame
	UReadyOrNotSignificanceManager::ForceActorRelevant(GetOwningCharacter());
}

void UInventoryComponent::OnRep_ItemChangeRequest()
{
	if (LatestItemChangeRequest != LastReceivedItemChangeRequest)
	{
		LastReceivedItemChangeRequest = LatestItemChangeRequest;
		LatestItemChangeRequest.bIsComplete = false;
		
		if (GetOwningCharacter()->IsLocalPlayer())
		{
			PlayLocalHolster(LatestItemChangeRequest);
		}
		else
		{
			PlayTPHolster(LatestItemChangeRequest);
		}

		#if !UE_BUILD_SHIPPING
		FString DebugStr = FString::Format(TEXT("ItemChangeRequest: {0} IsComplete? {1} From: {2} To: {3}"), {LatestItemChangeRequest.ChangeId.ToString(),
				LatestItemChangeRequest.bIsComplete, LatestItemChangeRequest.FromItem ? LatestItemChangeRequest.FromItem->GetName() : "None", LatestItemChangeRequest.ToItem ? LatestItemChangeRequest.ToItem->GetName() : "None"});
		V_LOGM(LogReadyOrNot, "%s", *DebugStr);
		#endif
	}
}

void UInventoryComponent::DestroyAllEquippedItems()
{
	for (int32 i = 0; i < InventoryItems.Num(); i++)
	{
		if (InventoryItems[i])
		{
			if (LatestItemChangeRequest.ToItem == InventoryItems[i])
			{
				LatestItemChangeRequest.ToItem = nullptr;
			}
			if (LatestItemChangeRequest.FromItem == InventoryItems[i])
			{
				LatestItemChangeRequest.FromItem = nullptr;
			}
			GetWorld()->DestroyActor(InventoryItems[i]);
		}
	}

	SpawnedGear = FSpawnedGear();

	LatestItemChangeRequest = FItemChangeRequest();
	InventoryItems.Empty();

	Client_NotifyInventoryItemsDestroyed();
}

void UInventoryComponent::Server_AttemptEquipNewLoadout_Implementation(FSavedLoadout Loadout)
{
	AReadyOrNotGameMode* gm = Cast<AReadyOrNotGameMode>(GetWorld()->GetAuthGameMode());
	if (gm)
	{
		gm->RequestNewLoadout(GetOwningCharacter(), Loadout);
	}
}

bool UInventoryComponent::Server_AttemptEquipNewLoadout_Validate(FSavedLoadout Loadout)
{
	return true;
}

void UInventoryComponent::RefreshItemCategories()
{
	for (FItemSelectionGroup& ItemGroup : ItemSelectionGroups)
	{
		ItemGroup.ItemIndex = -1;
		ItemGroup.Items.Empty();

		ItemGroup.ItemGroupIcon = nullptr;

		for (const ABaseItem* Item : InventoryItems)
		{
			if (!IsValid(Item))
				continue;

			bool bMatchingCategory = false;
			for (EItemCategory Category : ItemGroup.AdditionalItemCategories)
			{
				if (Item->ItemCategories.Contains(Category))
				{
					bMatchingCategory = true;
					break;
				}
			}

			if (!Item->ItemCategories.Contains(ItemGroup.ItemCategory) && !bMatchingCategory)
				continue;

			ItemGroup.Items.AddUnique(Item->GetClass());
			
			if (!ItemGroup.ItemGroupIcon)
				ItemGroup.ItemGroupIcon = Item->Visuals.ItemIcon;
		}
	}
}

void UInventoryComponent::DestroyRemovedItems()
{
	for (ABaseItem* Item : RemovedInventoryItems)
	{
		if (Item)
		{
			Item->Destroy();
		}
	}
	RemovedInventoryItems.Empty();
}

bool UInventoryComponent::Holster(ABaseItem* Item, const bool bInstant)
{
	if (IsValid(Item))
	{
		// No equipping allowed whilst being carried by someone or if we are carrying someone
		if (GetOwningCharacter()->IsCarried() || (GetOwningCharacter()->IsCarrying() && !GetOwningCharacter()->IsDropping()))
			return false;

		// No holstering allowed whilst in ragdoll
		if (GetOwningCharacter()->IsInRagdoll() || GetOwningCharacter()->IsGettingUp())
			return false;

		if (IsAnyBlockingAnimationPlaying() && !bInstant)
			return false;
		
		FItemChangeRequest ItemChangeRequest;
		ItemChangeRequest.bInstant = bInstant;
		ItemChangeRequest.FromItem = Item;
		ItemChangeRequest.ToItem = nullptr;

		if (!GetOwningCharacter()->HasAuthority())
		{
			LatestItemChangeRequest = ItemChangeRequest;
			OnRep_ItemChangeRequest();
		}

		Server_ChangeEquippedItem(ItemChangeRequest);

		return true;
	}

	return false;
}

bool UInventoryComponent::HolsterEquippedItem(const bool bInstant)
{
	return Holster(GetEquippedItem(), bInstant);
}

bool UInventoryComponent::EquipHolsteredItem(const bool bInstant)
{
	return PutItemInHands(GetHolsteredItem(), bInstant);
}

bool UInventoryComponent::PutItemInHands(ABaseItem* Item, const bool bInstant, const bool bForce)
{
	if (!Item)
		return false;
	
	if (GetEquippedItem() == Item && !bForce)
		return true;

	// Cannot put armor in hands
	if (Cast<ABaseArmour>(Item))
		return false;

	// No equipping allowed whilst being carried by someone or if we are carrying someone
	if (GetOwningCharacter()->IsCarried() || ((GetOwningCharacter()->IsCarrying() && !GetOwningCharacter()->IsDropping()) && !bForce))
		return false;

	// No equipping allowed whilst in ragdoll
	if (GetOwningCharacter()->IsInRagdoll() || (GetOwningCharacter()->IsGettingUp() && !bForce))
		return false;

	#if !UE_BUILD_SHIPPING
	const FString ServerClientStr = GetNetMode() == NM_Client ? "[Client]" : "[Server]";
	V_LOGM(LogReadyOrNot, "%s Calling Put Item In Hands", *ServerClientStr);
	#endif
	
	if (Item->CanEquip(GetOwningCharacter()) || bForce)
	{
		if (GetOwningCharacter()->IsAnimationBlocking() && !bInstant && !bForce)
		{
			QueuedItemSwap = Item;
			return true;
		}

		QueuedItemSwap = nullptr;
		FItemChangeRequest ItemChangeRequest;
		ItemChangeRequest.bInstant = bInstant;
		ItemChangeRequest.FromItem = GetEquippedItem();
		ItemChangeRequest.ToItem = Item;

		if (!GetOwningCharacter()->HasAuthority())
		{
			LatestItemChangeRequest = ItemChangeRequest;
			OnRep_ItemChangeRequest();
		}

		Server_ChangeEquippedItem(ItemChangeRequest);

		return true;
	}
	
	return false;
}

void UInventoryComponent::Server_ChangeEquippedItem_Implementation(FItemChangeRequest ItemChangeRequest)
{
	LastEquippedItem = ItemChangeRequest.FromItem;
	//SetLastEquippedItemWheel(LastEquippedItem);
	
	if (Cast<ABaseWeapon>(ItemChangeRequest.ToItem))
	{
		LastEquippedWeapon = Cast<ABaseMagazineWeapon>(ItemChangeRequest.ToItem);
	}

	LatestItemChangeRequest = ItemChangeRequest;
	OnRep_ItemChangeRequest();
}

bool UInventoryComponent::Server_ChangeEquippedItem_Validate(FItemChangeRequest ItemChangeRequest)
{
	return true;
}

bool UInventoryComponent::CanEquip(ABaseItem* Item)
{
	return true;
}

FSpawnedGear& UInventoryComponent::GetSpawnedGear()
{
	return SpawnedGear;
}

TArray<ABaseItem*> UInventoryComponent::GetInventoryItemsOfType(const EItemCategory ItemCategory) const
{
	TArray<ABaseItem*> ItemsFound;
	for (ABaseItem* Item : InventoryItems)
	{
		if (Item && Item->ContainsItemCategory(ItemCategory))
		{
			ItemsFound.Add(Item);
		}
	}

	return ItemsFound;
}

TArray<ABaseItem*> UInventoryComponent::GetInventoryItems() const
{
	return InventoryItems;
}

TArray<ABaseItem*> UInventoryComponent::GetRemovedInventoryItems() const
{
	return RemovedInventoryItems;
}

bool UInventoryComponent::HasAnyInventoryItems() const
{
	return InventoryItems.Num() > 0;
}

bool UInventoryComponent::HasAnyInventoryItemsOfType(const EItemCategory ItemCategory) const
{
	return GetInventoryItemsOfType(ItemCategory).Num() > 0;
}

bool UInventoryComponent::HasAnyInventoryItemsOfClass(TSubclassOf<ABaseItem> ItemClass) const
{
	return GetInventoryItemsOfClass<ABaseItem>(ItemClass).Num() > 0;
}

ABaseItem* UInventoryComponent::GetInventoryItemOfClass(UClass* Class, const bool bCanEquipCheck) const
{
	if (!Class)
		return nullptr;

	for (ABaseItem* Item : InventoryItems)
	{
		if (Item && Item->IsA(Class) && (!bCanEquipCheck || (bCanEquipCheck && Item->CanEquip(GetOwningCharacter()))))
		{
			return Item;
		}
	}
	
	return nullptr;
}

ABaseItem* UInventoryComponent::GetInventoryItemOfType(const EItemCategory ItemCategory) const
{
	for (ABaseItem* Item : InventoryItems)
	{
		if (Item && Item->ContainsItemCategory(ItemCategory) && Item->CanEquip(GetOwningCharacter()))
		{
			return Item;
		}
	}
	
	return nullptr;
}

ABaseItem* UInventoryComponent::GetInventoryItemOfClassType(const EItemClass ItemClass) const
{
	for (ABaseItem* Item : InventoryItems)
	{
		if (Item && Item->ItemClass == ItemClass)
		{
			return Item;
		}
	}
	
	return nullptr;
}

bool UInventoryComponent::IsItemEquipped(const EItemCategory ItemCategory) const
{
	if (LatestItemChangeRequest.ToItem)
	{
		return LatestItemChangeRequest.ToItem->ContainsItemCategory(ItemCategory);
	}


	return false;
}

bool UInventoryComponent::IsItemEquipped_Class(const TSubclassOf<ABaseItem> ItemClass) const
{
	if (LatestItemChangeRequest.ToItem)
	{
		return LatestItemChangeRequest.ToItem->IsA(ItemClass);
	}
	
	return false;
}

int32 UInventoryComponent::CountInventoryItemType(const EItemCategory ItemCategory) const
{
	int32 ItemCount = 0;

	for (ABaseItem* Item : InventoryItems)
	{
		if (Item && Item->ContainsItemCategory(ItemCategory))
		{
			ItemCount++;
		}
	}

	return ItemCount;
}

void UInventoryComponent::OnRep_InventoryItemsChanged()
{
	for (ABaseItem* i : RemovedInventoryItems)
	{
		if (i)
		{
			i->SetActorTickEnabled(true);
		}
	}

	TimeToTickItems = 1.0f;
	
	if (GetOwningPlayerCharacter())
	{
		if (GetOwningPlayerCharacter()->IsLocalPlayer())
		{
			UpdateItemVisualizations();
			
			for (ABaseItem* Item : InventoryItems)
			{
				if (Item)
				{
					UReadyOrNotSignificanceManager::UnregisterActorWithSignificanceManager(Item);
				}
			}

			if (InventoryItems.Contains(nullptr))
			{
				bPendingInventoryItemsChanged = true;
			}
			
			RefreshItemCategories();

			if (GetOwningPlayerCharacter()->HumanCharacterWidget_V2)
			{
				GetOwningPlayerCharacter()->HumanCharacterWidget_V2->OnInventoryItemsChanged();
			}
		}
	}

	OnInventoryItemsChanged.Broadcast();
	ReattachAllGear();
}

void UInventoryComponent::ReattachAllGear()
{
	if (GetOwningCharacter())
	{
		for (ABaseItem* Item : InventoryItems)
		{
			if (!Item)
				continue;
			
			FName OverrideSocket = GetOwningCharacter()->GetSocketOverride(Item->BodySocket);
			Item->BodySocket = OverrideSocket;
		}
	}
	
	if (GetOwningCharacter() && !GetOwningCharacter()->IsLocalPlayer())
	{
		for (ABaseItem* Item : InventoryItems)
		{
			if (!Item)
				continue;

			if (!Item->CanEquip(GetOwningCharacter()))
				continue;
			
			if (Item != GetEquippedItem())
			{
				bool bShouldDisplay = false;
				for (EItemCategory ItemCategory : DisplayedBodyItemCategories)
				{
					if (Item->ContainsItemCategory(ItemCategory))
					{
						bShouldDisplay = true;
						break;
					}
				}
				for (TSubclassOf<ABaseItem> ItemClass : DisplayedBodyItemClass)
				{
					if (Item->IsA(ItemClass))
					{
						bShouldDisplay = true;
						break;
					}
				}
				for (TSubclassOf<ABaseItem> ItemClass : HiddenBodyItemClass)
				{
					if (Item->IsA(ItemClass))
					{
						bShouldDisplay = false;
						break;
					}
				}
				
				Item->SetActorTickEnabled(Item == GetEquippedItem() || !Item->bDisableTickWhenNotEquipped);
				if (bShouldDisplay)
				{
					Item->AttachToComponent(GetOwningCharacter()->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, Item->BodySocket);
					Item->GetItemMesh()->SetSkeletalMesh(Item->GetAppropriateSkeletalMesh());

					Item->GetItemMesh()->bNoSkeletonUpdate = false;
					
					//#if !UE_BUILD_SHIPPING
					//const FString ServerClientStr = GetNetMode() == NM_Client ? "[Client]" : "[Server]";
					//V_LOGM(LogReadyOrNot, "%s[%s][%d][%s] Changing TP: %s to socket %s", *ServerClientStr, *GetName(), __LINE__, *FString(__FUNCTION__), *Item->GetName(), *Item->BodySocket.ToString());
					//#endif
					
					if (Item->BodySocket == "root" || Item->BodySocket.IsNone())
					{
						Item->GetItemMesh()->SetMasterPoseComponent(GetOwningCharacter()->GetMesh());
						// if (Item == SpawnedGear.Armor)
						// {
						// 	if (GetOwningCharacter()->GetMeshGearSlot()->SkeletalMesh != Item->GetAppropriateSkeletalMesh())
						// 	{
						// 		GetOwningCharacter()->GetMeshGearSlot()->SetSkeletalMesh(Item->GetAppropriateSkeletalMesh());
						// 		GetOwningCharacter()->GetMeshGearSlot()->EmptyOverrideMaterials();
						//
						// 		GetOwningCharacter()->GetSkinnedDecalSampler()->SetupMaterials();
						// 	}
						// 	Item->GetItemMesh()->SetSkeletalMesh(nullptr);
						// }
						Item->AddTickPrerequisiteComponent(GetOwningCharacter()->GetMesh());
					}
					else
					{
						// Make sure we reset the master pose component, some skins may change the body socket
						Item->GetItemMesh()->SetMasterPoseComponent(nullptr);
					}
				}
				else 
				{
					Item->SetItemVisibility(false);
				}
			}
			else
			{
				if (!Item->IsPlayingDraw() && !Item->bAttachOnDrawComplete)
				{
					Item->GetItemMesh()->SetSimulatePhysics(false);
					Item->GetItemMesh()->SetCollisionProfileName("Item");
					Item->AttachToComponent(GetOwningCharacter()->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, Item->HandsSocket);
					Item->GetItemMesh()->AttachToComponent(GetEquippedItem()->GetRootComponent(), FAttachmentTransformRules::SnapToTargetIncludingScale);
					Item->GetItemMesh()->SetRelativeLocation(FVector::ZeroVector);
					Item->GetItemMesh()->SetRelativeRotation(FRotator::ZeroRotator);
				}

// #if !UE_BUILD_SHIPPING
// 				const FString ServerClientStr = GetNetMode() == NM_Client ? "[Client]" : "[Server]";
// 				V_LOGM(LogReadyOrNot, "%s[%s][%d][%s] Changing TP: %s to socket %s", *ServerClientStr, *GetName(), __LINE__, *FString(__FUNCTION__), *Item->GetName(), *Item->HandsSocket.ToString());
// #endif
			}
		}
	}
	else if (GetOwningPlayerCharacter())
	{
		for (ABaseItem* Item : InventoryItems)
		{
			if (!Item)
				continue;
			
			if (!Item->CanEquip(GetOwningCharacter()))
				continue;

			// Fixes collision where host puts shield on their back
			if (Cast<ABallisticsShield>(Item))
			{
				Item->AttachToComponent(Item == GetEquippedItem() ? GetOwningPlayerCharacter()->GetMesh1P() : GetOwningCharacter()->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, Item->BodySocket);
				Item->SetActorEnableCollision(true);
				Item->SetItemVisibility(true); 
			}
			else
			{
				//Item->GetItemMesh()->SetCastShadow(false);
				Item->GetItemMesh()->bSelfShadowOnly = true;
				Item->GetItemMesh()->bCastContactShadow = false;
				Item->SetItemVisibility(Item == GetEquippedItem());
				Item->SetActorTickEnabled(Item == GetEquippedItem() || !Item->bDisableTickWhenNotEquipped);
			}

			// Overrides for armor, could use item vis component, but idk why we even need them
			if (Cast<ABaseArmour>(Item) && (Item->BodySocket == "root" || Item->BodySocket.IsNone()))
			{
				Item->GetItemMesh()->SetMasterPoseComponent(GetOwningCharacter()->GetMesh());
				Item->GetItemMesh()->SetCastShadow(true);
			}
		}
	}
}

ABaseArmour* UInventoryComponent::GetArmour() const
{
	return Cast<ABaseArmour>(SpawnedGear.Armor);
}

AHeadwear* UInventoryComponent::GetHeadArmour() const
{
	return Cast<AHeadwear>(SpawnedGear.Helmet);
}

AHeadwear* UInventoryComponent::GetHeadwear() const
{
	return Cast<AHeadwear>(GetInventoryItemOfClassType(EItemClass::IC_Headgear));
}

bool UInventoryComponent::IsWearingArmour() const
{
	return GetArmour() != nullptr;
}

bool UInventoryComponent::IsWearingHeadArmour() const
{
	return GetHeadArmour() != nullptr;
}

bool UInventoryComponent::IsWearingAntiFlashGoggles() const
{
	return GetInventoryItemOfType(EItemCategory::IC_Goggles) != nullptr; 
}

bool UInventoryComponent::IsWearingExplosiveVest() const
{
	return Cast<AExplosiveVest>(SpawnedGear.Armor) != nullptr;
}

bool UInventoryComponent::IsAnyBlockingAnimationPlaying() const
{
	for (ABaseItem* Item : InventoryItems)
	{
		if ( Item && Item->GetOwner() == GetOwningCharacter() && Item->IsBlockingAnimationPlaying({}))
			return true;
	}
	return false;
}

ABaseItem* UInventoryComponent::GetEquippedItem() const
{
	ABaseItem* Item = LatestItemChangeRequest.bIsComplete ? LatestItemChangeRequest.ToItem : LatestItemChangeRequest.FromItem;
	return Item && InventoryItems.Contains(Item) ? Item : nullptr;
}

bool UInventoryComponent::AddInventoryItem(ABaseItem* Item)
{
	if (!Item)
		return false;

	if (!InventoryItems.Contains(Item))
	{
		AReadyOrNotCharacter* OwnerCharacter = GetOwningCharacter();
		Item->ItemMesh->MoveIgnoreActors.Add(OwnerCharacter);
		OwnerCharacter->MoveIgnoreActorAdd(Item);

		Item->SetOwner(OwnerCharacter);
		Item->SetActorEnableCollision(true);
		Item->GetItemMesh()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Item->bInInventory = true;
		
		Item->ItemMesh->MoveIgnoreActors.Add(GetOwningCharacter());
		GetOwningCharacter()->MoveIgnoreActorAdd(Item);

		InventoryItems.AddUnique(Item);
		RemovedInventoryItems.Remove(Item);
		OnItemAddedToInventory.Broadcast(Item);
		Multicast_NotifyInventoryItemsChanged();
		return true;
	}
	
	return false;
}

bool UInventoryComponent::RemoveInventoryItem(ABaseItem* Item, const bool bNullOwner)
{
	if (!Item)
		return false;
	
	if (InventoryItems.Contains(Item))
	{
		#if !UE_BUILD_SHIPPING
		if (CHECK_DEBUG_SUBSYSTEM && DEBUG_SUBSYSTEM->bInfiniteSWATItems)
		{
			if (const ASWATCharacter* InstigatorCharacter = Cast<ASWATCharacter>(GetOwningCharacter()))
			{
				ABaseItem* NewItem = GetWorld()->SpawnActor<ABaseItem>(Item->GetClass());
				if (NewItem)
				{
					InstigatorCharacter->GetInventoryComponent()->AddInventoryItem(NewItem);
				}
			}
		}
		#endif
		
		if (bNullOwner)
		{
			Item->SetOwner(nullptr);
		}
		
		AReadyOrNotCharacter* OwnerCharacter = GetOwningCharacter();
		Item->ItemMesh->MoveIgnoreActors.Remove(OwnerCharacter);
		OwnerCharacter->MoveIgnoreActorRemove(Item);
		Item->SetActorHiddenInGame(false);
		Item->bInInventory = false;
		OnItemRemovedFromInventory.Broadcast(Item);
		InventoryItems.Remove(Item);
		RemovedInventoryItems.AddUnique(Item);
		if (LatestItemChangeRequest.ToItem == Item)
		{
			LatestItemChangeRequest.ToItem = nullptr;
		}
		if (LatestItemChangeRequest.FromItem == Item)
		{
			LatestItemChangeRequest.FromItem = nullptr;
		}
		
		OnRep_InventoryItemsChanged();
		return true;
	}
	
	return false;
}

bool UInventoryComponent::DestroyInventoryItem(ABaseItem* Item)
{
	if (!Item)
		return false;
	
	if (GetOwner()->GetLocalRole() < ROLE_Authority && Item->GetIsReplicated())
		return false;

	if (LatestItemChangeRequest.ToItem == Item)
	{
		LatestItemChangeRequest.ToItem = nullptr;
	}
	if (LatestItemChangeRequest.FromItem == Item)
	{
		LatestItemChangeRequest.FromItem = nullptr;
	}

	InventoryItems.Remove(Item);
	SpawnedGear.Remove(Item);
	Item->bInInventory = false;
	Item->SetOwner(nullptr);
	Item->Destroy();
	Client_NotifyInventoryItemsDestroyed();
	return true;
}

void UInventoryComponent::ManuallySetEquippedItem(ABaseItem* Item)
{
	LatestItemChangeRequest.ToItem = Item;
	LatestItemChangeRequest.bIsComplete = true;
}

void UInventoryComponent::ClearEquippedItem()
{
	LatestItemChangeRequest.ToItem = nullptr;
}

void UInventoryComponent::ThrowEquippedItem()
{
	if (ABaseItem* EquippedItem = GetEquippedItem())
	{
		ThrowSpecificItem(EquippedItem);
		
		LatestItemChangeRequest.ToItem = nullptr;
		if (LatestItemChangeRequest.FromItem == EquippedItem)
			LatestItemChangeRequest.FromItem = nullptr;
	}
}

void UInventoryComponent::ThrowAllWeapons()
{
	ThrowEquippedItem();

	for (ABaseWeapon* Weapon : GetInventoryItemsOfClass<ABaseWeapon>())
		ThrowSpecificItem(Weapon);
}

void UInventoryComponent::ThrowAllItems()
{
	ThrowEquippedItem();
	
	for (ABaseItem* Item : GetInventoryItems())
		ThrowSpecificItem(Item);
}

void UInventoryComponent::ThrowSpecificItem_Implementation(ABaseItem* Item)
{
	if (!IsValid(Item))
		return;

	AReadyOrNotCharacter* OwnerCharacter = Item->GetOwnerCharacter();
	if (!IsValid(OwnerCharacter))
		return;
		
	Item->OnThrownFromInventory(OwnerCharacter);

	// If it's a grenade, don't clear the owner
	bool bNullOwner = Cast<ABaseGrenade>(Item) ? false : true;
	RemoveInventoryItem(Item, bNullOwner);	
}

ABaseItem* UInventoryComponent::EquipItemFromGroup_Index(const int32 GroupIndex, const int32 ItemCategoryIndex)
{
	for (int32 i = 0; i < ItemSelectionGroups.Num(); i++)
	{
		if (i == GroupIndex)
		{
			return EquipItemFromGroup_Internal(ItemSelectionGroups[i], ItemCategoryIndex);
		}
	}

	return nullptr;
}

ABaseItem* UInventoryComponent::EquipItemFromGroup_Name(const FName GroupName, const int32 ItemCategoryIndex)
{
	for (int32 i = 0; i < ItemSelectionGroups.Num(); i++)
	{
		if (ItemSelectionGroups[i].ItemGroupName == GroupName)
		{
			return EquipItemFromGroup_Internal(ItemSelectionGroups[i], ItemCategoryIndex);
		}
	}

	return nullptr;
}

ABaseItem* UInventoryComponent::EquipItemOfType(const EItemCategory ItemCategory, const bool bInstant)
{
	ABaseItem* ItemToEquip = GetInventoryItemOfType(ItemCategory);
	PutItemInHands(ItemToEquip, bInstant);
	return ItemToEquip;
}

ABaseItem* UInventoryComponent::EquipItemFromGroup_Internal(const FItemSelectionGroup& ItemGroup, const int32 ItemIndex)
{
	if (ItemIndex > -1)
	{
		if (ItemGroup.Items.IsValidIndex(ItemIndex))
			return EquipItemOfClass(ItemGroup.Items[ItemIndex]);
		
		#if !UE_BUILD_SHIPPING
		ULog::Error("Failed to equip item at index (" + FString::FromInt(ItemIndex) + ") from group (" + ItemGroup.ItemGroupName.ToString() + "). Index " + FString::FromInt(ItemIndex) + " does not exist.");
		#endif

		return nullptr;
	}

	// Try to equip the next item in the group if we've already got something from this group equipped
	if (!GetEquippedItem())
		return EquipItemOfType(ItemGroup.ItemCategory);

	int32 DesiredIndex = ItemGroup.Items.IndexOfByKey(GetEquippedItem()->GetClass()) + 1;
	if (DesiredIndex > ItemGroup.Items.Num() - 1)
		DesiredIndex = 0;

	if (ItemGroup.Items.IsValidIndex(DesiredIndex))
		return EquipItemOfClass(ItemGroup.Items[DesiredIndex]);

	return EquipItemOfType(ItemGroup.ItemCategory);
}

ABaseItem* UInventoryComponent::EquipItemOfClass(UClass* ClassType, bool bInstant)
{
	if (ClassType == ABaseMagazineWeapon::StaticClass())
	{
		if (LastEquippedWeapon)
		{
			PutItemInHands(LastEquippedWeapon, bInstant);
			return LastEquippedItem;
		}
	}

	ABaseItem* ItemToEquip = GetInventoryItemOfClass(ClassType);
	PutItemInHands(ItemToEquip, bInstant);
	return ItemToEquip;
}

bool UInventoryComponent::IsEquippingItemOfType(EItemCategory ItemCategory)
{
	return LatestItemChangeRequest.ToItem && LatestItemChangeRequest.ToItem->ContainsItemCategory(ItemCategory)  && !LatestItemChangeRequest.bIsComplete;
}

bool UInventoryComponent::IsEquippingItemOfClass(UClass* ClassType)
{
	return LatestItemChangeRequest.ToItem &&LatestItemChangeRequest.ToItem->IsA(ClassType) && !LatestItemChangeRequest.bIsComplete;
}

bool UInventoryComponent::IsEquippingItem()
{
	return LatestItemChangeRequest.ToItem != nullptr && !LatestItemChangeRequest.bIsComplete;
}

bool UInventoryComponent::IsEquippingSpecificItem(ABaseItem* Item)
{
	return LatestItemChangeRequest.ToItem == Item  && !LatestItemChangeRequest.bIsComplete;
}

void UInventoryComponent::SetLastEquippedItem(ABaseItem* Item)
{
	if (!Item)
		return;

	if (Item->bDeployable)
		return;

	if (Cast<AChemlight>(Item))
		return;

	if (Cast<AZipcuffs>(Item))
		return;

#if !UE_BUILD_SHIPPING
	const FString ServerClientStr = GetNetMode() == NM_Client ? "[Client]" : "[Server]";
	V_LOGM(LogReadyOrNot, "%s %s Last Equipped Item: %s", *ServerClientStr, *GetName(), *Item->GetName());
#endif

	LastEquippedItem = Item;
	// SetLastEquippedItemWheel(Item);
}

PRAGMA_DISABLE_OPTIMIZATION

void UInventoryComponent::SetLastEquippedItemWheel(ABaseItem* Item)
{
	if (!Item || Item == LastEquippedItemWheel)
		return;

	if(LastEquippedItemWheel == nullptr)
	{
		LastEquippedItemWheel = Item;
	}	
	
	if (Item->ContainsItemCategory(EItemCategory::IC_TacticalDevice) ||
		Item->ContainsItemCategory(EItemCategory::IC_Grenade) ||
		Item->ContainsItemCategory(EItemCategory::IC_Zipcuffs) ||
		Item->ContainsItemCategory(EItemCategory::IC_Multitool) ||
		Item->ContainsItemCategory(EItemCategory::IC_Detonator))
	{
		
		LastEquippedItemWheel = Item;
	}
}


void UInventoryComponent::EquipLastEquippedItem(bool bInstant, bool bForce)
{
	PutItemInHands(LastEquippedItem, bInstant, bForce);
}

void UInventoryComponent::EquipLastEquippedItemWheel(bool bInstant, bool bForce)
{
	if (LastEquippedItemWheel == nullptr)
	{
		LastEquippedItemWheel = GetInventoryItemOfType(EItemCategory::IC_Zipcuffs);
	}

	PutItemInHands(LastEquippedItemWheel, bInstant, bForce);
}

void UInventoryComponent::EquipLastEquippedWeapon(bool bInstant, bool bForce)
{
	PutItemInHands(LastEquippedWeapon, bInstant, bForce);
}

PRAGMA_ENABLE_OPTIMIZATION

bool UInventoryComponent::IsEquippedWithShield( const ABaseItem* Item) const
{
	ABallisticsShield* Shield = GetEquippedItem<ABallisticsShield>();
	if (Shield)
	{
		if (Shield->PistolEquippedWithShield == Item)
			return true;
	}
	return false;
}
