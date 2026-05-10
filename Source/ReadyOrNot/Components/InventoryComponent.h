// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "Actors/BaseArmour.h"
#include "Actors/BaseMagazineWeapon.h"
#include "Actors/Items/Taser.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"

USTRUCT(BlueprintType)
struct FItemChangeRequest
{
	GENERATED_USTRUCT_BODY()
	
	UPROPERTY()
	FGuid ChangeId;
	
	UPROPERTY()
	ABaseItem* FromItem;

	UPROPERTY()
	ABaseItem* ToItem;

	UPROPERTY()
	bool bInstant;

	UPROPERTY()
	bool bNoDraw;

	UPROPERTY(NotReplicated)
	bool bIsComplete;
	
	FItemChangeRequest()
	{
		ChangeId = FGuid::NewGuid();
		FromItem = nullptr;
		ToItem = nullptr;
		bInstant = false;
		bNoDraw = false;
		bIsComplete = false;
	}
	
	FORCEINLINE bool operator==(const FItemChangeRequest& Other) const
	{
		return ChangeId == Other.ChangeId;
	}

	FORCEINLINE bool operator!=(const FItemChangeRequest& Other) const
	{
		return ChangeId != Other.ChangeId;
	}
};

USTRUCT(BlueprintType)
struct FItemSelectionGroup
{
	GENERATED_BODY()

	// The name of this group, used to identify this specific group in gameplay or UI code
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName ItemGroupName = NAME_None;

	// The input action binding that will trigger the inventory component to equip an item from this group
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName InputActionName = NAME_None;

	// The default item category to use when trying to equip an item from this group
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EItemCategory ItemCategory = EItemCategory::IC_None;

	// (Optional) Any additional item categories to add to this item selection group
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<EItemCategory> AdditionalItemCategories;

	// The list of unique item classes in this item selection group
	UPROPERTY(BlueprintReadOnly)
	TArray<TSubclassOf<ABaseItem>> Items;

	// The currently selected item index
	UPROPERTY(BlueprintReadOnly)
	int32 ItemIndex = -1;

	// An icon representing this group, to be used with UI
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* ItemGroupIcon = nullptr;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class READYORNOT_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

	class AReadyOrNotCharacter* GetOwningCharacter() const;
	class APlayerCharacter* GetOwningPlayerCharacter() const;

	float TimeToTickItems = 5.0f;

protected:
	virtual void BeginPlay() override;

	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

	bool bHasBoundToSignificanceManager = false;
	UFUNCTION()
	void OnActorRelevancyChanged(AActor* Actor, bool bIsRelevant);

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	// Variable used to track whether or not we're waiting for inventory item spawns to replicate
	bool bPendingInventorySpawnReplication = false;

public:
	UFUNCTION(Client, Reliable)
	void Client_NotifyInventorySpawned();
	void Client_NotifyInventorySpawned_Implementation();
	
	UFUNCTION(Client, Reliable)
	void Client_NotifyInventoryItemsChanged();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyInventoryItemsChanged();

	UFUNCTION(Client, Reliable)
	void Client_NotifyInventoryItemsDestroyed();


	/*
	 * Delegates
	 */
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEquippedItemChanged);
	UPROPERTY(BlueprintAssignable, Category = "Equip")
	FOnEquippedItemChanged OnEquippedItemChanged;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FClientOnItemAddedToInventory, class ABaseItem*, Item);

	UPROPERTY(BlueprintAssignable)
	FClientOnItemAddedToInventory OnClientItemAddedToInventory;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemEquipped, class ABaseItem*, Item);
	UPROPERTY(BlueprintAssignable, Category = "Equipped Item Event")
	FOnItemEquipped OnItemAddedToInventory;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemRemoved, ABaseItem*, Item);
	UPROPERTY(BlueprintAssignable, Category = "Item Events")
	FOnItemRemoved OnItemRemovedFromInventory;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerItemChanged, ABaseItem*, Item);

	UPROPERTY(BlueprintAssignable, Category = Gameplay)
	FOnPlayerItemChanged OnItemEquipped;

	UPROPERTY(BlueprintAssignable, Category = Gameplay)
	FOnPlayerItemChanged OnItemHolstered;

	DECLARE_MULTICAST_DELEGATE(FOnInventoryItemsChanged)
	FOnInventoryItemsChanged OnInventoryItemsChanged;

protected:

	FTimerHandle TH_OnHolsterComplete;
	FTimerHandle TH_AttachEquippedItemDelay;

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_PlayTPHolster(FItemChangeRequest ItemChangeRequest);

	void PlayTPHolster(FItemChangeRequest ItemChangeRequest);

	UFUNCTION()
	void PlayTPDraw(FItemChangeRequest ItemChangeRequest);

	UFUNCTION()
	void OnNewItemChangeDrawComplete(ABaseItem* Item);

	public:
	UFUNCTION(Client, Reliable)
	void PlayLocalHolster(FItemChangeRequest ItemChangeRequest);
	UFUNCTION(Client, Reliable)
	void PlayLocalDraw(FItemChangeRequest ItemChangeRequest);

	UFUNCTION(BlueprintPure)
	ABaseItem* GetLastEquippedItem() const { return LastEquippedItem; }
	
	UFUNCTION(BlueprintPure)
	ABaseWeapon* GetLastEquippedWeapon() const { return LastEquippedWeapon; }
	
protected:
	void OnLocalHolsterComplete();

	void LocalAttachEquippedItemToHands(APlayerCharacter* Player, ABaseItem* Item);

	FTimerHandle TH_UpdateItemVisualizations;
	void UpdateItemVisualizations();
	
	void OnTPHolsterComplete();

	UFUNCTION()
	void OnRep_ItemChangeRequest();

	UPROPERTY(ReplicatedUsing=OnRep_ItemChangeRequest)
	FItemChangeRequest LatestItemChangeRequest;

	UPROPERTY()
	FItemChangeRequest LastReceivedItemChangeRequest;

	UPROPERTY()
	ABaseItem* QueuedItemSwap;
	
	UPROPERTY(Replicated, VisibleAnywhere, Category = Equip)
	ABaseItem* LastEquippedItem;
	UPROPERTY(Replicated, VisibleAnywhere, Category = Equip)
	ABaseWeapon* LastEquippedWeapon;
	UPROPERTY(Replicated, VisibleAnywhere, Category = Equip)
	ABaseItem* LastEquippedItemWheel;
	UPROPERTY(ReplicatedUsing=OnRep_InventoryItemsChanged, VisibleAnywhere, Category = Items)
	TArray<ABaseItem*> InventoryItems;
	UPROPERTY(ReplicatedUsing=OnRep_InventoryItemsChanged, VisibleAnywhere, Category = Items)
	TArray<ABaseItem*> RemovedInventoryItems;
	UPROPERTY(ReplicatedUsing=OnRep_SpawnedGear, BlueprintReadWrite, Category = Inventory)
	FSpawnedGear SpawnedGear;

	UFUNCTION()
	void OnRep_SpawnedGear();
	
	UPROPERTY(Replicated, Transient)
	FSavedLoadout LastEquippedLoadout;
	UPROPERTY(Replicated, BlueprintReadWrite, Category = Inventory)
	ABaseItem* SelectedDevice;

	TArray<EItemCategory> DisplayedBodyItemCategories = {EItemCategory::IC_LongTactical, EItemCategory::IC_Primary, EItemCategory::IC_Secondary};
	TArray<TSubclassOf<ABaseItem>> DisplayedBodyItemClass = { ABaseArmour::StaticClass(), ABaseMagazineWeapon::StaticClass() };
	TArray<TSubclassOf<ABaseItem>> HiddenBodyItemClass = { ATaser::StaticClass() };
	
public:
	UFUNCTION(BlueprintPure)
	ABaseItem* GetHolsteredItem() const;

	UFUNCTION(BlueprintPure)
	bool IsAnyItemAttachedToHands() const;
	UFUNCTION(BlueprintPure)
	bool IsAnyItemAttachedToBody() const;
	
	UFUNCTION(BlueprintCallable, Category = Gameplay)
	void DestroyAllEquippedItems();

	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation)
	void Server_AttemptEquipNewLoadout(FSavedLoadout Loadout);

	UFUNCTION(BlueprintPure)
	FORCEINLINE FSavedLoadout GetLastEquippedLoadout() const { return LastEquippedLoadout; }
	
	void SetLastEquippedLoadout(FSavedLoadout NewLastEquippedLoadout) { LastEquippedLoadout = NewLastEquippedLoadout; }

	void RefreshItemCategories();

	void DestroyRemovedItems();

	UFUNCTION(BlueprintCallable)
	bool Holster(ABaseItem* Item, bool bInstant = false);
	UFUNCTION(BlueprintCallable)
	bool HolsterEquippedItem(bool bInstant = false);
	
	UFUNCTION(BlueprintCallable)
	bool EquipHolsteredItem(bool bInstant = false);

	void SetSelectedDevice(ABaseItem* InSelectedDevice) { SelectedDevice = InSelectedDevice; }
	UFUNCTION(BlueprintCallable)
	bool PutItemInHands(ABaseItem* Item, bool bInstant = false, bool bForce = false);
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_ChangeEquippedItem(FItemChangeRequest ItemChangeRequest);
	UFUNCTION(BlueprintCallable)
	bool CanEquip(ABaseItem* Item);
	
	FSpawnedGear& GetSpawnedGear();
	
	template<typename ItemClass>
	TArray<ItemClass*> GetInventoryItemsOfClass(UClass* Class = ItemClass::StaticClass()) const;

	UFUNCTION(BlueprintPure)
	TArray<ABaseItem*> GetInventoryItemsOfType(EItemCategory ItemCategory) const;

	UFUNCTION(BlueprintPure, Category = Items)
	TArray<ABaseItem*> GetInventoryItems() const;
	UFUNCTION(BlueprintPure, Category = Items)
	TArray<ABaseItem*> GetRemovedInventoryItems() const;
	
	UFUNCTION(BlueprintPure, Category = Items)
	bool HasAnyInventoryItems() const;
	
	UFUNCTION(BlueprintPure, Category = Items)
	bool HasAnyInventoryItemsOfType(EItemCategory ItemCategory) const;
	
	UFUNCTION(BlueprintPure, Category = Items)
	bool HasAnyInventoryItemsOfClass(TSubclassOf<ABaseItem> ItemClass) const;
	
	template<typename T>
	bool HasAnyInventoryItemsOfClass() const;

	template<typename T>
	T* GetInventoryItemOfClass_Native(UClass* Class, bool bCanEquipCheck = true) const;
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Items)
	ABaseItem* GetInventoryItemOfClass(UClass* Class, bool bCanEquipCheck = true) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Items)
	ABaseItem* GetInventoryItemOfType(EItemCategory ItemCategory) const;
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Items)
	ABaseItem* GetInventoryItemOfClassType(EItemClass ItemClass) const;

	UFUNCTION(BlueprintPure, Category = Inventory)
	bool IsItemEquipped(EItemCategory ItemCategory) const;
	UFUNCTION(BlueprintPure, Category = Inventory)
	bool IsItemEquipped_Class(TSubclassOf<ABaseItem> ItemClass) const;
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Items)
	int32 CountInventoryItemType(EItemCategory ItemCategory) const;

	UFUNCTION()
	virtual void OnRep_InventoryItemsChanged();

	void ReattachAllGear();

	bool bPendingInventoryItemsChanged = false;

	UFUNCTION(BlueprintPure, Category = Gameplay)
	class ABaseArmour* GetArmour() const;

	UFUNCTION(BlueprintPure, Category = Gameplay)
	class AHeadwear* GetHeadArmour() const;
	
	UFUNCTION(BlueprintPure, Category = Gameplay)
	class AHeadwear* GetHeadwear() const;

	UFUNCTION(BlueprintPure, Category = Gameplay)
	bool IsWearingArmour() const;

	UFUNCTION(BlueprintPure, Category = Gameplay)
	bool IsWearingHeadArmour() const;

	UFUNCTION(BlueprintPure, Category = Gameplay)
	bool IsWearingAntiFlashGoggles() const;

	UFUNCTION(BlueprintPure, Category = Gameplay)
	bool IsWearingExplosiveVest() const;

	UFUNCTION(BlueprintPure)
	bool IsAnyBlockingAnimationPlaying() const;

	// Gets our currently equipped item
	UFUNCTION(BlueprintPure, Category = Gameplay)
	ABaseItem* GetEquippedItem() const;

	template<typename T>
	T* GetEquippedItem() const;

	UFUNCTION(BlueprintCallable)
	bool AddInventoryItem(ABaseItem* Item);
	UFUNCTION(BlueprintCallable)
	bool RemoveInventoryItem(ABaseItem* Item, bool bNullOwner = true);
	UFUNCTION(BlueprintCallable)
	bool DestroyInventoryItem(ABaseItem* Item);
	
	// used in premission
	void ManuallySetEquippedItem(ABaseItem* Item);

	void ClearEquippedItem();

	UFUNCTION(BlueprintCallable)
	void ThrowEquippedItem();
	UFUNCTION(BlueprintCallable)
	void ThrowAllWeapons();
	UFUNCTION(BlueprintCallable)
	void ThrowAllItems();
	
	UFUNCTION(NetMulticast, Reliable)
	void ThrowSpecificItem(ABaseItem* Item);

	UFUNCTION(BlueprintPure)
	FORCEINLINE bool HasInventoryItem(ABaseItem* Item) const { return InventoryItems.Contains(Item); }

	UFUNCTION(BlueprintCallable, DisplayName = "Equip Item From Group (Index)")
	ABaseItem* EquipItemFromGroup_Index(int32 GroupIndex, int32 ItemCategoryIndex = -1);
	UFUNCTION(BlueprintCallable, DisplayName = "Equip Item From Group (Name)")
	ABaseItem* EquipItemFromGroup_Name(FName GroupName, int32 ItemCategoryIndex = -1);

	UFUNCTION(BlueprintCallable)
	ABaseItem* EquipItemOfType(const EItemCategory ItemCategory, const bool bInstant = false);
	UFUNCTION(BlueprintCallable)
	ABaseItem* EquipItemOfClass(UClass* ClassType, bool bInstant = false);
	
	UFUNCTION(BlueprintPure)
	bool IsEquippingItemOfType(EItemCategory ItemCategory);
	UFUNCTION(BlueprintPure)
	bool IsEquippingItemOfClass(UClass* ClassType);
	UFUNCTION(BlueprintPure)
	bool IsEquippingItem();
	UFUNCTION(BlueprintPure)
	bool IsEquippingSpecificItem(ABaseItem* Item);

	void SetLastEquippedItem(ABaseItem* Item);
	void SetLastEquippedItemWheel(ABaseItem* Item);
	UFUNCTION(BlueprintCallable)
	void EquipLastEquippedItem(bool bInstant = false, bool bForce = false);
	UFUNCTION(BlueprintCallable)
	void EquipLastEquippedItemWheel(bool bInstant = false, bool bForce = false);
	UFUNCTION(BlueprintCallable)
	void EquipLastEquippedWeapon(bool bInstant = false, bool bForce = false);
	UFUNCTION(BlueprintPure)
	bool IsEquippedWithShield(const ABaseItem* Item) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Setup", meta = (TitleProperty = "ItemGroupName"))
	TArray<FItemSelectionGroup> ItemSelectionGroups;
	
private:
	ABaseItem* EquipItemFromGroup_Internal(const FItemSelectionGroup& ItemGroup, int32 ItemIndex = -1);
};

template <typename ItemClass>
TArray<ItemClass*> UInventoryComponent::GetInventoryItemsOfClass(UClass* Class) const
{
	TArray<ItemClass*> ItemsOfClass;
	
	for (int32 i = 0; i < InventoryItems.Num(); i++)
	{
		if (ItemClass* Item = Cast<ItemClass>(InventoryItems[i]))
		{
			if (Item->IsA(Class))
			{
				ItemsOfClass.Add(Item);
			}
		}
	}
	
	return ItemsOfClass;
}


template <typename T>
bool UInventoryComponent::HasAnyInventoryItemsOfClass() const
{
	return GetInventoryItemsOfClass<T>(T::StaticClass()).Num() > 0;
}

template <typename T>
T* UInventoryComponent::GetInventoryItemOfClass_Native(UClass* Class, const bool bCanEquipCheck) const
{
	return Cast<T>(GetInventoryItemOfClass(Class, bCanEquipCheck));
}

template <typename T>
T* UInventoryComponent::GetEquippedItem() const
{
	return Cast<T>(GetEquippedItem());
};



